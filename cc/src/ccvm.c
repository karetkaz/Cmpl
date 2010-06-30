#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "ccvm.h"

const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, STR) {TYPE, SIZE, STR},
	#include "incl/defs.h"
	{0},
};
const opc_inf opc_tbl[255] = {
	//#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem)
	#define OPCDEF(Name, Code, Size, Chck, Diff, Time, Mnem)\
		{Code, Size, Chck, Diff, Mnem},
	#include "code.h"
	{0},
};

/*static int fixargs2(symn fun, int sp) {
	symn arg;
	for (arg = fun->args; arg; arg = arg->next) {
		sp += arg->type->size / 4;
	}
	for (arg = fun->args; arg; arg = arg->next) {
		arg->offs = -sp;
		sp -= arg->type->size / 4;

		switch (castTy(arg->type)) {
			default: fatal("FixMe");
			case TYPE_u32:
			case TYPE_i32:
			case TYPE_f32:
			case TYPE_i64:
			case TYPE_f64:
			//~ case TYPE_p4x:
				break;
		}
	}
	return sp;
}

void fixargs(symn fun, astn argn, int sp) {
	// in case sp == 0 inline args
	// in case argn == null revert
	symn args = fun->args;
	if (argn) while (args && argn) {
		args->offs = argn->XXXX;
		args = args->next;
		argn = argn->next;
	}
	else while (args) {
		args->kind = TYPE_ref;
		args->offs = 1;

		args = args->next;
	}
}// */

#define dbg_ast(__AST) debug("cgen(err, %+k)\n%3K", __AST, __AST)
#define dbg_arg(__AST) debug("cgen(arg, %+k)\n%3K", __AST, __AST)
#define dbg_rhs(__AST) debug("cgen(rhs, %+k)\n%3K", (__AST)->op.rhso, __AST)
#define dbg_lhs(__AST) debug("cgen(rhs, %+k)\n%3K", (__AST)->op.lhso, __AST)
#define internalerror error(s, ast->line, "internal error : %+k", ast)

