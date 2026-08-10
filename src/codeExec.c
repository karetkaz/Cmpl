#include "code.h"
#include "type.h"
#include "debuger.h"
#include "printer.h"
#include "compiler.h"

#include <math.h>
#include <time.h>

// instead of void *
typedef uint32_t *stkptr;
typedef uint8_t *memptr;

/* Execution unit, and invoke() re-entry bridge.
 * invoke() runs a nested exec() for a native callback, pushing `vm.px` (the halt
 * stub) as a synthetic return address instead of a real call site. traceCalls()
 * cannot resolve that address to a caller, so invoke() links a node recording the
 * real (ip, sp) it is bridging over onto this chain, for traceCalls() to
 * substitute in its place. Each node lives in the stack frame of the invoke()
 * call that pushed it: no allocation, no fixed depth limit, unlinked on return.
 */
struct vmContextRec {
	/* Stack pointers
	 * the stacks are located at the end of the heap.
	 * base pointer
	 *    is the base of the stack, and should not be modified during execution.
	 * trace pointer
	 *    is available only in debug mode, and gets incremented on each function call.
	 *    it points to debug data for tracing and profiling (struct traceRec)
	 *    the trace pointer must be greater than the base pointer
	 * stack pointer
	 *     is initialized with the base pointer plus the stack size and is decremented on push.
	 *     it points to local variables, arguments.
	 *     the stack pointer must be greater than the trace pointer
	 */
	memptr ip;		// Instruction pointer
	memptr bp;		// Stack base
	stkptr sp;		// Stack pointer
	size_t ss;		// Stack size

	symn fun;
	rtContext rt;
	vmContext vm;        // invoke(): enclosing (outer) bridge that was bridged over, or NULL if there is none
	nfcContext nfc;	     // invoke(): the native function that made this call, or NULL for the top level execution unit
};

/// Check for stack overflow.
static inline int ovf(vmContext pu) {
	return (memptr)pu->sp < pu->bp;
}

static inline int f32Bool(float value) {
	return (value != 0) && !isnan(value);
}
static inline int f64Bool(double value) {
	return (value != 0) && !isnan(value);
}

vmError dbgError(dbgContext ctx, vmError error, size_t pc, size_t ss, void *sp, int mode) {
	if (error == noError) {
		return noError;
	}

	const rtContext rt = ctx->rt;
	const char *errMsg = vmErrorMessage(error);
	vmInstruction ip = vmPointer(rt, pc);

	// source code position
	const char *file = NULL;
	int line = 0;
	dbgn dbg = mapDbgStatement(rt, pc, NULL);
	if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
		file = dbg->file;
		line = dbg->line;
	}

	// current function
	symn fun = rtLookup(rt, pc, 0);
	size_t offs = pc;
	if (fun != NULL) {
		offs -= fun->offs;
	}

	if (error == nativeCallError) {
		libc *nativeCalls = rt->vm.nfc;
		symn nc = nativeCalls[ip->rel]->sym;
		error(rt, file, line, ERR_EXEC_NATIVE_CALL, errMsg, pc, fun, offs, nc);
	}
	else {
		error(rt, file, line, ERR_EXEC_INSTRUCTION, errMsg, pc, fun, offs, ip);
	}
	// print stack trace including this function
	traceCalls(rt, rt->logFile, 1, maxLogItems, 0);
	return error;
	(void) mode;
	(void) sp;
	(void) ss;
}

/**
 * Execute bytecode.
 *
 * @param ctx Runtime context.
 * @param pu Cell to execute on.
 * @param fun The function to execute.
 * @param extra Extra data for native calls.
 * @return Error code of execution, noError on success.
 */
