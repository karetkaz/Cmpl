//~ wcl386 -zq -ei -6s -d2  -fe=../main code.c clog.c parse.c tree.c type.c ccvm.c main.c
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
	#define OPCDEF(NAME, CODE, SIZE, CHCK, DIFF, TIME, MNEM)\
		{CODE, SIZE, CHCK, DIFF, MNEM},
	#include "incl/defs.h"
	{0},
};

#ifdef __WATCOMC__
//~ Warning! W124: Comparison result always 0
//~ Warning! W201: Unreachable code
//~ #pragma disable_message(124);
#pragma disable_message(201);
#endif

#if DEBUGGING > 3
#define dbg_ast(__AST) debug("cgen[%t->%t](err, %+k)\n%7K", get, ret, __AST, __AST)
#define dbg_arg(__AST) debug("cgen[%t->%t](arg, %+k)\n%3K", get, ret, __AST, __AST)
#define dbg_rhs(__AST) debug("cgen[%t->%t](rhs, %+k)\n%3K", get, ret, (__AST)->op.rhso, __AST)
#define dbg_lhs(__AST) debug("cgen[%t->%t](rhs, %+k)\n%3K", get, ret, (__AST)->op.lhso, __AST)
//~ #define dbgstmt(__AST) debug("cgen[%t->%t](err, %+k)\n%7K", get, ret, __AST, __AST)
#else
#define dbg_ast(__AST) debug("cgen[%t->%t](err, %+k)", get, ret, __AST, __AST)
#define dbg_arg(__AST) debug("cgen[%t->%t](arg, %+k)", get, ret, __AST, __AST)
#define dbg_rhs(__AST) debug("cgen[%t->%t](rhs, %+k)", get, ret, (__AST)->op.rhso, __AST)
#define dbg_lhs(__AST) debug("cgen[%t->%t](rhs, %+k)", get, ret, (__AST)->op.lhso, __AST)
//~ #else
//~ #define dbg_ast(__AST) 
//~ #define dbg_arg(__AST) 
//~ #define dbg_rhs(__AST) 
//~ #define dbg_lhs(__AST) 
#endif
#define internalerror error(s, ast->line, "internal error : %+k", ast)