static int push(state s, astn ast) {
	struct astn tmp;
	int cast;

	if (!ast) {
		fatal("null ast");
		return 0;
	}

	cast = ast->cst2;

	if (s->vm->opti && eval(&tmp, ast)) {
		ast = &tmp;
	}

	return cgen(s, ast, cast);
}
int cgen(state s, astn ast, int get) {
	#if DEBUGGING
	#if DEBUGGING > 2
	int asdbg = 0x119;
	#endif
	int ipdbg = emit(s->vm, get_ip);
	#endif
	struct astn tmp;
	int ret = 0;

	if (!ast) {
		//~ if (get == TYPE_vid)
			//~ return TYPE_vid;
		fatal("FixMe");
		return 0;
	}

	dieif(!ast->type, "FixMe %d: %t: %+k", ast->line, ast->kind, ast);

	if (get == 0)
		get = ast->cst2;
	ret = ast->type->cast;

	switch (ast->kind) {
		default:
			fatal("FixMe");
			return 0;

		//{ STMT
		case STMT_do: {	// expr statement or decl
			int stpos = emit(s->vm, get_sp);
			emit(s->vm, opc_line, ast->line);
			if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
				dbg_ast(ast->stmt.stmt);
				//~ dumpasmdbg(stderr, s->vm, ipdbg, 0x10);
				return 0;
			}
			if (stpos < emit(s->vm, get_sp))
				warn(s, 1, s->cc->file, ast->line, "statement underflows stack: %+k", ast->stmt.stmt);
			#if DEBUGGING > 2
				fputfmt(stdout, "%+k\n", ast);
				dumpasmdbg(stdout, s->vm, ipdbg, -1, asdbg);
			#endif
		} return TYPE_vid;
		case STMT_beg: {	// {}
			astn ptr;
			int stpos = emit(s->vm, get_sp);
			emitint(s->vm, opc_line, ast->line);
			for (ptr = ast->stmt.stmt; ptr; ptr = ptr->next) {
				if (!cgen(s, ptr, TYPE_vid)) {
					error(s, ptr->line, "emmiting statement %+k", ptr);
				}
			}
			//~ for (sym = ast->type; sym; sym = sym->next);	// destruct?
			if (get == TYPE_vid && stpos != emit(s->vm, get_sp)) {
				if (!emitidx(s->vm, opc_drop, stpos)) {
					internalerror;
					return 0;
				}
			}
		} return TYPE_vid;
		case STMT_if:  {
			int jmpt = 0, jmpf = 0;
			int tt = eval(&tmp, ast->stmt.test);

			dieif(get != TYPE_vid, "FixMe");
			emitint(s->vm, opc_line, ast->line);

			if (ast->cst2 == QUAL_sta && (ast->stmt.step || !tt)) {
				error(s, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "else part is invalid");
				return 0;
			}
			if (tt && (s->vm->opti || ast->cst2 == QUAL_sta)) {
				astn gen = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
				//~ debug("bool(%+k) = %k = %d", ast->stmt.test, &tmp, constbol(&tmp));
				if (gen && !cgen(s, gen, TYPE_any)) {	// leave the stack
					debug("%+k", gen);
					return 0;
				}
			}
			else if (ast->stmt.stmt && ast->stmt.step) {		// if then else
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				#if DEBUGGING > 2
					fputfmt(stdout, "%+k\n", ast);
					dumpasmdbg(stdout, s->vm, ipdbg, -1, asdbg);
				#endif
				if (!(jmpt = emit(s->vm, opc_jz))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				if (!(jmpf = emit(s->vm, opc_jmp))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpf, emit(s->vm, get_ip), 0);
				#if DEBUGGING > 2
					fputfmt(stdout, "// endif %+k\n", ast);
				#endif
			}
			else if (ast->stmt.stmt) {					// if then
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				//~ if false skip THEN block
				#if DEBUGGING > 2
					fputfmt(stdout, "%+k\n", ast);
					dumpasmdbg(stdout, s->vm, ipdbg, -1, asdbg);
				#endif
				if (!(jmpt = emit(s->vm, opc_jz))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
				#if DEBUGGING > 2
					fputfmt(stdout, "// endif %+k\n", ast);
				#endif
			}
			else if (ast->stmt.step) {					// if else
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				#if DEBUGGING > 2
					fputfmt(stdout, "%+k\n", ast);
					dumpasmdbg(stdout, s->vm, ipdbg, -1, asdbg);
				#endif
				//~ if true skip ELSE block
				if (!(jmpt = emit(s->vm, opc_jnz))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
				#if DEBUGGING > 2
					fputfmt(stdout, "// endif %+k\n", ast);
				#endif
			}
		} return TYPE_vid;
		case STMT_for: {
			int beg, end, cmp = -1;
			int stpos = emit(s->vm, get_sp);

			emitint(s->vm, opc_line, ast->line);
			if (!cgen(s, ast->stmt.init, TYPE_vid)) {
				debug("%+k", ast);
				return 0;
			}

			//~ if (ast->cast == QUAL_par) ;		// 'parallel for'
			//~ else if (ast->cast == QUAL_sta) ;	// 'static for'
			beg = emit(s->vm, get_ip);
			if (ast->stmt.step) {
				int tmp = beg;
				if (ast->stmt.init)
					emit(s->vm, opc_jmp, 0);

				if (!(beg = emit(s->vm, get_ip))) {
					fatal("FixMe");
					return 0;
				}
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}

				if (ast->stmt.init)
					fixjump(s->vm, tmp, emit(s->vm, get_ip), 0);
			}
			if (ast->stmt.test) {
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				cmp = emit(s->vm, opc_jz, 0);		// if (!test) break;
			}

			// push(list_jmp, 0);
			if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {		// this will leave the stack clean
				debug("%+k", ast);
				return 0;
			}
			if (!(end = emit(s->vm, opc_jmp, beg))) {		// continue;
				debug("%+k", ast);
				internalerror;
				return 0;
			}
			fixjump(s->vm, end, beg, 0);
			end = emit(s->vm, get_ip);
			fixjump(s->vm, cmp, end, 0);			// break;

			//~ while (list_jmp) {
			//~ if (list_jmp->kind == break)
			//~ 	fixj(s, list_jmp->offs, end, 0);
			//~ if (list_jmp->kind == continue)
			//~ 	fixj(s, list_jmp->offs, beg, 0);
			//~ list_jmp = list_jmp->next;
			//~ }
			// pop(list_jmp);


			//~ TODO: if (init is decl) destruct;
			if (stpos != emit(s->vm, get_sp))
				dieif(!emitidx(s->vm, opc_drop, stpos), "FixMe");
		} return TYPE_vid;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/call/cast
			//~ int stargs = emit(s->vm, get_sp);
			astn argv = ast->op.rhso;
			symn var = linkOf(ast, 0);	// refs only
			//~ symn typ = ast->type;

			// TODO: dieif(!var || var->kind != TYPE_def, "FixMe");

			//~ debug("call %+k", ast);
			if (var && var->kind == TYPE_def) {
				symn as = var->args;
				// also no matter in case of varargs
				if (argv) {
					astn an = NULL;

					// this is done by lookup... but...
					while (argv->kind == OPER_com) {
						astn arg = argv->op.rhso;
						argv = argv->op.lhso;

						arg->next = an;
						an = arg;
					}
					argv->next = an;
					an = argv;

					// this should be more complicated
					// TODO: inline arg only if TYPE_(ref, int, flt OR str)
					while (an && as) {
						//~ symn aty = as->type;
						as->kind = TYPE_def;
						as->init = an;
						an = an->next;
						as = as->next;
					}
				}
				//~ debug("inline %+k = %+k", ast, var->init);
				if (!(ret = cgen(s, var->init, ret))) {
					dbg_arg(var->init);
				}
				for (as = var->args; as; as = as->next) {
					as->kind = TYPE_ref;
					as->init = NULL;
				}
				if (get == TYPE_ref) {
					debug("cast for `%+k`, %t: %t", ast, get, ret);
					//~ return get;
				}
				//~ debug("call %+k", var->init);
				//~ if (!ret) ret = get;
				//~ if (ret == get) return ret;
				break;
			}

			// push args
			if (argv) {
				astn argl = NULL;
				while (argv->kind == OPER_com) {
					astn arg = argv->op.rhso;
					if (!push(s, arg)) {
						dbg_arg(arg);
						return 0;
					}
					//~ arg->XXXX = emit(s->vm, get_sp);
					arg->next = argl;
					argl = arg;
					argv = argv->op.lhso;
				}
				if (var == emit_opc && istype(argv))
					return castTy(argv->type);
				else if (!push(s, argv)) {
					dbg_arg(argv);
					return 0;
				}
				argv->next = argl;
				argl = argv;
			}

			if (var) {					// call()
				if (var == emit_opc) {	// emit()
				}
				else if (var->call) {
					/*int spbc = emit(s->vm, get_sp);
					if (var->offs == 0) {			// inline or error
						if (!var->init || var->init->kind != OPER_fnc) {
							error(s, ast->line, "invalid function call: %+T", var);
							if (var->file && var->line)
								info(s, var->file, var->line, "defined here");
							return 0;
						}
						fixargs2(var, -stargs);

						if (!cgen(s, var->init, ret)) {
							debug("%+k:%+T", var->init, ast->type);
							return 0;
						}
						if (spbc <= emit(s->vm, get_sp)) {
							error(s, ast->line, "invalid stack size %+k", ast);
							//~ dumpasmdbg(stderr, s->vm, ipdbg, 0x10);
							return 0;
						}
						// TODO: cgen.call: fix this, result should be a reference
						switch (ret) {
							default: fatal("FixMe");
							case TYPE_u32:
							case TYPE_i32:
							case TYPE_f32: if (!emitidx(s->vm, opc_set1, stargs -= 1)) return 0; break;
							case TYPE_i64:
							case TYPE_f64: if (!emitidx(s->vm, opc_set2, stargs -= 2)) return 0; break;
							case TYPE_p4x: if (!emitidx(s->vm, opc_set4, stargs -= 4)) return 0; break;
						}
						if (!emitidx(s->vm, opc_drop, stargs)) {
							internalerror;
							return 0;
						}
					}
					else {
						fatal("FixMe");
						/ * TODO: implement function calls
						if (!emit(s->vm, opc_call, var->offs)) {
							debug("%+k", ast);
							internalerror;
							return 0;
						}// * /
					}*/
					fatal("FixMe");
				}
				else {
					error(s, ast->line, "called object is not a function: %+T", var);
					//~ but can have an operator: call() with this args
					return 0;
				}
			}
			else {						// cast()
				//~ debug("cast(%+k):%t", ast, ast->cst2);
				if (!argv || argv != ast->op.rhso /* && cast*/)
					warn(s, 1, s->cc->file, ast->line, "multiple values, array ?: '%+k'", ast);
			}
		} break;
		case OPER_idx: {	// '[]'
			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				dbg_lhs(ast);
				return 0;
			}

			if (s->vm->opti && eval(&tmp, ast->op.rhso)) {
				int offs = sizeOf(ast->type) * constint(&tmp);
				if (constint(&tmp) >= ast->op.lhso->type->size) {
					error(s, ast->line, " indes out of bounds: %+k", ast);
				}
				if (!emit(s->vm, opc_inc, offs)) {
					if (!emit(s->vm, opc_ldc4, offs)) {
						dbg_ast(ast);
						internalerror;
						return 0;
					}
					if (!emit(s->vm, i32_add)) {
						dbg_ast(ast);
						internalerror;
						return 0;
					}
				}
			}
			else {
				if (!cgen(s, ast->op.rhso, TYPE_u32)) {
					dbg_rhs(ast);
					return 0;
				}
				if (!emit(s->vm, opc_ldc4, sizeOf(ast->type))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
				if (!emit(s->vm, u32_mad)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emit(s->vm, opc_ldi, sizeOf(ast->type))) {
				dbg_ast(ast);
				internalerror;
				return 0;
			}
		} break; // */
		case OPER_dot: {	// '.'
			// TODO: this should work as indexing
			//~ symn var = linkOf(ast->op.rhso, 0);
			symn var = ast->op.rhso->id.link;

			if (istype(ast->op.lhso)) {
				return cgen(s, ast->op.rhso, get);
			} // */

			if (!var) {
				//~ TODO: review
				debug("%T", var);
				dbg_rhs(ast);
				internalerror;
				return 0;
			}// */

			if (var->kind == TYPE_def/* || var->stat*/) {
				debug("%T", var);
				return cgen(s, ast->op.rhso, get);
			}

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				dbg_lhs(ast);
				return 0;
			}

			if (!emit(s->vm, opc_inc, var->offs)) {
				if (!emit(s->vm, opc_ldc4, var->offs)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
				if (!emit(s->vm, i32_add)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emit(s->vm, opc_ldi, sizeOf(ast->type))) {
				dbg_ast(ast);
				internalerror;
				return 0;
			}
		} break;

		case OPER_not:		// '!'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			int opc = -1;
			switch (ast->kind) {
				default: fatal("FixMe"); return 0;
				case OPER_pls: return cgen(s, ast->op.rhso, get);
				case OPER_mns: opc = opc_neg; break;
				case OPER_not: opc = opc_not; break;
				case OPER_cmt: opc = opc_cmt; break;
			}
			if (!cgen(s, ast->op.rhso, get)) {
				dbg_rhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc, get)) {
				internalerror;
				dbg_ast(ast);
				return 0;
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
			int opc = -1;
			switch (ast->kind) {
				default: fatal("FixMe");
				case OPER_add: opc = opc_add; break;
				case OPER_sub: opc = opc_sub; break;
				case OPER_mul: opc = opc_mul; break;
				case OPER_div: opc = opc_div; break;
				case OPER_mod: opc = opc_mod; break;

				case OPER_neq: opc = opc_cne; break;
				case OPER_equ: opc = opc_ceq; break;
				case OPER_geq: opc = opc_cge; break;
				case OPER_lte: opc = opc_clt; break;
				case OPER_leq: opc = opc_cle; break;
				case OPER_gte: opc = opc_cgt; break;

				case OPER_shl: opc = opc_shl; break;
				case OPER_shr: opc = opc_shr; break;
				case OPER_and: opc = opc_and; break;
				case OPER_ior: opc = opc_ior; break;
				case OPER_xor: opc = opc_xor; break;
			}
			if (!cgen(s, ast->op.lhso, 0)) {
				dbg_lhs(ast);
				return 0;
			}
			if (!cgen(s, ast->op.rhso, 0)) {
				dbg_rhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc, ast->op.rhso->cst2)) {
				dbg_ast(ast);
				internalerror;
				return 0;
			}

			#if DEBUGGING
			// these things must be this way
			dieif(ast->op.lhso->cst2 != ast->op.rhso->cst2, "RemMe");
			dieif(ret != castTy(ast->type), "RemMe");
			switch (ast->kind) {
				case OPER_neq:
				case OPER_equ:
				case OPER_geq:
				case OPER_lte:
				case OPER_leq:
				case OPER_gte:
					dieif(ret != TYPE_bit, "RemMe");
					break;
			}
			#endif
		} break;

		//~ case OPER_lnd:		// '&&'
		//~ case OPER_lor:		// '||'
		case OPER_sel: {		// '?:'
			int jmpt, jmpf;
			if (s->vm->opti && eval(&tmp, ast->op.test)) {
				// TODO: old: ret = cgen(s, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, get);
				if (!cgen(s, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, ret)) {
					dbg_ast(ast);
					return 0;
				}
			}
			else {
				int stpos = emit(s->vm, get_sp);
				if (!cgen(s, ast->op.test, TYPE_bit)) {
					dbg_ast(ast);
					return 0;
				}
				if (!(jmpt = emit(s->vm, opc_jz))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}

				if (!cgen(s, ast->op.lhso, 0)) {
					dbg_ast(ast);
					return 0;
				}
				if (!(jmpf = emit(s->vm, opc_jmp))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);

				if (!emitint(s->vm, set_sp, -stpos)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->op.rhso, 0)) {
					dbg_ast(ast);
					return 0;
				}
				fixjump(s->vm, jmpf, emit(s->vm, get_ip), 0);
			}
		} break;

		case ASGN_set: {		// ':='
			// TODO: lhs should be evaluated first ??? or not

			if (!cgen(s, ast->op.rhso, ret)) {
				dbg_rhs(ast);
				return 0;
			}
			if (get != TYPE_vid) {
				// in case a = b = sum(2, 700);
				// dupplicate the result
				if (!emitint(s->vm, opc_ldsp, 0)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
				if (!emit(s->vm, opc_ldi, sizeOf(ast->type))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
			}
			else
				ret = get;

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				dbg_lhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc_sti, sizeOf(ast->type))) {
				dbg_ast(ast);
				internalerror;
				return 0;
			}
		} break;
		//}
		//{ TVAL
		case TYPE_bit:
		case TYPE_int: switch (get) {
			//~ TODO: check overflows
			case TYPE_vid: return TYPE_vid;
			case TYPE_bit: return emiti32(s->vm, constbol(ast)) ? TYPE_u32 : 0;
			case TYPE_u32: return emiti32(s->vm, ast->con.cint) ? TYPE_u32 : 0;
			case TYPE_i32: return emiti32(s->vm, ast->con.cint) ? TYPE_i32 : 0;
			case TYPE_i64: return emiti64(s->vm, ast->con.cint) ? TYPE_i64 : 0;
			case TYPE_f32: return emitf32(s->vm, ast->con.cint) ? TYPE_f32 : 0;
			case TYPE_f64: return emitf64(s->vm, ast->con.cint) ? TYPE_f64 : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast);
		} return 0;
		case TYPE_flt: switch (get) {
			//~ TODO: check overflows
			case TYPE_vid: return TYPE_vid;
			case TYPE_bit: return emiti32(s->vm, constbol(ast)) ? TYPE_u32 : 0;
			case TYPE_u32: return emiti32(s->vm, ast->con.cflt) ? TYPE_u32 : 0;
			case TYPE_i32: return emiti32(s->vm, ast->con.cflt) ? TYPE_i32 : 0;
			case TYPE_i64: return emiti64(s->vm, ast->con.cflt) ? TYPE_i64 : 0;
			case TYPE_f32: return emitf32(s->vm, ast->con.cflt) ? TYPE_f32 : 0;
			case TYPE_f64: return emitf64(s->vm, ast->con.cflt) ? TYPE_f64 : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast);
		} return 0;
		case TYPE_str: switch (get) {
			case TYPE_vid: return TYPE_vid;
			case TYPE_ref: return emitptr(s->vm, opc_ldcr, ast->id.name) ? TYPE_ref : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast);
		} return 0;
		//~ case CNST_str: return 0;	// unimpl yet(ref)

		case TYPE_ref: {					// use (var, func, define)
			//~ symn typ = ast->type;			// type
			symn var = ast->id.link;		// link
			dieif(!var, "FixMe");

			switch (var->kind) {
				case TYPE_ref:
					if (!emitref(s->vm, opc_ldcr, var->offs)) {
						dbg_ast(ast);
						return 0;
					}
					if (get != TYPE_ref) {				// value needed
						// TODO: (Sign-Zero) extend
						if (!emit(s->vm, opc_ldi, sizeOf(ast->type))) {
							dbg_ast(ast);
							return 0;
						}
					}
					else	// TODO: why?
						ret = TYPE_ref;
					break;
				case EMIT_opc:
					dieif(get == TYPE_ref, "FixMe");
					if (!emitint(s->vm, var->offs, var->init ? constint(var->init) : 0)) {
						error(s, ast->line, "error emiting opcode: %+k", ast);
						if (emit(s->vm, get_sp) > 0)
							info(s, s->cc->file, ast->line, "%+k underflows stack.", ast);
						return 0;
					}
					return TYPE_vid;

				case TYPE_def:
					// TODO: this sucks
					if (var->init)
						return cgen(s, var->init, get);
				default:
					error(s, ast->line, "invalid rvalue `%+k`", ast);
					return 0;
			}
		} break;
		case TYPE_def: {					// new (var, func, define)
			symn var = ast->id.link;
			symn typ = ast->type;
			int stktop = emit(s->vm, get_sp);


			dieif(!var, "FixMe");
			dieif(typ != var->type, "FixMe(%T / %T)", typ, var->type);	//

			if (var->kind == TYPE_ref) {

				if (var->call)		// TODO: this should be in an other pass or not
					return TYPE_vid;

				if (var->init) {
					astn val = var->init;

					if (val->type == emit_opc) {
						ret = get;//var->type->cast;
						//~ ret = castTy(typ);//get;// = 0;
					}
					else if (val->type != typ) {
					//~ else if (val->type != var->type) {
						//~ TODO: this typecheck should be fixed elsewere
						error(s, ast->line, "incompatible types in initialization(%-T := %-T)", typ, val->type);
					}

					if (s->vm->opti && eval(&tmp, val)) {
						val = &tmp;
					}// * /

					//~ debug("cast(`%+k`, %t):%t", ast, get, ret);
					
					if (!cgen(s, val, ret)) {
						dbg_arg(val);
						return 0;
					}
					var->offs = emit(s->vm, get_sp);
					if (4 * (stktop - var->offs) != sizeOf(typ) && val->type != emit_opc) {
						error(s, ast->line, "invalid initializer size: %d <> %d",stktop - var->offs, sizeOf(typ));
					}
				}
				else {		// static alloc a block of the size of the type;
					int size = sizeOf(typ);
					if (!emitint(s->vm, opc_spc, size)) {
						dbg_arg(ast);
						return 0;
					}
					var->offs = emit(s->vm, get_sp);
					if (-4 * var->offs < size) {
						error(s, ast->line, "stack underflow", -4 * var->offs, size);
						#if DEBUGGING
						dumpasmdbg(stderr, s->vm, ipdbg, -1, 0x10);
						#endif
						return 0;
					}
					//~ return TYPE_vid;
				}
				//~ break;
				return get;
			}// */

			dieif(get != TYPE_vid, "FixMe");
			return get;
			// else [enum, struct, define]
		} break;

		//~ case TYPE_rec:
		//~ case TYPE_enu: break;
		//}
	}

	if (get != ret) switch (get) {
		case TYPE_vid: return TYPE_vid;
		case TYPE_u32: switch (ret) {
			case TYPE_bit:
			case TYPE_i32: break;
			case TYPE_i64: if (!emit(s->vm, i64_i32)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_i32)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_i32)) return 0; break;
			//~ case TYPE_ref: if (!emit(s->vm, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i32: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64: if (!emit(s->vm, i64_i32)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_i32)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_i32)) return 0; break;
			//~ case TYPE_ref: if (!emit(s->vm, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i64: switch (ret) {
			case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: if (!emit(s->vm, i32_i64)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_i64)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_i64)) return 0; break;
			//~ case TYPE_ref: if (!emit(s->vm, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f32: switch (ret) {
			case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: if (!emit(s->vm, i32_f32)) return 0; break;
			case TYPE_i64: if (!emit(s->vm, i64_f32)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_f32)) return 0; break;
			//~ case TYPE_ref: if (!emit(s->vm, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f64: switch (ret) {
			case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: if (!emit(s->vm, i32_f64)) return 0; break;
			case TYPE_i64: if (!emit(s->vm, i64_f64)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_f64)) return 0; break;
			//~ case TYPE_ref: if (!emit(s->vm, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32: /*emit(s, i32_bol);*/ break;
			case TYPE_i32: /*emit(s, i32_bol);*/ break;
			case TYPE_i64: if (!emit(s->vm, i64_bol)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_bol)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_bol)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_ref:
			error(s, ast->line, "invalid rvalue: %+k", ast);
			return 0;
		default:
			debug("invalid cast: [%d->%d](%t to %t) %k: '%+k'", ret, get, ret, get, ast, ast);
			dbg_ast(ast);
			fatal("unimplemented(cast for `%+k`, %t):%t", ast, get, ret);
		errorcast2:
			debug("invalid cast: [%d->%d](%t to %t) %k: '%+k'", ret, get, ret, get, ast, ast);
			return 0;
	}

	return ret;
}

