/*******************************************************************************
 *   File: core.c
 *   Date: 2011/06/23
 *   Desc: type system
 *******************************************************************************
the core:
	convert ast to bytecode
	initializations, memory management

	emit:
	emit is a low level function like construct. (intrinsic)
	emit's type is the type of the first argument's.
	emit can have as parameters values and opcodes.
		as the first argument of emit we can pass a type or the struct keyword.
			if the first argument is a type, static cast can be done
				ex emit(float32, int32(2))
			if the first argument is struct in a declaration,
			the resulting type will match the declared type.
			size of variable must match the size generated by emit.
				ex complex a = emit(struct, f64(1), f64(-1));
	emit is used for libcalls, constructors, ..., optimizations
*******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "internal.h"

const struct tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, STR) {TYPE, SIZE, STR},
	#include "defs.inl"
	//~ {0}
};
const struct opc_inf opc_tbl[255] = {
	#define OPCDEF(NAME, CODE, SIZE, CHCK, DIFF, MNEM) {CODE, SIZE, CHCK, DIFF, MNEM},
	//~ #define OPCDEF(NAME, CODE, SIZE, CHCK, DIFF, MNEM) {CODE, SIZE, CHCK, DIFF, "`"#NAME"`"},
	#include "defs.inl"
	//~ {0}
};

/// Initialize runtime context; @see header
rtContext rtInit(void* mem, size_t size) {
	rtContext rt = paddptr(mem, rt_size);

	if (rt != mem) {
		size_t diff = (char*)rt - (char*) mem;
		//~ fatal("runtime memory pardded with %d bytes", (int)diff);
		size -= diff;
	}

	if (rt == NULL && size > sizeof(struct rtContextRec)) {
		rt = malloc(size);
		logif(rt == NULL, ERR_INTERNAL_ERROR);
	}

	if (rt != NULL) {
		memset(rt, 0, sizeof(struct rtContextRec));

		*(void**)&rt->api.ccBegin = ccBegin;
		*(void**)&rt->api.ccDefInt = ccDefInt;
		*(void**)&rt->api.ccDefFlt = ccDefFlt;
		*(void**)&rt->api.ccDefStr = ccDefStr;
		*(void**)&rt->api.ccAddType = ccAddType;
		*(void**)&rt->api.ccAddCall = ccAddCall;
		*(void**)&rt->api.ccAddCode = ccAddCode;
		*(void**)&rt->api.ccEnd = ccEnd;
		*(void**)&rt->api.getsym = getsym;
		*(void**)&rt->api.invoke = invoke;
		*(void**)&rt->api.rtAlloc = rtAlloc;

		*(size_t*)&rt->_size = size - sizeof(struct rtContextRec);
		rt->_end = rt->_mem + rt->_size;
		rt->_beg = rt->_mem;

		rt->logLevel = 7;
		//
		rt->foldConst = 1;
		rt->fastInstr = 1;
		rt->fastAssign = 0;
		rt->genGlobals = 1;

		logFILE(rt, stdout);
		rt->cc = NULL;
	}
	return rt;
}

/// Allocate, resize or free memory; @see rtContext.api.rtAlloc
void* rtAlloc(rtContext rt, void* ptr, size_t size, void dbg(rtContext rt, void* mem, size_t size, int used)) {
	/* memory manager
	 * using one linked list containing both used and unused memory chunks.
	 * The idea is when allocating a block of memory we always must to traverse the list of chunks.
	 * when freeing a block not. Because of this if a block is used prev points to the previos block.
	 * a block is free when prev is null.
	 .
	 :
	 +------+------+
	 | next        | next chunk (allocated or free)
	 | prev        | previous chunk (null for free chunks)
	 +------+------+
	 |             |
	 .             .
	 : ...         :
	 +-------------+
	 | next        |
	 | prev        |
	 +------+------+
	 |             |
	 .             .
	 : ...         :
	 +------+------+
	 :
	*/

	typedef struct memchunk {
		struct memchunk* prev;		// null for free chunks
		struct memchunk* next;		// next chunk
		char data[];				// here begins the user data
		//~ struct memchunk* free;		// TODO: next free chunk oredered by size
		//~ struct memchunk* free_skip;	// TODO: next free whitch 2x biger tahan this
	} *memchunk;

	const ptrdiff_t minAllocationSize = sizeof(struct memchunk);
	memchunk chunk = (memchunk)((char*)ptr - offsetOf(memchunk, data));
	size_t allocSize = padded(size + minAllocationSize, minAllocationSize);

	// memory manager is not initialized, initialize it first
	if (rt->vm.heap == NULL) {
		memchunk heap = paddptr(rt->_beg, minAllocationSize);
		memchunk last = paddptr(rt->_end - minAllocationSize, minAllocationSize);

		heap->next = last;
		heap->prev = NULL;

		last->next = NULL;
		last->prev = NULL;

		rt->vm.heap = heap;
	}

	// chop or free.
	if (ptr != NULL) {
		size_t chunksize = chunk->next ? ((char*)chunk->next - (char*)chunk) : 0;

		if ((unsigned char*)ptr < rt->_beg || (unsigned char*)ptr > rt->_end) {
			dieif((unsigned char*)ptr < rt->_beg, "invalid heap reference(%06x)", vmOffset(rt, ptr));
			dieif((unsigned char*)ptr > rt->_end, "invalid heap reference(%06x)", vmOffset(rt, ptr));
			return NULL;
		}

		if (1) { // extra check if ptr is in used list.
			memchunk find = rt->vm.heap;
			memchunk prev = find;
			while (find && find != chunk) {
				prev = find;
				find = find->next;
			}
			if (find != chunk || chunk->prev != prev) {
				dieif(find != chunk, "unallocated reference(%06x)", vmOffset(rt, ptr));
				dieif(chunk->prev != prev, "unallocated reference(%06x)", vmOffset(rt, ptr));
				return NULL;
			}
		}

		if (size == 0) {							// free
			memchunk next = chunk->next;
			memchunk prev = chunk->prev;

			// merge with next block if free
			if (next && next->prev == NULL) {
				next = next->next;
				chunk->next = next;
				if (next && next->prev != NULL) {
					next->prev = chunk;
				}
			}

			// merge with previos block if free
			if (prev && prev->prev == NULL) {
				chunk = prev;
				chunk->next = next;
				if (next && next->prev != NULL) {
					next->prev = chunk;
				}
			}

			// mark as unused.
			chunk->prev = NULL;
			chunk = NULL;
		}
		else if (allocSize < chunksize) {			// chop
			memchunk next = chunk->next;
			memchunk free = (memchunk)((char*)chunk + allocSize);

			// do not make a free unaligned block (realoc 161 to 160 bytes)
			if (((char*)next - (char*)free) > minAllocationSize) {
				chunk->next = free;
				free->next = next;
				free->prev = NULL;

				// merge with next block if free
				if (next->prev == NULL) {
					next = next->next;
					free->next = next;
					if (next && next->prev != NULL) {
						next->prev = free;
					}
				}
				else {
					next->prev = free;
				}
			}
		}
		else {										// grow: reallocation to a bigger chunk is not supported.
			error(rt, __FILE__, __LINE__, "can not grow allocated chunk (unimplemented).");
			chunk = NULL;
		}
	}
	// allocate.
	else if (size > 0) {
		memchunk prev = chunk = rt->vm.heap;

		while (chunk) {
			memchunk next = chunk->next;

			// check if block is free.
			if (chunk->prev == NULL && next) {
				size_t chunksize = (char*)next - (char*)chunk - allocSize;
				if (allocSize < chunksize) {
					ptrdiff_t diff = chunksize - allocSize;
					if (diff > minAllocationSize) {
						memchunk free = (memchunk)((char*)chunk + allocSize);
						chunk->next = free;
						free->prev = NULL;
						free->next = next;
					}
					chunk->prev = prev;
					break;
				}
			}
			prev = chunk;
			chunk = next;
		}
	}
	else {
		chunk = NULL;
		if (dbg != NULL) {
			memchunk mem;
			for (mem = rt->vm.heap; mem; mem = mem->next) {
				if (mem->next) {
					int size = (char*)mem->next - (char*)mem - sizeof(struct memchunk);
					dbg(rt, mem->data, size, mem->prev != NULL);
				}
			}
		}
	}

	return chunk ? chunk->data : NULL;
}