static vmError exec(vmContext pu, const void *extra) {
	rtContext ctx = pu->rt;
	symn fun = pu->fun;

	vmError execError = noError;
	const size_t ms = ctx->_size;			// memory size
	const size_t ro = ctx->vm.ro;			// read-only region
	const memptr mp = (void*)ctx->_mem;
	const stkptr st = (void*)(pu->bp + pu->ss);
	libc *nativeCalls = ctx->vm.nfc;

	if (fun == NULL || fun->offs == 0) {
		error(ctx, NULL, 0, ERR_EXEC_FUNCTION, fun);
		return illegalState;
	}

	// run in debug or profile mode
	const dbgContext dbg = ctx->dbg;
	if (dbg != NULL && dbg->debug != NULL) {
		const stkptr spMin = (stkptr)pu->bp;
		const stkptr spMax = (stkptr)(pu->bp + pu->ss);
		const size_t ssReset = st - pu->sp;

		const vmInstruction ipMin = (vmInstruction)(ctx->_mem + ctx->vm.ro);
		const vmInstruction ipMax = (vmInstruction)(ctx->_mem + ctx->vm.px);

		vmError (*debugger)(dbgContext, vmError, size_t, size_t, void*, int) = dbg->debug;

		// invoked function(from external code) will return with a `ret` instruction, removing trace info
		execError = debugger(dbg, noError, fun->offs, st - pu->sp, pu->sp, 1);
		if (execError != noError) {
			// leave the previously entered method
			return execError;
		}

		for ( ; ; ) {
			const vmInstruction ip = (vmInstruction)pu->ip;
			const size_t pc = vmOffset(ctx, ip);
			const stkptr sp = pu->sp;

			if (ip > ipMax || ip < ipMin) {
				return debugger(dbg, illegalState, pc, ssReset, sp, -1);
			}
			if (sp > spMax || sp < spMin) {
				return debugger(dbg, illegalState, pc, ssReset, sp, -1);
			}

			vmError error = debugger(dbg, noError, pc, ssReset, sp, 0);
			if (error != noError) {
				// abort execution from debugger
				return error;
			}
			switch (ip->opc) {
				dbg_stop_vm:	// halt virtual machine
				if (execError != noError || pu->vm == NULL) {
					return debugger(dbg, execError, pc, ssReset, sp, -1);
				}
				return noError;

				dbg_error_opc:
					execError = illegalInstruction;
					goto dbg_stop_vm;

				dbg_error_ovf:
					execError = stackOverflow;
					goto dbg_stop_vm;

				dbg_error_mem:
					execError = illegalMemoryAccess;
					goto dbg_stop_vm;

				dbg_error_idx:
					execError = illegalArrayAccess;
					goto dbg_stop_vm;

				dbg_error_div_flt:
					debugger(dbg, divisionByZero, pc, st - sp, sp, 0);
					// continue execution on floating point division by zero.
					break;

				dbg_error_div:
					execError = divisionByZero;
					goto dbg_stop_vm;

				dbg_error_libc:
					execError = nativeCallError;
					goto dbg_stop_vm;

				#define NEXT(__IP, __SP, __CHK) pu->sp -= (__SP); pu->ip += (__IP);
				#define STOP(__ERR, __CHK) do {if (__CHK) {goto dbg_##__ERR;}} while(0)
				#define TRACE(__PC, __MODE) do { if ((execError = debugger(dbg, noError, __PC, st - sp, sp, __MODE)) != noError) goto dbg_stop_vm; } while(0)
				#define EXEC
				#include "exec.i"
			}
		}
		return execError;
	}

	// code for maximum execution speed
	for ( ; ; ) {
		const vmInstruction ip = (vmInstruction)pu->ip;
		const stkptr sp = pu->sp;
		switch (ip->opc) {
			stop_vm:	// halt virtual machine
				if (execError != noError && fun == ctx->main) {
					struct dbgContextRec dbgRec = {0};
					dbgRec.rt = ctx;
					dbgError(&dbgRec, execError, vmOffset(ctx, ip), st - sp, sp, 0);
				}
				return execError;

			error_opc:
				execError = illegalInstruction;
				goto stop_vm;

			error_ovf:
				execError = stackOverflow;
				goto stop_vm;

			error_mem:
				execError = illegalMemoryAccess;
				goto stop_vm;

			error_idx:
				execError = illegalArrayAccess;
				goto stop_vm;

			error_div_flt:
				// continue execution on floating point division by zero.
				break;

			error_div:
				execError = divisionByZero;
				goto stop_vm;

			error_libc:
				execError = nativeCallError;
				goto stop_vm;

			#define NEXT(__IP, __SP, __CHK) { pu->sp -= (__SP); pu->ip += (__IP); }
			#define STOP(__ERR, __CHK) do {if (__CHK) {goto __ERR;}} while(0)
			#define TRACE(__PC, __MODE)
			#define EXEC
			#include "exec.i"
		}
	}
	return execError;
}