int logfile(state s, char* file) {
	if (s->logf == NULL);
	else if (s->logf == stderr);
	else if (s->logf == stdout);
	else fclose(s->logf);
	s->logf = 0;
	if (file) {
		s->logf = fopen(file, "wb");
		if (!s->logf) return -1;
	}
	return 0;
}
int srcfile(state s, char* file) {
	if (!ccOpen(s, srcFile, file))
		return -1;
	return 0;
}
int compile(state s, int level) {
	if (s->cc == NULL)
		return -1;

	s->cc->warn = level;
	if (scan(s->cc, -1) != 0) {
		return -2;
	}

	return ccDone(s);
}
int gencode(state s, int level) {
	ccEnv cc = s->cc;
	vmEnv vm = s->vm;

	if (s->errc)
		return -2;

	if (!s->cc || s->vm) {
		fatal("invalid enviroment: cc:%x, vm:%x, er:%d", cc, vm, s->errc);
		return -2;
	}

	if (!(vm = vmInit(s)))
		return -1;

	s->vm->opti = level;
	/* TODOS
	// emit global data first
	for (sym = cc->all; sym; sym = sym->defs) {
		if (sym->kind == TYPE_ref && !sym->call) {
			sym->offs = vm->ds;
			vm->ds += sizeOf(sym->type);
			//~ mem[code->ds] = sym->init;
		}
	}
	// emit global functions
	for (sym = cc->defs; sym; sym = sym->next) {
		if (sym->kind == TYPE_ref && sym->call) {
			debug("(%T)", sym);
			//~ cgen(s, sym->init, TYPE_vid);
			continue;
		}
	}
	// */

	//~ header:
	//~ seg:Data ro: initialized, meta, strings.
	//~ seg:Code rx

	//~ emit(s->vm, loc_data, 256 * 4);
	emit(s->vm, seg_code);
	emit(s->vm, opc_nop);
	if (s->cc->root)
		cgen(s, cc->root, 0);			// TODO: TYPE_vid: to clear the stack
	emit(s->vm, opc_libc, 0);
	emit(s->vm, seg_code);
	//~ emit debug symbols

	return s->errc;
}
//~ int execute(state s, int stack) ;