//#{ symbols: install and query
/// Begin a namespace; @see rtContext.api.ccBegin
symn ccBegin(rtContext rt, const char* cls) {
	symn result = NULL;
	if (rt->cc != NULL) {
		if (cls != NULL) {
			result = install(rt->cc, cls, ATTR_stat | ATTR_const | TYPE_rec, TYPE_vid, 0, rt->cc->type_vid, NULL);
		}
		if (cls == NULL || result) {
			enter(rt->cc, NULL);
		}
	}
	return result;
}

/// Close a namespace; @see header
void ccExtEnd(rtContext rt, symn cls, int mode) {
	if (cls != NULL) {
		symn fields = leave(rt->cc, cls, (mode & ATTR_stat) != 0);
		if (mode & 1) {
			fields->next = cls->flds;
		}
		cls->flds = fields;
	}
}
/// Close a namespace; @see rtContext.api.ccEnd
void ccEnd(rtContext rt, symn cls) {
	ccExtEnd(rt, cls, ATTR_stat);
}

/// Declare int constant; @see rtContext.api.ccDefInt
symn ccDefInt(rtContext rt, char* name, int64_t value) {
	if (!rt || !rt->cc || !name) {
		trace("%x, %s, %D", rt, name, value);
		return NULL;
	}
	name = mapstr(rt->cc, name, -1, -1);
	return install(rt->cc, name, TYPE_def, TYPE_int, 0, rt->cc->type_i32, intnode(rt->cc, value));
}
/// Declare float constant; @see rtContext.api.ccDefFlt
symn ccDefFlt(rtContext rt, char* name, double value) {
	if (!rt || !rt->cc || !name) {
		trace("%x, %s, %F", rt, name, value);
		return NULL;
	}
	name = mapstr(rt->cc, name, -1, -1);
	return install(rt->cc, name, TYPE_def, TYPE_flt, 0, rt->cc->type_f64, fltnode(rt->cc, value));
}
/// Declare string constant; @see rtContext.api.ccDefStr
symn ccDefStr(rtContext rt, char* name, char* value) {
	if (!rt || !rt->cc || !name) {
		trace("%x, %s, %s", rt, name, value);
		return NULL;
	}
	name = mapstr(rt->cc, name, -1, -1);
	if (value != NULL) {
		value = mapstr(rt->cc, value, -1, -1);
	}
	return install(rt->cc, name, TYPE_def, TYPE_str, 0, rt->cc->type_str, strnode(rt->cc, value));
}

/// Find symbol by name; @see header
symn ccFindSym(ccContext cc, symn in, char* name) {
	struct astNode ast;
	memset(&ast, 0, sizeof(struct astNode));
	ast.kind = TYPE_ref;
	ast.ref.name = name;
	ast.ref.hash = rehash(name, -1) % TBLS;
	return lookup(cc, in ? in->flds : cc->rt->vars, &ast, NULL, 1);
}