vmError invoke(nfcContext ctx, symn fun, void *res, void *args, const void *extra) {
	rtContext rt = ctx->rt;
	vmContext cell = rt->vm.vm;

	if (cell == NULL) {
		fatal(ERR_INTERNAL_ERROR": can not invoke %?T without execute", fun);
		return illegalState;
	}

	// invoked symbol must be a static function
	if (fun->params == NULL || fun->offs == 0 || !isStatic(fun)) {
		dieif(fun->params == NULL, ERR_INTERNAL_ERROR);
		dieif(fun->offs == 0, ERR_INTERNAL_ERROR);
		dieif(!isStatic(fun), ERR_INTERNAL_ERROR);
		return illegalState;
	}

	// result is the last argument.
	size_t argSize = argsSize(fun);
	size_t resSize = fun->params->size;

	// create a new execution context, based on the previous one
	struct vmContextRec link = {
		.rt = ctx->rt,
		.ip = vmPointer(rt, fun->offs),
		.bp = cell->bp,
		.sp = cell->sp,
		.ss = cell->ss,
		.fun = fun,
		.nfc = ctx,
		.vm = cell,
	};

	// make space for result and arguments
	link.sp -= argSize / vm_stk_align;

	if (args != NULL) {
		memcpy((char *)link.sp, args, argSize - resSize);
	}

	// return here: vm->px: program halt
	*(link.sp -= 1) = rt->vm.px;


	rt->vm.vm = &link;
	vmError result = exec(&link, extra);
	rt->vm.vm = cell;

	if (result == noError && res != NULL) {
		memcpy(res, (memptr) cell->sp - resSize, resSize);
	}
	return result;
}
vmError execute(rtContext ctx, const void *extra) {
	// invalidate compiler, release the malloc allocated buffers before losing them
	if (ctx->cc != NULL) {
		freeBuff(&ctx->cc->jumps);
	}
	ctx->cc = NULL;

	// invalidate memory manager
	ctx->vm.heap = NULL;

	// allocate processor(s)
	ctx->_end = ctx->_mem + ctx->_size;
	ctx->_end -= sizeof(struct vmContextRec);
	vmContext vm = (void*)ctx->_end;
	ctx->vm.vm = vm;

	// all pointers must be aligned
	logif(ctx->_size != padOffset(ctx->_size, vm_mem_align), ERR_INTERNAL_ERROR);
	logif(ctx->_mem !=  padPointer(ctx->_mem, vm_mem_align), ERR_INTERNAL_ERROR);
	logif(ctx->_end !=  padPointer(ctx->_end, vm_stk_align), ERR_INTERNAL_ERROR);

	if (ctx->vm.ss == 0) {
		ctx->vm.ss = ctx->_size / 4;
	}
	ctx->_end -= ctx->vm.ss;
	if (ctx->_beg > ctx->_end) {
		fatal(ERR_INTERNAL_ERROR": memory overrun");
		return illegalState;
	}

	vm->rt = ctx;
	vm->ip = ctx->_mem + ctx->vm.pc;
	vm->bp = ctx->_end;
	vm->sp = (stkptr)(ctx->_end + ctx->vm.ss);
	vm->ss = ctx->vm.ss;
	vm->fun = ctx->main;
	vm->nfc = NULL;
	vm->vm = NULL;

	if (vm->bp > (memptr)vm->sp) {
		fatal(ERR_INTERNAL_ERROR": invalid stack size");
		return illegalState;
	}

	return exec(vm, extra);
}

int isChecked(rtContext ctx) {
	vmContext bridge = ctx->vm.vm;
	while (bridge != NULL) {
		if (bridge->nfc != NULL && bridge->nfc->proto == sys_try_exec) {
			return 1;
		}
		bridge = bridge->vm;
	}
	return 0;
}

