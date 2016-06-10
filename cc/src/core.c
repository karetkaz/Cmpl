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

/// Allocate, resize or free memory; @see rtContext.api.rtAlloc
void *rtAlloc(rtContext rt, void* ptr, size_t size, void dbg(rtContext rt, void* mem, size_t size, char *kind)) {
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
		//~ struct memchunk* free;		// TODO: next free chunk ordered by size
		//~ struct memchunk* free_skip;	// TODO: next free which is 2x bigger than this
	} *memchunk;

	const ptrdiff_t minAllocationSize = sizeof(struct memchunk);
	size_t allocSize = padded(size + minAllocationSize, minAllocationSize);
	memchunk chunk = (memchunk)((char*)ptr - offsetOf(memchunk, data));

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
		size_t chunkSize = chunk->next ? ((char*)chunk->next - (char*)chunk) : 0;

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

			// merge with previous block if free
			if (prev && prev->prev == NULL) {
				chunk = prev;
				chunk->next = next;
				if (next && next->prev != NULL) {
					next->prev = chunk;
				}
			}

			// mark chunk as free.
			chunk->prev = NULL;
			chunk = NULL;
		}
		else if (allocSize < chunkSize) {			// chop
			memchunk next = chunk->next;
			memchunk free = (memchunk)((char*)chunk + allocSize);

			// do not make a free unaligned block (realloc 161 to 160 bytes)
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
		else { // grow: reallocation to a bigger chunk is not supported.
			error(rt, __FILE__, __LINE__, "can not grow allocated chunk (unimplemented).");
			chunk = NULL;
		}
	}
	// allocate.
	else if (size > 0) {
		memchunk prev = chunk = rt->vm.heap;

		while (chunk != NULL) {
			memchunk next = chunk->next;

			// check if block is free.
			if (chunk->prev == NULL && next != NULL) {
				size_t chunkSize = (char*)next - (char*)chunk - allocSize;
				if (allocSize < chunkSize) {
					ptrdiff_t diff = chunkSize - allocSize;
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
					dbg(rt, mem->data, size, mem->prev != NULL ? "used" : "free");
				}
			}
		}
	}

	return chunk ? chunk->data : NULL;
}

static void ccEndApi(rtContext rt, symn cls) {
	ccEnd(rt, cls, ATTR_stat);
}

static void *rtAllocApi(rtContext rt, void* ptr, size_t size) {
	return rtAlloc(rt, ptr, size, NULL);
}

static symn rtFindSymApi(rtContext rt, void* offs) {
	size_t vmOffs = vmOffset(rt, offs);
	symn sym = rtFindSym(rt, vmOffs, 0);
	if (sym != NULL && sym->offs == vmOffs) {
		return sym;
	}
	return NULL;
}

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
		*(void**)&rt->api.ccEnd = ccEndApi;

		*(void**)&rt->api.ccDefInt = ccDefInt;
		*(void**)&rt->api.ccDefFlt = ccDefFlt;
		*(void**)&rt->api.ccDefStr = ccDefStr;

		*(void**)&rt->api.ccDefType = ccDefType;
		*(void**)&rt->api.ccDefCall = ccDefCall;
		*(void**)&rt->api.ccAddUnit = ccAddUnit;

		*(void**)&rt->api.invoke = invoke;
		*(void**)&rt->api.rtAlloc = rtAllocApi;
		*(void**)&rt->api.rtFindSym = rtFindSymApi;

		*(size_t*)&rt->_size = size - sizeof(struct rtContextRec);
		rt->_end = rt->_mem + rt->_size;
		rt->_beg = rt->_mem;

		rt->freeMem = mem == NULL;
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

void rtClose(rtContext rt) {
	// close log file
	logfile(rt, NULL, 0);

	// release debugger memory
	if (rt->dbg != NULL) {
		freeBuff(&rt->dbg->functions);
		freeBuff(&rt->dbg->statements);
	}
	// release memory
	if (rt->freeMem) {
		free(rt);
	}
}

