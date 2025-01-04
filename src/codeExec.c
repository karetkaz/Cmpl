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

// method tracing
typedef struct trcptr {
	size_t caller;      // Instruction pointer of caller
	size_t callee;      // Instruction pointer of callee
	clock_t func;       // time when the function was invoked
	stkptr sp;          // Stack pointer
} *trcptr;

// execution unit
typedef struct vmProcessor {
	/* Stack pointers
	 * the stacks are located at the end of the heap.
	 * base pointer
	 *    is the base of the stack, and should not be modified during execution.
	 * trace pointer
	 *    is available only in debug mode, and gets incremented on each function call.
	 *    it points to debug data for tracing and profiling (struct traceRec)
	 *    the trace pointer must be greater than the base pointer
	 * stack pointer
	 *     is initialized with base pointer + stack size, and is decremented on push.
	 *     it points to local variables, arguments.
	 *     the stack pointer must be greater than the trace pointer
	 */
	memptr ip;		// Instruction pointer
	memptr bp;		// Stack base
	trcptr tp;		// Trace pointer
	stkptr sp;		// Stack pointer
	size_t ss;		// Stack size
	unsigned try: 1; // flag: try exec: fatal error recoverable will not exit the process
} *vmProcessor;

/// Check for stack overflow.
static inline int ovf(vmProcessor pu) {
	return (memptr)pu->sp < (memptr)pu->tp;
}

static inline int f32Bool(float value) {
	return (value != 0) && !isnan(value);
}
static inline int f64Bool(double value) {
	return (value != 0) && !isnan(value);
}

/** manage function call stack traces
 * @param ctx runtime context
 * @param sp tack pointer (for tracing arguments
 * @param caller the address of the caller, NULL on start and end.
 * @param callee the called function address, -1 on leave.
 */
static vmError vmTrace(rtContext ctx, void *sp, size_t caller, ssize_t callee) {
	const dbgContext dbg = ctx->dbg;
	clock_t now = clock();
	if (dbg == NULL) {
		return noError;
	}

	// trace statement
	if (callee == 0) {
		fatal(ERR_INTERNAL_ERROR);
		return illegalState;
	}

	vmError (*debugger)(dbgContext, vmError, size_t, void*, size_t, size_t) = dbg->debug;
	vmProcessor pu = ctx->vm.cell;
	// trace return from function
	if (callee < 0) {
		if (pu->tp - (trcptr)pu->bp < 1) {
			dbgTrace("stack underflow(tp: %d, sp: %d)", pu->tp - (trcptr)pu->bp, pu->sp - (stkptr)pu->bp);
			return illegalState;
		}
		pu->tp -= 1;

		if (debugger != NULL) {
			const trcptr tp = pu->tp;
			const trcptr bp = (trcptr) pu->bp;

			//* TODO: measure times in debugger function
			clock_t ticks = now - tp->func;
			dbgn calleeFunc = mapDbgFunction(ctx, tp->callee);
			if (calleeFunc != NULL) {
				int recursive = 0;
				for (trcptr p = bp; p < tp; p++) {
					if (p->callee == tp->callee) {
						recursive = 1;
						break;
					}
				}
				if (callee != -1) {
					calleeFunc->fails += 1;
				}
				if (!recursive) {
					calleeFunc->total += ticks;
				}
				calleeFunc->time += ticks;
			}

			dbgn callerFunc = mapDbgFunction(ctx, tp->caller);
			if (callerFunc != NULL) {
				callerFunc->time -= ticks;
			}
			// */
			debugger(dbg, noError, tp - bp, NULL, caller, callee);
		}
		return noError;
	}

	// trace call function
	if ((trcptr)pu->sp - pu->tp < 1) {
		dbgTrace("stack overflow(tp: %d, sp: %d)", pu->tp - (trcptr)pu->bp, pu->sp - (stkptr)pu->bp);
		return stackOverflow;
	}

	const trcptr tp = pu->tp;
	pu->tp += 1;

	tp->caller = caller;
	tp->callee = callee;
	tp->func = now;
	tp->sp = sp;

	if (debugger != NULL) {
		//* TODO: measure times in debugger function
		dbgn calleeFunc = mapDbgFunction(ctx, tp->callee);
		if (calleeFunc != NULL) {
			calleeFunc->hits += 1;
		}
		// */
		debugger(dbg, noError, tp - (trcptr)pu->bp, NULL, caller, callee);
	}
	return noError;
}

