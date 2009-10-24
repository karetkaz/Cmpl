#include <math.h>
#include <string.h>
#include "pvmc.h"

#ifdef __WATCOMC__
#pragma disable_message(124);
#endif

#pragma pack(push, 1)
typedef struct bcde {			// byte code decoder
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
			uns32t opc:4;		// 0 ... 15
			uns32t mem:2;		// 
			uns32t res:6;		// Result
			uns32t lhs:6;		// a
			uns32t rhs:6;		// b
			
		}ext;*/
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
} *bcde;
#pragma pack(pop)
//~ /*
typedef struct cell {			// processor
	//~ unsigned int	ss;			// ss / stack size
	//~ unsigned int	cs;			// child start(==0 join) / code size (pc)
	//~ unsigned int	pp;			// parent proc(==0 main) / ???

	unsigned char	*ip;			// Instruction pointer
	unsigned char	*sp;			// Stack pointer(bp + ss)
	unsigned char	*bp;			// Stack base

	// flags ?
	//~ unsigned	zf:1;			// zero flag
	//~ unsigned	sf:1;			// sign flag
	//~ unsigned	cf:1;			// carry flag
	//~ unsigned	of:1;			// overflow flag

} *cell; // */

//{ libc.c ---------------------------------------------------------------------

void f32abs(state s) {
	flt32t x = poparg(s, flt32t);
	setret(flt32t, s, fabs(x));
}
void f32sin(state s) {
	flt32t x = poparg(s, flt32t);
	setret(flt32t, s, sin(x));
}
void f32cos(state s) {
	flt32t x = poparg(s, flt32t);
	setret(flt32t, s, cos(x));
}
void f32tan(state s) {
	flt32t x = poparg(s, flt32t);
	setret(flt32t, s, tan(x));
}
void f32sqrt(state s) {
	flt32t x = poparg(s, flt32t);
	setret(flt32t, s, sqrt(x));
}

void f64abs(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, fabs(x));
}
void f64sin(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sin(x));
}
void f64cos(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, cos(x));
}
void f64atan2(state s) {
	flt64t x = poparg(s, flt64t);
	flt64t y = poparg(s, flt64t);
	setret(flt64t, s, atan2(x, y));
	//~ debug("flt64t atan2(flt64t x(%G), flt64t y(%G)) = %G", x, y, atan2(x, y));
}

/*void f64lg2(state s) {
	double log2(double);
	flt64t *sp = stk;
	*sp = log2(*sp);
}
void f64_xp2(state s) {
	double exp2(double);
	flt64t *sp = stk;
	*sp = exp2(*sp);
}*/
void f64log(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, log(x));
}
void f64exp(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, exp(x));
}