/// private dummy on exit function.
static vmError haltDummy(libcContext args) {
	(void)args;
	return noError;
}
static vmError typenameGetName(libcContext args) {
	symn sym = argsym(args, 0);
	if (sym == NULL) {
		return executionAborted;
	}
	reti32(args, vmOffset(args->rt, sym->name));
	return noError;
}
static vmError typenameGetFile(libcContext args) {
	symn sym = argsym(args, 0);
	if (sym == NULL) {
		return executionAborted;
	}
	reti32(args, vmOffset(args->rt, sym->file));
	return noError;
}
static vmError typenameGetBase(libcContext args) {
	symn sym = argsym(args, 0);
	if (sym == NULL) {
		return executionAborted;
	}
	reti32(args, vmOffset(args->rt, sym->type));
	return noError;
}

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

	// TODO: emit is a keyword ???
	cc->emit_opc = install(cc, "emit", EMIT_opc, TYPE_any, 0, NULL, NULL);

	if (cc->emit_opc && (mode & installEopc) == installEopc) {
		symn opc, type_p4x;
		symn type_vid = cc->type_vid;
		symn type_bol = cc->type_bol;
		symn type_u32 = cc->type_u32;
		symn type_i32 = cc->type_i32;
		symn type_f32 = cc->type_f32;
		symn type_i64 = cc->type_i64;
		symn type_f64 = cc->type_f64;

		ccBegin(rt, NULL);
		install(cc, "nop", EMIT_opc, TYPE_any, opc_nop, type_vid, NULL);
		install(cc, "not", EMIT_opc, TYPE_any, opc_not, type_bol, NULL);
		install(cc, "set", EMIT_opc, TYPE_any, opc_set1, type_vid, intnode(cc, 1));
		install(cc, "join", EMIT_opc, TYPE_any, opc_sync, type_vid, intnode(cc, 1));
		install(cc, "call", EMIT_opc, TYPE_any, opc_call, type_vid, NULL);

		type_p4x = install(cc, "p4x", ATTR_stat | ATTR_const | TYPE_rec, TYPE_rec, 16, cc->type_rec, NULL);

		if ((opc = ccBegin(rt, "dupp")) != NULL) {
			install(cc, "x1", EMIT_opc, TYPE_any, opc_dup1, type_i32, intnode(cc, 0));
			install(cc, "x2", EMIT_opc, TYPE_any, opc_dup2, type_i64, intnode(cc, 0));
			install(cc, "x4", EMIT_opc, TYPE_any, opc_dup4, type_p4x, intnode(cc, 0));
			// duplicate the second and or third element on stack
			//~ install(cc, "x1_1", EMIT_opc, TYPE_any, opc_dup1, type_i32, intnode(cc, 1));
			//~ install(cc, "x1_2", EMIT_opc, TYPE_any, opc_dup1, type_i32, intnode(cc, 2));
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "load")) != NULL) {
			// load zero
			install(cc, "z32", EMIT_opc, TYPE_any, opc_ldz1, type_i32, NULL);
			install(cc, "z64", EMIT_opc, TYPE_any, opc_ldz2, type_i64, NULL);
			install(cc, "z128", EMIT_opc, TYPE_any, opc_ldz4, type_p4x, NULL);

			// load memory
			install(cc, "i8",   EMIT_opc, TYPE_any, opc_ldi1, type_i32, NULL);
			install(cc, "i16",  EMIT_opc, TYPE_any, opc_ldi2, type_i32, NULL);
			install(cc, "i32",  EMIT_opc, TYPE_any, opc_ldi4, type_i32, NULL);
			install(cc, "i64",  EMIT_opc, TYPE_any, opc_ldi8, type_i64, NULL);
			install(cc, "i128", EMIT_opc, TYPE_any, opc_ldiq, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "store")) != NULL) {
			install(cc, "i8",   EMIT_opc, TYPE_any, opc_sti1, type_vid, NULL);
			install(cc, "i16",  EMIT_opc, TYPE_any, opc_sti2, type_vid, NULL);
			install(cc, "i32",  EMIT_opc, TYPE_any, opc_sti4, type_vid, NULL);
			install(cc, "i64",  EMIT_opc, TYPE_any, opc_sti8, type_vid, NULL);
			install(cc, "i128", EMIT_opc, TYPE_any, opc_stiq, type_vid, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "cmt")) != NULL) {   // complement
			install(cc, "u32", EMIT_opc, TYPE_any, b32_cmt, type_u32, NULL);
			//install(cc, "u64", EMIT_opc, TYPE_any, b64_cmt, type_u64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "and")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, b32_and, type_u32, NULL);
			//install(cc, "u64", EMIT_opc, TYPE_any, b64_and, type_u64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "or")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, b32_ior, type_u32, NULL);
			//install(cc, "u64", EMIT_opc, TYPE_any, b64_ior, type_u64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "xor")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, b32_xor, type_u32, NULL);
			//install(cc, "u64", EMIT_opc, TYPE_any, b64_xor, type_u64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "shl")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, b32_shl, type_u32, NULL);
			//install(cc, "u64", EMIT_opc, TYPE_any, b64_shl, type_u64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}

		if ((opc = ccBegin(rt, "shr")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, b32_shr, type_u32, NULL);
			install(cc, "i32", EMIT_opc, TYPE_any, b32_sar, type_i32, NULL);
			//install(cc, "u64", EMIT_opc, TYPE_any, b64_shr, type_u64, NULL);
			//install(cc, "i64", EMIT_opc, TYPE_any, b64_sar, type_i64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "neg")) != NULL) {
			install(cc, "i32", EMIT_opc, TYPE_any, i32_neg, type_i32, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_neg, type_i64, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_neg, type_f32, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_neg, type_f64, NULL);
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_neg, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_neg, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "add")) != NULL) {
			install(cc, "i32", EMIT_opc, TYPE_any, i32_add, type_i32, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_add, type_i64, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_add, type_f32, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_add, type_f64, NULL);
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_add, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_add, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "sub")) != NULL) {
			install(cc, "i32", EMIT_opc, TYPE_any, i32_sub, type_i32, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_sub, type_i64, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_sub, type_f32, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_sub, type_f64, NULL);
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_sub, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_sub, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "mul")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, u32_mul, type_u32, NULL);
			install(cc, "i32", EMIT_opc, TYPE_any, i32_mul, type_i32, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_mul, type_i64, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_mul, type_f32, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_mul, type_f64, NULL);
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_mul, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_mul, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "div")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, u32_div, type_u32, NULL);
			install(cc, "i32", EMIT_opc, TYPE_any, i32_div, type_i32, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_div, type_i64, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_div, type_f32, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_div, type_f64, NULL);
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_div, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_div, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "mod")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, u32_mod, type_u32, NULL);
			install(cc, "i32", EMIT_opc, TYPE_any, i32_mod, type_i32, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_mod, type_i64, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_mod, type_f32, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_mod, type_f64, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "ceq")) != NULL) {
			install(cc, "i32", EMIT_opc, TYPE_any, i32_ceq, type_bol, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_ceq, type_bol, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_ceq, type_bol, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_ceq, type_bol, NULL);
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_ceq, type_bol, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_ceq, type_bol, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "clt")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, u32_clt, type_bol, NULL);
			install(cc, "i32", EMIT_opc, TYPE_any, i32_clt, type_bol, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_clt, type_bol, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_clt, type_bol, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_clt, type_bol, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "cgt")) != NULL) {
			install(cc, "u32", EMIT_opc, TYPE_any, u32_cgt, type_bol, NULL);
			install(cc, "i32", EMIT_opc, TYPE_any, i32_cgt, type_bol, NULL);
			install(cc, "i64", EMIT_opc, TYPE_any, i64_cgt, type_bol, NULL);
			install(cc, "f32", EMIT_opc, TYPE_any, f32_cgt, type_bol, NULL);
			install(cc, "f64", EMIT_opc, TYPE_any, f64_cgt, type_bol, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "min")) != NULL) {
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_min, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_min, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = ccBegin(rt, "max")) != NULL) {
			install(cc, "p4f", EMIT_opc, TYPE_any, v4f_max, type_p4x, NULL);
			install(cc, "p2d", EMIT_opc, TYPE_any, v2d_max, type_p4x, NULL);
			ccEnd(rt, opc, ATTR_stat);
		}
		if ((opc = type_p4x) != NULL) {
			ccBegin(rt, NULL);
			install(cc, "dp3", EMIT_opc, TYPE_any, v4f_dp3, type_f32, NULL);
			install(cc, "dp4", EMIT_opc, TYPE_any, v4f_dp4, type_f32, NULL);
			install(cc, "dph", EMIT_opc, TYPE_any, v4f_dph, type_f32, NULL);
			// p4x.xyzw
			if ((mode & installEswz) == installEswz) {
				unsigned i;
				struct { char* name; astn node; } swz[256];
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
				for (i = 0; i < lengthOf(swz); i += 1) {
					install(cc, swz[i].name, EMIT_opc, TYPE_any, p4x_swz, type_p4x, swz[i].node);
				}
			}
			ccEnd(rt, opc, ATTR_stat);
		}
		ccEnd(rt, cc->emit_opc, ATTR_stat);
	}
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
		ccBegin(rt, NULL);
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
		if ((arg = ccDefCall(rt, typenameGetFile, NULL, "string file;"))) {
			rt->cc->libc->chk += 1;
			rt->cc->libc->pop += 1;
			arg->stat = 0;
			arg->memb = 1;
		}
		else {
			error = 1;
		}

		// HACK: `operator(typename type).name = typename.name(type);`
		if ((arg = ccDefCall(rt, typenameGetName, NULL, "string name;"))) {
			rt->cc->libc->chk += 1;
			rt->cc->libc->pop += 1;
			arg->stat = 0;
			arg->memb = 1;
		}
		else {
			error = 1;
		}

		error = error || !ccDefCall(rt, typenameGetBase, NULL, "typename base(typename type);");

		ccEnd(rt, cc->type_rec, 0);

		/* TODO: more 4 reflection

		error = error || !ccDefCall(rt, typeFunction, (void*)typeOpGetFile, "variant setValue(typename field, variant value)");
		error = error || !ccDefCall(rt, typeFunction, (void*)typeOpGetFile, "variant getValue(typename field)");

		install(cc, "typename lookup(variant &obj, int options, string name, variant args...)");
		install(cc, "variant invoke(variant &obj, int options, string name, variant args...)");
		install(cc, "bool canassign(typename toType, variant value, bool canCast)");
		install(cc, "bool instanceof(typename &type, variant obj)");
		//~ */
	}
	return error;
}

