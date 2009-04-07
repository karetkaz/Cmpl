#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ccvm.h"

//~ ~/owc/binl/wmake

#if 0	// traces
int dbg_lev = 0;
#define LEVEL 0x00
#define trace(__LEV, msg, ...) {int L = __LEV; if (L < 0) dbg_lev -= 1; if ((__LEV) == 0 || ((L < 0 ? -L : L) & (LEVEL))) fputmsg(stderr, "%s:%d:% I%ctrace:"msg"\n", __FILE__, __LINE__, dbg_lev, "}{"[L > 1], ##__VA_ARGS__); if (L > 1) dbg_lev += 1;}
#define trace(__LEV, msg, ...) {int L = __LEV; if (L < 0) lev -= 1; if ((__LEV) >= 0 ) fputmsg(stderr, "%s:%d:%I%ctrace:"msg"\n", __FILE__, __LINE__, lev, "}{"[L > 1], ##__VA_ARGS__); if (L > 1) lev += 1;}
#endif

// default values
static const int wl = 9;		// warninig level
static const int ol = 1;		// optimize level
static const int dl = 2;		// deebug level
static const int cc = 1;		// cores
static const int ss = 4 << 20;		// stack size (4M)
//~ static const int hs = 32 << 20;		// heap size (32M)

#ifdef __WATCOMC__
#pragma disable_message(124, 201);
#endif

#if defined(WINDOWS) || defined(_WIN32)
static const char* os = "Windows";
#elif defined(linux) || defined(_linux)
static const char* os = "Linux";
#else
static const char* os = "Unknown";
#endif

const tok_inf tok_tbl[255] = {
	#define TOKDEF(NAME, TYPE, SIZE, KIND, STR) {KIND, TYPE, SIZE, STR},
	#include "incl/defs.inl" 
};

const opc_inf opc_tbl[255] = {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) {Code, Size, Mnem},
	#include "incl/defs.inl"
};

/*typedef union {
	flt32t d[4];
	struct {
		flt32t x;
		flt32t y;
		flt32t z;
		flt32t w;
	};
	struct {
		flt32t r;
		flt32t g;
		flt32t b;
		flt32t a;
	};
} f32x4t, *f32x4;
typedef union {
	flt64t d[2];
	struct {
		flt64t x;
		flt64t y;
	};
	struct {
		flt64t re;
		flt64t im;
	};
} f64x2t, *f64x2;
typedef union {
	uint8_t _b[16];
	uint16_t _w[8];
	uint32_t _l[4];
	uint64_t _q[2];
	flt32t _s[4];
	flt64t _d[2];
} XMMReg;
typedef union {
	uint8_t _b[8];
	uint16_t _w[2];
	uint32_t _l[1];
	uint64_t q;
} MMXReg;*/

