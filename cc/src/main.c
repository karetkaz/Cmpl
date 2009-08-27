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
static const int ss = 2 << 20;		// execution stack(256K)
//~ static const int hs = 128 << 20;		// execution heap(128M)
void dumpxml(FILE *fout, node ast, int lev, const char* text, int level);

#ifdef __WATCOMC__
#pragma disable_message(124);
#endif
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
			{Code, Size + 1, Args, Push, Mnem},
	#include "incl/defs.h"
};

//~ #include "clog.c"
//~ #include "scan.c"
//~ #include "tree.c"
//~ #include "type.c"
//{ #include "code.c"
//~ code.c - Builder & Executor ------------------------------------------------
#include <math.h>
#include "incl/libc.c"

#pragma pack(push, 1)
typedef union {			// stack value type
	int08t	i1;
	uns08t	u1;
	int16t	i2;
	uns16t	u2;
	int32t	i4;
	uns32t	u4;
	int64t	i8;
	uns64t	u8;
	flt32t	f4;
	flt64t	f8;
	f32x4t	pf;
	f64x2t	pd;
	u32x4t	x4;
} stkval;
struct bcdc_t {			// byte code decoder
	uns08t opc;
	union {
		stkval arg;		// argument (load const)
		uns08t idx;		// usualy index for stack
		int32t jmp:24;		// jump relative
		struct {		// when starting a task
			uns08t	dl;	// data to copy to stack
			uns16t	cl;	// code to exec paralel
		};
		/*struct {				// extended
			uint8_t opc:4;		// 0-15
			uint8_t opt:2;		// idx(3) / rev(3) / imm(6) / mem(6)
			uint8_t mem:2;		// mem size
			union {
				uint8_t si[4];
				val32t	arg;
			};
		}ext;*/
	};
};
#pragma pack(pop)

defn emit_opc = 0;

int emit(state s, int opc, ...) {
	bcde ip = 0;
	stkval arg = *(stkval*)(&opc + 1);

	if (s->code.cs >= s->code.mems - 16) {
		error (s, 0, 0, "memory overrun");
		return -1;
	}
	if (s->code.sm < s->code.ss)
		s->code.sm = s->code.ss;

	if (opc == opc_line) {
		static int line = 0;
		if (line != arg.i4) {
			//~ info(s, s->file, arg.i4, "emit(line)");
			line = arg.i4;
		}
		return s->code.cs;
	}

	//~ if (s->code.cs & 1) emit(s, opc_nop, arg);

	/*else if (opc == opc_ldci) {
		if (arg.u8 == 0) opc = opc_ldz;
		else if (arg.u8 <= 0xff) opc = opc_ldc1;
		else if (arg.u8 <= 0xffff) opc = opc_ldc2;
		else if (arg.u8 <= 0xffffffff) opc = opc_ldc4;
		else opc = opc_ldc8;
	}*/
	/*else if (opc == opc_ldi) switch (arg.i4) {
		case 1: opc = opc_ldi1; break;
		case 2: opc = opc_ldi2; break;
		case 4: opc = opc_ldi4; break;
		default: return -1;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case 1: opc = opc_sti1; break;
		case 2: opc = opc_sti2; break;
		case 4: opc = opc_sti4; break;
		default: return -1;
	}*/

	else if (opc == opc_neg) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_neg; break;
		case TYPE_i64: opc = i64_neg; break;
		case TYPE_f32: opc = f32_neg; break;
		case TYPE_f64: opc = f64_neg; break;
		//~ case TYPE_pf2: opc = p4d_neg; break;
		//~ case TYPE_pf4: opc = p4f_neg; break;
		default: return -1;
	}
	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_add; break;
		case TYPE_i64: opc = i64_add; break;
		case TYPE_f32: opc = f32_add; break;
		case TYPE_f64: opc = f64_add; break;
		//~ case TYPE_pf2: opc = p4d_add; break;
		//~ case TYPE_pf4: opc = p4f_add; break;
		default: return -1;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_sub; break;
		case TYPE_i64: opc = i64_sub; break;
		case TYPE_f32: opc = f32_sub; break;
		case TYPE_f64: opc = f64_sub; break;
		//~ case TYPE_pf2: opc = p4d_sub; break;
		//~ case TYPE_pf4: opc = p4f_sub; break;
		default: return -1;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32: opc = u32_mul; break;
		case TYPE_i32: opc = i32_mul; break;
		case TYPE_i64: opc = i64_mul; break;
		case TYPE_f32: opc = f32_mul; break;
		case TYPE_f64: opc = f64_mul; break;
		//~ case TYPE_pf2: opc = p4d_mul; break;
		//~ case TYPE_pf4: opc = p4f_mul; break;
		default: return -1;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32: opc = u32_div; break;
		case TYPE_i32: opc = i32_div; break;
		case TYPE_i64: opc = i64_div; break;
		case TYPE_f32: opc = f32_div; break;
		case TYPE_f64: opc = f64_div; break;
		//~ case TYPE_pf2: opc = p4d_div; break;
		//~ case TYPE_pf4: opc = p4f_div; break;
		default: return -1;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		//~ case TYPE_u32: opc = u32_mod; break;
		case TYPE_i32: opc = i32_mod; break;
		case TYPE_i64: opc = i64_mod; break;
		case TYPE_f32: opc = f32_mod; break;
		case TYPE_f64: opc = f64_mod; break;
		//~ case TYPE_pf2: opc = p4d_mod; break;
		//~ case TYPE_pf4: opc = p4f_mod; break; p4f_crs ???
		default: return -1;
	}

	// cmp
	else if (opc == opc_ceq) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_ceq; break;
		case TYPE_f32: opc = f32_ceq; break;
		case TYPE_i64: opc = i64_ceq; break;
		case TYPE_f64: opc = f64_ceq; break;
		default: return -1;
	}
	else if (opc == opc_clt) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_clt; break;
		case TYPE_f32: opc = f32_clt; break;
		case TYPE_i64: opc = i64_clt; break;
		case TYPE_f64: opc = f64_clt; break;
		default: return -1;
	}
	else if (opc == opc_cgt) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_cgt; break;
		case TYPE_f32: opc = f32_cgt; break;
		case TYPE_i64: opc = i64_cgt; break;
		case TYPE_f64: opc = f64_cgt; break;
		default: return -1;
	}

	// bit
	else if (opc == opc_shl) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_shl; break;
		default: return -1;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_i32: opc = b32_sar; break;
		case TYPE_u32: opc = b32_shr; break;
		default: return -1;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_and; break;
		default: return -1;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_ior; break;
		default: return -1;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_xor; break;
		default: return -1;
	}

	/*~:)) ?? rollback (optimize)
	else if (opc == opc_ldz1) {
		ip = (bcde)&s->code.mem[s->code.pc];
		if (ip->opc == opc_ldz1) {
			opc = opc_ldz2;
			s->code.cs = s->code.pc;
			s->code.sm -= 1;
		}
	}
	else if (opc == opc_ldz2) {
		ip = (bcde)&s->code.mem[s->code.pc];
		if (ip->opc == opc_ldz2) {
			opc = opc_ldz4;
			s->code.cs = s->code.pc;
			s->code.sm -= 2;
		}
	}
	else if (opc == opc_dup1) {			// TODO 
		ip = (bcde)&s->code.mem[s->code.pc];
		if (ip->opc == opc_dup1 && ip->idx == arg.u4) {
			ip->opc = opc_dup2;
			ip->idx -= 1;
			s->code.sm += 1;
			return s->code.pc;
		}
	}*/
	/*else if (opc == opc_dup2) {
		ip = (bcde)&s->code.mem[s->code.pc];
		if (ip->opc == opc_dup2 && ip->idx == arg.u4) {
			ip->opc = opc_dup4;
			ip->idx -= 2;
			s->code.sm += 2;
			return s->code.pc;
		}
	}// */
	/*else if (opc == opc_jnz || opc == opc_jz) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_bit && ip->arg.u1 == 1) {
			opc = (opc == opc_jnz) ? opc_jz : opc_jnz;
			s->code.cs = s->code.pc;
		}
	}// */
	/*else if (opc == opc_shl || opc == opc_shr || opc == opc_sar) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_ldc4) {
			if (opc == opc_shl) arg.i1 = 1 << 5;
			if (opc == opc_shr) arg.i1 = 2 << 5;
			if (opc == opc_sar) arg.i1 = 3 << 5;
			s->code.rets -= 1;
			opc = opc_bit;
			arg.i1 |= ip->arg.u1 & 0x1F;
			s->code.cs = s->code.pc;
		}
	}// */

	if (opc > opc_last) {
		//~ fatal(s, __FILE__, __LINE__, "xxx_opc%x", opc);
		fatal(s, "xxx_opc%x", opc);
		return 0;
	}

	ip = (bcde)&s->code.mem[s->code.cs];
	s->code.ic += s->code.pc != s->code.cs;
	s->code.pc = s->code.cs;

	ip->opc = opc;
	ip->arg = arg;

	switch (opc) {
		error_opc: fatal(s, "invalid opcode: [%02x]", ip->opc); return -1;
		error_stc: fatal(s, "stack underflow: near opcode %A", ip); return -1;
		#define STOP(__ERR) goto __ERR
		#define NEXT(__IP, __SP, __CHK)\
			if(s->code.ss < (__CHK)) STOP(error_stc);\
			s->code.ss += (__SP);\
			s->code.cs += (__IP);
		//~ #define EXEC(__STMT)
		//~ #define MEMP(__MPTR)
		//~ #define CHECK()
		#include "incl/exec.c"
	}
	return s->code.pc;
}

