#include "tree.h"
#include "type.h"
#include "code.h"
#include "debuger.h"
#include "printer.h"
#include "compiler.h"

// forward declarations
static ccKind genAst(ccContext ctx, astn ast, ccKind get);
static ccKind genVariable(ccContext ctx, symn variable, ccKind get, astn ast);

static inline int isTypeCast(symn sym) {
	if (sym->params == NULL || sym->tag == NULL) {
		return 0;
	}
	// the name of the function is the name of the returned type
	return sym->params->type->name == sym->tag->id.name;
}

/// Utility function to use the correct opcode in 32/64 bit
static inline vmOpcode vmSelect(vmOpcode opc32, vmOpcode opc64) {
	return vm_ref_size == 8 ? opc64 : opc32;
}

/// Utility function to get the absolute position on stack, of a relative offset
static inline size_t stkOffset(rtContext ctx, size_t size) {
	logif(size > ctx->_size, "Error(expected: %d, actual: %d)", ctx->_size, size);
	return padOffset(size, vm_stk_align) + ctx->vm.ss * vm_stk_align;
}

/// Emit an offset: address or index (32 or 64 bit based on vm size)
static inline size_t emitOffs(rtContext ctx, size_t value) {
	vmValue arg = {0};
	arg.i64 = value;
	return emitOpc(ctx, vmSelect(opc_lc32, opc_lc64), arg);
}

static inline void addJump(ccContext ctx, size_t offs, ccToken kind, astn node) {
	jumpToFix jmp = insBuff(&ctx->jumps, ctx->jumps.cnt, NULL);
	if (jmp == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return;
	}
	jmp->kind = kind;
	jmp->offs = offs;
	jmp->node = node;
}
static inline void fixJumps(ccContext ctx, size_t first, size_t last, size_t jmpStop, size_t jmpNext) {
	for (size_t i = first; i < last; i++) {
		jumpToFix jmp = getBuff(&ctx->jumps, i);
		switch (jmp->kind) {
			default :
				fatal(ERR_INTERNAL_ERROR"%t", jmp->node);
				return;

			case TOKEN_any:
				break;

			case OPER_all:
			case STMT_brk:
				fixJump(ctx->rt, jmp->offs, jmpStop, -1);
				break;

			case OPER_any:
			case STMT_con:
				fixJump(ctx->rt, jmp->offs, jmpNext, -1);
				break;
		}
		jmp->kind = TOKEN_any;
	}
	if (ctx->jumps.cnt == last) {
		ctx->jumps.cnt = first;
	}
}

/// Emit an instruction indexing nth element on stack.
static inline size_t emitStack(rtContext ctx, vmOpcode opc, ssize_t arg) {
	vmValue tmp = {0};
	tmp.u64 = ctx->vm.ss * vm_stk_align - arg;

	switch (opc) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return 0;

		case opc_drop:
		case opc_ldsp:
			if (tmp.u64 > ctx->vm.ss * vm_stk_align) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return 0;
			}
			break;

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			tmp.u64 /= vm_stk_align;
			if (tmp.u64 > vm_regs) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return 0;
			}
			break;
	}

	return emitOpc(ctx, opc, tmp);
}

/// Emit the address of the variable.
static inline size_t emitVarOffs(rtContext ctx, symn var) {
	if (!isStatic(var)) {
		return emitStack(ctx, opc_ldsp, var->offs);
	}
	return emitInt(ctx, opc_lref, var->offs);
}