///// Compiler

/// Initialze compiler context; @see header
ccContext ccInit(rtContext rt, int mode, vmError onHalt(libcContext)) {
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

	ccDefCall(rt, onHalt ? onHalt : haltDummy, NULL, "void Halt(int Code);");

	cc->root->type = cc->type_vid;
	cc->root->cst2 = TYPE_any;

	// install a void arg for functions with no arguments
	if (cc->type_vid && (cc->void_tag = newnode(cc, TYPE_ref))) {
		cc->void_tag->next = NULL;
		cc->void_tag->ref.name = "";

		ccBegin(rt, NULL);
		declare(cc, TYPE_ref, cc->void_tag, cc->type_vid);
		ccEnd(rt, NULL, 0);
	}

	if (cc->emit_opc && (cc->emit_tag = newnode(cc, TYPE_ref))) {
		cc->emit_tag->ref.link = cc->emit_opc;
		cc->emit_tag->ref.name = "emit";
		cc->emit_tag->ref.hash = -1;
	}

	install_base(rt, mode);

	return cc;
}

/// Begin a namespace; @see rtContext.api.ccBegin
symn ccBegin(rtContext rt, const char *name) {
	symn result = NULL;
	if (rt->cc != NULL) {
		if (name != NULL) {
			result = install(rt->cc, name, ATTR_stat | ATTR_const | TYPE_rec, TYPE_vid, 0, rt->cc->type_vid, NULL);
		}
		enter(rt->cc, NULL);
	}
	return result;
}