// TODO: to be removed
int ccSymValInt(symn sym, int* res) {
	struct astNode ast;
	if (sym && eval(&ast, sym->init)) {
		*res = (int)constint(&ast);
		return 1;
	}
	return 0;
}
// TODO: to be removed
int ccSymValFlt(symn sym, double* res) {
	struct astNode ast;
	if (sym && eval(&ast, sym->init)) {
		*res = constflt(&ast);
		return 1;
	}
	return 0;
}

/// Lookup symbol by offset; @see rtContext.api.mapsym
symn mapsym(rtContext rt, size_t offs, int callsOnly) {
	symn sym = NULL;
	dieif(offs > rt->_size, "invalid offset: %06x", offs);
	if (offs > rt->vm.px + px_size) {
		// local variable on stack ?
		return NULL;
	}
	for (sym = rt->vars; sym; sym = sym->gdef) {
		if (callsOnly && !sym->call) {
			continue;
		}
		if (offs >= sym->offs && offs < sym->offs + sym->size) {
			return sym;
		}
	}
	return NULL;
}

// TODO: to be removed
symn getsym(rtContext rt, void* offs) {
	size_t vmoffs = vmOffset(rt, offs);
	symn sym = mapsym(rt, vmoffs, 0);
	if (sym != NULL && sym->offs == vmoffs) {
		return sym;
	}
	return NULL;
}

char* getResStr(rtContext rt, size_t offs) {
	int i;
	char *str = getip(rt, offs);
	if (rt->cc == NULL) {
		return NULL;
	}
	for (i = 0; i < TBLS; i += 1) {
		list lst;
		for (lst = rt->cc->strt[i]; lst; lst = lst->next) {
			if (str == (char*)lst->data) {
				return str;
			}
		}
	}
	return NULL;
}

//#}

// install type system
static void install_type(ccContext cc, int mode) {
	symn type_rec, type_vid, type_bol, type_ptr = NULL, type_var = NULL;
	symn type_i08, type_i16, type_i32, type_i64;
	symn type_u08, type_u16, type_u32;
	symn type_f32, type_f64;
	symn type_obj, type_chr;

	type_rec = install(cc, "typename", ATTR_stat | ATTR_const | TYPE_rec, TYPE_ref,      0, NULL, NULL);

	// TODO: !cycle: typename is instance of typename
	type_rec->type = type_rec;

	type_vid = install(cc,     "void", ATTR_stat | ATTR_const | TYPE_rec, TYPE_vid,       0, type_rec, NULL);
	type_bol = install(cc,     "bool", ATTR_stat | ATTR_const | TYPE_rec, TYPE_bit, vm_size, type_rec, NULL);
	type_i08 = install(cc,     "int8", ATTR_stat | ATTR_const | TYPE_rec, TYPE_i32,       1, type_rec, NULL);
	type_i16 = install(cc,    "int16", ATTR_stat | ATTR_const | TYPE_rec, TYPE_i32,       2, type_rec, NULL);
	type_i32 = install(cc,    "int32", ATTR_stat | ATTR_const | TYPE_rec, TYPE_i32,       4, type_rec, NULL);
	type_i64 = install(cc,    "int64", ATTR_stat | ATTR_const | TYPE_rec, TYPE_i64,       8, type_rec, NULL);
	type_u08 = install(cc,    "uint8", ATTR_stat | ATTR_const | TYPE_rec, TYPE_u32,       1, type_rec, NULL);
	type_u16 = install(cc,   "uint16", ATTR_stat | ATTR_const | TYPE_rec, TYPE_u32,       2, type_rec, NULL);
	type_u32 = install(cc,   "uint32", ATTR_stat | ATTR_const | TYPE_rec, TYPE_u32,       4, type_rec, NULL);
	// type_ = install(cc,   "uint64", ATTR_stat | ATTR_const | TYPE_rec, TYPE_u64,       8, type_rec, NULL);
	type_f32 = install(cc,  "float32", ATTR_stat | ATTR_const | TYPE_rec, TYPE_f32,       4, type_rec, NULL);
	type_f64 = install(cc,  "float64", ATTR_stat | ATTR_const | TYPE_rec, TYPE_f64,       8, type_rec, NULL);

	type_chr = install(cc,     "char", ATTR_stat | ATTR_const | TYPE_rec, TYPE_u32,       1, type_rec, NULL);

	if (mode & install_ptr) {
		type_ptr = install(cc, "pointer", ATTR_stat | ATTR_const | TYPE_rec, TYPE_ref, 1 * vm_size, type_rec, NULL);
	}
	if (mode & install_var) {
		// TODO: variant should cast to TYPE_var
		type_var = install(cc, "variant", ATTR_stat | ATTR_const | TYPE_rec, TYPE_rec, 2 * vm_size, type_rec, NULL);
	}
	if (mode & install_obj) {
		type_obj = install(cc,  "object", ATTR_stat | ATTR_const | TYPE_rec, TYPE_ref, 2 * vm_size, type_rec, NULL);
	    //~ type = install(cc,"function", ATTR_stat | ATTR_const | TYPE_rec, TYPE_ref, 2 * vm_size, type_rec, NULL);
	}

	if (type_ptr != NULL) {
		cc->null_ref = install(cc, "null", ATTR_stat | ATTR_const | TYPE_ref, TYPE_any, vm_size, type_ptr, NULL);
	}

	type_vid->pfmt = NULL;
	type_bol->pfmt = "%d";
	type_i08->pfmt = "%d";
	type_i16->pfmt = "%d";
	type_i32->pfmt = "%d";
	type_i64->pfmt = "%D";
	type_u08->pfmt = "%u";
	type_u16->pfmt = "%u";
	type_u32->pfmt = "%u";
	// type_->pfmt = "%U";
	type_f32->pfmt = "%f";
	type_f64->pfmt = "%F";
	type_chr->pfmt = "'%c'";

	cc->type_rec = type_rec;
	cc->type_vid = type_vid;
	cc->type_bol = type_bol;
	cc->type_i32 = type_i32;
	cc->type_i64 = type_i64;
	cc->type_u32 = type_u32;
	cc->type_f32 = type_f32;
	cc->type_f64 = type_f64;
	cc->type_ptr = type_ptr;
	cc->type_var = type_var;
    (void)type_obj; //TODO: cc->type_obj = type_obj;

	// aliases.
	install(cc, "int",    ATTR_stat | ATTR_const | TYPE_def, TYPE_any, 0, type_i32, NULL);
	install(cc, "long",   ATTR_stat | ATTR_const | TYPE_def, TYPE_any, 0, type_i64, NULL);
	install(cc, "float",  ATTR_stat | ATTR_const | TYPE_def, TYPE_any, 0, type_f32, NULL);
	install(cc, "double", ATTR_stat | ATTR_const | TYPE_def, TYPE_any, 0, type_f64, NULL);

	install(cc, "true",   ATTR_stat | ATTR_const | TYPE_def, TYPE_any, 0, type_bol, intnode(cc, 1));
	install(cc, "false",  ATTR_stat | ATTR_const | TYPE_def, TYPE_any, 0, type_bol, intnode(cc, 0));

	//~ TODO: struct string: char[] { ... }, temporarily string is alias for char[*]
	cc->type_str = install(cc, "string", ATTR_stat | ATTR_const | TYPE_arr, TYPE_ref, vm_size, type_chr, NULL);
	cc->type_str->init = intnode(cc, -1); // hack: strings are static sized arrays with a length of -1.
	cc->type_str->pfmt = "\"%s\"";

	// Add variant and string to the runtime context, for printing purposes.
	cc->rt->type_str = cc->type_str;
	cc->rt->type_var = type_var;
}