void b32btc(state s) {
	uns32t x = poparg(s, uns32t);
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	setret(uns32t, s, x & 0x3f);
}
void b32bsf(state s) {
	uns32t x = poparg(s, uns32t);
	int ans = 0;
	if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { ans +=  1; }
	setret(uns32t, s, x ? ans : -1);
}
void b32bsr(state s) {
	uns32t x = poparg(s, uns32t);
	unsigned ans = 0;
	if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { ans +=  1; }
	setret(uns32t, s, x ? ans : -1);
}
void b32swp(state s) {
	uns32t x = poparg(s, uns32t);
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	setret(uns32t, s, (x >> 16) | (x << 16));
}
void b32hib(state s) {
	uns32t x = poparg(s, uns32t);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	setret(uns32t, s, x - (x >> 1));
}
void b32lob(state s) ;
void b32shl(state s) {
	uns32t x = poparg(s, uns32t);
	uns32t y = poparg(s, uns32t);
	setret(uns32t, s, x << y);
	//~ debug("uns32 shl(uns32t x(%d), uns32t y(%d)) = %d", x, y, x << y);
}
void b32shr(state s) {
	uns32t x = poparg(s, uns32t);
	uns32t y = poparg(s, uns32t);
	setret(uns32t, s, x >> y);
	//~ debug("uns32 shl(uns32 x(%d), uns32 y(%d)) = %d", x, y, x >> y);
}
void b32sar(state s) {
	int32t x = poparg(s, int32t);
	int32t y = poparg(s, int32t);
	setret(int32t, s, x >> y);
	//~ debug("int32 shl(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}

void b32zxt(state s) {
	uns32t val = poparg(s, int32t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, s, val >> (32 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}
void b32sxt(state s) {
	int32t val = poparg(s, int32t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, s, val >> (32 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}
void b64zxt(state s) {
	uns64t val = poparg(s, int64t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, s, val >> (64 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}
void b64sxt(state s) {
	int64t val = poparg(s, int64t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, s, val >> (64 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}

void b64shl(state s) {
	uns64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x << y);
	//~ debug("int64 shl(int64 x(%D), int32 y(%D)) = %D", x, y, x << y);
}
void b64shr(state s) {
	uns64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x >> y);
	//~ debug("int64 shl(int64 x(%D), int32 y(%D)) = %D", x, y, x << y);
}
void b64sar(state s) {
	int64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x >> y);
	//~ debug("int64 shl(int64 x(%D), int32 y(%D)) = %D", x, y, x << y);
}

static int libccnt = 0;
static struct lfun {
	void (*call)(state);
	const char* proto;//, *name;
	uns08t chk, pop;
	symn sym;
}
libcfnc[256] = {
	{f32abs, "flt32 abs(flt32 x)"},
	{f32sin, "flt32 sin(flt32 x)"},
	{f32cos, "flt32 cos(flt32 x)"},
	{f32tan, "flt32 tan(flt32 x)"},
	{f32sqrt, "flt32 sqrt(flt32 x)"},

	//~ {f64abs, "flt64 abs(flt64 x)"},
	//~ {f64sin, "flt64 sin(flt64 x)"},
	//~ {f64cos, "flt64 cos(flt64 x)"},
	//~ {f64atan2, "flt64 atan2(flt64 x, flt64 y)"},
	//~ {f64lg2, "flt64 log2(flt64 x)"},
	//~ {f64xp2, "flt64 exp2(flt64 x)"},
	{f64log, "flt64 log(flt64 x)"},
	{f64exp, "flt64 exp(flt64 x)"},

	//~ {b32btc, "int32 btc(uns32 x)"},		// bitcount
	//~ {b32btc, "int32 btc(uns32 x)"},
	//~ {b32bsf, "int32 bsf(uns32 x)"},
	//~ {b32bsr, "int32 bsr(uns32 x)"},
	{b32bsf, "int32 bsf(int32 x)"},
	{b32bsr, "int32 bsr(int32 x)"},
	{b32swp, "int32 swp(int32 x)"},
	{b32hib, "int32 bhi(int32 x)"},
	{b32hib, "uns32 bhi(uns32 x)"},

	//~ {b32shl, "int32 shl(uns32 x, int32 y)"},
	//~ {b32shl, "int32 shl(int32 x, int32 y)"},
	//~ {b32shr, "int32 shr(uns32 x, int32 y)"},
	//~ {b32sar, "int32 shr(int32 x, int32 y)"},

	//~ {b64shl, "int64 shl(int64 x, int32 y)"},
	//~ {b64shl, "int64 shl(uns64 x, int32 y)"},
	//~ {b64shr, "int64 shr(int64 x, int32 y)"},
	//~ {b64sar, "int64 sar(int64 x, int32 y)"},
	//~ {b64shr, "int64 shr(uns64 x, int32 y)"},
	//~ {setpos, "void setPos(flt32 x, flt32 y, flt32 z)", 3, 3},
	//~ {setpos, "void setNrm(flt32 x, flt32 y, flt32 z)", 3, 3},
	//~ {setpos, "void setCol(flt32 x, flt32 y, flt32 z)", 3, 3},
	//~ {getarg, "flt32 getArg(int32 arg, flt32 min, flt32 max)", 3, 2},
	//~ {getTex, "argb tex2d(flt32, flt32)"},
};

void installlibc(state s, void libc(state), const char* proto) {
	int i = libccnt;

	if (libccnt == 0) {
		while (libcfnc[libccnt].proto)
			libccnt += 1;
	}

	if (libccnt >= 255)
		return;

	libcfnc[libccnt].call = libc;
	libcfnc[libccnt].proto = proto;

	while (libcfnc[i].proto) {
		symn sym = install(s->cc, -1, libcfnc[i].proto, 0);
		int stdiff = 0, retsize = sym->type->size;
		symn arg;

		for (arg = sym->args; arg; arg = arg->next)
			stdiff += arg->type->size;

		libcfnc[i].sym = sym;
		libcfnc[i].sym->offs = i;

		libcfnc[i].pop = (stdiff - retsize) / 4;
		libcfnc[i].chk = stdiff / 4;

		i++;
	}
	libccnt = i;
}
//}

int emit(vmEnv s, int opc, ...) {
	bcde ip = 0;
	stkval arg = *(stkval*)(&opc + 1);

	if (opc == get_ip) {
		return s->cs;
	}
	if (opc == get_sp) {
		return -s->ss;
	}
	/*if (opc == seg_data) {
		return s->ss;
	}// */
	if (opc == loc_data) {
		s->_ptr += arg.i4;
		return 0;
	}
	if (opc == seg_code) {
		s->pc = s->_ptr - s->_mem;
		s->_ptr += s->cs;
		return s->ss;
	}
	if (opc == opc_line) {
		static int line = 0;
		if (line != arg.i4) {
			//~ debug("emit(line = %d)", arg.i4);
			line = arg.i4;
		}
		return s->pc;
	}

	if (s->_end - s->_ptr < 16) {
		debug("memory overrun");
		return -1;
	}
	/*else if (opc == opc_ldc) {
		if (arg.u8 == 0) opc = opc_ldz;
		else if (arg.u8 <= 0xff) opc = opc_ldc1;
		else if (arg.u8 <= 0xffff) opc = opc_ldc2;
		else if (arg.u8 <= 0xffffffff) opc = opc_ldc4;
		else opc = opc_ldc8;
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
	else if (opc == opc_cne) {
		switch (arg.i4) {
			case TYPE_u32:
			case TYPE_i32: opc = i32_ceq; break;
			case TYPE_f32: opc = f32_ceq; break;
			case TYPE_i64: opc = i64_ceq; break;
			case TYPE_f64: opc = f64_ceq; break;
			default: return -1;
		}
		if (emit(s, opc) <= 0)
			return -1;
		return emit(s, b32_not);
	}
	else if (opc == opc_cle) {
		switch (arg.i4) {
			case TYPE_u32:
			case TYPE_i32: opc = i32_cgt; break;
			case TYPE_f32: opc = f32_cgt; break;
			case TYPE_i64: opc = i64_cgt; break;
			case TYPE_f64: opc = f64_cgt; break;
			default: return -1;
		}
		if (emit(s, opc) <= 0)
			return -1;
		return emit(s, b32_not);
	}
	else if (opc == opc_cge) {
		switch (arg.i4) {
			case TYPE_u32:
			case TYPE_i32: opc = i32_clt; break;
			case TYPE_f32: opc = f32_clt; break;
			case TYPE_i64: opc = i64_clt; break;
			case TYPE_f64: opc = f64_clt; break;
			default: return -1;
		}
		if (emit(s, opc) <= 0)
			return -1;
		return emit(s, b32_not);
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

	/*else if (opc == opc_ldi) switch (arg.i4) {
		case  1: opc = opc_ldi1; break;
		case  2: opc = opc_ldi2; break;
		case  4: opc = opc_ldi4; break;
		case  8: opc = opc_ldi8; break;
		case 16: opc = opc_ldiq; break;
		default: return -1;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case  1: opc = opc_sti1; break;
		case  2: opc = opc_sti2; break;
		case  4: opc = opc_sti4; break;
		case  8: opc = opc_sti8; break;
		case 16: opc = opc_stiq; break;
		default: return -1;
	}*/

	/*~:)) ?? rollbacks
	if (opc == opc_ldz1) {
		ip = (bcde)&s->_ptr[s->pc];
		if (ip->opc == opc_ldz1) {
			opc = opc_ldz2;
			s->cs = s->pc;
			s->ss -= 1;
		}
	}
	else if (opc == opc_ldz2) {
		ip = (bcde)&s->mem[s->pc];
		if (ip->opc == opc_ldz2) {
			opc = opc_ldz4;
			s->cs = s->pc;
			s->ss -= 2;
		}
	}
	else if (opc == opc_dup1) {			// TODO 
		ip = (bcde)&s->mem[s->pc];
		if (ip->opc == opc_dup1 && ip->idx == arg.u4) {
			ip->opc = opc_dup2;
			ip->idx -= 1;
			s->sm += 1;
			return s->pc;
		}
	}
	else if (opc == opc_dup2) {
		ip = (bcde)&s->mem[s->pc];
		if (ip->opc == opc_dup2 && ip->idx == arg.u4) {
			ip->opc = opc_dup4;
			ip->idx -= 2;
			s->sm += 2;
			return s->pc;
		}
	}
	/ *
	else if (opc == b32_not) {
		ip = (bcde)&s->mem[s->pc];
		if (ip->opc == b32_not) {
			s->cs = s->pc;
			return s->pc;
		}
	}
	else if (opc == opc_jnz || opc == opc_jz) {
		ip = (bcde)&s->memp[s->pc];
		if (ip->opc == b32_not && ip->arg.u1 == 1) {
			opc = (opc == opc_jnz) ? opc_jz : opc_jnz;
			s->cs = s->pc;
		}
	}// * /
	/ *else if (opc == opc_shl || opc == opc_shr || opc == opc_sar) {
		ip = (bcde)&s->memp[s->pc];
		if (ip->opc == opc_ldc4) {
			if (opc == opc_shl) arg.i1 = 1 << 5;
			if (opc == opc_shr) arg.i1 = 2 << 5;
			if (opc == opc_sar) arg.i1 = 3 << 5;
			s->rets -= 1;
			opc = opc_bit;
			arg.i1 |= ip->arg.u1 & 0x1F;
			s->cs = s->pc;
		}
	}// */

	if (opc > opc_last) {
		//~ fatal("xxx_opc%x", opc);
		fatal("invalid opc_0x%x", opc);
		return 0;
	}

	ip = (bcde)&s->_ptr[s->cs];
	ip->opc = opc;
	ip->arg = arg;

	s->pc = s->cs;
	//~ debug("@%04x[ss:%03d]: %A", s->pc, s->ss, ip);
	switch (opc) {		//#exec.c
		error_opc: fatal("invalid opcode: [%02x]", ip->opc); return -1;
		error_stc: fatal("stack underflow: near opcode %A", ip); return -1;
		#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
		#define NEXT(__IP, __CHK, __SP)\
			STOP(error_stc, s->ss < (__CHK));\
			s->ss += (__SP);\
			s->cs += (__IP);
		//~ #define EXEC(__STMT)
		//~ #define MEMP(__MPTR)
		//~ #define CHECK()
		#include "incl/exec.c"
	}

	s->ic += s->pc != s->cs;

	if (s->sm < s->ss)
		s->sm = s->ss;

	return s->pc;
}
int emiti32(vmEnv s, int32t arg) {
	stkval tmp;
	tmp.i4 = arg;
	return emit(s, opc_ldc4, tmp);
}
int emiti64(vmEnv s, int64t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emit(s, opc_ldc8, tmp);
}
int emitf32(vmEnv s, flt32t arg) {
	stkval tmp;
	tmp.f4 = arg;
	return emit(s, opc_ldcf, tmp);
}
int emitf64(vmEnv s, flt64t arg) {
	stkval tmp;
	tmp.f8 = arg;
	return emit(s, opc_ldcF, tmp);
}
int emitidx(vmEnv s, int opc, int arg) {
	stkval tmp;
	tmp.i4 = s->ss + arg;
	if (tmp.u4 > 255) {
		debug("%d", tmp.u4);
		return 0;
	}
	if (tmp.u4 > s->ss) {
		debug("%d", tmp.u4);
		return 0;
	}
	return emit(s, opc, tmp);
}
int emitint(vmEnv s, int opc, int64t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emit(s, opc, tmp);
}
int fixjump(vmEnv s, int src, int dst, int stc) {
	bcde ip = (bcde)(s->_ptr + src);
	if (src >= 0) switch (ip->opc) {
		case opc_jmp:
		case opc_jnz:
		case opc_jz: ip->jmp = dst - src; break;
		default: debug("expecting jump, but found: '%+A'", ip);
	}
	return 0;
}

/*static cell task(vmEnv ee, cell pu, int cc, int n, int cl, int dl) {
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
	return pu + n;
}// */

/*static int done(vmEnv ee, int cp) {
	pu[pu[cp].pp].cp -= 1;
	sigsnd(pp->pp, SIG_quit);
}// */

static cell getProc(vmEnv ee, cell pu) {return ((pu && pu->ip) ? pu : 0);}

void vm_tags(ccEnv s, char *sptr, int slen) {
	FILE *fout = stdout;
	symn ptr;
	for (ptr = s->defs; ptr; ptr = ptr->next) {
		if (ptr->kind == TYPE_ref && ptr->offs < 0) {
			int spos = slen + ptr->offs;
			stkval* sp = (stkval*)(sptr + 4 * spos);
			symn typ = ptr->type;
			if (ptr->file && ptr->line)
				fputfmt(fout, "%s:%d:", ptr->file, ptr->line);
			fputfmt(fout, "sp(%d):%s", spos, ptr->name);
			switch(typ ? typ->kind : 0) {
				case TYPE_p4x: {
					if (typ == type_f32x4)
						fputfmt(fout, " = f32x4(%g, %g, %g, %g)\n", sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w);
					if (typ == type_f64x2)
						fputfmt(fout, " = f64x2(%G, %G)\n", sp->pd.x, sp->pd.y);
				} break;
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

				default:
				TYPE_XXX:
					fputfmt(fout, " = ????\n");
					break;
			}
		}
	}
}

void vmInfo(vmEnv vm) {
	int i;
	//~ i = vm->sm; printf("stack max: %dM, %dK, %dB, %d slots\n", i >> 20, i >> 20, i, vm->sm);
	//~ i = vm->cs; printf("code size: %dM, %dK, %dB, %d instructions\n", i >> 20, i >> 10, i, 0);//, vm->ic);

	i = vm->cs; printf("code(@.%04x) size: %dM, %dK, %dB, %d instructions\n", vm->pc, i >> 20, i >> 10, i, vm->ic);
	//~ i = vm->cs; printf("code(@.%04x) size: %dM, %dK, %dB\n", vm->pc, i >> 20, i >> 10, i);
	i = vm->ds; printf("data(@.%04x) size: %dM, %dK, %dB\n", 0, i >> 20, i >> 10, i);
	printf("stack minimum size: %d\n", vm->sm);
}

/** exec
 * @arg env: enviroment
 * @arg cc: cell count
 * @arg ss: stack size
 * @return: error code
**/

int exec(vmEnv vm, unsigned cc, unsigned ss, dbgf dbg) {
	struct cell proc[1], *pu = proc;	// execution units

	register bcde ip;
	register unsigned char *st;

	/*if (cc > sizeof(proc) / sizeof(*proc)) {
		debug("cell overrun\n");
		return -1;
	}* /
	/ *if (ss < env->sm) {
		debug("stack overrun(%d, %d)\n", ss, env->sm);
		return -1;
	}
	/ *if ((env->cs + ss * cc) >= ?) {
		debug("memory overrun\n");
		return -1;
	}
	//~TODO: memset(vm->_mem, 0, vm->ds);
	*/

	memset(vm->_mem, 0, vm->ds);
	pu->ip = vm->_mem + vm->pc;
	pu->bp = vm->_end - ss;
	pu->sp = vm->_end;

	if (dbg == NULL)
		dbg = nodbg;

	while ((pu = getProc(vm, proc))) {

		//~ s->ic += 1;
		st = pu->sp;			// stack
		ip = (bcde)pu->ip;

		//~ tick += pu == proc;		// count tiks

		if (dbg(vm, 0, ip, (long*)st, (vm->_end - st) / 4))
			return -9;

		switch (ip->opc) {		// exec
			error_opc: debug("invalid opcode: [%02x]", ip->opc); return -1;
			error_ovf: debug("stack overflow: op[%A]", ip); return -2;
			error_div: debug("division by zero: op[%A]", ip); return -3;
			//~ error_mem: debug("segmentation fault"); return -5;
			//~ #define CDBG
			//~ #define FLGS
			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			//~ #define MEMP(__MPTR) (mp = (__MPTR))
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			#include "incl/exec.c"
		}
	}

	dbg(vm, 0, NULL, (long*)proc->sp, (vm->_end - proc->sp) / 4);

	//~ dbg(vm, 0, NULL, (long*)pu->sp, (vm->_end - pu->sp) / 4);
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

void fputasm(FILE *fout, unsigned char* beg, int len, int offs) {
	unsigned is = 12345, i;
	for (i = 0; i < len; i += is) {
		bcde ip = (bcde)(beg + i);
		switch (ip->opc) {
			error_opc: fputfmt(stderr, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			#define NEXT(__IP, __CHK, __SP) {is = (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "incl/exec.c"
		}
		//~ fputfmt(fout, ".%04x:%9A\n", i, ip);
		//~ fputfmt(fout, ".%04x:\t%A\n", i, ip);
		fputfmt(fout, ".%04x: ", i);
		fputopc(fout, ip, 0, i);
		fputc('\n', fout);
	}
}

void dumpasm(FILE *fout, vmEnv s, int mode) {
	unsigned is = 12345, ss = 0, i;
	for (i = 0; i < s->cs; i += is) {
		bcde ip = (bcde)(s->_mem + s->pc + i);

		switch (ip->opc) {
			error_opc: fputfmt(stderr, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			#define NEXT(__IP, __CHK, __SP) {is = (__IP); ss += (__SP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "incl/exec.c"
		}

		//~ fputfmt(fout, ".%04x:%9A\n", i, ip);
		//~ fputfmt(fout, ".%04x:\t%A\n", i, ip);

		if (mode & 0x10)		// offset
			fputfmt(fout, ".%04x: ", i);

		if (mode & 0x20)		// stack
			fputfmt(fout, "ss(%03d): ", ss);

		fputopc(fout, ip, mode & 0xf, mode & 0x10 ? i : -1);

		fputc('\n', fout);
	}
	//~ fputasm(fout, s->_mem + s->pc, s->cs, offs);
}