unsigned i32CmpF(uns32t lhs, uns32t rhs) {
	uns32t res = lhs - rhs;
	int sf = res < 0;
	int zf = res == 0;
	int cf = res < lhs;
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

//~ #include "clog.c" // no printf
//~ #include "clex.c" // no printf
//~ #include "scan.c" // no printf
//~ #include "tree.c" // no printf
//~ #include "type.c" // no printf

//{ #include "code.c"
//~ defn type_opc = NULL;
//~ code.c - Builder & Executor ------------------------------------------------
#include <math.h>
#include "libs/lbit.c"		// binary
#include "incl/libc.inl"

static const stkval noarg = {0};
stkval argf32(flt64t arg) {
	stkval tmp;
	tmp.f4 = (flt32t)arg;
	return tmp;
}

stkval argf64(flt64t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return tmp;
}

stkval argi32(int32t arg) {
	stkval tmp;
	tmp.i4 = arg;
	return tmp;
}

stkval argi64(int64t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return tmp;
}

stkval argi(int arg) {
	stkval tmp;
	tmp.i4 = arg;
	return tmp;
}

stkval argidx(state s, int arg) {
	stkval tmp;
	tmp.i4 = s->code.rets - arg;
	return tmp;
}

int emit(state s, int opc, stkval arg) {
	bcde ip = 0;//(bcde)&s->code.memp[s->code.pc];
	if (s->code.cs >= s->code.mems - 16) {
		error (s, 0, 0, "memory overrun");
		return -1;
	}
	if (s->code.mins < s->code.rets)
		s->code.mins = s->code.rets;

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
		case 1 : opc = opc_ldi1; break;
		case 2 : opc = opc_ldi2; break;
		case 4 : opc = opc_ldi4; break;
		default: return -1;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case 1 : opc = opc_sti1; break;
		case 2 : opc = opc_sti2; break;
		case 4 : opc = opc_sti4; break;
		default: return -1;
	}*/

	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32 :
		case TYPE_i32 : opc = i32_add; break;
		case TYPE_i64 : opc = i64_add; break;
		case TYPE_f32 : opc = f32_add; break;
		case TYPE_f64 : opc = f64_add; break;
		//~ case TYPE_pf2 : opc = p4d_add; break;
		//~ case TYPE_pf4 : opc = p4f_add; break;
		default: return -1;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32 :
		case TYPE_i32 : opc = i32_sub; break;
		case TYPE_i64 : opc = i64_sub; break;
		case TYPE_f32 : opc = f32_sub; break;
		case TYPE_f64 : opc = f64_sub; break;
		//~ case TYPE_pf2 : opc = p4d_sub; break;
		//~ case TYPE_pf4 : opc = p4f_sub; break;
		default: return -1;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32 : //opc = u32_mul; break;
		case TYPE_i32 : opc = i32_mul; break;
		case TYPE_i64 : opc = i64_mul; break;
		case TYPE_f32 : opc = f32_mul; break;
		case TYPE_f64 : opc = f64_mul; break;
		//~ case TYPE_pf2 : opc = p4d_mul; break;
		//~ case TYPE_pf4 : opc = p4f_mul; break;
		default: return -1;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32 : //opc = u32_div; break;
		case TYPE_i32 : opc = i32_div; break;
		case TYPE_i64 : opc = i64_div; break;
		case TYPE_f32 : opc = f32_div; break;
		case TYPE_f64 : opc = f64_div; break;
		//~ case TYPE_pf2 : opc = p4d_div; break;
		//~ case TYPE_pf4 : opc = p4f_div; break;
		default: return -1;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		case TYPE_u32 : //opc = u32_mod; break;
		case TYPE_i32 : opc = i32_mod; break;
		case TYPE_i64 : opc = i64_mod; break;
		case TYPE_f32 : opc = f32_mod; break;
		case TYPE_f64 : opc = f64_mod; break;
		//~ case TYPE_pf2 : opc = p4d_mod; break;
		//~ case TYPE_pf4 : opc = p4f_mod; break; p4f_crs ???
		default: return -1;
	}

	else if (opc == opc_not) switch (arg.i4) {
		//~ case TYPE_u32 :
		//~ case TYPE_i32 : opc = i32_add; break;
		//~ case TYPE_i64 : opc = i64_add; break;
		//~ case TYPE_f32 : opc = f32_add; break;
		//~ case TYPE_f64 : opc = f64_add; break;
		//~ case TYPE_pf2 : opc = p4d_add; break;
		//~ case TYPE_pf4 : opc = p4f_add; break;
		default: return -1;
	}
	else if (opc == opc_ceq) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i64; break;
		case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_ceq | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_clt) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_clt | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i64; break;
		case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_clt | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_clt | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_cle) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_cle | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i64; break;
		case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_cle | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_cle | cmp_f64; break;
		default: return -1;
	}

	else if (opc == opc_cne) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i64; break;
		case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_not | opc_ceq | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_cge) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i64; break;
		case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_not | opc_clt | cmp_f64; break;
		default: return -1;
	}
	else if (opc == opc_cgt) switch (arg.i4) {
		case TYPE_i32 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i32; break;
		case TYPE_u32 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i32 | cmp_tun; break;
		case TYPE_f32 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_f32; break;
		case TYPE_i64 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i64; break;
		case TYPE_u64 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_i64 | cmp_tun; break;
		case TYPE_f64 : opc = opc_cmp; arg.u8 = opc_cgt | cmp_f64; break;
		default: return -1;
	}

	// bits
	else if (opc == opc_shl) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_shl; break;
		default: return -1;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_u32 : opc = b32_sar; break;
		case TYPE_i32 : opc = b32_shr; break;
		default: return -1;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_and; break;
		default: return -1;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_ior; break;
		default: return -1;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32 : 
		case TYPE_i32 : opc = b32_xor; break;
		default: return -1;
	}

	//~ :)) ?? rollback (optimize)
	else if (opc == opc_ldz1) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_ldz1) {
			opc = opc_ldz2;
			s->code.cs = s->code.pc;
			s->code.rets -= 1;
		}
	}
	else if (opc == opc_ldz2) { 
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_ldz2) {
			opc = opc_ldz4;
			s->code.cs = s->code.pc;
			s->code.rets -= 2;
		}
	}
	/*else if (opc == opc_dup1) {			// TODO 
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_dup1 && ip->idx == arg.u4) {
			opc = opc_dup2;
			s->code.cs = s->code.pc;
			s->code.rets -= 1;
		}
	}// */
	else if (opc == opc_dup2) {
		ip = (bcde)&s->code.memp[s->code.pc];
		if (ip->opc == opc_dup2 && ip->idx == arg.u4) {
			ip->opc = opc_dup4;
			ip->idx -= 2;
			s->code.rets += 2;
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

	ip = (bcde)&s->code.memp[s->code.cs];
	s->code.ic += s->code.pc != s->code.cs;
	s->code.pc = s->code.cs;

	ip->opc = opc;
	ip->arg = arg;

	switch (opc) {
		error_opc : fatal(s, "emit:invalid opcode:[%02x]:%A", ip->opc, ip); return -1;
		error_stc : fatal(s, "emit:stack underflow: near opcode %A", ip); return -1;
		#define NEXT(__IP, __SP, __CHK)\
			if(s->code.rets < (__CHK)) goto error_stc;\
			s->code.rets += (__SP);\
			s->code.cs += (__IP);
		#define STOP(__ERR) goto __ERR
		//~ #define EXEC(__STMT)
		//~ #define MEMP(__MPTR)
		//~ #define CHECK()
		#include "incl/exec.inl"
	}
	return s->code.pc;
}

int fixj(state s, int src, int dst, int stc) {
	bcde ip = (bcde)(s->code.memp + src);
	if (src >= 0) switch (ip->opc) {
		case opc_jmp : 
		case opc_jnz : 
		case opc_jz  : ip->jmp = dst - src; break;
		default : 
			debug("expecting jump, but found : '%+A'", ip);
	}
	return 0;
}

/*int fixt(state s, int dst, int src) {
	if(s->code.rets < 1) return -1;
	if (dst != src) switch (dst) {
		case TYPE_bol : switch (src) {
			case TYPE_u32 : break;	// cannot happen : get == ret
			case TYPE_i32 : break;	// cannot happen : get == ret
			case TYPE_f32 : {
				emit(s, f32_not, noarg);
				emit(s, opc_not, noarg);
			} break;
			case TYPE_f64 : {
				emit(s, f64_not, noarg);
				emit(s, opc_not, noarg);
			} break;
			default : goto errorcast;
		} break;
		case TYPE_u32 : switch (src) {
			case TYPE_bol : break;
			case TYPE_u32 : break;	// cannot happen : get == ret
			case TYPE_i32 : break;	// cannot happen : get == ret
			case TYPE_f32 : emit(s, f32_i32, noarg); break;
			case TYPE_f64 : emit(s, f64_i32, noarg); break;
			default : goto errorcast;
		} break;
		case TYPE_i32 : switch (src) {
			case TYPE_bol : break;
			case TYPE_u32 : break;	// cannot happen : get == ret
			case TYPE_i32 : break;	// cannot happen : get == ret
			case TYPE_f32 : emit(s, f32_i32, noarg); break;
			case TYPE_f64 : emit(s, f64_i32, noarg); break;
			default : goto errorcast;
		} break;
		case TYPE_f32 : switch (src) {
			case TYPE_bol :
			case TYPE_u32 :
			case TYPE_i32 : emit(s, i32_f32, noarg); break;
			case TYPE_f64 : emit(s, f64_f32, noarg); break;
			//~ case TYPE_flt : break;	// cannot happen : get == ret
			default : goto errorcast;
		} break;
		case TYPE_f64 : switch (src) {
			case TYPE_bol :
			case TYPE_u32 :
			case TYPE_i32 : emit(s, i32_f64, noarg); break;
			case TYPE_f32 : emit(s, f32_f64, noarg); break;
			//~ case TYPE_f64 : break;	// cannot happen : get == ret
			default : goto errorcast;
		} break;
		default : //debug("error:cgen: invalid (%04x) : %04x", dst, src);
		errorcast: debug("error:cgen: invalid cast :(%s to %s)", tok_tbl[dst].name, tok_tbl[src].name);
	}
	return 0;
}*/

int isemit(node ast) {
	return ast && ast->kind == EMIT_opc;
}

defn linkof(node ast) {
	if (ast && ast->kind == OPER_dot)
		ast = ast->rhso;
	if (ast && ast->kind == TYPE_ref)
		return ast->link;
	debug("linkof:%s",tok_tbl[ast->kind].name);
 	//~ if (ast && ast->kind == TYPE_lnk)
		//~ return ast->link;
	return 0;
}

int cgen(state s, node ast, int get) {
	long sc = s->code.rets;
	struct astn_t tmp;
	int ret = 0;

	if (!ast) return 0;

	if (tok_tbl[ast->kind].argc && eval(&tmp, ast, get))
		return cgen(s, &tmp, get);

	trace(0, "cgen(%?T(%k)):%?s", ast->type, ast, tok_tbl[get].name);

	switch (ast->kind) {
		//{ STMT
		//~ case TYPE_def : break;
		case OPER_nop : {		// expr/defn/decl
			emit(s, opc_line, argi32(ast->line));	// lineInfo
			if (ast->type && ast->stmt) {
				node tag = ast->stmt;
				while (tag != NULL) {
					cgen(s, tag, TYPE_any);
					tag = tag->next;
				}
				get = ret;
			}
			else if (ast->stmt) {
				int get = TYPE_vid;
				node n = ast->stmt;
				if (n && n->kind == ASGN_set) {
					n = n->rhso;
				} 
				//~ get = n == ast->stmt && n->lhso && n->lhso->kind == EMIT_opc
				if (n && n->kind == OPER_fnc)
					n = n->lhso;
				if (n && n->kind == EMIT_opc) {
					ret = get;// = TYPE_vid;
					cgen(s, ast->stmt, EMIT_opc);
				}
				else cgen(s, ast->stmt, TYPE_vid);
			}
			else if (ast->type) {
				//~ typedef(static init ???)
			}
		} return 0;
		case OPER_beg : {		// {}
			defn typ;
			node ptr;
			int stc = 0;
			emit(s, opc_line, argi32(ast->line));
			for (ptr = ast->stmt; ptr; ptr = ptr->next) {
				cgen(s, ptr, TYPE_vid);
				//~ if (stc) // this can be a type decl
			}
			for (typ = ast->type; typ; typ = typ->next)
				if (typ->onst) stc += 1;
			if (stc) emit(s, opc_pop, argi32(stc));
		} return 0;
		case OPER_jmp : {		// if ( ) then {} else {}
			emit(s, opc_line, argi32(ast->line));
			if (ast->test) {					// if then else
				struct astn_t test;
				int jmpt, jmpf;
				if (s->copt > 0 && eval(&test, ast->test, TYPE_bol)) {		// if true
					if (constbol(&test))
						cgen(s, ast->stmt, TYPE_vid);
					else
						cgen(s, ast->step, TYPE_vid);
				}
				else if (ast->stmt && ast->step) {		// if then else
					cgen(s, ast->test, TYPE_bol);
					jmpt = emit(s, opc_jz, noarg);
					cgen(s, ast->stmt, TYPE_vid);
					jmpf = emit(s, opc_jmp, noarg);
					fixj(s, jmpt, s->code.cs, 0);
					cgen(s, ast->step, TYPE_vid);
					fixj(s, jmpf, s->code.cs, 0);
				}
				else if (ast->stmt) {				// if then
					cgen(s, ast->test, TYPE_bol);
					//~ if false skip THEN block
					jmpt = emit(s, opc_jz, noarg);
					cgen(s, ast->stmt, TYPE_vid);
					fixj(s, jmpt, s->code.cs, 0);
				}
				else if (ast->step) {				// if else
					cgen(s, ast->test, TYPE_bol);
					//~ if true skip ELSE block
					jmpt = emit(s, opc_jnz, noarg);
					cgen(s, ast->step, TYPE_vid);
					fixj(s, jmpt, s->code.cs, 0);
				}
				else {
					//~ cgen(s, ast->test, TYPE_vid);
					//~ print(s, 8, s->file, ast->line, "");
				}
			}
			else debug("unimplemented: goto / break/ continue;");
		} return 0;
		case OPER_for : {		// for ( ; ; ) {}
			//!TODO: replace 'xxx = s->code.cs' with 'xxx = emit(...)'
			defn typ;
			int stc = 0;
			int beg, end, cmp = -1;

			emit(s, opc_line, argi32(ast->line));
			cgen(s, ast->init, TYPE_any);

			beg = s->code.cs;
			if (ast->step) {
				int tmp = beg;
				if (ast->init) emit(s, opc_jmp, argi32(0));

				beg = s->code.cs;
				cgen(s, ast->step, TYPE_vid);
				if (ast->init) fixj(s, tmp, s->code.cs, 0);
			}
			if (ast->test) {
				cgen(s, ast->test, TYPE_bol);
				cmp = emit(s, opc_jz, argi32(0));		// if (!test) break;
			}

			// push(list_jmp, 0);
			cgen(s, ast->stmt, TYPE_vid);
			end = emit(s, opc_jmp, argi32(beg));			// continue;
			fixj(s, end, beg, 0);
			fixj(s, cmp, s->code.cs, 0);				// break;
			//~ fixj(s, cmp, end, 0);				// break;

			//~ while (list_jmp) {
			//~ if (list_jmp->kind == break)
			//~ 	fixj(s, list_jmp->offs, end, 0);
			//~ if (list_jmp->kind == continue)
			//~ 	fixj(s, list_jmp->offs, beg, 0);
			//~ list_jmp = list_jmp->next;
			//~ }
			// pop(list_jmp);


			for (typ = ast->type; typ; typ = typ->next)
				if (typ->onst) stc += 1;
			if (stc) emit(s, opc_pop, argi32(stc));
		} return 0;
		//}

		//{ OPER
		case OPER_dot : {
			ret = cgen(s, ast->rhso, typeId(ast->rhso));
			//~ if (!cgen(s, ast->rhso, TYPE_i32)) return 0;
		} break;
		case OPER_fnc : {		// '()' emit/cast/call/libc
			if (ast->lhso == NULL) {		// ()
				ret = ast->cast;
				//~ ret = cgen(s, ast->rhso, ast->cast);
				if (cgen(s, ast->rhso, ret) != ret) {
					fatal(s, "cgen(%k)", ast);
					return 0;
				}
			}
			else if (istype(ast->lhso)) {		// cast()
				//~ defn typ = 0;//, lnk = 0;
				//~ node argv = ast->rhso;
				//~ typ = ast->lhso->type;
				ret = cgen(s, ast->rhso, typeId(ast->lhso));
				//~ debug("cast %+k(%+k):%?T", ast->lhso, ast->rhso, typ);
			}
			else if (isemit(ast->lhso)) {		// emit()
				int stu = s->code.cs;
				defn typ = 0, lnk = 0;
				node argv = ast->rhso;
				typ = ast->lhso->type;
				while (argv && argv->kind == OPER_com) {
					if ((ret = cgen(s, argv->rhso, EMIT_opc)) < 0) {
						fatal(s, "cgen:arg(%k)", argv->rhso);
						return 0;
					}
					argv = argv->lhso;
				}
				if ((lnk = linkof(argv))) {
					stu = s->code.cs - stu;
					/*if (lnk->size != stu) {
						fatal(s, "cgen:arg(%k)", argv->rhso);
						return 0;
					}// */
					if (emit(s, lnk->offs, noarg) < 0) {
						fatal(s, "cgen(%k)", ast);
						return 0;
					}// */
					//~ debug("emit(%+k:%02x]):%T", argv, (int32t)lnk->offs, lnk->type);
					ret = get;//lnk->type->kind;
				}
			}
			else {					// call()
				defn typ = 0, lnk = 0;
				node argv = ast->rhso;
				typ = ast->lhso->type;
				while (argv) {
					node arg = argv->kind == OPER_com ? argv->rhso : argv;
					//~ debug("arg: %+k:%T", arg, arg->type);
					//~ cgen(s, arg, argt->type->kind);
					//~ cgen(s, arg, typeId(arg));
					if ((ret = cgen(s, arg, arg->cast)) < 0) {
						fatal(s, "cgen:arg(%k)", arg);
						return 0;
					}

					argv = argv->kind == OPER_com ? argv->lhso : 0;
					//~ argt = argt->next;
				}
				switch (ast->lhso->kind) {			// get fun
					case TYPE_ref : {
						typ = ast->lhso->type;
						lnk = ast->lhso->link;
					} break;
					case OPER_dot : if (ast->lhso->rhso) {
						typ = ast->lhso->rhso->type;
						lnk = ast->lhso->rhso->link;
					} break;
					//~ case OPER_arr : break;
				}
				if (lnk && lnk->kind == TYPE_fnc) {
					ret = typ ? typ->kind : 0;
					if (lnk->libc) {
						debug("libc %+k(%+k):%T [%+T]", ast->lhso, ast->rhso, typ, lnk);
						emit(s, opc_libc, argi32(lnk->offs));
					}
					else {
						debug("call %+k(%+T):%T", ast->lhso, lnk->args, typ);
						fatal(s, "unimpl function call %+1k", ast->lhso);
					}
				}
				else debug("call ???? %+k(%+k):%T", ast->lhso, ast->rhso, typ);
			}
			//~ debug("cgen(`%+k`, %s):%s", ast, tok_tbl[get].name, tok_tbl[ret].name);
		} break;
		/*case OPER_idx : {		// '[]'
			if (!cgen(s, ast->lhso, TYPE_ref)) return 0;
			if (!cgen(s, ast->rhso, TYPE_i32)) return 0;
			emit(s, opc_ldc4, argi32(ast->type->size));
			emit(s, opc_madu, noarg);
			if (!ast->type->init) {		// []
				emit(s, opc_ldi4, noarg);
			}
			if ((ret = get) != TYPE_ref) {
				ret = typeId(ast);
				emit(s, opc_ldi, argi32(ast->type->size));
			}
		} goto castret;*/

		case OPER_pls : {
			ret = cgen(s, ast->rhso, ast->cast);
		} break;
		case OPER_not : {		// '!'
			ret = cgen(s, ast->rhso, ast->cast);
			emit(s, opc_not, noarg);
		} break;
		case OPER_mns : {		// '-'
			ret = cgen(s, ast->rhso, ast->cast);
			switch (ret) {
				case TYPE_u32 :
				case TYPE_i32 : emit(s, i32_neg, noarg); break;
				case TYPE_i64 : emit(s, i64_neg, noarg); break;
				case TYPE_f32 : emit(s, f32_neg, noarg); break;
				case TYPE_f64 : emit(s, f64_neg, noarg); break;
				default : debug("ret = %04x:%s", ret, tok_tbl[ret].name); return 0;
			}
		} break;
		case OPER_cmt : {		// '~'
			switch (ret = cgen(s, ast->rhso, ast->cast)) {
				case TYPE_u32 :
				case TYPE_i32 : emit(s, b32_cmt, noarg); break;
				default : 
					fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
					return 0;
			}
		} break;

		case OPER_shl : 		// '>>'
		case OPER_shr : 		// '<<'
		case OPER_and : 		// '&'
		case OPER_ior : 		// '|'
		case OPER_xor : {		// '^'
			ret = ast->cast;
			if (cgen(s, ast->lhso, ret) != ret) {
				fatal(s, "cgen(%k / `%+k`, %s):%s", ast, ast->rhso, tok_tbl[get].name, tok_tbl[ret].name);
				return 0;
			}
			if (cgen(s, ast->rhso, ret) != ret) {
				fatal(s, "cgen(%k / `%+k`, %s):%s", ast, ast->rhso, tok_tbl[get].name, tok_tbl[ret].name);
				return 0;
			}
			switch (ast->kind) {
				case OPER_shl : if (emit(s, opc_shl, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_shr : if (emit(s, opc_shr, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_and : if (emit(s, opc_and, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_xor : if (emit(s, opc_xor, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_ior : if (emit(s, opc_ior, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				default : fatal(s, "cgen(%k):`%+k`", ast, ast->rhso); return 0;
			}// */
		} break;

		case OPER_equ : 		// '=='
		case OPER_neq : 		// '!='
		case OPER_lte : 		// '<'
		case OPER_leq : 		// '<='
		case OPER_gte : 		// '>'
		case OPER_geq : {		// '>='
			ret = ast->cast;
			if (cgen(s, ast->lhso, ret) != ret) {
				fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
				return 0;
			}
			if (cgen(s, ast->rhso, ret) != ret) {
				fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
				return 0;
			}
			switch (ast->kind) {
				case OPER_equ : emit(s, opc_ceq, argi(ast->cast)); break;
				case OPER_lte : emit(s, opc_clt, argi(ast->cast)); break;
				case OPER_leq : emit(s, opc_cle, argi(ast->cast)); break;
				case OPER_neq : emit(s, opc_cne, argi(ast->cast)); break;
				case OPER_gte : emit(s, opc_cgt, argi(ast->cast)); break;
				case OPER_geq : emit(s, opc_cge, argi(ast->cast)); break;
				default : 
					fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
					return 0;
			}// */
			ret = TYPE_bol;
		} break;

		case OPER_add : 		// '+'
		case OPER_sub : 		// '-'
		case OPER_mul : 		// '*'
		case OPER_div : 		// '/'
		case OPER_mod : {		// '%'
			ret = ast->cast;
			if (cgen(s, ast->lhso, ret) != ret) {
				fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
				return 0;
			}
			if (cgen(s, ast->rhso, ret) != ret) {
				fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
				return 0;
			}
			switch (ast->kind) {
				case OPER_add : if (emit(s, opc_add, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_sub : if (emit(s, opc_sub, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_mul : if (emit(s, opc_mul, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_div : if (emit(s, opc_div, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				case OPER_mod : if (emit(s, opc_mod, argi(ret)) < 0) fatal(s, "cgen(%k)", ast); break;
				default : fatal(s, "cgen(%k):`%+k`", ast, ast->rhso); return 0;
			}// */
		} break;

		case ASGN_set : {		// '='
			// TODO: something is wrong: ast->cast should be var->type->kind
			defn var = ast->lhso ? ast->lhso->link : 0;	// lhso always exist ?
			if (var && var->onst) {
				switch (ret = cgen(s, ast->rhso, ast->cast)) {
					case TYPE_u32 :
					case TYPE_i32 :
					case TYPE_f32 : {
						if (get != TYPE_vid && get != EMIT_opc)
							emit(s, opc_dup1, argi32(0));
						emit(s, opc_set1, argidx(s, var->offs));
					} break;
					case TYPE_u64 :
					case TYPE_i64 :
					case TYPE_f64 : {
						if (get != TYPE_vid && get != EMIT_opc)
							emit(s, opc_dup2, argi32(0));
						emit(s, opc_set2, argidx(s, var->offs));
					} break;
					//~ case TYPE_pf2 :
					//~ case TYPE_pf4 : emit(s, opc_set4, argi32(pos+4)); break;
					default : debug("error (%+k):%04x:%s", ast, ret, tok_tbl[ret].name); return 0;
				}
			}
			else if (var) {
				/*if (!cgen(s, ast->lhso, TYPE_ref)) return 0;
				switch (ret = cgen(s, ast->rhso, ast->cast)) {
					case TYPE_u32 :
					case TYPE_i32 :
					case TYPE_f32 : emit(s, opc_sti, argi32(4)); break;
					case TYPE_u64 :
					case TYPE_i64 :
					case TYPE_f64 : emit(s, opc_sti, argi32(8)); break;
						//~ emit(s, opc_sti, argi32(ast->type->size)); break;
					default : debug("error (%+k):%04x:%s", ast, tmp, tok_tbl[tmp].name); return -1;
				}
				//~ cgen(s, ast->lhso, TYPE_ref);
				//~ cgen(s, ast->rhso, TYPE_i32);
				emit(s, opc_ldc4, argi32(ast->type->size));
				emit(s, opc_madu, noarg);
				if (get != TYPE_ref) {
					emit(s, opc_ldi, argi32(ast->type->size));
				}*/
				debug("unimpl '%+k'(%T)", ast, var);
				return 0;
			}
			else {
				fatal(s, "cgen(%k):`%+k`", ast, ast->rhso);
				return 0;
			}
		} break;
		//}

		//{ TVAL
		case CNST_int : switch (get) {
			default : debug("error %04x:%s", get, tok_tbl[get].name); return 0;
			case TYPE_vid : return TYPE_vid;
			case TYPE_any :
			case TYPE_u08 :
			case TYPE_i08 :
			case TYPE_u16 :
			case TYPE_i16 :
			case TYPE_u32 :
			//~ case TYPE_enu : 
			case TYPE_i32 : emit(s, opc_ldc4, argi32((int32t)ast->cint)); return get;
			case TYPE_i64 : emit(s, opc_ldc8, argi64((int64t)ast->cint)); return get;
			case TYPE_f32 : emit(s, opc_ldcf, argf32((flt32t)ast->cint)); return get;
			case TYPE_f64 : emit(s, opc_ldcF, argf64((flt64t)ast->cint)); return get;
				/*if (ast->type && ast->type->kind == TYPE_enu) {
					emit(s, opc_ldc4, argi32((int32t)ast->cint)); return get;
					break;
				}// */
				//~ return 0;
		} return 0;
		case CNST_flt : switch (get) {
			default : debug("error %04x:%s %+k", get, tok_tbl[get].name, ast); break;
			case TYPE_vid : return TYPE_vid;
			case TYPE_u32 :
			case TYPE_i32 : emit(s, opc_ldc4, argi32((int32t)ast->cflt)); return TYPE_i32;
			case TYPE_i64 : emit(s, opc_ldc8, argi64((int64t)ast->cflt)); return TYPE_i64;
			//~ case TYPE_any :
			case TYPE_f32 : emit(s, opc_ldcf, argf32(ast->cflt)); return TYPE_f32;
			case TYPE_f64 : emit(s, opc_ldcF, argf64(ast->cflt)); return TYPE_f64;
		} return 0;
		case CNST_str : switch (get) {
			default : debug("error %04x:%s %+k", get, tok_tbl[get].name, ast); break;
			case TYPE_vid : return TYPE_vid;
			//~ case TYPE_ref : emit(s, opc_ldc4, argi32(0)); return TYPE_ref;
			//~ case TYPE_ptr : emit(s, opc_ldc4, argi32(0)); return TYPE_ptr;
			//~ case TYPE_u32 :
			//~ case TYPE_i32 : emit(s, opc_ldc4, argi32(ast->cflt)); return TYPE_i32;
			//~ case TYPE_i64 : emit(s, opc_ldc8, argi32(ast->cflt)); return TYPE_i64;
			//~ case TYPE_f32 : emit(s, opc_ldcf, argf32(ast->cflt)); return TYPE_f32;
			//~ case TYPE_f64 : emit(s, opc_ldcF, argf64(ast->cflt)); return TYPE_f64;
		} return 0;

		case TYPE_ref : {
			defn typ = ast->type;		// type
			defn var = ast->link;		// link
			ret = typ ? typ->kind : 0;
			if (ast->stmt) {			// lnk
				struct astn_t tmp, *ref = ast->stmt;
				if (eval(&tmp, ref, 0)) ref = &tmp;
				return cgen(s, ref, get);
			}
			else if (var && var->offs == 0) {	// new
				//~ debug("cgen(TYPE_ref:'new %T'):%T", var, typ);
				node val = var->init;
				switch (ret = typ->kind) {
					case TYPE_u32 :
					case TYPE_i32 :
					case TYPE_f32 : {
						var->onst = 1;// s->level > 1;
						if (val) cgen(s, val, ret);
						else emit(s, opc_ldz1, noarg);
						var->offs = s->code.rets;
					} return ret;
					case TYPE_i64 :
					case TYPE_f64 : {
						var->onst = 1;
						if (val) cgen(s, val, ret);
						else emit(s, opc_ldz2, noarg);
						var->offs = s->code.rets;
					} break;
					/*
					//~ case TYPE_ptr :
					case TYPE_arr : {	// malloc();
						//~ debug("unimpl ptr or arr %+k %+T", ast, typ);
						emit(s, opc_ldcr, argi32(0xff00ff));
						//~ var->onst = 1;
						//~ if (val) cgen(s, val, typ->kind);
						//~ else emit(s, opc_ldz1, noarg);
						//~ var->offs = s->code.rets;
					} break;
					//~ case TYPE_rec :
					//~ case TYPE_enu :	// malloc(); || salloc();
					//~ */
					default : 
						debug("unimpl %08x[%k]:%T", typ->kind, ast, typ);
						//~ debug("unimpl%T", typ->kind, ast, typ);
						break;
				}
			}
			else if (var && var->onst) {		// local
				switch (typeId(ast)) {
					default : debug("error %04x:%s", get, tok_tbl[get].name); break;
					case TYPE_u32 : 
					case TYPE_i32 : 
					case TYPE_f32 : emit(s, opc_dup1, argidx(s, var->offs)); break;
					case TYPE_u64 : 
					case TYPE_i64 : 
					case TYPE_f64 : emit(s, opc_dup2, argidx(s, var->offs)); break;
				}
			}
			else if (var) {				// global
				debug("unimpl"); return -1;
				//~ emit(s, opc_ldcr, argi32(ast->link->offs));
				//~ if (get == TYPE_ref) ret = TYPE_ref;
				//~ else emit(s, opc_ldi, argi32(ast->type->size));
			}
			trace(0, "cgen(TYPE_ref:'%T':%b@%x):%T", var, var->onst, var->offs, typ);
		} break;
		//}

		default: fatal(s, "Node(%k)", ast); return 0;

		case CODE_inf : {	// TODO remove_me
			ast->cinf.spos = s->code.rets;
			ast->cinf.cbeg = s->code.memp + s->code.cs;
			cgen(s, ast->cinf.stmt, get);
			ast->cinf.cend = s->code.memp + s->code.cs;
			ast = ast->next;
		} return 0;
	}
	if (get && get != ret) switch (get) {
		//~ default : fixt(s, get, ret); break;
		case EMIT_opc : return ret;
		case TYPE_vid : if (sc != s->code.rets) {
			fatal(s, "invalid stack size : %d tok: '%k' in '%+k'", s->code.rets - sc, ast, ast);
		} return TYPE_vid;
		/*case TYPE_bol : switch (ret) {
			case TYPE_u32 : break;
			case TYPE_i32 : break;
			case TYPE_f32 : {
				emit(s, f32_tob, noarg);
			} break;
			case TYPE_f64 : {
				emit(s, f64_not, noarg);
				emit(s, opc_not, noarg);
			} break;
			default : goto errorcast;
		} return get;	// */// conversion success
		case TYPE_u32 : switch (ret) {
			case TYPE_bol : break;
			//~ case TYPE_u32 : break;	// cannot happen : get == ret
			case TYPE_i32 : break;
			case TYPE_f32 : emit(s, f32_i32, noarg); break;
			case TYPE_f64 : emit(s, f64_i32, noarg); break;
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_i32 : switch (ret) {
			case TYPE_bol : break;
			case TYPE_u32 : break;
			//~ case TYPE_i32 : break;	// cannot happen : get == ret
			case TYPE_i64 : emit(s, i64_i32, noarg); break;
			case TYPE_f32 : emit(s, f32_i32, noarg); break;
			case TYPE_f64 : emit(s, f64_i32, noarg); break;
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_i64 : switch (ret) {
			case TYPE_bol : emit(s, i32_i64, noarg); break;
			case TYPE_u32 : emit(s, i32_i64, noarg); break;
			case TYPE_i32 : emit(s, i32_i64, noarg); break;
			case TYPE_f32 : emit(s, f32_i64, noarg); break;
			case TYPE_f64 : emit(s, f64_i64, noarg); break;
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_f32 : switch (ret) {
			case TYPE_bol :
			case TYPE_u32 :
			case TYPE_i32 : emit(s, i32_f32, noarg); break;
			case TYPE_f64 : emit(s, f64_f32, noarg); break;
			//~ case TYPE_flt : break;	// cannot happen : get == ret
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_f64 : switch (ret) {
			case TYPE_bol :
			case TYPE_u32 :
			case TYPE_i32 : emit(s, i32_f64, noarg); break;
			case TYPE_f32 : emit(s, f32_f64, noarg); break;
			//~ case TYPE_f64 : break;	// cannot happen : get == ret
			default : goto errorcast;
		} return get;	// conversion success
		case TYPE_ref : switch (ret) {	// ... if we neer a reference: array, pointer, ... is ok
			case TYPE_arr : break;
			default : goto errorcast;
		} return get;	// conversion success
		default : fatal(s, "cgen(`%+k`, %s):%s", ast, tok_tbl[get].name, tok_tbl[ret].name);
		errorcast: debug("cgen: invalid cast :[%d->%d](%?s to %?s) '%+k'", ret, get, tok_tbl[ret].name, tok_tbl[get].name, ast);
	}
	//~ if (ast) info(s, s->file, ast ? ast->line:0, "cgen(`%+k`, %s):%s", ast, tok_tbl[get&15].name, tok_tbl[ret&15].name);
	//~ trace(32, "leave:cgen('%k')", ast);
	return ret;
}

//~ int ngen(state s, ???);

static cell vm_task(cell proc, int cl, int dl, int ss) {
	// find an empty cpu
	cell pu = proc;
	while (pu && pu->ip)
		pu = pu->next;
	if (pu != NULL) {
		// set ip and copy stack
		pu->ip = proc->ip;
		pu->sp = pu->bp + ss - cl;
		memcpy(pu->sp, proc->sp, cl * 4);
	}
	return pu;
}

/**
 * @arg env : enviroment
 * @arg cc  : cell count
 * @arg ss  : stack size
 * @return  : error code
**/
int vm_exec(state env, unsigned cc, unsigned ss, int dbg) {
	//~ struct vm_cpu_t proc[32], *pu = proc;	// execution units
	//~ unsigned long csiz = env->code.cs;		// code size (stop)
	//~ unsigned long dsiz = env->code.ds;
	//~ unsigned long ssiz = env->code.rets;
	cell pu;
	unsigned long size = env->code.mems;
	unsigned char *mem = env->code.memp;
	unsigned char *end = mem + size;

	register bcde ip, li = 0, bi = 0;
	//~ register unsigned char *sptr;//, *mp;
	unsigned i = 0;

	if (env->errc) {
		//~ fputmsg(stderr, "errors\n");
		return -1;
	}

	if (cc > 32) {
		error(env, "VM_ERR", 0, "cell overrun\n");
		return -1;
	}

	if (env->code.mins * 4 >= ss) {
		error(env, "VM_ERR", 0, "stack overrun\n");
		return -1;
	}

	if ((env->code.cs + env->code.ds + ss * cc) >= size) {
		error(env, "VM_ERR", 0, "memory overrun\n");
		return -1;
	}

	env->code.pu = 0;
	for (i = 0; i < cc; i++) {
		end -= sizeof(struct cell_t) + ss;
		pu = (cell)end;
		pu->next = env->code.pu;
		env->code.pu = pu;
		//~ pu->zf = 0;
		//~ pu->sf = 0;
		//~ pu->cf = 0;
		//~ pu->of = 0;
		pu->sp = pu->bp + ss;
		pu->ip = 0;
	}

	pu->sp = pu->bp + ss;
	pu->ip = mem;//env->code.ip;		// start firt processor

	bi = li = (bcde)(mem + env->code.cs);

	env->time = clock();
	for (env->code.ic = i = 0; ; env->code.ic += 1) {//for (i = 0 ; i < cc; i++) {
		//~ pu = env->code.cell;		// get proc (should be in a queue)
		register unsigned char *sptr = pu->sp;			// stack
		//~ register unsigned char *sptop = pu->sp;			// stack
		//~ register unsigned char *spres = pu->sp;			// stack
		ip = (bcde)pu->ip;		// get instr

		if (ip && ip == bi) {			// break
			if (ip == li) break;	// stop
			//~ next brp
			//~ debug
		}
		switch (ip->opc) {		// exec
			error_opc : error(env, "VM_ERR", 0, "invalid opcode:[%02x]", ip->opc); return -1;
			error_ovf : error(env, "VM_ERR", 0, "stack overflow :op[%A]", ip); return -2;
			error_div : error(env, "VM_DBG", 0, "division by zero :op[%A]", ip); break;
			//~ error_mem : error("VM_ERR : %s\n", "segmentatin fault"); return -5;
			//~ error_stc : error("VM_ERR : %s\n", "stack underflow"); return -3;
			//~ debug_ovf : error("\nVM_ERR : %s", "overflow"); break;
			#define CDBG
			#define FLGS	// 
			#define NEXT(__IP, __SP, __CHK) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			//~ #define MEMP(__MPTR) (mp = (__MPTR))
			#define STOP(__ERR) goto __ERR
			#define EXEC
			#include "incl/exec.inl"
		}
		//~ donepu(pu);
		if (dbg > 0) {				// debug
			int stki, stkn = (pu->bp + ss - pu->sp);
			//~ stkval* stop = (stkval*)(pu->sp);
			/*switch (ip->opc & 0xf0) {
				stkval* sp = (stkval*)pu->sp;
				//~ case 0x00 : // sys
				//~ case 0x10 : // stk
				//~ case 0x20 : // mem
				case 0x30 : // u32
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%u}\n", ssiz, i, ip, sp->u4);
					break;
				case 0x40 : // i32
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%d}\n", ssiz, i, ip, sp->i4);
					break;
				case 0x50 : // f32
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%f}\n", ssiz, i, ip, sp->f4);
					break;
				case 0x60 : // i64
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%D}\n", ssiz, i, ip, sp->i8);
					break;
				case 0x70 : // f64
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%F}\n", ssiz, i, ip, sp->f8);
					break;
				case 0x80 : // p4f
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%g, %g, %g, %g}\n", ssiz, i, ip, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w);
					break;
				case 0x90 : // p4d
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%G, %G}\n", ssiz, i, ip, sp->pd.x, sp->pd.y);
					break;
				default :
					fputmsg(stdout, ">exec: ss:[%03d] pu:(%02d) : %A\t {%d, %D, %g, %G, (%g, %g, %g, %g), (%G, %G)}\n", ssiz, i, ip, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
			}// */
			//~ fputmsg(stdout, "exec: ss:[%03d] pu:(%02d) : %A\n", ssiz, i, ip);
			//~ fputmsg(stdout, ">exec:pu%02d:[ss:%03d zf:%d sf:%d cf:%d of:%d]: %A\n", i, ssiz, pu->zf, pu->sf, pu->cf, pu->of, ip);
			//~ fputmsg(stdout, ">exec:pu%02d:[ss:%03d, zf:%d sf:%d cf:%d of:%d]: %A\n", i, ssiz, pu->zf, pu->sf, pu->cf, pu->of, ip);
			if (dbg == 7) {
				stkval* stkv = (stkval*)pu->sp;
				fputmsg(stdout, ">exec:pu%02d:[ss:%03d]:int(%d) %A\n", i, stkn/4, stkv->i4, ip);
			}
			else {
				fputmsg(stdout, ">exec:pu%02d:[ss:%03d]: %A\n", i, stkn/4, ip);
				if (dbg > 1) for (stki = 0; stki < stkn; stki += 4) {
					stkval* stkv = (stkval*)(pu->sp + stki);
					fputmsg(stdout, "\tsp(%03d): {int32t(%d), flt32(%g), int64t(%D), flt64(%G)}\n", stki/4, stkv->i4, stkv->f4, stkv->i8, stkv->f8);
					//~ fputmsg(stdout, "\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G), p4f32(%g, %g, %g, %g), p4f64(%G, %G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
					//~ fputmsg(stdout, "!\tsp(%03d): {int32t(%d), int64t(%D), flt32(%g), flt64(%G)}\n", ssiz, sp->i4, sp->i8, sp->f4, sp->f8);
					//~ sp = (stkval*)(((char*)sp) + 4);
				}
			}
			//~ */
			fflush(stdout);
		}
		//~ #endif
	}

	env->time = clock() - env->time;
	return 0;
}
//}
//{ core.c ---------------------------------------------------------------------

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

int cc_outf(state s, char* file) {
	if (s->logf)
		fclose(s->logf);
	s->logf = 0;
	if (file) {
		s->logf = fopen(file, "wb");
		if (!s->logf) return -1;
	}
	return 0;
}

void instint(state s, defn it, int64t sgn) {
	if (it != NULL) {
		/*
		int size = it->size;
		int bits = size * 8;
		int64t mask = (-1LLU >> -bits);
		int64t minv = sgn << (bits - 1);
		int64t maxv = (minv - 1) & mask;
		enter(s, 0 ,0);
		install(s, "min", 0, CNST_int, it)->init = intnode(s, minv);
		install(s, "max", 0, CNST_int, it)->init = intnode(s, maxv);
		install(s, "mask", 0, CNST_int, it)->init = intnode(s, mask);
		install(s, "bits", 0, CNST_int, it)->init = intnode(s, bits);
		install(s, "size", 0, CNST_int, it)->init = intnode(s, size);
		leave(s, 3, &it->args);
		//~ */
	}
}

// replace with install(...)
defn instlibc(state s, char* name, unsigned size, int kind, defn type) {
	int argsize = 0;
	node ftag = 0;
	defn args = 0, ftyp = 0;
	s->_ptr = name;
	s->_cnt = strlen(name);
	s->_tok = 0;
	if (!(ftag = next(s, TYPE_ref))) return 0;	// int ...
	if (!(ftyp = lookup(s, ftag, 0))) return 0;
	if (!istype(ftag)) return 0;

	//~ ret_ty = qual(s, 0);

	if (!(ftag = next(s, TYPE_ref))) return 0;	// int bsr ...

	if (skip(s, PNCT_lp)) {				// int bsr ( ...
		if (!skip(s, PNCT_rp)) {		// int bsr ( )
			enter(s, ftyp);
			while (peek(s)) {
				node atag = 0;
				defn atyp = 0;

				if (!(atag = next(s, TYPE_ref))) break;	// int
				if (!(atyp = lookup(s, atag, 0))) break;
				if (!istype(atag)) break;

				//~ arg_ty = qual(s, 0);

				if (!(atag = next(s, TYPE_ref)))
					atag = newnode(s, TYPE_ref);

				declare(s, TYPE_ref, atag, atyp, 0);
				argsize += atyp->size;

				if (skip(s, PNCT_rp)) break;
				if (!skip(s, OPER_com)) break;
			}
			leave(s, 3, &args);
		}
	}
	if (!peek(s)) {
		defn def = declare(s, TYPE_fnc, ftag, ftyp, args);
		def->size = argsize;
		return def;
	}
	return NULL;
}// */

void cc_initVM(state s) {
	defn opc, module;
	enter(s, module = install(s, "vm", 0, TYPE_enu));	// not vm, something else

	enter(s, opc = install(s, "add", 0, TYPE_enu));
	install(s, "i32", opc_tbl[i32_add].size, CNST_int)->init = intnode(s, i32_add);
	install(s, "i64", opc_tbl[i64_add].size, CNST_int)->init = intnode(s, i64_add);
	install(s, "f32", opc_tbl[f32_add].size, CNST_int)->init = intnode(s, f32_add);
	install(s, "f64", opc_tbl[f64_add].size, CNST_int)->init = intnode(s, f64_add);
	install(s, "p4f", opc_tbl[p4f_add].size, CNST_int)->init = intnode(s, p4f_add);
	install(s, "p4d", opc_tbl[p4d_add].size, CNST_int)->init = intnode(s, p4d_add);
	leave(s, 3, &opc->args);

	//~ leave(s, 3, &opt->args);
	install(s, "emit", 0, TYPE_def);// warn(int, void ...);
	install(s, "warn", 0, TYPE_def);// warn(int, string);
	//~ install(s, "line", 0, CNST_int, type_int);// current line;
	//~ install(s, "file", 0, CNST_str, type_str);// current file;
	//~ install(s, "time", 0, CNST_str, type_str);// compile time;
	//~ install(s, "type", 0, CNST_str, type_str);// current type;
	leave(s, 3, &module->args);
}

void cc_init00000(state s) {
	defn opc, module, typ;
	//~ How it should be
	//{ install typenames
	defn type_bol = 0, type_bit = 0;
	defn type_i08 = 0, type_i16 = 0;
	defn type_u08 = 0, type_u16 = 0;
	defn type_f16 = 0, type_u64 = 0;
	enum { TYPE_bit = 2, TYPE_uns = 3, TYPE_int = 4, TYPE_flt = 5};

	type_vid = install(s, "void",  0, TYPE_vid);

	type_bol = install(s, "bool",  1, TYPE_bit);
	type_bit = install(s, "bit",  -1, TYPE_bit);
	//+ -> bitfields <=> array of bits ???

	type_u08 = install(s, "uns08", 1, TYPE_uns);
	type_u16 = install(s, "uns16", 2, TYPE_uns);
	type_u32 = install(s, "uns32", 4, TYPE_uns);
	type_u64 = install(s, "uns64", 8, TYPE_uns);

	type_i08 = install(s, "int08", 1, TYPE_int);
	type_i16 = install(s, "int16", 2, TYPE_int);
	type_i32 = install(s, "int32", 4, TYPE_int);
	type_i64 = install(s, "int64", 8, TYPE_int);

	type_f16 = install(s, "flt16", 2, TYPE_flt);
	type_f32 = install(s, "flt32", 4, TYPE_flt);
	type_f64 = install(s, "flt64", 8, TYPE_flt);

	//~ type_ptr = install(s, "pointer", 0, TYPE_ptr);
	type_str = install(s, "string", 0, TYPE_ptr);

	//} types
	//{ install vmopcodes
	typ = type_u08;
	enter(s, module = install(s, "vm", 0, TYPE_enu));	// not vm, something else
	enter(s, opc = install(s, "add", 1, TYPE_enu));
	typ = install(s, "i32", 1, TYPE_enu);
	typ->size = opc_tbl[typ->offs = i32_add].size;
	typ = install(s, "i64", 1, TYPE_enu);
	typ->size = opc_tbl[typ->offs = i64_add].size;
	typ = install(s, "p4d", 1, TYPE_enu);
	typ->size = opc_tbl[typ->offs = p4d_add].size;

	//~ install(s, "i64", opc_tbl[i64_add].size, CNST_int)->init = intnode(s, i64_add);
	//~ install(s, "f32", opc_tbl[f32_add].size, CNST_int)->init = intnode(s, f32_add);
	//~ install(s, "f64", opc_tbl[f64_add].size, CNST_int)->init = intnode(s, f64_add);
	//~ install(s, "p4f", opc_tbl[p4f_add].size, CNST_int)->init = intnode(s, p4f_add);
	//~ install(s, "p4d", opc_tbl[p4d_add].size, CNST_int)->init = intnode(s, p4d_add);
	leave(s, 3, &opc->args);

	//~ install(s, "emit", 0, TYPE_def);// warn(int, void ...);
	//~ install(s, "warn", 0, TYPE_def);// warn(int, string);
	//~ install(s, "line", 0, CNST_int);// current line;
	//~ install(s, "file", 0, CNST_str);// current file;
	//~ install(s, "time", 0, CNST_str);// compile time;
	//~ install(s, "type", 0, CNST_str);// current type;
	leave(s, 3, &module->args);
	//}
	//{ install operators
	install(s, "int32 add(int32 a, uns32 b) asm {emit(add.i32, int32(a), int32(b));}", 0, 0);
	install(s, "int64 add(int32 a, uns64 b) asm {emit(add.i64, int64(a), int64(b));}", 0, 0);
	install(s, "int32 add(int32 a, int32 b) asm {emit(add.i32, int32(a), int32(b));}", 0, 0);
	install(s, "int64 add(int32 a, int64 b) asm {emit(add.i64, int64(a), int64(b));}", 0, 0);
	install(s, "flt32 add(int32 a, flt32 b) asm {emit(add.f32, flt32(a), flt32(b));}", 0, 0);
	install(s, "flt64 add(int32 a, flt64 b) asm {emit(add.f64, flt64(a), flt64(b));}", 0, 0);
	install(s, "int32 add(out int32 a, uns32 b) {return a = add(a, b)}", 0, 0);
	install(s, "int64 add(out int32 a, uns64 b) {return a = add(a, b)}", 0, 0);
	install(s, "int32 add(out int32 a, int32 b) {return a = add(a, b)}", 0, 0);
	install(s, "int64 add(out int32 a, int64 b) {return a = add(a, b)}", 0, 0);
	install(s, "flt32 add(out int32 a, flt32 b) {return a = add(a, b)}", 0, 0);
	install(s, "flt64 add(out int32 a, flt64 b) {return a = add(a, b)}", 0, 0);

	install(s, "int32 add(int32 a, uns32 b) asm {emit(add.i32, int32(a), int32(b));}", 0, 0);	// +
	install(s, "int32 sub(int32 a, uns32 b) asm {emit(sub.i32, int32(a), int32(b));}", 0, 0);	// -
	install(s, "int32 mul(int32 a, uns32 b) asm {emit(mul.i32, int32(a), int32(b));}", 0, 0);	// *
	install(s, "int32 div(int32 a, uns32 b) asm {emit(div.i32, int32(a), int32(b));}", 0, 0);	// /
	install(s, "int32 mod(int32 a, uns32 b) asm {emit(mod.i32, int32(a), int32(b));}", 0, 0);	// %

	install(s, "int32 ceq(int32 a, uns32 b) asm {emit(ceq.b32, int32(a), int32(b));}", 0, 0);	// ==
	install(s, "int32 cne(int32 a, uns32 b) asm {emit(cne.b32, int32(a), int32(b));}", 0, 0);	// !=
	install(s, "int32 clt(int32 a, uns32 b) asm {emit(clt.i32, int32(a), int32(b));}", 0, 0);	// <
	install(s, "int32 cle(int32 a, uns32 b) asm {emit(cle.i32, int32(a), int32(b));}", 0, 0);	// <=
	install(s, "int32 cgt(int32 a, uns32 b) asm {emit(cgt.i32, int32(a), int32(b));}", 0, 0);	// >
	install(s, "int32 cge(int32 a, uns32 b) asm {emit(cge.i32, int32(a), int32(b));}", 0, 0);	// >=

	install(s, "int32 shr(int32 a, uns32 b) asm {emit(shl.b32, int32(a), int32(b));}", 0, 0);	// >>
	install(s, "int32 shl(int32 a, uns32 b) asm {emit(sar.b32, int32(a), int32(b));}", 0, 0);	// <<

	install(s, "int32 shl(int32 a, uns32 b) asm {emit(and.b32, int32(a), int32(b));}", 0, 0);	// &
	install(s, "int32 shl(int32 a, uns32 b) asm {emit( or.b32, int32(a), int32(b));}", 0, 0);	// |
	install(s, "int32 shl(int32 a, uns32 b) asm {emit(xor.b32, int32(a), int32(b));}", 0, 0);	// ^

	install(s, "int32 pls(int32 a) asm {emit(nop, int32(a));}", 0, 0);	// +
	install(s, "int32 neg(int32 a) asm {emit(neg.i32, int32(a));}", 0, 0);	// -
	install(s, "int32 cmt(int32 a) asm {emit(cmt.b32, int32(a));}", 0, 0);	// ~
	install(s, "int32 not(int32 a) asm {emit(ceq.i32, int32(a), int32(0));}", 0, 0);	// !

	//}
}

int cc_init(state s) {
	s->warn = wl;
	s->copt = ol;
	s->errc = 0;

	s->logf = 0;
	s->file = 0;
	s->line = 0;
	s->nest = 0;
	s->_fin = -1;
	s->_chr = -1;
	s->_ptr = 0;
	s->_cnt = 0;

	s->buffp = s->buffer;

	s->root = 0;
	s->defs = 0;
	s->deft = cc_calloc(s, sizeof(defn*) * TBLS);
	s->strt = cc_calloc(s, sizeof(list*) * TBLS);

	{	// install base types
		const int sgnd = -1;
		const int unsd = 0;
		//~ defn tmp;
		defn type_i08 = 0, type_i16 = 0;
		defn type_u08 = 0, type_u16 = 0;

		type_vid = install(s, "void",  0, TYPE_vid);
		type_u08 = install(s, "uns08", 1, TYPE_u08);
		type_u16 = install(s, "uns16", 2, TYPE_u16);
		type_u32 = install(s, "uns32", 4, TYPE_u32);
		//+ type = install(s, "uns64", 2, TYPE_u64);
		type_i08 = install(s, "int08", 1, TYPE_i08);
		type_i16 = install(s, "int16", 2, TYPE_i16);
		type_i32 = install(s, "int32", 4, TYPE_i32);
		type_i64 = install(s, "int64", 8, TYPE_i64);
		//+ type = install(s, "flt16", 2, TYPE_f16);
		type_f32 = install(s, "flt32", 4, TYPE_f32);
		type_f64 = install(s, "flt64", 8, TYPE_f64);

		instint(s, type_i08, sgnd);
		instint(s, type_i16, sgnd);
		instint(s, type_i32, sgnd);
		instint(s, type_i64, sgnd);

		instint(s, type_u08, unsd);
		instint(s, type_u16, unsd);
		instint(s, type_u32, unsd);

		install(s, "int", 0, TYPE_def)->type = type_i32;
		install(s, "long", 0, TYPE_def)->type = type_i64;
		install(s, "float", 0, TYPE_def)->type = type_f32;
		install(s, "double", 0, TYPE_def)->type = type_f64;

		type_str = install(s, "string", 0, TYPE_def);

		/*tmp = install(s, "rgb", 4, TYPE_u32);
		enter(s, 0 ,0);
		install(s, "red", 0, CNST_int)->init = intnode(s, 0xffFF0000);
		install(s, "green", 0, CNST_int)->init = intnode(s, 0xff00FF00);
		install(s, "blue", 0, CNST_int)->init = intnode(s, 0xff0000FF);
		//install(s, "cian", 0, CNST_int)->init = intnode(s, 0xff0000FF);
		install(s, "magenta", 0, CNST_int)->init = intnode(s, 0xffFF00FF);
		//install(s, "yelow", 0, CNST_int)->init = intnode(s, 0xff00FFFF);
		leave(s, 3, &tmp->args);*/

	}
	{	// install Math const
		int i;
		//~ node nan = fh8node(s, 0xfff8000000000000LL); //fltnode(s, 0./0);
		//~ node inf = fh8node(s, 0x7ff0000000000000LL); //fltnode(s, 1./0);

		defn def;
		enter(s, def = install(s, "os", 0, TYPE_enu));
			install(s,  "host", 5, CNST_str)->init = strnode(s, (char*)os);
			//~ install(s,  "name",  5, CNST_str)->init = strnode(s, (char*)os);
			install(s,  "bits", 32, CNST_int)->init = intnode(s, 32);
		leave(s, 3, &def->args);

		enter(s, def = install(s, "math", 0, TYPE_enu));

		//~ install(s,"qnan", 0, CNST_flt)->init = fh8node(s, 0x7FFFFFFFFFFFFFFFLL);
		install(s, "nan", 0, CNST_flt)->init = fh8node(s, 0xfff8000000000000LL);
		install(s, "inf", 0, CNST_flt)->init = fh8node(s, 0x7ff0000000000000LL);
		install(s, "l2e", 0, CNST_flt)->init = fh8node(s, 0x3FF71547652B82FELL);	// log_2(e)
		install(s, "l2t", 0, CNST_flt)->init = fh8node(s, 0x400A934F0979A371LL);	// log_2(10)
		install(s, "lg2", 0, CNST_flt)->init = fh8node(s, 0x3FD34413509F79FFLL);	// log_10(2)
		install(s, "ln2", 0, CNST_flt)->init = fh8node(s, 0x3FE62E42FEFA39EFLL);	// log_e(2)
		install(s,  "pi", 0, CNST_flt)->init = fh8node(s, 0x400921fb54442d18LL);	// 3.1415...
		install(s,   "e", 0, CNST_flt)->init = fh8node(s, 0x4005bf0a8b145769LL);	// 2.7182...

		leave(s, 3, &def->args);

		enter(s, def = install(s, "libc", 0, TYPE_enu));
		for (i = 0; i < sizeof(libcfnc) / sizeof(*libcfnc); i += 1) {
			defn fun = instlibc(s, libcfnc[i].proto,0, 0, 0);
			libcfnc[i].sym = fun;
			libcfnc[i].sym->offs = i;
			libcfnc[i].sym->libc = 1;
			//libcfnc[i].arg = argsize;
			libcfnc[i].ret = fun->type->size;
			libcfnc[i].pop = fun->size;
			//~ cc_libc(s, &libcfnc[i], i);
		}
		leave(s, 3, &def->args);
	}// */
	{
		//{ install vmopcodes
		defn opc, typ, type_p4f;
		type_p4f = install(s, "p4f32", 128, TYPE_enu);
		
		enter(s, opc = install(s, "add", 1, TYPE_enu));
		typ = install(s, "i32", 1, EMIT_opc);
		typ->size = opc_tbl[i32_add].size;
		typ->type = type_i32;
		typ->offs = i32_add;
		typ = install(s, "f64", 1, EMIT_opc);
		typ->size = opc_tbl[i32_add].size;
		typ->type = type_f64;
		typ->offs = f64_add;
		typ = install(s, "p4d", 1, EMIT_opc);
		typ->size = opc_tbl[p4d_add].size;
		typ->type = type_p4f;
		typ->offs = p4d_add;
		leave(s, 3, &opc->args);

		enter(s, opc = install(s, "sub", 1, TYPE_enu));
		typ = install(s, "i32", 1, EMIT_opc);
		typ->size = opc_tbl[i32_sub].size;
		typ->type = type_i32;
		typ->offs = i32_sub;
		typ = install(s, "f64", 1, EMIT_opc);
		typ->size = opc_tbl[i32_sub].size;
		typ->type = type_f64;
		typ->offs = f64_sub;
		typ = install(s, "p4d", 1, EMIT_opc);
		typ->size = opc_tbl[p4d_sub].size;
		typ->type = type_p4f;
		typ->offs = p4d_sub;
		leave(s, 3, &opc->args);

		enter(s, opc = install(s, "mul", 1, TYPE_enu));
		typ = install(s, "i32", 1, EMIT_opc);
		typ->size = opc_tbl[i32_mul].size;
		typ->type = type_i32;
		typ->offs = i32_mul;
		typ = install(s, "p4d", 1, EMIT_opc);
		typ->size = opc_tbl[p4d_mul].size;
		typ->type = type_p4f;
		typ->offs = p4d_mul;
		leave(s, 3, &opc->args);

		//~ install(s, "emit", 0, TYPE_def);// warn(int, void ...);
		//~ install(s, "warn", 0, TYPE_def);// warn(int, string);
		//~ install(s, "line", 0, CNST_int);// current line;
		//~ install(s, "file", 0, CNST_str);// current file;
		//~ install(s, "time", 0, CNST_str);// compile time;
		//~ install(s, "type", 0, CNST_str);// current type;
		//~ leave(s, 3, &type_opc->args);
		//}
	}
	//~ /*
	/*if (file) {
		char* path = realpath(file, NULL);
		if (path) {
			char* name = strrchr(path, '/');
			if (name > path) {
				*name++ = 0;
				s->binn = lookupstr(s, name, strlen(name), rehash(name, strlen(name)));
				s->binp = lookupstr(s, path, strlen(path), rehash(path, strlen(name)));
				//~ debug("path = '%s' name = '%s'\n", path, name);
				//~ debug("cwd : %s\n", getcwd(buff, sizeof(buff)));
				//~ debug("env : %s\n", getenv("PATH"));
			}
			free(path);
		}
	}// */
	//~ cc_initVM(s);
	return 0;
}

int cc_file(state s, int mode, char *file) {
	if (cc_srcf(s, file)) return -1;
	s->time = clock();
	s->root = scan(s, mode);
	cc_srcf(s, NULL);
	s->time = clock() - s->time;
	return s->errc;
}

int cc_buff(state s, int mode, char *file, int line, char *buff) {
	cc_srcf(s, NULL);
	s->_ptr = buff;
	s->_cnt = strlen(buff);

	s->file = file;
	s->line = line;
	//~ s->file = 0;
	//~ s->line = 0;

	s->root = scan(s, mode);

	return s->errc;
}

int vm_cgen(state s) {
	if (s->errc) return -1;

	s->time = clock();
	s->code.memp = (unsigned char*)s->buffp;
	s->code.mems = bufmaxlen - (s->buffp - s->buffer);

	s->code.ds = s->code.cs = s->code.pc = 0;
	s->code.rets = s->code.mins = 0;
	s->code.ic = 0;

	if (!s->root)
		warn(s, 0, s->file, 1, "no data");
	cgen(s, s->root, TYPE_any);
	s->time = clock() - s->time;
	//~ dumpasm(stdout, s->code.ip, 0);
	return s->errc;
}

int cc_done(state s) {
	if (s->nest)
		error(s, s->file, s->line, "premature end of file");
	cc_srcf(s, NULL);
	cc_outf(s, NULL);
	return s->errc;
}

/** print symbols
 * output looks like:
 * file:line:kind:?prot:name:type
 * file: is the file name of decl
 * line: is the line number of decl
 * kind:
 * 	'@' : typename(dcl)
 * 	'#' : constant(def)
 * 	'$' : variable(ref)
 *+	'&' : function?
 *+	'*' : operator?
 *+prot: protection level
 * 	'+' : public
 * 	'-' : private
 * 	'=' : protected?
 * 
**/
int psym(state s, defn loc, int mode, FILE *fout) {
	int tch, line;
	//~ FILE *fout = stdout;
	defn ptr, bs[TOKS], *ss = bs;
	defn *tp, bp[TOKS], *sp = bp;
	for (*sp = loc; sp >= bp;) {
		if (!(ptr = *sp)) {
			--sp, --ss;
			continue;
		}
		*sp = ptr->next;
		if (ptr->link) {
			line = ptr->link->line;
		} else line = 0;

		//~ if (!line) continue;
		switch (ptr->kind) {

			// constant
			case CNST_int :
			case CNST_flt :
			case CNST_str : tch = '#'; break;

			// variable/function
			case TYPE_fnc :
			case TYPE_ref : tch = '$'; break;

			// typename
			case TYPE_vid :

			case TYPE_u08 :
			case TYPE_u16 :
			case TYPE_u32 :

			case TYPE_i08 :
			case TYPE_i16 :
			case TYPE_i32 :
			case TYPE_i64 :

			case TYPE_f32 :
			case TYPE_f64 :

			case TYPE_enu :
			case TYPE_rec :
				*++sp = ptr->args;
				*ss++ = ptr;

			// fall through
			case TYPE_def :
				tch = '@';
				break;

			default :
				tch = '?';
				break;
		}
		if (line) fputmsg(fout, "%s:%d:", s->file, line);
		fputc(tch, fout);
		for (tp = bs; tp < ss; tp++) {
			defn typ = *tp;
			if (typ != ptr && typ && typ->name)
				fputmsg(fout, "%s.", typ->name);
		}// */

		fputmsg(fout, "%?s", ptr->name);

		if (ptr->kind == TYPE_fnc) {
			defn arg = ptr->args;
			fputc('(', fout);
			while (arg) {
				fputmsg(fout, "%?s", arg->type->name);
				if (arg->name) fputmsg(fout, " %s", arg->name);
				if (arg->init) fputmsg(fout, "= %+k", arg->init);
				if ((arg = arg->next)) fputs(", ", fout);
			}
			fputc(')', fout);
		}
		if (ptr->type) {
			fputmsg(fout, ":%T", ptr->type);
			if (ptr->init) fputmsg(fout, "(%+k)", ptr->init);
		}
		fputc('\n', fout);
		fflush(fout);
	}
	return 0;
}

int cc_papi(state s, char* file) {
	return psym(s, s->defs, -1, stdout);
}

int cc_tags(state s, char* file) {
	FILE *fout = stdout;
	if (file && !(fout = fopen(file, "wt")))
		return -1;
	psym(s, s->glob, -1, fout);
	if (fout != stdout) fclose(fout);
	return 0;
}

int cc_walk(state s, char* file) {
	//~ dumpdag(stdout, s->root, 0);
	fputast(stdout, s->root, 1, 0);
	//~ fflush(stdout);
	//~ dumpdag(stdout, s->root, 0);
	//~ cc_outf(s, NULL);
	return 0;
}

int dump(state s, char* file) {
	//~ dumpdag(stdout, s->root, 0);
	FILE *out = stdout;
	fputast(out, s->root, 1, 0);
	//~ dumpdag(stdout, s->root, 0);
	//~ cc_outf(s, NULL);
	return 0;
}

int vm_dasm_old(state s, char* file) {
	unsigned st = 0, i, is, n = 1;

	FILE *fout = stdout;
	if (file && !(fout = fopen(file, "wt")))
		return -1;

	for (i = 0; i < s->code.cs; n++) {
		int ofs = i;
		bcde ip = (bcde)&s->code.memp[i];
		switch (ip->opc) {
			error_opc : error(s, "VM_ERR", 0, "invalid opcode:[%02x]", ip->opc); return -1;
			#define NEXT(__IP, __SP, __CHK) i += is = (__IP); st += (__SP);
			#define STOP(__ERR) goto __ERR
			#include "incl/exec.inl"
		}
		fputmsg(fout, ".%3d .0x%04x[%04d](%d) : %09A\n", n, ofs, st, is, ip);
		//~ fputasm(fout, ip, 0, 9, offs); fputc('\n', fout);
		fflush(fout);
	}
	//~ fflush(fout);
	if (fout != stdout)
		fclose(fout);
	return 0;
}

int vm_dasm(state s, char* file) {
	unsigned st = 0, i, is, n = 1;
	unsigned offs = 0;
	unsigned char* beg = s->code.memp + offs;
	int len = s->code.cs - offs;

	FILE *fout = stdout;
	if (file && !(fout = fopen(file, "wt")))
		return -1;

	for (i = 0; i < len; n++) {
		int ofs = i;
		bcde ip = (bcde)(beg + ofs);
		switch (ip->opc) {
			error_opc : error(s, "VM_ERR", 0, "invalid opcode:[%02x]", ip->opc); return -1;
			#define NEXT(__IP, __SP, __CHK) i += is = (__IP); st += (__SP);
			#define STOP(__ERR) goto __ERR
			#include "incl/exec.inl"
		}
		//~ fputmsg(fout, "%3d .0x%04x[%04d](%d) %09A\n", n, ofs, st, is, ip);
		fputmsg(fout, "SP[%04d]", st);
		fputasm(fout, ip, 9, ofs); fputc('\n', fout);
		fflush(fout);
	}
	if (fout != stdout)
		fclose(fout);
	return 0;
}

//~ int cc_dump(state s, char* file) ;		// pcast export ?
//~ int cc_open(state s, char* file) ;		// pcast import ?
//~ int vm_dump(state s, char* file) ;
//~ int vm_open(state s, char* file) ;

//}
//{ misc.c ---------------------------------------------------------------------
/*int vm_hgen() {
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
			error_opc : e += err = 1; break;
			#define NEXT(__IP, __SP, __CHK) if (opc_tbl[i].size != (__IP)) goto error_opc;
			#include "exec.inl"
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
}*/

/*int mk_test(char *file, int size) {
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
}*/

//~ void neg_i32(stkval *res, stkval *rhs);
//~ void mul_i32(stkval *res, stkval *lhs, stkval *rhs);
//~ ...

void vm_tags(state s) {
	defn ptr;
	FILE *fout = stdout;
	unsigned* sb = (unsigned*)s->code.pu->sp;
	for (ptr = s->glob; ptr; ptr = ptr->next) {
		if (ptr->kind == TYPE_ref && ptr->onst) {
			int spos = s->code.rets - ptr->offs;
			stkval* sp = (stkval*)(sb + spos);
			int line = ptr->link->line;
			defn typ = ptr->type;
			if (s->file && line)
				fputmsg(fout, "%s:%d:", s->file, line);
			fputmsg(fout, "sp(0x%02x):%s", spos, ptr->name);
			switch(typ ? typ->kind : 0) {
				default :
					fputmsg(fout, " = ????\n");
					break;
				case TYPE_u32 :
					fputmsg(fout, " = uns32[%08x](%u)\n", sp->i4, sp->i4);
					break;
				case TYPE_i32 :
					fputmsg(fout, " = int32[%08x](%d)\n", sp->i4, sp->i4);
					break;
				case TYPE_i64 :
					fputmsg(fout, " = int64[%016X](%D)\n", sp->i8, sp->i8);
					break;
				case TYPE_u64 :
					fputmsg(fout, " = int64[%016X](%U)\n", sp->i8, sp->i8);
					break;
				case TYPE_f32 :
					fputmsg(fout, " = flt32[%08x](%.40g)\n", sp->i4, sp->f4);
					break;
				case TYPE_f64 :
					fputmsg(fout, " = flt64[%016X](%.40G)\n", sp->i8, sp->f8);
					break;
			}
		}
	}
}

//} */

//~ #define USE_COMPILED_CODE
#include "incl/test.c"

void usage(state s, char* name, char* cmd) {
	int n = 0;
	//~ char *pn = strrchr(name, '/');
	//~ if (pn) name = pn+1;
	if (strcmp(cmd, "-api") == 0) {
		cc_init(s);
		psym(s, s->defs, -1, stdout);
		return ;
	}
	if (cmd == NULL) {
		puts("Usage : program command [options] ...\n");
		puts("where command can be one of:\n");
		puts("\t-c: compile\n");
		puts("\t-e: execute\n");
		puts("\t-m: make\n");
		puts("\t-h: help\n");
		//~ puts("\t-d: dump\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-c") == 0) {
		//~ puts("compile : %s -c [options] files...\n", name);
		puts("compile : psvm -c [options] files...\n");
		puts("Options :\n");

		puts("\t[Output to]\n");
		puts("\t-o <file> set file for output.\n");
		puts("\t-l <file> set file for errors.\n");

		puts("\t[Output is]\n");
		puts("\t-t tags\n");
		puts("\t-d code\n");
		puts("\t-a asm\n");

		puts("\t[Errors & Warnings]\n");
		//~ puts("\t-w<num> set warning level to <num> [default=%d]\n", wl);
		puts("\t-w<num> set warning level to <num>\n");
		puts("\t-wa all warnings\n");
		puts("\t-wx treat warnings as errors\n");

		//~ puts("\t[Debug & Optimization]\n");
		//~ puts("\t-d0 generate no debug information\n");
		//~ puts("\t-d1 generate debug {line numbers}\n");
		//~ puts("\t-d2 generate debug {-d1 + Symbols}\n");
		//~ puts("\t-d3 generate debug {-d2 + Symbols (all)}\n");
		//~ puts("\t-O<num> set optimization level. [default=%d]\n", dol);
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-e") == 0) {
		puts("command : '-e' : execute\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-d") == 0) {
		puts("command : '-d' : disassemble\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-m") == 0) {
		puts("command : '-m' : make\n");
		n += 1;
	}
	if (!cmd || strcmp(cmd, "-h") == 0) {
		puts("command : '-h' : help\n");
		n += 1;
	}
	if (!n)
		puts("invalid command\n");
}

int prog(state s, int argc, char *argv[]) {
	if (argc < 3) usage(s, argv[0], argv[1]);
	else if (strcmp(argv[1], "=") == 0) {	// eval
		struct astn_t res, *exp;
		cc_init(s);
		s->_ptr = argv[2];
		s->_cnt = strlen(argv[2]);
		if (eval(&res, exp = expr(s, 0), 0))
			fputmsg(stdout, "%+k = %+k\n", exp, &res);
		else fputmsg(stdout, "not evaluable\n");
		return 0;
		//~ return calc(s, argv[2]);
	}
	else if (strcmp(argv[1], "-c") == 0) {	// compile
		enum {
			out_bin = 0,	// vm_dump
			out_tag = 1,	// cc_tags
			out_pch = 2,	// cc_dump
			out_ast = 4,	// cc_walk
			out_asm = 5,	// vm_dasm
			run_bin = 8,	// vm_exec
		};
		int dbgl = 0, arg = 2;
		int warn = wl;
		//~ int optl = ol;
		int outc = 0;			// output type
		char* outf = 0;			// output file
		char* logf = 0;			// logger file

		// options
		for (arg = 2; arg < argc; arg += 1) {
			if (!argv[arg] || *argv[arg] != '-') break;

			// Override output file
			else if (strcmp(argv[arg], "-l") == 0) {		// log
				if (++arg >= argc || logf) {
					fputmsg(stderr, "logger error\n");
					return -1;
				}
				logf = argv[arg];
			}
			else if (strcmp(argv[arg], "-o") == 0) {		// out
				if (++arg >= argc || outf) {
					fputmsg(stderr, "output error\n");
					return -1;
				}
				outf = argv[arg];
			}

			// Override output type
			else if (strcmp(argv[arg], "-t") == 0) {		// tag
				outc = out_tag;
			}
			else if (strcmp(argv[arg], "-a") == 0) {		// asm
				outc = out_asm;
			}
			else if (strcmp(argv[arg], "-c") == 0) {		// ast
				outc = out_ast;
			}
			else if (strcmp(argv[arg], "-h") == 0) {		// pch
				outc = out_ast;
			}

			// Override settings
			else if (strncmp(argv[arg], "-x", 2) == 0) {		// execute
				outc = run_bin;
			}

			else if (strncmp(argv[arg], "-d", 2) == 0) {		// execute
				outc = run_bin;
				switch (argv[arg][2]) {
					case  0  : dbgl = 7; break;
					case '1' : dbgl = 1; break;
					case '2' : dbgl = 2; break;
					default  : 
						fputmsg(stderr, "invalid debug level '%c'\n", argv[arg][2]);
						dbgl = 2;
						break;
				}
			}
			else if (strncmp(argv[arg], "-w", 2) == 0) {		// warning level
				if (!argv[arg][3]) switch (argv[arg][2]) {
					case '0' : warn = 0; break;
					case '1' : warn = 1; break;
					case '2' : warn = 2; break;
					case '3' : warn = 3; break;
					case '4' : warn = 4; break;
					case '5' : warn = 5; break;
					case '6' : warn = 6; break;
					case '7' : warn = 7; break;
					case '8' : warn = 8; break;
					case '9' : warn = 9; break;
					case 'a' : warn = 9; break;		// all
					case 'x' : warn = -1; break;	// threat as errors
					default  :
						fputmsg(stderr, "invalid warning level '%c'\n", argv[arg][2]);
						break;
				}
				else fputmsg(stderr, "invalid warning level '%s'\n", argv[arg]);
			}
			else if (strncmp(argv[arg], "-d", 2) == 0) {		// optimize/debug level
				return -1;
			}
			else {
				fputmsg(stderr, "invalid option '%s' for -compile\n", argv[arg]);
				return -1;
			}
		}

		// compile
		cc_init(s);
		s->warn = warn;
		cc_outf(s, logf);

		while (arg < argc) {
			cc_file(s, 0, argv[arg]);
			arg += 1;
		}

		vm_cgen(s);

		// output
		switch (outc) {
			//~ case out_pch : cc_dump(s, outf); break;
			//~ case out_bin : vm_dump(s, outf); break;
			case out_tag : cc_tags(s, outf); break;
			case out_ast : cc_walk(s, outf); break;
			case out_asm : vm_dasm(s, outf); break;

			case run_bin : {		// exec
				//~ vm_exec(&env, cc, ss);
				int i = vm_exec(s, cc, ss, dbgl);
				fputmsg(stdout, "exec inst: %d\n", s->code.ic);
				if (i == 0) vm_tags(s);
			} break;
		}
		return cc_done(s);
	}
	else if (strcmp(argv[1], "-e") == 0) {	// execute
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
		//~ vm_open(&env, ...);
		//~ vm_exec(&env, ...);
		//~ return vm_done(&env, ...);
	}
	else if (strcmp(argv[1], "-d") == 0) {	// disassemble
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
	}
	else if (strcmp(argv[1], "-m") == 0) {	// make
		error(0, __FILE__, __LINE__, "unimplemented option '%s' \n", argv[1]);
	}
	else if (strcmp(argv[1], "-h") == 0) {	// help
		usage(s, argv[0], argc < 4 ? argv[3] : NULL);
	}
	else error(0, __FILE__, __LINE__, "invalid option '%s' \n", argv[1]);
	return 0;
}

int dbug(state s, char* src) {
	int i, ret;

	cc_init(s);
	ret = cc_file(s, 0, src);
	printf(">scan: Exit: %d\tTime: %lg\n", ret, (double)s->time / CLOCKS_PER_SEC);
	ret = vm_cgen(s);
	printf(">cgen: Exit: %d\tTime: %lg\n", ret, (double)s->time / CLOCKS_PER_SEC);
	ret = vm_exec(s, cc, ss, dl);
	printf(">exec: Exit: %d\tTime: %lg\n", ret, (double)s->time / CLOCKS_PER_SEC);

	i = s->buffp - s->buffer; printf("parse mem: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
	//~ i = s->code.ds; printf("data size: %dM, %dK, %dB\n", i >> 20, i >> 10, i);
	i = s->code.cs; printf("code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, s->code.ic);
	i = s->code.mins * 4; printf("max stack: %dM, %dK, %dB, %d slots\n", i >> 20, i >> 10, i, s->code.mins);

	//~ printf(">capi:\n"); fflush(stdout); cc_papi(&env, 0);
	//~ printf(">tags:\n"); fflush(stdout); cc_tags(&env, 0);
	printf(">code:\n"); fflush(stdout); cc_walk(s, 0);
	//~ dumpsym(stdout, s->glob, 0);
	//~ for (syms = s->defs; syms; syms = syms->next) dumpsym(stdout, syms, 0);
	//~ printf(">dasm:\n"); fflush(stdout); vm_dasm(&env, 0);

	if (ret == 0) {
		printf("exec inst: %d\n", s->code.ic);
		vm_tags(s);
	}

	return ret;
}

struct state_t env;

int emit_test(state s) {
	cc_init(s);
	cc_buff(s, 0, __FILE__, __LINE__ + 1,
		"flt64 re = 3, im = 4;\n"		// -14, 23
		"flt64 r2 = 2, i2 = 5;\n"
		"emit(mul.p4d, i2, r2, im, re);\n"	// tmp1,2 = a.re * b.re, a.im * b.im
		"emit(mul.p4d, r2, i2, im, re);\n"	// tmp3,4 = a.re * b.im, a.im * b.re
		"im = emit(add.f64);\n"			//>im = a.re * b.im + a.im * b.re
		"re = emit(sub.f64);\n"			//>re = a.re * b.re - a.im * b.im
	);
	vm_cgen(s);

	//~ printf("Stack max = %d / %d slots\n", s->code.mins * 4, s->code.mins);
	//~ printf("Code size = %d / %d instructions\n", s->code.cs, s->code.ic);
	vm_exec(s, cc, ss, 0);
	vm_tags(s);
	vm_dasm(s, NULL);
	//~ cc_tags(s, NULL);
	return cc_done(s);
}

int main(int argc, char *argv[]) {
	int i;
	//~ char *src = "test0.cxx";
	//~ if (vm_test()) return -1;
	//~ if (cc_test()) return -1;

	if (argc != 1)
		return prog(&env, argc, argv);
	//~ return cplfltmul(&env);
	//~ return cpldblmul(&env);
	//~ return swz_test(&env);
	return emit_test(&env);
	//~ return vm_hgen();
	//~ mk_test(src = "txx.cxx", 1<<20);
	//~ return dbug(&env, src);

	cc_init(&env);
	cc_papi(&env, 0);
	/* swizzle generator
	for(i = 0; i < 256; i++) {
		char c1 = "xyzw"[i>>0&3];
		char c2 = "xyzw"[i>>2&3];
		char c3 = "xyzw"[i>>4&3];
		char c4 = "xyzw"[i>>6&3];
		//~ printf("\t%c%c%c%c = 0x%02x;\n", c4, c3, c2, c1, i);
		printf("install(s, \"%c%c%c%c\", -1, CNST_int, %s)->init = intnode(s, 0x%02x);\n", c4, c3, c2, c1, "mod", i);
	} // */
	/*
	cc_init(&env);
	cc_papi(&env, 0);
	for(i = 0; i < sizeof(libcfnc)/sizeof(*libcfnc); i++) {
		fputmsg(stdout, "%T;ret:%d;pop:%d;arg:%d\n", libcfnc[i].sym, libcfnc[i].ret, libcfnc[i].pop, libcfnc[i].arg);
	}
	*/
	return i = 0;
}