static void install_bits(ccEnv cc, symn it, int64_t sgn) {
	int size = it->size;
	int bits = 8 * size;
	int64_t mask = (-1LLU >> -bits);
	int64_t minv = sgn << (bits - 1);
	int64_t maxv = (minv - 1) & mask;
	enter(cc, it);
	installex(cc, "min",  TYPE_def, 0, type_i64, intnode(cc, minv));
	installex(cc, "max",  TYPE_def, 0, type_i64, intnode(cc, maxv));
	installex(cc, "mask", TYPE_def, 0, type_i64, intnode(cc, mask));
	installex(cc, "bits", TYPE_def, 0, type_i64, intnode(cc, bits));
	installex(cc, "size", TYPE_def, 0, type_i64, intnode(cc, size));
	//~ installex(cc, "nop", EMIT_opc, opc_nop, type_vid, NULL);
	it->args = leave(cc, it);
}//~ */

symn emit_opc = NULL;
static void install_emit(ccEnv cc, unsigned level) {
	symn typ;

	//~ emit_opc->call = 1;
	//~ emit_opc->type = 0;

	if (!level || !emit_opc)
		return;

	enter(cc, NULL);

	installex(cc, "nop", EMIT_opc, opc_nop, type_vid, NULL);
	if ((typ = install(cc, "u32", TYPE_int, TYPE_u32, 4))) {
		enter(cc, typ);
		installex(cc, "cmt", EMIT_opc, b32_cmt, type_u32, NULL);
		//~ installex(cc, "adc", EMIT_opc, b32_adc, type_u32, NULL);
		//~ installex(cc, "sub", EMIT_opc, b32_sbb, type_u32, NULL);
		installex(cc, "mul", EMIT_opc, u32_mul, type_u32, NULL);
		installex(cc, "div", EMIT_opc, u32_div, type_u32, NULL);
		installex(cc, "mod", EMIT_opc, u32_mod, type_u32, NULL);
		installex(cc, "mad", EMIT_opc, u32_mad, type_u32, NULL);
		installex(cc, "clt", EMIT_opc, u32_clt, type_bol, NULL);
		installex(cc, "cgt", EMIT_opc, u32_cgt, type_bol, NULL);
		installex(cc, "and", EMIT_opc, b32_and, type_u32, NULL);
		installex(cc,  "or", EMIT_opc, b32_ior, type_u32, NULL);
		installex(cc, "xor", EMIT_opc, b32_xor, type_u32, NULL);
		installex(cc, "shl", EMIT_opc, b32_shl, type_u32, NULL);
		installex(cc, "shr", EMIT_opc, b32_shr, type_u32, NULL);
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "i32", TYPE_int, TYPE_i32, 4))) {
		enter(cc, typ);
		installex(cc, "cmt", EMIT_opc, b32_cmt, type_i32, NULL);
		installex(cc, "neg", EMIT_opc, i32_neg, type_i32, NULL);
		installex(cc, "add", EMIT_opc, i32_add, type_i32, NULL);
		installex(cc, "sub", EMIT_opc, i32_sub, type_i32, NULL);
		installex(cc, "mul", EMIT_opc, i32_mul, type_i32, NULL);
		installex(cc, "div", EMIT_opc, i32_div, type_i32, NULL);
		installex(cc, "mod", EMIT_opc, i32_mod, type_i32, NULL);

		installex(cc, "ceq", EMIT_opc, i32_ceq, type_bol, NULL);
		installex(cc, "clt", EMIT_opc, i32_clt, type_bol, NULL);
		installex(cc, "cgt", EMIT_opc, i32_cgt, type_bol, NULL);

		installex(cc, "and", EMIT_opc, b32_and, type_i32, NULL);
		installex(cc,  "or", EMIT_opc, b32_ior, type_i32, NULL);
		installex(cc, "xor", EMIT_opc, b32_xor, type_i32, NULL);
		installex(cc, "shl", EMIT_opc, b32_shl, type_i32, NULL);
		installex(cc, "shr", EMIT_opc, b32_sar, type_i32, NULL);
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "i64", TYPE_int, TYPE_i64, 8))) {
		enter(cc, typ);
		installex(cc, "neg", EMIT_opc, i64_neg, type_i64, NULL);
		installex(cc, "add", EMIT_opc, i64_add, type_i64, NULL);
		installex(cc, "sub", EMIT_opc, i64_sub, type_i64, NULL);
		installex(cc, "mul", EMIT_opc, i64_mul, type_i64, NULL);
		installex(cc, "div", EMIT_opc, i64_div, type_i64, NULL);
		installex(cc, "mod", EMIT_opc, i64_mod, type_i64, NULL);
		installex(cc, "ceq", EMIT_opc, i64_ceq, type_bol, NULL);
		installex(cc, "clt", EMIT_opc, i64_clt, type_bol, NULL);
		installex(cc, "cgt", EMIT_opc, i64_cgt, type_bol, NULL);
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "f32", TYPE_flt, TYPE_f32, 4))) {
		enter(cc, typ);
		installex(cc, "neg", EMIT_opc, f32_neg, type_f32, NULL);
		installex(cc, "add", EMIT_opc, f32_add, type_f32, NULL);
		installex(cc, "sub", EMIT_opc, f32_sub, type_f32, NULL);
		installex(cc, "mul", EMIT_opc, f32_mul, type_f32, NULL);
		installex(cc, "div", EMIT_opc, f32_div, type_f32, NULL);
		installex(cc, "mod", EMIT_opc, f32_mod, type_f32, NULL);
		installex(cc, "ceq", EMIT_opc, f32_ceq, type_bol, NULL);
		installex(cc, "clt", EMIT_opc, f32_clt, type_bol, NULL);
		installex(cc, "cgt", EMIT_opc, f32_cgt, type_bol, NULL);
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "f64", TYPE_flt, TYPE_f64, 8))) {
		enter(cc, typ);
		installex(cc, "neg", EMIT_opc, f64_neg, type_f64, NULL);
		installex(cc, "add", EMIT_opc, f64_add, type_f64, NULL);
		installex(cc, "sub", EMIT_opc, f64_sub, type_f64, NULL);
		installex(cc, "mul", EMIT_opc, f64_mul, type_f64, NULL);
		installex(cc, "div", EMIT_opc, f64_div, type_f64, NULL);
		installex(cc, "mod", EMIT_opc, f64_mod, type_f64, NULL);
		installex(cc, "ceq", EMIT_opc, f64_ceq, type_bol, NULL);
		installex(cc, "clt", EMIT_opc, f64_clt, type_bol, NULL);
		installex(cc, "cgt", EMIT_opc, f64_cgt, type_bol, NULL);
		typ->args = leave(cc, typ);
		//~ typ->call = 1;
	}

	if ((typ = install(cc, "v4f", TYPE_enu, 0, 16))) {
		enter(cc, typ);
		installex(cc, "neg", EMIT_opc, v4f_neg, type_v4f, NULL);
		installex(cc, "add", EMIT_opc, v4f_add, type_v4f, NULL);
		installex(cc, "sub", EMIT_opc, v4f_sub, type_v4f, NULL);
		installex(cc, "mul", EMIT_opc, v4f_mul, type_v4f, NULL);
		installex(cc, "div", EMIT_opc, v4f_div, type_v4f, NULL);
		//~ installex(cc, "ceq", EMIT_opc, v4f_ceq, type_bol, NULL);
		installex(cc, "min", EMIT_opc, v4f_min, type_v4f, NULL);
		installex(cc, "max", EMIT_opc, v4f_max, type_v4f, NULL);
		installex(cc, "dp3", EMIT_opc, v4f_dp3, type_f32, NULL);
		installex(cc, "dp4", EMIT_opc, v4f_dp4, type_f32, NULL);
		installex(cc, "dph", EMIT_opc, v4f_dph, type_f32, NULL);
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "v2d", TYPE_enu, 0, 16))) {
		enter(cc, typ);
		installex(cc, "neg", EMIT_opc, v2d_neg, type_v2d, NULL);
		installex(cc, "add", EMIT_opc, v2d_add, type_v2d, NULL);
		installex(cc, "sub", EMIT_opc, v2d_sub, type_v2d, NULL);
		installex(cc, "mul", EMIT_opc, v2d_mul, type_v2d, NULL);
		installex(cc, "div", EMIT_opc, v2d_div, type_v2d, NULL);
		installex(cc, "min", EMIT_opc, v2d_min, type_v4f, NULL);
		installex(cc, "max", EMIT_opc, v2d_max, type_v4f, NULL);
		typ->args = leave(cc, typ);
	}

	if (level > 1) {
		int i;
		struct {
			char *name;
			astn node;
			//~ char *swz, *msk;	// swizzle and mask
		} swz[256];
		for (i = 0; i < 256; i += 1) {
			dieif(cc->_cnt < 5, "memory overrun");
			cc->_ptr[0] = "xyzw"[i>>0&3];
			cc->_ptr[1] = "xyzw"[i>>2&3];
			cc->_ptr[2] = "xyzw"[i>>4&3];
			cc->_ptr[3] = "xyzw"[i>>6&3];
			cc->_ptr[4] = 0;

			swz[i].name = mapstr(cc, cc->_ptr, -1, -1);
			swz[i].node = intnode(cc, i);
		}

		enter(cc, typ = install(cc, "swz", TYPE_enu, 0, 0));
		for (i = 0; i < 256; i += 1)
			installex(cc, swz[i].name, EMIT_opc, p4d_swz, type_v4f, swz[i].node);
		typ->args = leave(cc, typ);

		//~ extended set(masked) and dup(swizzle): p4d.dup.xyxy / p4d.set.xyz0
		enter(cc, typ = install(cc, "dup", TYPE_enu, 0, 0));
		for (i = 0; i < 256; i += 1)
			installex(cc, swz[i].name, EMIT_opc, 0x1d, type_v4f, swz[i].node);
		typ->args = leave(cc, typ);

		enter(cc, typ = install(cc, "set", TYPE_enu, 0, 0));
		installex(cc,    "x", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,    "y", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,    "z", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,    "w", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,  "xyz", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyzw", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyz0", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyz1", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyz_", EMIT_opc, 0x1e, type_v4f, intnode(cc, 0));
		typ->args = leave(cc, typ);
	}
	emit_opc->args = leave(cc, emit_opc);
}