// install emit operator
static void install_emit(ccContext cc, int mode) {
	rtContext rt = cc->rt;
	symn typ = NULL;
	symn type_v4f = NULL;

	// TODO: emit is a keyword ???
	cc->emit_opc = install(cc, "emit", EMIT_opc, TYPE_any, 0, NULL, NULL);

	if (cc->emit_opc && (mode & installEopc) == installEopc) {

		symn u32, i32, i64, f32, f64, v4f, v2d;

		ccBegin(rt, NULL);
		//~ cc->emit_ref = install(cc, "ref", ATTR_stat | ATTR_const | TYPE_rec, TYPE_ref, vm_size, cc->type_rec, NULL);
		install(cc, "ref", ATTR_stat | ATTR_const | TYPE_def, TYPE_def, 0, cc->type_ptr, NULL);

		u32 = install(cc, "u32", ATTR_stat | ATTR_const | TYPE_rec, TYPE_u32, 4, cc->type_rec, NULL);
		i32 = install(cc, "i32", ATTR_stat | ATTR_const | TYPE_rec, TYPE_i32, 4, cc->type_rec, NULL);
		i64 = install(cc, "i64", ATTR_stat | ATTR_const | TYPE_rec, TYPE_i64, 8, cc->type_rec, NULL);
		f32 = install(cc, "f32", ATTR_stat | ATTR_const | TYPE_rec, TYPE_f32, 4, cc->type_rec, NULL);
		f64 = install(cc, "f64", ATTR_stat | ATTR_const | TYPE_rec, TYPE_f64, 8, cc->type_rec, NULL);
		v4f = install(cc, "v4f", ATTR_stat | ATTR_const | TYPE_rec, TYPE_rec, 16, cc->type_rec, NULL);
		v2d = install(cc, "v2d", ATTR_stat | ATTR_const | TYPE_rec, TYPE_rec, 16, cc->type_rec, NULL);

		install(cc, "nop", EMIT_opc, TYPE_any, opc_nop, cc->type_vid, NULL);
		install(cc, "not", EMIT_opc, TYPE_any, opc_not, cc->type_bol, NULL);
		install(cc, "set", EMIT_opc, TYPE_any, opc_set1, cc->type_vid, intnode(cc, 1));
		install(cc, "join", EMIT_opc, TYPE_any, opc_sync, cc->type_vid, intnode(cc, 1));
		install(cc, "call", EMIT_opc, TYPE_any, opc_call, cc->type_vid, NULL);

		//~ install(cc, "pop1", EMIT_opc, 0, opc_drop, cc->type_vid, intnode(cc, 1));
		//~ install(cc, "set0", EMIT_opc, 0, opc_set1, cc->type_vid, intnode(cc, 0));
		//~ install(cc, "set1", EMIT_opc, 0, opc_set1, cc->type_vid, intnode(cc, 1));

		if ((typ = ccBegin(rt, "dupp")) != NULL) {
			install(cc, "x1", EMIT_opc, TYPE_any, opc_dup1, cc->type_i32, intnode(cc, 0));
			install(cc, "x2", EMIT_opc, TYPE_any, opc_dup2, cc->type_i64, intnode(cc, 0));
			install(cc, "x4", EMIT_opc, TYPE_any, opc_dup4, cc->type_vid, intnode(cc, 0));
			// dupplicate the second and or third 32bit element on stack
			//~ install(cc, "x1_1", EMIT_opc, TYPE_any, opc_dup1, cc->type_i32, intnode(cc, 1));
			//~ install(cc, "x1_2", EMIT_opc, TYPE_any, opc_dup1, cc->type_i32, intnode(cc, 2));
			ccEnd(cc->rt, typ);
		}
		if ((typ = ccBegin(rt, "load")) != NULL) {
			// load zero
			install(cc, "z32", EMIT_opc, TYPE_any, opc_ldz1, cc->type_vid, NULL);
			install(cc, "z64", EMIT_opc, TYPE_any, opc_ldz2, cc->type_vid, NULL);
			install(cc, "z128", EMIT_opc, TYPE_any, opc_ldz4, cc->type_vid, NULL);

			// load memory
			install(cc, "i8",   EMIT_opc, TYPE_any, opc_ldi1, cc->type_vid, NULL);
			install(cc, "i16",  EMIT_opc, TYPE_any, opc_ldi2, cc->type_vid, NULL);
			install(cc, "i32",  EMIT_opc, TYPE_any, opc_ldi4, cc->type_vid, NULL);
			install(cc, "i64",  EMIT_opc, TYPE_any, opc_ldi8, cc->type_vid, NULL);
			install(cc, "i128", EMIT_opc, TYPE_any, opc_ldiq, cc->type_vid, NULL);
			ccEnd(rt, typ);
		}
		if ((typ = ccBegin(rt, "store")) != NULL) {
			install(cc, "i8",   EMIT_opc, TYPE_any, opc_sti1, cc->type_vid, NULL);
			install(cc, "i16",  EMIT_opc, TYPE_any, opc_sti2, cc->type_vid, NULL);
			install(cc, "i32",  EMIT_opc, TYPE_any, opc_sti4, cc->type_vid, NULL);
			install(cc, "i64",  EMIT_opc, TYPE_any, opc_sti8, cc->type_vid, NULL);
			install(cc, "i128", EMIT_opc, TYPE_any, opc_stiq, cc->type_vid, NULL);
			ccEnd(rt, typ);
		}

		if ((typ = u32) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "cmt", EMIT_opc, TYPE_any, b32_cmt, cc->type_u32, NULL);
			install(cc, "and", EMIT_opc, TYPE_any, b32_and, cc->type_u32, NULL);
			install(cc,  "or", EMIT_opc, TYPE_any, b32_ior, cc->type_u32, NULL);
			install(cc, "xor", EMIT_opc, TYPE_any, b32_xor, cc->type_u32, NULL);
			install(cc, "shl", EMIT_opc, TYPE_any, b32_shl, cc->type_u32, NULL);
			install(cc, "shr", EMIT_opc, TYPE_any, b32_shr, cc->type_u32, NULL);
			install(cc, "sar", EMIT_opc, TYPE_any, b32_sar, cc->type_u32, NULL);

			install(cc, "mul", EMIT_opc, TYPE_any, u32_mul, cc->type_u32, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, u32_div, cc->type_u32, NULL);
			install(cc, "mod", EMIT_opc, TYPE_any, u32_mod, cc->type_u32, NULL);

			install(cc, "clt", EMIT_opc, TYPE_any, u32_clt, cc->type_bol, NULL);
			install(cc, "cgt", EMIT_opc, TYPE_any, u32_cgt, cc->type_bol, NULL);
			//~ install(cc, "to_i64", EMIT_opc, 0, u32_i64, cc->type_i64, NULL);
			ccEnd(rt, typ);
		}
		if ((typ = i32) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "cmt", EMIT_opc, TYPE_any, b32_cmt, cc->type_i32, NULL);
			install(cc, "neg", EMIT_opc, TYPE_any, i32_neg, cc->type_i32, NULL);
			install(cc, "add", EMIT_opc, TYPE_any, i32_add, cc->type_i32, NULL);
			install(cc, "sub", EMIT_opc, TYPE_any, i32_sub, cc->type_i32, NULL);
			install(cc, "mul", EMIT_opc, TYPE_any, i32_mul, cc->type_i32, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, i32_div, cc->type_i32, NULL);
			install(cc, "mod", EMIT_opc, TYPE_any, i32_mod, cc->type_i32, NULL);

			install(cc, "ceq", EMIT_opc, TYPE_any, i32_ceq, cc->type_bol, NULL);
			install(cc, "clt", EMIT_opc, TYPE_any, i32_clt, cc->type_bol, NULL);
			install(cc, "cgt", EMIT_opc, TYPE_any, i32_cgt, cc->type_bol, NULL);

			install(cc, "and", EMIT_opc, TYPE_any, b32_and, cc->type_i32, NULL);
			install(cc,  "or", EMIT_opc, TYPE_any, b32_ior, cc->type_i32, NULL);
			install(cc, "xor", EMIT_opc, TYPE_any, b32_xor, cc->type_i32, NULL);
			install(cc, "shl", EMIT_opc, TYPE_any, b32_shl, cc->type_i32, NULL);
			install(cc, "shr", EMIT_opc, TYPE_any, b32_sar, cc->type_i32, NULL);
			ccEnd(rt, typ);
		}
		if ((typ = i64) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "neg", EMIT_opc, TYPE_any, i64_neg, cc->type_i64, NULL);
			install(cc, "add", EMIT_opc, TYPE_any, i64_add, cc->type_i64, NULL);
			install(cc, "sub", EMIT_opc, TYPE_any, i64_sub, cc->type_i64, NULL);
			install(cc, "mul", EMIT_opc, TYPE_any, i64_mul, cc->type_i64, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, i64_div, cc->type_i64, NULL);
			install(cc, "mod", EMIT_opc, TYPE_any, i64_mod, cc->type_i64, NULL);
			install(cc, "ceq", EMIT_opc, TYPE_any, i64_ceq, cc->type_bol, NULL);
			install(cc, "clt", EMIT_opc, TYPE_any, i64_clt, cc->type_bol, NULL);
			install(cc, "cgt", EMIT_opc, TYPE_any, i64_cgt, cc->type_bol, NULL);
			ccEnd(rt, typ);
		}
		if ((typ = f32) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "neg", EMIT_opc, TYPE_any, f32_neg, cc->type_f32, NULL);
			install(cc, "add", EMIT_opc, TYPE_any, f32_add, cc->type_f32, NULL);
			install(cc, "sub", EMIT_opc, TYPE_any, f32_sub, cc->type_f32, NULL);
			install(cc, "mul", EMIT_opc, TYPE_any, f32_mul, cc->type_f32, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, f32_div, cc->type_f32, NULL);
			install(cc, "mod", EMIT_opc, TYPE_any, f32_mod, cc->type_f32, NULL);
			install(cc, "ceq", EMIT_opc, TYPE_any, f32_ceq, cc->type_bol, NULL);
			install(cc, "clt", EMIT_opc, TYPE_any, f32_clt, cc->type_bol, NULL);
			install(cc, "cgt", EMIT_opc, TYPE_any, f32_cgt, cc->type_bol, NULL);
			ccEnd(rt, typ);
		}
		if ((typ = f64) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "neg", EMIT_opc, TYPE_any, f64_neg, cc->type_f64, NULL);
			install(cc, "add", EMIT_opc, TYPE_any, f64_add, cc->type_f64, NULL);
			install(cc, "sub", EMIT_opc, TYPE_any, f64_sub, cc->type_f64, NULL);
			install(cc, "mul", EMIT_opc, TYPE_any, f64_mul, cc->type_f64, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, f64_div, cc->type_f64, NULL);
			install(cc, "mod", EMIT_opc, TYPE_any, f64_mod, cc->type_f64, NULL);
			install(cc, "ceq", EMIT_opc, TYPE_any, f64_ceq, cc->type_bol, NULL);
			install(cc, "clt", EMIT_opc, TYPE_any, f64_clt, cc->type_bol, NULL);
			install(cc, "cgt", EMIT_opc, TYPE_any, f64_cgt, cc->type_bol, NULL);
			ccEnd(rt, typ);
		}

		if ((typ = v4f) != NULL) {
			type_v4f = typ;
			ccBegin(rt, NULL);
			install(cc, "neg", EMIT_opc, TYPE_any, v4f_neg, type_v4f, NULL);
			install(cc, "add", EMIT_opc, TYPE_any, v4f_add, type_v4f, NULL);
			install(cc, "sub", EMIT_opc, TYPE_any, v4f_sub, type_v4f, NULL);
			install(cc, "mul", EMIT_opc, TYPE_any, v4f_mul, type_v4f, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, v4f_div, type_v4f, NULL);
			install(cc, "equ", EMIT_opc, TYPE_any, v4f_ceq, cc->type_bol, NULL);
			install(cc, "min", EMIT_opc, TYPE_any, v4f_min, type_v4f, NULL);
			install(cc, "max", EMIT_opc, TYPE_any, v4f_max, type_v4f, NULL);
			install(cc, "dp3", EMIT_opc, TYPE_any, v4f_dp3, cc->type_f32, NULL);
			install(cc, "dp4", EMIT_opc, TYPE_any, v4f_dp4, cc->type_f32, NULL);
			install(cc, "dph", EMIT_opc, TYPE_any, v4f_dph, cc->type_f32, NULL);
			ccEnd(rt, typ);
		}
		if ((typ = v2d) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "neg", EMIT_opc, TYPE_any, v2d_neg, typ, NULL);
			install(cc, "add", EMIT_opc, TYPE_any, v2d_add, typ, NULL);
			install(cc, "sub", EMIT_opc, TYPE_any, v2d_sub, typ, NULL);
			install(cc, "mul", EMIT_opc, TYPE_any, v2d_mul, typ, NULL);
			install(cc, "div", EMIT_opc, TYPE_any, v2d_div, typ, NULL);
			install(cc, "equ", EMIT_opc, TYPE_any, v2d_ceq, cc->type_bol, NULL);
			install(cc, "min", EMIT_opc, TYPE_any, v2d_min, typ, NULL);
			install(cc, "max", EMIT_opc, TYPE_any, v2d_max, typ, NULL);
			ccEnd(rt, typ);
		}

		if ((mode & installEswz) == installEswz) {
			unsigned i;
			struct {
				char* name;
				astn node;
			} swz[256];
			for (i = 0; i < lengthOf(swz); i += 1) {
				if (rt->_end - rt->_beg < 5) {
					fatal(ERR_MEMORY_OVERRUN);
					return;
				}
				rt->_beg[0] = (unsigned char) "xyzw"[(i >> 0) & 3];
				rt->_beg[1] = (unsigned char) "xyzw"[(i >> 2) & 3];
				rt->_beg[2] = (unsigned char) "xyzw"[(i >> 4) & 3];
				rt->_beg[3] = (unsigned char) "xyzw"[(i >> 6) & 3];
				rt->_beg[4] = 0;

				swz[i].name = mapstr(cc, (char*)rt->_beg, -1, -1);
				swz[i].node = intnode(cc, i);
			}
			if ((typ = install(cc, "swz", ATTR_stat | ATTR_const | TYPE_rec, TYPE_any, 0, NULL, NULL))) {
				ccBegin(rt, NULL);
				for (i = 0; i < 256; i += 1) {
					install(cc, swz[i].name, EMIT_opc, TYPE_any, p4x_swz, type_v4f, swz[i].node);
				}
				ccEnd(rt, typ);
			}
		}
		ccEnd(rt, cc->emit_opc);
	}
}