/// Generate byte-code for OPER_all and OPER_any usig jumps `a && b || c`.
static ccKind genCondition(ccContext ctx, astn ast) {
	if (ast == NULL || ast->type == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return CAST_any;
	}

	dbgCgen("%?s:%?u: `%t`: %T -> %K", ast->file, ast->line, ast, ast->type, CAST_bit);
	// generate instructions
	switch (ast->kind) {
		default:
			return genAst(ctx, ast, CAST_bit);

		case OPER_all:		// '&&'
		case OPER_any:		// '||'
			if (!genCondition(ctx, ast->op.lhso)) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			vmOpcode opc = ast->kind != OPER_all ? opc_jnz : opc_jz;
			size_t jmpOffs = emit(ctx->rt, opc);
			if (jmpOffs == 0) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			addJump(ctx, jmpOffs, ast->kind, ast);

			if (!genCondition(ctx, ast->op.rhso)) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			#ifdef DEBUGGING	// extra check: validate some conditions.
			dieif(ast->op.lhso->type != ctx->type_bol, ERR_INTERNAL_ERROR);
			dieif(ast->op.rhso->type != ctx->type_bol, ERR_INTERNAL_ERROR);
			#endif
			break;
	}
	return CAST_bit;
}
/// Generate byte-code from STMT_if.
static ccKind genBranch(ccContext ctx, astn ast) {
	rtContext rt = ctx->rt;
	if (ast->stmt.init && !genAst(ctx, ast->stmt.init, CAST_vid)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	struct astNode testValue;
	if (eval(ctx, &testValue, ast->stmt.test)) {
		if (ast->kind == STMT_sif || rt->foldConst) {
			// generate only 'then' or 'else' part according to the value of the condition
			astn part = bolValue(&testValue) ? ast->stmt.stmt : ast->stmt.step;
			// in case of 'static if', do not clean up local variables
			ccKind cast = ast->kind == STMT_sif ? CAST_any : CAST_vid;

			if (part != NULL && !genAst(ctx, part, cast)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			return CAST_vid;
		}
	}

	if (ast->kind == STMT_sif) {
		error(rt, ast->file, ast->line, ERR_INVALID_CONST_EXPR, ast->stmt.test);
		return CAST_any;
	}

	if (ast->stmt.stmt == NULL && ast->stmt.step == NULL) {
		// if (condition) {} else {}
		if (!genAst(ctx, ast->stmt.test, CAST_vid)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
		warn(rt, ast->file, ast->line, WARN_EMPTY_STATEMENT, ast);
		return CAST_vid;
	}

	// generate condition, then and else branches if necessary
	size_t thisJumps = ctx->jumps.cnt;
	if (!genCondition(ctx, ast->stmt.test)) {
		dbgTraceAst(ast);
		return CAST_any;
	}
	size_t endJumps = ctx->jumps.cnt;

	size_t jmpTrue = emit(rt, opc_jz);
	if (jmpTrue <= 0) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t stmtThen = emit(rt, markIP);
	if (ast->stmt.stmt && !genAst(ctx, ast->stmt.stmt, CAST_vid)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t jmpFalse = 0;
	if (rt->vm.pc == jmpTrue) {
		// no then branch generated
		rollbackPc(rt);
		jmpFalse = emit(rt, opc_jnz);
		jmpTrue = 0;
	} else {
		jmpFalse = emit(rt, opc_jmp);
	}

	if (jmpFalse <= 0) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t stmtElse = emit(rt, markIP);
	if (ast->stmt.step && !genAst(ctx, ast->stmt.step, CAST_vid)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	if (rt->vm.pc == jmpFalse) {
		// no else branch generated
		rollbackPc(rt);
		stmtElse = jmpFalse;
		jmpFalse = 0;
	}

	fixJump(rt, jmpTrue, stmtElse, -1);
	fixJump(rt, jmpFalse, emit(rt, markIP), -1);
	fixJumps(ctx, thisJumps, endJumps, stmtElse, stmtThen);
	return CAST_vid;
}
/// Generate byte-code from STMT_for.
static ccKind genLoop(ccContext ctx, astn ast) {
	rtContext rt = ctx->rt;
	size_t thisJumps = ctx->jumps.cnt;
	size_t spBegin = stkOffset(rt, 0);

	if (ast->stmt.init && !genAst(ctx, ast->stmt.init, CAST_vid)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t jStep = emit(rt, opc_jmp);
	if (jStep <= 0) {		// continue;
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t lBody = emit(rt, markIP);
	if (ast->stmt.stmt && !genAst(ctx, ast->stmt.stmt, CAST_vid)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t lCont = emit(rt, markIP);
	if (ast->stmt.step && !genAst(ctx, ast->stmt.step, CAST_vid)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t lInc = emit(rt, markIP);
	fixJump(rt, jStep, emit(rt, markIP), -1);
	if (ast->stmt.test) {
		if (!genAst(ctx, ast->stmt.test, CAST_bit)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
		if (!emitInt(rt, opc_jnz, lBody)) {		// continue;
			dbgTraceAst(ast);
			return CAST_any;
		}
	} else {
		if (!emitInt(rt, opc_jmp, lBody)) {		// continue;
			dbgTraceAst(ast);
			return CAST_any;
		}
	}

	size_t lBreak = emit(rt, markIP);
	addDbgStatement(rt, lCont, lInc, ast->stmt.step);
	addDbgStatement(rt, lInc, lBreak, ast->stmt.test);
	fixJumps(ctx, thisJumps, ctx->jumps.cnt, lBreak, lCont);

	// TODO: destroy local variables.
	if (!emitStack(rt, opc_drop, spBegin)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	return CAST_vid;
}

/// Generate byte-code for variable declaration with initialization.
static ccKind genDeclaration(ccContext ctx, symn variable, ccKind get) {
	if (!isVariable(variable)) {
		// function, struct, alias or enum declaration.
		fatal(ERR_INTERNAL_ERROR);
		return CAST_vid;
	}

	if (variable->size == 0) {
		if (variable->type == NULL || variable->type->size == 0) {
			error(ctx->rt, variable->file, variable->line, ERR_EMIT_VARIABLE, variable);
			return CAST_vid;
		}
		if (variable->init == NULL || variable->init != ctx->init_und) {
			// not an undefined initializer: {...}
			warn(ctx->rt, variable->file, variable->line, WARN_VARIABLE_TYPE_INCOMPLETE, variable);
		}
		variable->size = variable->type->size;
	}
	ccKind varCast = refCast(variable);
	if (get != CAST_vid) {
		logif(get != varCast, "%?s:%?u: %T(%K->%K)", variable->file, variable->line, variable, varCast, get);
	}

	rtContext rt = ctx->rt;
	astn varInit = variable->init;
	size_t varOffset = stkOffset(rt, variable->size);
	if (varInit == NULL) {
		varInit = variable->type->init;
		if (varInit != NULL) {
			debug(ctx->rt, variable->file, variable->line, WARN_USING_DEF_TYPE_INITIALIZER, variable, varInit);
		}
		else if (!isMutable(variable)) {
			error(rt, variable->file, variable->line, ERR_UNINITIALIZED_CONSTANT, variable);
		}
		else if (isInvokable(variable)) {
			error(rt, variable->file, variable->line, ERR_UNIMPLEMENTED_FUNCTION, variable);
		}
		else if (isFixedSizeArray(variable->type)) {
			warn(rt, variable->file, variable->line, ERR_UNINITIALIZED_ARRAY, variable);
		}
		else if (ctx->errUninitialized) {
			error(rt, variable->file, variable->line, ERR_UNINITIALIZED_VARIABLE, variable);
		}
		else {
			warn(rt, variable->file, variable->line, ERR_UNINITIALIZED_VARIABLE, variable);
		}
	}

	if (variable->offs == 0) {
		// local variable => on stack
		if (varInit == NULL || varInit->kind == STMT_beg) {
			if (!emitInt(rt, opc_spc, padOffset(variable->size, vm_stk_align))) {
				return CAST_any;
			}
			variable->offs = stkOffset(rt, 0);
		}
		if (varInit != NULL) {
			if (!(varCast = genAst(ctx, varInit, varCast))) {
				dbgTraceAst(varInit);
				return CAST_any;
			}
			variable->offs = stkOffset(rt, 0);
		}
		dbgInfo("%?s:%?u: %.T is local(@%06x)", variable->file, variable->line, variable, variable->offs);
		if (varOffset != variable->offs) {
			dbgTrace(ERR_INTERNAL_ERROR);
			return CAST_any;
		}
	}
	else if (!isInline(variable)) {
		// global variable or function argument
		if (varInit != NULL) {
			if (varCast != genAst(ctx, varInit, varCast)) {
				dbgTraceAst(varInit);
				return CAST_any;
			}
			if (varInit->kind != STMT_beg) {
				if (!emitVarOffs(rt, variable)) {
					return CAST_any;
				}
				if (!emitInt(rt, opc_sti, variable->size)) {
					return CAST_any;
				}
			}
		}
		if (isStatic(variable)) {
			dbgInfo("%?s:%?u: %.T is global(@%06x)", variable->file, variable->line, variable, variable->offs);
		} else {
			dbgInfo("%?s:%?u: %.T is param(@%06x)", variable->file, variable->line, variable, variable->offs);
		}
	}
	return varCast;
}

/// Generate byte-code for variable indirect load.
static ccKind genIndirection(ccContext ctx, symn variable, ccKind get, int isIndex) {
	if (get == KIND_var) {
		// load only the offset of the variable, no matter if it's a reference or not
		// HACK: currently used to get the length of a slice
		return CAST_ptr;
	}

	rtContext rt = ctx->rt;
	symn type = variable->type;
	ccKind cast = refCast(variable);

	switch (variable->kind & MASK_kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;

		case KIND_typ:
		case KIND_fun:
			// typename is by reference, ex: pointer b = int32;
			if (!isIndex) {
				cast = CAST_any;
			}
			type = variable;
			break;

		case KIND_var:
			if (isEnumType(type)) {
				type = type->type;
			}
			break;
	}

	if (get == CAST_ptr || get == CAST_obj || get == CAST_arr || get == CAST_var) {
		if (cast == get) {
			// make a copy of a reference, variant or slice
			if (!emitInt(rt, opc_ldi, refSize(variable))) {
				return CAST_any;
			}
			return get;
		}

		if (cast == CAST_ptr || cast == CAST_obj || cast == CAST_arr || cast == CAST_var) {
			// convert a variant or an array to a reference (ex: `int &a = var;`)
			if (!emitInt(rt, opc_ldi, vm_ref_size)) {
				return CAST_any;
			}
			return get;
		}

		// use the offset of the variable.
		return get;
	}

	if (cast == CAST_ptr || cast == CAST_obj) {
		// load reference indirection (variable is a reference to a value)
		if (!emitInt(rt, opc_ldi, vm_ref_size)) {
			return CAST_any;
		}
	}

	// load the value of the variable
	if (cast == CAST_u32) {
		int size = refSize(type);
		switch (size) {
			case 1:
				if (!emit(rt, opc_ldiu1)) {
					return CAST_any;
				}
				break;

			case 2:
				if (!emit(rt, opc_ldiu2)) {
					return CAST_any;
				}
				break;

			default:
				if (!emitInt(rt, opc_ldi, size)) {
					return CAST_any;
				}
				break;
		}
	} else {
		if (!emitInt(rt, opc_ldi, refSize(type))) {
			return CAST_any;
		}
	}

	return castOf(type);
}
static ccKind genPreVarLen(ccContext ctx, astn ast, symn type, ccKind get, ccKind got) {
	rtContext rt = ctx->rt;

	// array: push length of variable first or copy.
	if (get == CAST_arr && got != CAST_arr) {
		symn length = type->fields;
		if (length == NULL || !isStatic(length)) {
			error(rt, ast->file, ast->line, ERR_EMIT_LENGTH, NULL);
			return CAST_any;
		}
		if (!genVariable(ctx, length, refCast(length), ast)) {
			return CAST_any;
		}
	}

	// variant: push type of variable first or copy.
	if (get == CAST_var && got != CAST_var) {
		if (!emitVarOffs(rt, type)) {
			return CAST_any;
		}
	}
	return get;
}

/// Generate byte-code for variable usage.
static ccKind genVariable(ccContext ctx, symn variable, ccKind get, astn ast) {
	rtContext rt = ctx->rt;
	symn type = variable->type;
	ccKind got = castOf(variable);

	dbgCgen("%?s:%?u: `%T`: %K -> %K", ast->file, ast->line, variable, variable->kind, get);
	if (variable == ctx->null_ref) {
		switch (get) {
			default:
				dbgTraceAst(ast);
				return CAST_any;

			case CAST_var:
				// variant a = null;
				if (!emitVarOffs(rt, type)) {
					return CAST_any;
				}
				break;

			case CAST_arr:
				// int a[] = null;
				if (!emitOffs(rt, 0)) {
					return CAST_any;
				}
				break;

			case CAST_ptr:
			case CAST_obj:
				// pointer a = null;
				break;
		}
		if (!emitInt(rt, opc_lref, 0)) {	//push reference
			return CAST_any;
		}
		return get;
	}

	switch (variable->kind & MASK_kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;

		case KIND_def:
			// generate inline
			return genAst(ctx, variable->init, get);

		case KIND_typ:
		case KIND_fun:
			// typename is by reference
			got = CAST_ptr;
			break;

		case KIND_var:
			break;
	}

	if (variable->offs == 0) {
		error(rt, ast->file, ast->line, ERR_EMIT_VARIABLE, variable);
	}

	if (!genPreVarLen(ctx, ast, type, get, got)) {
		return CAST_any;
	}

	// generate the address of the variable
	if (!emitVarOffs(rt, variable)) {
		return CAST_any;
	}

	return genIndirection(ctx, variable, get, 0);
}
/// Generate byte-code for OPER_dot `a.b`.
static ccKind genMember(ccContext ctx, astn ast, ccKind get) {
	rtContext rt = ctx->rt;
	symn object = linkOf(ast->op.lhso, 1);
	symn member = linkOf(ast->op.rhso, 1);

	dbgCgen("%?s:%?u: %t", ast->file, ast->line, ast);
	if (member == NULL && ast->op.rhso != NULL) {
		astn call = ast->op.rhso;
		if (call->kind == OPER_fnc) {
			member = linkOf(call->op.lhso, 1);
		}
	}
	if (object == NULL && ast->op.lhso != NULL) {
		// we may have an expression that is not a variable: a[0].x;
		object = ast->op.lhso->type;
	}

	if (!object || !member) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	if (isStatic(member)) {
		if (isVariable(object) && !isArrayType(object->type)) {
			warn(rt, ast->file, ast->line, WARN_STATIC_FIELD_ACCESS, member, object->type);
		}
		return genAst(ctx, ast->op.rhso, get);
	}

	if (isTypeExpr(ast->op.lhso)) {
		// check what is on the left side
		error(rt, ast->file, ast->line, ERR_INVALID_FIELD_ACCESS, member);
		return CAST_any;
	}

	if (!genPreVarLen(ctx, ast, member->type, get, castOf(member))) {
		return CAST_any;
	}

	// HACK: dynamic array length: do not load indirect the address of the first element
	ccKind lhsCast = member == ctx->length_ref ? KIND_var : CAST_ptr;
	if (!genAst(ctx, ast->op.lhso, lhsCast)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	if (!emitInt(rt, opc_inc, member->offs)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	return genIndirection(ctx, member, get, 0);
}
/// Generate byte-code for OPER_idx `a[b]`.
static ccKind genIndex(ccContext ctx, astn ast, ccKind get) {
	rtContext rt = ctx->rt;
	struct astNode tmp = {0};

	if (!genPreVarLen(ctx, ast, ast->type, get, refCast(ast->type))) {
		return CAST_any;
	}

	dbgCgen("%?s:%?u: %t", ast->file, ast->line, ast);
	if (!genAst(ctx, ast->op.lhso, CAST_ptr)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t elementSize = refSize(ast->type);	// size of array element
	if (rt->foldConst && eval(ctx, &tmp, ast->op.rhso) == CAST_i64) {
		size_t offs = elementSize * intValue(&tmp);
		if (!emitInt(rt, opc_inc, offs)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
	}
	else {
		if (!genAst(ctx, ast->op.rhso, refCast(ctx->type_int))) {
			dbgTraceAst(ast);
			return CAST_any;
		}
		if (!emitInt(rt, opc_mad, elementSize)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
	}

	return genIndirection(ctx, ast->type, get, 1);
}
/// Generate byte-code for OPER_idx `a[...n]`.
static ccKind genSlice(ccContext ctx, astn ast) {
	if (ast->op.rhso == NULL || ast->op.rhso->kind != PNCT_dot3) {
		return CAST_any;
	}

	if (ast->op.rhso->op.lhso != NULL) {
		// not supported yet: `a[1 ... n]`.
		return CAST_any;
	}

	if (!genAst(ctx, ast->op.rhso->op.rhso, refCast(ctx->type_int))) {
		return CAST_any;
	}

	if (!genAst(ctx, ast->op.lhso, CAST_ptr)) {
		return CAST_any;
	}

	return CAST_arr;
}

/// Generate byte-code for OPER_fnc `a(b)`.
static ccKind genEmit(ccContext ctx, astn ast, ccKind get) {
	rtContext rt = ctx->rt;
	symn function = linkOf(ast->op.lhso, 0);
	if (function == NULL || function != ctx->emit_opc) {
		error(rt, ast->file, ast->line, ERR_INVALID_CAST, ast);
		dbgTraceAst(ast);
		return CAST_any;
	}

	// emit intrinsic
	astn args = chainArgs(ast->op.rhso);
	const size_t locals = stkOffset(rt, 0);
	dbgCgen("%?s:%?u: emit: %t", ast->file, ast->line, ast);
	// Reverse Polish notation: int32 c = emit(int32(a), int32(b), i32.add);
	for (astn arg = args; arg != NULL; arg = arg->next) {
		dbgCgen("%?s:%?u: emit.arg: %t", args->file, args->line, args);
		if (!genAst(ctx, arg, CAST_any)) {
			dbgTraceAst(args);
			return CAST_any;
		}
		// extra check to not underflow the stack
		if (stkOffset(rt, 0) < locals) {
			error(rt, ast->file, ast->line, ERR_EMIT_STATEMENT, ast);
		}

		if (arg->kind == OPER_fnc && arg->op.lhso != NULL) {
			symn lnk = linkOf(arg->op.lhso, 1);
			if (lnk != NULL && lnk == arg->type) {
				// old type cast: pointer(value)
				continue;
			}
			if (lnk != NULL && isTypeCast(lnk)) {
				// new type cast: int32(value)
				continue;
			}
		}

		symn lnk = linkOf(arg, 1);
		if (lnk != NULL && lnk->init && lnk->init->kind == TOKEN_opc) {
			// instruction: add.i32
			continue;
		}

		if (ast->file == NULL && ast->line == 0) {
			// generated emit, do not warn
			continue;
		}
		if (lnk != NULL && arg == args && arg->next == NULL) {
			// single argument emit: emit(v), do not warn
			continue;
		}
		warn(rt, ast->file, ast->line, WARN_PASS_ARG_NO_CAST, arg, arg->type);
	}
	return get;
}

// TODO: simplify
static ccKind genCall(ccContext ctx, astn ast, ccKind get) {
	rtContext rt = ctx->rt;
	symn function = linkOf(ast->op.lhso, 0);
	if (function != NULL && isTypename(function)) {
		// cast the result of the emit to type: int32(emit(value))
		astn arg = ast->op.rhso;
		if (arg != NULL && arg->kind == OPER_fnc) {
			if (linkOf(arg->op.lhso, 0) == ctx->emit_opc) {
				size_t offs = stkOffset(rt, padOffset(function->size, vm_stk_align));
				ccKind result = genEmit(ctx, ast->op.rhso, get);
				if (offs != stkOffset(rt, 0)) {
					error(rt, ast->file, ast->line, ERR_INVALID_CAST, ast);
				}
				return result;
			}
		}
	}

	if (ast->op.lhso && ast->op.lhso->kind == RECORD_kwd) {
		dieif(function != ctx->type_rec, ERR_INTERNAL_ERROR);
		if (!emitVarOffs(rt, ast->op.rhso->type)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
		return castOf(ctx->type_rec);
	}

	if (ast->op.lhso && ast->op.lhso->kind == INLINE_kwd) {
		dieif (function != ctx->emit_opc, ERR_INTERNAL_ERROR);
		return genEmit(ctx, ast, get);
	}
	if (function == ctx->type_vid) {
		return genAst(ctx, ast->op.rhso, CAST_vid);
	}

	if (function == ctx->type_var || function == ctx->type_ptr) {
		astn arg = ast->op.rhso;
		// casts may have a single argument
		if (arg == NULL || arg->kind == OPER_com) {
			error(rt, ast->file, ast->line, ERR_INVALID_CAST, ast);
			return CAST_any;
		}

		ccKind cast = refCast(function);
		return genAst(ctx, arg, cast);
	}

	if (function == NULL || function->params == NULL) {
		error(rt, ast->file, ast->line, ERR_EMIT_STATEMENT, ast);
		dbgTraceAst(ast);
		return CAST_any;
	}

	struct astNode extraFile = {0};
	struct astNode extraLine = {0};
	struct astNode extraResult = {0};
	size_t offsets[maxTokenCount];
	astn values[maxTokenCount];
	symn vaElemType = NULL;
	astn vaList = NULL;

	// save parameter offsets and init value
	if (function->params != NULL) {
		// add extra (file, line, ...) arguments to the raise builtin function
		astn arg = chainArgs(ast->op.rhso);
		symn prm = function->params;
		int argc = 0;

		if (function == ctx->libc_dbg) {
			const char *file = ast->file;
			int line = ast->line;

			// use the location of the expression statement, not the expansion
			if (ctx->file != NULL && ctx->line > 0) {
				file = ctx->file;
				line = ctx->line;
				dbgCgen("%?s:%?u: Using the location of the last expression statement", file, line);
			}

			// add two extra computed arguments to the raise function(file and line)
			extraFile.kind = TOKEN_val;
			extraLine.kind = TOKEN_val;
			extraFile.type = function->params->next->type;
			extraLine.type = function->params->next->next->type;
			extraFile.id.name = file;
			extraLine.cInt = line;

			// chain the new arguments
			extraFile.next = &extraLine;
			extraLine.next = arg;
			arg = &extraFile;
		}

		if (function->params->init != NULL) {
			extraResult = *function->params->init;
		}

		extraResult.type = function->params->type;
		extraResult.next = arg;
		arg = &extraResult;

		while (prm != NULL) {
			if (argc >= maxTokenCount) {
				error(rt, ast->file, ast->line, ERR_EXPR_TOO_COMPLEX, ast);
				dbgTraceAst(ast);
				return CAST_any;
			}
			offsets[argc] = prm->offs;
			values[argc] = prm->init;
			prm->init = arg;
			argc += 1;

			// advance
			if ((prm->kind & ARGS_varg) != 0 && (arg == NULL || arg->kind != PNCT_dot3)) {
				vaElemType = prm->type->type;
				vaList = arg;
				arg = NULL;
			}
			prm = prm->next;
			if (arg == NULL) {
				break;
			}
			arg = arg->next;
		}

		// more or less args than params is a fatal error
		// should have been raised by the type checker
		if (prm != NULL || arg != NULL) {
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;
		}
	}

	const ccKind resultCast = refCast(function->params->type);
	const size_t resultOffs = stkOffset(rt, function->params->size);
	struct {
		struct symNode var;
		struct astNode ast;
	} tempArguments[8], *temp = tempArguments;

	// first make an array of varargs
	ssize_t vaLength = 0;
	if (vaElemType != NULL) {
		size_t vaStore = 0;
		if (vaElemType->size % vm_stk_align != 0) {
			vaStore = vaElemType->size;

			size_t vaAlloc = 0;
			for (astn arg = vaList; arg; arg = arg->next) {
				vaAlloc += vaStore;
			}

			vaAlloc = padOffset(vaAlloc, vm_stk_align);
			if (!emitInt(rt, opc_spc, vaAlloc)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
		}
		else {
			// reverse the list of arguments: stack works in reverse order
			astn reverse = NULL;
			for (astn arg = vaList; arg; ) {
				astn next = arg->next;
				arg->next = reverse;
				reverse = arg;
				arg = next;
			}
			vaList = reverse;
		}

		// generate the arguments
		size_t vaOffset = stkOffset(rt, 0);
		for (astn arg = vaList; arg; arg = arg->next) {
			if (!genAst(ctx, arg, refCast(vaElemType))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			if (vaStore != 0) {
				if (!emitStack(rt, opc_ldsp, vaOffset)) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				if (!emitInt(rt, opc_sti, vaStore)) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				vaOffset -= vaStore;
			}
			vaLength += 1;
		}
	}

	// convert const ref parameter arguments to local variables
	for (symn prm = function->params; prm; prm = prm->next) {
		astn arg = prm->init;

		if ((prm->kind & ARGS_varg) != 0 && (arg == NULL || arg->kind != PNCT_dot3)) {
			size_t vaOffset = stkOffset(rt, 0);
			if (!emitOffs(rt, vaLength)) {
				return CAST_any;
			}
			if (!emitStack(rt, opc_ldsp, vaOffset)) {
				return CAST_any;
			}

			if (temp > tempArguments + lengthOf(tempArguments)) {
				fatal(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
			memset(temp, 0, sizeof(*temp));
			temp->var.name = "<.va_arg>";
			temp->var.kind = KIND_var | CAST_arr;
			temp->var.type = prm->type;
			temp->var.init = prm->init;
			temp->var.offs = stkOffset(rt, 0);
			temp->var.size = prm->size;
			temp->var.unit = prm->unit;
			temp->var.file = prm->file;
			temp->var.line = prm->line;

			temp->ast.kind = TOKEN_var;
			temp->ast.type = prm->type;
			temp->ast.id.link = &temp->var;
			temp->ast.id.name = prm->name;
			temp->ast.id.hash = 0;
			temp->ast.id.used = NULL;
			prm->init = &temp->ast;
			temp++;
			break;
		}

		if (!isIndirect(prm)) {
			continue;
		}
		ccKind typCast = refCast(prm->type);
		if (typCast == CAST_ptr || typCast == CAST_obj) {
			continue;
		}

		dieif(arg == NULL, ERR_INTERNAL_ERROR);
		switch (arg->kind) {

			default:
				break;

			case TOKEN_var:
			case OPER_dot:
			case OPER_idx:
			case OPER_adr:
			case PNCT_dot3:
				// if argument is already a variable, do not create a new one
				continue;

			case TOKEN_any:
			case TOKEN_val:
				// no need to extract "string constants" to variable
				if (isIndirect(arg->type)) {
					continue;
				}
				break;
		}

		debug(rt, ast->file, ast->line, WARN_ADDING_TEMPORARY_VAR, prm, arg);
		if (temp > tempArguments + lengthOf(tempArguments)) {
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;
		}

		memset(temp, 0, sizeof(*temp));
		temp->var.kind = KIND_var | typCast;
		temp->var.type = prm->type;
		temp->var.init = prm->init;
		temp->var.offs = 0;
		temp->var.size = prm->type->size;
		temp->var.unit = prm->unit;
		temp->var.file = prm->file;
		temp->var.line = prm->line;

		temp->ast.kind = TOKEN_var;
		temp->ast.type = prm->type;
		temp->ast.id.link = &temp->var;
		temp->ast.id.name = prm->name;
		temp->ast.id.hash = 0;
		temp->ast.id.used = NULL;

		ccKind got = genDeclaration(ctx, &temp->var, CAST_vid);
		if (got == CAST_any) {
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;
		}

		prm->init = &temp->ast;
		temp++;
	}

	// generate arguments (push or cache)
	const size_t locals = stkOffset(rt, 0);
	size_t resultPos = stkOffset(rt, function->params->size);
	if (preAllocateArgs && isFunction(function)) {
		size_t preAlloc = argsSize(function);
		// alloc space for result and arguments
		if (preAlloc > 0 && !emitInt(rt, opc_spc, preAlloc)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
	}

	// generate arguments
	size_t offs = 0;
	for (symn prm = function->params; prm; prm = prm->next) {

		// skip generating void parameter
		if (prm->size == 0) {
			continue;
		}

		// update offset of inline variable
		if (isInline(prm)) {
			prm->offs = locals + offs;
			continue;
		}

		astn arg = prm->init;
		if (arg && arg->kind == PNCT_dot3 && (prm->kind & ARGS_varg) != 0) {
			// drop `...`
			arg = arg->op.rhso;
		}
		if (arg && arg->kind == OPER_adr && isIndirect(prm)) {
			// drop `&`
			arg = arg->op.rhso;
		}

		// allocate space for uninitialized arguments
		if (arg == NULL || arg->kind == TOKEN_any) {
			if (prm->name && *prm->name == '.') {
				debug(rt, ast->file, ast->line, ERR_UNINITIALIZED_VARIABLE, prm);
			}
			else if (ctx->errUninitialized) {
				error(rt, ast->file, ast->line, ERR_UNINITIALIZED_VARIABLE, prm);
			}
			else {
				warn(rt, ast->file, ast->line, ERR_UNINITIALIZED_VARIABLE, prm);
			}
			if (!emitInt(rt, opc_spc, prm->size)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			continue;
		}

		// generate value
		offs += padOffset(prm->size, vm_stk_align);

		// generate the argument value
		dbgCgen("%?s:%?u: %K: `%T` -> %K", ast->file, ast->line, prm->kind, prm, refCast(prm));
		size_t argOffs = stkOffset(rt, prm->size);
		if (!genAst(ctx, arg, refCast(prm))) {
			dbgTraceAst(ast);
			return CAST_any;
		}

		if (argOffs != stkOffset(rt, 0)) {
			fatal(ERR_INTERNAL_ERROR": argument size does not match parameter size");
			dbgTraceAst(ast);
			return CAST_any;
		}

		if (argOffs - locals > prm->offs) {
			if (!emitStack(rt, opc_ldsp, locals + prm->offs)) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
			if (!emitInt(rt, opc_sti, prm->size)) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
		}

		if (isInline(function)) {
			prm->offs = locals + offs;
		}
	}

	if (isInline(function)) {
		// generate inline expression
		if (!genAst(ctx, function->init, resultCast)) {
			dbgTraceAst(function->init);
			return CAST_any;
		}
		resultPos = stkOffset(rt, 0);
	}
	else {
		if (!genAst(ctx, ast->op.lhso, CAST_ptr)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
		if (!emit(rt, opc_call)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
	}

	// restore parameter offsets and reset init value
	int argc = 0;
	for (symn prm = function->params; prm != NULL; prm = prm->next) {
		prm->offs = offsets[argc];
		prm->init = values[argc];
		argc += 1;
	}

	if (resultPos != resultOffs && function->params->size > 0) {
		// move result value and free some temporary locals

		size_t assignBegin = emit(rt, markIP);
		// move result value and free some temporary locals
		if (!emitStack(rt, opc_ldsp, resultPos)) {
			dbgTrace(ERR_INTERNAL_ERROR);
			return CAST_any;
		}
		if (!emitInt(rt, opc_ldi, function->params->size)) {
			dbgTrace(ERR_INTERNAL_ERROR);
			return CAST_any;
		}
		size_t assignEnd = emit(rt, markIP);
		if (!emitStack(rt, opc_ldsp, resultOffs)) {
			dbgTrace(ERR_INTERNAL_ERROR);
			return CAST_any;
		}
		if (!emitInt(rt, opc_sti, function->params->size)) {
			dbgTrace(ERR_INTERNAL_ERROR);
			return CAST_any;
		}

		// optimize generated assignment to a single `set` instruction
		if (foldAssignment(rt, assignBegin, assignEnd)) {
			dbgInfo("assignment optimized: %t", ast);
		}
	}

	// drop cached arguments, except result.
	if (!emitStack(rt, opc_drop, resultOffs)) {
		fatal(ERR_INTERNAL_ERROR);
		return CAST_any;
	}

	return resultCast;
}

/// Emit operator based on type: (add, sub, mul, ...)
// TODO: this should be implemented using operator overloading in `lang.cmpl`
static ccKind genOperator(rtContext ctx, ccToken token, ccKind cast) {
	vmOpcode opc = opc_last;
	switch (token) {
		default:
			// opc = opc_last;
			break;

		case OPER_not:
			switch (cast) {
				default:
					break;

				case CAST_bit:
					opc = opc_not;
					break;
			}
			break;

		case OPER_ceq:
			switch (cast) {
				default:
					break;

				case CAST_ptr:
				case CAST_obj:
					opc = vmSelect(i32_ceq, i64_ceq);
					break;

				case CAST_bit:
				case CAST_i32:
				case CAST_u32:
					opc = i32_ceq;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = i64_ceq;
					break;

				case CAST_f32:
					opc = f32_ceq;
					break;

				case CAST_f64:
					opc = f64_ceq;
					break;
			}
			break;

		case OPER_cne:
			if (!genOperator(ctx, OPER_ceq, cast)) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
			opc = opc_not;
			break;

		case OPER_clt:
			switch (cast) {
				default:
					break;

				case CAST_bit:
				case CAST_i32:
					opc = i32_clt;
					break;

				case CAST_i64:
					opc = i64_clt;
					break;

				case CAST_u32:
					opc = u32_clt;
					break;

				case CAST_u64:
					opc = u64_clt;
					break;

				case CAST_f32:
					opc = f32_clt;
					break;

				case CAST_f64:
					opc = f64_clt;
					break;
			}
			break;

		case OPER_cle:
			if (cast == CAST_f64) {
				if (!emitInt(ctx, opc_dup4, 0)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f64_clt)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_dup4, 1)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f64_ceq)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, u32_ior)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_set1, 4)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_drop, 12)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				opc = opc_nop;
				break;
			}
			if (cast == CAST_f32) {
				if (!emitInt(ctx, opc_dup2, 0)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f32_clt)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_dup2, 1)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f32_ceq)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, u32_ior)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_set1, 2)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_drop, 4)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				opc = opc_nop;
				break;
			}

			if (!genOperator(ctx, OPER_cgt, cast)) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
			opc = opc_not;
			break;

		case OPER_cgt:
			switch (cast) {
				default:
					break;

				case CAST_bit:
				case CAST_i32:
					opc = i32_cgt;
					break;

				case CAST_i64:
					opc = i64_cgt;
					break;

				case CAST_u32:
					opc = u32_cgt;
					break;

				case CAST_u64:
					opc = u64_cgt;
					break;

				case CAST_f32:
					opc = f32_cgt;
					break;

				case CAST_f64:
					opc = f64_cgt;
					break;
			}
			break;

		case OPER_cge:
			if (cast == CAST_f64) {
				if (!emitInt(ctx, opc_dup4, 0)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f64_cgt)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_dup4, 1)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f64_ceq)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, u32_ior)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_set1, 4)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_drop, 12)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				opc = opc_nop;
				break;
			}
			if (cast == CAST_f32) {
				if (!emitInt(ctx, opc_dup2, 0)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f32_cgt)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_dup2, 1)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, f32_ceq)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emit(ctx, u32_ior)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_set1, 2)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				if (!emitInt(ctx, opc_drop, 4)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return CAST_any;
				}
				opc = opc_nop;
				break;
			}
			if (!genOperator(ctx, OPER_clt, cast)) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
			opc = opc_not;
			break;

		case OPER_mns:
			switch (cast) {
				default:
					break;

				case CAST_i32:
				case CAST_u32:
					opc = i32_neg;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = i64_neg;
					break;

				case CAST_f32:
					opc = f32_neg;
					break;

				case CAST_f64:
					opc = f64_neg;
					break;
			}
			break;

		case OPER_add:
			switch (cast) {
				default:
					break;

				case CAST_i32:
				case CAST_u32:
					opc = i32_add;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = i64_add;
					break;

				case CAST_f32:
					opc = f32_add;
					break;

				case CAST_f64:
					opc = f64_add;
					break;
			}
			break;

		case OPER_sub:
			switch (cast) {
				default:
					break;

				case CAST_i32:
				case CAST_u32:
					opc = i32_sub;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = i64_sub;
					break;

				case CAST_f32:
					opc = f32_sub;
					break;

				case CAST_f64:
					opc = f64_sub;
					break;
			}
			break;

		case OPER_mul:
			switch (cast) {
				default:
					break;

				case CAST_i32:
					opc = i32_mul;
					break;

				case CAST_i64:
					opc = i64_mul;
					break;

				case CAST_u32:
					opc = u32_mul;
					break;

				case CAST_u64:
					opc = u64_mul;
					break;

				case CAST_f32:
					opc = f32_mul;
					break;

				case CAST_f64:
					opc = f64_mul;
					break;
			}
			break;

		case OPER_div:
			switch (cast) {
				default:
					break;

				case CAST_i32:
					opc = i32_div;
					break;

				case CAST_i64:
					opc = i64_div;
					break;

				case CAST_u32:
					opc = u32_div;
					break;

				case CAST_u64:
					opc = u64_div;
					break;

				case CAST_f32:
					opc = f32_div;
					break;

				case CAST_f64:
					opc = f64_div;
					break;
			}
			break;

		case OPER_mod:
			switch (cast) {
				default:
					break;

				case CAST_i32:
					opc = i32_rem;
					break;

				case CAST_i64:
					opc = i64_rem;
					break;

				case CAST_u32:
					opc = u32_rem;
					break;

				case CAST_u64:
					opc = u64_rem;
					break;

				case CAST_f32:
					opc = f32_rem;
					break;

				case CAST_f64:
					opc = f64_rem;
					break;
			}
			break;

		case OPER_cmt:
			switch (cast) {
				default:
					break;

				case CAST_i32:
				case CAST_u32:
					opc = u32_cmt;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = u64_cmt;
					break;
			}
			break;

		case OPER_shl:
			switch (cast) {
				default:
					break;

				case CAST_i32:
				case CAST_u32:
					opc = u32_shl;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = u64_shl;
					break;
			}
			break;

		case OPER_shr:
			switch (cast) {
				default:
					break;

				case CAST_i32:
					opc = u32_sar;
					break;

				case CAST_i64:
					opc = u64_sar;
					break;

				case CAST_u32:
					opc = u32_shr;
					break;

				case CAST_u64:
					opc = u64_shr;
					break;
			}
			break;

		case OPER_and:
			switch (cast) {
				default:
					break;

				case CAST_bit:
				case CAST_i32:
				case CAST_u32:
					opc = u32_and;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = u64_and;
					break;
			}
			break;

		case OPER_ior:
			switch (cast) {
				default:
					break;

				case CAST_bit:
				case CAST_i32:
				case CAST_u32:
					opc = u32_ior;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = u64_ior;
					break;
			}
			break;

		case OPER_xor:
			switch (cast) {
				default:
					break;

				case CAST_bit:
				case CAST_u32:
				case CAST_i32:
					opc = u32_xor;
					break;

				case CAST_i64:
				case CAST_u64:
					opc = u64_xor;
					break;
			}
			break;
	}

	if (opc != opc_nop && !emit(ctx, opc)) {
		return CAST_any;
	}

	return cast;
}

/// Generate byte-code for OPER_sel `a ? b : c`.
static ccKind genLogical(ccContext ctx, astn ast) {
	rtContext rt = ctx->rt;
	struct astNode tmp;

	dieif(ast->op.rhso == NULL || ast->op.rhso->kind != OPER_cln, ERR_INTERNAL_ERROR);
	if (rt->foldConst && eval(ctx, &tmp, ast->op.lhso) == CAST_bit) {
		if (!genAst(ctx, bolValue(&tmp) ? ast->op.rhso->op.lhso : ast->op.rhso->op.rhso, CAST_any)) {
			dbgTraceAst(ast);
			return CAST_any;
		}
		return refCast(ast->type);
	}

	size_t spBegin = stkOffset(rt, 0);
	if (!genAst(ctx, ast->op.lhso, CAST_bit)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t jmpTrue = emit(rt, opc_jz);
	if (jmpTrue == 0) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	if (!genAst(ctx, ast->op.rhso->op.lhso, CAST_any)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	size_t jmpFalse = emit(rt, opc_jmp);
	if (jmpFalse == 0) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	fixJump(rt, jmpTrue, emit(rt, markIP), spBegin);
	if (!genAst(ctx, ast->op.rhso->op.rhso, CAST_any)) {
		dbgTraceAst(ast);
		return CAST_any;
	}

	fixJump(rt, jmpFalse, emit(rt, markIP), -1);
	return refCast(ast->type);
}

/// Generate bytecode from abstract syntax tree.
static ccKind genAst(ccContext ctx, astn ast, ccKind get) {
	rtContext rt = ctx->rt;

	if (ast == NULL || ast->type == NULL) {
		fatal(ERR_INTERNAL_ERROR);
		return CAST_any;
	}

	const size_t ipBegin = emit(rt, markIP);
	const size_t spBegin = stkOffset(rt, 0);
	const size_t spEnd = stkOffset(rt, refCast(ast->type));
	ccKind op, got = refCast(ast->type);

	if (get == CAST_any) {
		get = got;
	}

	dbgCgen("%?s:%?u: `%t`: %T -> %K", ast->file, ast->line, ast, ast->type, get);
	// generate instructions
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return CAST_any;

		case STMT_beg:
			// statement block or function body or initializer
			for (astn ptr = ast->stmt.stmt; ptr != NULL; ptr = ptr->next) {
				int errors = rt->errors;
				int nested = ptr->kind == STMT_beg;
				size_t ipStart2 = emit(rt, markIP);
				ctx->file = ptr->file;
				ctx->line = ptr->line;
				if (!genAst(ctx, ptr, nested ? get : CAST_any)) {
					if (errors == rt->errors) {
						// report unreported error
						error(rt, ptr->file, ptr->line, ERR_EMIT_STATEMENT, ptr);
					}
					return CAST_any;
				}
				addDbgStatement(rt, ipStart2, emit(rt, markIP), ptr);
			}
			if (get == CAST_vid) {
				// clean the stack: remove local variables
				got = CAST_val;
			}
			if (get == CAST_val) {
				// {...}: may produce local variables
				got = CAST_val;
			}
			break;

		case STMT_end:
			// expression statement
			// save the position for inline calls
			ctx->file = ast->file;
			ctx->line = ast->line;
			if (!genAst(ctx, ast->stmt.stmt, CAST_vid)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			dieif(spBegin != stkOffset(rt, 0), ERR_INTERNAL_ERROR);
			break;

		case STMT_for:
			if (!(got = genLoop(ctx, ast))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);
			break;

		case STMT_if:
		case STMT_sif:
			if (!(got = genBranch(ctx, ast))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);
			break;

		case STMT_con:
		case STMT_brk:
			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);

			size_t stBreak = stkOffset(rt, 0);
			for (symn s = ast->jmp.scope; s != NULL; s = s->scope) {
				if (s->nest <= ast->jmp.nest) {
					break;
				}
				if (!isVariable(s) || isStatic(s)) {
					continue;
				}

				stBreak -= padOffset(s->size, vm_stk_align);
				dbgCgen("todo.destroy: %T", s);
			}

			// drop cached arguments, except result.
			if (!emitStack(rt, opc_drop, stBreak)) {
				fatal(ERR_INTERNAL_ERROR);
				return CAST_any;
			}

			size_t ifJmp = emit(rt, opc_jmp);
			if (ifJmp == 0) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			fixJump(rt, 0, 0, spBegin);
			addJump(ctx, ifJmp, ast->kind, ast);
			break;

		case STMT_ret:
			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);
			if (ast->ret.value != NULL) {
				astn res = ast->ret.value;
				// `return 3;` must be modified to `return (result := 3);`
				dieif(res->kind != ASGN_set, ERR_INTERNAL_ERROR);

				if (res->type == ctx->type_vid && res->kind == ASGN_set) {
					// do not generate assignment to void(ie: `void f1() { return f2(); }`)
					res = res->op.rhso;
				}
				if (!genAst(ctx, res, CAST_vid)) {
					dbgTraceAst(ast);
					return CAST_any;
				}
			}
			// TODO: destroy local variables.
			if (!emitStack(rt, opc_drop, vm_ref_size + argsSize(ast->ret.func))) {
				dbgTrace(ERR_INTERNAL_ERROR);
				return CAST_any;
			}
			if (!emit(rt, opc_jmpi)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			fixJump(rt, 0, 0, spBegin);
			break;

		case OPER_fnc:		// '()'
			// parenthesised expression: `(value)`
			if (ast->op.lhso == NULL) {
				if (!(got = genAst(ctx, ast->op.rhso, get))) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				break;
			}

			// function call: `function(arguments)`
			if (!(got = genCall(ctx, ast, get))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;

		case OPER_idx:      // '[]'
			if (ast->op.rhso && ast->op.rhso->kind == PNCT_dot3) {
				// make a slice: a[ ... 3]
				if (!(got = genSlice(ctx, ast))) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				break;
			}
			if (!(got = genIndex(ctx, ast, get))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;

		case OPER_dot:      // '.'
			if (!(got = genMember(ctx, ast, get))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;

		case OPER_adr:		// '&'
		case PNCT_dot3:		// '...'
			// `&` and `...` may be used only in arguments, raise error if used somewhere else
			error(rt, ast->file, ast->line, ERR_INVALID_OPERATOR, ast, ctx->type_ptr, ast->type);
			// fall through

		case OPER_pls:		// '+'
			if (!(got = genAst(ctx, ast->op.rhso, get))) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;

		case OPER_not:		// '!'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
			if (!genAst(ctx, ast->op.rhso, got)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			if (!genOperator(rt, ast->kind, got)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'

		case OPER_ceq:		// '=='
		case OPER_cne:		// '!='
		case OPER_clt:		// '<'
		case OPER_cle:		// '<='
		case OPER_cgt:		// '>'
		case OPER_cge:		// '>='

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod:		// '%'
			if (!(op = genAst(ctx, ast->op.lhso, CAST_any))) {    // (int == int): bool
				dbgTraceAst(ast);
				return CAST_any;
			}
			if (!genAst(ctx, ast->op.rhso, CAST_any)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			if (!genOperator(rt, ast->kind, op)) {    // uint % int => u32.mod
				dbgTraceAst(ast);
				return CAST_any;
			}

			#ifdef DEBUGGING	// extra check: validate some conditions.
			switch (ast->kind) {
				default:
					if (ast->op.lhso->type != ast->op.rhso->type) {
						fatal(ERR_INTERNAL_ERROR);
						break;
					}
					break;

				case OPER_shl:
				case OPER_shr:
					dieif(ast->type != ast->op.lhso->type, ERR_INTERNAL_ERROR);
					dieif(ast->op.rhso->type != ctx->type_i32, ERR_INTERNAL_ERROR);
					break;
			}
			switch (ast->kind) {
				default:
					break;

				case OPER_ceq:
				case OPER_cne:
				case OPER_clt:
				case OPER_cle:
				case OPER_cgt:
				case OPER_cge:
					dieif(got != CAST_bit, ERR_INTERNAL_ERROR": (%t) -> %K", ast, got);
			}
			#endif
			break;

		case OPER_all:		// '&&'
		case OPER_any:		// '||'
			if (!genAst(ctx, ast->op.lhso, CAST_bit)) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			// duplicate lhs and check if it is true
			if (!emitInt(rt, opc_dup1, 0)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			vmOpcode opc = ast->kind != OPER_all ? opc_jnz : opc_jz;
			size_t jmp = emit(rt, opc);
			if (jmp == 0) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			// discard lhs and replace it with rhs
			if (!emitInt(rt, opc_spc, -vm_stk_align)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			if (!genAst(ctx, ast->op.rhso, CAST_bit)) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			fixJump(rt, jmp, emit(rt, markIP), spEnd);

			#ifdef DEBUGGING	// extra check: validate some conditions.
			dieif(ast->op.lhso->type != ctx->type_bol, ERR_INTERNAL_ERROR);
			dieif(ast->op.rhso->type != ctx->type_bol, ERR_INTERNAL_ERROR);
			dieif(got != CAST_bit, ERR_INTERNAL_ERROR": (%t) -> %K", ast, got);
			#endif
			break;

		case OPER_sel:      // '?:'
			if (!genLogical(ctx, ast)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			break;

		case INIT_set:  	// '='
		case ASGN_set: {	// '='
			ccKind cast = refCast(ast->op.lhso->type);
			size_t size = refSize(ast->op.lhso->type);
			dieif(size == 0, ERR_INTERNAL_ERROR);

			if (ast->kind == INIT_set) {
				symn var = linkOf(ast->op.lhso, 0);
				// initialize variables by reference as reference
				if (var != NULL && isIndirect(var)) {
					size = vm_ref_size;
					cast = CAST_ptr;
				}
			}

			if (!genAst(ctx, ast->op.rhso, cast)) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			if (ast->type == ctx->emit_opc && spBegin != stkOffset(rt, -padOffset(size, vm_stk_align))) {
				// raise an error if the size of the variable is not the same as the emitted size: `int32 var = emit(int64(0));`
				error(rt, ast->op.rhso->file, ast->op.rhso->line, ERR_INVALID_EMIT_SIZE, ast, size, stkOffset(rt, -spBegin));
			}

			if (get != CAST_vid) {
				// in case a = b = sum(2, 700);
				// duplicate the result
				if (!emitInt(rt, opc_ldsp, 0)) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				if (!emitInt(rt, opc_ldi, size)) {
					dbgTraceAst(ast);
					return CAST_any;
				}
			}

			switch (cast) {
				default:
					cast = CAST_ptr;
					break;

				case CAST_ptr:
					size = ctx->type_ptr->size;
					cast = KIND_var;
					break;

				case CAST_obj:
					size = ctx->type_obj->size;
					cast = KIND_var;
					break;

				case CAST_arr:
				case CAST_var:
					// assign arrays and variants
					size = ctx->type_var->size;
					cast = KIND_var;
					break;
			}

			if (!genAst(ctx, ast->op.lhso, cast)) {
				dbgTraceAst(ast);
				return CAST_any;
			}
			size_t codeEnd = emitInt(rt, opc_sti, size);
			if (!codeEnd) {
				dbgTraceAst(ast);
				return CAST_any;
			}

			// optimize assignments.
			if (rt->foldAssign && foldAssignment(rt, ipBegin, codeEnd)) {
				if (get == CAST_vid && stkOffset(rt, 0) < spBegin) {
					// we probably cleared the stack
					got = CAST_vid;
				}
			}
			break;
		}

		case TOKEN_opc:
			if (!emitInt(rt, ast->opc.code, ast->opc.args)) {
				return CAST_any;
			}
			if (ast->opc.code == opc_nfc) {
				// in case of native call the result is what was requested: emit(float32(3), float32.sin)
				// float32.sin is not a reference, the native call is invoked and a value returned
				got = get;
			}
			break;

		case TOKEN_val:
			if (get == CAST_vid) {
				// void("message") || void(0)
				debug(rt, ast->file, ast->line, WARN_NO_CODE_GENERATED, ast);
				return CAST_vid;
			}
			switch (got) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return CAST_any;

				case CAST_bit:
				case CAST_i32:
				case CAST_u32:
				case CAST_i64:
				case CAST_u64:
					if (!emitI64(rt, ast->cInt)) {
						dbgTraceAst(ast);
						return CAST_any;
					}
					got = CAST_i64;
					break;

				case CAST_f32:
				case CAST_f64:
					if (!emitF64(rt, ast->cFlt)) {
						dbgTraceAst(ast);
						return CAST_any;
					}
					got = CAST_f64;
					break;

				case CAST_ptr:
				case CAST_obj:
				case CAST_arr: // push reference
					if (get == CAST_var && !emitOffs(rt, ast->type->offs)) {
						dbgTraceAst(ast);
						return CAST_any;
					}
					if (get == CAST_arr && !emitOffs(rt, strlen(ast->id.name))) {
						dbgTraceAst(ast);
						return CAST_any;
					}
					if (!emitRef(rt, vmOffset(rt, ast->id.name))) {
						dbgTraceAst(ast);
						return CAST_any;
					}
					if (get == CAST_arr) {
						got = CAST_arr;
					}
					else {
						got = CAST_ptr;
					}
					break;
			}
			break;

		case TOKEN_var: {                // var, func, type, enum, alias
			symn var = ast->id.link;
			dieif(isTypename(var) && !isStatic(var), ERR_INTERNAL_ERROR);	// types are static
			dieif(isFunction(var) && !isStatic(var), ERR_INTERNAL_ERROR);	// functions are static

			if (var->tag == ast) {
				if (isStatic(var) || isInline(var)) {
					// static variables are already generated.
					dbgInfo("%?s:%?u: not a variable declaration: %T", ast->file, ast->line, var);
				}
				else if (!(got = genDeclaration(ctx, var, get))) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				// TODO: type definition returns CAST_vid
				get = got;
			}
			else {
				if (ast->type == ctx->emit_opc) {
					// TODO: find a better solution
					get = KIND_var;
				}
				if (!(got = genVariable(ctx, var, get, ast))) {
					dbgTraceAst(ast);
					return CAST_any;
				}
			}
			break;
		}
	}

	if (get == KIND_var && (got == CAST_ptr || got == CAST_obj || got == CAST_arr)) {
		get = got;
	}

	// generate cast
	if (get != got) {
		if ((get == CAST_u32 || get == CAST_u64) && (got == CAST_f32 || got == CAST_f64)) {
			warn(rt, ctx->file, ctx->line, WARN_USING_SIGNED_CAST, ast);
		}
		if ((get == CAST_f32 || get == CAST_f64) && (got == CAST_u32 || got == CAST_u64)) {
			warn(rt, ctx->file, ctx->line, WARN_USING_SIGNED_CAST, ast);
		}

		vmOpcode cast = opc_last;
		switch (get) {
			default:
				break;

			case CAST_vid:
				// TODO: destroy local variables.
				if (!emitStack(rt, opc_drop, spBegin)) {
					dbgTraceAst(ast);
					return CAST_any;
				}
				cast = opc_nop;
				break;

			case CAST_bit:
				switch (got) {
					default:
						break;

					case CAST_u32:
					case CAST_i32:
						cast = i32_bol;
						break;

					case CAST_i64:
					case CAST_u64:
						cast = i64_bol;
						break;

					case CAST_f32:
						cast = f32_bol;
						break;

					case CAST_f64:
						cast = f64_bol;
						break;
				}
				break;

			case CAST_i32:
			case CAST_u32:
				switch (got) {
					default:
						break;

					case CAST_bit:
					case CAST_i32:
					case CAST_u32:
						cast = opc_nop;
						break;

					case CAST_i64:
					case CAST_u64:
						cast = i64_i32;
						break;

					case CAST_f32:
						cast = f32_i32;
						break;

					case CAST_f64:
						cast = f64_i32;
						break;
				}
				break;

			case CAST_u64:
			case CAST_i64:
				switch (got) {
					default:
						break;

					case CAST_bit:
					case CAST_i32:
						cast = i32_i64;
						break;

					case CAST_u32:
						cast = u32_i64;
						break;

					case CAST_i64:
					case CAST_u64:
						cast = opc_nop;
						break;

					case CAST_f32:
						cast = f32_i64;
						break;

					case CAST_f64:
						cast = f64_i64;
						break;
				}
				break;

			case CAST_f32:
				switch (got) {
					default:
						break;

					case CAST_bit:
					case CAST_i32:
					case CAST_u32:
						cast = i32_f32;
						break;

					case CAST_i64:
					case CAST_u64:
						cast = i64_f32;
						break;

					case CAST_f64:
						cast = f64_f32;
						break;
				}
				break;

			case CAST_f64:
				switch (got) {
					default:
						break;

					case CAST_bit:
					case CAST_i32:
					case CAST_u32:
						cast = i32_f64;
						break;

					case CAST_i64:
					case CAST_u64:
						cast = i64_f64;
						break;

					case CAST_f32:
						cast = f32_f64;
						break;
				}
				break;

			case CAST_ptr:
			case CAST_obj:
				switch (got) {
					default:
						break;
					case CAST_ptr:
					case CAST_obj:
						cast = opc_nop;
						break;
				}
				break;
		}

		if (cast != opc_nop && !emit(rt, cast)) {
			error(rt, ast->file, ast->line, ERR_CAST_EXPRESSION, ast, got, get);
			return CAST_any;
		}

		got = get;
	}

	return got;
}

int ccGenCode(ccContext ctx, int debug) {
	rtContext rt = ctx->rt;

	if (rt->errors != 0) {
		dieif(ctx == NULL, ERR_INTERNAL_ERROR);
		dbgTrace("can not generate code with errors");
		return rt->errors;
	}

	// leave the global scope.
	rt->main = install(ctx, ".main", ATTR_stat | KIND_fun, ctx->type_fun->size, ctx->type_fun, ctx->root);
	ctx->scope = leave(ctx, ctx->genStaticGlobals ? ATTR_stat | KIND_def : KIND_def, 0, 0, NULL, NULL);
	dieif(ctx->scope != ctx->global, ERR_INTERNAL_ERROR);

	/* reorder the initialization of static variables and functions.
	 *	int f() {
	 *		static int g = 9;
	 *		// ...
	 *	}
	 *	// variable `g` must be generated before function `f` is generated
	 */
	if (ctx->global != NULL) {
		symn prevGlobal = NULL;

		for (symn global = ctx->global; global; global = global->global) {
			symn prevInner = NULL;

			for (symn inner = global; inner != NULL; inner = inner->global) {
				if (inner->owner == global && !isInline(inner)) {
					// logif(">", "global `%T` must be generated before `%T`", inner, global);
					if (prevGlobal == NULL || prevInner == NULL) {
						// the first globals are the builtin types, so we should never get here
						fatal("global `%T` cant be generated before `%T`", inner, global);
						return -1;
					}

					// remove inner from list
					prevInner->global = inner->global;

					// place it before its owner
					inner->global = prevGlobal->global;
					prevGlobal->global = inner;

					// continue with the next symbol
					global = prevGlobal;
					inner = prevInner;
				}
				prevInner = inner;
			}
			prevGlobal = global;
		}
	}

	// prepare to emit instructions
	if (debug && !dbgInit(rt, dbgError)) {
		return -2;
	}
	if (!vmInit(rt, NULL)) {
		return -2;
	}

	//~ read only memory ends here.
	//~ strings, types, add(constants, functions, enums, ...)
	rt->vm.ro = emit(rt, markIP);
	rt->vm.cs = rt->vm.ds = 0;

	// static variables & functions
	for (symn var = ctx->global; var; var = var->global) {

		if (var == rt->main || isInline(var)) {
			// exclude main and inline symbols
			continue;
		}

		if (isTypename(var)) {
			// types are "generated" when they are declared
			dieif(var->offs == 0, ERR_INTERNAL_ERROR);
			dieif(!isStatic(var), ERR_INTERNAL_ERROR);
			continue;
		}

		if (!isStatic(var)) {
			// exclude non static variables
			continue;
		}

		dieif(var->offs != 0, "Error `%T` offs: %d", var, var->offs);    // already generated ?

		// align memory. Speed up read, write and execution.
		rt->_beg = padPointer(rt->_beg, vm_mem_align);

		if (isFunction(var)) {
			size_t offs = 0;
			// recalculate the offsets of the parameters
			for (symn param = var->params; param != NULL; param = param->next) {
				if (isStatic(param)) {
					fatal(ERR_INTERNAL_ERROR);
					continue;
				}
				if (param->size == 0) {
					param->size = padOffset(param->type->size, vm_stk_align);
				}
				param->offs = offs += param->size;
			}

			// function starts here
			var->offs = emit(rt, markIP);

			// reset the stack size and max jump
			fixJump(rt, 0, var->offs, vm_ref_size + argsSize(var));

			if (!genAst(ctx, var->init, CAST_vid)) {
				error(rt, var->file, var->line, ERR_EMIT_FUNCTION, var);
#ifdef DEBUGGING
				size_t end = emit(rt, markIP);
				for (size_t n = var->offs; n <= end; ) {
					unsigned char *ip = nextOpc(rt, &n, NULL);
					if (ip == NULL) {
						// invalid offset
						break;
					}
					size_t pc = vmOffset(rt, ip);
					printFmt(stdout, NULL, "%06x  %.9A\n", pc, ip);
				}
#endif
				continue;
			}

			if (stkOffset(rt, 0) != vm_ref_size + argsSize(var)) {
				error(rt, var->file, var->line, ERR_EMIT_FUNCTION, var);
				continue;
			}

			// remove the last (drop; ret) instructions if possible
			if (testOcp(rt, rt->vm.pc, opc_spc, NULL)) {
				size_t pc = var->offs;
				size_t pc1 = pc;
				while (pc < rt->vm.pc) {
					pc1 = pc;
					if (!nextOpc(rt, &pc, NULL)) {
						fatal(ERR_INTERNAL_ERROR);
						return -1;
					}
				}
				if (rt->vm.px < pc1 && testOcp(rt, pc1, opc_jmpi, NULL)) {
					// it is safe to remove the last opcode (clear stack)
					if (rollbackPc(rt)) {
						rt->vm.pc = pc1;
					}
				}
			}
			// last instruction of a function must be: ret
			if (!testOcp(rt, rt->vm.pc, opc_jmpi, NULL)) {
				if (var->params->type != ctx->type_vid) {
					error(rt, var->file, var->line, ERR_EMIT_FUNCTION_NO_RET, var);
					continue;
				}
				if (!emit(rt, opc_jmpi)) {
					error(rt, var->file, var->line, ERR_EMIT_FUNCTION, var);
					continue;
				}
			}
			if (ctx->jumps.cnt > 0) {
				jumpToFix jmp = getBuff(&ctx->jumps, 0);
				fatal(ERR_INTERNAL_ERROR": invalid jump: `%t`", jmp->node);
			}
			var->size = emit(rt, markIP) - var->offs;
			var->kind |= ATTR_stat;

			rt->vm.cs += var->size;
			addDbgFunction(rt, var);
		}
		else if (isVariable(var)) {
			dieif(var->size <= 0, "Error `%T` size: %d", var, var->size);    // instance of void ?

			// allocate the memory for the global variable
			var->offs = vmOffset(rt, rt->_beg);
			rt->_beg += var->size;

			if (rt->_beg >= rt->_end) {
				error(rt, var->file, var->line, ERR_MEMORY_OVERRUN);
				return -3;
			}
			rt->vm.ds += var->size;
		}
		else {
			fatal(ERR_INTERNAL_ERROR);
			return -1;
		}

		dbgInfo("Global `%T` offs: %d, size: %d", var, var->offs, var->size);
	}

	rt->main->offs = emit(rt, markIP);
	if (ctx->root != NULL) {
		// reset the stack
		fixJump(rt, 0, 0, 0);

		dieif(ctx->root->kind != STMT_beg, ERR_INTERNAL_ERROR);

		// generate static variable initializations
		for (symn var = ctx->global; var; var = var->global) {
			if (!isVariable(var)) {
				continue;
			}
			size_t begin = emit(rt, markIP);
			if (!genDeclaration(ctx, var, CAST_vid)) {
				dbgTraceAst(var->tag);
				return -4;
			}
			addDbgStatement(rt, begin, emit(rt, markIP), var->tag);
		}

		// CAST_vid clears the stack
		if (!genAst(ctx, ctx->root, ctx->genStaticGlobals ? CAST_vid : CAST_val)) {
			dbgTraceAst(ctx->root);
			return -5;
		}

		if (ctx->jumps.cnt > 0) {
			jumpToFix jmp = getBuff(&ctx->jumps, 0);
			fatal(ERR_INTERNAL_ERROR": invalid jump: `%t`", jmp->node);
			return -6;
		}
	}

	// program entry(main()) and exit(halt())
	rt->vm.px = emitInt(rt, opc_nfc, 0);
	rt->vm.pc = rt->main->offs;
	rt->vm.ss = 0;

	// build the main initializer function.
	rt->main->size = emit(rt, markIP) - rt->main->offs;
	rt->main->fields = ctx->global;
	rt->main->fmt = rt->main->name;
	rt->main->unit = ctx->unit;

	rt->vm.cs += rt->main->size;
	addDbgFunction(rt, rt->main);

	if (rt->dbg != NULL) {
		// TODO: replace bubble sort with qsort
		struct arrBuffer *codeMap = &rt->dbg->statements;
		for (size_t i = 0; i < codeMap->cnt; ++i) {
			for (size_t j = i; j < codeMap->cnt; ++j) {
				dbgn a = getBuff(codeMap, i);
				dbgn b = getBuff(codeMap, j);
				if (a->end > b->end) {
					memSwap(a, b, sizeof(struct dbgNode));
				}
				else if (a->end == b->end) {
					if (a->start < b->start) {
						memSwap(a, b, sizeof(struct dbgNode));
					}
				}
			}
		}
	}

	return rt->errors;
}
