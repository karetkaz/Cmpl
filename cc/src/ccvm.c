#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "ccvm.h"

#ifdef __WATCOMC__
//~ Warning! W124: Comparison result always 0
//~ Warning! W201: Unreachable code
//~ #pragma disable_message(124);
#pragma disable_message(201);
#endif
const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, STR) {TYPE, SIZE, STR},
	#include "incl/defs.h"
	{0},
};
const opc_inf opc_tbl[255] = {
	//#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem)
	#define OPCDEF(NAME, CODE, SIZE, CHCK, DIFF, TIME, MNEM) {CODE, SIZE, CHCK, DIFF, MNEM},
	#include "incl/defs.h"
	{0},
};

static inline int genRef(state s, int offset) {
	if (offset > 0)
		return emitidx(s, opc_ldsp, offset);
	return emitint(s, opc_ldcr, -offset);
} // */
static inline int genVal(state s, astn ast) {
	int cgen(state s, astn ast, int get);
	struct astn tmp;
	int cast;

	if (!ast) {
		fatal("null ast");
		return 0;
	}

	cast = ast->cst2;

	if (s->vm.opti && eval(&tmp, ast)) {
		ast = &tmp;
	}

	return cgen(s, ast, cast);
}
int cgen(state s, astn ast, int get) {
	#if DEBUGGING > 1
	int ipdbg = emitopc(s, markIP);
	#endif
	struct astn tmp;
	int ret = 0;
	int qual = 0;

	if (!ast || !ast->type) {
		dieif(!ast || !ast->type, "FixMe %k", ast);
	}

	// TODO: this sucks
	if (get == TYPE_any)
		get = ast->cst2;

	if (!(ret = ast->type->cast))
		ret = ast->type->kind;

	//~ debug("%+k", ast);

	if (ast->kind >= STMT_beg && ast->kind <= STMT_end) {
		qual = ast->cst2;
	}

	switch (ast->kind) {
		default:
			fatal("FixMe");
			return 0;

		//{ STMT
		case STMT_do:  {	// expr statement or decl or function body
			int stpos = stkoffs(s, 0);
			emitint(s, opc_line, ast->line);
			if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}
			if (stpos != stkoffs(s, 0)) {
				if (stpos > stkoffs(s, 0)) {
					warn(s, 1, s->cc->file, ast->line, "statement underflows stack: %+k", ast->stmt.stmt);
				}
				/*if (get == TYPE_vid && !emitidx(s->vm, opc_drop, stpos)) {
					trace("error");
					return 0;
				}// */
			}
			#if 0 && DEBUGGING > 1
			debug("cgen[%t->%t](err, %+k)\n%7K", get, ret, ast, ast);
			fputasm(stdout, s->vm, ipdbg, -1, 0x119);
			#endif
		} break;
		case STMT_beg: {	// {}
			astn ptr;
			int stpos = stkoffs(s, 0);
			emitint(s, opc_line, ast->line);
			for (ptr = ast->stmt.stmt; ptr; ptr = ptr->next) {
				#if DEBUGGING > 1
				ipdbg = emitopc(s, markIP);
				#endif
				if (!cgen(s, ptr, TYPE_vid)) {		// we will free stack on scope close
					error(s, ptr->line, "emmiting statement `%+k`", ptr);
					#if DEBUGGING > 1
					debug("cgen[%t->%t](err, %+k)\n%7K", get, ret, ptr, ptr);
					fputasm(stdout, s, ipdbg, -1, 0x119);
					#endif
				}
			}
			//~ for (sym = ast->type; sym; sym = sym->next);	// destruct?
			if (get == TYPE_vid && stpos != stkoffs(s, 0)) {
				if (!emitidx(s, opc_drop, stpos)) {
					trace("error");
					return 0;
				}
			}
		} break;
		case STMT_if:  {
			int jmpt = 0, jmpf = 0;
			int tt = eval(&tmp, ast->stmt.test);

			//~ dieif(get != TYPE_any && get != TYPE_vid, "FixMe");
			emitint(s, opc_line, ast->line);

			if (qual == QUAL_sta) {
				//~ debug("static if: `%k`: ", ast);
				if (ast->stmt.step || !tt) {
					error(s, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "else part is invalid");
					return 0;
				}
				qual = 0;
			}

			if (tt && (s->vm.opti || ast->cst2 == QUAL_sta)) {	// static if then else
				astn gen = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
				if (gen && !cgen(s, gen, TYPE_any)) {	// leave the stack
					trace("%+k", gen);
					return 0;
				}
			}
			else if (ast->stmt.stmt && ast->stmt.step) {		// if then else
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				if (!(jmpt = emitopc(s, opc_jz))) {
					trace("%+k", ast);
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				if (!(jmpf = emitopc(s, opc_jmp))) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(s, jmpt, emitopc(s, markIP), 0);
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(s, jmpf, emitopc(s, markIP), 0);
			}
			else if (ast->stmt.stmt) {							// if then
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				//~ if false skip THEN block
				if (!(jmpt = emitopc(s, opc_jz))) {
					trace("%+k", ast);
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(s, jmpt, emitopc(s, markIP), 0);
			}
			else if (ast->stmt.step) {							// if else
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				//~ if true skip ELSE block
				if (!(jmpt = emitopc(s, opc_jnz))) {
					trace("%+k", ast);
					return 0;
				}
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(s, jmpt, emitopc(s, markIP), 0);
			}
		} break;
		case STMT_for: {
			astn jl = s->cc->jmps;
			int jstep, lcont, lbody, lbreak;
			int stbreak, stpos = stkoffs(s, 0);

			dieif(get != TYPE_vid, "FixMe");
			emitint(s, opc_line, ast->line);
			if (ast->stmt.init && !cgen(s, ast->stmt.init, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}

			if (!(jstep = emitint(s, opc_jmp, 0))) {		// continue;
				trace("%+k", ast);
				return 0;
			}

			lbody = emitopc(s, markIP);
			if (ast->stmt.stmt && !cgen(s, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}

			lcont = emitopc(s, markIP);
			if (ast->stmt.step && !cgen(s, ast->stmt.step, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}

			fixjump(s, jstep, emitopc(s, markIP), 0);
			if (ast->stmt.test) {
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				if (!emitint(s, opc_jnz, lbody)) {		// continue;
					trace("%+k", ast);
					return 0;
				}
			}
			else {
				if (!emitint(s, opc_jmp, lbody)) {		// continue;
					trace("%+k", ast);
					return 0;
				}
			}

			// TODO: fix all breaks here
			lbreak = emitopc(s, markIP);
			stbreak = stkoffs(s, 0);

			while (s->cc->jmps != jl) {
				astn jmp = s->cc->jmps;
				s->cc->jmps = jmp->next;

				if (jmp->go2.stks != stbreak) {
					error(s, jmp->line, "`%k` statement is invalid due to previous variable declaration within loop", jmp);
					return 0;
				}

				switch (jmp->kind) {
					default : error(s, jmp->line, "invalid goto statement: %k", jmp); return 0;
					case STMT_brk: fixjump(s, jmp->go2.offs, lbreak, jmp->go2.stks); break;
					case STMT_con: fixjump(s, jmp->go2.offs, lcont, jmp->go2.stks); break;
				}
			}

			//! TODO: if (init is decl) destruct;
			if (stpos != stkoffs(s, 0)) {
				dieif(!emitidx(s, opc_drop, stpos), "FixMe");
			}
		} break;
		case STMT_con:
		case STMT_brk: {
			int offs;
			dieif(get != TYPE_vid, "FixMe");
			emitint(s, opc_line, ast->line);
			if (!(offs = emitint(s, opc_jmp, 0))) {
				trace("%+k", ast);
				return 0;
			}

			ast->go2.offs = offs;
			ast->go2.stks = stkoffs(s, 0);

			ast->next = s->cc->jmps;
			s->cc->jmps = ast;

		} break;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/call/cast
			int stkret = stkoffs(s, sizeOf(ast->type));
			astn argv = ast->op.rhso;
			symn var = linkOf(ast->op.lhso);

			dieif(!var && ast->op.lhso, "FixMe[%+k]", ast);

			// inline
			if (var && var->kind == TYPE_def) {
				symn as = var->args;

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

					while (an && as) {
						int inlineArg = 0;
						astn arg = an;

						if (s->vm.opti && eval(&tmp, arg)) {
							arg = &tmp;
						}

						if (as->cast == TYPE_def) {
							inlineArg = 1;
						}
						else if (an->kind == TYPE_ref) {
							// static variables should not be inlined ?
							// what about indirect references ?
							//~ if (!an->id.link->stat)
								inlineArg = 1;
						}

						if (inlineArg) {
							as->kind = TYPE_def;
							as->init = an;
						}
						else {
							trace("arg(%k:[%t]): %+k", arg, as->type->cast, ast);
							if (!cgen(s, arg, as->cast)) {
								trace("%+k", arg);
								return 0;
							}
							as->offs = stkoffs(s, 0);
							as->kind = TYPE_ref;
						}
						an = an->next;
						as = as->next;
					}
					dieif(an || as, "Error");
				}

				//~ debug("inline(%t):%t %+k = %+k", get, ret, ast, var->init);
				//~ fputasm(stdout, s->vm, ipdbg, -1, 0x119);

				if (!(ret = cgen(s, var->init, ret))) {
					trace("%+T", var);
					return 0;
				}

				if (stkret != stkoffs(s, 0)) {
					trace("%+T: (%d - %d): %+k", ast->type, stkoffs(s, 0), stkret, ast);
					if (!emitidx(s, opc_ldsp, stkret)) {	// get stack
						trace("error");
						return 0;
					}
					if (get != TYPE_vid && !emitint(s, opc_sti, sizeOf(ast->type))) {
						error(s, ast->line, "store indirect: %T:%d of `%+k`", ast->type, sizeOf(ast->type), ast);
						return 0;
					}
					if (stkret < stkoffs(s, 0) && !emitidx(s, opc_drop, stkret)) {
						trace("%+k", ast);
						return 0;
					}
				}

				for (as = var->args; as; as = as->next) {
					as->kind = TYPE_ref;
					as->init = NULL;
				}
				if (get == TYPE_ref) {
					debug("cast for `%+k`, %t: %t", ast, get, ret);
					//~ return get;
				}
				break;
			}

			// push Result
			if (var && !isType(var) && var != emit_opc && var->call) {
				//~ trace("call %-T: %-T: %d", var, ast->type, var->type->size);
				if (sizeOf(var->type) && !emitint(s, opc_spc, sizeOf(var->type))) {
					trace("%+k", ast);
					return 0;
				}
			}
			// push args
			if (argv) {
				astn argl = NULL;

				while (argv->kind == OPER_com) {
					astn arg = argv->op.rhso;

					if (!genVal(s, arg)) {
						trace("%+k", arg);
						return 0;
					}
					arg->next = argl;
					argl = arg;
					argv = argv->op.lhso;
				}

				if (var == emit_opc && istype(argv))
					return castOf(argv->type);

				if (!genVal(s, argv)) {
					trace("%+k", argv);
					return 0;
				}

				argv->next = argl;
				argl = argv;
			}

			if (isType(var)) {				// cast()
				//~ debug("cast: %+k", ast);
				if (var == emit_opc) {	// emit()
					debug("emit: %+k", ast);
				}
				if (var == type_vid) {	// void(...)
					//~ debug("emit: %+k", ast);
				}
				else if (!argv || argv != ast->op.rhso)
					warn(s, 1, s->cc->file, ast->line, "multiple values, array ?: '%+k'", ast);
			}
			else if (var) {					// call()
				//~ debug("call: %+k", ast);
				if (var == emit_opc) {	// emit()
				}
				else if (var->call) {
					if (!cgen(s, ast->op.lhso, TYPE_ref)) {
						trace("%+k", ast);
						return 0;
					}
					if (!emitopc(s, opc_call)) {
						trace("%+k", ast);
						return 0;
					}
					// clean the stack
					if (stkret != stkoffs(s, 0) && !emitidx(s, opc_drop, stkret)) {
						trace("%+k", ast);
						return 0;
					}
				}
				else {
					//? can have an operator: call() with this args
					error(s, ast->line, "called object is not a function: %+T", var);
					return 0;
				}
			}
			else {							// ()
				dieif(ast->op.lhso, "FixMe %+k:%+k", ast, ast->op.lhso);
				if (!argv || argv != ast->op.rhso /* && cast*/)
					warn(s, 1, s->cc->file, ast->line, "multiple values, array ?: '%+k'", ast);
			}
		} break;
		case OPER_idx: {	// '[]'

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				trace("%+k", ast);
				return 0;
			}

			if (s->vm.opti && eval(&tmp, ast->op.rhso) == TYPE_int) {
				int offs = sizeOf(ast->type) * constint(&tmp);
				if (constint(&tmp) >= ast->op.lhso->type->size) {
					error(s, ast->line, " index out of bounds: %+k", ast);
				}
				if (!emitint(s, opc_inc, offs)) {
					trace("%+k", ast);
					return 0;
				}
			}
			else {
				if (!cgen(s, ast->op.rhso, TYPE_u32)) {
					trace("%+k", ast);
					return 0;
				}
				if (!emitint(s, opc_ldc4, sizeOf(ast->type))) {
					trace("%+k", ast);
					return 0;
				}
				if (!emitopc(s, u32_mad)) {
					trace("%+k", ast);
					return 0;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emitint(s, opc_ldi, sizeOf(ast->type))) {
				trace("%+k", ast);
				return 0;
			}
		} break; // */
		case OPER_dot: {	// '.'
			// TODO: this should work as indexing
			symn var = linkOf(ast->op.rhso);

			if (istype(ast->op.lhso)) {
				return cgen(s, ast->op.rhso, get);
			} // */

			if (!var) {
				//~ TODO: review
				trace("%+k", ast);
				return 0;
			}

			if (var->kind == TYPE_def) {
				debug("%+T", var);
				return cgen(s, ast->op.rhso, get);
			}

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				trace("%+k", ast);
				return 0;
			}

			if (!emitint(s, opc_inc, var->offs)) {
				trace("%+k", ast);
				return 0;
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emitint(s, opc_ldi, sizeOf(ast->type))) {
				trace("%+k", ast);
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
			if (!(ret = cgen(s, ast->op.rhso, 0))) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(s, opc, ret)) {
				trace("%+k", ast);
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
				default: fatal("FixMe"); return 0;
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
			if (!cgen(s, ast->op.lhso, ast->op.lhso->cst2)) {
				trace("%+k", ast);
				return 0;
			}
			if (!cgen(s, ast->op.rhso, ast->op.rhso->cst2)) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(s, opc, ast->op.lhso->cst2)) {	// uint % int => u32.mod
				trace("opc__%02x:%+k", opc, ast);
				return 0;
			}

			#if DEBUGGING
			// these must be true
			dieif(ast->op.lhso->cst2 != ast->op.rhso->cst2, "RemMe", ast);
			dieif(ret != castOf(ast->type), "RemMe");
			switch (ast->kind) {
				case OPER_neq:
				case OPER_equ:
				case OPER_geq:
				case OPER_lte:
				case OPER_leq:
				case OPER_gte:
					dieif(ret != TYPE_bit, "RemMe(%t): %+7K", ret, ast);
					break;
			}
			#endif
		} break;

		//~ case OPER_lnd:		// '&&'
		//~ case OPER_lor:		// '||'
		case OPER_sel: {		// '?:'
			int jmpt, jmpf;
			if (0 && s->vm.opti && eval(&tmp, ast->op.test)) {
				if (!cgen(s, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, 0)) {
					trace("%+k", ast);
					return 0;
				}
			}
			else {
				int bppos = stkoffs(s, 0);

				if (!cgen(s, ast->op.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}

				if (!(jmpt = emitopc(s, opc_jz))) {
					trace("%+k", ast);
					return 0;
				}

				if (!cgen(s, ast->op.lhso, 0)) {
					trace("%+k", ast);
					return 0;
				}

				if (!(jmpf = emitopc(s, opc_jmp))) {
					trace("%+k", ast);
					return 0;
				}

				fixjump(s, jmpt, emitopc(s, markIP), bppos);

				if (!cgen(s, ast->op.rhso, 0)) {
					trace("%+k", ast);
					return 0;
				}

				fixjump(s, jmpf, emitopc(s, markIP), -sizeOf(ast->type));
			}
		} break;

		case ASGN_set: {		// ':='
			// TODO: lhs should be evaluated first ??? or not
			if (!cgen(s, ast->op.rhso, ret)) {
				trace("%+k", ast);
				return 0;
			}
			if (get != TYPE_vid) {
				// in case a = b = sum(2, 700);
				// dupplicate the result
				if (!emitint(s, opc_ldsp, 0)) {
					trace("%+k", ast);
					return 0;
				}
				if (!emitint(s, opc_ldi, sizeOf(ast->type))) {
					trace("%+k", ast);
					return 0;
				}
			}
			else
				ret = get;

			if (!cgen(s, ast->op.lhso, TYPE_ref)) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(s, opc_sti, sizeOf(ast->type))) {
				trace("%+k", ast);
				return 0;
			}
		} break;// */
		//}
		//{ TVAL
		//~ case TYPE_bit:	// no constant of this kind
		case TYPE_int: switch (get) {
			case TYPE_vid: return TYPE_vid;
			case TYPE_bit: return emiti32(s, constbol(ast)) ? TYPE_u32 : 0;
			case TYPE_u32: return emiti32(s, ast->con.cint) ? TYPE_u32 : 0;
			case TYPE_i32: return emiti32(s, ast->con.cint) ? TYPE_i32 : 0;
			case TYPE_i64: return emiti64(s, ast->con.cint) ? TYPE_i64 : 0;
			case TYPE_f32: return emitf32(s, ast->con.cint) ? TYPE_f32 : 0;
			case TYPE_f64: return emitf64(s, ast->con.cint) ? TYPE_f64 : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast); return 0;
		} break;
		case TYPE_flt: switch (get) {
			case TYPE_vid: return TYPE_vid;
			case TYPE_bit: return emiti32(s, constbol(ast)) ? TYPE_u32 : 0;
			case TYPE_u32: return emiti32(s, ast->con.cflt) ? TYPE_u32 : 0;
			case TYPE_i32: return emiti32(s, ast->con.cflt) ? TYPE_i32 : 0;
			case TYPE_i64: return emiti64(s, ast->con.cflt) ? TYPE_i64 : 0;
			case TYPE_f32: return emitf32(s, ast->con.cflt) ? TYPE_f32 : 0;
			case TYPE_f64: return emitf64(s, ast->con.cflt) ? TYPE_f64 : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast); return 0;
		} break;
		case TYPE_str: switch (get) {
			case TYPE_vid: return TYPE_vid;
			case TYPE_ref: return emitptr(s, ast->id.name) ? TYPE_ref : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast); return 0;
		} break;

		case TYPE_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->id.link;		// link
			dieif(!typ || !var, "FixMe");

			switch (var->kind) {
				case TYPE_ref: {

					if (!genRef(s, var->offs)) {
						trace("%+k", ast);
						return 0;
					}

					if (var->cast == TYPE_ref && ast->cst2 != ASGN_set) {
						if (!emitint(s, opc_ldi, 4)) {
							trace("%+k", ast);
							return 0;
						}
					}

					if (get == ASGN_set)
						get = TYPE_ref;

					if (get != TYPE_ref) {
						if (!emitint(s, opc_ldi, sizeOf(typ))) {
							trace("%+k", ast);
							return 0;
						}
					}
					else {
						get = ret = TYPE_ref;
					}
					//~ debug("cgen[%t->%t](%+k)\n%7K", get, ret, ast, ast);
					//~ fputasm(stderr, s->vm, ipdbg, -1, 0x119);
				} break;
				case EMIT_opc:
					dieif(get == TYPE_ref, "FixMe");
					if (!emitint(s, var->offs, var->init ? constint(var->init) : 0)) {
						error(s, ast->line, "error emiting opcode: %+k", ast);
						if (stkoffs(s, 0) > 0)
							info(s, s->cc->file, ast->line, "%+k underflows stack.", ast);
						return 0;
					}
					return TYPE_vid;

				case TYPE_def:
					// TODO: this sucks, but it works
					if (var->init)
						return cgen(s, var->init, get);
				default:
					error(s, ast->line, "invalid rvalue `%+k:` %t", ast, var->kind);
					return 0;
			}
		} break;
		case TYPE_def: {					// new (var, func, define)
			symn typ = ast->type;
			symn var = ast->id.link;
			int stktop = stkoffs(s, sizeOf(var));
			dieif(!typ || !var, "FixMe");

			if (var->kind == TYPE_ref) {

				// static variable or function.
				if (0 && var->offs < 0) {
					//~ trace("%-T", var);

					/*// function
					if (var->call) {
						return ret;
					}

					// initialize
					if (var->init) {
						//~ if variable was allocated
						//~ make assigment from initializer
						struct astn id = {0}; //~ memset(&id, 0, sizeof(struct astn));
						struct astn op = {0}; //~ memset(&op, 0, sizeof(struct astn));
						op.kind = ASGN_set;
						op.cst2 = TYPE_vid;
						op.type = var->type;
						op.op.lhso = &id;
						op.op.rhso = var->init;

						id.kind = TYPE_ref;
						id.cst2 = TYPE_ref;		// wee need a reference here
						id.type = var->type;
						id.id.link = var;

						//~ fatal("FixMe");

						return cgen(s, &op, 0);
					}*/

					return ret;
				}

				/*if (typ->cast == TYPE_ref) {	// this should be not here
					var->cast = TYPE_ref;
					//~ var->load = 1;
				}*/

				if (var->init) {
					astn val = var->init;

					//~ debug("new %T, %d", var, var->line);
					if (var->call) {
						ret = TYPE_ref;
					}
					else if (val->type == emit_opc) {
						ret = get;
					}
					else if (val->type != typ && !promote(val->type, typ)) {
						error(s, ast->line, "incompatible types in initialization(%-T := %-T)", typ, val->type);
					}

					if (s->vm.opti && eval(&tmp, val)) {
						val = &tmp;
					}// * /

					//~ debug("cast(`%+k`, %t):%t", ast, get, ret);

					/*/ TEMP: get a pointer to the stack top: `pointer sp = emit;`
					if (val->kind == EMIT_opc) {
						if (!emitint(s->vm, opc_ldsp, 0)) {
							trace("%+k", ast);
							return 0;
						}
						var->cast = TYPE_ref;
					}
					else // */
					if (!cgen(s, val, ret)) {
						trace("%+k", ast);
						return 0;
					}

					if (!var->offs) {
						var->offs = stkoffs(s, 0);
						if (stktop != var->offs && val->type != emit_opc) {
							error(s, ast->line, "invalid initializer size: %d <> got(%d): `%+k`", stktop, var->offs, val);
							return 0;
						}
					}
					else {
						if (!genRef(s, var->offs)) {
							trace("%+k", ast);
							return 0;
						}
						if (!emitint(s, opc_sti, sizeOf(ast->type))) {
							trace("%+k", ast);
							return 0;
						}
						return ret;
					}
				}
				else if (!var->offs) {		// static alloc a block of the size of the type;
					int size = sizeOf(typ);
					if (!emitint(s, opc_spc, size)) {
						trace("%+k", ast);
						return 0;
					}
					var->offs = stkoffs(s, 0);
					if (var->offs < size) {
						error(s, ast->line, "stack underflow", var->offs, size);
						return 0;
					}
				}
				else {
					// static uninitialized variable
				}
				//~ return get;
			}

			dieif(get != TYPE_vid, "FixMe");
			return get;
			// else [enum, struct, define]
		} break;

		case EMIT_opc:
			trace("%+k", ast);
			return 0;
		//~ case TYPE_rec:break;
		//}
	}

	if (get != ret) switch (get) {
		case TYPE_vid: return TYPE_vid;
		case TYPE_any: switch (ret) {
			case TYPE_vid: break;
			default: goto errorcast2;
		} break;// */

		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32: /*emitopc(s, i32_bol);*/ break;
			case TYPE_i32: /*emitopc(s, i32_bol);*/ break;
			case TYPE_i64:
				if (!emitopc(s, i64_bol)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(s, f32_bol)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(s, f64_bol)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			default: goto errorcast2;
		} break;

		case TYPE_u32: switch (ret) {
			case TYPE_bit:
			case TYPE_i32: break;
			case TYPE_i64: {
				if (!emitopc(s, i64_i32)) {
					trace("%+k", ast);
					return 0;
				}
			} break;
			//~ case TYPE_f32: if (!emitopc(s, f32_i32)) return 0; break;
			//~ case TYPE_f64: if (!emitopc(s, f64_i32)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i32: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64:
				if (!emitopc(s, i64_i32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(s, f32_i32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(s, f64_i32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			//~ case TYPE_ref: if (!emitopc(s, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i64: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_u32:	// TODO: zero extend, but how ?
			case TYPE_u32:
				if (!emitopc(s, u32_i64)) {
					trace("%+k", ast);
					return 0; 
				}
				break;
			case TYPE_i32:
				if (!emitopc(s, i32_i64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(s, f32_i64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(s, f64_i64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			//~ case TYPE_ref: if (!emitopc(s, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;

		case TYPE_f32: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_u32:
			case TYPE_i32: 
				if (!emitopc(s, i32_f32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_i64: 
				if (!emitopc(s, i64_f32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(s, f64_f32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			//~ case TYPE_ref: if (!emitopc(s, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f64: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_u32:
			case TYPE_i32:
				if (!emitopc(s, i32_f64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_i64: 
				if (!emitopc(s, i64_f64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(s, f32_f64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			//~ case TYPE_ref: if (!emitopc(s, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;

		case TYPE_ref:
			trace("cgen[%t->%t](%+k)\n%7K", ret, get, ast, ast);
			error(s, ast->line, "invalid rvalue: %+k", ast);
			return 0;
		default:
			trace("cgen[%t->%t](%+k)\n%7K", ret, get, ast, ast);
			fatal("%d: unimplemented(cast for `%+k`, %t):%t", ast->line, ast, get, ret);
		errorcast2:
			trace("cgen[%t->%t](%+k)\n%7K", ret, get, ast, ast);
			return 0;
	}

	dieif (qual, "unimplemented `%+k`: %t", ast, qual);
	return ret;
}

int logFILE(state s, FILE* file) {
	if (s->closelog)
		fclose(s->logf);
	s->logf = file;
	s->closelog = 0;
	return 0;
}
int logfile(state s, char* file) {

	logFILE(s, NULL);

	if (file) {
		s->logf = fopen(file, "wb");
		if (!s->logf) return -1;
		s->closelog = 1;
	}
	return 0;
}

void ccSource(ccState cc, char *file, int line) {
	cc->file = file;
	cc->line = line;
}

int gencode(state s, int level) {
	ccState cc = s->cc;
	int Lmain;

	if (s->errc)
		return -2;

	if (!s->cc) {
		fatal("invalid enviroment: cc:%x, er:%d", cc, s->errc);
		return -2;
	}

	s->defs = leave(s->cc, NULL);

	if (!vmInit(s))
		return -1;

	//~ level = -2;
	s->vm.opti = level < 0 ? -level : level;	// 

	//~ DONE: Data[RO]: metadata, strings
	//~ TODO: Data[RO]: constants, functions, enums ...
	s->vm.ro = s->vm.pos;

	// static vars & functions
	if (1 && s->defs) {
		symn var = s->defs;
		astn lend = NULL, lbeg = NULL;

		dieif(s->defs != s->cc->all, "FixMe");

		// make global symbols static ?
		if (level >= 0) {
			for (var = s->defs; var; var = var->next) {

				if (var->stat)		// skip already static variables
					continue;

				if (var->kind != TYPE_ref)
					continue;

				//~ debug("static global: %+T@%x", var, var->offs);
				var->stat = 1;
				var->glob = 1;
			}
		}// */

		// loop through all symbols generate static non global variables
		for (var = s->defs; var; var = var->defs) {

			if (!var->stat)
				continue;

			if (var == null_ref)
				continue;

			if (var->kind != TYPE_ref) {
				debug("%-T", var);	// static what ?
				continue;
			}

			if (var->call && var->cast != TYPE_ref)
				continue;

			dieif(var->offs, "FixMe %-T@%d", var, var->offs);

			var->offs = -s->vm.pos;
			s->vm.pos += sizeOf(var);

			if (var->init && !var->glob) {
				//~ if variable was allocated
				//~ make assigment from initializer
				//~ TODO: do not create assigment !!!
				astn st = newnode(cc, STMT_do);
				astn id = newnode(cc, TYPE_ref);
				astn op = newnode(cc, ASGN_set);

				op->kind = ASGN_set;
				op->cst2 = var->cast;
				op->type = var->type;
				op->op.lhso = id;
				op->op.rhso = var->init;

				id->kind = TYPE_ref;
				id->cst2 = TYPE_ref;		// wee need a reference here
				id->type = var->type;
				id->id.link = var;

				st->kind = STMT_do;
				st->cst2 = TYPE_any;
				st->type = type_vid;
				st->stmt.stmt = op;

				op->cst2 = TYPE_vid;
				//~ op->type = type_vid;
				//~ /*
				if (var->cast == TYPE_ref) {
					op->cst2 = TYPE_vid;
					op->type = type_ptr;
					id->type = type_ptr;
					var->init->cst2 = TYPE_ref;
					if (var->call)
						id->cst2 = ASGN_set;
				}// */

				dieif(!cc->root || cc->root->kind != STMT_beg, "FixMe");

				if (lend == NULL) {
					lbeg = st;
					//~ cc->root->stmt.stmt = st;
				}
				else {
					lend->next = st;
				}

				st->next = NULL;
				var->init = NULL;
				lend = st;
			}// */
			trace("static variable: %+T@%x:%d", var, -var->offs, sizeOf(var));
		}

		if (lbeg && lend) {
			if (cc->root) {
				dieif(cc->root->kind != STMT_beg, "FixMe");
				lend->next = cc->root->stmt.stmt;
				cc->root->stmt.stmt = lbeg;
			}
		}

		Lmain = s->vm.pos;
		// loop through all symbols generate functions
		for (var = s->defs; var; var = var->defs) {

			if (!var->stat)
				continue;

			if (!var->call)
				continue;

			if (var == null_ref)
				continue;

			if (var->kind != TYPE_ref) {
				debug("%-T", var);	// static what ?
				continue;
			}

			if (var->call && var->cast != TYPE_ref) {
				int seg = emitopc(s, markIP);

				if (!var->init) {
					debug("`%T` will be skipped", var);
					return 0;
				}

				s->vm.sm = 0;
				fixjump(s, 0, 0, 4 + var->offs);
				var->offs = -emitopc(s, markIP);

				if (!cgen(s, var->init, TYPE_vid)) {
					return 0;
				}
				if (!emitopc(s, opc_jmpi)) {
					error(s, var->line, "error emiting function: %-T", var);
					return 0;
				}
				while (cc->jmps) {
					error(s, 0, "invalid jump: `%k`", cc->jmps);
					cc->jmps = cc->jmps->next;
				}
				var->size = emitopc(s, markIP) - seg;

				//~ debug("function @%d: %+T\n%7K", -var->offs, var, var->init);
				//~ debug("function @%d: %-T", -var->offs, var, var->init);
				//~ fputasm(stderr, vm, seg, -1, 0x129);

				//~ trace("static function: %+T@%x", var, -var->offs);
				var->init = NULL;
			}
		}
	}

	Lmain = s->vm.pos;
	if (1 && cc->root) {
		// TODO: TYPE_vid: to clear the stack
		int seg = s->vm.pos;
		s->vm.ss = s->vm.sm = 0;
		if (!cgen(s, cc->root, 0))
			fputasm(stderr, s, seg, -1, 0x10);
		while (cc->jmps) {
			error(s, 0, "invalid jump: `%k`", cc->jmps);
			cc->jmps = cc->jmps->next;
		}
	}
	s->vm.px = emitint(s, opc_libc, 0);		// TODO: Halt only in case of scripts

	// program entry point
	s->vm.pc = Lmain;

	return s->errc;
}
//~ int execute(state s, int stack) ;

symn emit_opc = NULL;
static void install_emit(ccState cc, int mode) {
	symn typ;
	symn type_v4f = NULL;

	if (!(mode & creg_emit))
		return;

	emit_opc = install(cc, "emit", EMIT_opc | symn_read, 0, 0);

	if (!emit_opc)
		return;

	if (emit_opc && (mode & creg_eopc) == creg_eopc) {

		symn u32, i32, i64, f32, f64, v4f, v2d, ref, val;
		enter(cc, NULL);

		u32 = install(cc, "u32", TYPE_rec, TYPE_u32, 4);
		i32 = install(cc, "i32", TYPE_rec, TYPE_i32, 4);
		i64 = install(cc, "i64", TYPE_rec, TYPE_i64, 8);
		f32 = install(cc, "f32", TYPE_rec, TYPE_f32, 4);
		f64 = install(cc, "f64", TYPE_rec, TYPE_f64, 8);
		v4f = install(cc, "v4f", TYPE_rec, TYPE_rec, 16);		// TODO: ???
		v2d = install(cc, "v2d", TYPE_rec, TYPE_rec, 16);		// TODO: ???
		ref = install(cc, "ref", TYPE_rec, TYPE_ref, 4);
		val = install(cc, "val", TYPE_rec, TYPE_rec, -1);

		installex(cc, "nop", EMIT_opc, opc_nop, type_vid, NULL);
		installex(cc, "not", EMIT_opc, opc_not, type_bol, NULL);
		installex(cc, "pop", EMIT_opc, opc_drop, type_vid, intnode(cc, 4));

		installex(cc, "set", EMIT_opc, opc_set1, type_vid, intnode(cc, 1));
		installex(cc, "set0", EMIT_opc, opc_set1, type_vid, intnode(cc, 0));
		installex(cc, "set1", EMIT_opc, opc_set1, type_vid, intnode(cc, 1));

		if ((typ = install(cc, "dupp", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "x1", EMIT_opc, opc_dup1, type_i32, intnode(cc, 0));
			installex(cc, "x2", EMIT_opc, opc_dup2, type_i64, intnode(cc, 0));
			installex(cc, "x4", EMIT_opc, opc_dup4, type_vid, intnode(cc, 0));
			typ->args = leave(cc, typ);
		}

		if ((typ = install(cc, "ldz", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "x1", EMIT_opc, opc_ldz1, type_vid, intnode(cc, 0));
			installex(cc, "x2", EMIT_opc, opc_ldz2, type_vid, intnode(cc, 0));
			installex(cc, "x4", EMIT_opc, opc_ldz4, type_vid, intnode(cc, 0));
			typ->args = leave(cc, typ);
		}
		if ((typ = install(cc, "load", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "b8",   EMIT_opc, opc_ldi1, type_vid, NULL);
			installex(cc, "b16",  EMIT_opc, opc_ldi2, type_vid, NULL);
			installex(cc, "b32",  EMIT_opc, opc_ldi4, type_vid, NULL);
			installex(cc, "b64",  EMIT_opc, opc_ldi8, type_vid, NULL);
			installex(cc, "b128", EMIT_opc, opc_ldiq, type_vid, NULL);
			typ->args = leave(cc, typ);
		}
		if ((typ = install(cc, "store", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "b8",   EMIT_opc, opc_sti1, type_vid, NULL);
			installex(cc, "b16",  EMIT_opc, opc_sti2, type_vid, NULL);
			installex(cc, "b32",  EMIT_opc, opc_sti4, type_vid, NULL);
			installex(cc, "b64",  EMIT_opc, opc_sti8, type_vid, NULL);
			installex(cc, "b128", EMIT_opc, opc_stiq, type_vid, NULL);
			typ->args = leave(cc, typ);
		}// */

		installex(cc, "call", EMIT_opc, opc_call, type_vid, NULL);

		if ((typ = u32) != NULL) {
			enter(cc, NULL);
			installex(cc, "cmt", EMIT_opc, b32_cmt, type_u32, NULL);
			//~ installex(cc, "adc", EMIT_opc, b32_adc, type_u32, NULL);
			//~ installex(cc, "sub", EMIT_opc, b32_sbb, type_u32, NULL);5

			//~ type_muldiv
			//~ installex(cc, "mul", EMIT_opc, u32_mul, type_u32, NULL);
			//~ installex(cc, "div", EMIT_opc, u32_div, type_u32, NULL);
			//~ installex(cc, "mod", EMIT_opc, u32_mod, type_u32, NULL);

			installex(cc, "mad", EMIT_opc, u32_mad, type_u32, NULL);
			installex(cc, "clt", EMIT_opc, u32_clt, type_bol, NULL);
			installex(cc, "cgt", EMIT_opc, u32_cgt, type_bol, NULL);
			installex(cc, "and", EMIT_opc, b32_and, type_u32, NULL);
			installex(cc,  "or", EMIT_opc, b32_ior, type_u32, NULL);
			installex(cc, "xor", EMIT_opc, b32_xor, type_u32, NULL);
			installex(cc, "shl", EMIT_opc, b32_shl, type_u32, NULL);
			installex(cc, "shr", EMIT_opc, b32_shr, type_u32, NULL);
			//~ installex(cc, "toi64", EMIT_opc, u32_i64, type_i64, NULL);
			u32->args = leave(cc, u32);
		}
		if ((typ = i32) != NULL) {
			enter(cc, NULL);
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
		if ((typ = i64) != NULL) {
			enter(cc, NULL);
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
		if ((typ = f32) != NULL) {
			enter(cc, NULL);
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
		if ((typ = f64) != NULL) {
			enter(cc, NULL);
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

		if ((typ = v4f) != NULL) {
			type_v4f = typ;
			enter(cc, NULL);
			installex(cc, "neg", EMIT_opc, v4f_neg, type_v4f, NULL);
			installex(cc, "add", EMIT_opc, v4f_add, type_v4f, NULL);
			installex(cc, "sub", EMIT_opc, v4f_sub, type_v4f, NULL);
			installex(cc, "mul", EMIT_opc, v4f_mul, type_v4f, NULL);
			installex(cc, "div", EMIT_opc, v4f_div, type_v4f, NULL);
			installex(cc, "equ", EMIT_opc, v4f_ceq, type_bol, NULL);
			installex(cc, "min", EMIT_opc, v4f_min, type_v4f, NULL);
			installex(cc, "max", EMIT_opc, v4f_max, type_v4f, NULL);
			installex(cc, "dp3", EMIT_opc, v4f_dp3, type_f32, NULL);
			installex(cc, "dp4", EMIT_opc, v4f_dp4, type_f32, NULL);
			installex(cc, "dph", EMIT_opc, v4f_dph, type_f32, NULL);
			typ->args = leave(cc, typ);
		}
		if ((typ = v2d) != NULL) {
			enter(cc, NULL);
			installex(cc, "neg", EMIT_opc, v2d_neg, typ, NULL);
			installex(cc, "add", EMIT_opc, v2d_add, typ, NULL);
			installex(cc, "sub", EMIT_opc, v2d_sub, typ, NULL);
			installex(cc, "mul", EMIT_opc, v2d_mul, typ, NULL);
			installex(cc, "div", EMIT_opc, v2d_div, typ, NULL);
			installex(cc, "equ", EMIT_opc, v2d_ceq, type_bol, NULL);
			installex(cc, "min", EMIT_opc, v2d_min, typ, NULL);
			installex(cc, "max", EMIT_opc, v2d_max, typ, NULL);
			typ->args = leave(cc, typ);
		}

		if ((mode & creg_swiz) == creg_swiz) {
			int i;
			struct {
				char *name;
				astn node;
				//~ char *swz, *msk;	// swizzle and mask
			} swz[256];
			for (i = 0; i < 256; i += 1) {
				dieif(cc->_end - cc->_beg < 5, "memory overrun");
				cc->_beg[0] = "xyzw"[i>>0&3];
				cc->_beg[1] = "xyzw"[i>>2&3];
				cc->_beg[2] = "xyzw"[i>>4&3];
				cc->_beg[3] = "xyzw"[i>>6&3];
				cc->_beg[4] = 0;

				swz[i].name = mapstr(cc, cc->_beg, -1, -1);
				swz[i].node = intnode(cc, i);
			}

			if ((typ = install(cc, "swz", TYPE_def, 0, 0))) {
				enter(cc, NULL);
				for (i = 0; i < 256; i += 1)
					installex(cc, swz[i].name, EMIT_opc, p4d_swz, type_v4f, swz[i].node);
				typ->args = leave(cc, typ);
			}
			if ((typ = install(cc, "dup", TYPE_def, 0, 0))) {
				//~ extended set(masked) and dup(swizzle): p4d.dup.xyxy / p4d.set.xyz0
				enter(cc, NULL);
				for (i = 0; i < 256; i += 1)
					installex(cc, swz[i].name, EMIT_opc, 0x1d, type_v4f, swz[i].node);
				typ->args = leave(cc, typ);
			}
			if ((typ = install(cc, "set", TYPE_def, 0, 0))) {
				enter(cc, NULL);
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
		}
		emit_opc->args = leave(cc, emit_opc);
		(void)val;
		(void)ref;
	}
}

state rtInit(void* mem, unsigned size) {
	state s = mem;
	s->_cnt = size - sizeof(struct state);
	s->_ptr = s->_mem;
	logFILE(s, stderr);
	s->cc = NULL;
	return s;
}

ccState ccInit(state s, int mode) {
	ccState cc = getmem(s, sizeof(struct ccState), -1);

	//~ symn type_tmp;
	symn type_i08 = NULL, type_i16 = NULL;
	symn type_u08 = NULL, type_u16 = NULL;

	if (cc == NULL)
		return NULL;

	cc->s = s;
	s->cc = cc;

	cc->_chr = -1;

	cc->fin._ptr = 0;
	cc->fin._cnt = 0;
	cc->fin._fin = -1;

	cc->_beg = (char*)s->_ptr;
	cc->_end = cc->_beg + s->_cnt;

	//{ install Type
	type_vid = install(cc,  "void", TYPE_rec | symn_read, TYPE_vid, 0);

	type_bol = install(cc,  "bool", TYPE_rec | symn_read, TYPE_bit, 4);

	type_i08 = install(cc,  "int8", TYPE_rec | symn_read, TYPE_i32, 1);
	type_i16 = install(cc, "int16", TYPE_rec | symn_read, TYPE_i32, 2);
	type_i32 = install(cc, "int32", TYPE_rec | symn_read, TYPE_i32, 4);
	type_i64 = install(cc, "int64", TYPE_rec | symn_read, TYPE_i64, 8);

	type_u08 = install(cc,  "uint8", TYPE_rec | symn_read, TYPE_u32, 1);
	type_u16 = install(cc, "uint16", TYPE_rec | symn_read, TYPE_u32, 2);
	type_u32 = install(cc, "uint32", TYPE_rec | symn_read, TYPE_u32, 4);
	//~ type_u64 = install(cc, "uint64", TYPE_rec | symn_read, TYPE_i64, 8);

	//~ type_f16 = install(cc, "float16", TYPE_rec | symn_read, TYPE_f32, 2);
	type_f32 = install(cc, "float32", TYPE_rec | symn_read, TYPE_f32, 4);
	type_f64 = install(cc, "float64", TYPE_rec | symn_read, TYPE_f64, 8);

	if (mode & creg_emit)
		install_emit(cc, mode);

	//~ type_arr = install(cc, ".array", TYPE_rec | symn_read, TYPE_ref, 4);
	type_ptr = install(cc, ".pointer", TYPE_rec | symn_read, TYPE_ref, 4);
	type_str = install(cc, "string", TYPE_rec | symn_read, TYPE_ref, 4);
	//~ type_str = install(cc, "string", TYPE_arr | symn_read, TYPE_ref, 4);

	//~ type_chr = installex(cc, "char", TYPE_def, 0, type_i08, NULL);
	//~ type_str = installex(cc, "string", TYPE_arr | symn_read | symn_stat, TYPE_ref, type_i08, NULL);

	installex(cc, "int",    TYPE_def, 0, type_i32, NULL);
	installex(cc, "long",   TYPE_def, 0, type_i64, NULL);
	installex(cc, "float",  TYPE_def, 0, type_f32, NULL);
	installex(cc, "double", TYPE_def, 0, type_f64, NULL);
	installex(cc, "true",   TYPE_def, 0, type_bol, intnode(cc, 1));
	installex(cc, "false",  TYPE_def, 0, type_bol, intnode(cc, 0));

	null_ref = installex(cc, "null", TYPE_ref | symn_read | symn_stat, 0, type_ptr, NULL);

	//} */// types

	// install a void arg for functions with no arguments
	if ((cc->argz = newnode(cc, TYPE_ref))) {
		enter(cc, NULL);
		//~ cc->argz->type = type_vid;
		//~ cc->argz->cst2 = TYPE_vid;
		//~ cc->argz->id.name = NULL;
		//~ cc->argz->id.hash = 0;
		//~ cc->argz->id.link = installex(cc, "", TYPE_ref | symn_read, type_vid, TYPE_ref, 0, NULL);;
		//~ cc->argz->next = NULL;

		cc->argz->id.name = "";
		cc->argz->next = NULL;
		declare(cc, TYPE_ref, cc->argz, type_vid);
		leave(cc, NULL);
	}

	return cc;
}
ccState ccOpen(state s, srcType mode, char *src) {
	if (s->cc || ccInit(s, creg_def)) {
		if (source(s->cc, mode & srcFile, src) != 0)
			return NULL;
	}
	return s->cc;
}
int ccDone(state s) {
	ccState cc = s->cc;

	// if not initialized
	if (cc == NULL)
		return -1;

	if (peek(s->cc)) {
		astn ast = peek(s->cc);
		error(s, ast->line, "unexpected: `%k`", ast);
		return -1;
	}
	// close input
	source(cc, 0, 0);

	// set used memory
	s->_cnt -= cc->_beg - (char*)s->_ptr;
	s->_ptr = (unsigned char*)cc->_beg;

	// return errors
	return s->errc;
}

state vmInit(state s) {
	s->vm.pos = s->_ptr - s->_mem;
	s->vm._end = s->_mem + s->_cnt;
	return s;
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
// lookup a value
symn findsym(ccState s, symn in, char *name) {
	struct astn ast;
	memset(&ast, 0, sizeof(struct astn));
	ast.kind = TYPE_ref;
	//ast.id.hash = rehash(name);
	ast.id.name = name;
	return lookup(s, in ? in : s->s->defs, &ast, NULL, 1);
}
symn findref(state s, void *ptr) {
	int offs;
	symn sym = s->defs;

	if (ptr == NULL) {
		trace("null");
		return NULL;
	}

	if (ptr < (void*)s->_mem) {
		fatal("Error");
		return NULL;
	}

	if (ptr > (void*)s->_mem + s->vm.pc) {		// pc 
		fatal("Error");
		return NULL;
	}

	offs = (unsigned char*)ptr - s->_mem;
	for (; sym; sym = sym->next) {
		if (sym->offs == -offs)
			return sym;
	}

	return NULL;
}

int findint(ccState s, char *name, int* res) {
	struct astn ast;
	symn sym = findsym(s, NULL, name);
	if (sym && eval(&ast, sym->init)) {
		*res = constint(&ast);
		return 1;
	}
	return 0;
}
int findflt(ccState s, char *name, double* res) {
	struct astn ast;
	symn sym = findsym(s, NULL, name);
	if (sym && eval(&ast, sym->init)) {
		*res = constflt(&ast);
		return 1;
	}
	return 0;
}
int findnzv(ccState s, char *name) {
	struct astn ast;
	symn sym = findsym(s, NULL, name);
	if (sym && eval(&ast, sym->init)) {
		return constbol(&ast);
	}
	return 0;
}

#if 0

//static void install_rtti(ccState cc, unsigned level) ;		// req emit: libc(import, define, lookup, ...)

static void install_lang(ccState cc, unsigned level) {
	if (install(cc, "Math", TYPE_def, 0, 0)) {
		installex(cc, "pi", TYPE_def, 0, type_f64, fh8node(cc, 0x400921fb54442d18LL));
	}
	if (install(cc, "Compiler", TYPE_def, 0, 0)) {
		//~ install(cc, "version", VERS_get, 0, 0);	// type int
		//~ install(cc, "file", LINE_get, 0, 0);	// type string
		//~ install(cc, "line", FILE_get, 0, 0);	// type int
		//~ install(cc, "date", DATE_get, 0, 0);	// type ???
		//~ install(cc, "emit", DATE_get, 0, 0);	// type ???
	}
	if (install(cc, "Runtime", TYPE_def, 0, 0)) {
		install(cc, "Env", TYPE_def, 0, 0);	// map<String, String> Enviroment;
		install(cc, "Args", TYPE_arr, 0, 0);// string Args[int];
		install(cc, "assert(bool test, string error, var args...)", TYPE_def, 0, 0);
	}
	if (install(cc, "Reflection", TYPE_def, 0, 0)) {
		install(cc, "symbol", TYPE_rec, 0, 0);
		install(cc, "invoke(type t, string fun, var args...)", TYPE_def, 0, 0);
	}
}// */
#endif
// vm test&help
#if 0
int vm_hgen() {
	int i, e = 0;
	FILE *out = stdout;
	for (i = 0; i < opc_last; i++) {
		int err = 0;
		if (!opc_tbl[i].name) continue;
		if (!opc_tbl[i].size) continue;

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
	int n, sp = sizeof(test) - 6;
	FILE* f = fopen(file, "wb");

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
	else
		debug("file is too large (128M max)");

	fclose(f);
	return 0;
}
#endif
// arrBuffer
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

