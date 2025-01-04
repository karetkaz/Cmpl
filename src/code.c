#include "code.h"
#include "compiler.h"
#include "debuger.h"
#include "printer.h"
#include "type.h"
#include "util.h"

#include <stdarg.h>

const struct opcodeRec opcode_tbl[256] = {
	#define OPCODE_DEF(Name, Code, Size, In, Out, Text) {Code, Size, In, Out, Text},
	#include "code.i"
};

/// Private raise wrapper for the api
static void rtRaise(nfcContext ctx, int level, const char *msg, ...) {
	static const struct nfcArgArr details = {0};
	const rtContext rt = ctx->rt;
	va_list vaList;
	va_start(vaList, msg);
	print_log(rt, level, NULL, 0, details, msg, vaList);
	va_end(vaList);

	// print stack trace including this function
	traceCalls(rt->dbg, rt->logFile, 1, maxLogItems, 0);
}

/**
 * Allocate free and resize memory inside the vm heap.
 *
 * @param ctx Execution context.
 * @param ptr resize this memory chunk or allocate new if null.
 * @param size new size of the allocation or free by resizing the chunk to 0 bytes.
 * @return Pointer to the allocated memory inside the vm heap.
 * cases:
 * 		size >  0 && ptr != null: re-alloc
 * 		size >  0 && ptr == null: alloc
 * 		size == 0 && ptr != null: free
 * 		size == 0 && ptr == null: nop
 * @note Invocation to `execute` must precede this call.
 *
 * how it works:
 * Using one linked list containing both used and unused memory chunks.
 * The idea is when allocating a block of memory, we always must traverse the list of chunks.
 * When freeing a block not. Because of this, if a block is used, `prev` points to the previous block.
 * When deallocating, the previous and/or the next can be merged with the current if it is free (`prev` is set to `null`).
 *
 * ╭─────────────╮
 * │ next        │ next chunk (allocated or free)
 * │ prev        │ previous chunk (null if it is free)
 * ├─────────────┤
 * │             │
 * : ...         :
 * ├─────────────┤
 * │ next        │
 * │ prev        │
 * ├─────────────┤
 * │             │
 * : ...         :
 * ╰─────────────╯
 */