/// Close a namespace; @see rtContext.api.ccEnd
symn ccEnd(rtContext rt, symn cls, int mkStatic) {
	symn fields = NULL;
	if (rt->cc != NULL) {
		fields = leave(rt->cc, cls, mkStatic ? ATTR_stat : 0);
		if (cls != NULL && fields != NULL) {
			dieif(cls->flds != NULL, "Replacing static members of: %.T", cls);
			cls->flds = fields;
		}
	}
	return fields;
}

/// Declare int constant; @see rtContext.api.ccDefInt
symn ccDefInt(rtContext rt, const char* name, int64_t value) {
	if (!rt || !rt->cc || !name) {
		trace("%x, %s, %D", rt, name, value);
		return NULL;
	}
	name = mapstr(rt->cc, name, -1, -1);
	return install(rt->cc, name, TYPE_def, TYPE_int, 0, rt->cc->type_i32, intnode(rt->cc, value));
}
/// Declare float constant; @see rtContext.api.ccDefFlt
symn ccDefFlt(rtContext rt, const char* name, double value) {
	if (!rt || !rt->cc || !name) {
		trace("%x, %s, %F", rt, name, value);
		return NULL;
	}
	name = mapstr(rt->cc, name, -1, -1);
	return install(rt->cc, name, TYPE_def, TYPE_flt, 0, rt->cc->type_f64, fltnode(rt->cc, value));
}
/// Declare string constant; @see rtContext.api.ccDefStr
symn ccDefStr(rtContext rt, const char* name, char* value) {
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

/// Install a type; @see rtContext.api.ccDefType
symn ccDefType(rtContext rt, const char* name, unsigned size, int refType) {
	return install(rt->cc, name, ATTR_stat | ATTR_const | TYPE_rec, refType ? TYPE_ref : TYPE_rec, size, rt->cc->type_rec, NULL);
}

/// Find symbol by name; @see header
symn ccLookupSym(ccContext cc, symn in, char *name) {
	struct astNode ast;
	memset(&ast, 0, sizeof(struct astNode));
	ast.kind = TYPE_ref;
	ast.ref.name = name;
	ast.ref.hash = rehash(name, -1) % TBLS;
	return lookup(cc, in ? in->flds : cc->rt->vars, &ast, NULL, 1);
}

/// Lookup symbol by offset; @see rtContext.api.rtFindSym
symn rtFindSym(rtContext rt, size_t offs, int callsOnly) {
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

///// Debugger

// arrayBuffer
static void* setBuff(struct arrBuffer* buff, int idx, void* data) {
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
static void* insBuff(struct arrBuffer* buff, int idx, void* data) {
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
int initBuff(struct arrBuffer* buff, int initsize, int elemsize) {
	buff->cnt = 0;
	buff->ptr = 0;
	buff->esz = elemsize;
	buff->cap = initsize * elemsize;
	return setBuff(buff, initsize, NULL) != NULL;
}
void freeBuff(struct arrBuffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->cap = 0;
	buff->esz = 0;
}

dbgn getDbgStatement(rtContext rt, char* file, int line) {
	if (rt->dbg != NULL) {
		int i;
		dbgn result = (dbgn)rt->dbg->statements.ptr;
		for (i = 0; i < rt->dbg->statements.cnt; ++i) {
			if (result->file && strcmp(file, result->file) == 0) {
				if (line == result->line) {
					return result;
				}
			}
			result++;
		}
	}
	return NULL;
}
dbgn mapDbgStatement(rtContext rt, size_t position) {
	if (rt->dbg != NULL) {
		dbgn result = (dbgn)rt->dbg->statements.ptr;
		int i, n = rt->dbg->statements.cnt;
		for (i = 0; i < n; ++i) {
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
dbgn addDbgStatement(rtContext rt, size_t start, size_t end, astn tag) {
	dbgn result = NULL;
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

dbgn mapDbgFunction(rtContext rt, size_t position) {
	if (rt->dbg != NULL) {
		dbgn result = (dbgn)rt->dbg->functions.ptr;
		int i, n = rt->dbg->functions.cnt;
		for (i = 0; i < n; ++i) {
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
dbgn addDbgFunction(rtContext rt, symn fun) {
	dbgn result = NULL;
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