vmError dbgError(dbgContext ctx, vmError err, size_t ss, void *sp, size_t caller, size_t callee) {
	if (err == noError) {
		return noError;
	}

	const rtContext rt = ctx->rt;
	const char *errMsg = vmErrorMessage(err);
	vmInstruction ip = vmPointer(rt, caller);

	// source code position
	const char *file = NULL;
	int line = 0;
	dbgn dbg = mapDbgStatement(rt, caller, NULL);
	if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
		file = dbg->file;
		line = dbg->line;
	}

	// current function
	symn fun = rtLookup(rt, caller, 0);
	size_t offs = caller;
	if (fun != NULL) {
		offs -= fun->offs;
	}

	if (err == nativeCallError) {
		libc *nativeCalls = rt->vm.nfc;
		symn nc = nativeCalls[ip->rel]->sym;
		error(rt, file, line, ERR_EXEC_NATIVE_CALL, errMsg, caller, fun, offs, nc);
	}
	else {
		error(rt, file, line, ERR_EXEC_INSTRUCTION, errMsg, caller, fun, offs, ip);
	}
	// print stack trace including this function
	traceCalls(rt->dbg, rt->logFile, 1, maxLogItems, 0);
	return err;
	(void) callee;
	(void) sp;
	(void) ss;
}

/**
 * Execute bytecode.
 *
 * @param ctx Runtime context.
 * @param pu Cell to execute on.
 * @param fun Executing function.
 * @param extra Extra data for native calls.
 * @return Error code of execution, noError on success.
 */
