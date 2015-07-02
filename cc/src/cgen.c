/*******************************************************************************
 *   File: cgen.c
 *   Date: 2011/06/23
 *   Desc: code generation
 *******************************************************************************
description:
*******************************************************************************/
#include <string.h>
#include "core.h"

#ifdef DEBUGGING
// utility function for debuging only.
static void dumpTree(state rt, astn ast, size_t offsStart, size_t offsEnd) {
	struct symNode dbg;
	memset(&dbg, 0, sizeof(dbg));
	dbg.kind = TYPE_ref;
	dbg.name = "error";
	dbg.call = 1;
	dbg.init = ast;
	dbg.offs = offsStart;
	dbg.size = offsEnd - offsStart;
	dump(rt, dump_ast | dump_asm | 0x1ff, &dbg);
}
#endif

/**
 * @brief get absolute position on stack, of relative offset
 * @param rt Runtime context.
 * @param size size of variable.
 * @return the position of variable on stack.
 */
static inline size_t stkoffs(state rt, size_t size) {
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

size_t emitint(state rt, vmOpcode opc, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(rt, opc, tmp);
}

/// Emit an instruction indexing nth stack element.
static size_t emitidx(state rt, vmOpcode opc, size_t arg) {
	stkval tmp;
	tmp.i8 = rt->vm.ss * vm_size - arg;

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
			tmp.i8 /= vm_size;
			break;
	}

	if (tmp.i8 > vm_regs) {
		trace("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	if (tmp.i8 > rt->vm.ss * vm_size) {
		trace("opc_x%02x(%D(%d))", opc, tmp.i8, arg);
		return 0;
	}
	return emitarg(rt, opc, tmp);
}

/// Emit an instruction without argument.
static inline size_t emitopc(state rt, vmOpcode opc) {
	stkval arg;
	arg.i8 = 0;
	return emitarg(rt, opc, arg);
}

/// Increment the top of stack.
static inline size_t emitinc(state rt, size_t arg) {
	return emitint(rt, opc_inc, arg);
}

// emiting constant values.
static inline size_t emiti32(state rt, int32_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(rt, opc_lc32, tmp);
}
static inline size_t emiti64(state rt, int64_t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emitarg(rt, opc_lc64, tmp);
}
static inline size_t emitf64(state rt, float64_t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emitarg(rt, opc_lf64, tmp);
}
static inline size_t emitref(state rt, void* arg) {
	stkval tmp;
	tmp.i8 = vmOffset(rt, arg);
	return emitarg(rt, opc_lref, tmp);
}

// emit operator(add, sub, mul, ...), based on type
static size_t emitopr(state rt, vmOpcode opc, ccToken type) {
	// comparation
	if (opc == opc_ceq) switch (type) {
		default:
			break;

		case TYPE_bit:
		case TYPE_ref:
		case TYPE_u32:
		case TYPE_i32:
			opc = i32_ceq;
			break;

		case TYPE_f32:
			opc = f32_ceq;
			break;

		case TYPE_i64:
			opc = i64_ceq;
			break;

		case TYPE_f64:
			opc = f64_ceq;
			break;
	}
	else if (opc == opc_cne) {
		if (!emitopr(rt, opc_ceq, type)) {
			trace("Error");
			return 0;
		}
		opc = opc_not;
	}
	else if (opc == opc_clt) switch (type) {
		default:
			break;

		case TYPE_u32:
			opc = u32_clt;
			break;

		case TYPE_i32:
			opc = i32_clt;
			break;

		case TYPE_f32:
			opc = f32_clt;
			break;

		case TYPE_i64:
			opc = i64_clt;
			break;

		case TYPE_f64:
			opc = f64_clt;
			break;
	}
	else if (opc == opc_cge) {
		if (!emitopr(rt, opc_clt, type)) {
			trace("Error");
			return 0;
		}
		opc = opc_not;
	}
	else if (opc == opc_cgt) switch (type) {
		default:
			break;

		case TYPE_u32:
			opc = u32_cgt;
			break;

		case TYPE_i32:
			opc = i32_cgt;
			break;

		case TYPE_f32:
			opc = f32_cgt;
			break;

		case TYPE_i64:
			opc = i64_cgt;
			break;

		case TYPE_f64:
			opc = f64_cgt;
			break;
	}
	else if (opc == opc_cle) {
		if (!emitopr(rt, opc_cgt, type)) {
			trace("Error");
			return 0;
		}
		opc = opc_not;
	}

	// arithmetic
	else if (opc == opc_neg) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = i32_neg;
			break;

		case TYPE_i64:
			opc = i64_neg;
			break;

		case TYPE_f32:
			opc = f32_neg;
			break;

		case TYPE_f64:
			opc = f64_neg;
			break;
	}
	else if (opc == opc_add) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = i32_add;
			break;

		case TYPE_i64:
			opc = i64_add;
			break;

		case TYPE_f32:
			opc = f32_add;
			break;

		case TYPE_f64:
			opc = f64_add;
			break;
	}
	else if (opc == opc_sub) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = i32_sub;
			break;

		case TYPE_i64:
			opc = i64_sub;
			break;

		case TYPE_f32:
			opc = f32_sub;
			break;

		case TYPE_f64:
			opc = f64_sub;
			break;
	}
	else if (opc == opc_mul) switch (type) {
		default:
			break;

		case TYPE_u32:
			opc = u32_mul;
			break;

		case TYPE_i32:
			opc = i32_mul;
			break;

		case TYPE_i64:
			opc = i64_mul;
			break;

		case TYPE_f32:
			opc = f32_mul;
			break;

		case TYPE_f64:
			opc = f64_mul;
			break;
	}
	else if (opc == opc_div) switch (type) {
		default:
			break;

		case TYPE_u32:
			opc = u32_div;
			break;

		case TYPE_i32:
			opc = i32_div;
			break;

		case TYPE_i64:
			opc = i64_div;
			break;

		case TYPE_f32:
			opc = f32_div;
			break;

		case TYPE_f64:
			opc = f64_div;
			break;
	}
	else if (opc == opc_mod) switch (type) {
		default:
			break;

		case TYPE_u32:
			opc = u32_mod;
			break;

		case TYPE_i32:
			opc = i32_mod;
			break;

		case TYPE_i64:
			opc = i64_mod;
			break;

		case TYPE_f32:
			opc = f32_mod;
			break;

		case TYPE_f64:
			opc = f64_mod;
			break;
	}

	// bit operations
	else if (opc == opc_cmt) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = b32_cmt;
			break;

		/*case TYPE_i64:
			opc = b64_cmt;
			break;// */
	}
	else if (opc == opc_shl) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = b32_shl;
			break;

		/*case TYPE_i64:
			opc = b64_shl;
			break;// */
	}
	else if (opc == opc_shr) switch (type) {
		default:
			break;

		case TYPE_u32:
			opc = b32_shr;
			break;

		case TYPE_i32:
			opc = b32_sar;
			break;

		/*case TYPE_i64:
			opc = b64_sar;
			break;// */
	}
	else if (opc == opc_and) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = b32_and;
			break;

		/*case TYPE_i64:
			opc = b64_and;
			break;// */
	}
	else if (opc == opc_ior) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = b32_ior;
			break;

		/*case TYPE_i64:
			opc = b64_ior;
			break;// */
	}
	else if (opc == opc_xor) switch (type) {
		default:
			break;

		case TYPE_u32:
		case TYPE_i32:
			opc = b32_xor;
			break;

		/*case TYPE_i64:
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
static inline size_t emitvar(state rt, symn var) {
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
static ccToken cgen(state rt, astn ast, ccToken get) {
	#ifdef DEBUGGING
	ccToken stmt_qual = TYPE_any;
	#endif

	struct astNode tmp;
	ccToken got = TYPE_any;

	dieif(ast == NULL, "Error");
	dieif(ast->type == NULL, "Error");

	if (get == TYPE_any)
		get = ast->cst2;

	if (!(got = ast->type->cast)) {
		got = ast->type->kind;
	}

	#ifdef DEBUGGING
	// take care of qualified statements `static if` ...
	if (ast->kind >= STMT_beg && ast->kind <= STMT_end) {
		stmt_qual = ast->cst2;
	}
	#endif

	// generate code
	switch (ast->kind) {

		default:
			fatal(ERR_INTERNAL_ERROR);
			return TYPE_any;

		//#{ STATEMENTS
		case STMT_do: {	// expr statement
			size_t stpos = stkoffs(rt, 0);
			if (!cgen(rt, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			if (stpos > stkoffs(rt, 0)) {
				error(rt, ast->file, ast->line, "statement underflows stack: %+k", ast->stmt.stmt);
			}
		} break;
		case STMT_beg: {	// {} or function body
			size_t stpos = stkoffs(rt, 0);
			size_t ippar = 0;
			astn ptr;

			#ifdef DEBUGGING
			// TODO: a block may return nothing or the contained declarations(foreach may have 2 init declarations.)
			if (stmt_qual == TYPE_vid || stmt_qual == TYPE_rec) {
				stmt_qual = TYPE_any;
			}
			#endif
			if (ast->cst2 == QUAL_par) {
				ippar = emitopc(rt, opc_task);
				rt->vm.su = 0;

				#ifdef DEBUGGING
				// process qualifier
				stmt_qual = TYPE_any;
				#endif
			}
			for (ptr = ast->stmt.stmt; ptr; ptr = ptr->next) {
				size_t ipdbg = emitopc(rt, markIP);
				if (!cgen(rt, ptr, TYPE_vid)) {		// we will free stack on scope close
					#if DEBUGGING > 0
					dumpTree(rt, ptr, ipdbg, emitopc(rt, markIP));
					#endif
					error(rt, ptr->file, ptr->line, "emmiting statement `%+k`", ptr);
				}
				if (rt->dbg != NULL) {
					size_t ipEnd = emitopc(rt, markIP);
					if (ipdbg < ipEnd && ptr->kind != STMT_beg) {
						dbgMapCode(rt, ptr, ipdbg, ipEnd);
					}
				}
			}
			if (ast->cst2 == TYPE_rec) {
				debug("%t", get);
				get = TYPE_any;
			}
			if (get == TYPE_vid && stpos != stkoffs(rt, 0)) {
				// TODO: call destructor for variables
				if (!emitidx(rt, opc_drop, stpos)) {
					trace("error");
					return TYPE_any;
				}
			}
			if (ippar != 0) {
				fixjump(rt, ippar, emitopc(rt, markIP), rt->vm.su * vm_size);
				emitint(rt, opc_sync, 0);
				debug("parallel data on stack: %d", rt->vm.su);
			}
		} break;
		case STMT_if:  {
			size_t jmpt = 0, jmpf = 0;
			size_t stpos = stkoffs(rt, 0);
			int tt = eval(&tmp, ast->stmt.test);

			dieif(get != TYPE_vid, "Error");

			if (ast->cst2 == ATTR_stat) {
				if (ast->stmt.step || !tt) {
					error(rt, ast->file, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "else part is invalid");
					return TYPE_any;
				}
				#ifdef DEBUGGING
				// qualifier processed
				stmt_qual = TYPE_any;
				#endif
			}

			if (tt && (rt->vm.opti || ast->cst2 == ATTR_stat)) {	// static if then else
				astn gen = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
				if (gen && !cgen(rt, gen, TYPE_any)) {	// leave the stack
					trace("%+k", gen);
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
				block.kind = STMT_beg;
				block.type = rt->cc->type_vid;

				if (ast->stmt.stmt && ast->stmt.step) {		// if then else
					if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					if (!(jmpt = emitopc(rt, opc_jz))) {
						trace("%+k", ast);
						return TYPE_any;
					}

					block.stmt.stmt = ast->stmt.stmt;
					if (!cgen(rt, &block, TYPE_vid)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					if (!(jmpf = emitopc(rt, opc_jmp))) {
						trace("%+k", ast);
						return TYPE_any;
					}
					fixjump(rt, jmpt, emitopc(rt, markIP), -1);

					block.stmt.stmt = ast->stmt.step;
					if (!cgen(rt, &block, TYPE_vid)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					fixjump(rt, jmpf, emitopc(rt, markIP), -1);
				}
				else if (ast->stmt.stmt) {							// if then
					if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					//~ if false skip THEN block
					//~ TODO: warn: never executed code
					if (!(jmpt = emitopc(rt, opc_jz))) {
						trace("%+k", ast);
						return TYPE_any;
					}

					block.stmt.stmt = ast->stmt.stmt;
					if (!cgen(rt, &block, TYPE_vid)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					fixjump(rt, jmpt, emitopc(rt, markIP), -1);
				}
				else if (ast->stmt.step) {							// if else
					if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					//~ if true skip ELSE block
					//~ TODO: warn: never executed code
					if (!(jmpt = emitopc(rt, opc_jnz))) {
						trace("%+k", ast);
						return TYPE_any;
					}

					block.stmt.stmt = ast->stmt.step;
					if (!cgen(rt, &block, TYPE_vid)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					fixjump(rt, jmpt, emitopc(rt, markIP), -1);
				}
				dieif(get != TYPE_vid || stpos != stkoffs(rt, 0), "internal fatal error");
			}
			//~ TODO: destruct(ast->stmt.test)
			dieif(stpos != stkoffs(rt, 0), "invalid stacksize(%d:%d) in statement %+k", stkoffs(rt, 0), stpos, ast);
		} break;
		case STMT_for: {
			astn jl = rt->cc->jmps;
			size_t lincr;
			size_t jstep, lcont, lbody, lbreak;
			size_t stbreak, stpos = stkoffs(rt, 0);

			dieif(get != TYPE_vid, "Error");

			if (ast->stmt.init && !cgen(rt, ast->stmt.init, TYPE_vid)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (!(jstep = emitopc(rt, opc_jmp))) {		// continue;
				trace("%+k", ast);
				return TYPE_any;
			}

			lbody = emitopc(rt, markIP);
			if (ast->stmt.stmt && !cgen(rt, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			lcont = emitopc(rt, markIP);
			if (ast->stmt.step && !cgen(rt, ast->stmt.step, TYPE_vid)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			lincr = emitopc(rt, markIP);
			fixjump(rt, jstep, emitopc(rt, markIP), -1);
			if (ast->stmt.test) {
				if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				if (!emitint(rt, opc_jnz, lbody)) {		// continue;
					trace("%+k", ast);
					return TYPE_any;
				}
			}
			else {
				if (!emitint(rt, opc_jmp, lbody)) {		// continue;
					trace("%+k", ast);
					return TYPE_any;
				}
			}

			lbreak = emitopc(rt, markIP);
			stbreak = stkoffs(rt, 0);

			if (rt->dbg != NULL) {
				if (lcont < lincr) {
					dbgMapCode(rt, ast->stmt.step, lcont, lincr);
				}
				if (lincr < lbreak) {
					dbgMapCode(rt, ast->stmt.test, lincr, lbreak);
				}
				/*if (lcont < lbreak) {
					dbgMapCode(rt, ast, lcont, lbreak);
				}*/
			}

			while (rt->cc->jmps != jl) {
				astn jmp = rt->cc->jmps;
				rt->cc->jmps = jmp->next;

				if (jmp->go2.stks != stbreak) {
					error(rt, jmp->file, jmp->line, "`%k` statement is invalid due to previous variable declaration within loop", jmp);
					return TYPE_any;
				}
				switch (jmp->kind) {
					default :
						error(rt, jmp->file, jmp->line, "invalid goto statement: %k", jmp);
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
				dieif(!emitidx(rt, opc_drop, stpos), "Error");
			}
		} break;
		case STMT_con:
		case STMT_brk: {
			size_t offs;
			dieif(get != TYPE_vid, "Error");
			if (!(offs = emitopc(rt, opc_jmp))) {
				trace("%+k", ast);
				return TYPE_any;
			}

			ast->go2.offs = offs;
			ast->go2.stks = stkoffs(rt, 0);

			ast->next = rt->cc->jmps;
			rt->cc->jmps = ast;
		} break;
		case STMT_ret: {
			//~ TODO: declared reference variables should be freed.
			size_t bppos = stkoffs(rt, 0);
			if (ast->stmt.stmt && !cgen(rt, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			dieif(get != TYPE_vid, "Error");
			if (get == TYPE_vid && rt->vm.ro != stkoffs(rt, 0)) {
				if (!emitidx(rt, opc_drop, rt->vm.ro)) {
					trace("leve %d", rt->vm.ro);
					return TYPE_any;
				}
			}
			if (!emitopc(rt, opc_jmpi)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			fixjump(rt, 0, 0, bppos);
		} break;
		//#}
		//#{ OPERATORS
		case OPER_fnc: {	// '()' emit/call/cast
			size_t stktop = stkoffs(rt, 0);
			size_t stkret = stkoffs(rt, sizeOf(ast->type, 1));
			astn argv = ast->op.rhso;
			symn var = linkOf(ast->op.lhso);

			dieif(var == NULL && ast->op.lhso != NULL, "Error %+k", ast);

			// pointer(&info); variant(&info);
			if (var && (var == rt->cc->type_ptr || var == rt->cc->type_var)) {
				if (argv && argv->kind != OPER_com) {
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
								trace("%+k", ast);
								return TYPE_any;
							}
						}
						// push ref
						if (!emitvar(rt, object)) {
							trace("%+k", ast);
							return TYPE_any;
						}

						switch (object->cast) {
							default:
								if (argv->kind != OPER_adr && object->type->cast != TYPE_ref) {
									warn(rt, 2, argv->file, argv->line, "argument `%+k` is not explicitly passed by reference", argv);
								}
								break;

							case TYPE_arr:	// from slice
								warn(rt, 2, argv->file, argv->line, "converting `%+k` to %-T discards length property", argv, ast->type);
								//~ TODO: loadIndirect = 1;
								break;

							case TYPE_var:	// from variant
								warn(rt, 2, argv->file, argv->line, "converting `%+k` to %-T discards type property", argv, ast->type);
								//~ TODO: loadIndirect = 1;
								break;

							case TYPE_ref:	// from reference
								loadIndirect = 1;
								break;
						}
						if (loadIndirect != 0) {
							if (!emitint(rt, opc_ldi, vm_size)) {
								trace("%+k", ast);
								return TYPE_any;
							}
						}
						return TYPE_var;
					}
				}
			}

			// debug(...)
			if (var && var == rt->cc->libc_dbg) {
				if (argv != NULL) {
					while (argv->kind == OPER_com) {
						astn arg = argv->op.rhso;

						if (!cgen(rt, arg, arg->cst2)) {
							trace("%+k", arg);
							return TYPE_any;
						}
						argv = argv->op.lhso;
					}
					if (!cgen(rt, argv, argv->cst2)) {
						trace("%+k", argv);
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
				int chachedArgSize = 0;
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
						int useCount = usages(param) - 1;
						astn arg = an;

						// TODO: skip over aliases?
						while (arg && arg->kind == TYPE_ref) {
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
						if (rt->vm.opti && eval(&tmp, arg)) {
							arg = &tmp;
						}

						/* TODO: make args inlineable
						if (param->cast != TYPE_ref) {
							switch (arg->kind) {
								default:
									if (useCount <= 1) {
										inlineArg = 1;
									}
									break;

								case TYPE_ref:
									if (arg->type == param->type) {
										symn link = arg->ref.link;
										// static variables should not be inlined ?
										if (link->stat) {
											if (useCount > 1) {
												break;
											}
										}
										// what about indirect references ?
										//~ if (link->cast == TYPE_ref) { break; }
										inlineArg = 1;
									}
									break;
							}
						}
						//~ */

						inlineArg = 0;
						if (inlineArg || param->kind == TYPE_def) {
							if (param->cast != TYPE_def) {
								warn(rt, 16, ast->file, ast->line, "inlineing(%d) argument used %d times: %+T: %+k", inlineArg, useCount, param, arg);
							}
							//~ param->kind = TYPE_def;
							param->init = an;
						}
						else {
							size_t stktop = stkoffs(rt, sizeOf(param, 1));
							chachedArgSize += sizeOf(param, 1);
							warn(rt, 16, ast->file, ast->line, "caching argument used %d times: %+T: %+k", useCount, param, arg);
							if (!cgen(rt, an, param->cast == TYPE_def ? param->type->cast : param->cast)) {
								trace("%+k", arg);
								return TYPE_any;
							}

							param->offs = stkoffs(rt, 0);
							//~ param->kind = TYPE_ref;
							if (stktop != param->offs) {
								error(rt, ast->file, ast->line, "invalid size: %d <> got(%d): `%+k`", stktop, param->offs, param);
								return TYPE_any;
							}
						}
						an = an->next;
						param = param->next;
					}
					dieif(an || param, "Error");
				}

				if (var->init) {
					var->init->file = ast->file;
					var->init->line = ast->line;
				}

				if (!(got = cgen(rt, var->init, got))) {
					trace("%+k", ast);
					return TYPE_any;
				}

				if (stkret != stkoffs(rt, 0)) {
					logif(chachedArgSize != stkoffs(rt, 0) - stkret, "%+T(%d%+d): %+k", ast->type, stkret, stkoffs(rt, 0)-stkret, ast);
					if (get != TYPE_vid) {
						if (!emitidx(rt, opc_ldsp, stkret)) {	// get stack
							trace("error");
							return TYPE_any;
						}
						if (!emitint(rt, opc_sti, sizeOf(ast->type, 1))) {
							error(rt, ast->file, ast->line, "store indirect: %T:%d of `%+k`", ast->type, sizeOf(ast->type, 1), ast);
							return TYPE_any;
						}
					}
					if (stkret < stkoffs(rt, 0)) {
						if (!emitidx(rt, opc_drop, stkret)) {
							trace("%+k", ast);
							return TYPE_any;
						}
					}
				}

				for (param = var->prms; param; param = param->next) {
					param->init = NULL;
					// TODO: ugly hack
					//~ param->kind = param->cast == TYPE_def ? TYPE_def : TYPE_ref;
				}
				break;
			}

			// push Result, Arguments
			if (var && !istype(var) && var != rt->cc->emit_opc && var->call) {
				if (sizeOf(var->type, 1) && !emitint(rt, opc_spc, sizeOf(var->type, 1))) {
					trace("%+k", ast);
					return TYPE_any;
				}
				// result size on stack
				dieif(stkret != stkoffs(rt, 0), "Error");
			}
			if (argv != NULL) {
				astn argl = NULL;

				while (argv->kind == OPER_com) {
					astn arg = argv->op.rhso;
					if (!cgen(rt, arg, arg->cst2)) {
						trace("%+k", arg, arg->cst2);
						return TYPE_any;
					}
					arg->next = argl;
					argl = arg;
					argv = argv->op.lhso;
				}

				// static casting with emit: int32 i = emit(int32, float32(3.14));
				if (var == rt->cc->emit_opc && isType(argv)) {
					break;
				}

				if (!cgen(rt, argv, argv->cst2)) {
					trace("%+k", argv);
					return TYPE_any;
				}

				if (var && var != rt->cc->emit_opc && var != rt->cc->libc_dbg && var->call && var->cast != TYPE_ref) {
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
			else if (istype(var)) {				// cast()
				dieif(stkret != stkoffs(rt, 0), "Error: %+k", ast);
				if (!argv || argv != ast->op.rhso) {
					warn(rt, 1, ast->file, ast->line, "multiple values, array ?: '%+k'", ast);
				}
			}
			else if (var) {						// call()
				if (var->call) {
					if (!cgen(rt, ast->op.lhso, TYPE_ref)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					if (!emitopc(rt, opc_call)) {
						trace("%+k", ast);
						return TYPE_any;
					}
					// clean the stack
					if (stkret != stkoffs(rt, 0)) {
						if (!emitidx(rt, opc_drop, stkret)) {
							trace("%+k", ast);
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
				dieif(ast->op.lhso != NULL, "Error %+k", ast);
				if (!argv || argv != ast->op.rhso) {
					warn(rt, 1, ast->file, ast->line, "multiple values, array ?: '%+k'", ast);
				}
			}
		} break;
		case OPER_idx: {	// '[]'
			int r;
			size_t esize;
			if (!(r = cgen(rt, ast->op.lhso, TYPE_ref))) {
				trace("%+k", ast);
				return TYPE_any;
			}
			if (r == TYPE_arr) {
				if (!emitint(rt, opc_ldi, vm_size)) {
					trace("%+k", ast);
					return TYPE_any;
				}
			}

			esize = sizeOf(ast->type, 0);	// size of array element
			if (rt->vm.opti && eval(&tmp, ast->op.rhso) == TYPE_int) {
				size_t offs = sizeOf(ast->type, 0) * (int)constint(&tmp);
				if (!emitinc(rt, offs)) {
					trace("%+k", ast);
					return TYPE_any;
				}
			}
			else {
				if (!cgen(rt, ast->op.rhso, TYPE_u32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				if (esize > 1) {
					if (!emitint(rt, opc_mad, esize)) {
						trace("%+k", ast);
						return TYPE_any;
					}
				}
				else if (esize == 1) {
					if (!emitopc(rt, i32_add)) {
						trace("%+k", ast);
						return TYPE_any;
					}
				}
				else {
					fatal("invalid array element size");
					return TYPE_any;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			// ve need the value on that position (this can be a ref).
			if (!emitint(rt, opc_ldi, esize)) {
				trace("%+k", ast);
				return TYPE_any;
			}
		} break;
		case OPER_dot: {	// '.'
			// TODO: this should work as indexing
			symn object = linkOf(ast->op.lhso);
			symn member = linkOf(ast->op.rhso);
			int lhsstat = isType(ast->op.lhso);

			if (!object || !member) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (!member->stat && lhsstat) {
				error(rt, ast->file, ast->line, "An object reference is required to access the member `%+T`", member);
				return TYPE_any;
			}
			if (member->stat) {
				if (!lhsstat && object->kind != TYPE_arr) {
					warn(rt, 5, ast->file, ast->line, "accessing static member using instance variable `%+T`/ %+T", member, object->type);
				}
				return cgen(rt, ast->op.rhso, get);
			}

			if (member->memb) {
				if (!cgen(rt, ast->op.lhso, TYPE_ref)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				return cgen(rt, ast->op.rhso, get);
			}
			if (member->kind == TYPE_def) {
				// static array length is of this type
				debug("accessing inline field %+T: %+k", member, ast);
				return cgen(rt, ast->op.rhso, get);
			}

			if (!cgen(rt, ast->op.lhso, TYPE_ref)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (!emitinc(rt, member->offs)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (member->cast == TYPE_ref) {
				if (!emitint(rt, opc_ldi, vm_size)) {
					trace("%+k", ast);
					return TYPE_any;
				}
			}

			if (get == TYPE_ref) {
				return TYPE_ref;
			}

			if (!emitint(rt, opc_ldi, sizeOf(ast->type, 1))) {
				trace("%+k", ast);
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
					dieif(got != TYPE_bit, "Error");
					opc = opc_not;
					break;
				case OPER_cmt:
					opc = opc_cmt;
					break;
			}
			if (!cgen(rt, ast->op.rhso, got)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			if (!emitopr(rt, opc, got)) {
				trace("%+k", ast);
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
			#ifdef DEBUGGING
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
				trace("%+k", ast);
				return TYPE_any;
			}
			if (!cgen(rt, ast->op.rhso, TYPE_any)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			if (!emitopr(rt, opc, ast->op.lhso->cst2)) {	// uint % int => u32.mod
				trace("opc__%02x:%+k", opc, ast);
				return TYPE_any;
			}

			#ifdef DEBUGGING
			// these must be true
			if (ast->op.lhso->cst2 != ast->op.rhso->cst2) {
				struct symNode dbg;
				memset(&dbg, 0, sizeof(dbg));
				dbg.kind = TYPE_ref;
				dbg.name = "error";
				dbg.call = 1;
				dbg.init = ast;
				dbg.offs = ipdbg;
				dbg.size = emitopc(rt, markIP) - ipdbg;
				dump(rt, dump_ast | dump_asm | 0x1ff, &dbg);
			}
			dieif(ast->op.lhso->cst2 != ast->op.rhso->cst2, "RemMe", ast);
			dieif(got != castOf(ast->type), "RemMe");
			switch (ast->kind) {
				case OPER_neq:
				case OPER_equ:
				case OPER_geq:
				case OPER_lte:
				case OPER_leq:
				case OPER_gte:
					dieif(got != TYPE_bit, "RemMe(%t): %+7k", got, ast);
				default:
					break;
			}
			#endif
		} break;

		case OPER_lnd:		// '&&'
		case OPER_lor: {	// '||'
			vmOpcode opc;
			// TODO: short circuit && and ||
			switch (ast->kind) {
				default:
					fatal(ERR_INTERNAL_ERROR);
					return TYPE_any;

				case OPER_lnd:
					opc = opc_and;
					break;
				case OPER_lor:
					opc = opc_ior;
					break;
			}

			if (!cgen(rt, ast->op.lhso, TYPE_bit)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (!cgen(rt, ast->op.rhso, TYPE_bit)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (!emitopr(rt, opc, TYPE_u32)) {
				trace("opc__%02x:%+k", opc, ast);
				return TYPE_any;
			}

			/*astn jmp1 = 0, jmp2 = 0;
			switch (ast->kind) {
				default:
					fatal("Error");
					return 0;

				case OPER_lnd:
					jmp1 = newnode(rt->cc, STMT_brk);
					jmp2 = newnode(rt->cc, STMT_brk);
					opc = opc_jnz;
					break;

				case OPER_lor:
					jmp1 = newnode(rt->cc, STMT_con);
					jmp2 = newnode(rt->cc, STMT_con);
					opc = opc_jz;
					break;
			}

			if (!cgen(rt, ast->op.lhso, TYPE_bit)) {
				trace("%+k", ast);
				return 0;
			}
			jmp1->go2.offs = emitopc(rt, opc);
			jmp1->go2.stks = stkoffs(rt, 0);
			jmp1->next = rt->cc->jmps;
			rt->cc->jmps = jmp1;

			if (!cgen(rt, ast->op.rhso, TYPE_bit)) {
				trace("%+k", ast);
				return 0;
			}
			jmp2->go2.offs = emitopc(rt, opc);
			jmp2->go2.stks = stkoffs(rt, 0);
			jmp2->next = rt->cc->jmps;
			rt->cc->jmps = jmp2;
			// */

			#ifdef DEBUGGING
			if (rt->cc->warn > 0) {
				static int firstTimeShowOnly = 1;
				if (firstTimeShowOnly) {
					warn(rt, 1, ast->file, ast->line, "operators `&&` and `||` does not short-circuit yet", ast);
					firstTimeShowOnly = 0;
				}
			}
			// these must be true
			//~ dieif(ast->op.lhso->cst2 != ast->op.rhso->cst2, "RemMe");
			dieif(ast->op.lhso->cst2 != TYPE_bit, "RemMe");
			dieif(ast->op.lhso->cst2 != TYPE_bit, "RemMe");
			dieif(got != castOf(ast->type), "RemMe");
			dieif(got != TYPE_bit, "RemMe");
			#endif
		} break;
		case OPER_sel: {	// '?:'
			size_t jmpt, jmpf;
			if (rt->vm.opti && eval(&tmp, ast->op.test)) {
				if (!cgen(rt, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, TYPE_any)) {
					trace("%+k", ast);
					return TYPE_any;
				}
			}
			else {
				size_t bppos = stkoffs(rt, 0);

				if (!cgen(rt, ast->op.test, TYPE_bit)) {
					trace("%+k", ast);
					return TYPE_any;
				}

				if (!(jmpt = emitopc(rt, opc_jz))) {
					trace("%+k", ast);
					return TYPE_any;
				}

				if (!cgen(rt, ast->op.lhso, TYPE_any)) {
					trace("%+k", ast);
					return TYPE_any;
				}

				if (!(jmpf = emitopc(rt, opc_jmp))) {
					trace("%+k", ast);
					return TYPE_any;
				}

				fixjump(rt, jmpt, emitopc(rt, markIP), bppos);

				if (!cgen(rt, ast->op.rhso, TYPE_any)) {
					trace("%+k", ast);
					return TYPE_any;
				}

				fixjump(rt, jmpf, emitopc(rt, markIP), -1);
			}
		} break;

		case ASGN_set: {	// ':='
			// TODO: ast->type->size;
			size_t size = sizeOf(ast->type, 1);
			ccToken refAssign = TYPE_ref;
			int codeBegin, codeEnd;

			dieif(size == 0, "Error: %+k", ast);

			if (ast->op.lhso->kind == TYPE_ref) {
				symn typ = ast->op.lhso->type;
				//~ assign a reference type by reference
				if (typ->kind == TYPE_rec && typ->cast == TYPE_ref) {
					trace("reference assignment: %+k", ast);
					dieif(got != TYPE_ref, "Error");
					refAssign = ASGN_set;
					size = vm_size;
				}
			}

			codeBegin = emitopc(rt, markIP);
			if (!cgen(rt, ast->op.rhso, got)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			if (get != TYPE_vid) {
				/* TODO: int &a = b = 9;
				in case when: int &a = b = 9;
				dieif(get == TYPE_ref, "Error");
				*/
				// in case a = b = sum(2, 700);
				// dupplicate the result
				if (!emitint(rt, opc_ldsp, 0)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				if (!emitint(rt, opc_ldi, size)) {
					trace("%+k", ast);
					return TYPE_any;
				}
			}
			else {
				got = get;
			}

			codeEnd = emitopc(rt, markIP);
			if (!cgen(rt, ast->op.lhso, refAssign)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			if (!emitint(rt, opc_sti, size)) {
				trace("%+k", ast);
				return TYPE_any;
			}

			// optimize assignments .
			if (ast->op.rhso->kind >= OPER_beg && ast->op.rhso->kind <= OPER_end) {
				if (ast->op.lhso == ast->op.rhso->op.lhso) {
					// HACK: speed up for (int i = 0; i < 10, i += 1) ...
					if (optimizeAssign(rt, codeBegin, codeEnd)) {
						debug("assignment optimized: %+k", ast);
					}
				}
			}
		} break;
		//#}
		//#{ VALUES
		case TYPE_int:
			if (get == TYPE_vid) {
				// void(0);
				return get;
			}

			if (!emiti64(rt, ast->cint)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			got = TYPE_i64;
			break;

		case TYPE_flt:
			if (get == TYPE_vid) {
				// void(1.0);
				return get;
			}

			if (!emitf64(rt, ast->cflt)) {
				trace("%+k", ast);
				return TYPE_any;
			}
			got = TYPE_f64;
			break;

		case TYPE_str: switch (get) {
			default:
				error(rt, ast->file, ast->line, "invalid cast of `%+k`", ast);
				return TYPE_any;

			case TYPE_vid:
				// void("");
				return TYPE_vid;

			case TYPE_ref:
				if (!emitref(rt, ast->ref.name)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				return TYPE_ref;

			case TYPE_arr:
				if (!emiti32(rt, strlen(ast->ref.name))) {
					trace("%+k", ast);
					return TYPE_any;
				}
				if (!emitref(rt, ast->ref.name)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				return TYPE_ref;
		} break;

		case TYPE_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->ref.link;		// link
			// TODO: use ast->type->size;
			size_t size = sizeOf(typ, 1);

			dieif(typ == NULL, "Error");
			dieif(var == NULL, "Error");
			//TODO: size: dieif(size != typ->size, "Error: %-T: %d / %d", var, size, typ->size);

			// the enumeration is used as a value: `int pixels = Window.width * Window.height;`
			if (get != got && got == ENUM_kwd) {
				got = typ->type->cast;
			}

			switch (var->kind) {
				default:
					error(rt, ast->file, ast->line, "invalid rvalue `%+k:` %t", ast, var->kind);
					return TYPE_any;

				case TYPE_arr:		// typename
				case TYPE_rec:		// typename
				case TYPE_ref: {	// variable
					ccToken retarr = TYPE_any;
					dieif(var->size == 0, "invalid use of variable(%s:%d): `%-T`", ast->file, ast->line, var);

					// a slice is needed, push length first.
					if (get == TYPE_arr && got != TYPE_arr) {
						// ArraySize
						if (!emiti32(rt, typ->offs)) {
							trace("%+k", ast);
							return TYPE_any;
						}
						retarr = TYPE_arr;
						get = TYPE_ref;
					}

					if (!emitvar(rt, var)) {
						trace("%+k", ast);
						return TYPE_any;
					}

					// byVal references are assigned by value
					// byRef references are assigned by reference
					if (get == ASGN_set) {
						get = TYPE_ref;
					}
					//TODO: else if (var != typ && (var->cast == TYPE_ref || var->cast == TYPE_arr)) {
					else if (var->cast == TYPE_ref && var != typ) {
						if (!emitint(rt, opc_ldi, vm_size)) {
							trace("%+k", ast);
							return TYPE_any;
						}
					}

					if (get != TYPE_ref) {
						if (get == TYPE_arr && got == TYPE_arr) {
							//~ info(rt, ast->file, ast->line, "assign to array from %t @ %k", ret, ast);
							size = 8;
						}

						if (!emitint(rt, opc_ldi, size)) {
							trace("%+k", ast);
							return TYPE_any;
						}
					}
					else {
						if (var->cast == TYPE_arr) {
							//~ info(rt, __FILE__, __LINE__, "assign to array from %t @ %k", ret, ast);
							retarr = TYPE_arr;
						}
						got = TYPE_ref;
					}

					if (retarr != 0) {
						got = get = retarr;
					}
				} break;
				case EMIT_opc:
					dieif(get == TYPE_ref, "Error");
					if (!emitint(rt, (vmOpcode)var->offs, var->init ? constint(var->init) : 0)) {
						error(rt, ast->file, ast->line, "error emiting opcode: %+k", ast);
						if (stkoffs(rt, 0) > 0) {
							info(rt, ast->file, ast->line, "%+k underflows stack.", ast);
						}
						return TYPE_any;
					}
					return TYPE_vid;

				case TYPE_def:
					//~ TODO: reimplement: it works, but ...
					if (var->init != NULL) {
						if (get == ENUM_kwd) {
							get = ast->cst2;
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

			dieif(typ == NULL, "Error");
			dieif(var == NULL, "Error");

			if (var->kind == TYPE_ref) {

				// skip initialized static variables and function
				if (var->stat && var->offs && (var->call || !rt->cc->init)) {
					debug("variable already initialized: %+T", var);
					return TYPE_vid;
				}

				// initialize (and allocate local) variables.
				if (var->init != NULL) {
					astn val = var->init;

					// ... a = emit(...);	// initialization with emit
					if (val->type == rt->cc->emit_opc) {
						got = get;
					}

					// string a = null;		// initialization with null
					else if (rt->cc->null_ref == linkOf(val)) {
						trace("assigning null: %-T", var);
						val->cst2 = got = TYPE_ref;
					}

					// int a(int x) = abs;	// reference initialization
					else if (var->call || var->cast == TYPE_ref) {
						got = TYPE_ref;
					}

					// int a[3] = {1,2,3};	// array initialization by elements
					// FIXME: if valuetype is arrays base type
					if (typ->kind == TYPE_arr && var->prms == val->type) {
						size_t i, esize;
						symn base = typ;
						astn tmp = NULL;
						int nelem = 1;
						int ninit = 0;

						// TODO: base should not be stored in var->args !!!
						while (base && base != var->prms) {
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

						dieif(!base, "Error %+k", ast);

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
								trace("%+k", ast);
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
								trace("%+k", tmp);
								return TYPE_any;
							}

							if (var->offs == 0) {
								if (!emitint(rt, opc_ldsp, i)) {
									trace("%+k", ast);
									return TYPE_any;
								}
							}
							else {
								if (!emitvar(rt, var)) {
									trace("%+k", ast);
									return TYPE_any;
								}
								if (!emitinc(rt, i)) {
									trace("%+k", ast);
									return TYPE_any;
								}
							}

							if (!emitint(rt, opc_sti, esize)) {
								trace("%+k", ast);
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
								trace("%+k", tmp);
								return TYPE_any;
							}
							valOffs = stkoffs(rt, 0);

							if (var->offs == 0) {
								if (!emitint(rt, opc_ldsp, i - esize)) {
									trace("%+k", ast);
									return TYPE_any;
								}
							}
							else {
								if (!emitvar(rt, var)) {
									trace("%+k", ast);
									return TYPE_any;
								}
								if (!emitinc(rt, i - esize)) {
									trace("%+k", ast);
									return TYPE_any;
								}
							}

							// push end
							// push dst
							if (!emitint(rt, opc_dup1, 0)) {
								trace("%+k", ast);
								return TYPE_any;
							}
							if (!emitinc(rt, esize * (nelem - ninit))) {
								trace("%+k", ast);
								return TYPE_any;
							}

							if (!emitint(rt, opc_dup1, 1)) {
								trace("%+k", ast);
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
								trace("%+k", ast);
								return TYPE_any;
							}

							if (!emitidx(rt, opc_ldsp, valOffs)) {
								trace("%+k", ast);
								return TYPE_any;
							}
							if (!emitint(rt, opc_ldi, esize)) {
								trace("%+k", ast);
								return TYPE_any;
							}

							if (!emitidx(rt, opc_dup1, dstOffs)) {
								trace("%+k", ast);
								return TYPE_any;
							}

							if (!emitint(rt, opc_sti, esize)) {
								trace("%+k", ast);
								return TYPE_any;
							}

							//~ dupp.x2
							//~ clt.i32
							//~ jnz loopCopy
							if (!emitint(rt, opc_dup2, 0)) {
								trace("%+k", ast);
								return TYPE_any;
							}
							if (!emitopc(rt, i32_cgt)) {
								trace("%+k", ast);
								return TYPE_any;
							}
							if (!emitint(rt, opc_jnz, loopCpy)) {
								trace("%+k", ast);
								return TYPE_any;
							}
							if (!emitidx(rt, opc_drop, stkOffs)) {
								trace("%+k", ast);
								return TYPE_any;
							}
						}

						if (val != NULL) {
							error(rt, ast->file, ast->line, "Too many initializers: starting at `%+k`", val);
							return TYPE_any;
						}
					}

					// int a = 99;	// variable initialization
					else {
						logif(val->cst2 != var->cast, "cast error [%t -> %t] -> %t: %-T := %+k", val->cst2, var->cast, get, var, val);
						switch (val->kind) {
							case TYPE_int:
							case TYPE_flt:
								val->type = var->type;
							default:
								break;
						}
						if (!cgen(rt, val, TYPE_any)) {
							trace("%+k", ast);
							return TYPE_any;
						}
					}

					if (var->offs == 0) {		// create local variable
						var->offs = stkoffs(rt, 0);
						if (stktop != var->offs) {
							error(rt, ast->file, ast->line, "invalid initializer size: %d diff(%d): `%+k`", stktop, stktop - var->offs, ast);
							return TYPE_any;
						}
					}
					else if (size > 0) {		// initialize gloabal
						if (!emitvar(rt, var)) {
							trace("%+k", ast);
							return TYPE_any;
						}
						if (!emitint(rt, opc_sti, size)) {
							trace("%+k:sizeof(%-T) = %d", ast, var, size);
							return TYPE_any;
						}
					}
				}

				// alloc locally a block of the size of the type;
				else if (var->offs == 0) {
					logif(var->size != sizeOf(typ, 0), "size error: %T(%d, %d)", var, var->size, sizeOf(typ, 0));
					if (!emitint(rt, opc_spc, size)) {
						trace("%+k", ast);
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

			dieif(get != TYPE_vid, "Error");
			get = got = TYPE_vid;
		} break;

		case EMIT_opc:
			trace("%+k", ast);
			return TYPE_any;
		//#}
	}

	// generate cast
	if (get != got) switch (get) {

		case TYPE_vid:
			// FIXME: free stack.
			got = get;
			break;

		case TYPE_any: switch (got) {
			default:
				goto errorcast2;

			case TYPE_vid:
			case TYPE_ref:
				break;
		} break;

		case TYPE_bit: switch (got) {		// to boolean
			default:
				goto errorcast2;

			case TYPE_u32:
			case TYPE_i32:
				if (!emitopc(rt, i32_bol)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
			case TYPE_i64:
				if (!emitopc(rt, i64_bol)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
			case TYPE_f32:
				if (!emitopc(rt, f32_bol)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
			case TYPE_f64:
				if (!emitopc(rt, f64_bol)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
		} break;

		case TYPE_u32: switch (got) {
			default:
				goto errorcast2;

			case TYPE_bit:
			case TYPE_i32:
				break;

			case TYPE_i64: {
				if (!emitopc(rt, i64_i32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				//~ trace("cgen[%t->%t](%+k)", ret, get, ast);
				get = got = TYPE_int;
			} break;
			//~ case TYPE_f32: if (!emitopc(s, f32_i32)) return TYPE_any; break;
			//~ case TYPE_f64: if (!emitopc(s, f64_i32)) return TYPE_any; break;
		} break;

		case TYPE_i32: switch (got) {
			default:
				goto errorcast2;

			case TYPE_bit:
			case TYPE_u32:
				break;

			case TYPE_i64:
				if (!emitopc(rt, i64_i32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_f32:
				if (!emitopc(rt, f32_i32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_f64:
				if (!emitopc(rt, f64_i32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
		} break;

		case TYPE_i64: switch (got) {
			default:
				goto errorcast2;

			case TYPE_bit:
			case TYPE_u32:
				if (!emitopc(rt, u32_i64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_i32:
				if (!emitopc(rt, i32_i64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_f32:
				if (!emitopc(rt, f32_i64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_f64:
				if (!emitopc(rt, f64_i64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
		} break;

		case TYPE_f32: switch (got) {
			default:
				goto errorcast2;

			case TYPE_bit:
			//~ case TYPE_u32:
			case TYPE_i32:
				if (!emitopc(rt, i32_f32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_i64:
				if (!emitopc(rt, i64_f32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_f64:
				if (!emitopc(rt, f64_f32)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
		} break;

		case TYPE_f64: switch (got) {
			default:
				goto errorcast2;

			case TYPE_bit:
			//~ case TYPE_u32:
			case TYPE_i32:
				if (!emitopc(rt, i32_f64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_i64:
				if (!emitopc(rt, i64_f64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case TYPE_f32:
				if (!emitopc(rt, f32_f64)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
		} break;

		case TYPE_ref:
		case TYPE_arr: switch (got) {
			default:
				goto errorcast2;

			case EMIT_opc:
				return EMIT_opc;
		}

		case TYPE_ptr: switch (got) {
			default:
				goto errorcast2;

			case TYPE_ref:
				return TYPE_ptr;
		}

		default:
			fatal("unimplemented(cast for `%+k`, %t):%t (%s:%d)", ast, get, got, ast->file, ast->line);
			// fall to next case

		errorcast2:
			trace("cgen[%t->%t](%+k)", got, get, ast);
			return TYPE_any;
	}

	// zero extend ...
	if (get == TYPE_u32) {
		debug("zero extend[%t->%t]: %-T %+k (%s:%d)", got, get, ast->type, ast, ast->file, ast->line);
		switch (ast->type->size) {
			default:
				trace("Invalid cast(%t -> %t): %+k", got, get, ast);
				return TYPE_any;

			case 4:
				break;

			case 2:
				if (!emitint(rt, b32_bit, b32_bit_and | 16)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;

			case 1:
				if (!emitint(rt, b32_bit, b32_bit_and | 8)) {
					trace("%+k", ast);
					return TYPE_any;
				}
				break;
		}
	}

	#ifdef DEBUGGING
	logif(stmt_qual != 0, "unimplemented qualified statement `%+k`: %t", ast, stmt_qual);
	#endif

	return got;
}

int gencode(state rt, int mode) {
	ccState cc = rt->cc;
	size_t Lmain, Lmeta;

	// make global variables static ?
	int gstat = (mode & cgen_glob) == 0;

	dieif(rt->errc, "can not generate code");
	dieif(cc == NULL, "compiler state invalid");

	// leave the global scope.
	rt->init = ccAddType(rt, "<init>", 0, 0);
	rt->defs = leave(rt->cc, NULL, gstat);

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

			for (Ng = ng; Ng != NULL; Ng = Ng->gdef) {
				if (Ng->decl == ng) {
					break;
				}
				Pg = Ng;
			}

			//~ this must be generated before sym;
			if (Ng) {
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

	// used memeory by metadata (string constants and typeinfos)
	Lmeta = rt->_beg - rt->_mem;

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
		}

		rt->vm.libv = calls;
	}

	// debuginfo
	if (mode & cgen_info) {
		rt->dbg = (dbgState)(rt->_beg = paddptr(rt->_beg, rt_size));
		rt->_beg += sizeof(struct dbgStateRec);

		dieif(rt->_beg >= rt->_end, ERR_MEMORY_OVERRUN);
		memset(rt->dbg, 0, sizeof(struct dbgStateRec));

		initBuff(&rt->dbg->codeMap, 128, sizeof(struct dbgInfo));
	}

	//~ read only memory ends here.
	//~ strings, typeinfos, TODO(constants, functions, enums, ...)
	rt->vm.ro = rt->_beg - rt->_mem;

	// TODO: generate functions first
	rt->vm.opti = mode & cgen_opti;

	// static variables & functions
	if (rt->defs != NULL) {
		symn var;

		// we will append the list of declarations here.
		astn staticinitializers = newnode(cc, STMT_beg);

		// generate global and static variables & functions
		for (var = cc->gdef; var; var = var->gdef) {

			// exclude null
			if (var == rt->cc->null_ref)
				continue;

			// exclude aliases and typenames
			if (var->kind != TYPE_ref)
				continue;

			// exclude non static variables
			if (!var->stat)
				continue;

			dieif(var->offs != 0, "offset %06x for: %+T", var->offs, var);

			if (var->cnst && var->init == NULL) {
				error(cc->s, var->file, var->line, "uninitialized constant `%+T`", var);
			}

			if (var->call && var->init == NULL) {
				error(cc->s, var->file, var->line, "uninimplemented function `%+T`", var);
			}

			if (var->call && var->cast != TYPE_ref) {
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

				if (!cgen(rt, var->init, TYPE_vid)) {
					continue;
				}
				if (!emitopc(rt, opc_jmpi)) {
					error(rt, var->file, var->line, "error emiting function: %-T", var);
					continue;
				}
				while (cc->jmps) {
					error(rt, NULL, 0, "invalid jump: `%k`", cc->jmps);
					cc->jmps = cc->jmps->next;
				}
				var->size = emitopc(rt, markIP) - seg;
				var->stat = 1;
			}
			else {
				unsigned padd = rt_size;
				dieif(var->size <= 0, "Error %-T", var);	// instance of void ?
				dieif(var->offs != 0, "Error %-T", var);	// already generated ?

				// TODO: size of variable should be known here.
				dieif(var->size != sizeOf(var, 1), "size error: %-T: %d / %d", var, var->size, sizeOf(var, 1));
				var->size = sizeOf(var, 1);

				// align the memory of the variable. speeding up the read and write of it.
				if (var->size >= 16) {
					padd = 16;
				}
				else if (var->size >= 8) {
					padd = 8;
				}
				else if (var->size >= 4) {
					padd = 4;
				}
				else if (var->size >= 2) {
					padd = 2;
				}
				else if (var->size >= 1) {
					padd = 1;
				}
				else {
					fatal(ERR_INTERNAL_ERROR);
					return 0;
				}

				rt->_beg = paddptr(rt->_beg, padd);
				var->offs = vmOffset(rt, rt->_beg);
				rt->_beg += var->size;

				dieif(rt->_beg >= rt->_end, "Error");

				// TODO: recheck double initialization fix(var->nest > 0)
				if (var->init != NULL && var->nest > 0) {
					astn init = newnode(cc, TYPE_def);

					if (var->cnst && !isConst(var->init)) {
						warn(rt, 16, var->file, var->line, "non constant initialization of static variable `%-T`", var);
					}

					//~ make initialization from initializer
					init->type = var->type;
					init->file = var->file;
					init->line = var->line;
					init->cst2 = var->cast;
					init->ref.link = var;

					init = opnode(cc, STMT_do, NULL, init);
					init->type = cc->type_vid;
					init->file = var->file;
					init->line = var->line;

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
			dieif(cc->root == NULL || cc->root->kind != STMT_beg, "Error");
			staticinitializers->lst.tail->next = cc->root->stmt.stmt;
			cc->root->stmt.stmt = staticinitializers->lst.head;
			//~ staticinitializers->list.tail->next = cc->root->stmt.stmt;
			//~ cc->root = staticinitializers->list.head;
		}
	}

	Lmain = emitopc(rt, markIP);
	if (cc->root != NULL) {
		rt->vm.sm = rt->vm.ss = 0;

		// enable static var initialization
		rt->cc->init = 1;

		dieif(cc->root->kind != STMT_beg, "Error");

		// TYPE_vid clears the stack
		if (!cgen(rt, cc->root, gstat ? TYPE_vid : TYPE_any)) {
			trace("Error");
			return 0;
		}

		while (cc->jmps) {
			error(rt, NULL, 0, "invalid jump: `%k`", cc->jmps);
			cc->jmps = cc->jmps->next;
			trace("Error");
			return 0;
		}
	}

	// TODO: if the main function exists generate: exit(main());
	// application exit point: exit(0)
	// !needed when invoking functions inside vm.
	rt->vm.px = emitopc(rt, opc_ldz1);
	emitint(rt, opc_libc, 0);

	// program entry point
	rt->vm.pc = Lmain;

	dieif(rt->defs != cc->gdef, "globals are not the same with defs");

	// buils the initialization function.
	rt->init->file = NULL;
	rt->init->line = 0;
	rt->init->kind = TYPE_ref;
	rt->init->call = 1;
	rt->init->offs = Lmain;
	rt->init->size = emitopc(rt, markIP) - Lmain;
	rt->init->init = cc->root;

	rt->_end = rt->_mem + rt->_size;
	if (rt->dbg != NULL) {
		int i, j;
		struct arrBuffer *codeMap = &rt->dbg->codeMap;
		for (i = 0; i < codeMap->cnt; ++i) {
			for (j = i; j < codeMap->cnt; ++j) {
				dbgInfo a = getBuff(codeMap, i);
				dbgInfo b = getBuff(codeMap, j);
				if (a->end > b->end) {
					memswap(a, b, sizeof(struct dbgInfo));
				}
				else if (a->end == b->end) {
					if (a->start < b->start) {
						memswap(a, b, sizeof(struct dbgInfo));
					}
				}
			}
		}
	}

	return rt->errc == 0;
	(void)Lmeta;
}