static inline int genRef(vmState s, int offset) {
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

	if (s->vm->opti && eval(&tmp, ast)) {
		ast = &tmp;
	}

	return cgen(s, ast, cast);
}
int cgen(state s, astn ast, int get) {
	#if DEBUGGING
	int ipdbg = emitopc(s->vm, markIP);
	#endif
	struct astn tmp;
	int ret = 0;
	int qual = 0;

	if (!ast || !ast->type) {
		dieif(!ast || !ast->type, "FixMe %k", ast);
	}

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
		case STMT_do: {	// expr statement or decl or function body
			int stpos = stkoffs(s->vm, 0);
			emitint(s->vm, opc_line, ast->line);
			if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
				//~ fputasm(stderr, s->vm, ipdbg, 0x119);
				dbg_ast(ast->stmt.stmt);
				#if DEBUGGING
				fputasm(stdout, s->vm, ipdbg, -1, 0x119);
				#endif
				return 0;
			}
			if (stpos > stkoffs(s->vm, 0)) {
				warn(s, 1, s->cc->file, ast->line, "statement underflows stack: %+k", ast->stmt.stmt);
				#if DEBUGGING
				fputasm(stdout, s->vm, ipdbg, -1, 0x119);
				#endif
			}
			#if DEBUGGING
			if (0/*s->cc->verb & 1*/) {
				info(s, s->cc->file, ast->line, "%+k", ast->stmt.stmt);
				fputasm(stdout, s->vm, ipdbg, -1, 0x129);
			}
			#endif
		} break;
		case STMT_beg: {	// {}
			astn ptr;
			int stpos = stkoffs(s->vm, 0);
			emitint(s->vm, opc_line, ast->line);
			for (ptr = ast->stmt.stmt; ptr; ptr = ptr->next) {
				if (!cgen(s, ptr, TYPE_vid)) {
					error(s, ptr->line, "emmiting statement `%+k`", ptr);
					//~ #if DEBUGGING
					//~ fputasm(stdout, s->vm, ipdbg, -1, 0x119);
					//~ #endif
				}
			}
			//~ for (sym = ast->type; sym; sym = sym->next);	// destruct?
			if (get == TYPE_vid && stpos != stkoffs(s->vm, 0)) {
				if (!emitidx(s->vm, opc_drop, stpos)) {
					internalerror;
					return 0;
				}
			}
		} break;
		case STMT_if:  {
			int jmpt = 0, jmpf = 0;
			int tt = eval(&tmp, ast->stmt.test);

			dieif(get != TYPE_vid, "FixMe");
			emitint(s->vm, opc_line, ast->line);

			if (qual == QUAL_sta) {
				//~ debug("static if: `%k`: ", ast);
				if (ast->stmt.step || !tt) {
					error(s, ast->line, "invalid static if construct: %s", !tt ? "can not evaluate" : "else part is invalid");
					return 0;
				}
				qual = 0;
			}

			if (tt && (s->vm->opti || ast->cst2 == QUAL_sta)) {	// static if then else
				astn gen = constbol(&tmp) ? ast->stmt.stmt : ast->stmt.step;
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
				if (!(jmpt = emitopc(s->vm, opc_jz))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				if (!(jmpf = emitopc(s->vm, opc_jmp))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				fixjump(s->vm, jmpt, emitopc(s->vm, markIP), 0);
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpf, emitopc(s->vm, markIP), 0);
			}
			else if (ast->stmt.stmt) {							// if then
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				//~ if false skip THEN block
				if (!(jmpt = emitopc(s->vm, opc_jz))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->stmt.stmt, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emitopc(s->vm, markIP), 0);
			}
			else if (ast->stmt.step) {							// if else
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				//~ if true skip ELSE block
				if (!(jmpt = emitopc(s->vm, opc_jnz))) {
					debug("%+k", ast);
					internalerror;
					return 0;
				}
				if (!cgen(s, ast->stmt.step, TYPE_vid)) {
					debug("%+k", ast);
					return 0;
				}
				fixjump(s->vm, jmpt, emitopc(s->vm, markIP), 0);
			}
		} break;
		case STMT_for: {
			astn jl = s->cc->jmps;
			int jstep, lcont, lbody, lbreak;
			int stbreak, stpos = stkoffs(s->vm, 0);

			dieif(get != TYPE_vid, "FixMe");
			emitint(s->vm, opc_line, ast->line);
			if (ast->stmt.init && !cgen(s, ast->stmt.init, TYPE_vid)) {
				debug("%+k", ast);
				return 0;
			}

			//~ if (ast->cast == QUAL_par) ;		// 'parallel for'
			//~ else if (ast->cast == QUAL_sta) ;	// 'static for'

			if (!(jstep = emitint(s->vm, opc_jmp, 0))) {		// continue;
				debug("%+k", ast);
				internalerror;
				return 0;
			}

			lbody = emitopc(s->vm, markIP);
			if (ast->stmt.stmt && !cgen(s, ast->stmt.stmt, TYPE_vid)) {
				debug("%+k", ast);
				return 0;
			}

			lcont = emitopc(s->vm, markIP);
			if (ast->stmt.step && !cgen(s, ast->stmt.step, TYPE_vid)) {
				debug("%+k", ast);
				return 0;
			}

			fixjump(s->vm, jstep, emitopc(s->vm, markIP), 0);
			if (ast->stmt.test) {
				if (!cgen(s, ast->stmt.test, TYPE_bit)) {
					debug("%+k", ast);
					return 0;
				}
				if (!emitint(s->vm, opc_jnz, lbody)) {		// continue;
					debug("%+k", ast);
					internalerror;
					return 0;
				}
			}
			else {
				if (!emitint(s->vm, opc_jmp, lbody)) {		// continue;
					debug("%+k", ast);
					internalerror;
					return 0;
				}
			}

			// TODO: fix all breaks here
			lbreak = emitopc(s->vm, markIP);
			stbreak = stkoffs(s->vm, 0);

			while (s->cc->jmps != jl) {
				astn jmp = s->cc->jmps;
				s->cc->jmps = jmp->next;

				if (jmp->go2.stks != stbreak) {
					error(s, jmp->line, "`%k` statement is invalid due to previous variable declaration within loop", jmp);
					return 0;
				}

				switch (jmp->kind) {
					default : error(s, jmp->line, "invalid goto statement: %k", jmp); return 0;
					case STMT_brk: fixjump(s->vm, jmp->go2.offs, lbreak, jmp->go2.stks); break;
					case STMT_con: fixjump(s->vm, jmp->go2.offs, lcont, jmp->go2.stks); break;
				}
			}

			//! TODO: if (init is decl) destruct;
			if (stpos != stkoffs(s->vm, 0)) {
				dieif(!emitidx(s->vm, opc_drop, stpos), "FixMe");
			}
		} break;
		case STMT_con:
		case STMT_brk: {
			int offs;
			dieif(get != TYPE_vid, "FixMe");
			emitint(s->vm, opc_line, ast->line);
			if (!(offs = emitint(s->vm, opc_jmp, 0))) {
				debug("%+k", ast);
				internalerror;
				return 0;
			}

			ast->go2.offs = offs;
			ast->go2.stks = stkoffs(s->vm, 0);

			ast->next = s->cc->jmps;
			s->cc->jmps = ast;

		} break;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/call/cast
			int stkret = stkoffs(s->vm, sizeOf(ast->type));
			//~ int mark1 = emit(s->vm, markIP);
			astn argv = ast->op.rhso;
			symn var = linkOf(ast->op.lhso, 1);	// not refs only

			dieif(!var && ast->op.lhso, "FixMe[%+k]", ast);
			//~ dieif(!var || !var->call, "FixMe[%+k]", ast);

			// inline
			if (var && var->kind == TYPE_def) {
				int mark1 = emitopc(s->vm, markIP);
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
						astn arg = an;

						if (s->vm->opti && eval(&tmp, arg)) {
							arg = &tmp;
						}

						if (an->kind == TYPE_ref || as->cast == TYPE_def) {
						//~ if (an->kind == TYPE_ref || as->cast == TYPE_ref) {
						//~ if (an->kind == TYPE_ref || as->load) {
							as->kind = TYPE_def;
							as->init = an;
						}
						else {
							//~ debug("arg(%k:[%t])", arg, as->type->cast);
							if (!cgen(s, arg, TYPE_any)) {
								dbg_arg(an);
								return 0;
							}
							as->offs = stkoffs(s->vm, 0);
							as->kind = TYPE_ref;
						}
						an = an->next;
						as = as->next;
					}
				}
				//~ debug("inline %+k = %+k", ast, var->init); fputasm(stdout, s->vm, ipdbg, -1, 0x119);

				if (!(ret = cgen(s, var->init, ret))) {
					debug("%+T", var);
					dbg_arg(var->init);
					return 0;
				}

				if (stkret != stkoffs(s->vm, 0)) {
					mark1 = emitopc(s->vm, markIP);
					if (!emitidx(s->vm, opc_ldsp, stkret)) {	// get stack
						internalerror;
						dbg_ast(ast);
						return 0;
					}
					if (get != TYPE_vid && !emitint(s->vm, opc_sti, sizeOf(ast->type))) {
						error(s, ast->line, "store indirect: %T:%d of `%+k`", ast->type, sizeOf(ast->type), ast);
						//~ internalerror;
						dbg_ast(ast);
						return 0;
					}
					if (stkret < stkoffs(s->vm, 0) && !emitidx(s->vm, opc_drop, stkret)) {
						internalerror;
						dbg_ast(ast);
						return 0;
					}
				}// */

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

			// push Result
			if (var && var != ast->type && var != emit_opc && !emitint(s->vm, opc_spc, sizeOf(ast->type))) {
				internalerror;
				dbg_ast(ast);
				return 0;
			}// */
			// push args
			if (argv) {
				astn argl = NULL;

				while (argv->kind == OPER_com) {
					astn arg = argv->op.rhso;
					//~ if (arg->cst2 == TYPE_ref) {debug("byref: %k", arg);}
					if (!genVal(s, arg)) {
						dbg_arg(arg);
						return 0;
					}
					arg->next = argl;
					argl = arg;
					argv = argv->op.lhso;
				}

				if (var == emit_opc && istype(argv))
					return castOf(argv->type);

				else if (!genVal(s, argv)) {
					dbg_arg(argv);
					return 0;
				}

				argv->next = argl;
				argl = argv;
			}

			if (var == ast->type) {			// cast()
				//~ debug("cast: %+k", ast);
				if (var == emit_opc) {	// emit()
				}
				else if (!argv || argv != ast->op.rhso /* && cast*/)
					warn(s, 1, s->cc->file, ast->line, "multiple values, array ?: '%+k'", ast);
			}
			else if (var) {					// call()
				//~ debug("call: %+k", ast);
				if (var == emit_opc) {	// emit()
				}
				else if (var->call) {
					if (!cgen(s, ast->op.lhso, TYPE_ref)) {
						dbg_lhs(ast);
						return 0;
					}
					if (!emitopc(s->vm, opc_call)) {
						internalerror;
						dbg_ast(ast);
						return 0;
					}
					// clean the stack
					if (stkret != stkoffs(s->vm, 0) && !emitidx(s->vm, opc_drop, stkret)) {
						internalerror;
						dbg_ast(ast);
						return 0;
					}
				}
				else {
					error(s, ast->line, "called object is not a function: %+T", var);
					//~ but can have an operator: call() with this args
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
				dbg_lhs(ast);
				return 0;
			}

			if (s->vm->opti && eval(&tmp, ast->op.rhso) == TYPE_int) {
				int offs = sizeOf(ast->type) * constint(&tmp);
				if (constint(&tmp) >= ast->op.lhso->type->size) {
					error(s, ast->line, " index out of bounds: %+k", ast);
				}
				if (!emitint(s->vm, opc_inc, offs)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
			}
			else {
				if (!cgen(s, ast->op.rhso, TYPE_u32)) {
					dbg_rhs(ast);
					return 0;
				}
				if (!emitint(s->vm, opc_ldc4, sizeOf(ast->type))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
				if (!emitopc(s->vm, u32_mad)) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emitint(s->vm, opc_ldi, sizeOf(ast->type))) {
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

			//~ debug("%+k", ast->op.lhso);
			//~ fputasm(stdout, s->vm, ipdbg, -1, 0x119);

			if (!emitint(s->vm, opc_inc, var->offs)) {
				dbg_ast(ast);
				internalerror;
				return 0;
			}

			if (get == TYPE_ref)
				return TYPE_ref;

			if (!emitint(s->vm, opc_ldi, sizeOf(ast->type))) {
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
			if (!cgen(s, ast->op.rhso, 0)) {
				dbg_rhs(ast);
				return 0;
			}
			if (!emitint(s->vm, opc, ast->op.rhso->cst2)) {
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
			if (!emitint(s->vm, opc, ast->op.rhso->cst2)) {
				dbg_ast(ast);
				internalerror;
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
					dieif(ret != TYPE_bit, "RemMe");
					break;
			}
			#endif
		} break;

		//~ case OPER_lnd:		// '&&'
		//~ case OPER_lor:		// '||'
		case OPER_sel: {		// '?:'
			int jmpt, jmpf;
			if (0 && s->vm->opti && eval(&tmp, ast->op.test)) {
				if (!cgen(s, constbol(&tmp) ? ast->op.lhso : ast->op.rhso, 0)) {
					dbg_ast(ast);
					return 0;
				}
			}
			else {
				int bppos = stkoffs(s->vm, 0);
				int stpos = stkoffs(s->vm, -sizeOf(ast->type));

				if (!cgen(s, ast->op.test, TYPE_bit)) {
					dbg_ast(ast);
					return 0;
				}

				if (!(jmpt = emitopc(s->vm, opc_jz))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}

				if (!cgen(s, ast->op.lhso, 0)) {
					dbg_ast(ast);
					return 0;
				}

				if (!(jmpf = emitopc(s->vm, opc_jmp))) {
					dbg_ast(ast);
					internalerror;
					return 0;
				}

				fixjump(s->vm, jmpt, emitopc(s->vm, markIP), bppos);

				if (!cgen(s, ast->op.rhso, 0)) {
					dbg_ast(ast);
					return 0;
				}

				fixjump(s->vm, jmpf, emitopc(s->vm, markIP), -sizeOf(ast->type));
				debug("%d, %d, %d", bppos, stpos, stkoffs(s->vm, 0));
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
				if (!emitint(s->vm, opc_ldi, sizeOf(ast->type))) {
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
			if (!emitint(s->vm, opc_sti, sizeOf(ast->type))) {
				dbg_ast(ast);
				internalerror;
				return 0;
			}
		} break;
		//}
		//{ TVAL
		//~ case TYPE_bit:	// no constant of this kind
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
			case TYPE_ref: return emitptr(s->vm, ast->id.name) ? TYPE_ref : 0;
			default: debug("invalid cast: to (%t) '%+k'", get, ast);
		} return 0;

		case TYPE_ref: {					// use (var, func, define)
			symn typ = ast->type;			// type
			symn var = ast->id.link;		// link
			dieif(!typ || !var, "FixMe");

			switch (var->kind) {
				case TYPE_ref:
					//~ debug("ref:%T", var);
					if (!genRef(s->vm, var->offs)) {
						dbg_ast(ast);
						return 0;
					}

					//~ dieif(var->cast == TYPE_ref && (typ->cast != TYPE_ref || typ->cast != TYPE_ref), "FixMe");
					//~ dieif(typ->cast == TYPE_ref && (var->cast != TYPE_ref || var->cast != TYPE_def), "FixMe %k", ast);
					//~ if (var->load && !emit(s->vm, opc_ldi4)) {
					if ((var->cast == TYPE_ref) && !emitopc(s->vm, opc_ldi4)) {
						dbg_ast(ast);
						return 0;
					}
					if (get != TYPE_ref) {
						// TODO: (Sign / Zero) extend
						if (!emitint(s->vm, opc_ldi, sizeOf(ast->type))) {
							dbg_ast(ast);
							return 0;
						}
					}
					else ret = TYPE_ref;
					break;
				case EMIT_opc:
					dieif(get == TYPE_ref, "FixMe");
					if (!emitint(s->vm, var->offs, var->init ? constint(var->init) : 0)) {
						error(s, ast->line, "error emiting opcode: %+k", ast);
						if (stkoffs(s->vm, 0) > 0)
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
			symn typ = ast->type;
			symn var = ast->id.link;
			int stktop = stkoffs(s->vm, sizeOf(typ));
			dieif(!typ || !var, "FixMe");

			if (var->kind == TYPE_ref) {

				// static variable or function.
				if (var->offs < 0) {

					// function
					if (var->call) {
						return ret;
					}

					// initialize
					if (var->init) {
						//~ if variable was allocated
						//~ make assigment from initializer
						struct astn id, op;
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
					}

					return ret;
				}

				if (typ->cast == TYPE_ref) {
					var->cast = TYPE_ref;
					//~ var->load = 1;
				}

				if (var->init) {
					astn val = var->init;

					if (val->type == emit_opc) {
						ret = get;
					}
					else if (val->type != typ && !promote(val->type, typ)) {
						error(s, ast->line, "incompatible types in initialization(%-T := %-T)", typ, val->type);
					}

					if (s->vm->opti && eval(&tmp, val)) {
						val = &tmp;
					}// * /

					//~ debug("cast(`%+k`, %t):%t", ast, get, ret);

					if (val->kind == EMIT_opc) {
						if (!emitint(s->vm, opc_ldsp, 0)) {
							internalerror;
							dbg_ast(ast);
							return 0;
						}
						//~ var->load = 1;
						var->cast = TYPE_ref;
					}
					else if (val->kind != EMIT_opc && !cgen(s, val, ret)) {
						dbg_arg(val);
						return 0;
					}

					var->offs = stkoffs(s->vm, 0);

					if (stktop != var->offs && val->type != emit_opc) {
						error(s, ast->line, "invalid initializer size: %d <> got(%d): `%+k`", stktop, var->offs, val);
						//~ warn(s, 1, s->cc->file, ast->line, "invalid initializer size: %d <> %d", (stktop - var->offs), sizeOf(typ));
						return 0;
					}
				}
				else {		// static alloc a block of the size of the type;
					int size = sizeOf(typ);
					if (!emitint(s->vm, opc_spc, size)) {
						dbg_arg(ast);
						return 0;
					}
					var->offs = stkoffs(s->vm, 0);
					if (var->offs < size) {
						error(s, ast->line, "stack underflow", var->offs, size);
						#if DEBUGGING
						fputasm(stderr, s->vm, ipdbg, -1, 0x10);
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

		case EMIT_opc:
			return 0;
		//~ case TYPE_rec:
		//~ case TYPE_enu: break;
		//}
	}

	if (get != ret) switch (get) {
		case TYPE_any: switch (ret) {
			case TYPE_vid: break;
			default: goto errorcast2;
		} break;// */
		case TYPE_vid: return TYPE_vid;
		case TYPE_u32: switch (ret) {
			case TYPE_bit:
			case TYPE_i32: break;
			case TYPE_i64: if (!emitopc(s->vm, i64_i32)) return 0; break;
			//~ case TYPE_f32: if (!emitopc(s->vm, f32_i32)) return 0; break;
			//~ case TYPE_f64: if (!emitopc(s->vm, f64_i32)) return 0; break;
			default: goto errorcast2;
		} break;// */
		case TYPE_i32: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64: if (!emitopc(s->vm, i64_i32)) return 0; break;
			case TYPE_f32: if (!emitopc(s->vm, f32_i32)) return 0; break;
			case TYPE_f64: if (!emitopc(s->vm, f64_i32)) return 0; break;
			//~ case TYPE_ref: if (!emitopc(s->vm, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_i64: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_u32:	// TODO: zero extend, but how ?
			case TYPE_u32: if (!emitopc(s->vm, u32_i64)) return 0; break;
			case TYPE_i32: if (!emitopc(s->vm, i32_i64)) return 0; break;
			case TYPE_f32: if (!emitopc(s->vm, f32_i64)) return 0; break;
			case TYPE_f64: if (!emitopc(s->vm, f64_i64)) return 0; break;
			//~ case TYPE_ref: if (!emitopc(s->vm, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f32: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_u32:
			case TYPE_i32: if (!emitopc(s->vm, i32_f32)) return 0; break;
			case TYPE_i64: if (!emitopc(s->vm, i64_f32)) return 0; break;
			case TYPE_f64: if (!emitopc(s->vm, f64_f32)) return 0; break;
			//~ case TYPE_ref: if (!emitopc(s->vm, opc_ldi, 4)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_f64: switch (ret) {
			case TYPE_bit:
			//~ case TYPE_u32:
			case TYPE_i32: if (!emitopc(s->vm, i32_f64)) return 0; break;
			case TYPE_i64: if (!emitopc(s->vm, i64_f64)) return 0; break;
			case TYPE_f32: if (!emitopc(s->vm, f32_f64)) return 0; break;
			//~ case TYPE_ref: if (!emitopc(s->vm, opc_ldi, 8)) return 0; break;
			default: goto errorcast2;
		} break;
		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32: /*emitopc(s, i32_bol);*/ break;
			case TYPE_i32: /*emitopc(s, i32_bol);*/ break;
			case TYPE_i64: if (!emitopc(s->vm, i64_bol)) return 0; break;
			case TYPE_f32: if (!emitopc(s->vm, f32_bol)) return 0; break;
			case TYPE_f64: if (!emitopc(s->vm, f64_bol)) return 0; break;
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

	dieif (qual, "unimplemented `%t %k`", qual, ast);
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

void ccSource(ccState cc, char *file, int line) {
	cc->file = file;
	cc->line = line;
}

int gencode(state s, int level) {
	ccState cc = s->cc;
	vmState vm = s->vm;
	int Lmain, Lfunc;

	if (s->errc)
		return -2;

	if (!s->cc || s->vm) {
		fatal("invalid enviroment: cc:%x, vm:%x, er:%d", cc, vm, s->errc);
		return -2;
	}

	s->defs = leave(s->cc, NULL);

	if (!(vm = vmInit(s)))
		return -1;

	vm->opti = level;

	//~ DONE: Data[RO]: metadata, strings
	vm->ds = vm->pos;

	//~ TODO: Data[RO]: constants, functions, enums ...

	Lfunc = vm->pos;
	if (1 && s->defs) {
		symn var = s->defs;
		for (; var; var = var->next) {
			if (var->cast == QUAL_sta && var->call) {
				int seg = emitopc(s->vm, markIP);

				if (!var->init) {
					debug("`%T` will be skipped", var);
					continue;
				}

				vm->sm = 0;
				fixjump(s->vm, 0, 0, 4 + var->offs);
				var->offs = -emitopc(s->vm, markIP);

				if (!cgen(s, var->init, TYPE_vid)) {
					fputasm(stderr, vm, seg, -1, 0x10);
					return 0;
				}
				if (!emitopc(vm, opc_jmpi)) {
					fputasm(stderr, vm, seg, -1, 0x10);
					return 0;
				}
				while (cc->jmps) {
					error(s, 0, "invalid jump: `%k`", cc->jmps);
					cc->jmps = cc->jmps->next;
				}
				//~ fputasm(stderr, vm, seg, -1, 0x129);
			}
		}
	}

	vm->ds = vm->pos - 0;

	//~ DONE: CODE[RO]: meta, strings.

	// static vars
	if (1 && s->defs) {
		symn glob = s->defs;
		for (; glob; glob = glob->next) {

			if (glob->call)
				continue;

			//~ if (glob->kind != TYPE_ref)
				//~ continue;

			//~ if (glob->type->kind != TYPE_arr)
				//~ continue;

			//~ debug("global %+T", glob);
			if (glob->cast == QUAL_sta) {
				glob->offs = -vm->pos;
				vm->pos += sizeOf(glob->type);
				debug("global variable: %+T@%x", glob, -glob->offs);
			}

		}
	}// */

	Lmain = vm->pos;
	if (cc->root) {
		// TODO: TYPE_vid: to clear the stack
		int seg = vm->pos;
		vm->ss = vm->sm = 0;
		if (!cgen(s, cc->root, 0))
			fputasm(stderr, vm, seg, -1, 0x10);
		while (cc->jmps) {
			error(s, 0, "invalid jump: `%k`", cc->jmps);
			cc->jmps = cc->jmps->next;
		}
	}
	emitint(s->vm, opc_libc, 0);		// TODO: Halt only in case of scripts

	vm->pc = Lmain;
	vm->cs = vm->pos - Lmain;

	#if DEBUGGING > 1
	vmInfo(stdout, s->vm);
	#endif

	return s->errc;
}
//~ int execute(state s, int stack) ;

symn emit_opc = NULL;
static void install_emit(ccState cc) {
	symn typ;
	symn type_v4f = NULL;

	emit_opc = install(cc, "emit", EMIT_opc | symn_read, 0, 0);
	//~ emit_opc->type = 0;
	//~ emit_opc->call = 1;

	if (!emit_opc)
		return;

	enter(cc, NULL);

	installex(cc, "nop", EMIT_opc, opc_nop, type_vid, NULL);
	installex(cc, "not", EMIT_opc, opc_not, type_bol, NULL);
	installex(cc, "set", EMIT_opc, opc_set1, type_vid, intnode(cc, 1));
	installex(cc, "pop", EMIT_opc, opc_drop, type_vid, intnode(cc, 4));
	installex(cc, "call", EMIT_opc, opc_call, type_vid, NULL);

	installex(cc, "set0", EMIT_opc, opc_set1, type_vid, intnode(cc, 0));
	installex(cc, "set1", EMIT_opc, opc_set1, type_vid, intnode(cc, 1));

	if ((typ = install(cc, "void", TYPE_rec, TYPE_vid, 0))) ;

	if ((typ = install(cc, "val", TYPE_rec, TYPE_rec, -1))) ;
	if ((typ = install(cc, "ref", TYPE_rec, TYPE_ref, 4))) ;

	if ((typ = install(cc, "ldz", TYPE_enu, TYPE_vid, 0))) {
		enter(cc, NULL);
		installex(cc, "x1", EMIT_opc, opc_ldz1, type_vid, intnode(cc, 0));
		installex(cc, "x2", EMIT_opc, opc_ldz2, type_vid, intnode(cc, 0));
		installex(cc, "x4", EMIT_opc, opc_ldz4, type_vid, intnode(cc, 0));
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "dupp", TYPE_enu, TYPE_vid, 0))) {
		enter(cc, NULL);
		installex(cc, "x1", EMIT_opc, opc_dup1, type_i32, intnode(cc, 0));
		installex(cc, "x2", EMIT_opc, opc_dup2, type_i64, intnode(cc, 0));
		installex(cc, "x4", EMIT_opc, opc_dup4, type_vid, intnode(cc, 0));
		typ->args = leave(cc, typ);
	}

	if ((typ = install(cc, "u32", TYPE_rec, TYPE_u32, 4))) {
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
		installex(cc, "toi64", EMIT_opc, u32_i64, type_i64, NULL);
		typ->args = leave(cc, typ);
	}
	if ((typ = install(cc, "i32", TYPE_rec, TYPE_i32, 4))) {
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
	if ((typ = install(cc, "i64", TYPE_rec, TYPE_i64, 8))) {
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
	if ((typ = install(cc, "f32", TYPE_rec, TYPE_f32, 4))) {
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
	if ((typ = install(cc, "f64", TYPE_rec, TYPE_f64, 8))) {
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

	if ((typ = install(cc, "v4f", TYPE_rec, 0, 16))) {
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
	if ((typ = install(cc, "v2d", TYPE_rec, 0, 16))) {
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

	if ((COMPILER_LEVEL & creg_swiz) == creg_swiz) {
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

		if ((typ = install(cc, "swz", TYPE_enu, 0, 0))) {
			enter(cc, NULL);
			for (i = 0; i < 256; i += 1)
				installex(cc, swz[i].name, EMIT_opc, p4d_swz, type_v4f, swz[i].node);
			typ->args = leave(cc, typ);
		}
		if ((typ = install(cc, "dup", TYPE_enu, 0, 0))) {
			//~ extended set(masked) and dup(swizzle): p4d.dup.xyxy / p4d.set.xyz0
			enter(cc, NULL);
			for (i = 0; i < 256; i += 1)
				installex(cc, swz[i].name, EMIT_opc, 0x1d, type_v4f, swz[i].node);
			typ->args = leave(cc, typ);
		}
		if ((typ = install(cc, "set", TYPE_enu, 0, 0))) {
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
}

state rtInit(void* mem, unsigned size) {
	state s = mem;
	s->_cnt = size - sizeof(struct state);
	s->_ptr = s->_mem;
	s->logf = stderr;
	s->cc = NULL;
	s->vm = NULL;
	return s;
}

ccState ccInit(state s) {
	ccState cc = getmem(s, sizeof(struct ccState), -1);

	symn type_tmp;
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

	cc->_beg = s->_ptr;
	cc->_end = s->_ptr + s->_cnt;

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

	#if DEBUGGING > 0
	install(cc,  "hex8", TYPE_rec | symn_read, TYPE_i32, 1)->pfmt = "0x%02x";
	install(cc, "hex16", TYPE_rec | symn_read, TYPE_i32, 2)->pfmt = "0x%04x";
	type_tmp = install(cc, "hex32", TYPE_rec | symn_read, TYPE_i32, 4);
	type_tmp->pfmt = "0x%08x";
	install(cc, "hex64", TYPE_rec | symn_read, TYPE_i64, 8)->pfmt = "0x%016X";
	installex(cc, "hex", TYPE_def, 0, type_tmp, NULL);
	#endif

	if (COMPILER_LEVEL & creg_emit)
		install_emit(cc);

	//~ type_arr = install(cc, "array", TYPE_ptr, 0, 0);
	//~ type_ptr = install(cc, "pointer", TYPE_ptr, 0, 0);
	type_str = install(cc, "string", TYPE_rec | symn_read, TYPE_ref, 4);

	//~ type_chr = installex(cc, "char", TYPE_def, 0, type_i08, NULL);
	//~ type_str = installex(cc, "string", TYPE_arr, 0, type_chr, NULL);

	installex(cc, "int", TYPE_def, 0, type_i32, NULL);
	installex(cc, "long", TYPE_def, 0, type_i64, NULL);
	installex(cc, "float", TYPE_def, 0, type_f32, NULL);
	installex(cc, "double", TYPE_def, 0, type_f64, NULL);

	installex(cc, "true", TYPE_def, 0, type_bol, intnode(cc, 1));
	installex(cc, "false", TYPE_def, 0, type_bol, intnode(cc, 0));

	//~ installex(cc, "null", TYPE_def, 0, type_ptr, intnode(cc, 0));

	//} */// types

	// install a void arg for functions with no arguments
	if ((cc->argz = newnode(cc, TYPE_ref))) {
		enter(cc, NULL);
		cc->argz->id.name = "`VoId`";
		//~ cc->argz->next = NULL;
		declare(cc, TYPE_ref, cc->argz, type_vid);
		leave(cc, NULL);
	}

	/*if (1) {
		// DONE: symbols points to tree nodes, wich offset depends on the size of the vm.
		//~ debug("Copiler hashcode: 0x%08x, %d", rehash(s->_mem, cc->_beg - s->_mem), cc->_beg - s->_mem);
		FILE* out;
		if ((out = fopen("cc.obj", "wb"))) {
			char* ptr = s->_mem;
			while (ptr < cc->_beg) {
				fputc(*ptr, out);
				ptr += 1;
			}
			fclose(out);
		}
	} // */
	return cc;
}
ccState ccOpen(state s, srcType mode, char *src) {
	if (s->cc || ccInit(s)) {
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
	s->_cnt -= cc->_beg - s->_ptr;
	s->_ptr = cc->_beg;

	// return errors
	return s->errc;
}

vmState vmInit(state s) {
	int size = s->_cnt;
	vmState vm = getmem(s, size, sizeof(struct vmState));
	if (vm != NULL) {
		vm->s = s;
		//~ cleared by getmem
		//~ vm->ds = vm->ic = 0;
		//~ vm->cs = vm->pc = 0;
		//~ vm->ss = vm->sm = 0;
		vm->_mem = (unsigned char *)s->_mem;
		vm->pos = vm->_beg - vm->_mem;
		vm->_end = vm->_beg + size - sizeof(struct vmState);
	}
	return s->vm = vm;
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
// lookup a value
symn findsym(ccState s, symn in, char *name) {
	struct astn ast;
	memset(&ast, 0, sizeof(struct astn));
	ast.kind = TYPE_ref;
	//ast.id.hash = rehash(name);
	ast.id.name = name;
	return lookup(s, in ? in : s->s->defs, &ast, 1, NULL);
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

#if 0	// install_...

//static void install_rtti(ccState cc, unsigned level) ;		// req emit: libc(import, define, lookup, ...)

static void install_bits(ccState cc, symn it, int64_t sgn) {
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
}

static void install_Lang(ccState cc, unsigned level) {
	if (install(cc, "Math", TYPE_enu, 0, 0)) {
		installex(cc, "pi", TYPE_def, 0, type_f64, fh8node(cc, 0x400921fb54442d18LL));
	}
	if (install(cc, "Compiler", TYPE_enu, 0, 0)) {
		//~ install(cc, "version", VERS_get, 0, 0);	// type int
		//~ install(cc, "file", LINE_get, 0, 0);	// type string
		//~ install(cc, "line", FILE_get, 0, 0);	// type int
		//~ install(cc, "date", DATE_get, 0, 0);	// type ???
		//~ install(cc, "emit", DATE_get, 0, 0);	// type ???
	}
	if (install(cc, "Runtime", TYPE_enu, 0, 0)) {
		install(cc, "Env", TYPE_enu, 0, 0);	// map<String, String> Enviroment;
		install(cc, "Args", TYPE_arr, 0, 0);// string Args[int];
		install(cc, "assert(...)", TYPE_def, 0, 0);
	}
	if (install(cc, "Reflection", TYPE_enu, 0, 0)) {
		install(cc, "symbol", TYPE_rec, 0, 0);
		install(cc, "invoke(...)", TYPE_def, 0, 0);
	}
}// */
#endif
#if 0	// vm test&help
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
#if 0	// arrBuffer
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

