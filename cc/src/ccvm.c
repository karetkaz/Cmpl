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
	{0},	// hope it will fill whit 0
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
			default: fatal("NoIpHere");
			case TYPE_u32:
			case TYPE_i32:
			case TYPE_f32:
			case TYPE_i64:
			case TYPE_f64:
			case TYPE_p4x:
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

#define dbg_xml 15
#define dbg_arg(__AST) debug("cgen(arg, %+k)", __AST); dumpxml(stderr, __AST, 0, "debug", dbg_xml);
#define dbg_rhs(__AST) debug("cgen(rhs, %+k)", (__AST)->op.rhso); dumpxml(stderr, __AST, 0, "debug", dbg_xml);
#define dbg_lhs(__AST) debug("cgen(rhs, %+k)", (__AST)->op.lhso); dumpxml(stderr, __AST, 0, "debug", dbg_xml);
#define dbg_opc(__OPC, __AST) debug("emit(%02x, %+k, %t)", (__OPC), __AST, (__AST)->cast); dumpxml(stderr, __AST, 0, "debug", dbg_xml);

static int push(state s, astn ast) {
	struct astn tmp;
	int cast;

	if (!ast) {
		fatal("null ast");
		return 0;
	}

	cast = ast->cast;
	if (s->opti && eval(&tmp, ast, 0)) {
		tmp.cast = ast->cast;
		ast = &tmp;
	}

	//~ debug("push(arg, %+k, %t)", ast, cast);
	return cgen(s, ast, cast);
}