/// private dummy on exit function.
static int haltDummy(libcContext args) {
	(void)args;
	return 0;
}
static int typenameGetName(libcContext args) {
	symn sym = argsym(args, 0);
	if (sym == NULL) {
		return 1;
	}
	reti32(args, vmOffset(args->rt, sym->name));
	return 0;
}
static int typenameGetFile(libcContext args) {
	symn sym = argsym(args, 0);
	if (sym == NULL) {
		return 1;
	}
	reti32(args, vmOffset(args->rt, sym->file));
	return 0;
}
static int typenameGetBase(libcContext args) {
	symn sym = argsym(args, 0);
	if (sym == NULL) {
		return 1;
	}
	reti32(args, vmOffset(args->rt, sym->type));
	return 0;
}

/**
 * @brief Instal type system.
 * @param runtime context.
 + @param level warning level.
 + @param file additional file extending module
 */
static int install_base(rtContext rt, int mode) {
	int error = 0;
	ccContext cc = rt->cc;

	// 4 reflection
	if (cc->type_rec && (mode & install_var)) {
		symn arg = NULL;
		enter(cc, NULL);
		if ((arg = install(cc, "line", ATTR_const | TYPE_ref, TYPE_any, vm_size, cc->type_i32, NULL))) {
			arg->offs = offsetOf(symn, line);
		}
		else {
			error = 1;
		}

		if ((arg = install(cc, "size", ATTR_const | TYPE_ref, TYPE_any, vm_size, cc->type_i32, NULL))) {
			arg->offs = offsetOf(symn, size);
		}
		else {
			error = 1;
		}

		if ((arg = install(cc, "offset", ATTR_const | TYPE_ref, TYPE_any, vm_size, cc->type_i32, NULL))) {
			arg->offs = offsetOf(symn, offs);
			arg->pfmt = "@%06x";
		}
		else {
			error = 1;
		}

		// HACK: `operator(typename type).file = typename.file(type);`
		if ((arg = ccAddCall(rt, typenameGetFile, NULL, "string file;"))) {
			rt->cc->libc->chk += 1;
			rt->cc->libc->pop += 1;
			arg->stat = 0;
			arg->memb = 1;
		}
		else {
			error = 1;
		}

		// HACK: `operator(typename type).name = typename.name(type);`
		if ((arg = ccAddCall(rt, typenameGetName, NULL, "string name;"))) {
			rt->cc->libc->chk += 1;
			rt->cc->libc->pop += 1;
			arg->stat = 0;
			arg->memb = 1;
		}
		else {
			error = 1;
		}

		error = error || !ccAddCall(rt, typenameGetBase, NULL, "typename base(typename type);");

		ccExtEnd(rt, cc->type_rec, 0);

		/* TODO: more 4 reflection

		error = error || !ccAddCall(rt, typeFunction, (void*)typeOpGetFile, "variant setValue(typename field, variant value)");
		error = error || !ccAddCall(rt, typeFunction, (void*)typeOpGetFile, "variant getValue(typename field)");

		install(cc, "typename lookup(variant &obj, int options, string name, variant args...)");
		install(cc, "variant invoke(variant &obj, int options, string name, variant args...)");
		install(cc, "bool canassign(typename toType, variant value, bool canCast)");
		install(cc, "bool instanceof(typename &type, variant obj)");
		//~ */
	}
	return error;
}