static void install_math(ccEnv cc, unsigned level) {
	symn def = install(cc, "math", TYPE_enu, 0, 0);

	if (def == NULL)
		return;

	enter(cc, def);
	installex(cc,  "nan", TYPE_def, 0, type_f64, fh8node(cc, 0x7FFFFFFFFFFFFFFFLL));	// Qnan
	installex(cc, "Snan", TYPE_def, 0, type_f64, fh8node(cc, 0xfff8000000000000LL));	// Snan
	installex(cc,  "inf", TYPE_def, 0, type_f64, fh8node(cc, 0x7ff0000000000000LL));
	installex(cc,  "l2e", TYPE_def, 0, type_f64, fh8node(cc, 0x3FF71547652B82FELL));	// log_2(e)
	installex(cc,  "l2t", TYPE_def, 0, type_f64, fh8node(cc, 0x400A934F0979A371LL));	// log_2(10)
	installex(cc,  "lg2", TYPE_def, 0, type_f64, fh8node(cc, 0x3FD34413509F79FFLL));	// log_10(2)
	installex(cc,  "ln2", TYPE_def, 0, type_f64, fh8node(cc, 0x3FE62E42FEFA39EFLL));	// log_e(2)
	installex(cc,   "pi", TYPE_def, 0, type_f64, fh8node(cc, 0x400921fb54442d18LL));		// 3.1415...
	installex(cc,    "e", TYPE_def, 0, type_f64, fh8node(cc, 0x4005bf0a8b145769LL));		// 2.7182...
	if (level >= 1) {
		//~TODO: install(cc, "define isNan(flt64 x) = (x != x);", -1, 0, 0);
		//~TODO: install(cc, "define isNan(flt32 x) = (x != x);", -1, 0, 0);
	}
	def->args = leave(cc, def);
}

