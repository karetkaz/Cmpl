#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "ccvm.h"

//~ #include "clog.c"
//~ #include "scan.c"
//~ #include "code.c"
//~ #include "tree.c"
//~ #include "type.c"
//~ #include "main.c"

const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, STR) {TYPE, SIZE, STR},
	#include "incl/defs.h"
	{0},
};
const opc_inf opc_tbl[255] = {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) \
			{Code, Size, Args, Push, Mnem},
	#include "code.h"
	{0},
};

int fixargs2(symn fun, int sp) {
	symn arg;
	for (arg = fun->args; arg; arg = arg->next) {
		sp += arg->type->size / 4;
	}
	for (arg = fun->args; arg; arg = arg->next) {
		arg->offs = -sp;
		sp -= arg->type->size / 4;

		switch (castId(arg->type)) {
			default: fatal("FixMe!");
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

void fixargs(symn fun, astn argn) {
	symn args = fun->args;
	if (argn) while (args && argn) {
		args->offs = argn->XXXX;
		args = args->next;
		argn = argn->next;
	}
	else while (args) {
		args->offs = 1;
		args = args->next;
	}
}

#define dbg_ast(__AST) debug("cgen(err, %+k)\n%3K", __AST, __AST)
#define dbg_arg(__AST) debug("cgen(arg, %+k)\n%3K", __AST, __AST)
#define dbg_rhs(__AST) debug("cgen(rhs, %+k)\n%3K", (__AST)->op.rhso, __AST)
#define dbg_lhs(__AST) debug("cgen(rhs, %+k)\n%3K", (__AST)->op.lhso, __AST)
#define dbg_opc(__OPC, __AST) debug("emit(%02x, %+k, %t)\n%3K", (__OPC), __AST, (__AST)->cst2, __AST)

static int push(state s, astn ast) {
	struct astn tmp;
	int cast;

	if (!ast) {
		fatal("null ast");
		return 0;
	}

	cast = ast->cst2;

	//~ debug("push(arg, %+k, %t)", ast, cast);

	if (s->vm->opti && eval(&tmp, ast)) {
		ast = &tmp;
	}

	return cgen(s, ast, cast);
}
int cgen(state s, astn ast, int get) {
	//~ int ipdbg = emit(s->vm, get_ip);
	struct astn tmp;
	int ret = 0;

	if (!ast) {
		if (get == TYPE_vid)
			return TYPE_vid;
		debug("null ast");
		return 0;
	}

	dieif(!ast->type, "untyped ast: %t(%+k)", ast->kind, ast);

	if (get == 0)
		get = ast->cst2;

	ret = castId(ast->type);

	switch (ast->kind) {
		default: fatal("FixMe!: %t(%k)", ast->kind, ast);
		//{ STMT; TODO: catch and print errors here
		case OPER_nop: {	// expr statement or decl
			int stpos = emit(s->vm, get_sp);
			emit(s->vm, opc_line, ast->line);
			if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
				dbg_ast(ast->stmt.stmt);
				//~ dumpasmdbg(stderr, s->vm, ipdbg, 0x10);
				return 0;
			}
			if (stpos < emit(s->vm, get_sp))
				warn(s, 1, s->cc->file, ast->line, "statement underflows stack: %+k", ast->stmt.stmt);
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
				if (!emitidx(s->vm, opc_pop, stpos)) {
					debug("%+k", ast);
					return 0;
				}
			}
		} return TYPE_vid;
		case STMT_if:  {
			int jmpt = 0, jmpf = 0;
			int tt = eval(&tmp, ast->stmt.test);

			dieif(get != TYPE_vid, "FixMe!");
			emitint(s->vm, opc_line, ast->line);

			if (ast->cst2 == QUAL_sta && (ast->stmt.step || !tt)) {
				error(s, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "unexpected else part");
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
				if (!(jmpt = emit(s->vm, opc_jz))) {
					debug("%+k", ast);
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				if (!(jmpf = emit(s->vm, opc_jmp))) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpf, emit(s->vm, get_ip), 0);
			}
			else if (ast->stmt.stmt) {					// if then
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				//~ if false skip THEN block
				if (!(jmpt = emit(s->vm, opc_jz))) {
					debug("%+k", ast);
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
			}
			else if (ast->stmt.step) {					// if else
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				//~ if true skip ELSE block
				if (!(jmpt = emit(s->vm, opc_jnz))) {
					debug("%+k", ast);
					return 0;
				}
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
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
					debug("%+k", ast);
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
				dieif(!emitidx(s->vm, opc_pop, stpos), "FixMe!");
		} return TYPE_vid;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/call/cast
			int stargs = emit(s->vm, get_sp);
			astn argl = 0, argv = ast->op.rhso;
			symn var = linkOf(ast, 0);	// refs only
			//~ symn typ = ast->type;

			if (var && var->kind == TYPE_def) {
				if (argv) {
					astn an = NULL;
					symn as = var->args;

					while (argv->kind == OPER_com) {
						astn arg = argv->op.rhso;
						argv = argv->op.lhso;

						arg->next = an;
						an = arg;
					}

					argv->next = an;
					an = argv;

					while (an && as) {
						as->kind = TYPE_def;
						as->init = an;
						an = an->next;
						as = as->next;
					}

					if (!cgen(s, var->init, ret)) {
						dbg_arg(var->init);
					}

					as = var->args;
					while (as) {
						as->kind = TYPE_ref;
						as->init = NULL;
						as = as->next;
					}
				}
				//~ return ret;
				break;
			}

			// push args except the first
			while (argv && argv->kind == OPER_com) {
				astn arg = argv->op.rhso;
				if (!push(s, arg)) {
					dbg_arg(arg);
					return 0;
				}
				arg->XXXX = emit(s->vm, get_sp);
				arg->next = argl;
				argl = arg;
				argv = argv->op.lhso;
			}

			if (var == emit_opc) {			// emit()
				symn opc = linkOf(argv, 1);	// refs & types

				if (opc && opc->kind == EMIT_opc) {
					ret = castId(opc->type);
					if (ret == TYPE_vid) {
						// in case of nop 4ex
						//~ debug("emit(%+k): %T(%T)", ast->op.rhso, ast->type, opc);
						ret = get;
					}
					//~ only this emit can err
					if (!emitint(s->vm, opc->offs, opc->init ? opc->init->con.cint : 0)) {
						error(s, ast->line, "error emiting opcode: %+k", argv);
						if (emit(s->vm, get_sp) > 0)
							info(s, s->cc->file, ast->line, "%+k underflows stack.", ast);
							//~ debug("opcode expected, not %k : %T", argv, opc);
						return 0;
					}
				}
				else if (opc == type_vid) {
					ret = get;
				}
				else {
					error(s, ast->line, "opcode expected, and or arguments, got `%T`", opc);
					return 0;
				}
			}
			else if (var) {					// call()
				if (argv && !push(s, argv)) {
					dbg_arg(argv);
					return 0;
				}
				argv->next = argl;
				argl = argv;
				if (var->call) {
					int spbc = emit(s->vm, get_sp);
					if (!var->offs) {			// inline or error
						if (!var->init || var->init->kind != OPER_fnc) {
							error(s, ast->line, "invalid function call: %+T", var);
							info(s, var->file, var->line, "defined here");
							return 0;
						}
						trace("call.inl(%+k: %+T): %+T@", ast, var, ast->type);
						fixargs2(var, -stargs);
						//~ fixargs2(var, argl);

						if (!cgen(s, var->init, ret)) {
							debug("%+k:%+T", var->init, ast->type);
							return 0;
						}
						if (spbc <= emit(s->vm, get_sp)) {
							error(s, ast->line, "invalid stack size %+k", ast);
							//~ dumpasmdbg(stderr, s->vm, ipdbg, 0x10);
							return 0;
						}
						switch (ret) {
							default: fatal("FixMe!");
							case TYPE_u32:
							case TYPE_i32:
							case TYPE_f32: if (!emitidx(s->vm, opc_set1, stargs -= 1)) return 0; break;
							case TYPE_i64:
							case TYPE_f64: if (!emitidx(s->vm, opc_set2, stargs -= 2)) return 0; break;
							//~ case TYPE_p4x: if (!emitidx(s->vm, opc_set4, stargs -= 4)) return 0; break;
						}
						if (!emitidx(s->vm, opc_pop, stargs))
							return 0;
					}
					else {
						if (var->offs < 0) {
							trace("call.lib(%+k: %+T): %+T@", ast, var, ast->type);
							if (!emit(s->vm, opc_libc, -var->offs)) {
								debug("%+k", ast);
								return 0;
							}
						}
						else {
							trace("call.fun(%+k: %+T): %+T@", ast, var, ast->type);
							if (!emit(s->vm, opc_call, var->offs)) {
								debug("%+k", ast);
								return 0;
							}
						}
					}
				}
				else {
					error(s, ast->line, "called object is not a function: %+T", var);
					//~ but can have an operator: opCall() with this args
				}
			}
			else {							// cast()
				//~ debug("cast(%+k):%t", ast, ast->cst2);
				if (!argv || argv != ast->op.rhso /* && cast*/)
					warn(s, 5, s->cc->file, ast->line, "multiple values: '%+k'", ast);

				if (!(push(s, argv))) {
					dbg_arg(argv);
					return 0;
				}// */
			}
		} break;
		case OPER_idx: {	// '[]'
			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				dbg_lhs(ast);
				return 0;
			}
			if (eval(&tmp, ast->op.rhso)) {
				int offs = ast->type->size * constint(&tmp);
				if (!emit(s->vm, opc_inc, offs)) {
					if (!emit(s->vm, opc_ldc4, offs)) {
						dbg_opc(opc_ldc4, ast);
						return 0;
					}
					if (!emit(s->vm, i32_add)) {
						dbg_opc(opc_ldc4, ast);
						return 0;
					}
				}
			}
			else {
				if (!emit(s->vm, opc_ldc4, ast->type->size)) {
					dbg_opc(opc_ldc4, ast);
					return 0;
				}
				if (!cgen(s, ast->op.rhso, TYPE_u32)) {
					dbg_rhs(ast);
					return 0;
				}
				if (!emit(s->vm, u32_mad)) {
					debug("emit(opc_x%02x, %+k, %t)", u32_mad, ast, ast->cst2);
					return 0;
				}
			}
			if ((ret = get) != TYPE_ref) {
				ret = castId(ast->type);
				if (!emit(s->vm, opc_ldi, ast->type->size)) {
					debug("emit(opc_x%02x, %t, %+k, %t)", opc_ldi, ret, ast, ast->cst2);
					return 0;
				}
			}
		} break; // */
		case OPER_dot: {	// '.'
			symn var = linkOf(ast->op.rhso, 0);
			if (istype(ast->op.lhso)) {
				return cgen(s, ast->op.rhso, get);
			}

			if (!var) {
				debug("%T", var);
				dbg_rhs(ast);
				return 0;
			}// */

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				dbg_lhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc_inc, var->offs)) {
				if (!emit(s->vm, opc_ldc4, var->offs)) {
					dbg_opc(opc_ldc4, ast);
					return 0;
				}
				if (!emit(s->vm, i32_add)) {
					dbg_opc(opc_ldc4, ast);
					return 0;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emit(s->vm, opc_ldi, ast->type->size)) {
				dbg_opc(opc_ldi, ast);
				return 0;
			}
		} break;

		case OPER_not:		// '!'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			int opc = -1;
			switch (ast->kind) {
				default: fatal("FixMe!");
				case OPER_pls: return cgen(s, ast->op.rhso, get);
				case OPER_mns: opc = opc_neg; break;
				case OPER_not: opc = opc_not; break;
				//~ case OPER_cmt: opc = opc_cmt; break;
			}
			if (!cgen(s, ast->op.rhso, get)) {
				dbg_rhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc, get)) {
				dbg_opc(opc, ast);
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
				default: fatal("FixMe!");
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
				dbg_opc(opc, ast);
				return 0;
			}
			switch (ast->kind) {
				default:
					ret = castId(ast->type);
					break;

				case OPER_neq:
				case OPER_equ:
				case OPER_geq:
				case OPER_lte:
				case OPER_leq:
				case OPER_gte:
					ret = TYPE_bit;
					break;
			}
			//~ debug("OP(%+k: %t, %t): %t", ast, ast->cast, get, ret);
		} break;

		//~ case OPER_lnd:		// &&
		//~ case OPER_lor:		// ||
		case OPER_sel: {		// ?:
			int jmpt, jmpf;
			int tt = eval(&tmp, ast->op.test);
			if (s->vm->opti && tt) {
				ret = cgen(s, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, get);
			}
			else {
				int stpos = emit(s->vm, get_sp);
				if (!cgen(s, ast->op.test, TYPE_bit)) {
					debug("cgen(%+k)", ast);
					return 0;
				}
				if (!(jmpt = emit(s->vm, opc_jz))) {
					debug("cgen(%+k)", ast);
					return 0;
				}

				if (!cgen(s, ast->op.lhso, 0)) {
					debug("cgen(%+k)", ast);
					return 0;
				}
				if (!(jmpf = emit(s->vm, opc_jmp))) {
					debug("cgen(%+k)", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);

				if (!emitint(s->vm, set_sp, -stpos)) {
					debug("cgen(%+k)", ast);
					return 0;
				}
				if (!cgen(s, ast->op.rhso, 0)) {
					debug("cgen(%+k)", ast);
					return 0;
				}
				fixjump(s->vm, jmpf, emit(s->vm, get_ip), 0);
			}
			ret = castId(ast->type);
		} break;

		case ASGN_set: {		// '='
			// TODO: lhs should be evaluated first !!!
			//~ lhs in (TYPE_ref, OPER_dot, OPER_fnc, OPER_idx)
			int opc = -1;

			symn var = linkOf(ast->op.lhso, 0);
			ret = castId(ast->type);

			if (!cgen(s, ast->op.rhso, ret)) {
				dbg_rhs(ast);
				return 0;
			}
			if (get != TYPE_vid) {
				switch (ast->type->size) {
					default: fatal("FixMe!");
					case 1:
					case 2:
					case 4:
						opc = opc_dup1;
						break;

					case 8:
						opc = opc_dup2;
						break;

					case 16:
						opc = opc_dup4;
						break;
				}
				if (!emit(s->vm, opc, 0)) {
					debug("emit(%+k)", ast);
					return 0;
				}
			}

			if (var) {
				if (var->offs < 0) {
					switch (ast->type->size) {
						default: fatal("FixMe!");
						case 1:
						case 2:
						case 4:
							opc = opc_set1;
							break;

						case 8:
							opc = opc_set2;
							break;

						case 16:
							opc = opc_set4;
							break;
					}
					if (!emitidx(s->vm, opc, var->offs)) {
						debug("emit(%+k, %d)", ast);
						return 0;
					}
				}
				else {
					debug("%+k", ast);
					if (!cgen(s, ast->op.lhso, TYPE_ref)) {
						debug("cgen(lhs, %+k)", ast->op.lhso);
						return 0;
					}
				}
				//~ if (var->load) {}
			}
			else {
				if (!cgen(s, ast->op.lhso, TYPE_ref)) {
					dbg_rhs(ast);
					return 0;
				}
				if (!emit(s->vm, opc_sti, ast->type->size)) {
					debug("emit(%+k, %d)", ast);
					return 0;
				}
			}
		} break;
		//}
		//{ TVAL
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
		//~ case CNST_str: return 0;	// unimpl yet(ref)

		case TYPE_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->id.link;		// link
			dieif(!var, "FixMe!");

			if (var->kind == TYPE_ref) {
				if (get == TYPE_ref) {
					if (var->offs < 0) {
						return emitidx(s->vm, opc_ldsp, var->offs) ? TYPE_ref : 0;
					}
					else {
						return emitint(s->vm, opc_ldcr, var->offs) ? TYPE_ref : 0;
					}
				}

				//~ TODO: Indirection
				else if (var->offs < 0) {	// on stack
					switch (ret = castId(typ)) {
						default: fatal("FixMe!: (%t, %t):%+k:%T", get, ret, ast, typ);
						case TYPE_u32:
						case TYPE_i32:
						case TYPE_f32: if (!emitidx(s->vm, opc_dup1, var->offs)) return 0; break;
						case TYPE_i64:
						case TYPE_f64: if (!emitidx(s->vm, opc_dup2, var->offs)) return 0; break;
						//~ case TYPE_p4x: if (!emitidx(s->vm, opc_dup4, var->offs)) return 0; break;
					}
				}
				else {						// in memory
					if (!emit(s->vm, opc_ldc4, var->offs)) {
						dbg_opc(opc_ldc4, ast);
						return 0;
					}
					if (!emit(s->vm, opc_ldi, ast->type->size)) {
						dbg_opc(opc_ldc4, ast);
						return 0;
					}
				}
			}
			else {
				if (var->kind == TYPE_def && var->init)
					return cgen(s, var->init, get);

				error(s, ast->line, "invalid rvalue `%+k`", ast);
				return 0;
			}
		} break;
		case TYPE_def: {					// new (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->id.link;		// TODO: ?link?

			dieif(!var, "FixMe!");

			if (var->call) {
				// assert that if not inline fun, then offset > 0
				//~ fixargs(var, NULL);
				//~ debug("function: %+k", ast);
				return TYPE_vid;
			}// */
			if (var->kind == TYPE_ref) {
				astn val = var->init;
				//~ debug("cgen(var): %+k:%T", ast, ast->type);
				dieif(ast->type != var->type, "FixMe!");
				if (s->vm->opti && eval(&tmp, val)) {
					val = &tmp;
				}
				switch (ret = castId(typ)) {
					default: fatal("FixMe!");
					case TYPE_bit:
					case TYPE_u32:
					//~ case TYPE_int:
					case TYPE_i32:
					case TYPE_f32: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_ldz1))) {
							dbg_arg(val);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
					} break;
					case TYPE_f64:
					case TYPE_i64: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_ldz2))) {
							dbg_ast(val);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
					} break;
					/*case TYPE_p4x: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_ldz4))) {
							dbg_arg(val);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
					} break;// */
					case TYPE_rec: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_loc, ((typ->size + 3) / 4)))) {
							dbg_arg(val);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
						if (-4 * var->offs < typ->size) {
							error(s, ast->line, "stack underflow", -4*var->offs, typ->size);
							return 0;
						}
					} break;//*/
					case TYPE_arr: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_loc, ((typ->size + 3) / 4)))) {
							dbg_arg(val);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
						if (!emit(s->vm, opc_ldc4, var->offs)) {
							dbg_opc(opc_ldc4, ast);
							return 0;
						}
						if (-4 * var->offs < typ->size) {
							error(s, ast->line, "stack underflow", -4*var->offs, typ->size);
							return 0;
						}
					} break;
				}
				return ret;
			}

		} break;

		//~ case TYPE_rec:
		case TYPE_enu: break;
		//}
	}

	//~ ret = ast->type->kind;
	//~ if (ret != castId(ast->type))	//? emit(void, ...);
		//~ debug("gen(%t, %t, %t, '%+k')", get, castId(ast->type), ret, ast);
	if (get != ret) switch (get) {
		case TYPE_vid: return TYPE_vid;		// TODO: FixMe!
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
		default: fatal("unimplemented(`%+k`, %t):%t", ast, get, ret);
		errorcast2: debug("invalid cast: [%d->%d](%t to %t) %k: '%+k'", ret, get, ret, get, ast, ast);
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
			vm->ds += sym->type->size;
			//~ mem[code->ds] = sym->init;
		}
		//~ TODO: alloc and other stuff
	}
	// emit global functions
	for (sym = cc->defs; sym; sym = sym->next) {
		if (sym->kind == TYPE_ref && sym->call) {
			debug("TODO(%T)", sym);
			//~ cgen(s, sym->init, TYPE_vid);
			continue;
		}
		//~ TODO: alloc and other stuff
	}
	// */

	//~ header:
	//~ seg:Data ro: initialized
	//~ seg:Code rx
	//~ seg:dbg  ro(s->cc->defs || all->debug)

	//~ emit(s->vm, loc_data, 256 * 4);
	emit(s->vm, seg_code);
	emit(s->vm, opc_nop);
	if (s->cc->root)
		cgen(s, cc->root, 0);			// TODO: TYPE_vid: to clear the stack
	emit(s->vm, opc_sysc, 0);
	emit(s->vm, seg_code);
	//~ emit debug symbols

	return s->errc;
}
//~ int execute(state s, int stack) ;