/// Initialze compiler context; @see header
ccContext ccInit(rtContext rt, int mode, int onHalt(libcContext)) {
	ccContext cc;

	dieif(rt->cc != NULL, "Compiler context already initialzed.");
	dieif(rt->_beg != rt->_mem, "Compiler initialization failed.");
	dieif(rt->_end != rt->_mem + rt->_size, "Compiler initialization failed.");

	cc = (ccContext)(rt->_end - sizeof(struct ccContextRec));
	rt->_end -= sizeof(struct ccContextRec);
	rt->_beg += 1;	// HACK: make first symbol start not at null.

	if (rt->_end < rt->_beg) {
		fatal(ERR_MEMORY_OVERRUN);
		return NULL;
	}

	memset(rt->_end, 0, sizeof(struct ccContextRec));

	cc->rt = rt;
	rt->cc = cc;

	rt->vars = NULL;
	cc->defs = NULL;
	cc->gdef = NULL;

	cc->_chr = -1;

	cc->fin._ptr = 0;
	cc->fin._cnt = 0;
	cc->fin._fin = -1;

	cc->root = newnode(cc, STMT_beg);

	install_type(cc, mode);
	install_emit(cc, mode);

	ccAddCall(rt, onHalt ? onHalt : haltDummy, NULL, "void Halt(int Code);");

	cc->root->type = cc->type_vid;
	cc->root->cst2 = TYPE_any;

	// install a void arg for functions with no arguments
	if (cc->type_vid && (cc->void_tag = newnode(cc, TYPE_ref))) {
		cc->void_tag->next = NULL;
		cc->void_tag->ref.name = "";

		enter(cc, NULL);
		declare(cc, TYPE_ref, cc->void_tag, cc->type_vid);
		leave(cc, NULL, 0);
	}

	if (cc->emit_opc && (cc->emit_tag = newnode(cc, TYPE_ref))) {
		cc->emit_tag->ref.link = cc->emit_opc;
		cc->emit_tag->ref.name = "emit";
		cc->emit_tag->ref.hash = -1;
	}

	install_base(rt, mode);

	return cc;
}