state gsInit(void* mem, unsigned size) {
	state s = mem;
	s->_cnt = size - sizeof(struct state);
	s->_ptr = s->_mem;
	//~ s->logf = stderr;
	s->cc = NULL;
	s->vm = NULL;
	return s;
}
ccEnv ccInit(state s) {
	ccEnv cc = getmem(s, sizeof(struct ccEnv), -1);
	//~ symn def;//, type_chr = 0;
	symn type_i08 = 0, type_i16 = 0;
	symn type_u08 = 0, type_u16 = 0;
	//~ symn type_u64 = 0, type_f16 = 0;

	if (cc == NULL)
		return NULL;

	s->cc = cc;
	cc->s = s;

	s->errc = 0;
	s->logf = stderr;

	//~ s->warn = wl;
	//~ s->opti = ol;

	cc->file = 0;
	cc->line = 0;
	cc->nest = 0;

	cc->fin._fin = -1;
	cc->fin._ptr = 0;
	cc->fin._cnt = 0;

	cc->root = 0;
	cc->defs = 0;

	cc->_chr = -1;
	cc->_tok = 0;

	cc->tokp = 0;
	cc->all = 0;

	cc->_ptr = s->_ptr;
	cc->_cnt = s->_cnt;

	//{ install Type

	type_vid = install(cc,  "void", TYPE_vid | symn_read, TYPE_vid, 0);

	type_bol = install(cc,  "bool", TYPE_bit | symn_read, TYPE_bit, 1);

	type_u08 = install(cc,  "uns8", TYPE_int | symn_read, TYPE_u32, 1);
	type_u16 = install(cc, "uns16", TYPE_int | symn_read, TYPE_u32, 2);
	type_u32 = install(cc, "uns32", TYPE_int | symn_read, TYPE_u32, 4);
	//~ type_u64 = install(cc, "uns64", TYPE_int | symn_read, TYPE_i64, 8);

	type_i08 = install(cc,  "int8", TYPE_int | symn_read, TYPE_i32, 1);
	type_i16 = install(cc, "int16", TYPE_int | symn_read, TYPE_i32, 2);
	type_i32 = install(cc, "int32", TYPE_int | symn_read, TYPE_i32, 4);
	type_i64 = install(cc, "int64", TYPE_int | symn_read, TYPE_i64, 8);

	//~ type_f16 = install(cc, "flt16", TYPE_flt | symn_read, TYPE_f32, 2);
	type_f32 = install(cc, "flt32", TYPE_flt | symn_read, TYPE_f32, 4);
	type_f64 = install(cc, "flt64", TYPE_flt | symn_read, TYPE_f64, 8);

	if (COMPILER_LEVEL >= 1) {		// Packed types are be optional
		type_v4f = install(cc, "vec4f", TYPE_rec | symn_read, TYPE_p4x, 16);
		if (type_v4f) {
			enter(cc, type_v4f);
			installex(cc, "x", TYPE_ref,  0, type_f32, NULL);
			installex(cc, "y", TYPE_ref,  4, type_f32, NULL);
			installex(cc, "z", TYPE_ref,  8, type_f32, NULL);
			installex(cc, "w", TYPE_ref, 12, type_f32, NULL);
			type_v4f->args = leave(cc, type_v4f);
		}

		type_v2d = install(cc, "vec2d", TYPE_rec | symn_read, TYPE_p4x, 16);
		if (type_v2d) {
			enter(cc, type_v2d);
			installex(cc, "x", TYPE_ref,  0, type_f64, NULL);
			installex(cc, "y", TYPE_ref,  8, type_f64, NULL);
			type_v2d->args = leave(cc, type_v2d);
		}

		//~ type_xxx = install(cc, "i8x16", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "i16x8", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "i32x4", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "i64x2", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "u8x16", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "u16x8", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "u32x4", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "u64x2", TYPE_rec | symn_read, TYPE_p4x, 16);
		//~ type_xxx = install(cc, "f16x8", TYPE_rec | symn_read, TYPE_p4x, 16);
	}

	//~ type_arr = install(cc, "array", TYPE_ptr, 0, 0);
	//~ type_ptr = install(cc, "pointer", TYPE_ptr, 0, 0);
	type_str = install(cc, "string", TYPE_str | symn_read, TYPE_ref, 4);

	//~ type_chr = installex(cc, "char", TYPE_def, 0, type_i08, NULL);
	//~ type_str = installex(cc, "string", TYPE_arr, 0, type_chr, NULL);

	if (COMPILER_LEVEL >= 3) {		// fields of integer types
		install_bits(cc, type_i08, -1); install_bits(cc, type_u08, 0);
		install_bits(cc, type_i16, -1); install_bits(cc, type_u16, 0);
		install_bits(cc, type_i32, -1); install_bits(cc, type_u32, 0);
		install_bits(cc, type_i64, -1); //install_bits(cc, type_u64, 0);
	}

	installex(cc, "int", TYPE_def, 0, type_i32, NULL);
	installex(cc, "long", TYPE_def, 0, type_i64, NULL);
	installex(cc, "float", TYPE_def, 0, type_f32, NULL);
	installex(cc, "double", TYPE_def, 0, type_f64, NULL);

	installex(cc, "true", TYPE_def, 0, type_bol, intnode(cc, 1));
	installex(cc, "false", TYPE_def, 0, type_bol, intnode(cc, 0));

	installex(cc, "null", TYPE_def, 0, type_vid, intnode(cc, 0));

	//} */// types
	// install a void arg for functions with no arguments
	if ((cc->argz = newnode(cc, TYPE_ref))) {
		enter(cc, NULL);
		cc->argz->id.name = ".";
		declare(cc, TYPE_ref, cc->argz, type_vid);
		leave(cc, NULL);
	}
	emit_opc = install(cc, "emit", EMIT_opc | symn_read, 0, 0);
	//~ emit_opc->call = 1;

	if (COMPILER_LEVEL) {
		install_math(cc, COMPILER_LEVEL);
		install_emit(cc, COMPILER_LEVEL);
		libcall(s, NULL, "defaults");
	}

	return cc;
}
vmEnv vmInit(state s) {
	int size = s->_cnt;
	vmEnv vm = getmem(s, size, sizeof(struct vmEnv));
	if (vm != NULL) {
		vm->s = s;
		//~ done by getmem
		//~ vm->ds = vm->ic = 0;
		//~ vm->cs = vm->pc = 0;
		//~ vm->ss = vm->sm = 0;
		vm->_ptr = vm->_mem;
		vm->_end = vm->_mem + size - sizeof(struct vmEnv);
	}
	return s->vm = vm;
}

