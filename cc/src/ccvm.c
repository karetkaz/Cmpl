/*******************************************************************************
 *   File: ccvm.c
 *   Date: 2011/06/23
 *   Desc: type system
 *******************************************************************************
the core:
	convert ast to vmcode
	initializations, memory management
*******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "ccvm.h"

#ifdef __WATCOMC__
//~ Warning! W124: Comparison result always 0
//~ Warning! W201: Unreachable code
//~ #pragma disable_message(124);
//~ #pragma disable_message(201);
#endif
const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, STR) {TYPE, SIZE, STR},
	#include "defs.i"
	{0},
};
const opc_inf opc_tbl[255] = {
	//#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem)
	#define OPCDEF(NAME, CODE, SIZE, CHCK, DIFF, TIME, MNEM) {CODE, SIZE, CHCK, DIFF, MNEM},
	#include "defs.i"
	{0},
};

STINLINE int genRef(state rt, int offset) {
	if (offset > 0)
		return emitidx(rt, opc_ldsp, offset);
	return emitint(rt, opc_ldcr, -offset);
} // */

int cgen(state rt, astn ast, int get) {
	#if DEBUGGING > 1
	int ipdbg = emitopc(rt, markIP);
	#endif
	struct astn tmp;
	int ret = 0;
	int qual = 0;

	dieif(!ast || !ast->type, "FixMe %+k", ast);

	TODO("this sucks")
	if (get == TYPE_any)
		get = ast->cst2;

	if (!(ret = ast->type->cast))
		ret = ast->type->kind;

	//~ debug("%+k", ast);

	if (ast->kind > STMT_beg && ast->kind < STMT_end) {
		list l = setBuff(&rt->cc->dbg, rt->cc->dbg.cnt, NULL);
		dieif (l == NULL, "Fatal Error allocating @%d", rt->cc->dbg.cnt);
		l->size = emitopc(rt, markIP);
		l->data = (void*)ast;
	}
	if (ast->kind >= STMT_beg && ast->kind <= STMT_end) {
		qual = ast->cst2;
	}

	switch (ast->kind) {
		default:
			fatal("FixMe(%t)", ast->kind);
			return 0;

		//{ STMT
		case STMT_do:  {	// expr or decl statement
			int stpos = stkoffs(rt, 0);
			//~ trace("%+k", ast);
			if (!cgen(rt, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}
			if (stpos != stkoffs(rt, 0)) {
				if (stpos > stkoffs(rt, 0)) {
					warn(rt, 1, ast->file, ast->line, "statement underflows stack: %+k", ast->stmt.stmt);
				}
			}
			#if 0 && DEBUGGING > 1
			debug("cgen[%t->%t](err, %+k)\n%7K", get, ret, ast, ast);
			fputasm(stdout, rt, ipdbg, -1, 0x119);
			#endif
		} break;
		case STMT_beg: {	// {} or function body
			astn ptr;
			int stpos = stkoffs(rt, 0);
			#if DEBUGGING > 1
			ipdbg = emitopc(rt, markIP);
			#endif
			for (ptr = ast->stmt.stmt; ptr; ptr = ptr->next) {
				//~ #if DEBUGGING > 1
				//~ ipdbg = emitopc(s, markIP);
				//~ #endif
				if (!cgen(rt, ptr, TYPE_vid)) {		// we will free stack on scope close
					error(rt, ptr->file, ptr->line, "emmiting statement `%+k`", ptr);
					#if DEBUGGING > 1
					debug("cgen[%t->%t](err, %+k)\n%7K", get, ret, ptr, ptr);
					fputasm(stdout, rt, ipdbg, -1, 0x119);
					#endif
				}
			}
			//~ for (sym = ast->type; sym; sym = sym->next);	// destruct?
			if (ast->cst2 == TYPE_rec) {
				debug("%t", get);
				qual = 0;
				get = 0;
			}
			if (get == TYPE_vid && stpos != stkoffs(rt, 0)) {
				if (!emitidx(rt, opc_drop, stpos)) {
					trace("error");
					return 0;
				}
			}
		} break;
		case STMT_if:  {
			int jmpt = 0, jmpf = 0;
			int stpos = stkoffs(rt, 0);
			int tt = eval(&tmp, ast->stmt.test);

			dieif(get != TYPE_vid, "FixMe");

			if (qual == QUAL_sta) {
				//~ debug("static if: `%k`: ", ast);
				if (ast->stmt.step || !tt) {
					error(rt, ast->file, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "else part is invalid");
					return 0;
				}
				qual = 0;
			}

			if (tt && (rt->vm.opti || ast->cst2 == QUAL_sta)) {	// static if then else
				astn gen = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
				if (gen && !cgen(rt, gen, TYPE_any)) {	// leave the stack
					trace("%+k", gen);
					return 0;
				}
			}
			else if (ast->stmt.stmt && ast->stmt.step) {		// if then else
				if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				if (!(jmpt = emitopc(rt, opc_jz))) {
					trace("%+k", ast);
					return 0;
				}
				if (!cgen(rt, ast->stmt.stmt, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				if (!(jmpf = emitopc(rt, opc_jmp))) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(rt, jmpt, emitopc(rt, markIP), 0);
				if (!cgen(rt, ast->stmt.step, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(rt, jmpf, emitopc(rt, markIP), 0);
			}
			else if (ast->stmt.stmt) {							// if then
				if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				//~ if false skip THEN block
				if (!(jmpt = emitopc(rt, opc_jz))) {
					trace("%+k", ast);
					return 0;
				}
				if (!cgen(rt, ast->stmt.stmt, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(rt, jmpt, emitopc(rt, markIP), 0);
			}
			else if (ast->stmt.step) {							// if else
				if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				//~ if true skip ELSE block
				if (!(jmpt = emitopc(rt, opc_jnz))) {
					trace("%+k", ast);
					return 0;
				}
				if (!cgen(rt, ast->stmt.step, TYPE_vid)) {
					trace("%+k", ast);
					return 0;
				}
				fixjump(rt, jmpt, emitopc(rt, markIP), 0);
			}// */

			TODO(destruct(ast->stmt.test))
			if (ast->cst2 != QUAL_sta) {
				logif(stpos != stkoffs(rt, 0), "invalid stacksize(%d:%d) in statement %+k", stkoffs(rt, 0), stpos, ast);
			}// */
		} break;
		case STMT_for: {
			astn jl = rt->cc->jmps;
			int jstep, lcont, lbody, lbreak;
			int stbreak, stpos = stkoffs(rt, 0);

			dieif(get != TYPE_vid, "FixMe");
			if (ast->stmt.init && !cgen(rt, ast->stmt.init, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}

			if (!(jstep = emitopc(rt, opc_jmp))) {		// continue;
				trace("%+k", ast);
				return 0;
			}

			lbody = emitopc(rt, markIP);
			if (ast->stmt.stmt && !cgen(rt, ast->stmt.stmt, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}

			lcont = emitopc(rt, markIP);
			if (ast->stmt.step && !cgen(rt, ast->stmt.step, TYPE_vid)) {
				trace("%+k", ast);
				return 0;
			}

			fixjump(rt, jstep, emitopc(rt, markIP), 0);
			if (ast->stmt.test) {
				if (!cgen(rt, ast->stmt.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}
				if (!emitint(rt, opc_jnz, lbody)) {		// continue;
					trace("%+k", ast);
					return 0;
				}
			}
			else {
				if (!emitint(rt, opc_jmp, lbody)) {		// continue;
					trace("%+k", ast);
					return 0;
				}
			}

			lbreak = emitopc(rt, markIP);
			stbreak = stkoffs(rt, 0);

			while (rt->cc->jmps != jl) {
				astn jmp = rt->cc->jmps;
				rt->cc->jmps = jmp->next;

				if (jmp->go2.stks != stbreak) {
					error(rt, jmp->file, jmp->line, "`%k` statement is invalid due to previous variable declaration within loop", jmp);
					return 0;
				}

				switch (jmp->kind) {
					default : error(rt, jmp->file, jmp->line, "invalid goto statement: %k", jmp); return 0;
					case STMT_brk: fixjump(rt, jmp->go2.offs, lbreak, jmp->go2.stks); break;
					case STMT_con: fixjump(rt, jmp->go2.offs, lcont, jmp->go2.stks); break;
				}
			}

			TODO(destruct(ast->stmt.test))
			if (stpos != stkoffs(rt, 0)) {
				dieif(!emitidx(rt, opc_drop, stpos), "FixMe");
			}
		} break;
		case STMT_con:
		case STMT_brk: {
			int offs;
			dieif(get != TYPE_vid, "FixMe");
			if (!(offs = emitopc(rt, opc_jmp))) {
				trace("%+k", ast);
				return 0;
			}

			ast->go2.offs = offs;
			ast->go2.stks = stkoffs(rt, 0);

			ast->next = rt->cc->jmps;
			rt->cc->jmps = ast;
		} break;
		case STMT_ret: {
			TODO("declared reference variables should be freed.");
			int bppos = stkoffs(rt, 0);
			//~ trace("ss: %d, sm: %d, ro: %d", rt->vm.ss, rt->vm.sm, rt->vm.ro);
			if (get == TYPE_vid && rt->vm.ro != stkoffs(rt, 0)) {
				if (!emitidx(rt, opc_drop, rt->vm.ro)) {
					trace("leve %d", rt->vm.ro);
					return 0;
				}
			}
			if (!emitopc(rt, opc_jmpi)) {
				trace("%+k", ast);
				return 0;
			}
			fixjump(rt, 0, 0, bppos);
		} break;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/call/cast
			int stktop = stkoffs(rt, 0);
			int stkret = stkoffs(rt, sizeOf(ast->type));
			astn argv = ast->op.rhso;
			symn var = linkOf(ast->op.lhso);

			dieif(!var && ast->op.lhso, "FixMe[%+k]", ast);

			// inline
			if (var && var->kind == TYPE_def) {
				int chachedArgSize = 0;
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

						if (rt->vm.opti && eval(&tmp, arg)) {
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
							int stktop = stkoffs(rt, sizeOf(as));
							chachedArgSize += sizeOf(as);
							//~ warn(s, 9, ast->file, ast->line, "caching argument: %-T = %+k", as, arg);
							if (!cgen(rt, arg, as->cast)) {
								trace("%+k", arg);
								return 0;
							}
							as->offs = stkoffs(rt, 0);
							as->kind = TYPE_ref;
							if (stktop != as->offs) {
								error(rt, ast->file, ast->line, "invalid initializer size: %d <> got(%d): `%+k`", stktop, as->offs, as);
								return 0;
							}
						}
						an = an->next;
						as = as->next;
					}
					dieif(an || as, "Error");
				}

				if (!(ret = cgen(rt, var->init, ret))) {
					trace("%+k", ast);
					return 0;
				}

				if (stkret != stkoffs(rt, 0)) {
					logif(chachedArgSize != stkoffs(rt, 0) - stkret, "%+T(%d%+d): %+k", ast->type, stkret, stkoffs(rt, 0)-stkret, ast);
					if (get != TYPE_vid) {
						if (!emitidx(rt, opc_ldsp, stkret)) {	// get stack
							trace("error");
							return 0;
						}
						if (!emitint(rt, opc_sti, sizeOf(ast->type))) {
							error(rt, ast->file, ast->line, "store indirect: %T:%d of `%+k`", ast->type, sizeOf(ast->type), ast);
							return 0;
						}
					}
					if (stkret < stkoffs(rt, 0) && !emitidx(rt, opc_drop, stkret)) {
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

			// push Result, args
			if (var && !isType(var) && var != emit_opc && var->call) {
				//~ trace("call %-T: %-T: %d", var, ast->type, var->type->size);
				if (sizeOf(var->type) && !emitint(rt, opc_spc, sizeOf(var->type))) {
					trace("%+k", ast);
					return 0;
				}
				// result size on stack
				dieif(stkret != stkoffs(rt, 0), "FixMe");
			}
			if (argv) {
				astn argl = NULL;

				while (argv->kind == OPER_com) {
					astn arg = argv->op.rhso;

					if (!cgen(rt, arg, argv->cst2)) {
						trace("%+k", arg);
						return 0;
					}
					arg->next = argl;
					argl = arg;
					argv = argv->op.lhso;
				}

				if (var == emit_opc && istype(argv)) {
					break;
				}

				if (!cgen(rt, argv, argv->cst2)) {
					trace("%+k", argv);
					return 0;
				}

				if (var && var != emit_opc && var->call && var->cast != TYPE_ref) {
					if (var->args->offs != stkoffs(rt, 0) - stktop) {
						symn arg = var->args;
						trace("args:%T(%d != %d(%d - %d))", var, var->args->offs, stkoffs(rt, 0) - stktop, stkoffs(rt, 0), stktop);
						//~ warn(s, 1, ast->file, ast->line, "args:%T(%d != %d(%d - %d))", var, var->args->offs, stkoffs(s, 0) - stktop, stkoffs(s, 0), stktop);
						while (arg != NULL) {
							trace("arg %T@%d", arg, arg->offs);
							arg = arg->next;
						}
						return 0;
						//~ dieif(var->args->offs != stkoffs(s, 0) - stkret, "FixMe args:%T(%d != %d(%d - %d))", var, var->args->offs, stkoffs(s, 0) - stkret, stkoffs(s, 0), stkret);
					}
				}

				argv->next = argl;
				argl = argv;
			}

			//~ /*
			if (isType(var)) {				// cast()
				//~ debug("cast: %+k", ast);
				if (var == emit_opc) {	// emit()
					debug("emit: %+k", ast);
				}
				if (var == type_vid) {	// void(...)
					//~ debug("emit: %+k", ast);
				}
				else if (!argv || argv != ast->op.rhso)
					warn(rt, 1, ast->file, ast->line, "multiple values, array ?: '%+k'", ast);
			}
			else // */
			if (var) {					// call()
				//~ debug("call: %+k", ast);
				if (var == emit_opc) {	// emit()
				}
				else if (var->call) {
					if (!cgen(rt, ast->op.lhso, TYPE_ref)) {
						trace("%+k", ast);
						return 0;
					}
					if (!emitopc(rt, opc_call)) {
						trace("%+k", ast);
						return 0;
					}
					// clean the stack
					if (stkret != stkoffs(rt, 0) && !emitidx(rt, opc_drop, stkret)) {
						trace("%+k", ast);
						return 0;
					}
				}
				else {
					//? can have an operator: call() with this args
					error(rt, ast->file, ast->line, "called object is not a function: %+T", var);
					return 0;
				}
			}
			else {							// ()
				dieif(ast->op.lhso, "FixMe %+k:%+k", ast, ast->op.lhso);
				if (!argv || argv != ast->op.rhso /* && cast*/)
					warn(rt, 1, ast->file, ast->line, "multiple values, array ?: '%+k'", ast);
			}
		} break;
		case OPER_idx: {	// '[]'

			if (!cgen(rt, ast->op.lhso, TYPE_ref)) {
				trace("%+k", ast);
				return 0;
			}

			if (rt->vm.opti && eval(&tmp, ast->op.rhso) == TYPE_int) {
				int offs = sizeOf(ast->type) * constint(&tmp);
				/*
				symn arr = ast->op.lhso->type;
				if (arr && arr->init && constint(&tmp) >= arr->size) {
					error(s, ast->file, ast->line, "index out of bounds: %+k", ast);
				}// */
				if (!emitinc(rt, offs)) {
					trace("%+k", ast);
					return 0;
				}
			}
			else {
				int size = sizeOf(ast->type);	// size of array element
				if (!cgen(rt, ast->op.rhso, TYPE_u32)) {
					trace("%+k", ast);
					return 0;
				}
				if (size > 1) {
					TODO("an index operator (mad with immediate value should be used)")
					if (!emiti32(rt, size)) {
						trace("%+k", ast);
						return 0;
					}
					if (!emitopc(rt, u32_mad)) {
						trace("%+k", ast);
						return 0;
					}
				}
				else if (size == 1) {
					if (!emitopc(rt, i32_add)) {
						trace("%+k", ast);
						return 0;
					}
				}
				else {
					// size of array element size error
					fatal("FixMe");
					return 0;
				}
			}

			/* we hawe some problems with arrrays of references
			if (ast->type->cast == TYPE_ref/ *  && ast->cst2 != ASGN_set * /) {
				if (!emitint(s, opc_ldi, 4)) {
					trace("%+k", ast);
					return 0;
				}
			}// */

			if (get == ASGN_set) {
				/*if (var->attr & ATTR_const)
					error(s, ast->file, ast->line, "constant field assignment %T", var);
				//~ */
				//~ get = TYPE_ref;
				return TYPE_ref;
			}

			if (get == TYPE_ref && ast->type->cast != TYPE_ref)
				return TYPE_ref;

			if (get == TYPE_any && ret == TYPE_ref) {
				get = TYPE_ref;
			}

			if (!emitint(rt, opc_ldi, sizeOf(ast->type))) {
				trace("%+k", ast);
				return 0;
			}
		} break; // */
		case OPER_dot: {	// '.'
			// TODO: this should work as indexing
			symn var = linkOf(ast->op.rhso);

			if (istype(ast->op.lhso)) {
				return cgen(rt, ast->op.rhso, get);
			} // */

			if (!var) {
				trace("%+k", ast);
				return 0;
			}

			if (get == ASGN_set) {
				if (var->attr & ATTR_const)
					error(rt, ast->file, ast->line, "constant field assignment %T", var);
				get = TYPE_ref;
			}

			if (var->kind == TYPE_def) {
				// static array length is of this type
				debug("%+T", var);
				return cgen(rt, ast->op.rhso, get);
			}

			if (!cgen(rt, ast->op.lhso, TYPE_ref)) {
				trace("%+k", ast);
				return 0;
			}

			if (!emitinc(rt, var->offs)) {
				trace("%+k", ast);
				return 0;
			}

			if (var->cast == TYPE_ref/* && ast->cast == TYPE_ref*/) {
				if (!emitint(rt, opc_ldi, 4)) {
					trace("%+k", ast);
					return 0;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emitint(rt, opc_ldi, sizeOf(ast->type))) {
				trace("%+k", ast);
				return 0;
			}
		} break;

		case OPER_adr:
		case OPER_not:		// '!'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			int opc = -1;
			switch (ast->kind) {
				default: fatal("FixMe"); return 0;
				case OPER_pls: return cgen(rt, ast->op.rhso, get);
				case OPER_adr: return cgen(rt, ast->op.rhso, get);
				case OPER_mns: opc = opc_neg; break;
				case OPER_not: opc = opc_not; break;
				case OPER_cmt: opc = opc_cmt; break;
			}
			if (!(ret = cgen(rt, ast->op.rhso, TYPE_any))) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(rt, opc, ret)) {
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
				default: fatal("FixMe");
			}
			if (!cgen(rt, ast->op.lhso, TYPE_any)) {
				trace("%+k", ast);
				return 0;
			}
			if (!cgen(rt, ast->op.rhso, TYPE_any)) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(rt, opc, ast->op.lhso->cst2)) {	// uint % int => u32.mod
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
				default:
					break;
			}
			#endif
		} break;

		case OPER_lnd:		// '&&'
		case OPER_lor: {	// '||'
			int opc = -1;
			switch (ast->kind) {
				default: fatal("FixMe"); return 0;
				case OPER_lnd: opc = opc_and; break;
				case OPER_lor: opc = opc_ior; break;
			}
			if (!cgen(rt, ast->op.lhso, TYPE_bit)) {
				trace("%+k", ast);
				return 0;
			}
			if (!cgen(rt, ast->op.rhso, TYPE_bit)) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(rt, opc, TYPE_u32)) {
				trace("opc__%02x:%+k", opc, ast);
				return 0;
			}

			if (DEBUGGING) {
				static int firstTimeShowOnly = 1;
				if (firstTimeShowOnly) {
					warn(rt, 4, ast->file, ast->line, "operators `&&` and `||` does not short-circuit yet", ast);
					firstTimeShowOnly = 0;
				}
			}
			#if DEBUGGING
			// these must be true
			//~ dieif(ast->op.lhso->cst2 != ast->op.rhso->cst2, "RemMe", ast);
			dieif(ast->op.lhso->cst2 != TYPE_bit, "RemMe", ast);
			dieif(ast->op.lhso->cst2 != TYPE_bit, "RemMe", ast);
			dieif(ret != castOf(ast->type), "RemMe");
			dieif(ret != TYPE_bit, "RemMe");
			#endif
		} break;// */
		case OPER_sel: {	// '?:'
			int jmpt, jmpf;
			if (0 && rt->vm.opti && eval(&tmp, ast->op.test)) {
				if (!cgen(rt, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, TYPE_any)) {
					trace("%+k", ast);
					return 0;
				}
			}
			else {
				int bppos = stkoffs(rt, 0);

				if (!cgen(rt, ast->op.test, TYPE_bit)) {
					trace("%+k", ast);
					return 0;
				}

				if (!(jmpt = emitopc(rt, opc_jz))) {
					trace("%+k", ast);
					return 0;
				}

				if (!cgen(rt, ast->op.lhso, TYPE_any)) {
					trace("%+k", ast);
					return 0;
				}

				if (!(jmpf = emitopc(rt, opc_jmp))) {
					trace("%+k", ast);
					return 0;
				}

				fixjump(rt, jmpt, emitopc(rt, markIP), bppos);

				if (!cgen(rt, ast->op.rhso, TYPE_any)) {
					trace("%+k", ast);
					return 0;
				}

				fixjump(rt, jmpf, emitopc(rt, markIP), -sizeOf(ast->type));
			}
		} break;

		case ASGN_set: {	// ':='
			int byRef = ast->op.lhso->kind == OPER_adr || ast->type->cast == TYPE_ref;

			if (byRef) {
				ret = TYPE_ref;
			}

			if (!cgen(rt, ast->op.rhso, ret)) {
				trace("%+k", ast);
				return 0;
			}

			if (get != TYPE_vid) {
				// in case a = b = sum(2, 700);
				// dupplicate the result
				if (!emitint(rt, opc_ldsp, 0)) {
					trace("%+k", ast);
					return 0;
				}
				if (!emitint(rt, opc_ldi, sizeOf(ast->type))) {
					trace("%+k", ast);
					return 0;
				}
			}
			else
				ret = get;

			//~ debug("Assignment (%k<-%k):%k `%+t`", ast->op.lhso->cst2, ast->op.rhso->cst2, ast->cst2, ast);

			if (!cgen(rt, ast->op.lhso, ASGN_set)) {
				trace("%+k", ast);
				return 0;
			}
			if (!emitint(rt, opc_sti, byRef ? 4 : sizeOf(ast->type))) {
				trace("(%+k):%-T(%d):%t", ast, ast->type, sizeOf(ast->type), ast->cst2);
				return 0;
			}
		} break;
		//}
		//{ TVAL
		//~ case TYPE_bit:	// no constant of this kind
		case TYPE_int: switch (get) {
			//~ case TYPE_rec: return emiti32(s, ast->con.cint) ? TYPE_i32 : 0;
			case TYPE_vid: return TYPE_vid;
			case TYPE_bit: return emiti32(rt, constbol(ast)) ? TYPE_u32 : 0;
			case TYPE_u32: return emiti32(rt, (uint32_t)ast->con.cint) ? TYPE_u32 : 0;
			case TYPE_i32: return emiti32(rt, (int32_t)ast->con.cint) ? TYPE_i32 : 0;
			case TYPE_i64: return emiti64(rt, ast->con.cint) ? TYPE_i64 : 0;
			case TYPE_f32: return emitf32(rt, (float32_t)ast->con.cint) ? TYPE_f32 : 0;
			case TYPE_f64: return emitf64(rt, (float64_t)ast->con.cint) ? TYPE_f64 : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast); return 0;
		} break;
		case TYPE_flt: switch (get) {
			case TYPE_vid: return TYPE_vid;
			case TYPE_bit: return emiti32(rt, constbol(ast)) ? TYPE_u32 : 0;
			case TYPE_u32: return emiti32(rt, (uint32_t)ast->con.cflt) ? TYPE_u32 : 0;
			case TYPE_i32: return emiti32(rt, (int32_t)ast->con.cflt) ? TYPE_i32 : 0;
			case TYPE_i64: return emiti64(rt, ast->con.cflt) ? TYPE_i64 : 0;
			case TYPE_f32: return emitf32(rt, (float32_t)ast->con.cflt) ? TYPE_f32 : 0;
			case TYPE_f64: return emitf64(rt, (float64_t)ast->con.cflt) ? TYPE_f64 : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast); return 0;
		} break;
		case TYPE_str: switch (get) {
			case TYPE_vid: return TYPE_vid;
			case TYPE_arr: return emitref(rt, ast->id.name) ? TYPE_arr : 0;
			case TYPE_ref: return emitref(rt, ast->id.name) ? TYPE_ref : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast); return 0;
		} break;

		case TYPE_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->id.link;		// link
			dieif(!typ || !var, "FixMe");

			switch (var->kind) {
				case TYPE_ref: {

					if (!genRef(rt, var->offs)) {
						trace("%+k", ast);
						return 0;
					}

					if (var->cast == TYPE_ref && ast->cst2 != ASGN_set) {
						if (!emitint(rt, opc_ldi, 4)) {
							trace("%+k", ast);
							return 0;
						}
					}// */

					if (get == ASGN_set) {
						if (var->attr & ATTR_const)
							error(rt, ast->file, ast->line, "constant field assignment %T", var);
						get = TYPE_ref;
					}

					if (get != TYPE_ref) {
						if (!emitint(rt, opc_ldi, sizeOf(typ))) {
							trace("%+k", ast);
							return 0;
						}
					}
					else {
						ret = TYPE_ref;
					}
				} break;
				case EMIT_opc:
					dieif(get == TYPE_ref, "FixMe");
					if (!emitint(rt, var->offs, var->init ? constint(var->init) : 0)) {
						error(rt, ast->file, ast->line, "error emiting opcode: %+k", ast);
						if (stkoffs(rt, 0) > 0)
							info(rt, ast->file, ast->line, "%+k underflows stack.", ast);
						return 0;
					}
					return TYPE_vid;

				case TYPE_def:
					TODO("this sucks, but it works")
					if (var->init)
						return cgen(rt, var->init, get);
				default:
					error(rt, ast->file, ast->line, "invalid rvalue `%+k:` %t", ast, var->kind);
					return 0;
			}
		} break;
		case TYPE_def: {					// new (var, func, define)
			symn typ = ast->type;
			symn var = ast->id.link;
			int stktop = stkoffs(rt, sizeOf(var));
			dieif(!typ || !var, "FixMe");

			if (1/* s->cc->sini */) {
				struct astn init;
				symn sym;
				init.kind = TYPE_def;
				init.file = NULL;
				init.line = 0;
				for (sym = var->stat; sym; sym = sym->next) {
					init.type = sym->type;
					init.id.link = sym;
					rt->cc->sini = 1;
					cgen(rt, &init, TYPE_vid);
					//~ logif(1, "%-T.%-T", var, sym);
				}
			}

			if (var->kind == TYPE_ref) {

				//~ trace("new variable: `%-T`", var);

				if (var->offs && var->init && !rt->cc->sini) {
					return get;
				}

				if (var->init) {
					astn val = var->init;

					//~ logif(1, "new %T, %d", var, var->line);
					//~ debug("new %T, %d", var, var->line);
					if (var->call || var->cast == TYPE_ref) {
						ret = TYPE_ref;
					}
					// var a = emit(...);
					else if (val->type == emit_opc) {
						ret = get;
					}
					// var a = null;
					else if (var->cast == TYPE_ref && val->id.link == null_ref) {
						debug("assigning null");
						val->cst2 = ret = TYPE_ref;
					}
					// int64 a[256] = 1, ...
					else if (var->cast != TYPE_ref && typ->kind == TYPE_arr && typ->size > 0) {
						int i, esize;//, asize = sizeOf(typ);//,  = sizeOf(typ->type);
						symn base = typ;
						int nelem = 1;
						int cast = 0;

						while (base->kind == TYPE_arr) {
							if (base->size <= 0)
								break;
							nelem *= base->size;
							base = base->type;
						}

						if (base->kind != TYPE_arr) {
							switch (base->cast) {
								//~ case TYPE_vid:
								case TYPE_bit:
								case TYPE_u32:
								case TYPE_i32:
								case TYPE_i64:
								case TYPE_f32:
								case TYPE_f64:
								//~ case TYPE_rec:
								//~ case TYPE_ref:
									cast = base->cast;
									break;
							}
							if (!cast) {
								error(rt, ast->file, ast->line, "invalid array initialization");
								trace("%+k", ast);
								return 0;
							}
						}

						esize = sizeOf(base);
						if (!emitint(rt, opc_spc, esize * nelem)) {
							trace("%+k", ast);
							return 0;
						}

						var->offs = stkoffs(rt, 0);
						if (var->offs < esize * nelem) {
							error(rt, ast->file, ast->line, "stack underflow", var->offs, esize * nelem);
							return 0;
						}

						// int a[12] = 0,1,2,3,4,5,6,7;
						if (val->kind == OPER_com) {
							int ninit = 1;
							astn tmp = NULL;

							//~ info(s, ast->file, ast->line, "array initialization list");
							while (val->kind == OPER_com) {

								// stringify
								val->op.rhso->next = tmp;
								tmp = val->op.rhso;

								// advance and count
								val = val->op.lhso;
								ninit += 1;
							}
							val->next = tmp;

							if (ninit > nelem) {
								error(rt, ast->file, ast->line, "Too many initializers");
								return 0;
							}
							if (ninit < nelem) {
								warn(rt, 4, ast->file, ast->line, "Only the first %d of %d elements will be initialized", ninit, nelem);
							}

							for (i = 0; val; val = val->next, ++i) {
								if (!cgen(rt, val, cast)) {
									trace("%+k", ast);
									return 0;
								}
								if (!genRef(rt, var->offs - i * esize)) {
									trace("%+k", ast);
									return 0;
								}
								if (!emitint(rt, opc_sti, esize)) {
									trace("%+k", ast);
									return 0;
								}
							}
						}
						// int a[12] = 0;
						else if (var->cast != TYPE_ref) {
							//val->cst2 = cast;
							//~ info(s, ast->file, ast->line, "array initialization value");
							for (i = 0; i < nelem; ++i) {
								if (!cgen(rt, val, cast)) {
									trace("%+k", ast);
									return 0;
								}
								if (!genRef(rt, var->offs - i * esize)) {
									trace("%+k", ast);
									return 0;
								}
								if (!emitint(rt, opc_sti, esize)) {
									trace("%+k", ast);
									return 0;
								}
							}
						}
						return get;
					}// */
					else if (val->type != typ && !promote(val->type, typ)) {
						error(rt, ast->file, ast->line, "incompatible types in initialization(%-T := %-T)", typ, val->type);
					}

					//~ if (s->vm.opti && eval(&tmp, val)) {val = &tmp;}
					//~ debug("cast(`%+k`, %t):%t", ast, get, ret);

					if (!cgen(rt, val, var->cast)) {
						trace("%+k", ast);
						return 0;
					}

					// create local variable
					if (var->offs == 0) {
						var->offs = stkoffs(rt, 0);
						if (stktop != var->offs && val->type != emit_opc) {
							error(rt, ast->file, ast->line, "invalid initializer size: %d <> got(%d): `%+k`", stktop, var->offs, val);
							return 0;
						}
					}
					// initialize gloabal
					else {
						if (!genRef(rt, var->offs)) {
							trace("%+k", ast);
							return 0;
						}
						if (!emitint(rt, opc_sti, sizeOf(var))) {
							trace("%+k:sizeof(%-T) = %d", ast, var, sizeOf(var));
							return 0;
						}
						var->init = NULL;
						return ret;
					}
				}
				else if (!var->offs) {		// static alloc a block of the size of the type;
					int size = sizeOf(typ);	// var->size;
					if (!emitint(rt, opc_spc, size)) {
						trace("%+k", ast);
						return 0;
					}
					var->offs = stkoffs(rt, 0);
					if (var->offs < size) {
						error(rt, ast->file, ast->line, "stack underflow", var->offs, size);
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
			case TYPE_ref: break;
			default: goto errorcast2;
		} break;// */

		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32:
			case TYPE_i32:
				if (!emitopc(rt, i32_bol)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_i64:
				if (!emitopc(rt, i64_bol)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(rt, f32_bol)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(rt, f64_bol)) {
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
				if (!emitopc(rt, i64_i32)) {
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
				if (!emitopc(rt, i64_i32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(rt, f32_i32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(rt, f64_i32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			//~ case TYPE_ref: if (!emitopc(s, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i64: switch (ret) {
			case TYPE_bit:
			case TYPE_u32:
				if (!emitopc(rt, u32_i64)) {
					trace("%+k", ast);
					return 0; 
				}
				break;
			case TYPE_i32:
				if (!emitopc(rt, i32_i64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(rt, f32_i64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(rt, f64_i64)) {
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
				if (!emitopc(rt, i32_f32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_i64: 
				if (!emitopc(rt, i64_f32)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f64:
				if (!emitopc(rt, f64_f32)) {
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
				if (!emitopc(rt, i32_f64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_i64: 
				if (!emitopc(rt, i64_f64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case TYPE_f32:
				if (!emitopc(rt, f32_f64)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			//~ case TYPE_ref: if (!emitopc(s, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;

		case TYPE_ref:
			trace("cgen[%t->%t](%+k)\n%7K", ret, get, ast, ast);
			error(rt, ast->file, ast->line, "invalid rvalue: %+k", ast);
			return 0;
		default:
			fatal("%d: unimplemented(cast for `%+k`, %t):%t", ast->line, ast, get, ret);
		errorcast2:
			trace("cgen[%t->%t](%+k)\n%7K", ret, get, ast, ast);
			return 0;
	}

	// zero extend ...
	if (get == TYPE_u32) {
		switch (ast->type->size) {
			case 1:
				if (!emitint(rt, b32_bit, b32_bit_and | 8)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case 2:
				if (!emitint(rt, b32_bit, b32_bit_and | 16)) {
					trace("%+k", ast);
					return 0;
				}
				break;
			case 4: break;
			default:
				fatal("FixMe");
				break;
		}
	}

	dieif (qual, "unimplemented `%+k`: %t", ast, qual);
	return ret;
}

TODO("ggen shold go to cgen")
int ggen(state rt, symn var/* , astn add */) {
	ccState cc = rt->cc;
	if (var->call && var->cast != TYPE_ref) {
		int seg = emitopc(rt, markIP);

		if (!var->init) {
			debug("`%T` will be skipped", var);
			//~ continue;
			return 0;
		}

		rt->vm.sm = 0;
		fixjump(rt, 0, 0, 4 + var->offs);
		rt->vm.ro = stkoffs(rt, 0);
		rt->vm.sm = rt->vm.ss;	// leave return address on stack

		var->offs = -emitopc(rt, markIP);

		if (!cgen(rt, var->init, TYPE_vid)) {
			return 0;
		}
		if (!emitopc(rt, opc_jmpi)) {
			error(rt, var->file, var->line, "error emiting function: %-T", var);
			return 0;
		}
		while (cc->jmps) {
			error(rt, NULL, 0, "invalid jump: `%k`", cc->jmps);
			cc->jmps = cc->jmps->next;
		}
		var->size = emitopc(rt, markIP) - seg;

		//~ debug("function @%d: %+T\n%7K", -var->offs, var, var->init);
		//~ debug("function @%d: %-T", -var->offs, var, var->init);
		//~ fputasm(stderr, vm, seg, -1, 0x129);

		//~ trace("static function: %-T@%x", var, -var->offs);
		var->init = NULL;
	}
	else {
		dieif(var->offs, "FixMe %-T@%d", var, var->offs);
		var->size = sizeOf(var);
		var->offs = -rt->vm.pos;
		rt->vm.pos += var->size;

		/* TODO: delete Me
		if (var->init) {
		//~ if (var->init && !(var->attr & ATTR_glob)) {
			//~ if variable was allocated
			//~ make assigment from initializer

			astn init = newnode(cc, ASGN_set);

			init->op.lhso = lnknode(cc, var);
			init->op.rhso = var->init;
			init->type = var->type;
			init->cst2 = TYPE_vid;

			if (var->cast == TYPE_ref) {
				init->op.lhso->cst2 = ASGN_set;
				init->op.lhso = opnode(cc, OPER_adr, NULL, init->op.lhso);
				init->op.lhso->type = type_ptr;
			}

			init = opnode(cc, STMT_do, NULL, init);
			init->type = type_vid;
			init->file = var->file;
			init->line = var->line;

			dieif(add == NULL, "FixMe");
			if (add->list.head == NULL) {
				add->list.head = init;
			}
			else {
				add->list.tail->next = init;
			}
			add->list.tail = init;

			//~ st->next = NULL;

			var->init = NULL;
		}// */

		//~ trace("static variable: %+T@%x:%d", var, -var->offs, sizeOf(var));
	}

	//~ trace("static variable: %-T@%x:%d", var, -var->offs, sizeOf(var));
	//~ logif(1, "static variable: %-T@%x:%d = %+k", var, -var->offs, var->size, var->init);
	return 1;
}

//{ symbols: install and query
symn ccBegin(state rt, char *cls) {
	symn result = NULL;
	if (rt->cc) {
		result = install(rt->cc, cls, TYPE_rec, TYPE_vid, 0);
		if (result) {
			enter(rt->cc, NULL);
		}
	}
	return result;
}
void ccEnd(state rt, symn cls) {
	if (cls) {
		cls->args = leave(rt->cc, cls, 1);
	}
}

symn ccDefineInt(state rt, char *name, int32_t value) {
	name = mapstr(rt->cc, name, -1, -1);
	return installex(rt->cc, name, TYPE_def, 0, type_i32, intnode(rt->cc, value));
}
symn ccDefineFlt(state rt, char *name, double value) {
	name = mapstr(rt->cc, name, -1, -1);
	return installex(rt->cc, name, TYPE_def, 0, type_f64, fltnode(rt->cc, value));
}
symn ccDefineStr(state rt, char *name, char* value) {
	name = mapstr(rt->cc, name, -1, -1);
	value = mapstr(rt->cc, value, -1, -1);
	return installex(rt->cc, name, TYPE_def, 0, type_str, strnode(rt->cc, value));
}

symn findsym(ccState s, symn in, char *name) {
	struct astn ast;
	memset(&ast, 0, sizeof(struct astn));
	ast.kind = TYPE_ref;
	//ast.id.hash = rehash(name);
	ast.id.name = name;
	return lookup(s, in ? in->args : s->s->defs, &ast, NULL, 1);
}
symn findref(state rt, void *ptr) {
	int offs;
	symn sym = NULL;

	if (ptr == NULL) {
		trace("null");
		return NULL;
	}

	if (ptr < (void*)rt->_mem) {
		fatal("Error");
		return NULL;
	}

	if ((unsigned char*)ptr > rt->_mem + rt->vm.pc) {		// pc 
		fatal("Error");
		return NULL;
	}

	offs = -((unsigned char*)ptr - rt->_mem);
	for (sym = rt->gdef; sym; sym = sym->gdef) {
		if (sym->offs == offs)
			return sym;
	}

	return NULL;
}

int symvalint(symn sym, int* res) {
	struct astn ast;
	if (sym && eval(&ast, sym->init)) {
		*res = constint(&ast);
		return 1;
	}
	return 0;
}
int symvalflt(symn sym, double* res) {
	struct astn ast;
	if (sym && eval(&ast, sym->init)) {
		*res = constflt(&ast);
		return 1;
	}
	return 0;
}
//}

TODO("this should be called assemble or something similar")
int gencode(state rt, int level) {
	ccState cc = rt->cc;
	libc lc = NULL;
	int Lmain;

	if (rt->errc)
		return -2;

	dieif(cc == NULL, "invalid enviroment: erors: %d", rt->errc);

	// leave global + static
	rt->defs = leave(rt->cc, NULL, level >= 0);

	/* reorder the creation/initialization of static variables.
	 * ex: g must be generated before f
	 *	int f() {
	 *		static int g = 9;
	 *		// ...
	 *	}
	 */

	if (1) {
		symn ng, pg = NULL;
		for (ng = rt->gdef; ng; ng = ng->gdef) {
			symn Ng, Pg = NULL;

			//~ trace("checking symbol %-T", ng);
			for (Ng = ng; Ng; Ng = Ng->gdef) {
				if (Ng->decl == ng) {
					break;
				}
				Pg = Ng;
			}

			//~ this must be generated before sym;
			if (Ng) {
				//~ trace("symbol %-T before %-T", Ng, ng);
				Pg->gdef = Ng->gdef;	// remove

				Ng->gdef = ng;
				if (pg)
					pg->gdef = Ng;
				else
					rt->gdef = Ng;

				ng = pg;

			}

			pg = ng;
		}
	}// */

	// allocate used memeory
	rt->vm.pos = rt->_ptr - rt->_mem;
	rt->vm.opti = level < 0 ? -level : level;

	initBuff(&cc->dbg, 128, sizeof(struct list));

	TODO("Data[RO]: strings, typeinfos, constants, functions, ?")
	rt->vm.ro = rt->vm.pos;

	// libcalls
	if (cc->libc) {
		rt->libv = rt->_mem + rt->vm.pos;
		rt->vm.pos += sizeof(struct libc) * (cc->libc->pos + 1);
		for (lc = cc->libc; lc; lc = lc->next) {
			((struct libc*)rt->libv)[lc->pos] = *lc;
		}
	}

	// static vars & functions
	if (rt->defs) {
		symn var = rt->defs;

		// we will append the list of declarations here.
		//~ astn init = newnode(cc, STMT_beg);

		dieif(rt->defs != rt->cc->defs, "FixMe");

		// mark global variables, and make them static if needed.
		if (level >= 0) {
			for (var = rt->defs; var; var = var->next) {

				if (var->kind != TYPE_ref)
					continue;

				var->attr |= ATTR_glob | ATTR_stat;
			}
		}

		// generate global and static variables & functions
		for (var = rt->gdef; var; var = var->gdef) {

			if (var == null_ref)
				continue;

			if (var->kind != TYPE_ref)
				continue;

			if (!(var->attr & (ATTR_glob | ATTR_stat)))
				continue;

			dieif(var->kind != TYPE_ref, "FixMe");

			cc->sini = 0;
			if (!ggen(rt, var/* , init */)) {
				trace("FixMe");
				return 0;
			}// */
		}

		/*/ initialize static non global variables
		if (init && init->list.tail / * && cc->root * /) {
			dieif(!cc->root || cc->root->kind != STMT_beg, "FixMe");
			init->list.tail->next = cc->root->stmt.stmt;
			cc->root->stmt.stmt = init->list.head;
		}// */

	}

	Lmain = rt->vm.pos;
	if (1 && cc->root) {
		int seg = rt->vm.pos;
		rt->vm.ss = rt->vm.sm = 0;

		cc->sini = 1;
		// pass TYPE_vid to clear the stack
		if (!cgen(rt, cc->root, TYPE_vid))
			fputasm(stderr, rt, seg, -1, 0x10);

		while (cc->jmps) {
			error(rt, NULL, 0, "invalid jump: `%k`", cc->jmps);
			cc->jmps = cc->jmps->next;
		}
	}

	rt->vm.px = emiti32(rt, 0);
	emitint(rt, opc_libc, 0);

	// program entry point
	rt->vm.pc = Lmain;

	rt->_used = rt->_free = NULL;
	rtAlloc(rt, NULL, 0);
	/*freemem = (void*)padded((int)(s->_mem + s->vm.px + 16), 16);
	freemem->next = NULL;
	freemem->data = (unsigned char*)freemem + sizeof(struct list);
	freemem->size = s->_size - (freemem->data - s->_mem);

	s->_used = NULL;
	s->_free = freemem;
	*/

	rt->_ptr = NULL;

	return rt->errc;
}

symn emit_opc = NULL;
static void install_emit(ccState cc, int mode) {
	symn typ;
	symn type_v4f = NULL;

	if (!(mode & creg_emit))
		return;

	emit_opc = install(cc, "emit", symn_const | EMIT_opc, 0, 0);

	if (!emit_opc)
		return;

	if (emit_opc && (mode & creg_eopc) == creg_eopc) {

		symn u32, i32, i64, f32, f64, v4f, v2d, ref, val;
		enter(cc, NULL);

		ref = install(cc, "ref", TYPE_rec, TYPE_ref, 4);

		u32 = install(cc, "u32", TYPE_rec, TYPE_u32, 4);
		i32 = install(cc, "i32", TYPE_rec, TYPE_i32, 4);
		i64 = install(cc, "i64", TYPE_rec, TYPE_i64, 8);
		f32 = install(cc, "f32", TYPE_rec, TYPE_f32, 4);
		f64 = install(cc, "f64", TYPE_rec, TYPE_f64, 8);

		v4f = install(cc, "v4f", TYPE_rec, TYPE_rec, 16);
		v2d = install(cc, "v2d", TYPE_rec, TYPE_rec, 16);
		val = install(cc, "val", TYPE_rec, TYPE_rec, -1);

		installex(cc, "nop", EMIT_opc, opc_nop, type_vid, NULL);
		installex(cc, "not", EMIT_opc, opc_not, type_bol, NULL);

		installex(cc, "pop", EMIT_opc, opc_drop, type_vid, intnode(cc, 1));
		installex(cc, "set", EMIT_opc, opc_set1, type_vid, intnode(cc, 1));
		//~ installex(cc, "set0", EMIT_opc, opc_set1, type_vid, intnode(cc, 0));
		//~ installex(cc, "set1", EMIT_opc, opc_set1, type_vid, intnode(cc, 1));

		if ((typ = ccBegin(cc->s, "dupp"))) {
			installex(cc, "x1", EMIT_opc, opc_dup1, type_i32, intnode(cc, 0));
			installex(cc, "x2", EMIT_opc, opc_dup2, type_i64, intnode(cc, 0));
			installex(cc, "x4", EMIT_opc, opc_dup4, type_vid, intnode(cc, 0));
			ccEnd(cc->s, typ);
		}
		if ((typ = install(cc, "ldz", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "x1", EMIT_opc, opc_ldz1, type_vid, intnode(cc, 0));
			installex(cc, "x2", EMIT_opc, opc_ldz2, type_vid, intnode(cc, 0));
			installex(cc, "x4", EMIT_opc, opc_ldz4, type_vid, intnode(cc, 0));
			typ->args = leave(cc, typ, 1);
		}
		if ((typ = install(cc, "load", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "b8",   EMIT_opc, opc_ldi1, type_vid, NULL);
			installex(cc, "b16",  EMIT_opc, opc_ldi2, type_vid, NULL);
			installex(cc, "b32",  EMIT_opc, opc_ldi4, type_vid, NULL);
			installex(cc, "b64",  EMIT_opc, opc_ldi8, type_vid, NULL);
			installex(cc, "b128", EMIT_opc, opc_ldiq, type_vid, NULL);
			typ->args = leave(cc, typ, 1);
		}
		if ((typ = install(cc, "store", TYPE_rec, TYPE_vid, 0))) {
			enter(cc, NULL);
			installex(cc, "b8",   EMIT_opc, opc_sti1, type_vid, NULL);
			installex(cc, "b16",  EMIT_opc, opc_sti2, type_vid, NULL);
			installex(cc, "b32",  EMIT_opc, opc_sti4, type_vid, NULL);
			installex(cc, "b64",  EMIT_opc, opc_sti8, type_vid, NULL);
			installex(cc, "b128", EMIT_opc, opc_stiq, type_vid, NULL);
			typ->args = leave(cc, typ, 1);
		}// */

		installex(cc, "call", EMIT_opc, opc_call, type_vid, NULL);

		if ((typ = u32) != NULL) {
			enter(cc, NULL);
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
			//~ installex(cc, "toi64", EMIT_opc, u32_i64, type_i64, NULL);
			u32->args = leave(cc, u32, 1);
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
			typ->args = leave(cc, typ, 1);
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
			typ->args = leave(cc, typ, 1);
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
			typ->args = leave(cc, typ, 1);
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
			typ->args = leave(cc, typ, 1);
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
			typ->args = leave(cc, typ, 1);
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
			typ->args = leave(cc, typ, 1);
		}

		if ((mode & creg_swiz) == creg_swiz) {
			int i;
			struct {
				char *name;
				astn node;
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
			if ((typ = install(cc, "swz", TYPE_rec, 0, 0))) {
				enter(cc, NULL);
				for (i = 0; i < 256; i += 1)
					installex(cc, swz[i].name, EMIT_opc, p4x_swz, type_v4f, swz[i].node);
				typ->args = leave(cc, typ, 1);
			}
			/*if ((typ = install(cc, "dup", TYPE_rec, 0, 0))) {
				//~ extended set(masked) and dup(swizzle): p4d.dup.xyxy / p4d.set.xyz0
				enter(cc, NULL);
				for (i = 0; i < 256; i += 1)
					installex(cc, swz[i].name, EMIT_opc, p4x_dup, type_v4f, swz[i].node);
				typ->args = leave(cc, typ, 1);
			}// */
			/*if ((typ = install(cc, "set", TYPE_rec, 0, 0))) {
				enter(cc, NULL);
				installex(cc,    "x", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc,    "y", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc,    "z", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc,    "w", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc,  "xyz", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc, "xyzw", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc, "xyz0", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc, "xyz1", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				installex(cc, "xyz_", EMIT_opc, p4x_set, type_v4f, intnode(cc, 0));
				typ->args = leave(cc, typ, 1);
			}// */
		}
		emit_opc->args = leave(cc, emit_opc, 1);
		(void)val;
		(void)ref;
	}
}
static void install_type(ccState cc, int mode) {

	symn type_i08 = NULL, type_i16 = NULL;
	symn type_u08 = NULL, type_u16 = NULL;
	//~ symn type_chr = NULL, type_tmp = NULL;

	type_vid = install(cc,    "void", symn_const | TYPE_rec, TYPE_vid, 0);
	type_bol = install(cc,    "bool", symn_const | TYPE_rec, TYPE_bit, 4);
	type_i08 = install(cc,    "int8", symn_const | TYPE_rec, TYPE_i32, 1);
	type_i16 = install(cc,   "int16", symn_const | TYPE_rec, TYPE_i32, 2);
	type_i32 = install(cc,   "int32", symn_const | TYPE_rec, TYPE_i32, 4);
	type_i64 = install(cc,   "int64", symn_const | TYPE_rec, TYPE_i64, 8);
	type_u08 = install(cc,   "uint8", symn_const | TYPE_rec, TYPE_u32, 1);
	type_u16 = install(cc,  "uint16", symn_const | TYPE_rec, TYPE_u32, 2);
	type_u32 = install(cc,  "uint32", symn_const | TYPE_rec, TYPE_u32, 4);
	// type_ = install(cc,  "uint64", symn_const | TYPE_rec, TYPE_u64, 8);
	// type_ = install(cc, "float16", symn_const | TYPE_rec, TYPE_f64, 2);
	type_f32 = install(cc, "float32", symn_const | TYPE_rec, TYPE_f32, 4);
	type_f64 = install(cc, "float64", symn_const | TYPE_rec, TYPE_f64, 8);

	//~ addarg(cc, type_vid, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_vid)));
	//~ addarg(cc, type_bol, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_bol)));
	//~ addarg(cc, type_i08, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_i08)));
	//~ addarg(cc, type_i16, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_i16)));
	//~ addarg(cc, type_i32, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_i32)));
	//~ addarg(cc, type_i64, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_i64)));
	//~ addarg(cc, type_u08, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_u08)));
	//~ addarg(cc, type_u16, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_u16)));
	//~ addarg(cc, type_u32, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_u32)));
	//~ addarg(cc, type_f32, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_f32)));
	//~ addarg(cc, type_f64, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, sizeOf(type_f64)));

	if (1) {	// if pointer is needed.
		if ((type_ptr = installex(cc, "pointer", symn_const | TYPE_rec, 0, NULL, NULL))) {
			type_ptr->cast = TYPE_ref;
			type_ptr->size = 0;
		}
		null_ref = installex(cc, "null",   symn_stat | symn_const | TYPE_ref, TYPE_ref, type_ptr, NULL);
	}
	if (1) {	// if variant is needed.
		//~ type_var = install(cc, "variant", symn_const | TYPE_rec, TYPE_rec, 8);
	}

	if (1) {	// if aliases are needed.
		symn type_chr = type_u08;
		//~ type_chr = installex(cc, "char", symn_const | TYPE_def, 0, type_i08, NULL);
		if ((type_str = installex(cc, "string", symn_const | TYPE_arr, TYPE_ref, type_chr, NULL))) {// string is alias for char[]
			//~ addarg(cc, type_str, "sizeOf", symn_stat | TYPE_def, type_i32, intnode(cc, 4));
			type_str->cast = TYPE_ref;
			type_str->size = 4;
		}

		//~ installex(cc, "array",  symn_const | TYPE_arr, 0, type_var, NULL);		// array is alias for variant[]
		//~ installex(cc, "var",    symn_const | TYPE_def, 0, type_var, NULL);		// var is alias for variant

		installex(cc, "int",    TYPE_def, 0, type_i32, NULL);
		installex(cc, "long",   TYPE_def, 0, type_i64, NULL);
		installex(cc, "float",  TYPE_def, 0, type_f32, NULL);
		installex(cc, "double", TYPE_def, 0, type_f64, NULL);
		installex(cc, "true",   TYPE_def, 0, type_bol, intnode(cc, 1));
		installex(cc, "false",  TYPE_def, 0, type_bol, intnode(cc, 0));
	}
	// TODO:
	(void)type_i08;
	(void)type_i16;
	(void)type_u16;
}

static int libCallExitQuiet(state rt, int _) {
	return 0;
}
ccState ccInit(state rt, int mode, int libcExit(state, int)) {
	//~ ccState cc = getmem(s, sizeof(struct ccState), -1);
	ccState cc = (void*)rt->_ptr;

	if (rt->_mem + rt->_size - rt->_ptr < sizeof(struct ccState)) {
		return NULL;
	}

	memset(rt->_ptr, 0, sizeof(struct ccState));
	rt->_ptr += sizeof(struct ccState);

	if (cc == NULL)
		return NULL;

	cc->s = rt;
	rt->cc = cc;

	rt->defs = NULL;
	rt->gdef = NULL;
	cc->defs = NULL;

	cc->_chr = -1;

	cc->fin._ptr = 0;
	cc->fin._cnt = 0;
	cc->fin._fin = -1;

	cc->_beg = (char*)rt->_ptr;
	cc->_end = (char*)rt->_mem + rt->_size;

	install_type(cc, 1);

	if (mode & creg_emit)
		install_emit(cc, mode);

	// install a void arg for functions with no arguments
	if ((cc->void_tag = newnode(cc, TYPE_ref))) {
		enter(cc, NULL);
		cc->void_tag->id.name = "";
		cc->void_tag->next = NULL;
		declare(cc, TYPE_ref, cc->void_tag, type_vid);
		leave(cc, NULL, 0);
	}

	if (emit_opc && (cc->emit_tag = newnode(cc, TYPE_ref))) {
		cc->emit_tag->id.link = emit_opc;
		cc->emit_tag->id.name = "emit";
		cc->emit_tag->id.hash = -1;
	}

	libcall(rt, libcExit ? libcExit : libCallExitQuiet, 0, "void Exit(int code);");
	return cc;
}
ccState ccOpen(state rt, srcType mode, char *src) {
	if (rt->cc || ccInit(rt, creg_def, NULL)) {
		if (source(rt->cc, mode & srcFile, src) != 0)
			return NULL;
	}
	return rt->cc;
}
void ccSource(ccState cc, char *file, int line) {
	cc->file = file;
	cc->line = line;
}
int ccDone(state rt) {
	astn ast;
	ccState cc = rt->cc;

	// if not initialized
	if (cc == NULL)
		return -1;

	// check no token left to read
	if ((ast = peek(cc))) {
		error(rt, ast->file, ast->line, "unexpected: `%k`", ast);
		return -1;
	}

	// close input
	source(cc, 0, 0);

	// set used memory
	rt->_ptr = (unsigned char*)cc->_beg;

	// return errors
	return rt->errc;
}

state rtInit(void* mem, unsigned size) {
	state rt = mem;

	rt->_size = size - sizeof(struct state);
	rt->_ptr = rt->_mem;

	logFILE(rt, stderr);
	rt->cc = NULL;
	return rt;
}

/** allocate memory in the runtime state.
 * if (size != 0 && ptr != NULL): realloc
 * if (size != 0 && ptr == NULL): alloc
 * if (size == 0 && ptr != NULL): free
 * if (size == 0 && ptr == NULL): nothing
 * return NULL if cannot allocate, or size == 0
**/
void* rtAlloc(state rt, void *ptr, unsigned size) {
	//	memory manager for the vm.
	/* memory map
	 .
	 :
	>+--------------------------------+
	 | next                           |
	 +--------------------------------+
	 | size                           |
	 +--------------------------------+
	 | 0                              |
	 .                                .
	 : ...                            :
	 | size - 1 bytes                 |
	>+--------------------------------+
	 | next                           |
	 +--------------------------------+
	 | size                           |
	 +--------------------------------+
	 | 0                              |
	 : ...                            :
	 | size - 1 bytes                 |
	 +--------------------------------+
	 :
	the lowest bit in size indicates 
	*/

	typedef struct memchunk {
		unsigned int size;
		struct memchunk *next;
		char data[];
	} *memchunk;

	/*typedef struct memchunkUnused {
		unsigned int size;
		struct memchunk *next;
		struct memchunk *prev;	// free only
		char data[0];
	} *memchunk;*/

	memchunk memd = NULL;
	size = padded(size, sizeof(struct memchunk));

	if (ptr) {	// realloc or free.

		if (size == 0) {							// free

			memchunk prev = NULL;		// prevous free memory block
			memchunk next = rt->_free;	// is there a mergeable free memory block ?
			void *find;

			memd = (memchunk)((char*)ptr - sizeof(struct memchunk));

			find = (void*)(memd->data + memd->size);

			while (next && (void*)next != find) {
				trace("find to merge %06x, %06x", vmOffset(rt, next), vmOffset(rt, find));
				prev = next;
				next = next->next;
			}

			if (1) {	// unlink from used list
				memchunk prev = NULL, list = rt->_used;
				while (list && list != memd) {
					prev = list;
					list = list->next;
				}

				dieif(!list, "FixMe");

				if (prev) {
					prev->next = list->next;
				}
				else {
					rt->_used = list->next;
				}
			}

			// merge blocks if possible
			if (next && next == find) {
				//~ fputfmt(stdout, "%x: %x\n", vmOffset(rt, memd->data + memd->size), vmOffset(rt, next));
				//~ fputfmt(stdout, "%x: %x\n", vmOffset(rt, next), vmOffset(rt, find));
				//~ fputfmt(stdout, "%x: %x\n", next, find);
				memd->size += next->size + sizeof(struct memchunk);
				memd->next = next->next;
			}
			else {
				memd->next = next;
			}

			// link to free list
			if (prev != NULL) {
				prev->next = memd;
			}
			else {
				rt->_free = memd;
			}
		}
		else if (size > memd->size) {				// grow
			fatal("FixMe: %s", "realloc memory to grow");
		}
		else if (size > sizeof(struct memchunk)) {	// shrink
			fatal("FixMe: %s", "realloc memory to shrink");
		}
	}
	else {		// alloc or init.
		if (size > 0) {
			memchunk prev = NULL;

			int asize = sizeof(struct memchunk) + size;

			for (memd = rt->_free; memd; memd = memd->next) {
				if (memd->size >= asize) {
					break;
				}
				prev = memd;
			}

			// here we have the block
			if (memd) {

				// make a new free block
				memchunk free = (memchunk)(memd->data + size);

				// link to free list
				free->size = memd->size - asize;
				free->next = prev;
				if (prev != NULL) {
					prev->next = free;
				}
				else {
					rt->_free = free;
				}

				// todo link used block back ordered by size.
				memd->size = size;
				memd->next = rt->_used;
				rt->_used = memd;
			}
		}
		else {	// init
			if (rt->_free == NULL && rt->_used == NULL) {
				memchunk freemem = (void*)padded((int)(rt->_mem + rt->vm.px + 16), 16);
				freemem->next = NULL;
				//freemem->data = (unsigned char*)freemem + sizeof(struct memchunk);
				freemem->size = rt->_size - (freemem->data - (char*)rt->_mem);

				rt->_used = NULL;
				rt->_free = freemem;
			}
			else {
				memchunk mem;

				perr(rt, 0, NULL, 0, "memory info");
				for (mem = rt->_free; mem; mem = mem->next) {
					perr(rt, 0, NULL, 0, "free chunk@%06x[%06x:%06x] %d", vmOffset(rt, mem), vmOffset(rt, mem->data), vmOffset(rt, mem->data + mem->size), mem->size);
				}
				for (mem = rt->_used; mem; mem = mem->next) {
					perr(rt, 0, NULL, 0, "used chunk@%06x[%06x:%06x] %d", vmOffset(rt, mem), vmOffset(rt, mem->data), vmOffset(rt, mem->data + mem->size), mem->size);
					//~ perr(rt, 0, NULL, 0, "used mem@%06x[%d]", mem->data - (char*)rt->_mem, mem->size);
				}
				//fatal("FixMe: %s", "try to defrag free memory");
			}
		}
	}
	//~ debug("memmgr(%x, %d): (%x, %d)", vmOffset(rt, ptr), size, vmOffset(rt, memd ? memd->data : NULL), memd ? memd->size : -1);
	return memd ? memd->data : NULL;
}

//{ temp.c ---------------------------------------------------------------------
#if 0

//static void install_rtti(ccState cc, unsigned level) ;		// req emit: libc(import, define, lookup, ...)

static void install_lang(ccState cc, unsigned level) {
	state rt = cc->s;
	symn cls;
	if ((cls = ccBegin(rt, "Math"))) {
		ccDefineFlt(rt, "pi", 3.14);
		//~ installex(cc, "pi", TYPE_def, 0, type_f64, fh8node(cc, 0x400921fb54442d18LL));
		ccEnd(cc, cls);
	}
	if ((cls = ccBegin(rt, "Compiler"))) {
		//~ install(cc, "version", VERS_get, 0, 0);	// type int
		//~ install(cc, "file", LINE_get, 0, 0);	// type string
		//~ install(cc, "line", FILE_get, 0, 0);	// type int
		//~ install(cc, "date", DATE_get, 0, 0);	// type ???
		ccEnd(cc, cls);
	}
	if ((cls = ccBegin(rt, "Runtime"))) {
		install(cc, "Env", TYPE_def, 0, 0);	// map<String, String> Enviroment;
		install(cc, "Args", TYPE_arr, 0, 0);// string Args[int];
		install(cc, "assert(bool test, string error, var args...)", TYPE_def, 0, 0);
		ccEnd(cc, cls);
	}
	if ((cls = ccBegin(rt, "Reflection"))) {
		install(cc, "typename", TYPE_rec, 0, 0);
		install(cc, "bool canAssign(typename to, typename from, bool canCast)", TYPE_def, 0, 0);
		install(cc, "typename lookUp(typename &type, int options, string name, variant args...)", TYPE_def, 0, 0);

		install(cc, "bool invoke(typename t, string fun, variant args...)", TYPE_def, 0, 0);
		install(cc, "bool instanceof(typename &ty, var obj)", TYPE_def, 0, 0);
		ccEnd(cc, cls);
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
			#include "code.i"
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
#endif
// arrBuffer
#if 1
void* getBuff(arrBuffer* buff, int idx) {
	int pos = idx * buff->esz;

	if (pos >= buff->cap)
		return NULL;

	return buff->ptr + pos;
}
void* setBuff(arrBuffer* buff, int idx, void* data) {
	int pos = idx * buff->esz;

	//~ void* ptr = getBuff(buff, idx);
	if (pos >= buff->cap) {
		//~ trace("resizing setBuff(%d / %d)\n", idx, buff->cnt);
		buff->cap <<= 1;
		if (pos > 2 * buff->cap) {
			buff->cap = pos << 1;
		}
		buff->ptr = realloc(buff->ptr, buff->cap);
	}

	if (buff->cnt >= idx) {
		buff->cnt = idx + 1;
	}

	if (buff->ptr && data) {
		memcpy(buff->ptr + pos, data, buff->esz);
	}

	return buff->ptr ? buff->ptr + pos : NULL;
}
void initBuff(arrBuffer* buff, int initsize, int elemsize) {
	buff->cnt = 0;
	buff->ptr = 0;
	buff->esz = elemsize;
	buff->cap = initsize * elemsize;
	setBuff(buff, initsize, NULL);
}
void freeBuff(arrBuffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->cap = 0;
	buff->esz = 0;
}

#endif
//} */