// arrayBuffer
int initBuff(struct arrBuffer* buff, int initsize, int elemsize) {
	buff->cnt = 0;
	buff->ptr = 0;
	buff->esz = elemsize;
	buff->cap = initsize * elemsize;
	return setBuff(buff, initsize, NULL) != NULL;
}
void* setBuff(struct arrBuffer* buff, int idx, void* data) {
	void* newPtr;
	int pos = idx * buff->esz;

	//~ void* ptr = getBuff(buff, idx);
	if (pos >= buff->cap) {
		//~ trace("resizing setBuff(%d / %d)\n", idx, buff->cnt);
		buff->cap <<= 1;
		if (pos > 2 * buff->cap) {
			buff->cap = pos << 1;
		}
		newPtr = realloc(buff->ptr, (size_t)buff->cap);
		if (newPtr != NULL) {
			buff->ptr = newPtr;
		}
		else {
			freeBuff(buff);
		}
	}

	if (buff->cnt >= idx) {
		buff->cnt = idx + 1;
	}

	if (buff->ptr && data) {
		memcpy(buff->ptr + pos, data, (size_t) buff->esz);
	}

	return buff->ptr ? buff->ptr + pos : NULL;
}
void* insBuff(struct arrBuffer* buff, int idx, void* data) {
	void* newPtr;
	int pos = idx * buff->esz;
	int newCap = buff->cnt * buff->esz;
	if (newCap < pos) {
		newCap = pos;
	}

	// resize buffer
	if (newCap >= buff->cap) {
		if (newCap > 2 * buff->cap) {
			buff->cap = newCap;
		}
		buff->cap *= 2;
		newPtr = realloc(buff->ptr, (size_t) buff->cap);
		if (newPtr != NULL) {
			buff->ptr = newPtr;
		}
		else {
			freeBuff(buff);
		}
	}

	if (idx < buff->cnt) {
		memmove(buff->ptr + pos + buff->esz, buff->ptr + pos, (size_t) (buff->esz * (buff->cnt - idx)));
		idx = buff->cnt;
	}

	if (buff->cnt >= idx) {
		buff->cnt = idx + 1;
	}

	if (buff->ptr && data) {
		memcpy(buff->ptr + pos, data, (size_t) buff->esz);
	}

	return buff->ptr ? buff->ptr + pos : NULL;
}
void* getBuff(struct arrBuffer* buff, int idx) {
	int pos = idx * buff->esz;

	if (pos >= buff->cap)
		return NULL;

	return buff->ptr ? buff->ptr + pos : NULL;
}
void freeBuff(struct arrBuffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->cap = 0;
	buff->esz = 0;
}