ccEnv ccOpen(state s, srcType mode, char *src) {
	if (s->cc || ccInit(s)) {
		if (source(s->cc, mode & srcFile, src) != 0)
			return NULL;
	}
	return s->cc;
}

int ccDone(state s) {
	ccEnv cc = s->cc;

	// if not initialized
	if (cc == NULL)
		return -1;

	// close input
	source(cc, 0, 0);

	// set used memory
	s->_cnt -= cc->_ptr - s->_ptr;
	s->_ptr = cc->_ptr;

	dieif(s->_cnt != cc->_cnt, "FixMe", s->_cnt - cc->_cnt);
	//~ cc->_ptr = 0;
	cc->_cnt = 0;

	return s->errc;
}
/*int vmDone(state s) {
	// if not initialized
	if (s->vm == NULL)
		return 0;
	// flush and release files
	debug("code:%d Bytes", (s->_ptr - s->_mem) + s->vm->_ptr);
	return 0;
}// */

void* getmem(state s, int size, unsigned clear) {
	if (size <= s->_cnt) {
		void *mem = s->_ptr;
		s->_cnt -= size;
		s->_ptr += size;

		if (clear > size)
			clear = size;

		memset(mem, 0, clear);

		return mem;
	}
	return NULL;
}

//{ temp.c ---------------------------------------------------------------------
#if 0
int vm_hgen() {
	int i, e = 0;
	FILE *out = stdout;
	for (i = 0; i < opc_last; i++) {
		int err = 0;
		if (!opc_tbl[i].name) continue;
		//~ if (opc_tbl[i].size <= 0) continue;

		fprintf(out, "\nInstruction: %s: #\n", opc_tbl[i].name);
		fprintf(out, "Opcode		argsize		Stack trasition\n");
		fprintf(out, "0x%02x		%d		[..., a, b, c, d => [..., a, b, c, d#\n", opc_tbl[i].code, opc_tbl[i].size-1);

		fprintf(out, "\nDescription\n");
		if (opc_tbl[i].code != i) {
			e += err = 1;
		}
		else switch (i) {
			error_opc: e += err = 1; break;
			#define NEXT(__IP, __CHK, __SP) if (opc_tbl[i].size != (__IP)) goto error_opc;
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "code.h"
		}
		if (err)
		fprintf(out, "The '%s' instruction %s\n", opc_tbl[i].name, err ? "is invalid" : "#");

		fprintf(out, "\nOperation\n");
		fprintf(out, "#\n");

		fprintf(out, "\nExceptions\n");
		fprintf(out, "None#\n");

		fprintf(out, "\n");		// end of text
	}
	return e;
}
int mk_test(char *file, int size) {
	//~ char test[] = "signed _000000000000000001 = 8;\n";
	char test[] = "int _0000001=6;\n";
	FILE* f = fopen(file, "wb");
	int n, sp = sizeof(test) - 6;

	if (!f) {
		debug("cann not open file");
		return -1;
	}

	if (size <= (128 << 20)) {
		for (n = 0; n < size; n += sizeof(test)-1) {
			fputs(test, f);
			while (++test[sp] > '9') {
				test[sp--] = '0';
				if (test[sp] == '_') {
					fclose(f);
					return -2;
				}
			}
			sp = sizeof(test) - 6;
		}
	}
	else debug("file is too large (128M max)");

	fclose(f);
	return 0;
}
#endif