int emitidx(state s, int opc, int arg) {
	stkval tmp;
	tmp.i4 = s->code.ss - arg;
	return emit(s, opc, tmp);
}

int emiti32(state s, int opc, int32t arg) {
	stkval tmp;
	tmp.i4 = arg;
	return emit(s, opc, tmp);
}

int emiti64(state s, int opc, int64t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emit(s, opc, tmp);
}

int emitf32(state s, int opc, flt32t arg) {
	stkval tmp;
	tmp.f4 = arg;
	return emit(s, opc, tmp);
}

int emitf64(state s, int opc, flt64t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emit(s, opc, tmp);
}

int fixjump(state s, int src, int dst, int stc) {
	bcde ip = (bcde)(s->code.mem + src);
	if (src >= 0) switch (ip->opc) {
		case opc_jmp:
		case opc_jnz:
		case opc_jz: ip->jmp = dst - src; break;
		default: debug("expecting jump, but found: '%+A'", ip);
	}
	return 0;
}

int cgen(state s, node ast, int get) {
	struct astn_t tmp;
	int ret = 0;
	const int dxst = 15;	// dump

	if (!ast) return 0;

	//~ debug("cgen:(%+k):%?T", ast, ast->type);

	switch (ast->kind) {
		//{ STMT
		case OPER_nop: {		// expr statement
			int er, beg = s->code.cs;
			dieif(!ast->stmt);					// is cleaned
			emit(s, opc_line, ast->line);
			er = cgen(s, ast->stmt, TYPE_vid);
			#if 0
			void dasm(FILE *, unsigned char *beg, int len, int offs);
			info(s, s->file, ast->line, "cgen(%+k): %s", ast->stmt, tok_tbl[er].name);
			dasm(stdout, s->code.memp + beg, s->code.cs - beg, beg);
			#endif
			beg = s->code.cs;
		} return 0;
		case OPER_beg: {		// {}
			//~ defn typ;
			node ptr;
			int stpos = s->code.ss;
			emiti32(s, opc_line, ast->line);
			for (ptr = ast->stmt; ptr; ptr = ptr->next) {
				if (cgen(s, ptr, TYPE_vid) <= 0){}
			}
			//~ for (typ = ast->type; typ; typ = typ->next)
				//~ if (typ->onst) stc += 1;
			//~ if (stc) emiti32(s, opc_pop, stc);
			if (stpos < s->code.ss && get == TYPE_vid) {
				emiti32(s, opc_pop, s->code.ss - stpos);
			}
			else if (stpos > s->code.ss) {
				fatal(s, "stack underflow in %-k", ast);
				return -1;
			}
		} return 0;
		case OPER_jmp: {		// if ( ) then {} else {}
			int tt = eval(&tmp, ast->test, TYPE_bit);
			emiti32(s, opc_line, ast->line);
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
					jmpt = emiti32(s, opc_jz, 0);
					cgen(s, ast->stmt, TYPE_vid);
					jmpf = emiti32(s, opc_jmp, 0);
					fixjump(s, jmpt, s->code.cs, 0);
					cgen(s, ast->step, TYPE_vid);
					fixjump(s, jmpf, s->code.cs, 0);
				}
				else if (ast->stmt) {					// if then
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast->test);
						return 0;
					}
					//~ if false skip THEN block
					jmpt = emiti32(s, opc_jz, 0);
					cgen(s, ast->stmt, TYPE_vid);
					fixjump(s, jmpt, s->code.cs, 0);
				}
				else if (ast->step) {					// if else
					if (!cgen(s, ast->test, TYPE_bit)) {
						debug("cgen(%+k)", ast);
						return 0;
					}
					//~ if true skip ELSE block
					jmpt = emiti32(s, opc_jnz, 0);
					cgen(s, ast->step, TYPE_vid);
					fixjump(s, jmpt, s->code.cs, 0);
				}
				else {
					//~ cgen(s, ast->test);
					//~ print(s, 8, s->file, ast->line, "");
				}
			}
			else debug("unimplemented: goto / break/ continue;");
		} return 0;
		case OPER_for: {		// for ( ; ; ) {}
			//!TODO: replace 'xxx = s->code.cs' with 'xxx = emit(...)'
			int beg, end, cmp = -1;
			int stpos = s->code.ss;

			emiti32(s, opc_line, ast->line);
			cgen(s, ast->init, TYPE_vid);

			//~ if (ast->cast == QUAL_par) ;		// 'parallel for'
			//~ else if (ast->cast == QUAL_sta) ;	// 'static for' (unroll)
			beg = s->code.cs;
			if (ast->step) {
				int tmp = beg;
				if (ast->init) emiti32(s, opc_jmp, 0);

				beg = s->code.cs;
				cgen(s, ast->step, TYPE_vid);
				if (ast->init) fixjump(s, tmp, s->code.cs, 0);
			}
			if (ast->test) {
				cgen(s, ast->test, TYPE_bit);
				cmp = emiti32(s, opc_jz, 0);		// if (!test) break;
			}

			// push(list_jmp, 0);
			cgen(s, ast->stmt, TYPE_vid);
			end = emiti32(s, opc_jmp, beg);				// continue;
			fixjump(s, end, beg, 0);
			fixjump(s, cmp, s->code.cs, 0);				// break;
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
			if (stpos < s->code.ss) {
				emiti32(s, opc_pop, s->code.ss - stpos);
			}
			else if (stpos > s->code.ss) {
				fatal(s, "stack underflow: near statement %+k", ast);
				return -1;
			}
		} return 0;
		//}
		//{ OPER
		case OPER_fnc: {		// '()' emit/cast/call/libc
			node argv = ast->rhso;
			node func = ast->lhso;
			dieif(!ast->type);

			while (argv && argv->kind == OPER_com) {
				node arg = argv->rhso;
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
				defn opc = linkOf(argv);
				//~ debug("emit(%+k): %T", ast->rhso, ast->type);
				if (opc && opc->kind == EMIT_opc) {
					ret = castId(opc->type);
					if (emiti64(s, opc->offs, opc->init ? opc->init->cint : 0) <= 0)
						debug("opcode expected, not %k", argv);
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
			if (emit(s, opc_ldc4, ast->type->size) <= 0) {
				return 0;
			}
			if (cgen(s, ast->rhso, TYPE_u32) <= 0) {
				debug("cgen(rhs, %+k)", ast->rhso);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			if (emit(s, u32_mad) <= 0) return 0;
			//~ if (emit(s, i32_mul) <= 0) return 0;
			//~ if (emit(s, i32_add) <= 0) return 0;

			/*if (!ast->type->init) {		// indirect: []
				emit(s, opc_ldi4);
			}// */
			if ((ret = get) != TYPE_ref) {
				ret = castId(ast->type);
				//~ if (emit(s, opc_ldi, ast->type->size) <= 0) {
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
		case OPER_cmt: {		// '~'
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
			if (emit(s, opc, ret = get) <= 0) {
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
			if (emit(s, opc, ast->cast) <= 0) {
				debug("emit(%02x, %+k, 0x%02x)", opc, ast, ast->cast);
				dumpxml(stderr, ast, 0, "debug", dxst);
				return 0;
			}
			switch (ast->kind) {
				case OPER_neq:
				case OPER_leq:
				case OPER_geq: {
					if (emit(s, b32_not, ast->cast) <= 0) {
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
			defn var = linkOf(ast->lhso);
			ret = castId(ast->type);

			if (!ast->type)
				fatal(s, "%T, %T", ast->lhso->type, ast->rhso->type);
			dieif(!ast->type);

			if (var && var->onst) {
				if (cgen(s, ast->rhso, ret) <= 0) {
					debug("cgen(%+k, %s)", ast->rhso, tok_tbl[ret].name);
					dumpxml(stderr, ast, 0, "debug", dxst);
					return 0;
				}
				switch (ret) {
					case TYPE_u32:
					case TYPE_i32:
					case TYPE_f32: {
						if (get != TYPE_vid)
							if (emit(s, opc_dup1, 0) <= 0) {
								debug("emit(%+k)", ast);
								return 0;
							}
						if (emitidx(s, opc_set1, var->offs) <= 0) {
							debug("emit(%+k, %d)", ast, var->offs);
							return 0;
						};
					} break;
					case TYPE_i64:
					case TYPE_f64: {
						if (get != TYPE_vid)
							if (emit(s, opc_dup2, 0) <= 0) {
								debug("emit(%+k)", ast);
								return 0;
							}
						if (emitidx(s, opc_set2, var->offs) <= 0) {
							debug("emit(%+k)", ast);
							return 0;
						};
					} break;
					//~ case TYPE_pf2:
					//~ case TYPE_pf4: emit(s, opc_set4, argi32(pos+4)); break;
					default: debug("error (%+k): %04x: %s", ast, ret, tok_tbl[ret].name); return 0;
				}
				//~ debug("unimpl '%+k'(%T)", ast, var);
				//~ return 0;
			}
			else {
				if (cgen(s, ast->rhso, ret) <= 0) {
					debug("cgen(rhs, %+k)", ast->rhso);
					return 0;
				}
				if (cgen(s, ast->lhso, TYPE_ref) <= 0) {
					debug("cgen(lhs, %+k)", ast->lhso);
					return 0;
				}
				/*if (get != TYPE_vid) {
					if (!emit(s, opc_dup, ret)) {
						debug("emit(%+k)", ast);
						return 0;
					}
				}
				if (!emit(s, opc_sti, ret)) {
					return 0;
				}*/
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
			case TYPE_u32: emiti32(s, opc_ldc4, ast->cint); return TYPE_u32;
			//~ case TYPE_any:
			case TYPE_i32: emiti32(s, opc_ldc4, ast->cint); return TYPE_i32;
			case TYPE_i64: emiti64(s, opc_ldc8, ast->cint); return TYPE_i64;
			case TYPE_f32: emitf32(s, opc_ldcf, ast->cint); return TYPE_f32;
			case TYPE_f64: emitf64(s, opc_ldcF, ast->cint); return TYPE_f64;
		} return 0;
		case CNST_flt: switch (get) {
			default:
				debug("cgen: invalid cast: [%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
				//~ debug("cast(CNST_int to %s)", tok_tbl[ast->Cast].name);
				return 0;
			case TYPE_vid: return TYPE_vid;

			//~ case TYPE_bit:
			case TYPE_u32: emiti32(s, opc_ldc4, ast->cflt); return TYPE_u32;
			case TYPE_i32: emiti32(s, opc_ldc4, ast->cflt); return TYPE_i32;
			case TYPE_i64: emiti64(s, opc_ldc8, ast->cflt); return TYPE_i64;
			case TYPE_f32: emitf32(s, opc_ldcf, ast->cflt); return TYPE_f32;
			//~ case TYPE_any:
			case TYPE_f64: emitf64(s, opc_ldcF, ast->cflt); return TYPE_f64;
		} return 0;

		case TYPE_val: {
			struct astn_t tmp;
			eval(&tmp, ast->stmt, 0);
			return cgen(s, &tmp, get);
		}
		//~ case TYPE_new: 

		case TYPE_def: {
			if (islval(ast)) {	// TODO: TYPE_new ...
				defn typ = ast->type;		// type
				defn var = ast->link;		// link
				node val = var->init;		// init
				//~ debug("cgen(var): %+k:%T", ast, ast->type);
				switch (ret = castId(typ)) {
					case TYPE_u32:
					case TYPE_int:
					case TYPE_i32:
					case TYPE_f32: {
						var->onst = 1;// s->level > 1;
						if (val) cgen(s, val, ret);
						else emit(s, opc_ldz1);
						var->offs = s->code.ss;
					} break;
					case TYPE_f64: 
					case TYPE_i64: {
						var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(s, opc_ldz2);
						var->offs = s->code.ss;
					} break;
					/*
					//~ case TYPE_ptr:
					case TYPE_arr: {	// malloc();
						//~ debug("unimpl ptr or arr %+k %+T", ast, typ);
						emit(s, opc_ldcr, argi32(0xff00ff));
						//~ var->onst = 1;
						//~ if (val) cgen(s, val, typ->kind);
						//~ else emit(s, opc_ldz1, noarg);
						//~ var->offs = s->code.rets;
					} break;
					//~ case TYPE_rec:
					//~ case TYPE_enu:	// malloc(); || salloc();
					//~ */
					case TYPE_rec: {
						if (typ->size < 128) {
							emit(s, opc_loc, typ->size);
							var->onst = 1;
						}
						//~ if (val) cgen(s, val, ret);
						//~ else emit(s, opc_ldz2);
						var->offs = s->code.ss;
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
			defn typ = ast->type;		// type
			defn var = ast->link;		// link

			dieif(!typ || !var);
			dieif(!var->offs);
			if (var->libc) {	// libc
				//~ debug("libc(%k): %+T", ast, ast->link);
				ret = castId(typ);
				emit(s, opc_libc, var->offs);
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
					case TYPE_f32: emitidx(s, opc_dup1, var->offs); break;
					case TYPE_i64:
					case TYPE_f64: emitidx(s, opc_dup2, var->offs); break;
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
			case TYPE_i64: emit(s, i64_i32); break;
			case TYPE_f32: emit(s, f32_i32); break;
			case TYPE_f64: emit(s, f64_i32); break;
			default: goto errorcast;
		} break;
		case TYPE_i32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_any:
			case TYPE_u32: break;
			case TYPE_i64: emit(s, i64_i32); break;
			case TYPE_f32: emit(s, f32_i32); break;
			case TYPE_f64: emit(s, f64_i32); break;
			default: goto errorcast;
		} break;
		case TYPE_i64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(s, i32_i64); break;
			case TYPE_f32: emit(s, f32_i64); break;
			case TYPE_f64: emit(s, f64_i64); break;
			default: goto errorcast;
		} break;
		case TYPE_f32: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(s, i32_f32); break;
			case TYPE_i64: emit(s, i64_f32); break;
			case TYPE_f64: emit(s, f64_f32); break;
			default: goto errorcast;
		} break;
		case TYPE_f64: switch (ret) {
			//~ case TYPE_bit:
			case TYPE_u32:
			case TYPE_i32: emit(s, i32_f64); break;
			case TYPE_i64: emit(s, i64_f64); break;
			case TYPE_f32: emit(s, f32_f64); break;
			default: goto errorcast;
		} break;

		case TYPE_vid: return TYPE_vid;	// to nothing
		case TYPE_bit: switch (ret) {		// to boolean
			case TYPE_u32: /*emit(s, i32_bol);*/ break;
			case TYPE_i32: /*emit(s, i32_bol);*/ break;
			case TYPE_i64: emit(s, i64_bol); break;
			case TYPE_f32: emit(s, f32_bol); break;
			case TYPE_f64: emit(s, f64_bol); break;
			default: goto errorcast;
		} break;
		//~ case TYPE_ref: 				// address of
		default: fatal(s, "cgen(`%+k`, %s):%s", ast, tok_tbl[get].name, tok_tbl[ret].name);
		errorcast: debug("invalid cast: [%d->%d](%?s to %?s) %k: '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast, ast);
			return 0;
	}

	return ret;
}

typedef struct vmEnv {		// cgen enviroment
	state env;
	char *mem;
	int ic;
	int ss;
	//~ cell p, pu;		// current processor
	struct {			// bytecode
		unsigned char*	mem;			// memory == buffp
		unsigned long	mems;			// memory size ?

		unsigned int	ms;			// max stack size
		unsigned int	pc;			// prev / program counter
		unsigned int	ic;			// instruction count / executed

		unsigned int	ss;			// stack size / current stack size
		unsigned int	cs;			// code size / program counter
		unsigned int	ds;			// data size

		/** memory mapping
		 *	data(RW):ds		:	initialized data
		 *	code(RX):cs		:	instructions
		 *	heap(RW):		:	
		 *	stack(RW):ss	:	
		 *
		+mp------------+ip------------+hp------------+bp---------------+sp
		|   code[cs]   |   data[ds]   |   heap[??]   |  stack[ss*cc]   |
		+--------------+--------------+--------------+-----------------+
		?      r-x     |      r--     |     rw-      |       rw-       |
		+--------------+--------------+--------------+-----------------+
		enum Segment
		{
			Seg_Data = 0,
			Seg_Init = 1,
			Seg_Code = 1,
		};
		**/
	} code;
}*vmEnv;

/*static cell task(vmEnv ee, int cl, int dl) {
	// find an empty cpu
	cell pu = 0;//pp;
	while (pu && pu->ip)
		pu = pu->next;
	if (pu != NULL) {
		// set ip and copy stack
		pp->cp++;		// workers
		pu->pp = pp;		// parent
		pu->ip = pp->ip;
		pu->sp = pu->bp + ss - cl;
		memcpy(pu->sp, pp->sp, cl * 4);
	}
	return pu;
}// */

/*static int done(vmEnv ee, int cp) {
	pu[pu[cp].pp].cp -= 1;
	sigsnd(pp->pp, SIG_quit);
}// */

static cell getProc(vmEnv ee, cell pu) {return ((pu && pu->ip) ? pu : 0);}

typedef int (*dbgf)(state env, cell pu, int n, bcde ip, unsigned ss);

void vm_tags(state s, cell pu) {
	defn ptr;
	FILE *fout = stdout;
	for (ptr = s->glob; ptr; ptr = ptr->next) {
		if (ptr->kind == TYPE_ref && ptr->onst) {
			int spos = s->code.ss - ptr->offs;
			stkval* sp = (stkval*)(pu->sp + 4 * spos);
			defn typ = ptr->type;
			if (ptr->file && ptr->line)
				fputfmt(fout, "%s:%d:", ptr->file, ptr->line);
			fputfmt(fout, "sp(0x%02x):%s", spos, ptr->name);
			switch(typ ? typ->kind : 0) {
				default:
				TYPE_XXX:
					fputfmt(fout, " = ????\n");
					break;
				case TYPE_bit: switch (typ->size) {
					case 1: case 2:
					case 4: fputfmt(fout, " = u32[%008x](%u)\n", sp->i4, sp->i4); break;
					case 8: fputfmt(fout, " = u64[%016X](%U)\n", sp->i8, sp->i8); break;
					default: goto TYPE_XXX;
				} break;
				case TYPE_int: switch (typ->size) {
					case 1: case 2:
					case 4: fputfmt(fout, " = i32[%008x](%d)\n", sp->i4, sp->i4); break;
					case 8: fputfmt(fout, " = i64[%016X](%D)\n", sp->i8, sp->i8); break;
					default: goto TYPE_XXX;
				} break;
				case TYPE_flt: switch (typ->size) {
					case 4: fputfmt(fout, " = f32[%008x](%f)\n", sp->i4, sp->f4); break;
					case 8: fputfmt(fout, " = f64[%016X](%G)\n", sp->i8, sp->f8); break;
					default: goto TYPE_XXX;
				} break;
				case TYPE_rec: fputfmt(fout, ":struct\n"); break;
				case TYPE_arr: fputfmt(fout, ":array\n"); break;
			}
		}
	}
}

void vm_info(state s, cell pu, int tc, int cc) {
	int i;
	for (i = 0; i < cc; ++i) {
		//~ cell p = pu + i;
		
	}
}

/** exec
 * @arg env: enviroment
 * @arg cc: cell count
 * @arg ss: stack size
 * @return: error code
**/
int exec(state env, unsigned cc, unsigned ss, dbgf dbg) {
	struct cell_t proc[256], *pu = proc;	// execution units
	unsigned long size = env->code.mems;
	unsigned char *mem = env->code.mem;
	unsigned char *end = mem + size;

	register bcde ip;
	register unsigned char *st;

	if (env->errc)
		return -1;

	if (ss < env->code.sm * 4) {
		error(env, 0, "stack overrun\n");
		return -1;
	}
	if (cc > sizeof(proc) / sizeof(*proc)) {
		error(env, 0, "cell overrun\n");
		return -1;
	}
	if ((env->code.cs + env->code.ds + ss * cc) >= size) {
		error(env, 0, "`memory overrun\n");
		return -1;
	}

	for (; (signed)cc >= 0; cc -= 1) {
		pu = &proc[cc];
		pu->bp = end -= ss;
		pu->sp = 0;
		pu->ip = 0;
		//~ pu->zf = 0;
		//~ pu->sf = 0;
		//~ pu->cf = 0;
		//~ pu->of = 0;
	}

	pu = proc;
	pu->sp = pu->bp + ss;
	pu->ip = mem;//env->code.ip;		// start firt processor

	while ((pu = getProc(NULL, proc))) {

		st = pu->sp;			// stack
		ip = (bcde)pu->ip;

		//~ tick += pu == proc;		// count tiks

		if (dbg) {		// break
			// TODO: dbg function should set next break point ???
			switch (dbg(env, pu, 0, ip, ss)) {
				case 'c': dbg = 0; break;	// continue
				case 's': break;			// step into
				case  0:
				case 'n': break;			// step next
				case 'q': return 0;		// quit
				default: {
					debug("invalid return");
					return -1;
				}
			}
		}

		switch (ip->opc) {		// exec
			error_opc: error(env, 0, "invalid opcode: [%02x]", ip->opc); return -1;
			error_ovf: error(env, 0, "stack overflow: op[%A]", ip); return -2;
			error_div: error(env, 0, "division by zero: op[%A]", ip); break;
			//~ error_mem: error(env, 0, "segmentation fault"); return -5;
			//~ #define CDBG
			//~ #define FLGS
			#define NEXT(__IP, __SP, __CHK) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			//~ #define MEMP(__MPTR) (mp = (__MPTR))
			#define STOP(__ERR) goto __ERR
			#define EXEC
			#include "incl/exec.c"
		}
	}
	vm_tags(env, proc);
	return 0;
}

void fputopc(FILE *fout, bcde ip, int len, int offs) {
	int i;
	unsigned char* ptr = (unsigned char*)ip;
	if (len > 1 && len < opc_tbl[ip->opc].size) {
		for (i = 0; i < len - 2; i++) {
			if (i < opc_tbl[ip->opc].size) fprintf(fout, "%02x ", ptr[i]);
			else fprintf(fout, "   ");
		}
		if (i < opc_tbl[ip->opc].size)
			fprintf(fout, "%02x... ", ptr[i]);
	}
	else for (i = 0; i < len; i++) {
		if (i < opc_tbl[ip->opc].size) fprintf(fout, "%02x ", ptr[i]);
		else fprintf(fout, "   ");
	}
	//~ if (!opc) return;
	if (opc_tbl[ip->opc].name)
		fputs(opc_tbl[ip->opc].name, fout);
	else fputfmt(fout, "opc%02x", ip->opc);
	switch (ip->opc) {
		case opc_jmp:
		case opc_jnz:
		case opc_jz: {
			if (offs < 0) fprintf(fout, " %+d", ip->jmp);
			else fprintf(fout, " .%04x", offs + ip->jmp);
		} break;
		/*case opc_bit: switch (ip->arg.u1 >> 5) {
			default: fprintf(fout, "bit.???"); break;
			case  1: fprintf(fout, "shl %d", ip->arg.u1 & 0x1F); break;
			case  2: fprintf(fout, "shr %d", ip->arg.u1 & 0x1F); break;
			case  3: fprintf(fout, "sar %d", ip->arg.u1 & 0x1F); break;
			case  4: fprintf(fout, "rot %d", ip->arg.u1 & 0x1F); break;
			case  5: fprintf(fout, "get %d", ip->arg.u1 & 0x1F); break;
			case  6: fprintf(fout, "set %d", ip->arg.u1 & 0x1F); break;
			case  7: fprintf(fout, "cmt %d", ip->arg.u1 & 0x1F); break;
			case  0: switch (ip->arg.u1) {
				default: fprintf(fout, "bit.???"); break;
				case  1: fprintf(fout, "bit.any"); break;
				case  2: fprintf(fout, "bit.all"); break;
				case  3: fprintf(fout, "bit.sgn"); break;
				case  4: fprintf(fout, "bit.par"); break;
				case  5: fprintf(fout, "bit.cnt"); break;
				case  6: fprintf(fout, "bit.bsf"); break;		// most significant bit index
				case  7: fprintf(fout, "bit.bsr"); break;
				case  8: fprintf(fout, "bit.msb"); break;		// use most significant bit only
				case  9: fprintf(fout, "bit.lsb"); break;
				case 10: fprintf(fout, "bit.rev"); break;		// reverse bits
				// swp, neg, cmt, 
			} break;
		} break;*/
		case opc_pop: fprintf(fout, " %d", ip->idx); break;
		case opc_dup1: case opc_dup2:
		case opc_dup4: fprintf(fout, " sp(%d)", ip->idx); break;
		case opc_set1: case opc_set2:
		case opc_set4: fputfmt(fout, " sp(%d)", ip->idx); break;
		case opc_ldc1: fputfmt(fout, " %d", ip->arg.i1); break;
		case opc_ldc2: fputfmt(fout, " %d", ip->arg.i2); break;
		case opc_ldc4: fputfmt(fout, " %d", ip->arg.i4); break;
		case opc_ldc8: fputfmt(fout, " %D", ip->arg.i8); break;
		case opc_ldcf: fputfmt(fout, " %f", ip->arg.f4); break;
		case opc_ldcF: fputfmt(fout, " %F", ip->arg.f8); break;
		case opc_ldcr: fputfmt(fout, " %x", ip->arg.u4); break;
		//~ case opc_libc: fputfmt(fout, " %s", libcfnc[ip->idx].proto); break;
		case opc_libc: fputfmt(fout, " %+T", libcfnc[ip->idx].sym); break;
		case opc_sysc: switch (ip->arg.u1) {
			case  0: fprintf(fout, ".stop"); break;
			default: fprintf(fout, ".unknown"); break;
		} break;
		case p4d_swz: {
			char c1 = "xyzw"[ip->arg.u1 >> 0 & 3];
			char c2 = "xyzw"[ip->arg.u1 >> 2 & 3];
			char c3 = "xyzw"[ip->arg.u1 >> 4 & 3];
			char c4 = "xyzw"[ip->arg.u1 >> 6 & 3];
			fprintf(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->arg.u1);
		} break;
	}
}
void dumpasm(FILE *fout, unsigned char* beg, int len, int offs) {
	unsigned is = 1345, i;
	for (i = 0; i < len; i += is) {
		bcde ip = (bcde)(beg + i);
		switch (ip->opc) {
			error_opc: fputfmt(stderr, "invalid opcode: %02x '%?s'", ip->opc, tok_tbl[ip->opc].name); return;
			#define NEXT(__IP, __SP, __CHK) {is = (__IP);}
			#define STOP(__ERR) goto __ERR
			#include "incl/exec.c"
		}
		//~ fputfmt(fout, ".%04x:%9A\n", i, ip);
		//~ fputfmt(fout, ".%04x:\t%A\n", i, ip);
		fputfmt(fout, ".%04x: ", i);
		fputopc(fout, ip, 0, i);
		fputc('\n', fout);
	}
}

//}
//{ core.c ---------------------------------------------------------------------
int srctext(state s, char *file, int line, char *text) {
	if (source(s, source_text, text))
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
int gencode(state s, int level)
//!TODO level
{
	if (s->errc) return -1;

	s->code.mem = (unsigned char*)s->buffp;
	s->code.mems = bufmaxlen - (s->buffp - s->buffer);

	//~ s->code.ds = s->code.ic = 0;
	s->code.cs = s->code.pc = 0;
	s->code.ss = s->code.sm = 0;

	/* emit data seg
	for (sym = s->glob; sym; sym = sym->next) {
		if (sym->kind != TYPE_ref) continue;
		if (sym->init) {
			mem[s->code.ds] = sym->init;
		}
		s->code.ds += data->size;
		// TODO initialize
	}// */

	/* emit code seg
	for (ast = s->node; ast; ast = ast->next) {
		cgen(ast);
	}// */

	/*/ emit type inf
	for (data = s->glob; data; data = data->next) {
		if (data->kind != TYPE_def) continue;
		emit_typ(data);
		// TODO initialize
	}// */
	// emit init seg

	// emit dbug inf
	// emit(s, call, ".init()");
	// emit(s, call, ".main()");
	// emit(s, call, ".exit()");
	cgen(s, s->root, 0);	// keep locals on stack
	emit(s, opc_sysc, 0);
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

void instint(state s, defn it, int64t sgn) {
	int size = it->size;
	int bits = size * 8;
	int64t mask = (-1LLU >> -bits);
	int64t minv = sgn << (bits - 1);
	int64t maxv = (minv - 1) & mask;
	enter(s, it);
	install(s, CNST_int, "min",  0)->init = intnode(s, minv);
	install(s, CNST_int, "max",  0)->init = intnode(s, maxv);
	install(s, CNST_int, "mask", 0)->init = intnode(s, mask);
	install(s, CNST_int, "bits", 0)->init = intnode(s, bits);
	install(s, CNST_int, "size", 0)->init = intnode(s, size);
	it->args = leave(s);
}//~ */

int cc_init(state s) {
	int i, TYPE_opc = EMIT_opc;
	defn typ, def;
	defn type_i08 = 0, type_i16 = 0;
	defn type_u08 = 0, type_u16 = 0;
	defn type_v4f = 0, type_v2d = 0;
	//~ defn type_u64 = 0, type_f16 = 0;

	s->logf = 0; s->errc = 0;
	s->warn = wl; s->copt = ol;

	s->file = 0; s->line = s->nest = 0;

	s->_fin = s->_chr = -1;
	s->_ptr = 0; s->_cnt = 0;

	s->root = 0;
	s->defs = 0;
	s->tokp = 0;
	s->_tok = 0;
	s->all = 0;

	s->buffp = s->buffer;
	s->deft = (defT)cc_calloc(s, sizeof(defn*) * TBLS);
	s->strt = (strT)cc_calloc(s, sizeof(list*) * TBLS);

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
	type_v4f = install(s, TYPE_flt, "f32x4", 16);
	type_v2d = install(s, TYPE_flt, "f64x2", 16);

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
	typ->args = leave(s);

	enter(s, typ = install(s, TYPE_i64, "i64", 8));
	install(s, TYPE_opc, "neg", i64_neg)->type = type_i64;
	install(s, TYPE_opc, "add", i64_add)->type = type_i64;
	install(s, TYPE_opc, "sub", i64_sub)->type = type_i64;
	install(s, TYPE_opc, "mul", i64_mul)->type = type_i64;
	install(s, TYPE_opc, "div", i64_div)->type = type_i64;
	install(s, TYPE_opc, "mod", i64_mod)->type = type_i64;
	typ->args = leave(s);

	enter(s, typ = install(s, TYPE_f32, "f32", 4));
	install(s, TYPE_opc, "neg", f32_neg)->type = type_f32;
	install(s, TYPE_opc, "add", f32_add)->type = type_f32;
	install(s, TYPE_opc, "sub", f32_sub)->type = type_f32;
	install(s, TYPE_opc, "mul", f32_mul)->type = type_f32;
	install(s, TYPE_opc, "div", f32_div)->type = type_f32;
	install(s, TYPE_opc, "mod", f32_mod)->type = type_f32;
	typ->args = leave(s);

	enter(s, typ = install(s, TYPE_f64, "f64", 8));
	install(s, TYPE_opc, "neg", f64_neg)->type = type_f64;
	install(s, TYPE_opc, "add", f64_add)->type = type_f64;
	install(s, TYPE_opc, "sub", f64_sub)->type = type_f64;
	install(s, TYPE_opc, "mul", f64_mul)->type = type_f64;
	install(s, TYPE_opc, "div", f64_div)->type = type_f64;
	install(s, TYPE_opc, "mod", f64_mod)->type = type_f64;
	typ->args = leave(s);

	enter(s, typ = install(s, TYPE_enu, "v4f", 16));
	install(s, TYPE_opc, "neg", v4f_neg)->type = type_v4f;
	install(s, TYPE_opc, "add", v4f_add)->type = type_v4f;
	install(s, TYPE_opc, "sub", v4f_sub)->type = type_v4f;
	install(s, TYPE_opc, "mul", v4f_mul)->type = type_v4f;
	install(s, TYPE_opc, "div", v4f_div)->type = type_v4f;
	install(s, TYPE_opc, "dp3", v4f_dp3)->type = type_f32;
	install(s, TYPE_opc, "dp4", v4f_dp4)->type = type_f32;
	install(s, TYPE_opc, "dph", v4f_dph)->type = type_f32;
	typ->args = leave(s);

	enter(s, typ = install(s, TYPE_enu, "v2d", 16));
	install(s, TYPE_opc, "neg", v2d_neg)->type = type_v2d;
	install(s, TYPE_opc, "add", v2d_add)->type = type_v2d;
	install(s, TYPE_opc, "sub", v2d_sub)->type = type_v2d;
	install(s, TYPE_opc, "mul", v2d_mul)->type = type_v2d;
	install(s, TYPE_opc, "div", v2d_div)->type = type_v2d;
	typ->args = leave(s);

	enter(s, typ = install(s, TYPE_enu, "swz", 2));
	{
		int i;
		defn swz;
		for (i = 0; i < 256; i += 1) {
			s->buffp[0] = "wxyz"[i>>0&3];
			s->buffp[1] = "wxyz"[i>>2&3];
			s->buffp[2] = "wxyz"[i>>4&3];
			s->buffp[3] = "wxyz"[i>>6&3];
			s->buffp[4] = 0;

			//~ printf("\t%c%c%c%c = 0x%02x;\n", c4, c3, c2, c1, i);
			swz = install(s, TYPE_opc, mapstr(s, s->buffp, 4, -1), p4d_swz);
			swz->init = intnode(s, i);
			swz->type = typ;
		}
	}
	typ->args = leave(s);

	emit_opc->args = leave(s);

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
	def->args = leave(s);
	//} */
	//{ install Libc
	//~ enter(s, def = install(s, TYPE_enu, "Libc", 0));
	for (i = 0; i < sizeof(libcfnc) / sizeof(*libcfnc); i += 1) {
		libcfnc[i].sym = install(s, -1, libcfnc[i].proto, 0);
		//~ libcfnc[i].sym->offs = i;

		//~ defn fun = install(s, -1, libcfnc[i].proto, 0);
		//~ dieif(fun == NULL);
		//~ libcfnc[i].sym = fun;
		//~ libcfnc[i].sym->offs = i;
		//~ libcfnc[i].sym->libc = 1;
		//libcfnc[i].arg = argsize;
		//~ libcfnc[i].ret = fun->type->size;
		//~ libcfnc[i].pop = fun->size;
		//~ cc_libc(s, &libcfnc[i], i);
	}
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
int vm_test() {
	int e = 0;
	struct bcdc_t opc, *ip = &opc;
	opc.arg.i8 = 0;
	for (opc.opc = 0; opc.opc < opc_last; opc.opc++) {
		int err = 0;
		//~ if (opc_tbl[i].size <= 0 || !opc_tbl[i].name) continue;

		if (opc_tbl[opc.opc].code != opc.opc) {
			fprintf(stderr, "invalid '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc);
			e += err = 1;
		}
		else switch (opc.opc) {
			error_er1: e += 1; fprintf(stderr, "error1 '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc); break;
			error_er2: e += 1; fprintf(stderr, "error2 '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc); break;
			error_er3: e += 1; fprintf(stderr, "error3 '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc); break;
			error_opc: fprintf(stderr, "-unimplemented opcode %02x %s\n", opc.opc, opc_tbl[opc.opc].name); break;
			#define NEXT(__IP, __SP, __CHK) {\
				if (opc_tbl[opc.opc].size != -1 && opc_tbl[opc.opc].size != (__IP)) goto error_er1;\
				if (opc_tbl[opc.opc].chck != -1 && opc_tbl[opc.opc].chck != (__CHK)) goto error_er2;\
				if (opc_tbl[opc.opc].pops != -1 && opc_tbl[opc.opc].pops != -(__SP)) goto error_er3;\
			}
			#define STOP(__ccc) goto __ccc;
			#include "incl/exec.c"
		}
	}
	return e;
}

#if 0
int vm_hgen() {
	int i, e = 0;
	FILE *out = stdout;
	for (i = 0; i < opc_last; i++) {
		int err = 0;
		//~ if (opc_tbl[i].size <= 0 || !opc_tbl[i].name) continue;

		fprintf(out, "\nInstruction: %s: #\n", opc_tbl[i].name);
		fprintf(out, "Opcode		argsize		Stack trasition\n");
		fprintf(out, "0x%02x		%d		[..., a, b, c, d => [..., a, b, c, d#\n", opc_tbl[i].code, opc_tbl[i].size-1);

		fprintf(out, "\nDescription\n");
		if (opc_tbl[i].code != i) {
			e += err = 1;
		}
		else switch (i) {
			error_opc: e += err = 1; break;
			#define NEXT(__IP, __SP, __CHK) if (opc_tbl[i].size != (__IP)) goto error_opc;
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
	//~ char *pn = strrchr(prog, '/');
	//~ if (pn) prog = pn + 1;
	if (cmd == NULL) {
		fputfmt(stdout, "Usage: %s command [options] ...\n", prog);
		fputfmt(stdout, "where command can be one of:\n");
		fputfmt(stdout, "\t-c: compile\n");
		fputfmt(stdout, "\t-e: execute\n");
		fputfmt(stdout, "\t-m: make\n");
		fputfmt(stdout, "\t-h: help\n");
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
}

int evalexp(state s, char* text) {
	struct astn_t res;
	node ast;
	defn typ;
	int tid;

	source(s, source_text, text);
	ast = expr(s, 0);
	typ = lookup(s, 0, ast);
	tid = eval(&res, ast, 0);

	//~ dump(s, stdout, dump_ast);
	if (!ast || !typ || !tid || peek(s)) {
		fputfmt(stdout, "ERROR\n", exp, &res);
		//~ return -2;
	}

	fputfmt(stdout, "eval(`%+k`) = %T(%k)\n", ast, typ, &res, tok_tbl[tid].name);

	return 0;
}

struct state_t env;

int dbgCon(state env, cell pu, int pn, bcde ip, unsigned ss) {
	char cmd[1024];
	int stki, stkn = (pu->bp + ss - pu->sp);

	fputfmt(stdout, ">exec:pu%02d:[ss:%03d]: %A\n", pn, stkn/4, ip);
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
			stkval* sp = (stkval*)pu->sp;
			fputfmt(stdout, ">exec:pu%02d:[ss:%03d, zf:%d sf:%d cf:%d of:%d]: %A\n", pn, stkn/4, pu->zf, pu->sf, pu->cf, pu->of, ip);
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

	ret = srcfile(s, src);

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
	i = s->code.cs; printf(" code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, s->code.ic);
	i = s->code.sm; printf(" max stack: %dM, %dK, %dB, %d slots\n", i*4 >> 20, i*4 >> 10, i*4, s->code.sm);
	//~ i = s->code.ds; printf(" data size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);

	printf(">tags:file\n"); dump(s, stdout, dump_sym);
	//~ printf(">tags:lstd\n"); dumpsym(stdout, leave(s));
	//~ printf(">tags:emit\n"); dumpsym(stdout, emit_opc);
	/*
	{
		defn sym = s->all;
		for (sym = s->all; sym; sym = sym->all) {
			//~ fputfmt(stdout, "%s:%d:>%+T\n", sym->file, sym->line, sym);
			fputc('>', stdout);
			dumpsym(stdout, sym, 0);
		}
	}// */

	printf(">code:ast\n"); dump(s, stdout, dump_ast);
	printf(">code:asm\n"); dump(s, stdout, dump_asm);
	//~ printf(">code:xml\n"); dump(s, stdout, dump_xml);

	time = clock();
	ret = exec(s, cc, ss, 0);
	time = clock() - time;
	printf(">exec: Exit code: %d\tTime: %lg\n", ret, (double)time / CLOCKS_PER_SEC);

	return ret;
}// */

int main(int argc, char *argv[]) {
	state s = &env;
	char *prg, *cmd;
	int printinfo = 0;

	//~ /* TODO if debug
	//~ return vm_test();
	if (1 && argc == 1) {
		char *args[] = {
			"psvm",		// program name
			//~ "-api",
			"-c",		// compile command
			"main.cvx",
			"-s",		// execute
			//~ "-C0", //"-o", "test0.xml",		// xml
		};
		argc = sizeof(args) / sizeof(*args);
		argv = args;
		cc_init(s);
		//~ return dbgFile(s, "");
		return dbgFile(s, "main.cvx");
	}// */

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	cc_init(s);
	prg = argv[0];
	cmd = argv[1];
	if (argc < 2) usage(s, prg, NULL);
	else if (strcmp(cmd, "-api") == 0) {
		dumpsym(stdout, leave(s), 1);
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
			case out_dasm: dump(s, fout, dump_asm + level); break;

			case run_bin:		// exec
				exec(s, cc, ss, NULL);
				break;

			case dbg_bin:		// exec
				exec(s, cc, ss, dbgCon);
				break;
		}

		if (outf) {
			fclose(fout);
		}

		if (!printinfo) {
			int i;
			i = s->buffp - s->buffer; printf("parse mem: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
			i = s->code.sm * 4; printf("stack max: %dM, %dK, %dB, %d slots\n", i >> 20, i >> 20, i, s->code.sm);
			i = s->code.cs; printf("code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, s->code.ic);
			i = s->code.ds; printf("data size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
		}

		return cc_done(s);
	}
	else if (strcmp(cmd, "-e") == 0) {	// execute
		fatal(s, "unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return exec(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-d") == 0) {	// disassemble
		fatal(s, "unimplemented option '%s' \n", cmd);
		//~ objfile(s, ...);
		//~ return dumpasm(s, cc, ss, dbgl);
	}
	else if (strcmp(cmd, "-m") == 0) {	// make
		fatal(s, "unimplemented option '%s' \n", cmd);
	}
	else if (strcmp(cmd, "-h") == 0) {	// help
		usage(s, argv[0], argc < 4 ? argv[3] : NULL);
	}
	else if (argc == 2 && *cmd != '-') {			// try to eval
		return evalexp(s, cmd);
	}
	else fputfmt(stderr, "invalid option '%s'", cmd);
	return 0;
}

#if 0	//dbg_....
int dbg_test(state s) {
	//~ int c0, c1, c2;
	cc_init(s);
	srctext(s, __FILE__, __LINE__ + 1,
		"flt64 a_re = 3;\n"
		"flt64 a_im = 4;\n"		// -14, 23
		"flt64 b_re = 2;\n"
		"flt64 b_im = 5;\n"
		"emit(v2d.mul, f64(b_im), f64(b_re), f64(a_im), f64(a_re));\n"		// a.re * b.re, a.im * b.im
		"flt64 re = emit(f64.sub);\n"										//>re = a.re * b.re - a.im * b.im
		"emit(v2d.mul, f64(b_re), f64(b_im), f64(a_im), f64(a_re));\n"		// a.re * b.im, a.im * b.re
		"flt64 im = emit(f64.add);\n"										//>im = a.re * b.im + a.im * b.re

		//~ "emit(v4f.mul, f64(i2), f64(r2), f64(im), f64(re));\n"	// tmp1,2 = a.re * b.re, a.im * b.im
		//~ "im = emit(f64.add);\n"			//>im = a.re * b.im + a.im * b.re
		//~ "emit(v4f.mul, f64(r2), f64(i2), f64(im), f64(re));\n"	// tmp3,4 = a.re * b.im, a.im * b.re
		//~ "re = emit(f64.sub);\n"			//>re = a.re * b.re - a.im * b.im

		//~ "!flt32(2. + (3 + 1));\n"			//>re = a.re * b.re - a.im * b.im
		//~ "emit(f32.neg, f32(emit(f32.neg, f32(2))));\n"					//>re = a.re * b.re - a.im * b.im
		//~ "emit(f64.div, f64(2), flt64(3.14));\n"			//>re = a.re * b.re - a.im * b.im

	);// */

	//~ printf("Stack max = %d / %d slots\n", s->code.mins * 4, s->code.mins);
	//~ printf("Code size = %d / %d instructions\n", s->code.cs, s->code.ic);
	compile(s, 0);
	gencode(s, 0);

	//~ emitf64(s, opc_ldcF, 3.14);
	//~ emitf64(s, opc_ldcF, 2.);
	//~ emit(s, f64_div);

	//~ emitf32(s, opc_ldcf, 3.14);
	//~ emitf32(s, opc_ldcf, 2);
	//~ emit(s, f32_div);

	//~ emitidx(s, opc_dup4, c2);
	//~ emitopc(s, p4d_mul);
	//~ emitopc(s, f64_sub);
	dumpasm(s, stdout);

	exec(s, cc, ss, 0);

	return cc_done(s);
}

//~ #define USE_COMPILED_CODE
//~ #include "incl/test.c"
int main(int argc, char *argv[]) {

	cc_init(&env);				//for tests
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	//~ return dbg_test(&env);
	return dbg_file(&env, "main.cvx");

	/*for (i = 0; i < TBLS; i += 1) {
		for (sym = env.deft[i]; sym; sym = sym->next) {
			//~ dumpsym(stdout, "%?+T", env.deft[i]);
			dumpsym(stdout, env.deft[i]);
			fputs(sym->next ? ", " : "\n", stdout);
		}
	}// */

	//~ return cplfltmul(&env);
	//~ return cpldblmul(&env);
	//~ return swz_test(&env);
	//~ return dbg_emit(&env);
	//~ return vm_hgen();
	//~ mk_test(src = "txx.cxx", 1<<20);
	//~ return dbg_test(&env);
	//~ return dbg_eval(&env, "1 < 2");
	//~ return dbg_eval(&env, "math.sin(flt32(math.pi))");
	//~ return dbg_file(&env, src);

	//~ return dbg_emit(&env);

	/* swizzle generator
	for(i = 0; i < 256; i++) {
		char c1 = "xyzw"[i>>0&3];
		char c2 = "xyzw"[i>>2&3];
		char c3 = "xyzw"[i>>4&3];
		char c4 = "xyzw"[i>>6&3];
		//~ printf("\t%c%c%c%c = 0x%02x;\n", c4, c3, c2, c1, i);
		printf("install(s, \"%c%c%c%c\", -1, CNST_int, %s)->init = intnode(s, 0x%02x);\n", c4, c3, c2, c1, "mod", i);
	} // */
	//~ /*
	//~ cc_init(&env);
	//~ dumpall(stdout, &env);
	/*for(i = 0; i < sizeof(libcfnc)/sizeof(*libcfnc); i++) {
		fputfmt(stdout, "%T;ret:%d;pop:%d;arg:%d\n", libcfnc[i].sym, libcfnc[i].ret, libcfnc[i].pop, libcfnc[i].arg);
	}//~ */
	//~ return i = 0;
}

#endif
