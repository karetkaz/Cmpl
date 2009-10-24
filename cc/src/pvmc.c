#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "pvmc.h"

//~ astn iscdef(astn);
//~ #include "clog.c"
//~ #include "scan.c"
//~ #include "code.c"
//~ #include "tree.c"
//~ #include "type.c"

// default values
static const int wl = 9;		// warninig level
static const int ol = 0;		// optimize level

static const int cc = 1;			// execution cores
static const int ss = 1 << 10;		// execution stack(256K)
//~ static const int hs = 128 << 20;	// execution heap(128M)
void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

#if defined(WINDOWS) || defined(_WIN32)
const char* os = "Windows";
#elif defined(linux) || defined(_linux)
const char* os = "Linux";
#else
const char* os = "Unknown";
#endif

const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, KIND, STR) {KIND, TYPE, SIZE, STR},
	#include "incl/defs.h"
};
const opc_inf opc_tbl[255] = {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) \
			{Code, Size, Args, Push, Mnem},
	#include "incl/defs.h"
};

symn emit_opc = 0;
int cgen(state s, astn ast, int get) {
	int ret = 0;
	struct astn tmp;
	const int dxst = 15;	// TODO: debug
	//~ vmEnv code = s->vm;
	//~ ccEnv tree = s->cc;

	if (!ast) return 0;

	//~ debug("(%+k):%?T", ast, ast->type);

	switch (ast->kind) {
		//{ STMT
		case OPER_nop: {	// expr statement
			emit(s->vm, opc_line, ast->line);
			return cgen(s, ast->stmt, TYPE_vid);
		} return TYPE_vid;
		case OPER_beg: {	// {}
			astn ptr;
			int stpos = emit(s->vm, get_sp);
			emitint(s->vm, opc_line, ast->line);
			for (ptr = ast->stmt; ptr; ptr = ptr->next) {
				dieif(!cgen(s, ptr, TYPE_vid), "%+k", ast->stmt);
			}
			//~ for (sym = ast->type; sym; sym = sym->next);	// destruct?
			if (get == TYPE_vid && stpos != emit(s->vm, get_sp))
				dieif(emitidx(s->vm, opc_pop, stpos) <= 0, "");
		} return TYPE_vid;
		case OPER_jmp: {	// if ( ) then {} else {}
			int tt = eval(&tmp, ast->test, TYPE_bit);
			emitint(s->vm, opc_line, ast->line);
			//~ if (tt) debug("if (%+k) : %t(%k)", ast->test, tt, &tmp);
			/*if (ast->cast == QUAL_sta) {		// 'static if' (compile time)
				if (!tt) {
					error("static if cannot be evaluated");
					return 0;
				}
				if (constbol(&tmp))
					cgen(s, ast->stmt, TYPE_vid);
				else
					cgen(s, ast->step, TYPE_vid);
			}
			else if (ast->cast) {
				error(s, ast->line, "invalid qualifyer %k", argv);
				return 0;
			} else */
			if (ast->test) {					// if then else
				int jmpt, jmpf;
				if (/*TODO: s->copt > 0 */ 1 && tt) {		// if true
					if (constbol(&tmp))
						cgen(s, ast->stmt, TYPE_vid);
					else
						cgen(s, ast->step, TYPE_vid);
				}
				else if (ast->stmt && ast->step) {		// if then else
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast);
						return 0;
					}
					jmpt = emit(s->vm, opc_jz);
					cgen(s, ast->stmt, TYPE_vid);
					jmpf = emit(s->vm, opc_jmp);
					fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
					cgen(s, ast->step, TYPE_vid);
					fixjump(s->vm, jmpf, emit(s->vm, get_ip), 0);
				}
				else if (ast->stmt) {					// if then
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast->test);
						return 0;
					}
					//~ if false skip THEN block
					jmpt = emit(s->vm, opc_jz);
					cgen(s, ast->stmt, TYPE_vid);
					fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
				}
				else if (ast->step) {					// if else
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast);
						return 0;
					}
					//~ if true skip ELSE block
					jmpt = emit(s->vm, opc_jnz);
					cgen(s, ast->step, TYPE_vid);
					fixjump(s->vm, jmpt, emit(s->vm, get_ip), 0);
				}
				else fatal("error");
			}
			else debug("unimplemented: goto / break/ continue;");
		} return TYPE_vid;
		case OPER_for: {	// for ( ; ; ) {}
			int beg, end, cmp = -1;
			int stpos = emit(s->vm, get_sp);

			emitint(s->vm, opc_line, ast->line);
			cgen(s, ast->init, TYPE_vid);

			//~ if (ast->cast == QUAL_par) ;		// 'parallel for'
			//~ else if (ast->cast == QUAL_sta) ;	// 'static for'
			beg = emit(s->vm, get_ip);
			if (ast->step) {
				int tmp = beg;
				if (ast->init)
					emit(s->vm, opc_jmp, 0);

				beg = emit(s->vm, get_ip);
				cgen(s, ast->step, TYPE_vid);

				if (ast->init)
					fixjump(s->vm, tmp, emit(s->vm, get_ip), 0);
			}
			if (ast->test) {
				cgen(s, ast->test, TYPE_bit);
				cmp = emit(s->vm, opc_jz, 0);		// if (!test) break;
			}

			// push(list_jmp, 0);
			cgen(s, ast->stmt, TYPE_vid);			// this will leave the stack clean
			end = emit(s->vm, opc_jmp, beg);			// continue;
			fixjump(s->vm, end, beg, 0);
			end = emit(s->vm, get_ip);
			fixjump(s->vm, cmp, end, 0);				// break;

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
				dieif(emitidx(s->vm, opc_pop, stpos) <= 0, "");
		} return TYPE_vid;
		//}
		//{ OPER
		case OPER_fnc: {	// '()' emit/cast/call/libc
			astn argv = ast->rhso;
			astn func = ast->lhso;
			dieif(!ast->type, "");

			while (argv && argv->kind == OPER_com) {
				astn arg = argv->rhso;
				int cast = arg->cast;

				if (eval(&tmp, arg, TYPE_any))
					arg = &tmp;

				if (!cgen(s, arg, cast)) {
					debug("push(arg, %+k)", arg);
					return 0;
				}
				argv = argv->lhso;
			}
			if (!func || istype(func)) {			// cast()
				if (!cgen(s, ast->rhso, castId(ast->type))) {
					fatal("cgen(%k)", ast);
					return 0;
				}
				if (!argv || argv != ast->rhso) {
					fatal("multiple or no args '%k'", ast);
					return 0;
				}
				ret = castId(ast->type);
				//~ debug("cast(%+k): %T(%t)", ast->rhso, ast->type, ast->cast);
			}
			else if (isemit(ast)) {					// emit()
				symn opc = linkOf(argv);
				//~ debug("emit(%+k): %T(%T)", ast->rhso, ast->type, opc);
				if (opc && opc->kind == EMIT_opc) {
					ret = castId(opc->type);
					if (ret == TYPE_vid) {
						debug("emit(%+k): %T(%T)", ast->rhso, ast->type, opc);
						ret = get;
					}
					if (emitint(s->vm, opc->offs, opc->init ? opc->init->cint : 0) <= 0)
						debug("opcode expected, not %k", argv);
				}
				else if (opc == type_vid) {
					ret = get;
				}
				else {
					error(s, ast->line, "opcode expected, and or arguments");
					return 0;
				}
			}// */
			else {									// call()
				//~ debug("call(%+k): %t", ast->rhso, ast->cast);
				//~ debug("call(%+k): %T", ast->rhso, ast->type);
				astn arg = argv;
				if (arg) {
					int cast = arg->cast;
					if (eval(&tmp, arg, TYPE_any))
						arg = &tmp;

					if (!cgen(s, arg, cast)) {
						fatal("push(arg, %+k)", arg);
						return 0;
					}
				}
				if (!(ret = cgen(s, ast->lhso, 0))) {
					fatal("cgen:call(%+k)", ast->lhso);
					return 0;
				}
			}
		} break;
		case OPER_idx: {	// '[]'
			debug("TODO(cgen(s, ast->lhso, TYPE_ref))");
			if (cgen(s, ast->lhso, TYPE_u32) <= 0) {
				debug("cgen(lhs, %+k)", ast->lhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (emit(s->vm, opc_ldc4, ast->type->size) <= 0) {
				return 0;
			}
			if (cgen(s, ast->rhso, TYPE_u32) <= 0) {
				debug("cgen(rhs, %+k)", ast->rhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (emit(s->vm, u32_mad) <= 0) return 0;
			//~ if (emit(s->vm, i32_mul) <= 0) return 0;
			//~ if (emit(s->vm, i32_add) <= 0) return 0;

			/*if (!ast->type->init) {		// indirect: []
				emit(s->vm, opc_ldi4);
			}// */
			if ((ret = get) != TYPE_ref) {
				ret = castId(ast->type);
				//~ if (emit(s->vm, opc_ldi, ast->type->size) <= 0) {
					//~ return 0;
				//~ }
			}
		} break; // */
		case OPER_dot: {
			//~ debug("TODO(%+k)", ast);
			if (!cgen(s, ast->rhso, get)) {
				debug("cgen(%k, %+k)", ast, ast->rhso);
				return 0;
			}
			ret = get;
		} break; //*/

		case OPER_not:		// '!'	dieif(ast->cast != TYPE_bit);
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt: {	// '~'
			int opc = -1;
			dieif(!ast->type, "");
			switch (ast->kind) {
				case OPER_pls: return cgen(s, ast->rhso, get);
				case OPER_mns: opc = opc_neg; ret = ast->cast; break;
				case OPER_not: opc = b32_not; get = TYPE_bit; ret = ast->cast; break;
				//~ case OPER_cmt: break;//opc = opc_cmt; ret = ast->Cast; break;
			}
			if (cgen(s, ast->rhso, get) <= 0) {
				debug("cgen(rhs, %+k)", ast->rhso);
				return 0;
			}
			if (emit(s->vm, opc, ret = get) <= 0) {
				debug("emit(%02x, %+k)", opc, ast);
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
			dieif(!ast->type, "");
			switch (ast->kind) {
				case OPER_add: opc = opc_add; ret = ast->cast; break;
				case OPER_sub: opc = opc_sub; ret = ast->cast; break;
				case OPER_mul: opc = opc_mul; ret = ast->cast; break;
				case OPER_div: opc = opc_div; ret = ast->cast; break;
				case OPER_mod: opc = opc_mod; ret = ast->cast; break;

				case OPER_neq:
				case OPER_equ: opc = opc_ceq; ret = TYPE_u32; break;
				case OPER_geq:
				case OPER_lte: opc = opc_clt; ret = TYPE_u32; break;
				case OPER_leq:
				case OPER_gte: opc = opc_cgt; ret = TYPE_u32; break;

				case OPER_shl: opc = opc_shl; ret = ast->cast; break;
				case OPER_shr: opc = opc_shr; ret = ast->cast; break;
				case OPER_and: opc = opc_and; ret = ast->cast; break;
				case OPER_ior: opc = opc_ior; ret = ast->cast; break;
				case OPER_xor: opc = opc_xor; ret = ast->cast; break;
			}
			if (cgen(s, ast->lhso, ast->cast) <= 0) {
				debug("cgen(lhs, %+k)", ast->lhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (cgen(s, ast->rhso, ast->cast) <= 0) {
				debug("cgen(rhs, %+k)", ast->rhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (emit(s->vm, opc, ast->cast) <= 0) {
				debug("emit(%02x, %+k, 0x%02x)", opc, ast, ast->cast);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			switch (ast->kind) {
				case OPER_neq:
				case OPER_leq:
				case OPER_geq: {
					if (emit(s->vm, b32_not, ast->cast) <= 0) {
						debug("emit(%02x, %+k, 0x%02x)", opc, ast, ret);
						dumpxml(stderr, ast, 0, "debug", dxst);
						return 0;
					}
				} break;
			}

		} break;

		//~ case OPER_lnd:		// &&
		//~ case OPER_lor:		// ||
		//~ case OPER_sel:		// ?:

		case ASGN_set: {		// '='
			symn var = linkOf(ast->lhso);
			ret = castId(ast->type);

			dieif(!ast->type, "%T, %T", ast->lhso->type, ast->rhso->type);

			if (cgen(s, ast->rhso, ret) <= 0) {
				debug("cgen(%+k, %t)", ast->rhso, ret);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (var && var->offs < 0) {
				int opcSet, opcDup;
				switch (ret) {
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_f32: opcSet = opc_set1, opcDup = opc_dup1; break;
					case TYPE_i64:
					case TYPE_f64: opcSet = opc_set2, opcDup = opc_dup2; break;
					//~ case TYPE_pf2:
					//~ case TYPE_pf4: opcSet = opc_set4, opcDup = opc_dup4; break;
					default: debug("error (%+k): %04x: %t", ast, ret, ret); return 0;
				}
				if (get != TYPE_vid) {
					if (emit(s->vm, opcDup, 0) <= 0) {
						debug("emit(%+k)", ast);
						return 0;
					}
				}
				if (emitidx(s->vm, opcSet, var->offs) <= 0) {
					debug("emit(%+k, %d)", ast, var->offs);
					return 0;
				}
			}
			else {
				/*int opcSet, opcDup;
				if (!cgen(s, ast->lhso, TYPE_ref)) {
					debug("cgen(lhs, %+k)", ast->lhso);
					return 0;
				}
				switch (ret) {
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_f32: opcSet = opc_sti1, opcDup = opc_dup1; break;
					case TYPE_i64:
					case TYPE_f64: opcSet = opc_sti2, opcDup = opc_dup2; break;
					//~ case TYPE_pf2:
					//~ case TYPE_pf4: opcSet = opc_sti4, opcDup = opc_dup4; break;
					default: debug("error (%+k): %04x: %t", ast, ret, ret); return 0;
				}
				if (get != TYPE_vid) {
					if (!emit(s->vm, opcDup, ret)) {
						debug("emit(%+k)", ast);
						return 0;
					}
				}
				if (!emit(s->vm, opcSet, ret)) {
					return 0;
				}
				// */
				fatal("cgen('%k', `%+k`):%T", ast, ast->rhso, ast->type);
				return 0;
			}

		} break;
		//}
		//{ TVAL
		case CNST_int: switch (get) {
			default:
				debug("invalid cast: [%d->%d](%t to %t) '%+k'", ret, get, ret, get, ast);
				return 0;
			case TYPE_vid: return TYPE_vid;

			//~ case TYPE_bit:
			case TYPE_u32: emiti32(s->vm, ast->cint); return TYPE_u32;
			//~ case TYPE_any:
			case TYPE_int:
			case TYPE_i32: emiti32(s->vm, ast->cint); return TYPE_i32;
			case TYPE_i64: emiti64(s->vm, ast->cint); return TYPE_i64;
			case TYPE_f32: emitf32(s->vm, ast->cint); return TYPE_f32;
			case TYPE_f64: emitf64(s->vm, ast->cint); return TYPE_f64;
		} return 0;
		case CNST_flt: switch (get) {
			default:
				debug("invalid cast: [%d->%d](%t to %t) '%+k'", ret, get, ret, get, ast);
				return 0;
			case TYPE_vid: return TYPE_vid;

			//~ case TYPE_bit:
			case TYPE_u32: emiti32(s->vm, ast->cflt); return TYPE_u32;
			case TYPE_i32: emiti32(s->vm, ast->cflt); return TYPE_i32;
			case TYPE_i64: emiti64(s->vm, ast->cflt); return TYPE_i64;
			case TYPE_f32: emitf32(s->vm, ast->cflt); return TYPE_f32;
			//~ case TYPE_any:
			case TYPE_f64: emitf64(s->vm, ast->cflt); return TYPE_f64;
		} return 0;

		case TYPE_def: {
			symn typ = ast->type;		// type
			symn var = ast->link;		// link
			//~ debug("define %k", ast);
			dieif(!typ || !var, "typ:%T, var:%T", typ, var);

			if (var->kind == TYPE_ref) {		// TYPE_new
				astn val = var->init;

				//~ debug("cgen(var): %+k:%T", ast, ast->type);
				switch (ret = castId(typ)) {
					case TYPE_u32:
					case TYPE_int:
					case TYPE_i32:
					case TYPE_f32: {
						//~ var->onst = 1;// s->level > 1;
						if (val) cgen(s, val, ret);
						else emit(s->vm, opc_ldz1);
						var->offs = emit(s->vm, get_sp);
					} break;
					case TYPE_f64: 
					case TYPE_i64: {
						//~ var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(s->vm, opc_ldz2);
						var->offs = emit(s->vm, get_sp);
					} break;
					case TYPE_p4x: {
						//~ var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(s->vm, opc_ldz2);
						var->offs = emit(s->vm, get_sp);
					} break;
					/*
					//~ case TYPE_ptr:
					case TYPE_arr: {	// malloc();
						//~ debug("unimpl ptr or arr %+k %+T", ast, typ);
						emit(s, opc_ldcr, argi32(0xff00ff));
						//~ var->onst = 1;
						//~ if (val) cgen(s, val, typ->kind);
						//~ else emit(s, opc_ldz1, noarg);
						//~ var->offs = s->vm->rets;
					} break;
					//~ case TYPE_rec:
					//~ case TYPE_enu:	// malloc(); || salloc();
					//~ */
					case TYPE_rec: {
						if (typ->size < 128) {
							emit(s->vm, opc_loc, typ->size);
							var->offs = emit(s->vm, get_sp);
						}
						//~ if (val) cgen(s, val, ret);
						//~ else emit(s, opc_ldz2);
					} break;
					default:
						debug("unimpl %k:%T(%t)", ast, typ, ret);
						return 0;
				}
				return ret;
			}

			/*else if (var->kind == TYPE_def) {	// TYPE_def ...
				astn val = var->init;		// init?
				//~ debug("define(%s:%d) %k = %T", s->cc->file, ast->line, ast, ast->link);
				if (val) {
					debug("define %k = %+k", ast, val);
				}
				else {
					debug("define %k : %?T", ast, typ);
				}
				//~ return var ? cgen(s, var->init, get) : TYPE_vid;
				return TYPE_vid;
			}// */
		} break;
		case TYPE_ref: {		// (var, func)
			symn typ = ast->type;		// type
			symn var = ast->link;		// link
			dieif(!typ || !var, "%+t (%T || %T)", ast->kind, typ, var);

			if (var->kind == TYPE_ref) {
				if (var->libc) {	// libc
					//~ debug("libc(%k): %+T", ast, ast->link);
					ret = castId(typ);
					emit(s->vm, opc_libc, var->offs);
				}
				else if (var->call) {	// call
					debug("call(%k): %+T", ast, ast->link);
					return 0;
				}
				else if (var->offs < 0) {	// on stack
					switch (ret = castId(typ)) {
						default: debug("error %04x: %t", ret, ret); break;
						case TYPE_u32:
						case TYPE_i32:
						case TYPE_f32: emitidx(s->vm, opc_dup1, var->offs); break;
						case TYPE_i64:
						case TYPE_f64: emitidx(s->vm, opc_dup2, var->offs); break;
					}
					//~ fatal("unimpl '%+k'(%T)", ast, var);
					//~ return 0;
				}
				else /* if (var) */ {	// in memory
					fatal("unimpl '%+k'(%T)", ast, var);
					return 0;
				}
			}
			else if (var->kind == TYPE_def) {
				if (var->init)
					ret = cgen(s, var->init, get);
				if (get == TYPE_vid) ret = get;
				if (get == TYPE_any) ret = TYPE_vid;
			}
			else fatal("");
		} break;
		case TYPE_enu: break;
		//}
		default:
			fatal("Node(%t)%s:%d", ast->kind, s->cc->file, ast->line);
			return 0;
	}

	//~ if (get && get != ret) debug("ctyp(`%+k`, %t):%t", ast, get, ret);
	if (get && get != ret) switch (get) {
		// cast
		case TYPE_u32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_i32: break;
			case TYPE_i64: emit(s->vm, i64_i32); break;
			case TYPE_f32: emit(s->vm, f32_i32); break;
			case TYPE_f64: emit(s->vm, f64_i32); break;
			default: goto errorcast;
		} break;
		case TYPE_i32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64: emit(s->vm, i64_i32); break;
			case TYPE_f32: emit(s->vm, f32_i32); break;
			case TYPE_f64: emit(s->vm, f64_i32); break;
			default: goto errorcast;
		} break;
		case TYPE_i64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(s->vm, i32_i64); break;
			case TYPE_f32: emit(s->vm, f32_i64); break;
			case TYPE_f64: emit(s->vm, f64_i64); break;
			default: goto errorcast;
		} break;
		case TYPE_f32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(s->vm, i32_f32); break;
			case TYPE_i64: emit(s->vm, i64_f32); break;
			case TYPE_f64: emit(s->vm, f64_f32); break;
			default: goto errorcast;
		} break;
		case TYPE_f64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(s->vm, i32_f64); break;
			case TYPE_i64: emit(s->vm, i64_f64); break;
			case TYPE_f32: emit(s->vm, f32_f64); break;
			default: goto errorcast;
		} break;

		case TYPE_vid: return TYPE_vid;	// to nothing
		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32: /*emit(s, i32_bol);*/ break;
			case TYPE_i32: /*emit(s, i32_bol);*/ break;
			case TYPE_i64: emit(s->vm, i64_bol); break;
			case TYPE_f32: emit(s->vm, f32_bol); break;
			case TYPE_f64: emit(s->vm, f64_bol); break;
			default: goto errorcast;
		} break;
		//~ case TYPE_ref: 				// address of
		default: fatal("cgen(`%+k`, %t):%t", ast, get, ret);
		errorcast: debug("invalid cast: [%d->%d](%t to %t) %k: '%+k'", ret, get, ret, get, ast, ast);
			return 0;
	}

	return ret;
}

//{ core.c ---------------------------------------------------------------------
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
int compile(state s, srcType mode) {
	if (s->cc == NULL)
		return -1;
	if (scan(s->cc, mode & srcUnit) != 0)
		return -2;
	return ccDone(s);
}
int gencode(state s, int level) {
	//!TODO level

	/* emit data seg
	emit(s->code, seg_data);
	for (sym = s->defs; sym; sym = sym->next) {
		if (sym->kind != TYPE_ref) continue;
		if (sym->init) {
			mem[code->ds] = sym->init;
		}
		code->ds += data->size;
		// TODO initialize
	}// */
	/* emit code seg
	emit(s->code, seg_code);
	cgen(s, s->root, seg_code);
	for (ast = s->root; ast; ast = ast->next) {
		if (isfundecl(ast)) {
			symn fun = linkOf(ast);
			fun->offs = emit(s->code, get_ip);
			cgen(ast->stmt);
		}
	}
	for (ast = s->root; ast; ast = ast->next) {
		if (!isfundecl(ast)) {
			cgen(ast->stmt);
		}
	}
	// emit(s->code, call, ".main()");
	// emit(s->code, call, ".exit()");
	// */
	/* emit type inf (debug)
	for (data = s->defs; data; data = data->next) {
		if (data->kind != TYPE_def) continue;
		emit_typ(data);
		// TODO initialize
	}// */

	if (!vmInit(s))
		return -1;

	emit(s->vm, loc_data, 256 * 4);
	emit(s->vm, seg_code);
	cgen(s, s->cc->root, TYPE_any);	// TODO: TYPE_vid: clear the stack
	emit(s->vm, opc_sysc, 0);
	emit(s->vm, seg_code);
	return s->errc;
}

//~ int execute(state s, int cc, int ss) {return 0;}

int evalexp(ccEnv s, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	source(s, 0, text);
	ast = expr(s, 0);
	typ = lookup(s, 0, ast);
	tid = eval(&res, ast, TYPE_flt);

	if (peek(s))
		fputfmt(stdout, "unexpected: `%k`\n", peek(s));

	fputfmt(stdout, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(stdout, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(stdout, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);
	//~ dumpast(stdout, ast, 15);

	return -1;
}

symn install3(ccEnv s, int kind, const char* name, unsigned size, symn typ, astn val) {
	symn def = install(s, kind, name, size);
	if (def) {
		def->type = typ;
		def->init = val;
	}
	return def;
}

void instint(ccEnv s, symn it, int64t sgn) {
	int size = it->size;
	int bits = size * 8;
	int64t mask = (-1LLU >> -bits);
	int64t minv = sgn << (bits - 1);
	int64t maxv = (minv - 1) & mask;
	enter(s, it);
	install3(s, CNST_int, "min",  0, type_i64, intnode(s, minv));
	install3(s, CNST_int, "max",  0, type_i64, intnode(s, maxv));
	install3(s, CNST_int, "mask", 0, type_i64, intnode(s, mask));
	install3(s, CNST_int, "bits", 0, type_i64, intnode(s, bits));
	install3(s, CNST_int, "size", 0, type_i64, intnode(s, size));
	it->args = leave(s);
}//~ */

symn type_f32x4 = NULL;
symn type_f64x2 = NULL;

ccEnv ccInit(state s) {
	ccEnv c = getmem(s, sizeof(struct ccEnv));
	int i, TYPE_opc = EMIT_opc;
	symn typ, def;
	symn type_i08 = 0, type_i16 = 0;
	symn type_u08 = 0, type_u16 = 0;
	//~ symn type_u64 = 0, type_f16 = 0;

	if (c == NULL)
		return NULL;

	c->s = s;
	s->cc = c;

	s->errc = 0;
	s->logf = 0;

	//~ s->warn = wl; s->copt = ol;

	c->file = 0; c->line = c->nest = 0;

	c->_fin = c->_chr = -1;
	c->_ptr = 0; c->_cnt = 0;

	c->root = 0;
	c->defs = 0;
	//~ s->vm = 0;

	c->tokp = 0;
	c->_tok = 0;
	c->all = 0;

	c->deft = getmem(s, TBLS * sizeof(symn));
	c->strt = getmem(s, TBLS * sizeof(list));
	c->buffp = s->_ptr;

	//{ install Type
	//+ type_bit = install(c, "bit",  -1, TYPE_bit);
	//+ -> bitfields <=> array of bits ???

	type_vid = install(c, TYPE_vid, "void",  0);

	type_bol = install(c, TYPE_bit, "bool",  1);

	type_u08 = install(c, TYPE_bit, "uns8", 1);
	type_u16 = install(c, TYPE_bit, "uns16", 2);
	type_u32 = install(c, TYPE_bit, "uns32", 4);
	//~ type_u64 = install(c, TYPE_uns, "uns64", 8);

	type_i08 = install(c, TYPE_int, "int8", 1);
	type_i16 = install(c, TYPE_int, "int16", 2);
	type_i32 = install(c, TYPE_int, "int32", 4);
	type_i64 = install(c, TYPE_int, "int64", 8);

	//~ type_f16 = install(c, TYPE_flt, "flt16", 2);
	type_f32 = install(c, TYPE_flt, "flt32", 4);
	type_f64 = install(c, TYPE_flt, "flt64", 8);

	//~ type = install(c, TYPE_flt, "i8x16", 8);
	//~ type = install(c, TYPE_flt, "i16x8", 8);
	//~ type = install(c, TYPE_flt, "i32x4", 8);
	//~ type = install(c, TYPE_flt, "i64x2", 8);
	//~ type = install(c, TYPE_flt, "u8x16", 8);
	//~ type = install(c, TYPE_flt, "u16x8", 8);
	//~ type = install(c, TYPE_flt, "u32x4", 8);
	//~ type = install(c, TYPE_flt, "u64x2", 8);
	//~ type = install(c, TYPE_flt, "f16x8", 8);
	type_f32x4 = install(c, TYPE_p4x, "f32x4", 16);
	type_f64x2 = install(c, TYPE_p4x, "f64x2", 16);

	//~ type_arr = install(c, TYPE_ptr, "array", 0);
	//~ type_str = install(c, TYPE_ptr, "string", 0);

	instint(c, type_i08, -1); instint(c, type_u08, 0);
	instint(c, type_i16, -1); instint(c, type_u16, 0);
	instint(c, type_i32, -1); instint(c, type_u32, 0);
	instint(c, type_i64, -1);// instint(c, type_u64, 0);

	install(c, TYPE_def, "int", 0)->type = type_i32;
	install(c, TYPE_def, "long", 0)->type = type_i64;
	install(c, TYPE_def, "float", 0)->type = type_f32;
	install(c, TYPE_def, "double", 0)->type = type_f64;

	install3(c, CNST_int, "true", 0, type_i32, intnode(c, 1));
	install3(c, CNST_int, "false", 0, type_i32, intnode(c, 0));

	//} */// types
	//{ install Emit
	emit_opc = install(c, TYPE_opc, "emit", 0);
	//~ emit_opc->call = 1;
	//~ emit_opc->type = 0;
	enter(c, emit_opc);

	install(c, TYPE_opc, "nop", 0)->type = type_vid;

	enter(c, typ = install(c, TYPE_int, "i32", 4));
	install(c, TYPE_opc, "neg", i32_neg)->type = type_i32;
	install(c, TYPE_opc, "add", i32_add)->type = type_i32;
	install(c, TYPE_opc, "sub", i32_sub)->type = type_i32;
	install(c, TYPE_opc, "mul", i32_mul)->type = type_i32;
	install(c, TYPE_opc, "div", i32_div)->type = type_i32;
	install(c, TYPE_opc, "mod", i32_mod)->type = type_i32;
	typ->args = leave(c);

	enter(c, typ = install(c, TYPE_int, "i64", 8));
	install(c, TYPE_opc, "neg", i64_neg)->type = type_i64;
	install(c, TYPE_opc, "add", i64_add)->type = type_i64;
	install(c, TYPE_opc, "sub", i64_sub)->type = type_i64;
	install(c, TYPE_opc, "mul", i64_mul)->type = type_i64;
	install(c, TYPE_opc, "div", i64_div)->type = type_i64;
	install(c, TYPE_opc, "mod", i64_mod)->type = type_i64;
	typ->args = leave(c);

	enter(c, typ = install(c, TYPE_flt, "f32", 4));
	install(c, TYPE_opc, "neg", f32_neg)->type = type_f32;
	install(c, TYPE_opc, "add", f32_add)->type = type_f32;
	install(c, TYPE_opc, "sub", f32_sub)->type = type_f32;
	install(c, TYPE_opc, "mul", f32_mul)->type = type_f32;
	install(c, TYPE_opc, "div", f32_div)->type = type_f32;
	install(c, TYPE_opc, "mod", f32_mod)->type = type_f32;
	typ->args = leave(c);

	enter(c, typ = install(c, TYPE_flt, "f64", 8));
	install(c, TYPE_opc, "neg", f64_neg)->type = type_f64;
	install(c, TYPE_opc, "add", f64_add)->type = type_f64;
	install(c, TYPE_opc, "sub", f64_sub)->type = type_f64;
	install(c, TYPE_opc, "mul", f64_mul)->type = type_f64;
	install(c, TYPE_opc, "div", f64_div)->type = type_f64;
	install(c, TYPE_opc, "mod", f64_mod)->type = type_f64;
	typ->args = leave(c);

	enter(c, typ = install(c, TYPE_p4x, "v4f", 16));
	install(c, TYPE_opc, "neg", v4f_neg)->type = type_f32x4;
	install(c, TYPE_opc, "add", v4f_add)->type = type_f32x4;
	install(c, TYPE_opc, "sub", v4f_sub)->type = type_f32x4;
	install(c, TYPE_opc, "mul", v4f_mul)->type = type_f32x4;
	install(c, TYPE_opc, "div", v4f_div)->type = type_f32x4;
	install(c, TYPE_opc, "dp3", v4f_dp3)->type = type_f32;
	install(c, TYPE_opc, "dp4", v4f_dp4)->type = type_f32;
	install(c, TYPE_opc, "dph", v4f_dph)->type = type_f32;
	typ->args = leave(c);

	enter(c, typ = install(c, TYPE_p4x, "v2d", 16));
	install(c, TYPE_opc, "neg", v2d_neg)->type = type_f64x2;
	install(c, TYPE_opc, "add", v2d_add)->type = type_f64x2;
	install(c, TYPE_opc, "sub", v2d_sub)->type = type_f64x2;
	install(c, TYPE_opc, "mul", v2d_mul)->type = type_f64x2;
	install(c, TYPE_opc, "div", v2d_div)->type = type_f64x2;
	typ->args = leave(c);

	enter(c, typ = install(c, TYPE_p4x, "swz", 0x96));
	for (i = 0; i < 256; i += 1) {
		symn swz;
		c->buffp[0] = "xyzw"[i>>0&3];
		c->buffp[1] = "xyzw"[i>>2&3];
		c->buffp[2] = "xyzw"[i>>4&3];
		c->buffp[3] = "xyzw"[i>>6&3];
		c->buffp[4] = 0;
		swz = install(c, TYPE_opc, mapstr(c, c->buffp, 4, -1), typ->offs);
		swz->init = intnode(c, i);
		swz->type = typ;
	}
	typ->args = leave(c);
	//~ install(c, EMIT_opc, "swz", 0x96)->args = swz;
	//~ install(c, TYPE_enu, "dup", 0x97)->args = swz;
	//~ install(c, TYPE_enu, "set", 0x98)->args = swz;

	emit_opc->args = leave(c);

	//} */
	//{ install Math
	enter(c, def = install(c, TYPE_enu, "math", 0));

	install(c, CNST_flt, "nan", 0)->init = fh8node(c, 0x7FFFFFFFFFFFFFFFLL);	// Qnan
	install(c, CNST_flt, "Snan", 0)->init = fh8node(c, 0xfff8000000000000LL);	// Snan
	install(c, CNST_flt, "inf", 0)->init = fh8node(c, 0x7ff0000000000000LL);
	install(c, CNST_flt, "l2e", 0)->init = fh8node(c, 0x3FF71547652B82FELL);	// log_2(e)
	install(c, CNST_flt, "l2t", 0)->init = fh8node(c, 0x400A934F0979A371LL);	// log_2(10)
	install(c, CNST_flt, "lg2", 0)->init = fh8node(c, 0x3FD34413509F79FFLL);	// log_10(2)
	install(c, CNST_flt, "ln2", 0)->init = fh8node(c, 0x3FE62E42FEFA39EFLL);	// log_e(2)
	install(c, CNST_flt, "pi", 0)->init = fh8node(c, 0x400921fb54442d18LL);		// 3.1415...
	install(c, CNST_flt, "e", 0)->init = fh8node(c, 0x4005bf0a8b145769LL);		// 2.7182...
	def->args = leave(c);
	//} */
	//{ install Libc
	//~ enter(s, def = install(c, TYPE_enu, "Libc", 0));
	installlibc(s, NULL, NULL);
	//~ def->args = leave(s);
	//} */

	/*{ install ccon
	enter(s, typ = install(c, TYPE_enu, "ccon", 0));
	install(c, CNST_int, "version", 0)->init = intnode(s, 0x20090400);
	install(c, CNST_str, "host", 0)->init = strnode(s, (char*)os);
	//? install(c, TYPE_def, "emit", 0);// warn(int, void ...);
	//? install(c, TYPE_def, "warn", 0);// warn(int, string);

	//- install(c, CNST_str, "type", 0);// current type;
	//~ install(c, CNST_str, "date", 0)->init = &c->ccd;
	//~ install(c, CNST_str, "file", 0)->init = &c->ccfn;
	//~ install(c, CNST_str, "line", 0)->init = &c->ccfl;
	//~ install(c, CNST_str, "time", 0)->init = &c->ccft;
	typ->args = leave(s);

	//} */
	/*{ install operators
	install(c, -1, "int32 .add(int32 a, uns32 b) {return emit(add.i32, i32(a), i32(b));}", 0);
	install(c, -1, "int64 .add(int32 a, uns64 b) {return emit(add.i64, i64(a), i64(b));}", 0);
	install(c, -1, "int32 .add(int32 a, int32 b) {return emit(add.i32, i32(a), i32(b));}", 0);
	install(c, -1, "int64 .add(int32 a, int64 b) {return emit(add.i64, i64(a), i64(b));}", 0);
	install(c, -1, "flt32 .add(int32 a, flt32 b) {return emit(add.f32, f32(a), f32(b));}", 0);
	install(c, -1, "flt64 .add(int32 a, flt64 b) {return emit(add.f64, f64(a), f64(b));}", 0);
	install(c, -1, "int32 .add(out int32 a, uns32 b) {return a = add(a, b)}", 0);
	install(c, -1, "int64 .add(out int32 a, uns64 b) {return a = add(a, b)}", 0);
	install(c, -1, "int32 .add(out int32 a, int32 b) {return a = add(a, b)}", 0);
	install(c, -1, "int64 .add(out int32 a, int64 b) {return a = add(a, b)}", 0);
	install(c, -1, "flt32 .add(out int32 a, flt32 b) {return a = add(a, b)}", 0);
	install(c, -1, "flt64 .add(out int32 a, flt64 b) {return a = add(a, b)}", 0);

	install(c, -1, "int32 add(int32 a, uns32 b) {return emit(i32.add, i32(a), i32(b));}", 0);	// +
	install(c, -1, "int32 sub(int32 a, uns32 b) {return emit(i32.sub, i32(a), i32(b));}", 0);	// -
	install(c, -1, "int32 mul(int32 a, uns32 b) {return emit(i32.mul, i32(a), i32(b));}", 0);	// *
	install(c, -1, "int32 div(int32 a, uns32 b) {return emit(i32.div, i32(a), i32(b));}", 0);	// /
	install(c, -1, "int32 mod(int32 a, uns32 b) {return emit(i32.mod, i32(a), i32(b));}", 0);	// %

	install(c, -1, "int32 ceq(int32 a, uns32 b) {return emit(b32.ceq, i32(a), i32(b));}", 0);	// ==
	install(c, -1, "int32 cne(int32 a, uns32 b) {return emit(b32.cne, i32(a), i32(b));}", 0);	// !=
	install(c, -1, "int32 clt(int32 a, uns32 b) {return emit(i32.clt, i32(a), i32(b));}", 0);	// <
	install(c, -1, "int32 cle(int32 a, uns32 b) {return emit(i32.cle, i32(a), i32(b));}", 0);	// <=
	install(c, -1, "int32 cgt(int32 a, uns32 b) {return emit(i32.cgt, i32(a), i32(b));}", 0);	// >
	install(c, -1, "int32 cge(int32 a, uns32 b) {return emit(i32.cge, i32(a), i32(b));}", 0);	// >=

	install(c, -1, "int32 shr(int32 a, uns32 b) {return emit(b32.shl, i32(a), i32(b));}", 0);	// >>
	install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.sar, i32(a), i32(b));}", 0);	// <<
	install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.and, i32(a), i32(b));}", 0);	// &
	install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32. or, i32(a), i32(b));}", 0);	// |
	install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.xor, i32(a), i32(b));}", 0);	// ^

	install(c, -1, "int32 pls(int32 a) asm {return emit(void, i32(a));}", 0);	// +
	install(c, -1, "int32 neg(int32 a) asm {return emit(neg.i32, i32(a));}", 0);	// -
	install(c, -1, "int32 cmt(int32 a) asm {return emit(cmt.b32, i32(a));}", 0);	// ~
	install(c, -1, "int32 not(int32 a) asm {return emit(b32.not, i32(a));}", 0);	// !

	//} */
	return s->cc;
}
vmEnv vmInit(state s) {
	int size = s->_cnt;
	vmEnv vm = getmem(s, size);
	if (vm != NULL) {
		//~ memset(vm, 0, sizeof(struct vmEnv));
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
		if (source(s->cc, mode, src) != 0)
			return NULL;
	}
	return s->cc;
}

int ccDone(state s) {
	// if not initialized
	if (s->cc == NULL)
		return 0;

	//~ if (s->cc->nest)
		//~ error(s->cc, s->cc->line, "premature end of file");

	//~ s->cc->nest = 0;

	// close file
	source(s->cc, 0, 0);

	// set used memory
	s->_cnt -= s->cc->buffp - s->_ptr;
	s->_ptr = s->cc->buffp;

	return s->errc;
}
int vmDone(state s) {
	// if not initialized
	if (s->vm == NULL)
		return 0;
	//TODO:...
	return 0;
}

void* getmem(state s, int size) {
	if (size <= s->_cnt) {
		void *mem = s->_ptr;
		s->_cnt -= size;
		s->_ptr += size;
		return mem;
	}
	return NULL;
}

//}
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
			#include "incl/exec.c"
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
int vm_test() {
	int e = 0;
	struct bcdc_t opc, *ip = &opc;
	opc.arg.i8 = 0;
	for (opc.opc = 0; opc.opc < opc_last; opc.opc++) {
		int err = 0;
		if (opc_tbl[opc.opc].size == 0) continue;
		if (opc_tbl[opc.opc].code != opc.opc) {
			fprintf(stderr, "invalid '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc);
			e += err = 1;
		}
		else switch (opc.opc) {
			error_len: e += 1; debug("opcode size 0x%02x: '%A'", opc.opc, ip); break;
			error_chk: e += 1; debug("stack min size 0x%02x: '%A'", opc.opc, ip); break;
			error_dif: e += 1; debug("stack difference 0x%02x: '%A'", opc.opc, ip); break;
			error_opc: e += 1; debug("unimplemented opcode 0x%02x: '%A'", opc.opc, ip); break;
			#define NEXT(__IP, __CHK, __DIF) {\
				if (opc_tbl[opc.opc].size != 0 && opc_tbl[opc.opc].size != (__IP)) goto error_len;\
				if (opc_tbl[opc.opc].chck != 9 && opc_tbl[opc.opc].chck != (__CHK)) goto error_chk;\
				if (opc_tbl[opc.opc].diff != 9 && opc_tbl[opc.opc].diff != (__DIF)) goto error_dif;\
			}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "incl/exec.c"
		}
	}
	return e;
}
int mk_test(char *file, int size) {
	//~ char test[] = "signed _000000000000000001 = 8;\n";
	char test[] = "int _0000001=6;\n";
	FILE* f = fopen(file, "wb");
	int n, sp = sizeof(test) - 6;

	if (!f) {
		debug("cann not open file\n");
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
	else debug("file is too large (128M max)\n");

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

//~ symn lookin(symn sym, astn ref, astn args);
symn findsym(ccEnv s, char *name) {
	struct astn ast;
	memset(&ast, 0, sizeof(struct astn));
	ast.kind = TYPE_ref;
	ast.name = name;
	ast.hash = rehash(name, strlen(name));
	return lookin(s->defs, &ast, NULL);
}
int lookupint(ccEnv s, char *name, int* res) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init, TYPE_int)) {
		*res = ast.cint;
		return 1;
	}
	return 0;
}
int lookupflt(ccEnv s, char *name, double* res) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init, TYPE_flt)) {
		*res = ast.cflt;
		return 1;
	}
	return 0;
}

// lookup a value 
int lookup_nz(ccEnv s, char *name) {
	struct astn ast;
	symn sym = findsym(s, name);
	if (sym && eval(&ast, sym->init, TYPE_bit)) {
		return ast.cint;
	}
	return 0;
}

int vmHelp(char *cmd) {
	FILE *out = stdout;
	int i, k, n = 0;
	for (i = 0; i < opc_last; ++i) {
		char *opc = (char*)opc_tbl[i].name;
		if (opc && strfindstr(opc, cmd, 1)) {
			fputfmt(out, "Instruction: %s\n", opc);
			n += 1;
			k = i;
		}
	}
	if (n == 1 && strcmp(cmd, opc_tbl[k].name) == 0) {
		fputfmt(out, "Opcode: 0x%02x\n", opc_tbl[k].code);
		fputfmt(out, "Length: %d\n", opc_tbl[k].size);
		fputfmt(out, "Stack min: %d\n", opc_tbl[k].chck);
		fputfmt(out, "Stack diff: %d\n", opc_tbl[k].diff);
		//~ fputfmt(out, "0x%02x	%d		\n", opc_tbl[k].code, opc_tbl[k].size-1);
		//~ fputfmt(out, "\nDescription\n");
		//~ fputfmt(out, "The '%s' instruction %s\n", opc_tbl[k].name, "#");
		//~ fputfmt(out, "\nOperation\n");
		//~ fputfmt(out, "#\n");
		//~ fputfmt(out, "\nExceptions\n");
		//~ fputfmt(out, "None#\n");
		//~ fputfmt(out, "\n");		// end of text
	}
	else if (n == 0) {
		fputfmt(out, "No Entry for: '%s'\n", cmd);
	}
	return n;
}

//} */

void usage(state s, char* prog, char* cmd) {
	if (cmd == NULL) {
		fputfmt(stdout, "Usage: %s <command> [options] ...\n", prog);
		fputfmt(stdout, "<Commands>\n");
		fputfmt(stdout, "\t-c: compile\n");
		fputfmt(stdout, "\t-e: execute\n");
		fputfmt(stdout, "\t-d: diassemble\n");
		fputfmt(stdout, "\t-h: help\n");
		fputfmt(stdout, "\t=<expression>: eval\n");
		//~ fputfmt(stdout, "\t-d: dump\n");
	}
	else if (strcmp(cmd, "-c") == 0) {
		fputfmt(stdout, "compile: %s -c [options] files...\n", prog);
		fputfmt(stdout, "Options:\n");

		fputfmt(stdout, "\t[Output]\n");
		fputfmt(stdout, "\t-o <file> set file for output. [default=stdout]\n");
		fputfmt(stdout, "\t-t tags\n");
		fputfmt(stdout, "\t-s assembled code\n");

		fputfmt(stdout, "\t[Loging]\n");
		fputfmt(stdout, "\t-l <file> set file for errors. [default=stderr]\n");
		fputfmt(stdout, "\t-w<num> set warning level to <num> [default=%d]\n", wl);
		fputfmt(stdout, "\t-wa all warnings\n");
		fputfmt(stdout, "\t-wx treat warnings as errors\n");

		fputfmt(stdout, "\t[Debuging]\n");
		fputfmt(stdout, "\t-(ast|xml) output format\n");
		fputfmt(stdout, "\t-x<n> execute on <n> procs [default=%d]\n", cc);
		fputfmt(stdout, "\t-xd<n> debug on <n> procs [default=%d]\n", cc);

		//~ fputfmt(stdout, "\t[Debug & Optimization]\n");
	}
	else if (strcmp(cmd, "-e") == 0) {
		fputfmt(stdout, "command: '-e': execute\n");
	}
	else if (strcmp(cmd, "-d") == 0) {
		fputfmt(stdout, "command: '-d': disassemble\n");
	}
	else if (strcmp(cmd, "-m") == 0) {
		fputfmt(stdout, "command: '-m': make\n");
	}
	else if (strcmp(cmd, "-h") == 0) {
		fputfmt(stdout, "command: '-h': help\n");
	}
	else {
		fputfmt(stdout, "invalid help for: '%s'\n", cmd);
	}
}

int dbgInfo(vmEnv vm, int pu, void *ip, long* sptr, int sc) {
	if (ip == NULL) {
		vmInfo(vm);
		if (vm->s && vm->s->cc)
			vm_tags(vm->s->cc, (char*)sptr, sc);
		else if (!vm->s) {
			debug("!vm->s");
		}
		return 0;
	}
	return 0;
}
int dbgCon(vmEnv vm, int pu, void *ip, long* sptr, int sc) {
	static char buff[1024], cmd = 'n';
	char *arg;

	if (ip == NULL) {
		vmInfo(vm);
		if (vm->s && vm->s->cc)
			vm_tags(vm->s->cc, (char*)sptr, sc);
		else if (!vm->s) {
			debug("!vm->s");
		}
		else if (!vm->s->cc) {
			debug("!s->cc");
		}
		return 0;
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	fputfmt(stdout, ">exec:pu%02d:[ss:%03d]: %A\n", pu, sc, ip);

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

		switch (cmd) {
			default:
				debug("invalid command '%c'", cmd);
			case 0 :
				break;

			case 'r' :		// continue
			//~ case 'c' :		// step in
			//~ case 'C' :		// step out
			case 'n' :		// step over
				return 0;
			case 'p' : if (vm->s && vm->s->cc) {		// print
				symn sym = findsym(vm->s->cc, arg);
				if (sym && sym->offs < 0) {
					int i = sc - sym->offs;
					stkval* sp = (stkval*)(sptr + i);
					fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp->i4, sp->f4, sp->i8, sp->f8);
				}
			} break;
			case 's' : {
				stkval* sp = (stkval*)sptr;
				if (strcmp(arg, "all") == 0) fputfmt(stdout, "\tsp: {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
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
					if (strcmp(arg, "all") == 0) fputfmt(stdout, "\tsp(%d): {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", i, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
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
int nodbg(vmEnv vm, int pu, void *ip, long* sptr, int sc) {
	return 0;
}

int program(int argc, char *argv[]) {
	static struct state s[1];
	char *prg, *cmd, hexc = '#';
	dbgf dbg = dbgInfo;

	s->_cnt = sizeof(s->_mem);
	s->_ptr = s->_mem;
	s->cc = 0;
	s->vm = 0;

	prg = argv[0];
	cmd = argv[1];
	if (argc <= 2) {
		if (argc < 2) {
			usage(s, prg, NULL);
		}
		else if (*cmd != '-') {
			return evalexp(ccInit(s), cmd);
		}
		else if (strcmp(cmd, "-api") == 0) {
			dumpsym(stdout, leave(ccInit(s)), 1);
		}
		else if (strcmp(cmd, "-syms") == 0) {
			symn sym = leave(ccInit(s));
			while (sym) {
				dumpsym(stdout, sym, 0);
				sym = sym->defs;
			}
			//~ dumpsym(stdout, leave(s), 1);
		}
		else if (strcmp(cmd, "-emit") == 0) {
			ccInit(s);
			dumpsym(stdout, emit_opc->args, 1);
		}
		else usage(s, prg, cmd);
	}
	else if (strcmp(cmd, "-c") == 0) {	// compile
		int level = -1, argi = 2;
		int warn = wl;
		int outc = 0;			// output
		char *srcf = 0;			// source
		char *logf = 0;			// logger
		char *outf = 0;			// output
		enum {
			gen_code = 0x0010,
			out_tags = 0x0011,	// tags	// ?offs?
			out_tree = 0x0002,	// walk

			out_dasm = 0x0013,	// dasm
			run_code = 0x0014,	// exec
		};

		// options
		while (argi < argc) {
			char *arg = argv[argi];

			// source file
			if (*arg != '-') {
				if (srcf) {
					fputfmt(stderr, "multiple sources not suported\n");
					return -1;
				}
				srcf = arg;
			}

			// output file
			else if (strcmp(arg, "-l") == 0) {		// log
				if (++argi >= argc || logf) {
					fputfmt(stderr, "logger error\n");
					return -1;
				}
				logf = arg;
			}
			else if (strcmp(arg, "-o") == 0) {		// out
				if (++argi >= argc || outf) {
					fputfmt(stderr, "output error\n");
					return -1;
				}
				outf = arg;
			}

			// output text
			else if (strncmp(arg, "-x", 2) == 0) {		// exec
				char *str = arg + 2;
				outc = run_code;
				if (*str == 'd') {
					dbg = dbgCon;
					str += 1;
				}
				else if (!parseInt(arg + 2, &level, 0)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-t", 2) == 0) {		// tags
				if (!parseInt(arg + 2, &level, hexc)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
				outc = out_tags;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// dasm
				if (!parseInt(arg + 2, &level, hexc)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
				outc = out_dasm;
			}
			else if (strncmp(arg, "-c", 2) == 0) {		// tree
				if (!parseInt(arg + 2, &level, hexc)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
				outc = out_tree;
			}

			// Override settings
			else if (strncmp(arg, "-w", 2) == 0) {		// warning level
				if (strcmp(arg, "-wx"))
					warn = -1;
				else if (strcmp(arg, "-wa"))
					warn = 9;
				else if (!parseInt(arg + 2, &warn, 0)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			/*else if (strncmp(arg, "-d", 2) == 0) {		// optimize/debug level
				return -1;
			}*/

			else {
				fputfmt(stderr, "invalid option '%s' for -compile\n", arg);
				return -1;
			}
			++argi;
			//~ debug("level :0x%02x: arg[%d]: '%s'", level, argi - 2, arg);
		}
		if (logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (srcfile(s, srcf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (compile(s, !srcUnit) != 0) {
			//~ fputfmt(stderr, "can not open file `%s`\n", srcf);
			return s->errc;
		}
		if (gencode(s, 0) != 0) {
			//~ fputfmt(stderr, "can not open file `%s`\n", srcf);
			return s->errc;
		}

		switch (outc) {
			case out_tree: dump(s, outf, dump_ast | (level & 0xff), NULL); break;
			case out_dasm: dump(s, outf, dump_asm | (level & 0xff), NULL); break;
			case out_tags: dump(s, outf, dump_sym | (1), NULL); break;
			case run_code: exec(s->vm, cc, ss, dbg); break;
		}

		return 0;
	}
	else if (strcmp(cmd, "-e") == 0) {	// execute
		fatal("unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return exec(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-d") == 0) {	// assemble
		fatal("unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return dumpasm(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-m") == 0) {	// make
		fatal("unimplemented option '%s' \n", cmd);
	}
	else if (strcmp(cmd, "-h") == 0) {	// help
		char *t = argv[2];
		if (argc < 3) {
			usage(s, argv[0], argc < 4 ? argv[3] : NULL);
		}
		else if (strcmp(t, "-s") == 0) {
			int i = 3;
			if (argc == 3) {
				vmHelp("?");
			}
			else while (i < argc) {
				vmHelp(argv[i++]);
			}
		}
		//~ else if (strcmp(t, "-x") == 0) ;
	}
	else if (argc == 2 && *cmd != '-') {	// try to eval
		return evalexp(ccInit(s), cmd);
	}
	else fputfmt(stderr, "invalid option '%s'", cmd);
	return 0;
}