dbgInfo getDbgStatement(rtContext rt, char* file, int line) {
	if (rt->dbg != NULL) {
		int i;
		for (i = 0; i < rt->dbg->statements.cnt; ++i) {
			dbgInfo result = getBuff(&rt->dbg->statements, i);
			if (result->file && strcmp(file, result->file) == 0) {
				if (line == result->line) {
					return result;
				}
			}
		}
	}
	return NULL;
}
dbgInfo mapDbgStatement(rtContext rt, size_t position) {
	if (rt->dbg != NULL) {
		int i;
		dbgInfo result = (dbgInfo)rt->dbg->statements.ptr;
		for (i = 0; i < rt->dbg->statements.cnt; ++i) {
			if (position >= result->start) {
				if (position < result->end) {
					return result;
				}
			}
			result++;
		}
	}
	return NULL;
}
dbgInfo addDbgStatement(rtContext rt, size_t start, size_t end, astn tag) {
	dbgInfo result = NULL;
	if (rt->dbg != NULL && start < end) {
		int i = 0;
		for ( ; i < rt->dbg->statements.cnt; ++i) {
			result = getBuff(&rt->dbg->statements, i);
			if (start <= result->start) {
				break;
			}
		}

		if (result == NULL || start != result->start) {
			result = insBuff(&rt->dbg->statements, i, NULL);
		}

		if (result != NULL) {
			memset(result, 0, rt->dbg->statements.esz);
			if (tag != NULL) {
				result->stmt = tag;
				result->file = tag->file;
				result->line = tag->line;
			}
			result->start = start;
			result->end = end;
		}
	}
	return result;
}

dbgInfo mapDbgFunction(rtContext rt, size_t position) {
	if (rt->dbg != NULL) {
		int i;
		dbgInfo result = (dbgInfo)rt->dbg->functions.ptr;
		for (i = 0; i < rt->dbg->functions.cnt; ++i) {
			if (position >= result->start) {
				if (position < result->end) {
					return result;
				}
			}
			result++;
		}
	}
	return NULL;
}
dbgInfo addDbgFunction(rtContext rt, symn fun) {
	dbgInfo result = NULL;
	if (rt->dbg != NULL && fun != NULL) {
		int i;
		for (i = 0; i < rt->dbg->functions.cnt; ++i) {
			result = getBuff(&rt->dbg->functions, i);
			if (fun->offs <= result->start) {
				break;
			}
		}

		if (result == NULL || fun->offs != result->start) {
			result = insBuff(&rt->dbg->functions, i, NULL);
		}

		if (result != NULL) {
			memset(result, 0, rt->dbg->functions.esz);
			result->decl = fun;
			result->file = fun->file;
			result->line = fun->line;
			result->start = fun->offs;
			result->end = fun->offs + fun->size;
		}
	}
	return result;
}