static vmError exec(rtContext ctx, vmProcessor pu, symn fun, const void *extra) {
	libc *nativeCalls = ctx->vm.nfc;

	vmError execError = noError;
	const size_t ms = ctx->_size;			// memory size
	const size_t ro = ctx->vm.ro;			// read only region
	const memptr mp = (void*)ctx->_mem;
	const stkptr st = (void*)(pu->bp + pu->ss);

	if (fun == NULL || fun->offs == 0) {
		error(ctx, NULL, 0, ERR_EXEC_FUNCTION, fun);
		return illegalState;
	}

	// run in debug or profile mode
	const dbgContext dbg = ctx->dbg;
	if (dbg != NULL && dbg->debug != NULL) {
		const trcptr oldTP = pu->tp;
		const stkptr spMin = (stkptr)(pu->bp);
		const stkptr spMax = (stkptr)(pu->bp + pu->ss);
		const vmInstruction ipMin = (vmInstruction)(ctx->_mem + ctx->vm.ro);
		const vmInstruction ipMax = (vmInstruction)(ctx->_mem + ctx->vm.px);

		vmError (*debugger)(dbgContext, vmError, size_t, void*, size_t, size_t) = dbg->debug;
		// invoked function(from external code) will return with a ret instruction, removing trace info
		execError = vmTrace(ctx, pu->sp, 0, fun->offs);
		if (execError != noError) {
			debugger(dbg, execError, 0, pu->sp, vmOffset(ctx, pu->ip), 0);
			return execError;
		}

		for ( ; ; ) {
			const vmInstruction ip = (vmInstruction)pu->ip;
			const size_t pc = vmOffset(ctx, ip);
			const stkptr sp = pu->sp;

			if (ip > ipMax || ip < ipMin) {
				debugger(dbg, illegalState, st - sp, sp, pc, 0);
				fatal(ERR_INTERNAL_ERROR": invalid instruction pointer");
				return illegalState;
			}
			if (sp > spMax || sp < spMin) {
				debugger(dbg, illegalState, st - sp, sp, pc, 0);
				fatal(ERR_INTERNAL_ERROR": invalid stack pointer");
				return illegalState;
			}

			vmError error = debugger(dbg, noError, st - sp, sp, pc, 0);
			if (error != noError) {
				// abort execution from debugger
				return error;
			}
			switch (ip->opc) {
				dbg_stop_vm:	// halt virtual machine
					if (execError != noError) {
						debugger(dbg, execError, st - sp, sp, pc, 0);
						while (pu->tp != oldTP) {
							vmTrace(ctx, NULL, 0, (size_t) -2);
						}
					}
					while (pu->tp != oldTP) {
						vmTrace(ctx, NULL, 0, (size_t) -1);
					}
					return execError;

				dbg_error_opc:
					execError = illegalInstruction;
					goto dbg_stop_vm;

				dbg_error_ovf:
					execError = stackOverflow;
					goto dbg_stop_vm;

				dbg_error_mem:
					execError = illegalMemoryAccess;
					goto dbg_stop_vm;

				dbg_error_div_flt:
					debugger(dbg, divisionByZero, st - sp, sp, pc, 0);
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
				#define EXEC
				#define TRACE(__IP) do { if ((execError = vmTrace(ctx, sp, pc, __IP)) != noError) goto dbg_stop_vm; } while(0)
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
					dbgError(&dbgRec, execError, st - sp, sp, vmOffset(ctx, ip), 0);
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
			#define TRACE(__IP)
			#define EXEC
			#include "exec.i"
		}
	}
	return execError;
}

vmError invoke(nfcContext ctx, symn fun, void *res, void *args, const void *extra) {
	rtContext rt = ctx->rt;
	const vmProcessor pu = rt->vm.cell;

	if (pu == NULL) {
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
	void *ip = pu->ip;
	void *sp = pu->sp;
	void *tp = pu->tp;
	int try = pu->try;

	// make space for result and arguments
	pu->sp -= argSize / vm_stk_align;

	if (args != NULL) {
		memcpy((char *)pu->sp, args, argSize - resSize);
	}
	if (ctx->proto == sys_try_exec) {
		pu->try = 1;
	}

	// return here: vm->px: program halt
	*(pu->sp -= 1) = rt->vm.px;

	pu->ip = vmPointer(rt, fun->offs);
	vmError result = exec(rt, pu, fun, extra);
	if (result == noError && res != NULL) {
		memcpy(res, (memptr) sp - resSize, resSize);
	}

	pu->ip = ip;
	pu->sp = sp;
	pu->tp = tp;	// abort during exec
	pu->try = try;

	return result;
}
vmError execute(rtContext ctx, void *extra) {
	// TODO: cells should be in runtime read only memory?
	vmProcessor pu;

	// invalidate compiler
	ctx->cc = NULL;

	// invalidate memory manager
	ctx->vm.heap = NULL;

	// allocate processor(s)
	ctx->_end = ctx->_mem + ctx->_size;
	ctx->_end -= sizeof(struct vmProcessor);
	ctx->vm.cell = pu = (void*)ctx->_end;

	logif(ctx->_size != padOffset(ctx->_size, vm_mem_align), ERR_INTERNAL_ERROR);
	logif(ctx->_mem !=  padPointer(ctx->_mem, vm_mem_align), ERR_INTERNAL_ERROR);
	logif(ctx->_end !=  padPointer(ctx->_end, vm_stk_align), ERR_INTERNAL_ERROR);

	if (ctx->vm.ss == 0) {
		ctx->vm.ss = ctx->_size / 4;
	}
	ctx->_end -= ctx->vm.ss;

	pu->ss = ctx->vm.ss;
	pu->bp = ctx->_end;
	pu->tp = (trcptr)ctx->_end;
	pu->sp = (stkptr)(ctx->_end + ctx->vm.ss);
	pu->ip = ctx->_mem + ctx->vm.pc;

	if (ctx->_beg > ctx->_end) {
		fatal(ERR_INTERNAL_ERROR": memory overrun");
		return illegalState;
	}
	if (pu->bp > (memptr)pu->sp) {
		fatal(ERR_INTERNAL_ERROR": invalid stack size");
		return illegalState;
	}

	return exec(ctx, pu, ctx->main, extra);
}

int isChecked(rtContext rt) {
	return ((vmProcessor) rt->vm.cell)->try;
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

	if (out == NULL) {
		out = ctx->logFile;
	}
	if (file == NULL) {
		file = "native.code";
	}
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
void traceCalls(dbgContext ctx, FILE *out, int indent, size_t maxCalls, size_t skipCalls) {
	if (ctx == NULL) {
		return;
	}
	rtContext rt = ctx->rt;
	vmProcessor pu = rt->vm.cell;
	trcptr trcBase = (trcptr)pu->bp;
	size_t maxTrace = pu->tp - (trcptr)pu->bp;
	size_t hasOutput = 0;
	const char **esc = NULL;

	if (out == NULL) {
		out = rt->logFile;
	}

	maxCalls += skipCalls;
	if (maxCalls > maxTrace) {
		maxCalls = maxTrace;
	}

	for (size_t i = skipCalls; i < maxCalls; ++i) {
		trcptr trace = &trcBase[maxTrace - i - 1];
		dbgn trInfo = mapDbgStatement(rt, trace->caller, NULL);
		symn fun = rtLookup(rt, trace->callee, 0);
		stkptr sp = trace->sp;
		const char *file = NULL;
		int line = 0;

		dieif(fun == NULL, ERR_INTERNAL_ERROR);

		if (trInfo != NULL) {
			file = trInfo->file;
			line = trInfo->line;
		}
		if (hasOutput > 0) {
			printFmt(out, esc, "\n");
		}
		traceArgs(rt, out, fun, file, line, sp, indent, prArgs | prOneLine);
		hasOutput += 1;
	}
	if (maxCalls < maxTrace) {
		if (hasOutput > 0) {
			printFmt(out, esc, "\n");
		}
		printFmt(out, esc, "%I... %d more", indent, maxTrace - maxCalls);
		hasOutput += 1;
	}

	if (hasOutput > 0) {
		printFmt(out, esc, "\n");
	}
}