// lookup a value
symn findsym(ccEnv s, symn in, char *name) {
	struct astn ast;
	memset(&ast, 0, sizeof(struct astn));
	ast.kind = TYPE_ref;
	ast.id.name = name;
	return lookup(s, in ? in : s->defs, &ast, NULL);
}

/*int finddef(ccEnv s, char *name, stkval* data, int TYPE) {
	int t;
	symn sym;
	struct astn ast;
	if ((sym = findsym(s, name)) {
		if (TYPE == TYPE_any)
			return 1;

		t = eval(&ast, sym->init);
		switch (t) {
			case TYPE_int: return 1;
			case TYPE_flt: return 1;
			//~ case TYPE_str: return 1;
		}
	return 0;
}// */

int findint(ccEnv s, char *name, int* res) {
	struct astn ast;
	symn sym = findsym(s, NULL, name);
	if (sym && eval(&ast, sym->init)) {
		*res = constint(&ast);
		return 1;
	}
	return 0;
}
int findflt(ccEnv s, char *name, double* res) {
	struct astn ast;
	symn sym = findsym(s, NULL, name);
	if (sym && eval(&ast, sym->init)) {
		*res = constflt(&ast);
		return 1;
	}
	return 0;
}
int findnzv(ccEnv s, char *name) {
	struct astn ast;
	symn sym = findsym(s, NULL, name);
	if (sym && eval(&ast, sym->init)) {
		return constbol(&ast);
	}
	return 0;
}

#if 0
struct arrBuffer {
	char *ptr;
	int max;
	int esz;
	//~ int cnt;
};
static void* getBuff(struct arrBuffer* buff, int idx) {
	int pos = idx * buff->esz;
	if (pos >= buff->max) {
		buff->max = pos << 1;		//TODO: max(pos + buff->esz, pos * 2)
		buff->ptr = realloc(buff->ptr, buff->max);
	}
	return buff->ptr ? buff->ptr + pos : NULL;
}
static void* setBuff(struct arrBuffer* buff, int idx, void* data) {
	void* ptr;
	if ((ptr = getBuff(buff, idx)))
		memcpy(ptr, data, buff->esz);
	return ptr;
}
/*static int addBuff(struct arrBuffer* buff, void* data) {
	if (setBuff(buff, buff->cnt, data))
		return buff->cnt ++;
	return -1;
}*/
static void newBuff(struct arrBuffer* buff, int initsize, int elemsize) {
	buff->ptr = 0;
	buff->max = initsize;
	buff->esz = elemsize;
	getBuff(buff, initsize);
}
static void delBuff(struct arrBuffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->max = 0;
}

#endif
//} */

