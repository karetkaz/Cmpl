/*******************************************************************************
 *   File: cgen.c
 *   Date: 2011/06/23
 *   Desc: code generation
 *******************************************************************************
description:
*******************************************************************************/
#include <string.h>
#include "internal.h"

//#{ utility function for debugging only.
void dmpDbg(rtContext rt, astn ast, size_t offsStart, size_t offsEnd) {
#if DEBUGGING >= 3	// print generated code.
	prerr("ast", "%s:%u: %t", ast->file, ast->line, ast);
	for (size_t pc = offsStart; pc < offsEnd; ) {
		unsigned char* ip = getip(rt, pc);
		prerr("asm", "%.6x: %9A", pc, ip);
		pc += opc_tbl[*ip].size;
	}
#else
	(void)rt;
	(void)offsStart;
	(void)offsEnd;
#endif
	(void)ast;
}
//#}

/**
 * @brief get absolute position on stack, of relative offset
 * @param rt Runtime context.
 * @param size size of variable.
 * @return the position of variable on stack.
 */
static inline size_t stkoffs(rtContext rt, size_t size) {
	dieif(size > rt->_size, "Error(expected: %d, actual: %d)", rt->_size, size);
	return padded(size, vm_size) + rt->vm.ss * vm_size;
}

// utility function swap memory
static inline void memswap(void* _a, void* _b, size_t size) {
	register char *a = _a;
	register char *b = _b;
	register char *end = a + size;
	while (a < end) {
		char c = *a;
		*a = *b;
		*b = c;
		a += 1;
		b += 1;
	}
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmany-braces-around-scalar-init"

size_t emitint(rtContext rt, vmOpcode opc, int64_t value) {
	stkval arg = { .i8 = value };
	return emitarg(rt, opc, arg);
}

/// Emit an instruction indexing nth element on stack.
static size_t emitidx(rtContext rt, vmOpcode opc, size_t arg) {
	stkval tmp = { .i8 = rt->vm.ss * vm_size - arg };

	switch (opc) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return 0;

		case opc_drop:
		case opc_ldsp:
			return emitarg(rt, opc, tmp);

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			tmp.sz /= vm_size;
			break;
	}


	if (tmp.sz != (size_t) tmp.i8) {
		trace("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	if (tmp.sz > vm_regs) {
		trace("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	if (tmp.sz > rt->vm.ss * vm_size) {
		trace("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	return emitarg(rt, opc, tmp);
}

/// Emit an instruction without argument.
static inline size_t emitopc(rtContext rt, vmOpcode opc) {
	stkval arg = { .i8 = 0 };
	return emitarg(rt, opc, arg);
}

/// Increment the top of stack.
static inline size_t emitinc(rtContext rt, size_t value) {
	stkval arg = { .i8 = value };
	return emitarg(rt, opc_inc, arg);
}

// emit constant values.
static inline size_t emiti32(rtContext rt, int32_t value) {
	stkval arg = { .i8 = value };
	return emitarg(rt, opc_lc32, arg);
}
static inline size_t emiti64(rtContext rt, int64_t value) {
	stkval arg = { .i8 = value };
	return emitarg(rt, opc_lc64, arg);
}
static inline size_t emitf64(rtContext rt, float64_t value) {
	stkval arg = { .f8 = value };
	return emitarg(rt, opc_lf64, arg);
}

// emit a size value 32 or 64 bit constant based on vm size
static inline size_t emitofs(rtContext rt, size_t value) {
	stkval arg = { .i8 = value };
	return emitarg(rt, vm_size == (4) ? opc_lc32 : opc_lc64, arg);
}
// emit a size value 32 or 64 bit constant based on vm size
static inline size_t emitref(rtContext rt, void* value) {
	stkval arg = { .i8 = vmOffset(rt, value) };
	return emitarg(rt, opc_lref, arg);
}
#pragma clang diagnostic pop

// emit operator(add, sub, mul, ...), based on type
static size_t emitopr(rtContext rt, vmOpcode opc, ccKind cast) {
	// comparation
	if (opc == opc_ceq) switch (cast) {
		default:
			break;

		case CAST_bit:
		case CAST_ref:
		case CAST_u32:
		case CAST_i32:
			opc = i32_ceq;
			break;

		case CAST_i64:
			opc = i64_ceq;
			break;

		case CAST_f32:
			opc = f32_ceq;
			break;

		case CAST_f64:
			opc = f64_ceq;
			break;
	}
	else if (opc == opc_cne) {
		if (!emitopr(rt, opc_ceq, cast)) {
			trace(ERR_INTERNAL_ERROR);
			return 0;
		}
		opc = opc_not;
	}
	else if (opc == opc_clt) switch (cast) {
		default:
			break;

		case CAST_u32:
			opc = u32_clt;
			break;

		case CAST_i32:
			opc = i32_clt;
			break;

		case CAST_f32:
			opc = f32_clt;
			break;

		case CAST_i64:
			opc = i64_clt;
			break;

		case CAST_f64:
			opc = f64_clt;
			break;
	}
	else if (opc == opc_cge) {
		if (!emitopr(rt, opc_clt, cast)) {
			trace(ERR_INTERNAL_ERROR);
			return 0;
		}
		opc = opc_not;
	}
	else if (opc == opc_cgt) switch (cast) {
		default:
			break;

		case CAST_u32:
			opc = u32_cgt;
			break;

		case CAST_i32:
			opc = i32_cgt;
			break;

		case CAST_f32:
			opc = f32_cgt;
			break;

		case CAST_i64:
			opc = i64_cgt;
			break;

		case CAST_f64:
			opc = f64_cgt;
			break;
	}
	else if (opc == opc_cle) {
		if (!emitopr(rt, opc_cgt, cast)) {
			trace(ERR_INTERNAL_ERROR);
			return 0;
		}
		opc = opc_not;
	}

	// arithmetic
	else if (opc == opc_neg) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = i32_neg;
			break;

		case CAST_i64:
			opc = i64_neg;
			break;

		case CAST_f32:
			opc = f32_neg;
			break;

		case CAST_f64:
			opc = f64_neg;
			break;
	}
	else if (opc == opc_add) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = i32_add;
			break;

		case CAST_i64:
			opc = i64_add;
			break;

		case CAST_f32:
			opc = f32_add;
			break;

		case CAST_f64:
			opc = f64_add;
			break;
	}
	else if (opc == opc_sub) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = i32_sub;
			break;

		case CAST_i64:
			opc = i64_sub;
			break;

		case CAST_f32:
			opc = f32_sub;
			break;

		case CAST_f64:
			opc = f64_sub;
			break;
	}
	else if (opc == opc_mul) switch (cast) {
		default:
			break;

		case CAST_u32:
			opc = u32_mul;
			break;

		case CAST_i32:
			opc = i32_mul;
			break;

		case CAST_i64:
			opc = i64_mul;
			break;

		case CAST_f32:
			opc = f32_mul;
			break;

		case CAST_f64:
			opc = f64_mul;
			break;
	}
	else if (opc == opc_div) switch (cast) {
		default:
			break;

		case CAST_u32:
			opc = u32_div;
			break;

		case CAST_i32:
			opc = i32_div;
			break;

		case CAST_i64:
			opc = i64_div;
			break;

		case CAST_f32:
			opc = f32_div;
			break;

		case CAST_f64:
			opc = f64_div;
			break;
	}
	else if (opc == opc_mod) switch (cast) {
		default:
			break;

		case CAST_u32:
			opc = u32_mod;
			break;

		case CAST_i32:
			opc = i32_mod;
			break;

		case CAST_i64:
			opc = i64_mod;
			break;

		case CAST_f32:
			opc = f32_mod;
			break;

		case CAST_f64:
			opc = f64_mod;
			break;
	}

	// bit operations
	else if (opc == opc_cmt) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = b32_cmt;
			break;

		/*case CAST_i64:
			opc = b64_cmt;
			break;// */
	}
	else if (opc == opc_shl) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = b32_shl;
			break;

		/*case CAST_i64:
			opc = b64_shl;
			break;// */
	}
	else if (opc == opc_shr) switch (cast) {
		default:
			break;

		case CAST_u32:
			opc = b32_shr;
			break;

		case CAST_i32:
			opc = b32_sar;
			break;

		/*case CAST_i64:
			opc = b64_sar;
			break;// */
	}
	else if (opc == opc_and) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = b32_and;
			break;

		/*case CAST_i64:
			opc = b64_and;
			break;// */
	}
	else if (opc == opc_ior) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = b32_ior;
			break;

		/*case CAST_i64:
			opc = b64_ior;
			break;// */
	}
	else if (opc == opc_xor) switch (cast) {
		default:
			break;

		case CAST_u32:
		case CAST_i32:
			opc = b32_xor;
			break;

		/*case CAST_i64:
			opc = b64_xor;
			break;// */
	}

	return emitopc(rt, opc);
}