int cgen(state s, astn ast, int get) {
	int ipdbg = emit(s->vm, get_ip);
	struct astn tmp;
	int ret = 0;

	if (!ast) {
		if (get == TYPE_vid)
			return TYPE_vid;
		debug("null ast");
		return 0;
	}

	dieif(!ast->type, "untyped ast: %t(%+k)", ast->kind, ast);

	switch (ast->kind) {
		default: fatal("NoIpHere: %t(%k)", ast->kind, ast);
		//{ STMT; TODO: catch and print errors here
		case OPER_nop: {	// expr statement
			emit(s->vm, opc_line, ast->line);
			return cgen(s, ast->stmt.stmt, TYPE_vid);
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
			int tt = eval(&tmp, ast->stmt.test, TYPE_bit);

			dieif(get != TYPE_vid, "???");
			emitint(s->vm, opc_line, ast->line);

			if (ast->cast == QUAL_sta && (ast->stmt.step || !tt)) {
				error(s, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "unexpected else part");
				return 0;
			}
			if (tt && (s->opti || ast->cast == QUAL_sta)) {
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
				dieif(!emitidx(s->vm, opc_pop, stpos), "");
		} return TYPE_vid;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/call/cast
			int stargs = emit(s->vm, get_sp);
			astn argl = 0, argv = ast->op.rhso;
			symn var = linkOf(ast, 0);	// refs only
			symn typ = ast->type;

			ret = castId(typ);

			if (var && var->kind == TYPE_def) {
				debug("fun.def");

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
				}// */
				return cgen(s, var->init, get);

				}// */

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
				//~ debug("cast(%+k):%T", ast, opc);
				if (opc && opc->kind == EMIT_opc) {
					ret = castId(opc->type);
					if (ret == TYPE_vid) {
						// in case of nop 4ex
						//~ debug("emit(%+k): %T(%T)", ast->op.rhso, ast->type, opc);
						ret = get;
					}
					if (!emitint(s->vm, opc->offs, opc->init ? opc->init->con.cint : 0))
						debug("opcode expected, not %k : %T", argv, opc);
					// TODO: delme
					if (!get) get = ret;
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
				trace(0, "call(%+k):%t", ast, ast->cast);

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
						trace(0, "call.inl(%+k: %+T): %+T@", ast, var, typ);
						fixargs2(var, -stargs);
						//~ fixargs2(var, argl);

						if (!cgen(s, var->init, ret)) {
							debug("%+k:%+T", var->init, typ);
							return 0;
						}
						if (spbc <= emit(s->vm, get_sp)) {
							error(s, ast->line, "invalid stack size %+k", ast);
							dumpasmdbg(stderr, s->vm, ipdbg, 0x10);
							return 0;
						}
						switch (ret) {
							default: fatal("NoIpHere");
							case TYPE_u32:
							case TYPE_i32:
							case TYPE_f32: if (!emitidx(s->vm, opc_set1, stargs -= 1)) return 0; break;
							case TYPE_i64:
							case TYPE_f64: if (!emitidx(s->vm, opc_set2, stargs -= 2)) return 0; break;
							case TYPE_p4x: if (!emitidx(s->vm, opc_set4, stargs -= 4)) return 0; break;
						}
						if (!emitidx(s->vm, opc_pop, stargs))
							return 0;
					}
					else {
						if (var->offs < 0) {
							trace(0, "call.lib(%+k: %+T): %+T@", ast, var, typ);
							ret = castId(typ);
							if (!emit(s->vm, opc_libc, -var->offs)) {
								debug("%+k", ast);
								return 0;
							}
						}
						else {
							trace(0, "call.fun(%+k: %+T): %+T@", ast, var, typ);
							ret = castId(typ);
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

				if (!argv || argv != ast->op.rhso /* && cast*/)
					warn(s, 5, s->cc->file, ast->line, "multiple values: '%+k'", ast);

				if (!(ret = push(s, argv))) {
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
			if (!emit(s->vm, opc_ldc4, ast->type->size)) {
				dbg_opc(opc_ldc4, ast);
				return 0;
			}
			if (!cgen(s, ast->op.rhso, TYPE_u32)) {
				dbg_rhs(ast);
				return 0;
			}

			if (!emit(s->vm, u32_mad)) {
				debug("emit(opc_x%02x, %+k, %t)", u32_mad, ast, ast->cast);
				return 0;
			}
			//~ if (!emit(s->vm, i32_mul)) return 0;
			//~ if (!emit(s->vm, i32_add)) return 0;

			/* ? if (!ast->type->init) {		// indirect: []
				emit(s->vm, opc_ldi4);
			}// */
			if ((ret = get) != TYPE_ref) {
				ret = castId(ast->type);
				if (!emit(s->vm, opc_ldi, ast->type->size)) {
					debug("emit(opc_x%02x, %t, %+k, %t)", opc_ldi, ret, ast, ast->cast);
					return 0;
				}
			}
		} break; // */
		case OPER_dot: {
			symn var = ast->op.rhso && ast->op.rhso->kind == TYPE_ref ? ast->op.rhso->id.link : 0;
			if (istype(ast->op.lhso)) {
				return cgen(s, ast->op.rhso, get);
			}

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				dbg_lhs(ast);
				return 0;
			}
			if (!var || var->kind != TYPE_ref || var->offs < 0) {
				debug("%d", var->offs);
				dbg_rhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc_ldc4, var->offs)) {
				dbg_opc(opc_ldc4, ast);
				return 0;
			}
			if (!emit(s->vm, i32_add)) {
				dbg_opc(opc_ldc4, ast);
				return 0;
			}
			if (get == TYPE_ref)
				return TYPE_ref;
			fatal("unimplemented");
			return 0;
			
			/*if (!cgen(s, ast->op.rhso, get)) {
				debug("%+k", ast);
				return 0;
			}
			ret = get;
			//~ */
		} break;

		case OPER_not:		// '!'	dieif(ast->cast != TYPE_bit);
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			int opc = -1;
			switch (ast->kind) {
				default: fatal("NoIpHere");
				case OPER_pls: return cgen(s, ast->op.rhso, get);
				case OPER_mns: opc = opc_neg; break;
				case OPER_not: opc = opc_not; break;
				//~ case OPER_cmt: opc = opc_cmt; break;
			}
			ret = castId(ast->type);
			if (!cgen(s, ast->op.rhso, ast->cast)) {
				dbg_rhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc, ast->cast)) {
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
				default: fatal("NoIpHere");
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
			if (!cgen(s, ast->op.lhso, ast->cast)) {
				dbg_lhs(ast);
				return 0;
			}
			if (!cgen(s, ast->op.rhso, ast->cast)) {
				dbg_rhs(ast);
				return 0;
			}
			if (!emit(s->vm, opc, ast->cast)) {
				dbg_opc(opc, ast);
				return 0;
			}
			switch (ast->kind) {
				default:
					ret = ast->cast;
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
			//~ ret = castId(ast->type);
			//~ debug("OP(%+k: %t, %t): %t", ast, ast->cast, get, ret);
		} break;

		//~ case OPER_lnd:		// &&
		//~ case OPER_lor:		// ||
		case OPER_sel: {		// ?:
			int jmpt, jmpf;
			int tt = eval(&tmp, ast->op.test, TYPE_bit);
			if (s->opti && tt) {
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

				if (!cgen(s, ast->op.lhso, ast->cast)) {
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
				if (!cgen(s, ast->op.rhso, ast->cast)) {
					debug("cgen(%+k)", ast);
					return 0;
				}
				fixjump(s->vm, jmpf, emit(s->vm, get_ip), 0);
			}
			ret = ast->cast;
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
					default: fatal("NoIpHere");
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
				//~ dieif(var && var->load, "unimplemented");
				if (var->offs < 0) {
					switch (ast->type->size) {
						default: fatal("NoIpHere");
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
			dieif(!var, "what the fuck ?");

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
						default: fatal("NoIpHere: (%t, %t)%+k:%T", get, ret, ast, typ);
						case TYPE_u32:
						case TYPE_i32:
						case TYPE_f32: if (!emitidx(s->vm, opc_dup1, var->offs)) return 0; break;
						case TYPE_i64:
						case TYPE_f64: if (!emitidx(s->vm, opc_dup2, var->offs)) return 0; break;
						case TYPE_p4x: if (!emitidx(s->vm, opc_dup4, var->offs)) return 0; break;
					}
				}
				else {	// in memory
					int size = typ->size;
					int offs = var->offs;
					while (size > 0) {
						int len = size;
						int opc = -1;

						if (size > 8) {
							opc = opc_ldiq;
							len = 16;
						}
						else if (size > 4) {
							opc = opc_ldi8;
							len = 8;
						}
						else if (size > 2) {
							opc = opc_ldi4;
							len = 4;
						}
						else switch (size) {
							case 2: opc = opc_ldi2; break;
							case 1: opc = opc_ldi1; break;
						}

						if (!emitint(s->vm, opc_ldcr, offs))
							return 0;
						if (!emit(s->vm, opc))
							return 0;

						size -= len;
						offs += len;
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
			//~ debug("define %k", ast);
			dieif(!var, "what the fuck ?");

			if (var->call) {
				//~ fixargs(var, NULL);
				// make an assertion that if not inline fun, then offset != 0
				//~ debug("function: %+k", ast);
				return TYPE_vid;
			}// */
			if (var->kind == TYPE_ref) {
				astn val = var->init;
				if (s->opti && eval(&tmp, val, 0)) {
					val = &tmp;
				}
				//~ debug("cgen(var): %+k:%T", ast, ast->type);
				switch (ret = castId(typ)) {
					default: fatal("NoIpHere");
					case TYPE_u32:
					case TYPE_int:
					case TYPE_i32:
					case TYPE_f32: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_ldz1))) {
							debug("emit(%+k)", val);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
					} break;
					case TYPE_f64:
					case TYPE_i64: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_ldz2))) {
							debug("emit(%+k)", ast);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
					} break;
					case TYPE_p4x: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_ldz4))) {
							debug("emit(%+k)", ast);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
					} break;
					case TYPE_rec: {
						if (!(val ? cgen(s, val, ret) : emit(s->vm, opc_loc, typ->size))) {
							debug("emit(%+k)", ast);
							return 0;
						}
						var->offs = emit(s->vm, get_sp);
						if (-4 * var->offs < typ->size) {
							error(s, ast->line, "stack overflow", -4*var->offs, typ->size);
							return 0;
						}
					} break;//*/
				}
				return ret;
			}

		} break;

		case TYPE_rec:
		case TYPE_enu: break;
		//}
	}

	//~ if (get != ret) switch (ast->type->kind) {
	//~ ret = ast->type->kind;
	//~ debug("gen(%t, %t, %t, '%+k')", get, castId(ast->type), ret, ast);
	if (get != ret) switch (get) {
		case TYPE_vid: return TYPE_vid;
		case TYPE_u32: switch (ret) {
			case TYPE_bit:
			case TYPE_i32: break;
			case TYPE_i64: if (!emit(s->vm, i64_i32)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_i32)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_i32)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i32: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64: if (!emit(s->vm, i64_i32)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_i32)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_i32)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: if (!emit(s->vm, i32_i64)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_i64)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_i64)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: if (!emit(s->vm, i32_f32)) return 0; break;
			case TYPE_i64: if (!emit(s->vm, i64_f32)) return 0; break;
			case TYPE_f64: if (!emit(s->vm, f64_f32)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: if (!emit(s->vm, i32_f64)) return 0; break;
			case TYPE_i64: if (!emit(s->vm, i64_f64)) return 0; break;
			case TYPE_f32: if (!emit(s->vm, f32_f64)) return 0; break;
			//~ case TYPE_ref: if (!emit(s->vm, opc_ldi4)) return 0; break;
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
		//~ TODO: case TYPE_ref: 			// address of?
		default: fatal("unimplemented(`%+k`, %t):%t", ast, get, ret);
		errorcast2: debug("invalid cast: [%d->%d](%t to %t) %k: '%+k'", ret, get, ret, get, ast, ast);
			return 0;
	}

	return ret;
}