int vmSelfTest(void cb(const char *, const struct opcodeRec *)) {
	int errors = 0;
	struct vmInstruction ip[1] = {0};
	struct libc nfc0[1] = {0};

	struct libc *nativeCalls[1];
	nativeCalls[0] = nfc0;

	for (int i = 0; i < opc_last; i++) {
		const struct opcodeRec *info = &opcode_tbl[i];
		unsigned int IS, CHK, DIFF;
		char *error = NULL;

		if (info->name == NULL && info->size == 0 && info->stack_in == 0 && info->stack_out == 0) {
			// skip unimplemented instruction.
			continue;
		}

		// take care of some special opcodes
		switch (i) {
			default:
				break;

			// set.sp(0) will pop no elements from the stack,
			// actually it will do nothing, by setting itself to the same value ...
			case opc_set1:
				ip->idx = 1;
				break;

			case opc_set2:
				ip->idx = 2;
				break;

			case opc_set4:
				ip->idx = 4;
				break;
		}

		switch (ip->opc = (uint8_t) i) {
			error_opc:
				error = "Invalid instruction";
				errors += 1;
				break;
			error_len:
				error = "Invalid instruction length";
				errors += 1;
				break;

			error_chk:
				error = "Invalid stack arguments";
				errors += 1;
				break;

			error_diff:
				error = "Invalid stack size";
				errors += 1;
				break;

			#define NEXT(__IP, __SP, __CHK) {\
				IS = __IP;\
				CHK = __CHK;\
				DIFF = __SP;\
				if (IS && IS != info->size) {\
					goto error_len;\
				}\
				if (CHK != info->stack_in) {\
					goto error_chk;\
				}\
				if (DIFF != (info->stack_out - info->stack_in)) {\
					goto error_diff;\
				}\
			}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "exec.i"
		}
		if (cb != NULL) {
			cb(error, info);
		}
	}
	return errors;
}

// todo: relocate to debug.c
static void traceArgs(rtContext ctx, FILE *out, symn fun, const char *file, int line, void *sp, int indent, dmpMode mode) {
	int printFileLine = 0;
	const char **esc = NULL;

	printFmt(out, esc, "%I%?s:%?u: %?.T", indent, file, line, fun);
	if (indent < 0) {
		printFileLine = 1;
		indent = -indent;
	}

	dieif(sp == NULL, ERR_INTERNAL_ERROR);
	dieif(fun == NULL, ERR_INTERNAL_ERROR);
	if (fun->params != NULL) {
		int firstArg = 1;
		if (indent > 0) {
			printFmt(out, esc, "(");
		}
		else {
			printFmt(out, esc, "\n");
		}
		size_t args = 0;
		for (symn sym = fun->params->next; sym; sym = sym->next) {
			if (args < sym->offs) {
				args = sym->offs;
			}
		}
		for (symn sym = fun->params->next; sym; sym = sym->next) {
			size_t offs = args - sym->offs;

			if (firstArg == 0) {
				printFmt(out, esc, ", ");
			}
			else {
				firstArg = 0;
			}

			if (printFileLine) {
				if (sym->file != NULL && sym->line != 0) {
					printFmt(out, esc, "%I%s:%u: ", indent, sym->file, sym->line);
				}
				else {
					printFmt(out, esc, "%I", indent);
				}
			}

			dieif(isStatic(sym), ERR_INTERNAL_ERROR);
			if (isFunction(fun)) {
				// at vm_stk_align is the return value of the function.
				offs += vm_stk_align;
			}
			printVal(out, NULL, ctx, sym, (vmValue *)((char*)sp + offs), mode, -indent);
		}
		if (indent > 0) {
			printFmt(out, esc, ")");
		}
		else {
			printFmt(out, esc, "\n");
		}
	}
}

/** Compute the static stack depth at a position inside a function.
 * Walks the bytecode from the function entry following the branches,
 * accumulating the stack effect of each instruction.
 * @param ctx Runtime context
 * @param fun the function containing the position
 * @param target position of the instruction to be reached
 * @return stack depth in bytes relative to the function entry, -1 if unreachable.
 */
