#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include "main.h"

// default values
static const int wl = 9;		// warninig level
static const int ol = 1;		// optimize level

//~ static const int dl = 2;		// execution level
static const int cc = 1;		// execution cores
static const int ss = 1 << 10;		// execution stack(256K)
//~ static const int hs = 128 << 20;		// execution heap(128M)
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
//~ #include "clog.c"
//~ #include "scan.c"
//~ #include "tree.c"
//~ #include "type.c"
//~ #include "code.c"

symn emit_opc = 0;
int cgen(state s, astn ast, int get) {
	struct astn tmp;
	int ret = 0;
	const int dxst = 15;	// dump
	vmEnv code = s->code;

	if (!ast) return 0;

	//~ debug("cgen:(%+k):%?T", ast, ast->type);

	switch (ast->kind) {
		//{ STMT
		case OPER_nop: {		// expr statement
			int er, beg = emit(code, get_ip);//code->cs;
			dieif(!ast->stmt);					// is cleaned
			emit(code, opc_line, ast->line);
			er = cgen(s, ast->stmt, TYPE_vid);
			#if 0
			void dasm(FILE *, unsigned char *beg, int len, int offs);
			info(s, s->file, ast->line, "cgen(%+k): %s", ast->stmt, tok_tbl[er].name);
			dasm(stdout, code->memp + beg, code->cs - beg, beg);
			#endif
			beg = emit(code, get_ip);
		} return 0;
		case OPER_beg: {		// {}
			//~ symn typ;
			astn ptr;
			int stpos = emit(code, get_sp);//code->ss;
			emitint(code, opc_line, ast->line);
			for (ptr = ast->stmt; ptr; ptr = ptr->next) {
				if (cgen(s, ptr, TYPE_vid) <= 0){}
			}
			//~ for (typ = ast->type; typ; typ = typ->next)
				//~ if (typ->onst) stc += 1;
			//~ if (stc) emiti32(s, opc_pop, stc);
			if (stpos < emit(code, get_sp) && get == TYPE_vid) {
				//~ emiti32(code, opc_pop, emit(code, get_sp) - stpos);
				emitidx(code, opc_pop, stpos);
			}
			else if (stpos > emit(code, get_sp)) {
				fatal(s, "stack underflow in %-k", ast);
				return -1;
			}
		} return 0;
		case OPER_jmp: {		// if ( ) then {} else {}
			int tt = eval(&tmp, ast->test, TYPE_bit);
			emitint(code, opc_line, ast->line);
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
				if (s->copt > 0 && tt) {		// if true
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
					jmpt = emit(code, opc_jz);
					cgen(s, ast->stmt, TYPE_vid);
					jmpf = emit(code, opc_jmp);
					fixjump(code, jmpt, emit(code, get_ip), 0);
					cgen(s, ast->step, TYPE_vid);
					fixjump(code, jmpf, emit(code, get_ip), 0);
				}
				else if (ast->stmt) {					// if then
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast->test);
						return 0;
					}
					//~ if false skip THEN block
					jmpt = emit(code, opc_jz);
					cgen(s, ast->stmt, TYPE_vid);
					fixjump(code, jmpt, emit(code, get_ip), 0);
				}
				else if (ast->step) {					// if else
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast);
						return 0;
					}
					//~ if true skip ELSE block
					jmpt = emit(code, opc_jnz);
					cgen(s, ast->step, TYPE_vid);
					fixjump(code, jmpt, emit(code, get_ip), 0);
				}
				else {
					//~ cgen(s, ast->test);
					//~ print(s, 8, s->file, ast->line, "");
				}
			}
			else debug("unimplemented: goto / break/ continue;");
		} return 0;
		case OPER_for: {		// for ( ; ; ) {}
			//!TODO: replace 'xxx = code->cs' with 'xxx = emit(...)'
			int beg, end, cmp = -1;
			int stpos = emit(code, get_sp);

			emitint(code, opc_line, ast->line);
			cgen(s, ast->init, TYPE_vid);

			//~ if (ast->cast == QUAL_par) ;		// 'parallel for'
			//~ else if (ast->cast == QUAL_sta) ;	// 'static for' (unroll)
			beg = emit(code, get_ip);
			if (ast->step) {
				int tmp = beg;
				if (ast->init) emit(code, opc_jmp, 0);

				beg = emit(code, get_ip);
				cgen(s, ast->step, TYPE_vid);
				if (ast->init) fixjump(code, tmp, emit(code, get_ip), 0);
			}
			if (ast->test) {
				cgen(s, ast->test, TYPE_bit);
				cmp = emit(code, opc_jz, 0);		// if (!test) break;
			}

			// push(list_jmp, 0);
			cgen(s, ast->stmt, TYPE_vid);
			end = emit(code, opc_jmp, beg);				// continue;
			fixjump(code, end, beg, 0);
			fixjump(code, cmp, emit(code, get_ip), 0);	// break;
			//~ fixjump(s, cmp, end, 0);				// break;

			//~ while (list_jmp) {
			//~ if (list_jmp->kind == break)
			//~ 	fixj(s, list_jmp->offs, end, 0);
			//~ if (list_jmp->kind == continue)
			//~ 	fixj(s, list_jmp->offs, beg, 0);
			//~ list_jmp = list_jmp->next;
			//~ }
			// pop(list_jmp);


			//~ for (typ = ast->type; typ; typ = typ->next)
				//~ if (typ->onst) stc += typ->size;
			if (stpos < emit(code, get_sp)) {
				//~ emiti32(code, opc_pop, emit(code, get_sp) - stpos);
				emitidx(code, opc_pop, stpos);
			}
			else if (stpos > emit(code, get_sp)) {
				fatal(s, "stack underflow: near statement %+k", ast);
				return -1;
			}
		} return 0;
		//}
		//{ OPER
		case OPER_fnc: {		// '()' emit/cast/call/libc
			astn argv = ast->rhso;
			astn func = ast->lhso;
			dieif(!ast->type);

			while (argv && argv->kind == OPER_com) {
				astn arg = argv->rhso;
				if (!cgen(s, arg, arg->cast)) {
					debug("push(arg, %+k)", arg);
					return 0;
				}
				argv = argv->lhso;
			}
			if (!func || istype(func)) {			// cast()
				if (!cgen(s, ast->rhso, castId(ast->type))) {
					fatal(s, "cgen(%k)", ast);
					return 0;
				}
				if (!argv || argv != ast->rhso) {
					fatal(s, "multiple or no args '%k'", ast);
					return 0;
				}
				ret = castId(ast->type);
				//~ debug("cast(%+k): %T(%s)", ast->rhso, ast->type, tok_tbl[ast->cast].name);
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
					if (emitint(code, opc->offs, opc->init ? opc->init->cint : 0) <= 0)
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
				//~ debug("call(%+k): %s", ast->rhso, tok_tbl[ast->cast].name);
				//~ debug("call(%+k): %T", ast->rhso, ast->type);
				if (argv && !cgen(s, argv, argv->cast)) {
					fatal(s, "push(arg, %+k)", argv->rhso);
					return 0;
				}
				if (!(ret = cgen(s, ast->lhso, 0))) {
					fatal(s, "cgen:call(%+k)", ast->lhso);
					return 0;
				}
			}
		} break;
		case OPER_idx: {		// '[]'
			debug("TODO(cgen(s, ast->lhso, TYPE_ref))");
			if (cgen(s, ast->lhso, TYPE_u32) <= 0) {
				debug("cgen(lhs, %+k)", ast->lhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (emit(code, opc_ldc4, ast->type->size) <= 0) {
				return 0;
			}
			if (cgen(s, ast->rhso, TYPE_u32) <= 0) {
				debug("cgen(rhs, %+k)", ast->rhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (emit(code, u32_mad) <= 0) return 0;
			//~ if (emit(code, i32_mul) <= 0) return 0;
			//~ if (emit(code, i32_add) <= 0) return 0;

			/*if (!ast->type->init) {		// indirect: []
				emit(code, opc_ldi4);
			}// */
			if ((ret = get) != TYPE_ref) {
				ret = castId(ast->type);
				//~ if (emit(code, opc_ldi, ast->type->size) <= 0) {
					//~ return 0;
				//~ }
			}
		} break; // */
		case OPER_dot: {
			debug("TODO(%+k)", ast);
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
			dieif(!ast->type);
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
			if (emit(code, opc, ret = get) <= 0) {
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
		case OPER_div:	// '/'
		case OPER_mod: {		// '%'
			int opc = -1;
			dieif(!ast->type);
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
			if (emit(code, opc, ast->cast) <= 0) {
				debug("emit(%02x, %+k, 0x%02x)", opc, ast, ast->cast);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			switch (ast->kind) {
				case OPER_neq:
				case OPER_leq:
				case OPER_geq: {
					if (emit(code, b32_not, ast->cast) <= 0) {
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

			if (!ast->type)
				fatal(s, "%T, %T", ast->lhso->type, ast->rhso->type);
			dieif(!ast->type);

			if (cgen(s, ast->rhso, ret) <= 0) {
				debug("cgen(%+k, %s)", ast->rhso, tok_tbl[ret].name);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (var && var->onst) {
				int opcSet, opcDup;
				switch (ret) {
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_f32: opcSet = opc_set1, opcDup = opc_dup1; break;
					case TYPE_i64:
					case TYPE_f64: opcSet = opc_set2, opcDup = opc_dup2; break;
					//~ case TYPE_pf2:
					//~ case TYPE_pf4: opcSet = opc_set4, opcDup = opc_dup4; break;
					default: debug("error (%+k): %04x: %s", ast, ret, tok_tbl[ret].name); return 0;
				}
				if (get != TYPE_vid) {
					if (emit(code, opcDup, 0) <= 0) {
						debug("emit(%+k)", ast);
						return 0;
					}
				}
				if (emitidx(code, opcSet, var->offs) <= 0) {
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
					default: debug("error (%+k): %04x: %s", ast, ret, tok_tbl[ret].name); return 0;
				}
				if (get != TYPE_vid) {
					if (!emit(code, opcDup, ret)) {
						debug("emit(%+k)", ast);
						return 0;
					}
				}
				if (!emit(code, opcSet, ret)) {
					return 0;
				}
				// */
				fatal(s, "cgen('%k', `%+k`):%T", ast, ast->rhso, ast->type);
				return 0;
			}

		} break;
		//}
		//{ TVAL
		case CNST_int: switch (get) {
			default:
				debug("invalid cast: [%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
				//~ debug("cast(CNST_int to %s)", tok_tbl[ast->Cast].name);
				return 0;
			case TYPE_vid: return TYPE_vid;

			//~ case TYPE_bit:
			case TYPE_u32: emiti32(code, ast->cint); return TYPE_u32;
			//~ case TYPE_any:
			case TYPE_i32: emiti32(code, ast->cint); return TYPE_i32;
			case TYPE_i64: emiti64(code, ast->cint); return TYPE_i64;
			case TYPE_f32: emitf32(code, ast->cint); return TYPE_f32;
			case TYPE_f64: emitf64(code, ast->cint); return TYPE_f64;
		} return 0;
		case CNST_flt: switch (get) {
			default:
				debug("cgen: invalid cast: [%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
				//~ debug("cast(CNST_int to %s)", tok_tbl[ast->Cast].name);
				return 0;
			case TYPE_vid: return TYPE_vid;

			//~ case TYPE_bit:
			case TYPE_u32: emiti32(code, ast->cflt); return TYPE_u32;
			case TYPE_i32: emiti32(code, ast->cflt); return TYPE_i32;
			case TYPE_i64: emiti64(code, ast->cflt); return TYPE_i64;
			case TYPE_f32: emitf32(code, ast->cflt); return TYPE_f32;
			//~ case TYPE_any:
			case TYPE_f64: emitf64(code, ast->cflt); return TYPE_f64;
		} return 0;

		case TYPE_val: {
			struct astn tmp;
			eval(&tmp, ast->stmt, 0);
			return cgen(s, &tmp, get);
		}
		//~ case TYPE_new: 

		case TYPE_def: {
			if (islval(ast)) {	// TODO: TYPE_new ...
				symn typ = ast->type;		// type
				symn var = ast->link;		// link
				astn val = var->init;		// init
				//~ debug("cgen(var): %+k:%T", ast, ast->type);
				switch (ret = castId(typ)) {
					case TYPE_u32:
					case TYPE_int:
					case TYPE_i32:
					case TYPE_f32: {
						var->onst = 1;// s->level > 1;
						if (val) cgen(s, val, ret);
						else emit(code, opc_ldz1);
						var->offs = emit(code, get_sp);
					} break;
					case TYPE_f64: 
					case TYPE_i64: {
						var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(code, opc_ldz2);
						var->offs = emit(code, get_sp);
					} break;
					case TYPE_p4x: {
						var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(code, opc_ldz2);
						var->offs = emit(code, get_sp);
					} break;
					/*
					//~ case TYPE_ptr:
					case TYPE_arr: {	// malloc();
						//~ debug("unimpl ptr or arr %+k %+T", ast, typ);
						emit(s, opc_ldcr, argi32(0xff00ff));
						//~ var->onst = 1;
						//~ if (val) cgen(s, val, typ->kind);
						//~ else emit(s, opc_ldz1, noarg);
						//~ var->offs = code->rets;
					} break;
					//~ case TYPE_rec:
					//~ case TYPE_enu:	// malloc(); || salloc();
					//~ */
					case TYPE_rec: {
						if (typ->size < 128) {
							emit(code, opc_loc, typ->size);
							var->onst = 1;
							var->offs = emit(code, get_sp);
						}
						//~ if (val) cgen(s, val, ret);
						//~ else emit(s, opc_ldz2);
					} break;
					default:
						debug("unimpl %k:%T(%s)", ast, typ, tok_tbl[ret].name);
						return 0;
				}
				return ret;
			}
			else {				// TODO: TYPE_def ...
				//~ debug("define %k", ast);
				return 0;
			}
		} break;// */
		case TYPE_ref: {		// (var, func)
			symn typ = ast->type;		// type
			symn var = ast->link;		// link

			dieif(!typ || !var);
			dieif(!var->offs);
			if (var->libc) {	// libc
				//~ debug("libc(%k): %+T", ast, ast->link);
				ret = castId(typ);
				emit(code, opc_libc, var->offs);
			}
			else if (var->call) {	// call
				debug("call(%k): %+T", ast, ast->link);
				return 0;
			}
			else if (var->onst) {	// on stack
				switch (ret = castId(typ)) {
					default: debug("error %04x: %s", ret, tok_tbl[ret].name); break;
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_f32: emitidx(code, opc_dup1, var->offs); break;
					case TYPE_i64:
					case TYPE_f64: emitidx(code, opc_dup2, var->offs); break;
				}
				//~ fatal(s, "unimpl '%+k'(%T)", ast, var);
				//~ return 0;
			}
			else /* if (var) */ {	// in memory
				fatal(s, "unimpl '%+k'(%T)", ast, var);
				return 0;
			}
		} break;
		//}
		default:
			fatal(s, "Node(%k)", ast);
			return 0;
	}

	//~ if (get && get != ret) debug("ctyp(`%+k`, %s):%s", ast, tok_tbl[get].name, tok_tbl[ret].name);
	if (get && get != ret) switch (get) {
		// cast
		case TYPE_u32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_i32: break;
			case TYPE_i64: emit(code, i64_i32); break;
			case TYPE_f32: emit(code, f32_i32); break;
			case TYPE_f64: emit(code, f64_i32); break;
			default: goto errorcast;
		} break;
		case TYPE_i32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64: emit(code, i64_i32); break;
			case TYPE_f32: emit(code, f32_i32); break;
			case TYPE_f64: emit(code, f64_i32); break;
			default: goto errorcast;
		} break;
		case TYPE_i64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(code, i32_i64); break;
			case TYPE_f32: emit(code, f32_i64); break;
			case TYPE_f64: emit(code, f64_i64); break;
			default: goto errorcast;
		} break;
		case TYPE_f32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(code, i32_f32); break;
			case TYPE_i64: emit(code, i64_f32); break;
			case TYPE_f64: emit(code, f64_f32); break;
			default: goto errorcast;
		} break;
		case TYPE_f64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(code, i32_f64); break;
			case TYPE_i64: emit(code, i64_f64); break;
			case TYPE_f32: emit(code, f32_f64); break;
			default: goto errorcast;
		} break;

		case TYPE_vid: return TYPE_vid;	// to nothing
		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32: /*emit(s, i32_bol);*/ break;
			case TYPE_i32: /*emit(s, i32_bol);*/ break;
			case TYPE_i64: emit(code, i64_bol); break;
			case TYPE_f32: emit(code, f32_bol); break;
			case TYPE_f64: emit(code, f64_bol); break;
			default: goto errorcast;
		} break;
		//~ case TYPE_ref: 				// address of
		default: fatal(s, "cgen(`%+k`, %s):%s", ast, tok_tbl[get].name, tok_tbl[ret].name);
		errorcast: debug("invalid cast: [%d->%d](%?s to %?s) %k: '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast, ast);
			return 0;
	}

	return ret;
}

//{ core.c ---------------------------------------------------------------------
int srctext(state s, char *file, int line, char *text) {
	if (source(s, source_buff, text))
		return -1;
	s->file = file;
	s->line = line;
	return 0;
}
int srcfile(state s, char *file) {
	if (source(s, source_file, file))
		return -1;
	return 0;
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

int compile(state s, int mode) {
	if (s->errc) return -1;
	s->root = scan(s, mode);
	return s->errc;
}
int gencode(state s, int level) {
//!TODO level
	int memmax = 2 << 20;
	if (s->errc) return -1;
	//~ memmax = bufmaxlen - (s->buffp - s->buffer);
	s->code = vmGetEnv(s, memmax, 0);

	/* emit data seg
	for (sym = s->glob; sym; sym = sym->next) {
		if (sym->kind != TYPE_ref) continue;
		if (sym->init) {
			mem[code->ds] = sym->init;
		}
		code->ds += data->size;
		// TODO initialize
	}// */

	/* emit code seg
	for (ast = s->root; ast; ast = ast->next) {
		if (isfundecl(ast)) {
			symn fun = linkOf(ast);
			fun->offs = emit(s->code, get_ip);
			cgen(ast->stmt);
		}
	}
	for (ast = s->root; ast; ast = ast->next) {
		if ()
		cgen(ast);
	}// */
	// emit(s, call, ".main()");
	// emit(s, call, ".exit()");

/* emit type inf (debug)
	for (data = s->glob; data; data = data->next) {
		if (data->kind != TYPE_def) continue;
		emit_typ(data);
		// TODO initialize
	}// */

	cgen(s, s->root, 0);	// keep locals on stack
	emit(s->code, opc_sysc, 0);
	return s->errc;
}

void* cc_malloc(state s, unsigned size) {//unsigned allign
	void* ret = s->buffp;
	s->buffp += size;
	return ret;
}
void* cc_calloc(state s, unsigned size) {//unsigned allign
	void* ret = s->buffp;
	memset(ret, 0, size);
	s->buffp += size;
	return ret;
}
symn install3(state s, int kind, const char* name, unsigned size, symn typ, astn val) {
	symn def = install(s, kind, name, size);
	if (def) {
		def->type = typ;
		def->init = val;
	}
	return def;
}

void instint(state s, symn it, int64t sgn) {
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
	it->args = leave(s, it);
}//~ */

symn type_f32x4 = NULL;
symn type_f64x2 = NULL;
//~ symn type_i8x16 = NULL;
//~ symn type_i16x8 = NULL;
//~ symn type_i32x4 = NULL;
//~ symn type_i64x2 = NULL;
//~ symn type_u8x16 = NULL;
//~ symn type_u16x8 = NULL;
//~ symn type_u32x4 = NULL;
//~ symn type_u64x2 = NULL;

int cc_init(state s) {
	int i, TYPE_opc = EMIT_opc;
	symn typ, def;
	symn type_i08 = 0, type_i16 = 0;
	symn type_u08 = 0, type_u16 = 0;
	//~ symn type_v4f = 0, type_v2d = 0;
	//~ symn type_u64 = 0, type_f16 = 0;

	s->logf = 0; s->errc = 0;
	s->warn = wl; s->copt = ol;

	s->file = 0; s->line = s->nest = 0;

	s->_fin = s->_chr = -1;
	s->_ptr = 0; s->_cnt = 0;

	s->root = 0;
	s->defs = 0;
	s->code = 0;

	s->tokp = 0;
	s->_tok = 0;
	s->all = 0;

	s->buffp = s->buffer;
	s->deft = (symt)cc_calloc(s, TBLS * sizeof(symn));
	s->strt = (strt)cc_calloc(s, TBLS * sizeof(list));

	//{ install Type
	//+ type_bit = install(s, "bit",  -1, TYPE_bit);
	//+ -> bitfields <=> array of bits ???

	type_vid = install(s, TYPE_vid, "void",  0);

	type_bol = install(s, TYPE_bit, "bool",  1);

	type_u08 = install(s, TYPE_bit, "uns8", 1);
	type_u16 = install(s, TYPE_bit, "uns16", 2);
	type_u32 = install(s, TYPE_bit, "uns32", 4);
	//~ type_u64 = install(s, TYPE_uns, "uns64", 8);

	type_i08 = install(s, TYPE_int, "int8", 1);
	type_i16 = install(s, TYPE_int, "int16", 2);
	type_i32 = install(s, TYPE_int, "int32", 4);
	type_i64 = install(s, TYPE_int, "int64", 8);

	//~ type_f16 = install(s, TYPE_flt, "flt16", 2);
	type_f32 = install(s, TYPE_flt, "flt32", 4);
	type_f64 = install(s, TYPE_flt, "flt64", 8);

	//~ type = install(s, TYPE_flt, "i8x16", 8);
	//~ type = install(s, TYPE_flt, "i16x8", 8);
	//~ type = install(s, TYPE_flt, "i32x4", 8);
	//~ type = install(s, TYPE_flt, "i64x2", 8);
	//~ type = install(s, TYPE_flt, "u8x16", 8);
	//~ type = install(s, TYPE_flt, "u16x8", 8);
	//~ type = install(s, TYPE_flt, "u32x4", 8);
	//~ type = install(s, TYPE_flt, "u64x2", 8);
	//~ type = install(s, TYPE_flt, "f16x8", 8);
	type_f32x4 = install(s, TYPE_p4x, "f32x4", 16);
	type_f64x2 = install(s, TYPE_p4x, "f64x2", 16);

	//~ type_arr = install(s, TYPE_ptr, "array", 0);
	//~ type_str = install(s, TYPE_ptr, "string", 0);

	instint(s, type_i08, -1); instint(s, type_u08, 0);
	instint(s, type_i16, -1); instint(s, type_u16, 0);
	instint(s, type_i32, -1); instint(s, type_u32, 0);
	instint(s, type_i64, -1);// instint(s, type_u64, 0);

	install(s, TYPE_def, "int", 0)->type = type_i32;
	install(s, TYPE_def, "long", 0)->type = type_i64;
	install(s, TYPE_def, "float", 0)->type = type_f32;
	install(s, TYPE_def, "double", 0)->type = type_f64;

	install(s, CNST_int, "true", 0)->init = intnode(s, 1ULL);
	install(s, CNST_int, "false", 0)->init = intnode(s, 0ULL);

	//} */// types
	//{ install Emit
	emit_opc = install(s, TYPE_ref, "emit", -1);
	//~ emit_opc->call = 1;
	//~ emit_opc->type = 0;
	enter(s, emit_opc);

	install(s, TYPE_opc, "nop", 0)->type = type_vid;

	enter(s, typ = install(s, TYPE_i32, "i32", 4));
	install(s, TYPE_opc, "neg", i32_neg)->type = type_i32;
	install(s, TYPE_opc, "add", i32_add)->type = type_i32;
	install(s, TYPE_opc, "sub", i32_sub)->type = type_i32;
	install(s, TYPE_opc, "mul", i32_mul)->type = type_i32;
	install(s, TYPE_opc, "div", i32_div)->type = type_i32;
	install(s, TYPE_opc, "mod", i32_mod)->type = type_i32;
	typ->args = leave(s, typ);

	enter(s, typ = install(s, TYPE_i64, "i64", 8));
	install(s, TYPE_opc, "neg", i64_neg)->type = type_i64;
	install(s, TYPE_opc, "add", i64_add)->type = type_i64;
	install(s, TYPE_opc, "sub", i64_sub)->type = type_i64;
	install(s, TYPE_opc, "mul", i64_mul)->type = type_i64;
	install(s, TYPE_opc, "div", i64_div)->type = type_i64;
	install(s, TYPE_opc, "mod", i64_mod)->type = type_i64;
	typ->args = leave(s, typ);

	enter(s, typ = install(s, TYPE_f32, "f32", 4));
	install(s, TYPE_opc, "neg", f32_neg)->type = type_f32;
	install(s, TYPE_opc, "add", f32_add)->type = type_f32;
	install(s, TYPE_opc, "sub", f32_sub)->type = type_f32;
	install(s, TYPE_opc, "mul", f32_mul)->type = type_f32;
	install(s, TYPE_opc, "div", f32_div)->type = type_f32;
	install(s, TYPE_opc, "mod", f32_mod)->type = type_f32;
	typ->args = leave(s, typ);

	enter(s, typ = install(s, TYPE_f64, "f64", 8));
	install(s, TYPE_opc, "neg", f64_neg)->type = type_f64;
	install(s, TYPE_opc, "add", f64_add)->type = type_f64;
	install(s, TYPE_opc, "sub", f64_sub)->type = type_f64;
	install(s, TYPE_opc, "mul", f64_mul)->type = type_f64;
	install(s, TYPE_opc, "div", f64_div)->type = type_f64;
	install(s, TYPE_opc, "mod", f64_mod)->type = type_f64;
	typ->args = leave(s, typ);

	enter(s, typ = install(s, TYPE_p4x, "v4f", 16));
	install(s, TYPE_opc, "neg", v4f_neg)->type = type_f32x4;
	install(s, TYPE_opc, "add", v4f_add)->type = type_f32x4;
	install(s, TYPE_opc, "sub", v4f_sub)->type = type_f32x4;
	install(s, TYPE_opc, "mul", v4f_mul)->type = type_f32x4;
	install(s, TYPE_opc, "div", v4f_div)->type = type_f32x4;
	install(s, TYPE_opc, "dp3", v4f_dp3)->type = type_f32;
	install(s, TYPE_opc, "dp4", v4f_dp4)->type = type_f32;
	install(s, TYPE_opc, "dph", v4f_dph)->type = type_f32;
	typ->args = leave(s, typ);

	enter(s, typ = install(s, TYPE_p4x, "v2d", 16));
	install(s, TYPE_opc, "neg", v2d_neg)->type = type_f64x2;
	install(s, TYPE_opc, "add", v2d_add)->type = type_f64x2;
	install(s, TYPE_opc, "sub", v2d_sub)->type = type_f64x2;
	install(s, TYPE_opc, "mul", v2d_mul)->type = type_f64x2;
	install(s, TYPE_opc, "div", v2d_div)->type = type_f64x2;
	typ->args = leave(s, typ);

	enter(s, typ = install(s, TYPE_p4x, "swz", 0x96));
	for (i = 0; i < 256; i += 1) {
		symn swz;
		s->buffp[0] = "xyzw"[i>>0&3];
		s->buffp[1] = "xyzw"[i>>2&3];
		s->buffp[2] = "xyzw"[i>>4&3];
		s->buffp[3] = "xyzw"[i>>6&3];
		s->buffp[4] = 0;
		swz = install(s, TYPE_opc, mapstr(s, s->buffp, 4, -1), typ->offs);
		swz->init = intnode(s, i);
		swz->type = typ;
	}
	typ->args = leave(s, typ);
	//~ install(s, EMIT_opc, "swz", 0x96)->args = swz;
	//~ install(s, TYPE_enu, "dup", 0x97)->args = swz;
	//~ install(s, TYPE_enu, "set", 0x98)->args = swz;

	emit_opc->args = leave(s, emit_opc);

	//} */
	//{ install Math
	enter(s, def = install(s, TYPE_enu, "math", 0));

	install(s, CNST_flt, "nan", 0)->init = fh8node(s, 0x7FFFFFFFFFFFFFFFLL);	// Qnan
	install(s, CNST_flt, "Snan", 0)->init = fh8node(s, 0xfff8000000000000LL);	// Snan
	install(s, CNST_flt, "inf", 0)->init = fh8node(s, 0x7ff0000000000000LL);
	install(s, CNST_flt, "l2e", 0)->init = fh8node(s, 0x3FF71547652B82FELL);	// log_2(e)
	install(s, CNST_flt, "l2t", 0)->init = fh8node(s, 0x400A934F0979A371LL);	// log_2(10)
	install(s, CNST_flt, "lg2", 0)->init = fh8node(s, 0x3FD34413509F79FFLL);	// log_10(2)
	install(s, CNST_flt, "ln2", 0)->init = fh8node(s, 0x3FE62E42FEFA39EFLL);	// log_e(2)
	install(s, CNST_flt, "pi", 0)->init = fh8node(s, 0x400921fb54442d18LL);		// 3.1415...
	install(s, CNST_flt, "e", 0)->init = fh8node(s, 0x4005bf0a8b145769LL);		// 2.7182...
	def->args = leave(s, def);
	//} */
	//{ install Libc
	//~ enter(s, def = install(s, TYPE_enu, "Libc", 0));
	installlibc(s);
	//~ def->args = leave(s);
	//} */

	/*{ install ccon
	enter(s, typ = install(s, TYPE_enu, "ccon", 0));
	install(s, CNST_int, "version", 0)->init = intnode(s, 0x20090400);
	install(s, CNST_str, "host", 0)->init = strnode(s, (char*)os);
	//? install(s, TYPE_def, "emit", 0);// warn(int, void ...);
	//? install(s, TYPE_def, "warn", 0);// warn(int, string);

	//- install(s, CNST_str, "type", 0);// current type;
	//~ install(s, CNST_str, "date", 0)->init = &s->ccd;
	//~ install(s, CNST_str, "file", 0)->init = &s->ccfn;
	//~ install(s, CNST_str, "line", 0)->init = &s->ccfl;
	//~ install(s, CNST_str, "time", 0)->init = &s->ccft;
	//~ install(s, CNST_int, "bits", 32)->init = intnode(s, 32);
	typ->args = leave(s, 3);

	//} */
	/*{ install operators
	install(s, -1, "int32 add(int32 a, uns32 b) {return emit(add.i32, i32(a), i32(b));}", 0);
	install(s, -1, "int64 add(int32 a, uns64 b) {return emit(add.i64, i64(a), i64(b));}", 0);
	install(s, -1, "int32 add(int32 a, int32 b) {return emit(add.i32, i32(a), i32(b));}", 0);
	install(s, -1, "int64 add(int32 a, int64 b) {return emit(add.i64, i64(a), i64(b));}", 0);
	install(s, -1, "flt32 add(int32 a, flt32 b) {return emit(add.f32, f32(a), f32(b));}", 0);
	install(s, -1, "flt64 add(int32 a, flt64 b) {return emit(add.f64, f64(a), f64(b));}", 0);

	install(s, -1, "int32 add(out int32 a, uns32 b) {return a = add(a, b)}", 0);
	install(s, -1, "int64 add(out int32 a, uns64 b) {return a = add(a, b)}", 0);
	install(s, -1, "int32 add(out int32 a, int32 b) {return a = add(a, b)}", 0);
	install(s, -1, "int64 add(out int32 a, int64 b) {return a = add(a, b)}", 0);
	install(s, -1, "flt32 add(out int32 a, flt32 b) {return a = add(a, b)}", 0);
	install(s, -1, "flt64 add(out int32 a, flt64 b) {return a = add(a, b)}", 0);

	install(s, -1, "int32 add(int32 a, uns32 b) {return emit(i32.add, i32(a), i32(b));}", 0);	// +
	install(s, -1, "int32 sub(int32 a, uns32 b) {return emit(i32.sub, i32(a), i32(b));}", 0);	// -
	install(s, -1, "int32 mul(int32 a, uns32 b) {return emit(i32.mul, i32(a), i32(b));}", 0);	// *
	install(s, -1, "int32 div(int32 a, uns32 b) {return emit(i32.div, i32(a), i32(b));}", 0);	// /
	install(s, -1, "int32 mod(int32 a, uns32 b) {return emit(i32.mod, i32(a), i32(b));}", 0);	// %

	install(s, -1, "int32 ceq(int32 a, uns32 b) {return emit(b32.ceq, i32(a), i32(b));}", 0);	// ==
	install(s, -1, "int32 cne(int32 a, uns32 b) {return emit(b32.cne, i32(a), i32(b));}", 0);	// !=
	install(s, -1, "int32 clt(int32 a, uns32 b) {return emit(i32.clt, i32(a), i32(b));}", 0);	// <
	install(s, -1, "int32 cle(int32 a, uns32 b) {return emit(i32.cle, i32(a), i32(b));}", 0);	// <=
	install(s, -1, "int32 cgt(int32 a, uns32 b) {return emit(i32.cgt, i32(a), i32(b));}", 0);	// >
	install(s, -1, "int32 cge(int32 a, uns32 b) {return emit(i32.cge, i32(a), i32(b));}", 0);	// >=

	install(s, -1, "int32 shr(int32 a, uns32 b) {return emit(b32.shl, i32(a), i32(b));}", 0);	// >>
	install(s, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.sar, i32(a), i32(b));}", 0);	// <<
	install(s, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.and, i32(a), i32(b));}", 0);	// &
	install(s, -1, "int32 shl(int32 a, uns32 b) {return emit(b32. or, i32(a), i32(b));}", 0);	// |
	install(s, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.xor, i32(a), i32(b));}", 0);	// ^

	install(s, -1, "int32 pls(int32 a) asm {return emit(void, i32(a));}", 0);	// +
	install(s, -1, "int32 neg(int32 a) asm {return emit(neg.i32, i32(a));}", 0);	// -
	install(s, -1, "int32 cmt(int32 a) asm {return emit(cmt.b32, i32(a));}", 0);	// ~
	install(s, -1, "int32 not(int32 a) asm {return emit(b32.not, i32(a));}", 0);	// !

	//} */
	return 0;
}

int cc_done(state s) {
	if (s->nest)
		error(s, s->line, "premature end of file");
	s->nest = 0;
	source(s, 0, 0);
	return s->errc;
}

//}
//{ misc.c ---------------------------------------------------------------------
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
#if 0
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
unsigned i32CmpF(int32t lhs, int32t rhs) {
	int32t res = lhs - rhs;
	int sf = res < 0;
	int zf = res == 0;
	int cf = (unsigned)res < (unsigned)lhs;
	int of = ((rhs ^ lhs) >> 31) != (res >> 31);
	return (((((of << 1) | sf) << 1) | zf) << 1) | cf;
}
unsigned f32CmpF(flt32t lhs, flt32t rhs) {
	flt32t res = lhs - rhs;
	int sf = res < 0;
	int zf = res == 0;
	int cf = 0;
	//~ int of = ((rhs ^ lhs) >> 31) != (res >> 31);
	//~ OF = NAN || INF || -INF;
	int of = res != res || res == res + 1 || res == res - 1;
	return (((((of << 1) | sf) << 1) | zf) << 1) | cf;
}
#endif

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

int evalexp(state s, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	source(s, source_buff, text);
	ast = expr(s, 0);
	typ = lookup(s, 0, ast);
	tid = eval(&res, ast, 0);

	//~ dump(s, stdout, dump_ast);
	if (!ast || !typ || !tid || peek(s)) {
		fputfmt(stdout, "ERROR\n", ast, &res);
		//~ return -2;
	}

	fputfmt(stdout, "eval(`%+k`) = %T(%k)\n", ast, typ, &res, tok_tbl[tid].name);

	return 0;
}

struct state_t env;

int dbgCon(vmEnv env, cell pu, int n, void *ip, unsigned ss) {
	char cmd[1024];
	int stki, stkn = (pu[n].bp + ss - pu[n].sp);

	fputfmt(stdout, ">exec:pu%02d:[ss:%03d]: %A\n", n, stkn/4, ip);
	//~ fputfmt(stdout, ">exec:pu%02d:[ss:%03d, zf:%d sf:%d cf:%d of:%d]: %A", pn, stkn/4, pu->zf, pu->sf, pu->cf, pu->of, ip);

	for ( ; ; ) {

		if (fgets(cmd, 1024, stdin) == NULL) {
			//~ stkval* sp = (stkval*)pu->sp;
			//~ fputfmt(stdout, "sp: {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)} @%x", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y, sp);
			//~ fputfmt(stdout, "\nsp: @%x", sp);
			//~ fputs("\n", stdout);
			return 0;
		}

		// c: cont
		// n: next
		// s: step
		// q: quit

		if (*cmd == 'c') {
			return 'c';
		}

		if (*cmd == 'n') {
			return 's';
		}

		// quit
		if (*cmd == 'q') {
			return 'q';
		}

		// sp(0)
		if (*cmd == '\n') {
			stkval* sp = (stkval*)pu[n].sp;
			//~ fputfmt(stdout, ">exec:pu%02d:[ss:%03d, zf:%d sf:%d cf:%d of:%d]: %A\n", pn, stkn/4, pu->zf, pu->sf, pu->cf, pu->of, ip);
			fputfmt(stdout, "\tsp: {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)}", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
		}

		if (*cmd == 's') {
			stkval* sp = (stkval*)pu->sp;
			fputfmt(stdout, "\tsp: {int32t(%d), flt32(%g), int64t(%D), flt64(%G)}\n", sp->i4, sp->f4, sp->i8, sp->f8);
			fputfmt(stdout, "\tsp: {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)}\n", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
		}

		// stack
		if (*cmd == 'S') {
			for (stki = 0; stki < stkn; stki += 4) {
				stkval* stkv = (stkval*)(pu->sp + stki);
				fputfmt(stdout, "\tsp(%03d): {int32t(%d), flt32(%g), int64t(%D), flt64(%G)}\n", stki/4, stkv->i4, stkv->f4, stkv->i8, stkv->f8);
				//~ fputfmt(stdout, "\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
				//~ fputfmt(stdout, "!\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8);
				//~ sp = (stkval*)(((char*)sp) + 4);
			}
		}
	}
	return 0;
}

int dbgFile(state s, char* src) {

	int i, ret;
	time_t time;

	cc_init(s);

	if ((ret = srcfile(s, src)) != 0) {
		printf("file open error(%d): %s\n", ret, src);
		return ret;
	}

	time = clock();
	ret = compile(s, 0);
	time = clock() - time;
	printf(">scan: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);
	i = s->buffp - s->buffer;
	printf(" size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);

	time = clock();
	ret = gencode(s, 0);
	time = clock() - time;
	printf(">cgen: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);
	//~ i = s->code->cs; printf(" code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, s->code->ic);
	//~ i = s->code->sm; printf(" max stack: %dM, %dK, %dB, %d slots\n", i*4 >> 20, i*4 >> 10, i*4, s->code->sm);
	//~ i = s->code->ds; printf(" data size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);

	printf(">tags:file\n"); dump(s, stdout, dump_sym);
	//~ printf(">tags:lstd\n"); dumpsym(stdout, leave(s));
	//~ printf(">tags:emit\n"); dumpsym(stdout, emit_opc);
	/*
	{
		symn sym = s->all;
		for (sym = s->all; sym; sym = sym->all) {
			//~ fputfmt(stdout, "%s:%d:>%+T\n", sym->file, sym->line, sym);
			fputc('>', stdout);
			dumpsym(stdout, sym, 0);
		}
	}// */

	printf(">code:ast\n"); dump(s, stdout, dump_ast);
	printf(">code:asm\n"); dumpasm(stdout, s->code, 0);
	//~ printf(">code:xml\n"); dump(s, stdout, dump_xml);

	time = clock();
	ret = exec(s->code, cc, ss, 0);
	time = clock() - time;
	printf(">exec: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);

	return ret;
}// */

int asmHelp(char *cmd) {
	FILE *out = stdout;
	int i, k, n = 0;
	//~ debug("help : %s", cmd);
	for (i = 0; i < opc_last; ++i) {
		char *opc = (char*)opc_tbl[i].name;
		if (opc && strfindstr(opc, cmd, 1)) {
			fputfmt(out, "Instruction: %s\n", opc);
			n += 1;
			k = i;
		}
	}
	if (n == 0) {
		fputfmt(out, "No Entry for: '%s'\n", cmd);
	}
	else if (n == 1 && stricmp(cmd, opc_tbl[k].name) == 0) {
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
	return n;
}

int main(int argc, char *argv[]) {
	state s = &env;
	char *prg, *cmd;
	int printinfo = 0;

	//~ /* TODO if debug
	//~ return vm_test();
	if (1 && argc == 1) {
		char *args[] = {
			"psvm",		// program name
			//~ "-emit",
			//~ "-api",
			"-c",		// compile command
			"-x",		// compile command
			"main.cvx",
		};
		argc = sizeof(args) / sizeof(*args);
		argv = args;
		//~ cc_init(s);
		//~ return dbgFile(s, "");
		//~ return dbgFile(s, "test/main.cvx");
	}// */

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	cc_init(s);
	prg = argv[0];
	cmd = argv[1];
	if (argc <= 2) {
		if (strcmp(cmd, "-api") == 0) {
			dumpsym(stdout, leave(s, NULL), 1);
		}
		else if (strcmp(cmd, "-emit") == 0) {
			dumpsym(stdout, emit_opc->args, 1);
		}
		else {
			usage(s, prg, cmd);
		}
		//~ exit(0);
	}
	else if (strcmp(cmd, "-c") == 0) {	// compile
		FILE *fout = stdout;
		int level = -1, argi;
		int optl = ol;
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
			run_bin = 0x0014,	// exec
			dbg_bin = 0x0015,	// exec
		};

		// options
		for (argi = 2; argi < argc; ++argi) {
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
				outc = run_bin;
				if (*str == 'd') {
					str += 1;
					outc = dbg_bin;
				}
				else if (!parseint(arg + 2, &level)) {
					fputfmt(stderr, "invalid level '%c'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-t", 2) == 0) {		// tags
				if (!parseint(arg + 2, &level)) {
					fputfmt(stderr, "invalid level '%c'\n", arg + 2);
					return 0;
				}
				outc = out_tags;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// dasm
				if (!parseint(arg + 2, &level)) {
					fputfmt(stderr, "invalid level '%c'\n", arg + 2);
					return 0;
				}
				outc = out_dasm;
			}
			else if (strncmp(arg, "-c", 2) == 0) {		// tree
				if (!parseint(arg + 2, &level)) {
					fputfmt(stderr, "invalid level '%c'\n", arg + 2);
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
				else if (!parseint(arg + 2, &warn)) {
					fputfmt(stderr, "invalid level '%c'\n", arg + 2);
					return 0;
				}
			}
			/*else if (strncmp(arg, "-d", 2) == 0) {		// optimize/debug level
				return -1;
			}*/

			// execute code
			else {
				fputfmt(stderr, "invalid option '%s' for -compile\n", arg);
				return -1;
			}
		}

		if (!srcf) {
			fputfmt(stderr, "no input file\n");
			usage(s, prg, cmd);
			return -1;
		}

		s->warn = warn;
		/*if (source(s, "lstd.cxx") != 0) {
			fputfmt(stderr, "can not open standard library `%s`\n", "lstd.ccc");
			exit(-1);
		}
		scan(s, Type_def);
		// */

		if (logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (srcfile(s, srcf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", srcf);
			return -1;
		}
		if (compile(s, 0) != 0) {
			return s->errc;
		}
		if (outc & gen_code) {
			if (gencode(s, optl) != 0) {
				return s->errc;
			}
			//~ dump(s, stdout, dump_asm + level);
		}
		// output
		if (outf) {
			fout = fopen(outf, "wb");
			if (fout == NULL) {
				fputfmt(stderr, "can not open file `%s`\n", outf);
				return -1;
			}
		}

		switch (outc) {

			case out_tags: dump(s, fout, dump_sym + level); break;
			case out_tree: dump(s, fout, dump_ast + level); break;
			case out_dasm: dumpasm(fout, s->code, 0); break;

			case run_bin:		// exec
				exec(s->code, cc, ss, NULL);
				break;

			case dbg_bin:		// exec
				exec(s->code, cc, ss, (void*)dbgCon);
				break;
		}

		if (outf) {
			fclose(fout);
		}

		if (!printinfo) {
			int i;
			i = s->buffp - s->buffer; printf("parse mem: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
			if (s->code) {
				//~ i = s->code->sm * 4; printf("stack max: %dM, %dK, %dB, %d slots\n", i >> 20, i >> 20, i, s->code->sm);
				//~ i = s->code->cs; printf("code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, s->code->ic);
				//~ i = s->code->ds; printf("data size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
			}
		}

		return cc_done(s);
	}
	else if (strcmp(cmd, "-e") == 0) {	// execute
		fatal(s, "unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return exec(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-d") == 0) {	// assemble
		fatal(s, "unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return dumpasm(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-m") == 0) {	// make
		fatal(s, "unimplemented option '%s' \n", cmd);
	}
	else if (strcmp(cmd, "-h") == 0) {	// help
		char *t = argv[2];
		if (argc < 3) {
			usage(s, argv[0], argc < 4 ? argv[3] : NULL);
		}
		else if (strcmp(t, "-s") == 0) {
			int i = 3;
			if (argc == 3) {
				vm_hgen();
				//~ asmHelp("?");
			}
			else while (i < argc) {
				asmHelp(argv[i++]);
			}
		}
		else if (strcmp(t, "-x") == 0) {
		}
	}
	else if (argc == 2 && *cmd != '-') {			// try to eval
		return evalexp(s, cmd);
	}
	else fputfmt(stderr, "invalid option '%s'", cmd);
	return 0;
}