static void instint(ccEnv cc, symn it, int64t sgn) {
#if 0
	int size = it->size;
	int bits = 8 * size;
	int64t mask = (-1LLU >> -bits);
	int64t minv = sgn << (bits - 1);
	int64t maxv = (minv - 1) & mask;
	enter(s, it);
	installex(s, TYPE_def, "min",  0, type_i64, intnode(s, minv));
	installex(s, TYPE_def, "max",  0, type_i64, intnode(s, maxv));
	installex(s, TYPE_def, "mask", 0, type_i64, intnode(s, mask));
	installex(s, TYPE_def, "bits", 0, type_i64, intnode(s, bits));
	installex(s, TYPE_def, "size", 0, type_i64, intnode(s, size));
	it->args = leave(s, it);
#endif
}//~ */

symn emit_opc = 0;
static void install_emit(ccEnv cc) {
	int i = 0, TYPE_opc = EMIT_opc;
	symn typ;
	/*symn typ,tyq,*tyb;//,tyd;
	static char *TypeList[]={"u32","i32","f32", "i64", "f64", "v4f", "v2d",  NULL};
	symn TypeNode[] = {type_u32, type_i32, type_f32, type_i64, type_f64, type_v4f, type_v2d };
	char **Type;*/

	emit_opc = install(cc, "emit", TYPE_opc | symn_read, 0, 0);
	//~ emit_opc->call = 1;
	//~ emit_opc->type = 0;
	enter(cc, NULL);

	installex(cc, "nop", TYPE_opc, opc_nop, type_vid, NULL);

	enter(cc, typ = install(cc, "u32", TYPE_int, 0, 4));
	installex(cc, "cmt", TYPE_opc, b32_cmt, type_u32, NULL);
	//~ installex(cc, "adc", TYPE_opc, b32_adc, type_u32, NULL);
	//~ installex(cc, "sub", TYPE_opc, b32_sbb, type_u32, NULL);
	installex(cc, "mul", TYPE_opc, u32_mul, type_u32, NULL);
	installex(cc, "div", TYPE_opc, u32_div, type_u32, NULL);
	installex(cc, "mad", TYPE_opc, u32_mad, type_u32, NULL);
	installex(cc, "clt", TYPE_opc, u32_clt, type_bol, NULL);
	installex(cc, "cgt", TYPE_opc, u32_cgt, type_bol, NULL);
	installex(cc, "and", TYPE_opc, b32_and, type_u32, NULL);
	installex(cc,  "or", TYPE_opc, b32_ior, type_u32, NULL);
	installex(cc, "xor", TYPE_opc, b32_xor, type_u32, NULL);
	installex(cc, "shl", TYPE_opc, b32_shl, type_u32, NULL);
	installex(cc, "shr", TYPE_opc, b32_shr, type_u32, NULL);
	typ->args = leave(cc, typ);

	enter(cc, typ = install(cc, "i32", TYPE_int, TYPE_i32, 4));
	installex(cc, "cmt", TYPE_opc, b32_cmt, type_i32, NULL);
	installex(cc, "neg", TYPE_opc, i32_neg, type_i32, NULL);
	installex(cc, "add", TYPE_opc, i32_add, type_i32, NULL);
	installex(cc, "sub", TYPE_opc, i32_sub, type_i32, NULL);
	installex(cc, "mul", TYPE_opc, i32_mul, type_i32, NULL);
	installex(cc, "div", TYPE_opc, i32_div, type_i32, NULL);
	installex(cc, "mod", TYPE_opc, i32_mod, type_i32, NULL);

	installex(cc, "ceq", TYPE_opc, i32_ceq, type_bol, NULL);
	installex(cc, "clt", TYPE_opc, i32_clt, type_bol, NULL);
	installex(cc, "cgt", TYPE_opc, i32_cgt, type_bol, NULL);

	installex(cc, "and", TYPE_opc, b32_and, type_i32, NULL);
	installex(cc,  "or", TYPE_opc, b32_ior, type_i32, NULL);
	installex(cc, "xor", TYPE_opc, b32_xor, type_i32, NULL);
	installex(cc, "shl", TYPE_opc, b32_shl, type_i32, NULL);
	installex(cc, "shr", TYPE_opc, b32_sar, type_i32, NULL);
	typ->args = leave(cc, typ);

	enter(cc, typ = install(cc, "i64", TYPE_int, TYPE_i64, 8));
	installex(cc, "neg", TYPE_opc, i64_neg, type_i64, NULL);
	installex(cc, "add", TYPE_opc, i64_add, type_i64, NULL);
	installex(cc, "sub", TYPE_opc, i64_sub, type_i64, NULL);
	installex(cc, "mul", TYPE_opc, i64_mul, type_i64, NULL);
	installex(cc, "div", TYPE_opc, i64_div, type_i64, NULL);
	installex(cc, "mod", TYPE_opc, i64_mod, type_i64, NULL);
	typ->args = leave(cc, typ);

	enter(cc, typ = install(cc, "f32", TYPE_flt, TYPE_f32, 4));
	installex(cc, "neg", TYPE_opc, f32_neg, type_f32, NULL);
	installex(cc, "add", TYPE_opc, f32_add, type_f32, NULL);
	installex(cc, "sub", TYPE_opc, f32_sub, type_f32, NULL);
	installex(cc, "mul", TYPE_opc, f32_mul, type_f32, NULL);
	installex(cc, "div", TYPE_opc, f32_div, type_f32, NULL);
	installex(cc, "mod", TYPE_opc, f32_mod, type_f32, NULL);
	typ->args = leave(cc, typ);

	enter(cc, typ = install(cc, "f64", TYPE_flt, TYPE_f64, 8));
	installex(cc, "neg", TYPE_opc, f64_neg, type_f64, NULL);
	installex(cc, "add", TYPE_opc, f64_add, type_f64, NULL);
	installex(cc, "sub", TYPE_opc, f64_sub, type_f64, NULL);
	installex(cc, "mul", TYPE_opc, f64_mul, type_f64, NULL);
	installex(cc, "div", TYPE_opc, f64_div, type_f64, NULL);
	installex(cc, "mod", TYPE_opc, f64_mod, type_f64, NULL);
	typ->args = leave(cc, typ);

	enter(cc, typ = install(cc, "v4f", TYPE_enu, 0, 16));
	installex(cc, "neg", TYPE_opc, v4f_neg, type_v4f, NULL);
	installex(cc, "add", TYPE_opc, v4f_add, type_v4f, NULL);
	installex(cc, "sub", TYPE_opc, v4f_sub, type_v4f, NULL);
	installex(cc, "mul", TYPE_opc, v4f_mul, type_v4f, NULL);
	installex(cc, "div", TYPE_opc, v4f_div, type_v4f, NULL);
	installex(cc, "dp3", TYPE_opc, v4f_dp3, type_f32, NULL);
	installex(cc, "dp4", TYPE_opc, v4f_dp4, type_f32, NULL);
	installex(cc, "dph", TYPE_opc, v4f_dph, type_f32, NULL);
	typ->args = leave(cc, typ);

	enter(cc, typ = install(cc, "v2d", TYPE_enu, 0, 16));
	installex(cc, "neg", TYPE_opc, v2d_neg, type_v2d, NULL);
	installex(cc, "add", TYPE_opc, v2d_add, type_v2d, NULL);
	installex(cc, "sub", TYPE_opc, v2d_sub, type_v2d, NULL);
	installex(cc, "mul", TYPE_opc, v2d_mul, type_v2d, NULL);
	installex(cc, "div", TYPE_opc, v2d_div, type_v2d, NULL);
	typ->args = leave(cc, typ);

	//~ /*
	{
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
			installex(cc, swz[i].name, TYPE_opc, p4d_swz, type_v4f, swz[i].node);
		typ->args = leave(cc, typ);

		//~ extended set(masked) and dup(swizzle): p4d.dup.xyxy / p4d.set.xyz0
		enter(cc, typ = install(cc, "dup", TYPE_enu, 0, 0));
		for (i = 0; i < 256; i += 1)
			installex(cc, swz[i].name, TYPE_opc, 0x1d, type_v4f, swz[i].node);
		typ->args = leave(cc, typ);

		enter(cc, typ = install(cc, "set", TYPE_enu, 0, 0));

		installex(cc,    "x", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,    "y", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,    "z", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,    "w", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc,  "xyz", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyzw", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyz0", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyz1", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));
		installex(cc, "xyz_", TYPE_opc, 0x1e, type_v4f, intnode(cc, 0));

		typ->args = leave(cc, typ);
	} //~ */
	(void)i;
	emit_opc->args = leave(cc, emit_opc);
}

state gsInit(void* mem, unsigned size) {
	state s = mem;
	s->_cnt = size - sizeof(struct state);
	s->_ptr = s->_mem;
	s->cc = 0;
	s->vm = 0;
	return s;
}
ccEnv ccInit(state s) {
	ccEnv cc = getmem(s, sizeof(struct ccEnv), -1);
	symn def;//, type_chr = 0;
	symn type_i08 = 0, type_i16 = 0;
	symn type_u08 = 0, type_u16 = 0;
	//~ symn type_u64 = 0, type_f16 = 0;
	const int TYPE_p4x = 0;

	if (cc == NULL)
		return NULL;

	s->cc = cc;
	cc->s = s;

	s->errc = 0;
	s->logf = 0;

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

	cc->deft = getmem(s, TBLS * sizeof(symn), -1);
	cc->strt = getmem(s, TBLS * sizeof(list), -1);

	if (!cc->deft || !cc->strt)
		return NULL;

	cc->_ptr = s->_ptr;
	cc->_cnt = s->_cnt;

	//{ install Type

	type_vid = install(cc,  "void", TYPE_vid | symn_read, TYPE_vid, 0);

	type_bol = install(cc,  "bool", TYPE_bit | symn_read, TYPE_u32, 1);

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

	//~ type_arr = install(cc, "array", TYPE_ptr, 0, 0);
	//~ type_ptr = install(cc, "pointer", TYPE_ptr, 0, 0);
	
	//~ type_chr = installex(cc, "char", TYPE_bit, 1, NULL, NULL);
	//~ type_str = installex(cc, "string", TYPE_arr, 0, type_chr, NULL);

	instint(cc, type_i08, -1); instint(cc, type_u08, 0);
	instint(cc, type_i16, -1); instint(cc, type_u16, 0);
	instint(cc, type_i32, -1); instint(cc, type_u32, 0);
	instint(cc, type_i64, -1);// instint(cc, type_u64, 0);

	installex(cc, "int", TYPE_def, 0, type_i32, NULL);
	installex(cc, "long", TYPE_def, 0, type_i64, NULL);
	installex(cc, "float", TYPE_def, 0, type_f32, NULL);
	installex(cc, "double", TYPE_def, 0, type_f64, NULL);

	installex(cc, "null", TYPE_def, 0, type_vid, intnode(cc, 0));

	installex(cc, "true", TYPE_def, 0, type_bol, intnode(cc, 1));
	installex(cc, "false", TYPE_def, 0, type_bol, intnode(cc, 0));

	//} */// types
	//{ install Math
	enter(cc, def = install(cc, "math", TYPE_enu, 0, 0));

	//~ enter(cc, typ = install(c, TYPE_enu, "con", 0));
	installex(cc,  "nan", TYPE_def, 0, type_f64, fh8node(cc, 0x7FFFFFFFFFFFFFFFLL));	// Qnan
	installex(cc, "Snan", TYPE_def, 0, type_f64, fh8node(cc, 0xfff8000000000000LL));	// Snan
	installex(cc,  "inf", TYPE_def, 0, type_f64, fh8node(cc, 0x7ff0000000000000LL));
	installex(cc,  "l2e", TYPE_def, 0, type_f64, fh8node(cc, 0x3FF71547652B82FELL));	// log_2(e)
	installex(cc,  "l2t", TYPE_def, 0, type_f64, fh8node(cc, 0x400A934F0979A371LL));	// log_2(10)
	installex(cc,  "lg2", TYPE_def, 0, type_f64, fh8node(cc, 0x3FD34413509F79FFLL));	// log_10(2)
	installex(cc,  "ln2", TYPE_def, 0, type_f64, fh8node(cc, 0x3FE62E42FEFA39EFLL));	// log_e(2)
	installex(cc,   "pi", TYPE_def, 0, type_f64, fh8node(cc, 0x400921fb54442d18LL));		// 3.1415...
	installex(cc,    "e", TYPE_def, 0, type_f64, fh8node(cc, 0x4005bf0a8b145769LL));		// 2.7182...
	install(cc, "define isNan(flt64 x) = (x != x);", -1, 0, 0);
	install(cc, "define isNan(flt32 x) = (x != x);", -1, 0, 0);
	def->args = leave(cc, def);
	//} */
	install_emit(cc);
	libcall(s, NULL, "defaults");

	//~ (void)def;
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

	dieif(s->_cnt != cc->_cnt, "FixMe!(%d)", s->_cnt - cc->_cnt);
	//~ cc->_ptr = 0;
	cc->_cnt = 0;

	return s->errc;
}
/*int vmDone(state s) {
	// if not initialized
	if (s->vm == NULL)
		return 0;
	//TODO:...
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
void fputsymlst(FILE* fout, symn sym) {
	while (sym) {
		fputfmt(fout, "%+T\n", sym);
		sym = sym->next;
	}
}
void fputsymtbl(FILE* fout, ccEnv s) {
	int i;
	for (i = 0; i < TBLS; i++) {
		fputsymlst(fout, s->deft[i]);
	}
}
#endif

// lookup a value
symn findsym(ccEnv s, char *name) {
	struct astn ast;
	memset(&ast, 0, sizeof(struct astn));
	ast.kind = TYPE_ref;
	ast.id.name = name;
	return lookup(s, s->defs, &ast, NULL);
}
int findint(ccEnv s, char *name, int* res) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init)) {
		*res = constint(&ast);
		return 1;
	}
	return 0;
}
int findflt(ccEnv s, char *name, double* res) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init)) {
		*res = constflt(&ast);
		return 1;
	}
	return 0;
}
int lookup_nz(ccEnv s, char *name) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init)) {
		return constbol(&ast);
	}
	return 0;
}
//} */

int dbgInfo(vmEnv vm, int pu, void *ip, long* sptr, int sc) {
	if (ip == NULL) {
		if (vm->s && vm->s->cc)
			vmTags(vm->s->cc, (char*)sptr, sc);
		else if (!vm->s->cc) {
			debug("!s->cc");
		}
		else if (!vm->s) {
			debug("!vm->s");
		}
		vmInfo(stdout, vm);
		return 0;
	}
	return 0;
}
int dbgCon(vmEnv vm, int pu, void *ip, long* sptr, int sc) {
	static char buff[1024];
	static char cmd = 'N';
	char *arg;

	if (ip == NULL) {
		return dbgInfo(vm, pu, ip, sptr, sc);
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	fputfmt(stdout, ">exec:pu%02d@.%04x[ss:%03d]x[0x%016X]: %A\n", pu, ((char*)ip) - ((char*)vm->_mem), sc, *(int64t*)sptr, ip);

	if (cmd == 'N') return 0;

	for ( ; ; ) {
		if (fgets(buff, 1024, stdin) == NULL) {
			//~ chr = 'r'; // dont try next time to read
			return 0;
		}

		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0);		// no command, use last
		else if ((arg = parsecmd(buff, "print", " "))) {
			cmd = 'p';
		}
		else if ((arg = parsecmd(buff, "step", " "))) {
			if (!*arg) cmd = 'n';
			else if ((arg = parsecmd(buff, "over", " ")) && !*arg) {
				cmd = 'n';
			}
			else if ((arg = parsecmd(buff, "into", " ")) && !*arg) {
				cmd = 'n';
			}
		}
		else if ((arg = parsecmd(buff, "sp", " "))) {
			cmd = 's';
		}
		else if ((arg = parsecmd(buff, "st", " "))) {
			cmd = 'S';
		}
		else if (buff[1] == 0) {
			cmd = buff[0];
			arg = "";
		}
		else {
			debug("invalid command %s", buff);
			arg = "";
			cmd = 0;
		}
		if (!arg) arg = "";
		switch (cmd) {
			default:
				debug("invalid command '%c'", cmd);
			case 0 :
				break;

			case 'r' :		// resume
			//~ case 'c' :		// step in
			//~ case 'C' :		// step out
			case 'n' :		// step over
				return 0;
			case 'p' : if (vm->s && vm->s->cc) {		// print
				symn sym = findsym(vm->s->cc, arg);
				debug("arg:%T", sym);
				if (sym && sym->offs < 0) {
					int i = sc - sym->offs;
					stkval* sp = (stkval*)(sptr + i);
					fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp->i4, sp->f4, sp->i8, sp->f8);
				}
			} break;
			case 's' : {
				stkval* sp = (stkval*)sptr;
				//~ if (strcmp(arg, "all") == 0) fputfmt(stdout, "\tsp: {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
				if (strcmp(arg, "all") == 0);
				else if (strcmp(arg, "i32") == 0) fputfmt(stdout, "\tsp: i32(%d)\n", sp->i4);
				else if (strcmp(arg, "f32") == 0) fputfmt(stdout, "\tsp: f32(%d)\n", sp->i8);
				else if (strcmp(arg, "i64") == 0) fputfmt(stdout, "\tsp: i64(%d)\n", sp->f4);
				else if (strcmp(arg, "f64") == 0) fputfmt(stdout, "\tsp: f64(%d)\n", sp->f8);
				else fputfmt(stdout, "\tsp: {i32(%d), f32(%g), i64(%D), f64(%G)}\n", sp->i4, sp->f4, sp->i8, sp->f8);
			} break;
			case 'S' : {
				int i;
				for (i = 0; i < sc; i++) {
					stkval* sp = (stkval*)(sptr + i);
					//~ if (strcmp(arg, "all") == 0) fputfmt(stdout, "\tsp(%d): {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", i, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
					if (strcmp(arg, "") == 0);
					else if (strcmp(arg, "i32") == 0) fputfmt(stdout, "\tsp(%d): i32(%d)\n", i, sp->i4);
					else if (strcmp(arg, "f32") == 0) fputfmt(stdout, "\tsp(%d): f32(%d)\n", i, sp->i8);
					else if (strcmp(arg, "i64") == 0) fputfmt(stdout, "\tsp(%d): i64(%d)\n", i, sp->f4);
					else if (strcmp(arg, "f64") == 0) fputfmt(stdout, "\tsp(%d): f64(%d)\n", i, sp->f8);
					else fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp->i4, sp->f4, sp->i8, sp->f8);
				}
			} break;
		}
	}
	return 0;
}