static ssize_t stackDepth(rtContext ctx, symn fun, size_t target) {
	const size_t beg = fun->offs;
	const size_t size = fun->size;
	if (target < beg || target >= beg + size) {
		return -1;
	}

	ssize_t result = -1;
	ssize_t *depths = malloc(size * sizeof(ssize_t));
	size_t *worklist = malloc(size * sizeof(size_t));
	if (depths == NULL || worklist == NULL) {
		free(depths);
		free(worklist);
		return -1;
	}
	for (size_t i = 0; i < size; ++i) {
		depths[i] = -1;
	}

	size_t tasks = 0;
	depths[0] = 0;
	worklist[tasks++] = beg;
	while (tasks > 0) {
		size_t pc = worklist[--tasks];
		ssize_t depth = depths[pc - beg];
		if (pc == target) {
			result = depth;
			break;
		}

		size_t next = pc;
		vmInstruction ip = nextOpc(ctx, &next, &depth);
		if (ip == NULL) {
			// invalid instruction
			continue;
		}

		size_t branches[2] = {next, 0};
		switch (ip->opc) {
			default: break;

			case opc_jnz:
			case opc_jz:
				branches[1] = pc + ip->rel;
				break;

			case opc_jmp:
				branches[0] = pc + ip->rel;
				break;

			case opc_jmpi:
				// return from the function: no successor
				branches[0] = 0;
				break;

			case opc_call:
				// the callee returns dropping the return address
				depth -= vm_stk_align;
				break;
		}

		for (int i = 0; i < 2; ++i) {
			size_t to = branches[i];
			if (to < beg || to >= beg + size) {
				continue;
			}
			if (depths[to - beg] >= 0) {
				// already visited
				continue;
			}
			depths[to - beg] = depth;
			worklist[tasks++] = to;
		}
	}

	free(depths);
	free(worklist);
	return result;
}

void traceCalls(rtContext ctx, FILE *out, int indent, size_t maxCalls, size_t skipCalls) {
	vmContext pu = ctx->vm.vm;
	if (pu == NULL) {
		return;
	}

	const char **esc = NULL;
	size_t hasOutput = 0;
	if (out == NULL) {
		out = ctx->logFile;
	}

	const size_t pcMax = ctx->vm.px;
	const size_t pcMin = ctx->vm.ro;
	size_t pc = vmOffset(ctx, pu->ip);

	const stkptr spMin = (void*) pu->bp;
	const stkptr spMax = spMin + pu->ss;
	stkptr sp = pu->sp;

	vmContext bridge = pu->vm;

	for (size_t i = 0; i < maxCalls; ++i) {
		symn fun = rtLookup(ctx, pc, 0);
		if (fun == NULL || fun == ctx->main) {
			// logif("WIP", "the `%T` function has no return address", fun);
			pc = 0;
			break;
		}

		ssize_t depth = stackDepth(ctx, fun, pc) / vm_stk_align;
		if (depth < 0) {
			// unreachable
			break;
		}

		// position of the return address: the stack at the function entry
		stkptr args = sp + depth;
		sp = args + 1;
		if (sp >= spMax || sp < spMin) {
			fatal(ERR_INTERNAL_ERROR": invalid stack pointer unwinding the stack");
			pc = 0;
			break;
		}

		// continue in the caller, after the return address
		pc = args[0];
		vmContext nativeCall = NULL;
		if (pc == ctx->vm.px) {
			if (bridge == NULL) {
				pc = 0;
				break;
			}
			pc = vmOffset(ctx, bridge->ip);
			nativeCall = bridge;
			bridge = bridge->vm;
		}
		if (pc >= pcMax || pc < pcMin) {
			fatal(ERR_INTERNAL_ERROR": invalid program counter unwinding the stack");
			pc = 0;
			break;
		}

		if (i < skipCalls) {
			// do not print the n calls as requested byt the user
			continue;
		}

		// the return address points after the call instruction
		dbgn trInfo = mapDbgStatement(ctx, pc - 1, NULL);
		const char *file = fun->file;
		int line = fun->line;

		if (hasOutput > 0) {
			printFmt(out, esc, "\n");
			hasOutput += 1;
		}

		if (nativeCall != NULL && i < maxCalls - 1) {
			// native calls have no file and line position, use the function position
			traceArgs(ctx, out, fun, file, line, args, indent, prArgs | prOneLine);
			printFmt(out, esc, "\n");

			fun = nativeCall->fun;
			sp = nativeCall->sp;
			args = sp;
			i += 1;
		}

		// if no debug data is available, use the function position
		if (trInfo != NULL) {
			file = trInfo->file;
			line = trInfo->line;
		}

		traceArgs(ctx, out, fun, file, line, args, indent, prArgs | prOneLine);
		hasOutput += 1;
	}

	if (pc != 0 && pc != ctx->vm.px && maxCalls > 0) {
		printFmt(out, esc, "\n%I... more", indent);
		hasOutput += 1;
	}

	if (hasOutput > 0) {
		printFmt(out, esc, "\n");
	}
}