int logfile(state s, char* file) {
	if (s->logf)
		fclose(s->logf);
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
	if (scan(s->cc, -1) != 0)
		return -2;

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

	s->opti = level;
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
			//~ cgen(s, c->root, TYPE_any);	// TODO: TYPE_vid: clear the stack
			continue;
		}
		//~ TODO: alloc and other stuff
	}
	// */

	//~ header:
	//~ seg:Text ro
	//~ seg:Data rw
	//~ seg:Code rx
	//~ seg:dbg  ro(s->cc->defs || all)
	//~ seg:doc  ro

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

static void instint(ccEnv s, symn it, int64t sgn) {
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
static void install_emit(ccEnv c) {
	int i, TYPE_opc = EMIT_opc;
	symn typ,tyq,*tyb;//,tyd;
	static char *TypeList[]={"u32","i32","f32", "i64", "f64", "v4f", "v2d",  NULL};
	symn TypeNode[] = {type_u32, type_i32, type_f32, type_i64, type_f64, type_f32x4, type_f64x2 };
	char **Type;

	emit_opc = install(c, TYPE_opc, "emit", 0);
	//~ emit_opc->call = 1;
	//~ emit_opc->type = 0;
	enter(c, NULL);

	installex(c, TYPE_opc, "nop", opc_nop, type_vid, NULL);

	enter(c, typ = install(c, TYPE_int, "u32", 4));
	installex(c, TYPE_opc, "cmt", b32_cmt, type_u32, NULL);
	//~ installex(c, TYPE_opc, "adc", b32_adc, type_u32, NULL);
	//~ installex(c, TYPE_opc, "sub", b32_sbb, type_u32, NULL);
	installex(c, TYPE_opc, "mul", u32_mul, type_u32, NULL);
	installex(c, TYPE_opc, "div", u32_div, type_u32, NULL);
	installex(c, TYPE_opc, "mad", u32_mad, type_u32, NULL);
	installex(c, TYPE_opc, "clt", u32_clt, type_bol, NULL);
	installex(c, TYPE_opc, "cgt", u32_cgt, type_bol, NULL);
	installex(c, TYPE_opc, "and", b32_and, type_u32, NULL);
	installex(c, TYPE_opc,  "or", b32_ior, type_u32, NULL);
	installex(c, TYPE_opc, "xor", b32_xor, type_u32, NULL);
	installex(c, TYPE_opc, "shl", b32_shl, type_u32, NULL);
	installex(c, TYPE_opc, "shr", b32_shr, type_u32, NULL);
	installex(c, TYPE_opc, "sar", b32_sar, type_u32, NULL);

	typ->args = leave(c, typ);

	enter(c, typ = install(c, TYPE_int, "i32", 4));
	installex(c, TYPE_opc, "cmt", b32_cmt, type_i32, NULL);
	installex(c, TYPE_opc, "neg", i32_neg, type_i32, NULL);
	installex(c, TYPE_opc, "add", i32_add, type_i32, NULL);
	installex(c, TYPE_opc, "sub", i32_sub, type_i32, NULL);
	installex(c, TYPE_opc, "mul", i32_mul, type_i32, NULL);
	installex(c, TYPE_opc, "div", i32_div, type_i32, NULL);
	installex(c, TYPE_opc, "mod", i32_mod, type_i32, NULL);
	
	installex(c, TYPE_opc, "cvt2f32", i32_f32, type_f32, NULL);

	installex(c, TYPE_opc, "ceq", i32_ceq, type_bol, NULL);
	installex(c, TYPE_opc, "clt", i32_clt, type_bol, NULL);
	installex(c, TYPE_opc, "cgt", i32_cgt, type_bol, NULL);

	installex(c, TYPE_opc, "and", b32_and, type_i32, NULL);
	installex(c, TYPE_opc,  "or", b32_ior, type_i32, NULL);
	installex(c, TYPE_opc, "xor", b32_xor, type_i32, NULL);
	installex(c, TYPE_opc, "shl", b32_shl, type_i32, NULL);
	installex(c, TYPE_opc, "shr", b32_shr, type_i32, NULL);
	installex(c, TYPE_opc, "sar", b32_sar, type_i32, NULL);
	typ->args = leave(c, typ);

	enter(c, typ = install(c, TYPE_int, "i64", 8));
	installex(c, TYPE_opc, "neg", i64_neg, type_i64, NULL);
	installex(c, TYPE_opc, "add", i64_add, type_i64, NULL);
	installex(c, TYPE_opc, "sub", i64_sub, type_i64, NULL);
	installex(c, TYPE_opc, "mul", i64_mul, type_i64, NULL);
	installex(c, TYPE_opc, "div", i64_div, type_i64, NULL);
	installex(c, TYPE_opc, "mod", i64_mod, type_i64, NULL);
	typ->args = leave(c, typ);

	enter(c, typ = install(c, TYPE_flt, "f32", 4));
	installex(c, TYPE_opc, "neg", f32_neg, type_f32, NULL);
	installex(c, TYPE_opc, "add", f32_add, type_f32, NULL);
	installex(c, TYPE_opc, "sub", f32_sub, type_f32, NULL);
	installex(c, TYPE_opc, "mul", f32_mul, type_f32, NULL);
	installex(c, TYPE_opc, "div", f32_div, type_f32, NULL);
	installex(c, TYPE_opc, "mod", f32_mod, type_f32, NULL);
	typ->args = leave(c, typ);

	enter(c, typ = install(c, TYPE_flt, "f64", 8));
	installex(c, TYPE_opc, "neg", f64_neg, type_f64, NULL);
	installex(c, TYPE_opc, "add", f64_add, type_f64, NULL);
	installex(c, TYPE_opc, "sub", f64_sub, type_f64, NULL);
	installex(c, TYPE_opc, "mul", f64_mul, type_f64, NULL);
	installex(c, TYPE_opc, "div", f64_div, type_f64, NULL);
	installex(c, TYPE_opc, "mod", f64_mod, type_f64, NULL);
	typ->args = leave(c, typ);

	enter(c, typ = install(c, TYPE_p4x, "v4f", 16));
	installex(c, TYPE_opc, "neg", v4f_neg, type_f32x4, NULL);
	installex(c, TYPE_opc, "add", v4f_add, type_f32x4, NULL);
	installex(c, TYPE_opc, "sub", v4f_sub, type_f32x4, NULL);
	installex(c, TYPE_opc, "mul", v4f_mul, type_f32x4, NULL);
	installex(c, TYPE_opc, "div", v4f_div, type_f32x4, NULL);
	installex(c, TYPE_opc, "dp3", v4f_dp3, type_f32, NULL);
	installex(c, TYPE_opc, "dp4", v4f_dp4, type_f32, NULL);
	installex(c, TYPE_opc, "dph", v4f_dph, type_f32, NULL);
	typ->args = leave(c, typ);

	enter(c, typ = install(c, TYPE_p4x, "v2d", 16));
	installex(c, TYPE_opc, "neg", v2d_neg, type_f64x2, NULL);
	installex(c, TYPE_opc, "add", v2d_add, type_f64x2, NULL);
	installex(c, TYPE_opc, "sub", v2d_sub, type_f64x2, NULL);
	installex(c, TYPE_opc, "mul", v2d_mul, type_f64x2, NULL);
	installex(c, TYPE_opc, "div", v2d_div, type_f64x2, NULL);
	typ->args = leave(c, typ);


	enter(c, typ = install(c, TYPE_enu, "dup", 0)); {
		for(Type = TypeList, tyb = TypeNode; *Type; ++Type, ++tyb) {
			enter(c, tyq = install(c, TYPE_enu, *Type, 0));
			{
				int i, opc_dupx;
				switch((*tyb)->size) {
				case 4:
					opc_dupx = opc_dup1;
					break;
				case 8:
					opc_dupx = opc_dup2;
					break;
				case 16:
					opc_dupx = opc_dup4;
					break;
				default:
					fprintf(stderr, "%s: %d bytes, ;-)", *Type, (*tyb)->size);
					abort();
				}
				for(i=0;i<256;++i) {
					char str[6]; 
					sprintf(str, "_%d",i);
					installex(c, TYPE_opc, 
						mapstr(c, str, -1, -1), 
						opc_dupx , *tyb, intnode(c,i)
					);
				}
			}
			tyq->args = leave(c, tyq);
		}
	} typ->args = leave(c, typ);

	/*{
		struct {
			char *name;
			//~ char *swz, *msk;	// swizzle and mask
			astn node;
		} swz[256];
		for (i = 0; i < 256; i += 1) {
			c->buffp[0] = "xyzw"[i>>0&3];
			c->buffp[1] = "xyzw"[i>>2&3];
			c->buffp[2] = "xyzw"[i>>4&3];
			c->buffp[3] = "xyzw"[i>>6&3];
			c->buffp[4] = 0;

			//~ c->buffp[5] = '_';		// mask
			//~ c->buffp[6] = "_01x"[i>>0&3];
			//~ c->buffp[7] = "_01y"[i>>2&3];
			//~ c->buffp[8] = "_01z"[i>>4&3];
			//~ c->buffp[9] = "_01w"[i>>6&3];
			//~ c->buffp[10] = 0;

			swz[i].name = mapstr(c, c->buffp, 4, -1);
			swz[i].node = intnode(c, i);
		}

		enter(c, NULL);
		for (i = 0; i < 256; i += 1)
			//~ installex(c, TYPE_opc, swz, 0x96, type_f32x4, intnode(c, i));
			installex(c, TYPE_opc, swz[i].name, p4d_swz, type_f32x4, swz[i].node);
		typ = leave(c);
		install(c, TYPE_enu, "swz", 0)->args = typ;

		//~ extended set(masked) and dup(swizzle): p4d.dup.xyxy / p4d.set.xyz0
		enter(c, NULL);
		for (i = 0; i < 256; i += 1)
			installex(c, TYPE_opc, swz[i].name, 0x1d, type_f32x4, swz[i].node);
		typ = leave(c);
		install(c, TYPE_enu, "dup", 0)->args = typ;

		enter(c, NULL);
		// here should be used other names
		installex(c, TYPE_opc, "x", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "y", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "z", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "w", 0x1e, type_f32x4, intnode(c, 0));

		installex(c, TYPE_opc, "xy", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "xyz", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "xzw", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "xyw", 0x1e, type_f32x4, intnode(c, 0));

		installex(c, TYPE_opc, "xyzw", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "xyz0", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "xyz1", 0x1e, type_f32x4, intnode(c, 0));
		installex(c, TYPE_opc, "xyz_", 0x1e, type_f32x4, intnode(c, 0));

		typ = leave(c);
		install(c, TYPE_enu, "set", 0)->args = typ;
	} //~ */
	/*	// this is huge (37809-16009)
	enter(c, NULL);
	for (i = 0; i < 256; i += 1) {
		char *swz = c->_ptr;
		dieif(c->_cnt < 5, "memory overrun");
		swz[0] = "xyzw"[i >> 0 & 3];
		swz[1] = "xyzw"[i >> 2 & 3];
		swz[2] = "xyzw"[i >> 4 & 3];
		swz[3] = "xyzw"[i >> 6 & 3];
		swz[4] = 0;
		swz = mapstr(c, swz, 4, -1);
		installex(c, TYPE_opc, swz, 0x96, type_f32x4, intnode(c, i));
	}
	typ = leave(c);
	install(c, TYPE_enu, "swz", 0)->args = typ;
	// */
	emit_opc->args = leave(c, emit_opc);
	i = 0;
	//~ return emit_opc;
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
	ccEnv c = getmem(s, sizeof(struct ccEnv), -1);
	symn def;//, type_chr = 0;
	symn type_i08 = 0, type_i16 = 0;
	symn type_u08 = 0, type_u16 = 0;
	//~ symn type_u64 = 0, type_f16 = 0;

	if (c == NULL)
		return NULL;

	s->cc = c;
	c->s = s;

	s->errc = 0;
	s->logf = 0;

	//~ s->warn = wl;
	//~ s->opti = ol;

	c->file = 0; c->line = c->nest = 0;

	c->fin._fin = -1;
	c->fin._ptr = 0;
	c->fin._cnt = 0;

	c->root = 0;
	c->defs = 0;

	c->_chr = -1;
	c->_tok = 0;

	c->tokp = 0;
	c->all = 0;

	c->deft = getmem(s, TBLS * sizeof(symn), -1);
	c->strt = getmem(s, TBLS * sizeof(list), -1);

	if (!c->deft || !c->strt)
		return NULL;

	c->_ptr = s->_ptr;
	c->_cnt = s->_cnt;

	//{ install Type

	type_vid = install(c, TYPE_vid | symn_read, "void",  0);

	type_bol = install(c, TYPE_bit | symn_read, "bool",  1);

	type_u08 = install(c, TYPE_bit | symn_read, "uns8", 1);
	type_u16 = install(c, TYPE_bit | symn_read, "uns16", 2);
	type_u32 = install(c, TYPE_bit | symn_read, "uns32", 4);
	//~ type_u64 = install(c, TYPE_uns, "uns64", 8);

	type_i08 = install(c, TYPE_int | symn_read, "int8", 1);
	type_i16 = install(c, TYPE_int | symn_read, "int16", 2);
	type_i32 = install(c, TYPE_int | symn_read, "int32", 4);
	type_i64 = install(c, TYPE_int | symn_read, "int64", 8);

	//~ type_f16 = install(c, TYPE_flt | symn_read, "flt16", 2);
	type_f32 = install(c, TYPE_flt | symn_read, "flt32", 4);
	type_f64 = install(c, TYPE_flt | symn_read, "flt64", 8);

	//~ type = install(c, TYPE_p4x, "i8x16", 16);
	//~ type = install(c, TYPE_p4x, "i16x8", 16);
	//~ type = install(c, TYPE_p4x, "i32x4", 16);
	//~ type = install(c, TYPE_p4x, "i64x2", 16);
	//~ type = install(c, TYPE_p4x, "u8x16", 16);
	//~ type = install(c, TYPE_p4x, "u16x8", 16);
	//~ type = install(c, TYPE_p4x, "u32x4", 16);
	//~ type = install(c, TYPE_p4x, "u64x2", 16);
	//~ type = install(c, TYPE_p4x, "f16x8", 16);
	type_f32x4 = install(c, TYPE_p4x | symn_read, "f32x4", 16);
	type_f64x2 = install(c, TYPE_p4x | symn_read, "f64x2", 16);

	//~ type_arr = install(c, TYPE_ptr, "array", 0);
	//~ type_ptr = install(c, TYPE_ptr, "pointer", 0);
	
	//~ type_chr = installex(c, TYPE_bit, "char", 1, NULL, NULL);
	//~ type_str = installex(c, TYPE_arr, "string", 0, type_chr, NULL);

	instint(c, type_i08, -1); instint(c, type_u08, 0);
	instint(c, type_i16, -1); instint(c, type_u16, 0);
	instint(c, type_i32, -1); instint(c, type_u32, 0);
	instint(c, type_i64, -1);// instint(c, type_u64, 0);

	installex(c, TYPE_def, "int", 0, type_i32, NULL);
	installex(c, TYPE_def, "long", 0, type_i64, NULL);
	installex(c, TYPE_def, "float", 0, type_f32, NULL);
	installex(c, TYPE_def, "double", 0, type_f64, NULL);

	installex(c, TYPE_def, "true", 0, type_bol, intnode(c, 1));
	installex(c, TYPE_def, "false", 0, type_bol, intnode(c, 0));

	//} */// types
	/*/{ install Math
	enter(c, def = install(c, TYPE_enu, "math", 0));

	//~ enter(c, typ = install(c, TYPE_enu, "con", 0));
	installex(c, TYPE_def,  "nan", 0, type_f64, fh8node(c, 0x7FFFFFFFFFFFFFFFLL));	// Qnan
	installex(c, TYPE_def, "Snan", 0, type_f64, fh8node(c, 0xfff8000000000000LL));	// Snan
	installex(c, TYPE_def,  "inf", 0, type_f64, fh8node(c, 0x7ff0000000000000LL));
	installex(c, TYPE_def,  "l2e", 0, type_f64, fh8node(c, 0x3FF71547652B82FELL));	// log_2(e)
	installex(c, TYPE_def,  "l2t", 0, type_f64, fh8node(c, 0x400A934F0979A371LL));	// log_2(10)
	installex(c, TYPE_def,  "lg2", 0, type_f64, fh8node(c, 0x3FD34413509F79FFLL));	// log_10(2)
	installex(c, TYPE_def,  "ln2", 0, type_f64, fh8node(c, 0x3FE62E42FEFA39EFLL));	// log_e(2)
	installex(c, TYPE_def,   "pi", 0, type_f64, fh8node(c, 0x400921fb54442d18LL));		// 3.1415...
	installex(c, TYPE_def,    "e", 0, type_f64, fh8node(c, 0x4005bf0a8b145769LL));		// 2.7182...
	//~ install(c, -1, "bool isNan(flt64 x) = (x != x);", 0);
	//~ install(c, -1, "bool isNan(flt32 x) = (x != x);", 0);
	install(c, -1, "define isNan(flt64 x) = bool(x != x);", 0);
	install(c, -1, "define isNan(flt32 x) = bool(x != x);", 0);
	def->args = leave(c, def);
	//} */
	install_emit(s->cc);
	install_libc(s, NULL, "defaults");

	/*/{ install ccon
	enter(c, typ = install(c, TYPE_enu, "compiler", 0));
	//~ install(c, CNST_int, "version", 0)->init = intnode(s, 0x20091218);
	//~ install(c, CNST_str, "host", 0)->init = strnode(s, (char*)os);
	//- install(c, CNST_str, "type", 0);// current type;
	//- install(c, CNST_str, "defn", 0);// current type;
	//~ install(c, CNST_str, "date", 0)->init = &c->ccd;
	//~ install(c, CNST_str, "file", 0)->init = &c->ccfn;	// compiling file
	//~ install(c, TYPE_def, "line", 0)->init = &c->ccfl;	// compiling line
	//~ install(c, CNST_str, "time", 0)->init = &c->ccft;	// compiling time

	typ->args = leave(c);
	//} */

	(void)def;
	return s->cc;
}
vmEnv vmInit(state s) {
	int size = s->_cnt;
	vmEnv vm = getmem(s, size, -1);
	if (vm != NULL) {
		vm->s = s;
		//~ vm->ds = vm->ic = 0;
		vm->cs = vm->pc = 0;
		vm->ss = vm->sm = 0;
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
	// if not initialized
	if (s->cc == NULL)
		return -1;

	//~ if (s->cc->nest)
		//~ error(s->cc, s->cc->line, "premature end of file");

	//~ s->cc->nest = 0;

	// close input
	source(s->cc, 0, 0);

	// set used memory
	s->_cnt -= s->cc->_ptr - s->_ptr;
	s->_ptr = s->cc->_ptr;

	dieif (s->_cnt != s->cc->_cnt, "??? %d", s->_cnt - s->cc->_cnt);
	//~ s->cc->_ptr = 0;
	s->cc->_cnt = 0;

	return s->errc;
}
int vmDone(state s) {
	// if not initialized
	if (s->vm == NULL)
		return 0;
	//TODO:...
	debug("code:%d Bytes", (s->_ptr - s->_mem) + s->vm->_ptr);
	return 0;
}

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
	//~ ast.id.hash = rehash(name, strlen(name));
	return lookup(s, s->defs, &ast, NULL);
}
int findint(ccEnv s, char *name, int* res) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init, TYPE_int)) {
		*res = ast.con.cint;
		return 1;
	}
	return 0;
}
int findflt(ccEnv s, char *name, double* res) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init, TYPE_flt)) {
		*res = ast.con.cflt;
		return 1;
	}
	return 0;
}
int lookup_nz(ccEnv s, char *name) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init, TYPE_bit)) {
		return ast.con.cint;
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