static void *rtAlloc(nfcContext ctx, void *ptr, size_t size) {
	typedef struct memChunk *memChunk;
	struct memChunk {
		memChunk prev;      // null for free chunks
		memChunk next;      // next chunk
		char data[];        // here begins the user data
	};

	rtContext rt = ctx->rt;
	dbgContext dbg = rt->dbg;
	const ssize_t minAllocationSize = sizeof(struct memChunk);
	size_t allocSize = padOffset(size + minAllocationSize, minAllocationSize);
	memChunk chunk = NULL;

	// memory manager is not initialized, initialize it first
	if (rt->vm.heap == NULL) {
		memChunk heap = padPointer(rt->_beg, minAllocationSize);
		memChunk last = padPointer(rt->_end - 2 * minAllocationSize, minAllocationSize);

		heap->next = last;
		heap->prev = NULL;

		last->next = NULL;
		last->prev = NULL;

		rt->vm.heap = heap;
	}

	// chop or free.
	if (ptr != NULL) {
		chunk = (memChunk)((char*)ptr - offsetOf(struct memChunk, data));
		size_t chunkSize = chunk->next ? ((char*)chunk->next - (char*)chunk) : 0;

		if ((unsigned char*)ptr < rt->_beg || (unsigned char*)ptr > rt->_end) {
			dieif((unsigned char*)ptr < rt->_beg, ERR_INVALID_OFFSET, vmOffset(rt, ptr));
			dieif((unsigned char*)ptr > rt->_end, ERR_INVALID_OFFSET, vmOffset(rt, ptr));
			return NULL;
		}

		if (dbg != NULL && dbg->dbgAlloc != NULL) { // extra check if ptr is in used list.
			memChunk find = rt->vm.heap;
			memChunk prev = find;
			while (find && find != chunk) {
				prev = find;
				find = find->next;
			}
			if (find != chunk || chunk->prev != prev) {
				dbg->dbgAlloc(dbg, ptr, -1, "unallocated pointer to resize");
				return NULL;
			}
		}

		if (size == 0) {							// free
			memChunk next = chunk->next;
			memChunk prev = chunk->prev;

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
			memChunk next = chunk->next;
			memChunk free = (memChunk)((char*)chunk + allocSize);

			// do not make a small free unaligned block (chop 161 to 160 bytes)
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
			error(rt, NULL, 0, ERR_INTERNAL_ERROR);
			chunk = NULL;
		}
	}
	// allocate.
	else if (size > 0) {
		memChunk prev = chunk = rt->vm.heap;

		while (chunk != NULL) {
			memChunk next = chunk->next;

			// check if block is free.
			if (chunk->prev == NULL && next != NULL) {
				size_t chunkSize = (char*)next - (char*)chunk;
				if (allocSize < chunkSize) {
					ssize_t diff = chunkSize - allocSize;
					if (diff > minAllocationSize) {
						memChunk free = (memChunk)((char*)chunk + allocSize);
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

	if (dbg != NULL) {
		size_t free = 0, used = 0;
		void (*dbgFn)(dbgContext, void *, size_t, char *) = dbg->dbgAlloc;
		for (memChunk mem = rt->vm.heap; mem; mem = mem->next) {
			if (mem->next) {
				size_t chunkSize = (char*)mem->next - (char*)mem - sizeof(struct memChunk);
				if (mem->prev != NULL) {
					used += chunkSize;
					if (dbgFn != NULL) {
						dbgFn(dbg, mem->data, chunkSize, "used");
					}
				}
				else {
					free += chunkSize;
					if (dbgFn != NULL) {
						dbgFn(dbg, mem->data, chunkSize, "free");
					}
				}
			}
		}
		dbg->freeMem = free;
		dbg->usedMem = used;
	}

	return chunk ? chunk->data : NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Native Calls

/**
 * get an iterator to the native call methods.
 *
 * @param ctx the native call context.
 * @return iterator for argument list.
 */
static struct nfcArgs nfcArgList(nfcContext ctx) {
	struct nfcArgs result = {0};
	// the first argument is `result`
	result.param = ctx->sym->params;
	result.ctx = ctx;
	return result;
}

/**
 * Read the value at the given param converting offsets to pointers.
 * Advance to the next argument.
 *
 * @param args the arguments iterator.
 * @return the iterator, or NULL for ending, or if an error occurred.
 * @note offsets inside structs will be not converted to pointers.
 */
static nfcArgs nfcArgNext(nfcArgs args) {
	symn param = args->param->next;
	if (param == NULL) {
		return NULL;
	}

	args->param = param;
	args->offset = args->ctx->argc - param->offs;
	vmValue* value = (vmValue *) ((char *) args->ctx->argv + args->offset);

	rtContext rt = args->ctx->rt;
	switch (castOf(param)) {
		default:
			break;

		case CAST_bit:
		case CAST_u32:
		case CAST_f32:
			// zero extend 32 bits
			args->u64 = value->u32;
			return args;

		case CAST_i32:
			// sign extend 32 bits
			args->i64 = value->i32;
			return args;

		case CAST_i64:
		case CAST_u64:
		case CAST_f64:
			// copy 64 bits
			args->i64 = value->i64;
			return args;

		case CAST_ptr:
		case CAST_obj:
			args->ref = vmPointer(rt, value->ref);
			return args;

		case CAST_arr:
			args->arr.ref = vmPointer(rt, value->ref);
			args->arr.length = value->length;
			return args;

		case CAST_var:
			args->var.ref = vmPointer(rt, value->ref);
			args->var.type = rtLookup(rt, value->type, 0);
			return args;

		case CAST_val:
			if (param->size > sizeof(args->u64)) {
				break;
			}

			args->u64 = value->u64;
			return args;
	}

	fatal("invalid param: %T", param);
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Core

rtContext rtInit(void *mem, size_t size) {
	rtContext rt = padPointer(mem, vm_mem_align);

	if (rt != mem && mem != NULL) {
		size -= (char*) rt - (char*) mem;
	}

	if (rt == NULL && size > sizeof(struct rtContextRec)) {
		rt = malloc(size);
	}

	if (rt == NULL) {
		return NULL;
	}
	memset(rt, 0, sizeof(struct rtContextRec));

	*(void**)&rt->api.ccExtend = ccExtend;
	*(void**)&rt->api.ccBegin = ccBegin;
	*(void**)&rt->api.ccEnd = ccEnd;

	*(void**)&rt->api.ccDefInt = ccDefInt;
	*(void**)&rt->api.ccDefFlt = ccDefFlt;
	*(void**)&rt->api.ccDefStr = ccDefStr;
	*(void**)&rt->api.ccDefVar = ccDefVar;

	*(void**)&rt->api.ccAddType = ccAddType;
	*(void**)&rt->api.ccAddCall = ccAddCall;
	*(void**)&rt->api.ccAddUnit = ccAddUnit;
	*(void**)&rt->api.ccLookup = ccLookup;

	*(void**)&rt->api.alloc = rtAlloc;
	*(void**)&rt->api.raise = rtRaise;
	*(void**)&rt->api.lookup = rtLookup;
	*(void**)&rt->api.invoke = invoke;

	*(void**)&rt->api.nfcArgs = nfcArgList;
	*(void**)&rt->api.nextArg = nfcArgNext;

	// default values
	rt->logLevel = 5;
	rt->foldCasts = 1;
	rt->foldConst = 1;
	rt->foldInstr = 1;
	rt->foldMemory = 1;
	rt->foldAssign = 1;
	rt->freeMem = (unsigned) (mem == NULL);

	*(size_t*)&rt->_size = size - sizeof(struct rtContextRec);
	rt->_end = rt->_mem + rt->_size;
	rt->_beg = rt->_mem + 1;

	logFILE(rt, stdout);
	return rt;
}

int rtClose(rtContext ctx) {
	// close libraries
	closeLibs(ctx);

	// close log file
	logFILE(ctx, NULL);

	int errors = ctx->errors;
	// release debugger memory
	if (ctx->dbg != NULL) {
		freeBuff(&ctx->dbg->functions);
		freeBuff(&ctx->dbg->statements);
	}

	// release memory
	if (ctx->freeMem) {
		free(ctx);
	}

	return errors;
}

size_t vmInit(rtContext ctx, vmError onHalt(nfcContext)) {
	ccContext cc = ctx->cc;

	// initialize native calls
	if (cc != NULL && cc->native != NULL) {
		list lst = cc->native;
		libc last = (libc) lst->data;
		libc *calls = (libc*)(ctx->_beg = padPointer(ctx->_beg, vm_mem_align));

		ctx->_beg += (last->offs + 1) * sizeof(libc);
		if(ctx->_beg >= ctx->_end) {
			fatal(ERR_MEMORY_OVERRUN);
			return 0;
		}

		ctx->vm.nfc = calls;
		for (; lst != NULL; lst = lst->next) {
			libc nfc = (libc) lst->data;
			calls[nfc->offs] = nfc;

			// relocate native call offsets to be debuggable and traceable.
			nfc->sym->offs = vmOffset(ctx, nfc);
			nfc->sym->size = 0;
			addDbgFunction(ctx, nfc->sym);
		}
	}
	else {
		libc stop = (libc)(ctx->_beg = padPointer(ctx->_beg, vm_mem_align));
		ctx->_beg += sizeof(struct libc);
		if(ctx->_beg >= ctx->_end) {
			fatal(ERR_MEMORY_OVERRUN);
			return 0;
		}

		libc *calls = (libc*)(ctx->_beg = padPointer(ctx->_beg, vm_mem_align));
		ctx->_beg += sizeof(libc);
		if(ctx->_beg >= ctx->_end) {
			fatal(ERR_MEMORY_OVERRUN);
			return 0;
		}

		memset(stop, 0, sizeof(struct libc));
		stop->call = onHalt;
		calls[0] = stop;

		ctx->vm.nfc = calls;

		// add the main function
		if (ctx->main == NULL) {
			symn sym = (symn) (ctx->_beg = padPointer(ctx->_beg, vm_mem_align));
			ctx->_beg += sizeof(struct symNode);
			if (ctx->_beg >= ctx->_end) {
				fatal(ERR_MEMORY_OVERRUN);
				return 0;
			}
			memset(sym, 0, sizeof(struct symNode));
			sym->kind = ATTR_stat | KIND_fun | CAST_ptr;
			sym->offs = vmOffset(ctx, ctx->_beg);
			sym->name = ".main";
			ctx->main = sym;
		}
	}

	return ctx->_beg - ctx->_mem;
}