/**
 * @brief emit the address of the variable.
 * @param rt Runtime context.
 * @param var Variable to be used.
 * @return Program counter.
 */
static inline size_t emitvar(rtContext rt, symn var) {
	if (!var->stat) {
		return emitidx(rt, opc_ldsp, var->offs);
	}
	return emitint(rt, opc_lref, var->offs);
}

/**
 * @brief Generate bytecode from abstract syntax tree.
 * @param rt Runtime context.
 * @param ast Abstract syntax tree.
 * @param get Override node cast.
 * @return Should be get || cast of ast node.
 */
static ccKind cgen(rtContext rt, astn ast, ccKind get) {
	#ifdef DEBUGGING	// extra check: every statement qualifier must be processed.
	ccKind stmt_qual = TYPE_any;
	#endif

	struct astNode tmp;
	ccKind got = TYPE_any;

	dieif(ast == NULL, ERR_INTERNAL_ERROR);
	dieif(ast->type == NULL, ERR_INTERNAL_ERROR);

	if (get == TYPE_any)
		get = ast->cast;

	if (!(got = ast->type->cast)) {
		got = ast->type->kind;
	}

	#ifdef DEBUGGING	// extra check: every statement qualifier must be processed.
	// take care of qualified statements `static if` ...
	if (ast->kind >= STMT_beg && ast->kind <= STMT_end) {
		stmt_qual = ast->cast;
	}
	#endif

	// generate instructions
	switch (ast->kind) {

		default:
			fatal(ERR_INTERNAL_ERROR);
			return TYPE_any;

		//#{ STATEMENTS
		case STMT_beg: {	// {} or function body
			size_t stpos = stkoffs(rt, 0);
			size_t ippar = 0;
			astn ptr;

			#ifdef DEBUGGING	// extra check: statement qualifier processed.
			// TODO: a block may return nothing or the contained declarations(foreach may have 2 init declarations.)
			if (stmt_qual == CAST_vid || stmt_qual == TYPE_rec) {
				stmt_qual = TYPE_any;
			}
			#endif
			if (ast->cast == ATTR_paral) {
				ippar = emitopc(rt, opc_task);
				rt->vm.su = 0;

				#ifdef DEBUGGING	// extra check: statement qualifier processed.
				stmt_qual = TYPE_any;
				#endif
			}
			for (ptr = ast->stmt.stmt; ptr; ptr = ptr->next) {
				size_t ipStart = emitopc(rt, markIP);
				if (!cgen(rt, ptr, CAST_vid)) {		// we will free stack on scope close
					error(rt, ptr->file, ptr->line, "emitting statement");
					dmpDbg(rt, ptr, ipStart, emitopc(rt, markIP));
					return TYPE_any;
				}
				if (ptr->kind != STMT_beg) {
					size_t ipEnd = emitopc(rt, markIP);
					addDbgStatement(rt, ipStart, ipEnd, ptr);
				}
			}
			if (ast->cast == TYPE_rec) {
				debug("%K", get);
				get = TYPE_any;
			}
			if (get == CAST_vid && stpos != stkoffs(rt, 0)) {
				// TODO: call destructor for variables
				if (!emitidx(rt, opc_drop, stpos)) {
					trace(ERR_INTERNAL_ERROR);
					return TYPE_any;
				}
			}
			if (ippar != 0) {
				fixjump(rt, ippar, emitopc(rt, markIP), rt->vm.su * vm_size);
				emitint(rt, opc_sync, 0);
				debug("parallel data on stack: %d", rt->vm.su);
			}
		} break;
		case STMT_for: {
			astn jl = rt->cc->jmps;
			size_t lincr;
			size_t jstep, lcont, lbody, lbreak;
			size_t stbreak, stpos = stkoffs(rt, 0);

			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);

			if (ast->stmt.init && !cgen(rt, ast->stmt.init, CAST_vid)) {
				traceAst(ast);
				return TYPE_any;
			}

			if (!(jstep = emitopc(rt, opc_jmp))) {		// continue;
				traceAst(ast);
				return TYPE_any;
			}

			lbody = emitopc(rt, markIP);
			if (ast->stmt.stmt && !cgen(rt, ast->stmt.stmt, CAST_vid)) {
				traceAst(ast);
				return TYPE_any;
			}

			lcont = emitopc(rt, markIP);
			if (ast->stmt.step && !cgen(rt, ast->stmt.step, CAST_vid)) {
				traceAst(ast);
				return TYPE_any;
			}

			lincr = emitopc(rt, markIP);
			fixjump(rt, jstep, emitopc(rt, markIP), -1);
			if (ast->stmt.test) {
				if (!cgen(rt, ast->stmt.test, CAST_bit)) {
					traceAst(ast);
					return TYPE_any;
				}
				if (!emitint(rt, opc_jnz, lbody)) {		// continue;
					traceAst(ast);
					return TYPE_any;
				}
			}
			else {
				if (!emitint(rt, opc_jmp, lbody)) {		// continue;
					traceAst(ast);
					return TYPE_any;
				}
			}

			lbreak = emitopc(rt, markIP);
			stbreak = stkoffs(rt, 0);

			addDbgStatement(rt, lcont, lincr, ast->stmt.step);
			addDbgStatement(rt, lincr, lbreak, ast->stmt.test);

			while (rt->cc->jmps != jl) {
				astn jmp = rt->cc->jmps;
				rt->cc->jmps = jmp->next;

				if (jmp->go2.stks != stbreak) {
					error(rt, jmp->file, jmp->line,
						  "`%t` statement is invalid due to previous variable declaration within loop", jmp);
					return TYPE_any;
				}
				switch (jmp->kind) {
					default :
						error(rt, jmp->file, jmp->line, "invalid goto statement: %t", jmp);
						return TYPE_any;

					case STMT_brk:
						fixjump(rt, jmp->go2.offs, lbreak, jmp->go2.stks);
						break;

					case STMT_con:
						fixjump(rt, jmp->go2.offs, lcont, jmp->go2.stks);
						break;
				}
			}

			//~ TODO: destruct(ast->stmt.test)
			if (stpos != stkoffs(rt, 0)) {
				dieif(!emitidx(rt, opc_drop, stpos), ERR_INTERNAL_ERROR);
			}
		} break;
		case STMT_if:  {
			size_t jmpt = 0, jmpf = 0;
			size_t stpos = stkoffs(rt, 0);
			ccKind tt = eval(&tmp, ast->stmt.test);

			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);

			if (ast->cast == ATTR_stat) {
				if (ast->stmt.step || !tt) {
					error(rt, ast->file, ast->line, "invalid static if construct: %s",
						  !tt ? "can not evaluate" : "else part is invalid");
					return TYPE_any;
				}
				#ifdef DEBUGGING	// extra check: statement qualifier processed.
				stmt_qual = TYPE_any;
				#endif
			}

			if (tt && ast->cast == ATTR_stat) {	// static if then else
				astn gen = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
				if (gen != NULL && !cgen(rt, gen, TYPE_any)) {	// leave the stack
					traceAst(gen);
					return TYPE_any;
				}
				// local declarations inside a static if grows the stack.
				stpos = stkoffs(rt, 0);
			}
			else {
				// hack: `if (true) sin(pi/4);` leaves the result of sin on stack
				// so, code will be generated as: if (true) { sin(pi/4); }
				struct astNode block;

				memset(&block, 0, sizeof(block));
				block.type = rt->cc->type_vid;
				block.kind = STMT_beg;

				if (tt != TYPE_any && rt->foldConst) {	// if (const)
					block.stmt.stmt = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
					if (block.stmt.stmt != NULL) {
						if (!cgen(rt, &block, CAST_vid)) {
							traceAst(ast);
							return TYPE_any;
						}
					}
					else {
						warn(rt, 2, ast->file, ast->line, "no code will be generated for statement: %+t", ast);
					}
				}
				else if (ast->stmt.stmt && ast->stmt.step) {		// if then else
					if (!cgen(rt, ast->stmt.test, CAST_bit)) {
						traceAst(ast);
						return TYPE_any;
					}
					if (!(jmpt = emitopc(rt, opc_jz))) {
						traceAst(ast);
						return TYPE_any;
					}

					block.stmt.stmt = ast->stmt.stmt;
					if (!cgen(rt, &block, CAST_vid)) {
						traceAst(ast);
						return TYPE_any;
					}
					if (!(jmpf = emitopc(rt, opc_jmp))) {
						traceAst(ast);
						return TYPE_any;
					}
					fixjump(rt, jmpt, emitopc(rt, markIP), -1);

					block.stmt.stmt = ast->stmt.step;
					if (!cgen(rt, &block, CAST_vid)) {
						traceAst(ast);
						return TYPE_any;
					}
					fixjump(rt, jmpf, emitopc(rt, markIP), -1);
				}
				else if (ast->stmt.stmt) {							// if then
					if (!cgen(rt, ast->stmt.test, CAST_bit)) {
						traceAst(ast);
						return TYPE_any;
					}
					//~ if false skip THEN block
					//~ TODO: warn: never executed code
					if (!(jmpt = emitopc(rt, opc_jz))) {
						traceAst(ast);
						return TYPE_any;
					}

					block.stmt.stmt = ast->stmt.stmt;
					if (!cgen(rt, &block, CAST_vid)) {
						traceAst(ast);
						return TYPE_any;
					}
					fixjump(rt, jmpt, emitopc(rt, markIP), -1);
				}
				else if (ast->stmt.step) {							// if else
					if (!cgen(rt, ast->stmt.test, CAST_bit)) {
						traceAst(ast);
						return TYPE_any;
					}
					//~ if true skip ELSE block
					//~ TODO: warn: never executed code
					if (!(jmpt = emitopc(rt, opc_jnz))) {
						traceAst(ast);
						return TYPE_any;
					}

					block.stmt.stmt = ast->stmt.step;
					if (!cgen(rt, &block, CAST_vid)) {
						traceAst(ast);
						return TYPE_any;
					}
					fixjump(rt, jmpt, emitopc(rt, markIP), -1);
				}
				dieif(get != CAST_vid || stpos != stkoffs(rt, 0), "internal fatal error");
			}
			//~ TODO: destruct(ast->stmt.test)
			dieif(stpos != stkoffs(rt, 0), "invalid stacksize(%d:%d) in statement %+t", stkoffs(rt, 0), stpos, ast);
		} break;
		case STMT_con:
		case STMT_brk: {
			size_t offs;
			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);
			if (!(offs = emitopc(rt, opc_jmp))) {
				traceAst(ast);
				return TYPE_any;
			}

			ast->go2.offs = offs;
			ast->go2.stks = stkoffs(rt, 0);

			ast->next = rt->cc->jmps;
			rt->cc->jmps = ast;
		} break;
		case STMT_end: {	// expr statement
			size_t stpos = stkoffs(rt, 0);
			if (!cgen(rt, ast->stmt.stmt, CAST_vid)) {
				traceAst(ast);
				return TYPE_any;
			}
			if (stpos > stkoffs(rt, 0)) {
				error(rt, ast->file, ast->line, "statement underflows stack: %+t", ast->stmt.stmt);
			}
		} break;
		case STMT_ret: {
			//~ TODO: declared reference variables should be freed.
			size_t bppos = stkoffs(rt, 0);
			if (ast->stmt.stmt && !cgen(rt, ast->stmt.stmt, CAST_vid)) {
				traceAst(ast);
				return TYPE_any;
			}
			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);
			if (get == CAST_vid && rt->vm.ro != stkoffs(rt, 0)) {
				if (!emitidx(rt, opc_drop, rt->vm.ro)) {
					trace("leve %d", rt->vm.ro);
					return TYPE_any;
				}
			}
			if (!emitopc(rt, opc_jmpi)) {
				traceAst(ast);
				return TYPE_any;
			}
			fixjump(rt, 0, 0, bppos);
		} break;
		//#}
		//#{ OPERATORS
		case OPER_fnc: {	// '()' emit/call/cast
			size_t stktop = stkoffs(rt, 0);
			size_t stkret = stkoffs(rt, sizeOf(ast->type, 1));
			symn var = linkOf(ast->op.lhso);
			astn argv = ast->op.rhso;

			dieif(var == NULL && ast->op.lhso != NULL, "Error %+t", ast);

			// pointer(&info); variant(&info);
			if (var && (var == rt->cc->type_ptr || var == rt->cc->type_var)) {
				if (argv != NULL && argv->kind != OPER_com) {
					symn object;
					int loadIndirect = 0;
					if (argv->kind == OPER_adr) {
						object = linkOf(argv->op.rhso);
					}
					else {
						object = linkOf(argv);
					}
					if (object != NULL) {
						if (var == rt->cc->type_var) {
							// push type
							if (!emitint(rt, opc_lref, vmOffset(rt, object->type))) {
								traceAst(ast);
								return TYPE_any;
							}
						}
						// push ref
						if (!emitvar(rt, object)) {
							traceAst(ast);
							return TYPE_any;
						}

						switch (object->cast) {
							default:
								if (argv->kind != OPER_adr && object->type->cast != CAST_ref) {
									warn(rt, 2, argv->file, argv->line, WARN_PASS_ARG_BY_REF, argv);
								}
								break;

							case CAST_arr:	// from slice
								warn(rt, 2, argv->file, argv->line, WARN_DISCARD_DATA, argv, ast->type);
								//~ TODO: loadIndirect = 1;
								break;

							case CAST_var:	// from variant
								warn(rt, 2, argv->file, argv->line, WARN_DISCARD_DATA, argv, ast->type);
								//~ TODO: loadIndirect = 1;
								break;

							case CAST_ref:	// from reference
								loadIndirect = 1;
								break;
						}
						if (loadIndirect != 0) {
							if (!emitint(rt, opc_ldi, vm_size)) {
								traceAst(ast);
								return TYPE_any;
							}
						}
						return CAST_var;
					}
				}
			}

			// debug(...)
			if (var && var == rt->cc->libc_dbg) {
				if (argv != NULL) {
					while (argv->kind == OPER_com) {
						astn arg = argv->op.rhso;
						if (!cgen(rt, arg, arg->cast)) {
							traceAst(arg);
							return TYPE_any;
						}
						argv = argv->op.lhso;
					}
					if (!cgen(rt, argv, argv->cast)) {
						traceAst(argv);
						return TYPE_any;
					}
				}
				dieif(!emiti32(rt, ast->line), "__FILE__");
				dieif(!emitref(rt, ast->file), "__LINE__");
				dieif(!emitint(rt, opc_libc, rt->cc->libc_dbg_idx), "__LIBC__");
				dieif(!emitidx(rt, opc_drop, stkret), "__DROP__");
				break;
			}

			// inline expansion
			if (var && var->kind == TYPE_def) {
				size_t chachedArgSize = 0;
				symn param = var->prms;

				if (argv != NULL) {
					astn an = NULL;

					// flatten arguments. (this is also done by lookup !?)
					while (argv->kind == OPER_com) {
						astn arg = argv->op.rhso;
						argv = argv->op.lhso;

						arg->next = an;
						an = arg;
					}
					argv->next = an;
					an = argv;

					while (an && param) {
						int inlineArg = param->cast == TYPE_def;
						int useCount = countUsages(param) - 1;
						astn arg = an;

						// TODO: skip over aliases?
						while (arg && arg->kind == CAST_ref) {
							symn def = arg->ref.link;
							if (def == NULL) {
								break;
							}
							if (def->init == NULL) {
								break;
							}
							if (def->kind != TYPE_def) {
								break;
							}
							arg = def->init;
						}// */
						// try to evaluate as a constant.
						if (rt->foldConst && eval(&tmp, arg)) {
							arg = &tmp;
						}

						/* TODO: make args inlineable
						if (param->cast != CAST_ref) {
							switch (arg->kind) {
								default:
									if (useCount <= 1) {
										inlineArg = 1;
									}
									break;

								case CAST_ref:
									if (arg->type == param->type) {
										symn link = arg->ref.link;
										// static variables should not be inlined ?
										if (link->stat) {
											if (useCount > 1) {
												break;
											}
										}
										// what about indirect references ?
										//~ if (link->cast == CAST_ref) { break; }
										inlineArg = 1;
									}
									break;
							}
						}
						//~ */

						inlineArg = 0;
						if (inlineArg || param->kind == TYPE_def) {
							if (param->cast != TYPE_def) {
								warn(rt, 16, ast->file, ast->line, "inlineing(%d) argument used %d times: %+T: %+t",
									 inlineArg, useCount, param, arg);
							}
							//~ param->kind = TYPE_def;
							param->init = an;
						}
						else {
							size_t stktop = stkoffs(rt, sizeOf(param, 1));
							chachedArgSize += sizeOf(param, 1);
							warn(rt, 16, ast->file, ast->line, "caching argument used %d times: %+T: %+t", useCount,
								 param, arg);
							if (!cgen(rt, an, param->cast == TYPE_def ? param->type->cast : param->cast)) {
								traceAst(arg);
								return TYPE_any;
							}

							param->offs = stkoffs(rt, 0);
							//~ param->kind = CAST_ref;
							if (stktop != param->offs) {
								error(rt, ast->file, ast->line, "invalid size: %d <> got(%d): `%+t`", stktop,
									  param->offs, param);
								return TYPE_any;
							}
						}
						an = an->next;
						param = param->next;
					}
					dieif(an || param, ERR_INTERNAL_ERROR);
				}

				if (var->init) {
					var->init->file = ast->file;
					var->init->line = ast->line;
				}

				if (!(got = cgen(rt, var->init, got))) {
					traceAst(ast);
					return TYPE_any;
				}

				if (stkret != stkoffs(rt, 0)) {
					logif(chachedArgSize != stkoffs(rt, 0) - stkret, "%+T(%d%+d): %+t", ast->type, stkret, stkoffs(rt, 0)-stkret, ast);
					if (get != CAST_vid) {
						if (!emitidx(rt, opc_ldsp, stkret)) {	// get stack
							trace(ERR_INTERNAL_ERROR);
							return TYPE_any;
						}
						if (!emitint(rt, opc_sti, sizeOf(ast->type, 1))) {
							error(rt, ast->file, ast->line, "store indirect: %T: %d of `%+t`", ast->type, sizeOf(ast->type, 1), ast);
							return TYPE_any;
						}
					}
					if (stkret < stkoffs(rt, 0)) {
						if (!emitidx(rt, opc_drop, stkret)) {
							traceAst(ast);
							return TYPE_any;
						}
					}
				}

				for (param = var->prms; param; param = param->next) {
					param->init = NULL;
					// TODO: ugly hack
					//~ param->kind = param->cast == TYPE_def ? TYPE_def : CAST_ref;
				}
				break;
			}

			// push Result, Arguments
			if (var && !isType(var) && var != rt->cc->emit_opc && var->call) {
				if (sizeOf(var->type, 1) && !emitint(rt, opc_spc, sizeOf(var->type, 1))) {
					traceAst(ast);
					return TYPE_any;
				}
				// result size on stack
				dieif(stkret != stkoffs(rt, 0), ERR_INTERNAL_ERROR);
			}
			if (argv != NULL) {
				astn argl = NULL;

				while (argv->kind == OPER_com) {
					astn arg = argv->op.rhso;
					if (!cgen(rt, arg, arg->cast)) {
						traceAst(arg);
						return TYPE_any;
					}
					arg->next = argl;
					argl = arg;
					argv = argv->op.lhso;
				}

				// static casting with emit: int32 i = emit(int32, float32(3.14));
				if (var == rt->cc->emit_opc && isTypeExpr(argv)) {
					break;
				}

				if (!cgen(rt, argv, argv->cast)) {
					traceAst(argv);
					return TYPE_any;
				}

				if (var && var != rt->cc->emit_opc && var != rt->cc->libc_dbg && var->call && var->cast != CAST_ref) {
					if (var->prms->offs != stkoffs(rt, 0) - stktop) {
						trace("args:%T(%d != %d(%d - %d))", var, var->prms->offs, stkoffs(rt, 0) - stktop, stkoffs(rt, 0), stktop);
						return TYPE_any;
					}
				}
				argv->next = argl;
				argl = argv;
			}

			if (var == rt->cc->emit_opc) {		// emit()
			}
			else if (isType(var)) {				// cast()
				dieif(stkret != stkoffs(rt, 0), "Error: %+t", ast);
				if (!argv || argv != ast->op.rhso) {
					warn(rt, 1, ast->file, ast->line, "multiple values, array ?: '%+t'", ast);
				}
			}
			else if (var) {						// call()
				if (var->call) {
					if (!cgen(rt, ast->op.lhso, CAST_ref)) {
						traceAst(ast);
						return TYPE_any;
					}
					if (!emitopc(rt, opc_call)) {
						traceAst(ast);
						return TYPE_any;
					}
					// clean the stack
					if (stkret != stkoffs(rt, 0)) {
						if (!emitidx(rt, opc_drop, stkret)) {
							traceAst(ast);
							return TYPE_any;
						}
					}
				}
				else {
					error(rt, ast->file, ast->line, "called object is not a function: %+T", var);
					return TYPE_any;
				}
			}
			else {								// ()
				dieif(ast->op.lhso != NULL, "Error %+t", ast);
				if (!argv || argv != ast->op.rhso) {
					warn(rt, 1, ast->file, ast->line, "multiple values, array ?: '%+t'", ast);
				}
			}
		} break;
		case OPER_idx: {	// '[]'
			int r;
			size_t esize;
			if (!(r = cgen(rt, ast->op.lhso, CAST_ref))) {
				traceAst(ast);
				return TYPE_any;
			}
			if (r == CAST_arr) {
				if (!emitint(rt, opc_ldi, vm_size)) {
					traceAst(ast);
					return TYPE_any;
				}
			}

			esize = sizeOf(ast->type, 0);	// size of array element
			if (rt->foldConst && eval(&tmp, ast->op.rhso) == TYPE_int) {
				size_t offs = sizeOf(ast->type, 0) * (int)constint(&tmp);
				if (!emitinc(rt, offs)) {
					traceAst(ast);
					return TYPE_any;
				}
			}
			else {
				if (!cgen(rt, ast->op.rhso, CAST_u32)) {
					traceAst(ast);
					return TYPE_any;
				}
				if (esize > 1) {
					if (!emitint(rt, opc_mad, esize)) {
						traceAst(ast);
						return TYPE_any;
					}
				}
				else if (esize == 1) {
					if (!emitopc(rt, i32_add)) {
						traceAst(ast);
						return TYPE_any;
					}
				}
				else {
					fatal("invalid array element size");
					return TYPE_any;
				}
			}

			if (get == CAST_ref)
				return CAST_ref;

			// ve need the value on that position (this can be a ref).
			if (!emitint(rt, opc_ldi, esize)) {
				traceAst(ast);
				return TYPE_any;
			}
		} break;
		case OPER_dot: {	// '.'
			// TODO: this should work as indexing
			symn object = linkOf(ast->op.lhso);
			symn member = linkOf(ast->op.rhso);
			int lhsstat = isTypeExpr(ast->op.lhso);

			if (!object || !member) {
				traceAst(ast);
				return TYPE_any;
			}

			if (!member->stat && lhsstat) {
				error(rt, ast->file, ast->line, "An object reference is required to access the member `%+T`", member);
				return TYPE_any;
			}
			if (member->stat) {
				if (!lhsstat && object->kind != CAST_arr) {
					warn(rt, 5, ast->file, ast->line, "accessing static member using instance variable `%+T`/ %+T",
						 member, object->type);
				}
				return cgen(rt, ast->op.rhso, get);
			}

			if (member->memb) {
				if (!cgen(rt, ast->op.lhso, CAST_ref)) {
					traceAst(ast);
					return TYPE_any;
				}
				return cgen(rt, ast->op.rhso, get);
			}
			if (member->kind == TYPE_def) {
				// static array length is of this type
				debug("accessing inline field %+T: %+t", member, ast);
				return cgen(rt, ast->op.rhso, get);
			}

			if (!cgen(rt, ast->op.lhso, CAST_ref)) {
				traceAst(ast);
				return TYPE_any;
			}

			if (!emitinc(rt, member->offs)) {
				traceAst(ast);
				return TYPE_any;
			}

			if (member->cast == CAST_ref) {
				if (!emitint(rt, opc_ldi, vm_size)) {
					traceAst(ast);
					return TYPE_any;
				}
			}

			if (get == CAST_ref) {
				return CAST_ref;
			}

			if (!emitint(rt, opc_ldi, sizeOf(ast->type, 1))) {
				traceAst(ast);
				return TYPE_any;
			}
		} break;
		case OPER_not:		// '!'
		case OPER_adr:		// '&'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			vmOpcode opc;
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return TYPE_any;

				case OPER_adr:
				case OPER_pls:
					return cgen(rt, ast->op.rhso, get);

				case OPER_mns:
					opc = opc_neg;
					break;
				case OPER_not:
					dieif(got != CAST_bit, ERR_INTERNAL_ERROR);
					opc = opc_not;
					break;
				case OPER_cmt:
					opc = opc_cmt;
					break;
			}
			if (!cgen(rt, ast->op.rhso, got)) {
				traceAst(ast);
				return TYPE_any;
			}
			if (!emitopr(rt, opc, got)) {
				traceAst(ast);
				return TYPE_any;
			}
		} break;

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq:		// '>='

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod: {	// '%'
			vmOpcode opc;
			#ifdef DEBUGGING	// extra check: 
			size_t ipdbg = emitopc(rt, markIP);
			#endif
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return TYPE_any;

				case OPER_add:
					opc = opc_add;
					break;
				case OPER_sub:
					opc = opc_sub;
					break;
				case OPER_mul:
					opc = opc_mul;
					break;
				case OPER_div:
					opc = opc_div;
					break;
				case OPER_mod:
					opc = opc_mod;
					break;

				case OPER_neq:
					opc = opc_cne;
					break;
				case OPER_equ:
					opc = opc_ceq;
					break;
				case OPER_geq:
					opc = opc_cge;
					break;
				case OPER_lte:
					opc = opc_clt;
					break;
				case OPER_leq:
					opc = opc_cle;
					break;
				case OPER_gte:
					opc = opc_cgt;
					break;

				case OPER_shl:
					opc = opc_shl;
					break;
				case OPER_shr:
					opc = opc_shr;
					break;
				case OPER_and:
					opc = opc_and;
					break;
				case OPER_ior:
					opc = opc_ior;
					break;
				case OPER_xor:
					opc = opc_xor;
					break;
			}
			if (!cgen(rt, ast->op.lhso, TYPE_any)) {
				traceAst(ast);
				return TYPE_any;
			}
			if (!cgen(rt, ast->op.rhso, TYPE_any)) {
				traceAst(ast);
				return TYPE_any;
			}
			if (!emitopr(rt, opc, ast->op.lhso->cast)) {	// uint % int => u32.mod
				traceAst(ast);
				return TYPE_any;
			}

			#ifdef DEBUGGING	// extra check: validate some conditions.
			if (ast->op.lhso->cast != ast->op.rhso->cast) {
				dmpDbg(rt, ast, ipdbg, emitopc(rt, markIP));
			}
			dieif(ast->op.lhso->cast != ast->op.rhso->cast, "RemMe", ast);
			dieif(got != castOf(ast->type), "RemMe");
			switch (ast->kind) {
				case OPER_neq:
				case OPER_equ:
				case OPER_geq:
				case OPER_lte:
				case OPER_leq:
				case OPER_gte:
					dieif(got != CAST_bit, "RemMe(%K): %+t", got, ast);
				default:
					break;
			}
			#endif
		} break;

		case OPER_all:		// '&&'
		case OPER_any: {	// '||'
			vmOpcode opc;
			// TODO: short circuit && and ||
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return TYPE_any;

				case OPER_all:
					opc = opc_and;
					break;
				case OPER_any:
					opc = opc_ior;
					break;
			}

			if (!cgen(rt, ast->op.lhso, CAST_bit)) {
				traceAst(ast);
				return TYPE_any;
			}

			if (!cgen(rt, ast->op.rhso, CAST_bit)) {
				traceAst(ast);
				return TYPE_any;
			}

			if (!emitopr(rt, opc, CAST_u32)) {
				trace("opc__%02x:%+t", opc, ast);
				return TYPE_any;
			}

			/*astn jmp1 = 0, jmp2 = 0;
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return 0;

				case OPER_all:
					jmp1 = newnode(rt->cc, STMT_brk);
					jmp2 = newnode(rt->cc, STMT_brk);
					opc = opc_jnz;
					break;

				case OPER_any:
					jmp1 = newnode(rt->cc, STMT_con);
					jmp2 = newnode(rt->cc, STMT_con);
					opc = opc_jz;
					break;
			}

			if (!cgen(rt, ast->op.lhso, CAST_bit)) {
				traceAst(ast);
				return 0;
			}
			jmp1->go2.offs = emitopc(rt, opc);
			jmp1->go2.stks = stkoffs(rt, 0);
			jmp1->next = rt->cc->jmps;
			rt->cc->jmps = jmp1;

			if (!cgen(rt, ast->op.rhso, CAST_bit)) {
				traceAst(ast);
				return 0;
			}
			jmp2->go2.offs = emitopc(rt, opc);
			jmp2->go2.stks = stkoffs(rt, 0);
			jmp2->next = rt->cc->jmps;
			rt->cc->jmps = jmp2;
			// */

			#ifdef DEBUGGING	// extra check: validate some conditions.
			//~ dieif(ast->op.lhso->cast != ast->op.rhso->cast, "RemMe");
			dieif(ast->op.lhso->cast != CAST_bit, "RemMe");
			dieif(ast->op.lhso->cast != CAST_bit, "RemMe");
			dieif(got != castOf(ast->type), "RemMe");
			dieif(got != CAST_bit, "RemMe");
			#endif
			if (rt->cc->warn > 0) {
				static int firstTimeShowOnly = 1;
				if (firstTimeShowOnly) {
					warn(rt, 1, ast->file, ast->line, "operators `&&` and `||` does not short-circuit yet", ast);
					firstTimeShowOnly = 0;
				}
			}
		} break;
		case OPER_sel: {	// '?:'
			size_t jmpt, jmpf;
			if (rt->foldConst && eval(&tmp, ast->op.test)) {
				if (!cgen(rt, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, TYPE_any)) {
					traceAst(ast);
					return TYPE_any;
				}
			}
			else {
				size_t bppos = stkoffs(rt, 0);

				if (!cgen(rt, ast->op.test, CAST_bit)) {
					traceAst(ast);
					return TYPE_any;
				}

				if (!(jmpt = emitopc(rt, opc_jz))) {
					traceAst(ast);
					return TYPE_any;
				}

				if (!cgen(rt, ast->op.lhso, TYPE_any)) {
					traceAst(ast);
					return TYPE_any;
				}

				if (!(jmpf = emitopc(rt, opc_jmp))) {
					traceAst(ast);
					return TYPE_any;
				}

				fixjump(rt, jmpt, emitopc(rt, markIP), bppos);

				if (!cgen(rt, ast->op.rhso, TYPE_any)) {
					traceAst(ast);
					return TYPE_any;
				}

				fixjump(rt, jmpf, emitopc(rt, markIP), -1);
			}
		} break;

		case ASGN_set: {	// ':='
			// TODO: ast->type->size;
			size_t size = sizeOf(ast->type, 1);
			ccKind refAssign = CAST_ref;
			size_t codeBegin, codeEnd;

			dieif(size == 0, "Error: %+t", ast);

			if (ast->op.lhso->kind == CAST_ref) {
				symn typ = ast->op.lhso->type;
				//~ assign a reference type by reference
				if (typ->kind == TYPE_rec && typ->cast == CAST_ref) {
					debug("reference assignment: %+t", ast);
					dieif(got != CAST_ref, ERR_INTERNAL_ERROR);
					refAssign = ASGN_set;
					size = vm_size;
				}
			}

			codeBegin = emitopc(rt, markIP);
			if (!cgen(rt, ast->op.rhso, got)) {
				traceAst(ast);
				return TYPE_any;
			}

			if (get != CAST_vid) {
				/* TODO: int &a = b = 9;
				in case when: int &a = b = 9;
				dieif(get == CAST_ref, ERR_INTERNAL_ERROR);
				*/
				// in case a = b = sum(2, 700);
				// dupplicate the result
				if (!emitint(rt, opc_ldsp, 0)) {
					traceAst(ast);
					return TYPE_any;
				}
				if (!emitint(rt, opc_ldi, size)) {
					traceAst(ast);
					return TYPE_any;
				}
			}
			else {
				got = get;
			}

			codeEnd = emitopc(rt, markIP);
			if (!cgen(rt, ast->op.lhso, refAssign)) {
				traceAst(ast);
				return TYPE_any;
			}
			if (!emitint(rt, opc_sti, size)) {
				traceAst(ast);
				return TYPE_any;
			}

			// optimize assignments .
			if (ast->op.rhso->kind >= OPER_beg && ast->op.rhso->kind <= OPER_end) {
				if (ast->op.lhso == ast->op.rhso->op.lhso && rt->fastAssign) {
					// HACK: speed up for (int i = 0; i < 10, i += 1) ...
					if (optimizeAssign(rt, codeBegin, codeEnd)) {
						debug("assignment optimized: %+t", ast);
					}
				}
			}
		} break;
		//#}
		//#{ VALUES
		case TYPE_int:
			if (get == CAST_vid) {
				// void(0);
				return get;
			}

			if (!emiti64(rt, ast->cint)) {
				traceAst(ast);
				return TYPE_any;
			}
			got = CAST_i64;
			break;

		case TYPE_flt:
			if (get == CAST_vid) {
				// void(1.0);
				return get;
			}

			if (!emitf64(rt, ast->cflt)) {
				traceAst(ast);
				return TYPE_any;
			}
			got = CAST_f64;
			break;

		case TYPE_str: switch (get) {
			default:
				error(rt, ast->file, ast->line, "invalid cast of `%+t`", ast);
				return TYPE_any;

			case CAST_vid:
				// void("");
				return CAST_vid;

			case CAST_ref:
				if (!emitref(rt, ast->ref.name)) {
					traceAst(ast);
					return TYPE_any;
				}
				return CAST_ref;

			case CAST_arr:
				if (!emiti32(rt, strlen(ast->ref.name))) {
					traceAst(ast);
					return TYPE_any;
				}
				if (!emitref(rt, ast->ref.name)) {
					traceAst(ast);
					return TYPE_any;
				}
				return CAST_ref;
		} break;

		case CAST_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->ref.link;		// link
			// TODO: use ast->type->size;
			size_t size = sizeOf(typ, 1);

			dieif(typ == NULL, ERR_INTERNAL_ERROR);
			dieif(var == NULL, ERR_INTERNAL_ERROR);
			//TODO: size: dieif(size != typ->size, "Error: %-T: %d / %d", var, size, typ->size);

			// the enumeration is used as a value: `int pixels = Window.width * Window.height;`
			if (get != got && got == ENUM_kwd) {
				got = typ->type->cast;
			}

			switch (var->kind) {
				default:
					error(rt, ast->file, ast->line, "invalid rvalue `%+t:` %K", ast, var->kind);
					return TYPE_any;

				case CAST_arr:		// typename
				case TYPE_rec:		// typename
				case CAST_ref: {	// variable
					ccKind retarr = TYPE_any;
					dieif(var->size == 0, "invalid use of variable(%s:%d): `%-T`", ast->file, ast->line, var);

					// a slice is needed, push length first.
					if (get == CAST_arr && got != CAST_arr) {
						// ArraySize
						if (!emiti32(rt, typ->offs)) {
							traceAst(ast);
							return TYPE_any;
						}
						retarr = CAST_arr;
						get = CAST_ref;
					}

					if (!emitvar(rt, var)) {
						traceAst(ast);
						return TYPE_any;
					}

					// byVal references are assigned by value
					// byRef references are assigned by reference
					if (get == ASGN_set) {
						get = CAST_ref;
					}
					//TODO: else if (var != typ && (var->cast == CAST_ref || var->cast == CAST_arr)) {
					else if (var->cast == CAST_ref && var != typ) {
						if (!emitint(rt, opc_ldi, vm_size)) {
							traceAst(ast);
							return TYPE_any;
						}
					}

					if (get != CAST_ref) {
						if (get == CAST_arr && got == CAST_arr) {
							//~ info(rt, ast->file, ast->line, "assign to array from %K @ %+t", ret, ast);
							size = 2 * vm_size;
						}

						if (!emitint(rt, opc_ldi, size)) {
							traceAst(ast);
							return TYPE_any;
						}
					}
					else {
						if (var->cast == CAST_arr) {
							//~ info(rt, __FILE__, __LINE__, "assign to array from %K @ %+t", ret, ast);
							retarr = CAST_arr;
						}
						got = CAST_ref;
					}

					if (retarr != 0) {
						got = get = retarr;
					}
				} break;
				case EMIT_kwd:
					dieif(get == CAST_ref, ERR_INTERNAL_ERROR);
					if (!emitint(rt, (vmOpcode)var->offs, var->init ? constint(var->init) : 0)) {
						error(rt, ast->file, ast->line, "error emiting opcode: %+t", ast);
						if (stkoffs(rt, 0) > 0) {
							info(rt, ast->file, ast->line, "%+t underflows stack.", ast);
						}
						return TYPE_any;
					}
					return CAST_vid;

				case TYPE_def:
					//~ TODO: reimplement: it works, but ...
					if (var->init != NULL) {
						if (get == ENUM_kwd) {
							get = ast->cast;
						}
						return cgen(rt, var->init, get);
					}
			}
		} break;
		case TYPE_def: {					// new (var, func, define)
			symn typ = ast->type;
			symn var = ast->ref.link;
			size_t size = padded((size_t) sizeOf(var, 1), vm_size);
			size_t stktop = stkoffs(rt, size);

			dieif(typ == NULL, ERR_INTERNAL_ERROR);
			dieif(var == NULL, ERR_INTERNAL_ERROR);

			if (var->kind == CAST_ref) {

				// skip initialized static variables and function
				if (var->stat && var->offs && (var->call || !rt->cc->init)) {
					debug("variable already initialized: %+T", var);
					return CAST_vid;
				}

				// initialize (and allocate local) variables.
				if (var->init != NULL) {
					astn val = var->init;

					// ... a = emit(...);	// initialization with emit
					if (rt->cc->emit_opc && val->type == rt->cc->emit_opc) {
						got = get;
					}

					// string a = null;		// initialization with null
					else if (rt->cc->null_ref && rt->cc->null_ref == linkOf(val)) {
						debug("assigning null: %-T", var);
						val->cast = got = CAST_ref;
					}

					// int a(int x) = abs;	// reference initialization
					else if (var->call || var->cast == CAST_ref) {
						got = CAST_ref;
					}

					// int a[3] = {1,2,3};	// array initialization by elements
					if (typ->kind == CAST_arr && var->arrB == val->type) {
						size_t i, esize;
						symn base = typ;
						astn tmp = NULL;
						int nelem = 1;
						int ninit = 0;

						// TODO: base should not be stored in var->args !!!
						while (base && base != var->arrB) {
							if (base->init == NULL) {
								break;
							}
							// ArraySize
							nelem *= base->offs;
							base = base->type;
						}
						if (base == NULL) {
							base = typ;
						}

						dieif(!base, "Error %+t", ast);

						got = base->cast;
						esize = sizeOf(base, 1);

						// int a[8] = {0, ...};
						while (val->kind == OPER_com) {
							// stringify the initializers
							val->op.rhso->next = tmp;
							tmp = val->op.rhso;
							val = val->op.lhso;
						}
						val->next = tmp;

						// local variable
						if (var->offs == 0) {
							if (!emitint(rt, opc_spc, size)) {
								traceAst(ast);
								return TYPE_any;
							}
							i = padded(esize, vm_size);		// size of one element on stack
						}
						else {
							i = 0;
							size = 0;
						}

						for (tmp = val; val && ninit < nelem; ninit += 1, i += esize, val = val->next) {
							tmp = val;
							if (!cgen(rt, tmp, got)) {
								traceAst(tmp);
								return TYPE_any;
							}

							if (var->offs == 0) {
								if (!emitint(rt, opc_ldsp, i)) {
									traceAst(ast);
									return TYPE_any;
								}
							}
							else {
								if (!emitvar(rt, var)) {
									traceAst(ast);
									return TYPE_any;
								}
								if (!emitinc(rt, i)) {
									traceAst(ast);
									return TYPE_any;
								}
							}

							if (!emitint(rt, opc_sti, esize)) {
								traceAst(ast);
								return TYPE_any;
							}
						}

						if (ninit < nelem) {
							size_t loopCpy;
							size_t valOffs = 0;
							size_t dstOffs = 0;
							size_t stkOffs = stkoffs(rt, 0);

							// push val
							// push dst

							if (!cgen(rt, tmp, got)) {
								traceAst(tmp);
								return TYPE_any;
							}
							valOffs = stkoffs(rt, 0);

							if (var->offs == 0) {
								if (!emitint(rt, opc_ldsp, i - esize)) {
									traceAst(ast);
									return TYPE_any;
								}
							}
							else {
								if (!emitvar(rt, var)) {
									traceAst(ast);
									return TYPE_any;
								}
								if (!emitinc(rt, i - esize)) {
									traceAst(ast);
									return TYPE_any;
								}
							}

							// push end
							// push dst
							if (!emitint(rt, opc_dup1, 0)) {
								traceAst(ast);
								return TYPE_any;
							}
							if (!emitinc(rt, esize * (nelem - ninit))) {
								traceAst(ast);
								return TYPE_any;
							}

							if (!emitint(rt, opc_dup1, 1)) {
								traceAst(ast);
								return TYPE_any;
							}

							//~ :loopCopy
							loopCpy = emitopc(rt, markIP);
							dstOffs = stkoffs(rt, 0);
							//~ inc dst, size
							//~ dupp val
							//~ dupp dst
							//~ sti size
							if (!emitinc(rt, esize)) {
								traceAst(ast);
								return TYPE_any;
							}

							if (!emitidx(rt, opc_ldsp, valOffs)) {
								traceAst(ast);
								return TYPE_any;
							}
							if (!emitint(rt, opc_ldi, esize)) {
								traceAst(ast);
								return TYPE_any;
							}

							if (!emitidx(rt, opc_dup1, dstOffs)) {
								traceAst(ast);
								return TYPE_any;
							}

							if (!emitint(rt, opc_sti, esize)) {
								traceAst(ast);
								return TYPE_any;
							}

							//~ dupp.x2
							//~ clt.i32
							//~ jnz loopCopy
							if (!emitint(rt, opc_dup2, 0)) {
								traceAst(ast);
								return TYPE_any;
							}
							if (!emitopc(rt, i32_cgt)) {
								traceAst(ast);
								return TYPE_any;
							}
							if (!emitint(rt, opc_jnz, loopCpy)) {
								traceAst(ast);
								return TYPE_any;
							}
							if (!emitidx(rt, opc_drop, stkOffs)) {
								traceAst(ast);
								return TYPE_any;
							}
						}

						if (val != NULL) {
							error(rt, ast->file, ast->line, "Too many initializers: starting at `%+t`", val);
							return TYPE_any;
						}
					}

					// int a = 99;	// variable initialization
					else {
						logif(val->cast != var->cast, "cast error [%K -> %K] -> %K: %-T := %+t", val->cast, var->cast, get, var, val);
						switch (val->kind) {
							case TYPE_int:
							case TYPE_flt:
								val->type = var->type;
							default:
								break;
						}
						if (!cgen(rt, val, TYPE_any)) {
							traceAst(ast);
							return TYPE_any;
						}
					}

					if (var->offs == 0) {		// create local variable
						var->offs = stkoffs(rt, 0);
						if (stktop != var->offs) {
							error(rt, ast->file, ast->line, "invalid initializer size: %d diff(%d): `%+t`", stktop,
								  stktop - var->offs, ast);
							return TYPE_any;
						}
					}
					else if (size > 0) {		// initialize gloabal
						if (!emitvar(rt, var)) {
							traceAst(ast);
							return TYPE_any;
						}
						if (!emitint(rt, opc_sti, size)) {
							trace("%+t:sizeof(%-T) = %d", ast, var, size);
							return TYPE_any;
						}
					}
				}

				// alloc locally a block of the size of the type;
				else if (var->offs == 0) {
					logif(var->size != sizeOf(typ, 0), "size error: %T(%d, %d)", var, var->size, sizeOf(typ, 0));
					if (!emitint(rt, opc_spc, size)) {
						traceAst(ast);
						return TYPE_any;
					}
					var->offs = stkoffs(rt, 0);
					if (var->offs < size) {
						error(rt, ast->file, ast->line, "stack underflow", var->offs, size);
						return TYPE_any;
					}
				}
				else {
					// static uninitialized variable
				}
			}

			dieif(get != CAST_vid, ERR_INTERNAL_ERROR);
			get = got = CAST_vid;
		} break;

		case EMIT_kwd:
			traceAst(ast);
			return TYPE_any;
		//#}
	}

	// generate cast
	if (get != got) switch (get) {

		case CAST_vid:
			// FIXME: free stack.
			got = get;
			break;

		case TYPE_any: switch (got) {
			default:
				goto errorcast2;

			case CAST_vid:
			case CAST_ref:
				break;
		} break;

		case CAST_bit: switch (got) {		// to boolean
			default:
				goto errorcast2;

			case CAST_u32:
			case CAST_i32:
				if (!emitopc(rt, i32_bol)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
			case CAST_i64:
				if (!emitopc(rt, i64_bol)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
			case CAST_f32:
				if (!emitopc(rt, f32_bol)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
			case CAST_f64:
				if (!emitopc(rt, f64_bol)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
		} break;

		case CAST_u32: switch (got) {
			default:
				goto errorcast2;

			case CAST_bit:
			case CAST_i32:
				break;

			case CAST_i64: {
				if (!emitopc(rt, i64_i32)) {
					traceAst(ast);
					return TYPE_any;
				}
				get = got = TYPE_int;
			} break;
			//~ case CAST_f32: if (!emitopc(s, f32_i32)) return TYPE_any; break;
			//~ case CAST_f64: if (!emitopc(s, f64_i32)) return TYPE_any; break;
		} break;

		case CAST_i32: switch (got) {
			default:
				goto errorcast2;

			case CAST_bit:
			case CAST_u32:
				break;

			case CAST_i64:
				if (!emitopc(rt, i64_i32)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_f32:
				if (!emitopc(rt, f32_i32)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_f64:
				if (!emitopc(rt, f64_i32)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
		} break;

		case CAST_i64: switch (got) {
			default:
				goto errorcast2;

			case CAST_bit:
			case CAST_u32:
				if (!emitopc(rt, u32_i64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_i32:
				if (!emitopc(rt, i32_i64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_f32:
				if (!emitopc(rt, f32_i64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_f64:
				if (!emitopc(rt, f64_i64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
		} break;

		case CAST_f32: switch (got) {
			default:
				goto errorcast2;

			case CAST_bit:
			//~ case CAST_u32:
			case CAST_i32:
				if (!emitopc(rt, i32_f32)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_i64:
				if (!emitopc(rt, i64_f32)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_f64:
				if (!emitopc(rt, f64_f32)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
		} break;

		case CAST_f64: switch (got) {
			default:
				goto errorcast2;

			case CAST_bit:
			//~ case CAST_u32:
			case CAST_i32:
				if (!emitopc(rt, i32_f64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_i64:
				if (!emitopc(rt, i64_f64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case CAST_f32:
				if (!emitopc(rt, f32_f64)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
		} break;

		case CAST_ref:
		case CAST_arr: switch (got) {
			default:
				goto errorcast2;

			case EMIT_kwd:
				return EMIT_kwd;
		}

		case TYPE_ptr: switch (got) {
			default:
				goto errorcast2;

			case CAST_ref:
				return TYPE_ptr;
		}

		default:
			fatal("unimplemented(cast for `%+t`, %K):%K (%s:%d)", ast, get, got, ast->file, ast->line);
			// fall to next case

		errorcast2:
			trace("cgen[%K->%K](%+t)", got, get, ast);
			return TYPE_any;
	}

	// zero extend ...
	if (get == CAST_u32) {
		debug("zero extend[%K->%K]: %-T %+t (%s:%d)", got, get, ast->type, ast, ast->file, ast->line);
		switch (ast->type->size) {
			default:
				trace("Invalid cast(%K -> %K): %+t", got, get, ast);
				return TYPE_any;

			case 4:
				break;

			case 2:
				if (!emitint(rt, b32_bit, b32_bit_and | 16)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;

			case 1:
				if (!emitint(rt, b32_bit, b32_bit_and | 8)) {
					traceAst(ast);
					return TYPE_any;
				}
				break;
		}
	}

	#ifdef DEBUGGING	// extra check: every statement qualifier must be processed.
	logif(stmt_qual != 0, "unimplemented qualified statement %?K: %t", stmt_qual, ast);
	#endif

	return got;
}

int gencode(rtContext rt, int mode) {
	ccContext cc = rt->cc;
	size_t Lmain, Lmeta;

	// make global variables static
	int gStatic = rt->genGlobals;

	if (rt->errCount != 0) {
		trace("can not generate code with errors");
		return 0;
	}

	dieif(cc == NULL, "no compiler context");

	// leave the global scope.
	rt->main = ccDefType(rt, ".main", 0, 0);
	cc->vars = ccEnd(rt, NULL, gStatic);

	dieif(cc->vars != cc->gdef, "globals are not the same with defs");

	/* reorder the initialization of static variables and functions.
	 * TODO: optimize code.
	 *
	 *	ex: g must be generated before f
	 *	int f() {
	 *		static int g = 9;
	 *		// ...
	 *	}
	 */
	if (cc->gdef != NULL) {
		symn ng, pg = NULL;

		for (ng = cc->gdef; ng; ng = ng->gdef) {
			symn Ng, Pg = NULL;

			if (ng->call && ng->cast == CAST_ref) {
				// skip non functions
				continue;
			}

			for (Ng = ng; Ng != NULL; Ng = Ng->gdef) {
				if (Ng->decl == ng) {
					break;
				}
				Pg = Ng;
			}

			//~ this must be generated before sym;
			if (Ng != NULL) {
				trace("global `%T` must be generated before `%T`", Ng, ng);
				Pg->gdef = Ng->gdef;	// remove
				Ng->gdef = ng;
				if (pg) {
					pg->gdef = Ng;
				}
				else {
					cc->gdef = Ng;
					break;
				}
				ng = pg;
			}
			pg = ng;
		}
	}

	// set used memeory by metadata (string constants and typeinfos)
	Lmeta = rt->_beg - rt->_mem;

	// debuginfo
	if (mode != 0) {
		rt->dbg = (dbgContext)(rt->_beg = paddptr(rt->_beg, rt_size));
		rt->_beg += sizeof(struct dbgContextRec);

		dieif(rt->_beg >= rt->_end, ERR_MEMORY_OVERRUN);
		memset(rt->dbg, 0, sizeof(struct dbgContextRec));

		rt->dbg->rt = rt;
		initBuff(&rt->dbg->functions, 128, sizeof(struct dbgNode));
		initBuff(&rt->dbg->statements, 128, sizeof(struct dbgNode));
	}

	// libcalls
	if (cc->libc != NULL) {
		libc lc, calls;

		calls = (libc)(rt->_beg = paddptr(rt->_beg, rt_size));
		rt->_beg += sizeof(struct libc) * (cc->libc->pos + 1);
		dieif(rt->_beg >= rt->_end, ERR_MEMORY_OVERRUN);

		for (lc = cc->libc; lc; lc = lc->next) {
			// relocate libcall offsets to be unique and debugable.
			lc->sym->offs = vmOffset(rt, &calls[lc->pos]);
			lc->sym->size = sizeof(struct libc);
			calls[lc->pos] = *lc;
			addDbgFunction(rt, lc->sym);
		}
		rt->vm.libv = calls;
	}

	//~ read only memory ends here.
	//~ strings, typeinfos, TODO(constants, functions, enums, ...)
	rt->vm.ro = rt->_beg - rt->_mem;

	// TODO: generate functions first
	// static variables & functions
	if (cc->gdef != NULL) {
		symn var;

		// we will append the list of declarations here.
		astn staticinitializers = newnode(cc, STMT_beg);

		// generate global and static variables & functions
		for (var = cc->gdef; var; var = var->gdef) {

			// exclude null
			if (var == rt->cc->null_ref)
				continue;

			// exclude aliases and typenames
			if (var->kind != CAST_ref)
				continue;

			// exclude non static variables
			if (!var->stat)
				continue;

			dieif(var->offs != 0, "offset %06x for: %+T", var->offs, var);

			if (var->cnst && var->init == NULL) {
				error(cc->rt, var->file, var->line, "uninitialized constant `%+T`", var);
			}

			if (var->call && var->init == NULL) {
				error(cc->rt, var->file, var->line, "uninimplemented function `%+T`", var);
			}

			if (var->call && var->cast != CAST_ref) {
				size_t seg = emitopc(rt, markIP);

				if (!var->init) {
					dieif(1, "`%T` will be skipped", var);
					continue;
				}

				rt->cc->init = 0;
				rt->vm.sm = 0;
				fixjump(rt, 0, 0, vm_size + var->size);
				//TODO: why?:
				rt->vm.ro = stkoffs(rt, 0);
				rt->vm.sm = rt->vm.ss;		// leave return address on stack

				var->offs = emitopc(rt, markIP);

				if (!cgen(rt, var->init, CAST_vid)) {
					continue;
				}
				if (!emitopc(rt, opc_jmpi)) {
					error(rt, var->file, var->line, "error emiting function: %-T", var);
					continue;
				}
				while (cc->jmps) {
					error(rt, NULL, 0, "invalid jump: `%t`", cc->jmps);
					cc->jmps = cc->jmps->next;
				}
				var->size = emitopc(rt, markIP) - seg;
				var->stat = 1;

				addDbgFunction(rt, var);
			}
			else {
				unsigned align = rt_size;
				dieif(var->size <= 0, "Error %-T", var);	// instance of void ?
				dieif(var->offs != 0, "Error %-T", var);	// already generated ?

				// TODO: size of variable should be known here.
				dieif(var->size != sizeOf(var, 1), "size error: %-T: %d / %d", var, var->size, sizeOf(var, 1));
				var->size = sizeOf(var, 1);

				// align the memory of the variable. speeding up the read and write of it.
				if (var->size >= 16) {
					align = 16;
				}
				else if (var->size >= 8) {
					align = 8;
				}
				else if (var->size >= 4) {
					align = 4;
				}
				else if (var->size >= 2) {
					align = 2;
				}
				else if (var->size >= 1) {
					align = 1;
				}
				else {
					fatal(ERR_INTERNAL_ERROR);
					return 0;
				}

				rt->_beg = paddptr(rt->_beg, align);
				var->offs = vmOffset(rt, rt->_beg);
				rt->_beg += var->size;

				dieif(rt->_beg >= rt->_end, ERR_INTERNAL_ERROR);

				// TODO: recheck double initialization fix(var->nest > 0)
				if (var->init != NULL && var->nest > 0) {
					astn init = newnode(cc, TYPE_def);

					if (var->cnst && !isConstExpr(var->init)) {
						warn(rt, 16, var->file, var->line, "non constant initialization of static variable `%-T`", var);
					}

					//~ make initialization from initializer
					init->type = var->type;
					init->file = var->file;
					init->line = var->line;
					init->cast = var->cast;
					init->ref.link = var;

					init = wrapStmt(cc, init);

					if (staticinitializers->lst.head == NULL) {
						staticinitializers->lst.head = init;
					}
					else {
						staticinitializers->lst.tail->next = init;
					}
					staticinitializers->lst.tail = init;
				}
			}
			//~ trace("@%06x: %+T", var->offs, var);
		}

		// initialize static non global variables
		if (staticinitializers && staticinitializers->lst.tail) {
			dieif(cc->root == NULL || cc->root->kind != STMT_beg, ERR_INTERNAL_ERROR);
			staticinitializers->lst.tail->next = cc->root->stmt.stmt;
			cc->root->stmt.stmt = staticinitializers->lst.head;
		}
	}

	Lmain = emitopc(rt, markIP);
	if (cc->root != NULL) {
		rt->vm.sm = rt->vm.ss = 0;

		// enable static var initialization
		rt->cc->init = 1;

		dieif(cc->root->kind != STMT_beg, ERR_INTERNAL_ERROR);

		// CAST_vid clears the stack
		if (!cgen(rt, cc->root, gStatic ? CAST_vid : TYPE_any)) {
			trace(ERR_INTERNAL_ERROR);
			return 0;
		}

		while (cc->jmps) {
			error(rt, NULL, 0, "invalid jump: `%t`", cc->jmps);
			cc->jmps = cc->jmps->next;
			trace(ERR_INTERNAL_ERROR);
			return 0;
		}
	}

	// application exit point: halt(0)
	// !needed when invoking functions inside vm.
	rt->vm.px = emitopc(rt, opc_ldz1);
	emitint(rt, opc_libc, 0);

	// program entry point
	rt->vm.pc = Lmain;

	dieif(cc->vars != cc->gdef, "globals are not the same with defs");

	// buils the initialization function.
	rt->main->kind = CAST_ref;
	rt->main->type = cc->type_vid;
	rt->main->flds = cc->gdef;
	rt->main->prms = NULL;
	rt->main->init = cc->root;

	rt->main->file = "main";
	rt->main->line = 1;
	rt->main->size = emitopc(rt, markIP) - Lmain;
	rt->main->offs = Lmain;

	rt->_end = rt->_mem + rt->_size;
	if (rt->dbg != NULL) {
		int i, j;
		struct arrBuffer *codeMap = &rt->dbg->statements;
		for (i = 0; i < codeMap->cnt; ++i) {
			for (j = i; j < codeMap->cnt; ++j) {
				dbgn a = getBuff(codeMap, i);
				dbgn b = getBuff(codeMap, j);
				if (a->end > b->end) {
					memswap(a, b, sizeof(struct dbgNode));
				}
				else if (a->end == b->end) {
					if (a->start < b->start) {
						memswap(a, b, sizeof(struct dbgNode));
					}
				}
			}
		}
		addDbgFunction(rt, rt->main);
	}

	return rt->errCount == 0;
	(void)Lmeta;
}
