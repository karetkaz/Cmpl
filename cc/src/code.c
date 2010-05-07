#include <math.h>
#include <string.h>
#include "ccvm.h"
//~ #include "code.h"

#ifdef __WATCOMC__
#pragma disable_message(124);
//~ Warning! W124: Comparison result always 0
#endif

#pragma pack(push, 1)
typedef struct bcde {			// byte code decoder
	uns08t opc;
	union {
		stkval arg;		// argument (load const)
		uns08t idx;		// usualy index for stack
		int32t rel:24;	// relative offset (ip, sp) +- 7MB
		struct {		// when starting a task
			uns08t	dl;	// data to copy to stack
			uns16t	cl;	// code to exec paralel
		};
		/*struct {				// extended3: 4 bytes `res := lhs OP rhs`
			uns32t opc:4;		// 0 ... 15

			uns32t mem:2;		// mem
			uns32t res:6;		// res
			uns32t lhs:6;		// lhs
			uns32t rhs:6;		// rhs
			/ * --8<-------------------------------------------
			void *res = sp + ip->ext.res;
			void *lhs = sp + ip->ext.lhs;
			void *rhs = sp + ip->ext.rhs;
			switch (ip->ext.mem) {
				case 1: res = *(void**)res; break;
				case 2: lhs = *(void**)lhs; break;
				case 3: rhs = *(void**)rhs; break;
			}
			switch (ip->ext.opc) {
				case ext_add: add(res, lhs, rhs); break;
				case ext_...: ...(res, lhs, rhs); break;
			}
			// * / --8<----------------------------------------
		}ext;// */
	};
} *bcde;
#pragma pack(pop)

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

} *cell;

#define useBuiltInFuncs
//{ libc.c ---------------------------------------------------------------------
//~ #pragma intrinsic(log,cos,sin,tan,sqrt,fabs,pow,atan2,fmod)
//~ #pragma intrinsic(acos,asin,atan,cosh,exp,log10,sinh,tanh)

static int libHalt(state s) {
	return 0;
}
#ifdef useBuiltInFuncs
static int f64abs(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, abs(x));
	return 0;
}
static int f64sin(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sin(x));
	return 0;
}
static int f64cos(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, cos(x));
	return 0;
}
static int f64tan(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, tan(x));
	return 0;
}

static int f64log(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, log(x));
	return 0;
}
static int f64exp(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, exp(x));
	return 0;
}
static int f64pow(state s) {
	flt64t x = poparg(s, flt64t);
	flt64t y = poparg(s, flt64t);
	setret(flt64t, s, pow(x, y));
	return 0;
}
static int f64sqrt(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sqrt(x));
	return 0;
}

static int f64atan2(state s) {
	flt64t x = poparg(s, flt64t);
	flt64t y = poparg(s, flt64t);
	setret(flt64t, s, atan2(x, y));
	return 0;
}

/*static int f64lg2(state s) {
	double log2(double);
	flt64t *sp = stk;
	*sp = log2(*sp);
	return 0;
}
static int f64_xp2(state s) {
	double exp2(double);
	flt64t *sp = stk;
	*sp = exp2(*sp);
	return 0;
}*/

static int b32btc(state s) {
	uns32t x = poparg(s, uns32t);
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8) + (x >> 16);
	setret(uns32t, s, x & 0x3f);
	return 0;
}
static int b32bsf(state s) {
	uns32t x = poparg(s, uns32t);
	int ans = 0;
	if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { ans +=  1; }
	setret(uns32t, s, x ? ans : -1);
	return 0;
}
static int b32bsr(state s) {
	uns32t x = poparg(s, uns32t);
	unsigned ans = 0;
	if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { ans +=  1; }
	setret(uns32t, s, x ? ans : -1);
	return 0;
}
static int b32swp(state s) {
	uns32t x = poparg(s, uns32t);
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	setret(uns32t, s, (x >> 16) | (x << 16));
	return 0;
}
static int b32hib(state s) {
	uns32t x = poparg(s, uns32t);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	setret(uns32t, s, x - (x >> 1));
	return 0;
}
//~ static int b32lob(state s) := x & -x;

static int b32zxt(state s) {
	uns32t val = poparg(s, int32t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, s, val >> (32 - cnt));
	return 0;
}
static int b32sxt(state s) {
	int32t val = poparg(s, int32t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, s, val >> (32 - cnt));
	return 0;
}

static int b64shl(state s) {
	uns64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x << y);
	return 0;
}
static int b64shr(state s) {
	uns64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x >> y);
	return 0;
}
static int b64sar(state s) {
	int64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x >> y);
	return 0;
}
static int b64and(state s) {
	uns64t x = poparg(s, uns64t);
	uns64t y = poparg(s, uns64t);
	setret(uns64t, s, x & y);
	return 0;
}
static int b64ior(state s) {
	uns64t x = poparg(s, uns64t);
	uns64t y = poparg(s, uns64t);
	setret(uns64t, s, x | y);
	return 0;
}
static int b64xor(state s) {
	uns64t x = poparg(s, uns64t);
	uns64t y = poparg(s, uns64t);
	setret(uns64t, s, x ^ y);
	return 0;
}

static int b64zxt(state s) {
	uns64t val = poparg(s, int64t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, s, val >> (64 - cnt));
	return 0;
}
static int b64sxt(state s) {
	int64t val = poparg(s, int64t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, s, val >> (64 - cnt));
	return 0;
}

static int putchr(state s) {
	fputfmt(stdout, "%c", poparg(s, int32t));
	return 0;
}
static int puti64(state s) {
	fputfmt(stdout, "%D", poparg(s, int64t));
	return 0;
}
static int putf64(state s) {
	fputfmt(stdout, "%F", poparg(s, flt64t));
	return 0;
}

#endif

static struct lfun {
	int (*call)(state);
	const char* proto;
	symn sym;
	// TODO: check these values
	int08t chk, pop, pad[2];
}
libcfnc[256] = {
	{libHalt, "Halt", NULL, 0, 0},
#ifdef useBuiltInFuncs
	//{ MATH
	{f64abs, "flt64 abs(flt64 x)"},
	{f64sin, "flt64 sin(flt64 x)"},
	{f64cos, "flt64 cos(flt64 x)"},
	{f64tan, "flt64 tan(flt64 x)"},
	{f64log, "flt64 log(flt64 x)"},
	{f64exp, "flt64 exp(flt64 x)"},
	{f64pow, "flt64 pow(flt64 x, flt64 y)"},
	{f64sqrt, "flt64 sqrt(flt64 x)"},
	{f64atan2, "flt64 atan2(flt64 x, flt64 y)"},

	//~ {f64lg2, "flt64 log2(flt64 x)"},
	//~ {f64xp2, "flt64 exp2(flt64 x)"},
	//}

	//{ BITS
	{b32btc, "int32 btc(int32 x)"},		// bitcount
	{b32bsf, "int32 bsf(int32 x)"},
	{b32bsr, "int32 bsr(int32 x)"},
	{b32swp, "int32 swp(int32 x)"},
	{b32hib, "int32 bhi(int32 x)"},
	{b32zxt, "int32 zxt(int32 val, int offs, int bits)"},
	{b32sxt, "int32 sxt(int32 val, int offs, int bits)"},

	{b64shl, "int64 shl(int64 x, int32 y)"},
	{b64shr, "int64 shr(int64 x, int32 y)"},
	{b64sar, "int64 sar(int64 x, int32 y)"},
	{b64and, "int64 and(int64 x, int64 y)"},
	{b64ior, "int64  or(int64 x, int64 y)"},
	{b64xor, "int64 xor(int64 x, int64 y)"},
	{b64zxt, "int64 zxt(int64 val, int offs, int bits)"},
	{b64sxt, "int64 sxt(int64 val, int offs, int bits)"},
	//}

	//{ IO/MEM/EXIT
	//~ {memmgr, "pointer memmgr(pointer old, int32 cnt, int allign)"},		// allocate, reallocate, free
	//~ {memset, "pointer memset(pointer dst, byte src, int32 cnt)"},
	//~ {memcpy, "pointer memcpy(pointer dst, pointer src, int32 cnt)"},
	{putchr, "void putchr(int32 val)"},
	{puti64, "void puti64(int64 val)"},
	{putf64, "void putf64(flt64 val)"},
	//}

	// include some of the compiler functions
	// for reflection. (lookup, logger, exec as (ccinit, compile, execute))

#endif
	{NULL},
};

int libcall(state s, int libc(state), const char* proto) {
	static int libccnt = 0;
	int stdiff = 0;
	symn arg, sym;

	// TODO: can skip this
	if (!s->cc && !ccInit(s)) {
		return -1;
	}

	/*if (!libc && proto && strcmp(proto, "print") == 0) {
		int i;
		for (i = 0; libcfnc[i].proto; ++i) {
			fputfmt(stdout, "%s\n", libcfnc[i].proto);
		}
	}// */

	if (!libccnt && !libc/*  && strcmp(proto, "defaults") == 0 */) {
		libccnt = 1;
		if (!libc) while (libcfnc[libccnt].proto) {
			int i = libccnt;
			libcall(s, libcfnc[i].call, libcfnc[i].proto);
		}
		return libccnt;
	}

	if (libccnt >= (sizeof(libcfnc) / sizeof(*libcfnc))) {
		error(s, 0, "to many functions on install('%s')", proto);
		return -1;
	}

	libcfnc[libccnt].call = libc;
	libcfnc[libccnt].proto = proto;

	if (!libc || !proto)
		return 0;

	sym = install(s->cc, libcfnc[libccnt].proto, -1, 0, 0);

	if (sym == NULL) {
		error(s, 0, "install('%s')", proto);
		return -2;
	}

	for (arg = sym->args; arg; arg = arg->next)
		stdiff += sizeOf(arg->type);
	libcfnc[libccnt].chk = stdiff / 4;
	stdiff -= sizeOf(sym->type);
	libcfnc[libccnt].pop = stdiff / 4;

	libcfnc[libccnt].sym = sym;
	sym->offs = -libccnt;

	libccnt += 1;

	return libccnt - 1;
}
//}

static inline bcde getip(vmEnv s, int pos) {
	return (bcde)(s->_ptr + pos);
}

int emit(vmEnv s, int opc, ...) {
	bcde ip = 0;
	stkval arg = *(stkval*)(&opc + 1);

	if (opc == get_ip) {
		return s->cs;
	}
	if (opc == get_sp) {
		return -s->ss;
	}

	if (opc == set_sp) {
		s->ss = arg.i4;
		return s->cs;
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
			//~ debug("line = %d", arg.i4);
			line = arg.i4;
		}
		return s->pc;
	}

	if (s->_end - s->_ptr < 16) {
		debug("memory overrun");
		return 0;
	}

	else if (opc == opc_neg) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_neg; break;
		case TYPE_i64: opc = i64_neg; break;
		case TYPE_f32: opc = f32_neg; break;
		case TYPE_f64: opc = f64_neg; break;
		//~ case TYPE_pf2: opc = p4d_neg; break;
		//~ case TYPE_pf4: opc = p4f_neg; break;
	}
	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_add; break;
		case TYPE_i64: opc = i64_add; break;
		case TYPE_f32: opc = f32_add; break;
		case TYPE_f64: opc = f64_add; break;
		//~ case TYPE_pf2: opc = p4d_add; break;
		//~ case TYPE_pf4: opc = p4f_add; break;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_sub; break;
		case TYPE_i64: opc = i64_sub; break;
		case TYPE_f32: opc = f32_sub; break;
		case TYPE_f64: opc = f64_sub; break;
		//~ case TYPE_pf2: opc = p4d_sub; break;
		//~ case TYPE_pf4: opc = p4f_sub; break;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32: opc = u32_mul; break;
		case TYPE_i32: opc = i32_mul; break;
		case TYPE_i64: opc = i64_mul; break;
		case TYPE_f32: opc = f32_mul; break;
		case TYPE_f64: opc = f64_mul; break;
		//~ case TYPE_pf2: opc = p4d_mul; break;
		//~ case TYPE_pf4: opc = p4f_mul; break;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32: opc = u32_div; break;
		case TYPE_i32: opc = i32_div; break;
		case TYPE_i64: opc = i64_div; break;
		case TYPE_f32: opc = f32_div; break;
		case TYPE_f64: opc = f64_div; break;
		//~ case TYPE_pf2: opc = p4d_div; break;
		//~ case TYPE_pf4: opc = p4f_div; break;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		//~ case TYPE_u32: opc = u32_mod; break;
		case TYPE_i32: opc = i32_mod; break;
		case TYPE_i64: opc = i64_mod; break;
		case TYPE_f32: opc = f32_mod; break;
		case TYPE_f64: opc = f64_mod; break;
		//~ case TYPE_pf2: opc = p4d_mod; break;
		//~ case TYPE_pf4: opc = p4f_mod; break; p4f_crs ???
	}

	// cmp
	else if (opc == opc_ceq) switch (arg.i4) {
		case TYPE_u32: //opc = u32_ceq; break;
		case TYPE_i32: opc = i32_ceq; break;
		case TYPE_f32: opc = f32_ceq; break;
		case TYPE_i64: opc = i64_ceq; break;
		case TYPE_f64: opc = f64_ceq; break;
	}
	else if (opc == opc_clt) switch (arg.i4) {
		case TYPE_u32: opc = u32_clt; break;
		case TYPE_i32: opc = i32_clt; break;
		case TYPE_f32: opc = f32_clt; break;
		case TYPE_i64: opc = i64_clt; break;
		case TYPE_f64: opc = f64_clt; break;
	}
	else if (opc == opc_cgt) switch (arg.i4) {
		case TYPE_u32: opc = u32_cgt; break;
		case TYPE_i32: opc = i32_cgt; break;
		case TYPE_f32: opc = f32_cgt; break;
		case TYPE_i64: opc = i64_cgt; break;
		case TYPE_f64: opc = f64_cgt; break;
	}
	else if (opc == opc_cne) {
		if (emit(s, opc_ceq, arg))
			opc = opc_not;
	}
	else if (opc == opc_cle) {
		if (emit(s, opc_cgt, arg))
			opc = opc_not;
	}
	else if (opc == opc_cge) {
		if (emit(s, opc_clt, arg))
			opc = opc_not;
	}

	// bit
	else if (opc == opc_shl) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_shl; break;
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64: return 0;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_i32: opc = b32_sar; break;
		case TYPE_u32: opc = b32_shr; break;
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64: return 0;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_and; break;
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64: return 0;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_ior; break;
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64: return 0;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_xor; break;
		case TYPE_i64:
		case TYPE_f32:
		case TYPE_f64: return 0;
	}

	// load/store
	/*else if (opc == opc_ldc) {
		if (arg.u8 == 0) opc = opc_ldz;
		else if (arg.u8 <= 0xff) opc = opc_ldc1;
		else if (arg.u8 <= 0xffff) opc = opc_ldc2;
		else if (arg.u8 <= 0xffffffff) opc = opc_ldc4;
		else opc = opc_ldc8;
	}// */
	else if (opc == opc_ldi) switch (arg.i4) {
		case  1: opc = opc_ldi1; break;
		case  2: opc = opc_ldi2; break;
		case  4: opc = opc_ldi4; break;
		case  8: opc = opc_ldi8; break;
		case 16: opc = opc_ldiq; break;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case  1: opc = opc_sti1; break;
		case  2: opc = opc_sti2; break;
		case  4: opc = opc_sti4; break;
		case  8: opc = opc_sti8; break;
		case 16: opc = opc_stiq; break;
	}

	else if (opc == opc_drop) {		// pop nothing
		if (arg.i4 == 0)
			return s->pc;
	}

	else if (opc == opc_inc) {		// TODO: DelMe!
		if (!s->opti)
			return 0;
		ip = getip(s, s->pc);
		switch (ip->opc) {
			case opc_ldsp:
				if (arg.i4 < 0)
					return 0;
				ip->rel += arg.i4;
				return s->pc;
		}
		return 0;
	}

	if (opc > opc_last) {
		//~ fatal("invalid opc_0x%x", opc);
		return 0;
	}
	if (s->opti > 1) {
		if (0) ;
		else if (opc == opc_ldi1) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_ldi2) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}

		else if (opc == opc_ldi4) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup1;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_sti4) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set1;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}

		else if (opc == opc_ldi8) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup2;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_sti8) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set2;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}

		else if (opc == opc_ldiq) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_dup4;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_stiq) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldsp && ((ip->rel & 3) == 0) && ((ip->rel / 4) < 256)) {
				arg.i4 = ip->rel / 4;
				opc = opc_set4;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}

		// conditional jumps
		else if (opc == opc_not) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_not) {
				s->cs = s->pc;
				return s->pc;
			}
		}
		else if (opc == opc_jnz) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_not) {
				s->cs = s->pc;
				opc = opc_jz;
			}
		}
		else if (opc == opc_jz) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_not) {
				s->cs = s->pc;
				opc = opc_jnz;
			}
		}

		/*~:)) ?? others
		if (opc == opc_ldz1) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldz1) {
				opc = opc_ldz2;
				s->cs = s->pc;
				s->ss -= 1;
			}
		}
		else if (opc == opc_ldz2) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_ldz2) {
				opc = opc_ldz4;
				s->cs = s->pc;
				s->ss -= 2;
			}
		}
		else if (opc == opc_dup1) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_dup1 && ip->idx == arg.u4) {
				ip->opc = opc_dup2;
				ip->idx -= 1;
				s->sm += 1;
				return s->pc;
			}
		}
		else if (opc == opc_dup2) {
			ip = getip(s, s->pc);
			if (ip->opc == opc_dup2 && ip->idx == arg.u4) {
				ip->opc = opc_dup4;
				ip->idx -= 2;
				s->sm += 2;
				return s->pc;
			}
		}
		/ *else if (opc == opc_shl || opc == opc_shr || opc == opc_sar) {
			ip = getip(s, s->pc);
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

	}

	ip = getip(s, s->cs);

	s->pc = s->cs;

	ip->opc = opc;
	ip->arg = arg;

	if (opc == opc_ldsp) {
		if (ip->rel != arg.i8)
			return 0;
	}
	else if (opc == opc_spc) {
		if (ip->rel != arg.i8)
			return 0;
	}
	//~ debug("@%04x[ss:%03d]: %09.*A", s->pc, s->ss, s->pc, ip);
	//~ debug("%09.*A", s->pc, ip);

	switch (opc) {		//#exec.c
		//~ error_opc: error(s->s, 0, "invalid opcode: [%02x] %01.*A", ip->opc, s->cs, ip); return 0;
		//~ error_stc: error(s->s, 0, "stack underflow(%d): near opcode %A", s->ss, ip); s->ss = -1; return 0;
		error_opc: error(s->s, 0, "invalid opcode: op[%.*A]", s->pc, ip); return 0;
		error_stc: error(s->s, 0, "stack underflow: op[%.*A]", s->pc, ip); return 0;
		#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
		#define NEXT(__IP, __CHK, __SP)\
			STOP(error_stc, s->ss < (__CHK));\
			s->ss += (__SP);\
			s->cs += (__IP);
		#include "code.h"
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
	tmp.i8 = s->ss + arg;
	if (opc == opc_ldsp) {
		/*
		if (!emit(s, opc))
			return 0;
		if (!emiti32(s, 4 * tmp.i4))
			return 0;
		return emit(s, i32_add);
		//~ */
		tmp.i8 *= 4;
		return emit(s, opc, tmp);
	}
	if (tmp.u4 > 255) {
		fatal("0x%02x(%d)", opc, tmp.u4);
		fputasm(stdout, s->_ptr, s->cs, 0x31);
		return 0;
	}
	if (tmp.u4 > s->ss) {
		fatal("%d", tmp.u4);
		return 0;
	}
	return emit(s, opc, tmp);
}
int emitint(vmEnv s, int opc, int64t arg) {
	stkval tmp;
	tmp.i8 = arg;
	return emit(s, opc, tmp);
}
int emitref(vmEnv s, int opc, int offset) {
	switch (opc) {
		case opc_ldcr:
			if (offset < 0)
				return emitidx(s, opc_ldsp, offset);
			return emitint(s, opc_ldcr, offset);
		case opc_call:
			if (offset < 0)
				return emitint(s, opc_libc, -offset);
			return emitint(s, opc_call, s->pc - offset);
		// */
	}
	debug("FixMe");
	return 0;
}

int fixjump(vmEnv s, int src, int dst, int stc) {
	bcde ip = getip(s, src);

	if (src >= 0) switch (ip->opc) {
		default: fatal("FixMe");
		//~ case opc_ldsp:
		//~ case opc_call:
		case opc_jmp:
		case opc_jnz:
		case opc_jz: ip->rel = dst - src; break;
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

typedef enum {
	doInit,		// initialize, master is runing
	doTask,		// start a new processor
	doWait,		// wait for childs
	//~ doBrk,		// 
	//~ getNext,	// get next working cell
	//~ doSync = 1;		// syncronize
} doWhat;
static int lobit(int a) {return a & -a;}
static int mtt(vmEnv ee, doWhat cmd, cell pu, int n) {
	static int workers = 0;
	switch (cmd) {
		case doInit: {			// master thread is running
			int i;
			workers = 1;
			for (i = 0; i < n; ++i) {
				pu[i].ip = 0;
			}
		} break;
		case doTask: if (workers) {		// uninitialized or no more processors left
			// the lowest free processor
			int fpu = lobit(~workers);
			workers |= fpu;
			return fpu;
		} break;
		case doWait: {			// master thread is running
			workers = 1;
		} break;
		/*case doBrk: if (ee->dbg) {
			ee->dbg(ee, ....);
		} break;*/
	}
	return 0;
}

/** exec
 * executes on a single thread
 * @arg env: enviroment
 * @arg cc: cell count
 * @arg ss: stack size
 * @return: error code
**/
static int dbugpu(vmEnv vm, cell pu, dbgf dbg) {
	typedef uns32t *stkptr;
	typedef uns08t *memptr;

	register bcde ip;
	stkptr st = (void*)pu->sp;
	memptr mp = (void*)vm->_mem;

	while ((ip = (void*)pu->ip) != 0) {

		register stkptr sp = (void*)pu->sp;

		if (dbg && dbg(vm, 0, ip, (long*)sp, st - sp))
			return -9;

		switch (ip->opc) {		// exec
			//~ error_opc: error(vm->s, 0, "invalid opcode: .%04x: %02x", (char*)ip - (char*)(vm->_mem), ip->opc); return -1;
			error_opc: error(vm->s, 0, "invalid opcode: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip); return -2;
			error_ovf: error(vm->s, 0, "stack overflow: op[%.*A] / %d", (char*)ip - (char*)(vm->_mem), ip, (pu->sp - pu->bp)/4); return -2;
			error_mem: error(vm->s, 0, "segmentation fault: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip); return -2;
			error_div: error(vm->s, 0, "division by zero: op[%.*A]", (char*)ip - (char*)(vm->_mem), ip); return -3;
			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			//~ #define FLGS
			#include "code.h"
		}
	}

	if (dbg)
		dbg(vm, 0, NULL, (long*)pu->sp, (vm->_end - pu->sp) / 4);
	return 0;
}

int exec(vmEnv vm, unsigned ss, dbgf dbg) {
	struct cell pu[1];

	pu->ip = vm->_mem + vm->pc;
	pu->bp = vm->_end - ss;
	pu->sp = vm->_end;

	if ((((int)pu->sp) & 3)) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}

	if (pu->bp < pu->ip) {
		error(vm->s, 0, "invalid statck size");
		return -99;
	}

	return dbugpu(vm, pu, dbg);
}

void fputopc(FILE *fout, unsigned char* ptr, int len, int offs) {
	int i;
	//~ unsigned char* ptr = (unsigned char*)ip;
	bcde ip = (bcde)ptr;

	if (offs >= 0)
		fputfmt(fout, ".%04x: ", offs);

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
	else
		fputfmt(fout, "opc%02x", ip->opc);

	switch (ip->opc) {
		case opc_ldsp:
		case opc_spc:
			fprintf(fout, " %+d", ip->rel);
			break;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
			if (offs < 0)
				fprintf(fout, " %+d", ip->rel);
			else
				fprintf(fout, " .%04x", offs + ip->rel);
			break;
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
		case opc_loc:
		case opc_drop:
			fprintf(fout, " %d", ip->idx);
			break;

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			fputfmt(fout, " sp(%d)", ip->idx);
			break;

		case opc_ldc1: fputfmt(fout, " %d", ip->arg.i1); break;
		case opc_ldc2: fputfmt(fout, " %d", ip->arg.i2); break;
		case opc_ldc4: fputfmt(fout, " %d", ip->arg.i4); break;
		case opc_ldc8: fputfmt(fout, " %D", ip->arg.i8); break;
		case opc_ldcf: fputfmt(fout, " %f", ip->arg.f4); break;
		case opc_ldcF: fputfmt(fout, " %F", ip->arg.f8); break;
		case opc_ldcr: fputfmt(fout, " %x", ip->arg.u4); break;

		case opc_libc:
			if (libcfnc[ip->idx].sym)
				fputfmt(fout, " %-T", libcfnc[ip->idx].sym);
			else
				fputfmt(fout, " %s", libcfnc[ip->idx].proto);
			break;

		case p4d_swz: {
			char c1 = "xyzw"[ip->idx >> 0 & 3];
			char c2 = "xyzw"[ip->idx >> 2 & 3];
			char c3 = "xyzw"[ip->idx >> 4 & 3];
			char c4 = "xyzw"[ip->idx >> 6 & 3];
			fprintf(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->idx);
		} break;
	}
}
void fputasm(FILE *fout, unsigned char* beg, int len, int mode) {
	int i, is = 12345;//, ss = 0;
	for (i = 0; i < len; i += is) {
		bcde ip = (bcde)(beg + i);

		switch (ip->opc) {
			error_opc: error(NULL, 0, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			#define NEXT(__IP, __CHK, __SP) {if (__IP) is = (__IP);}// ss += (__SP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "code.h"
		}

		//~ if (mode & 0x10)		// offset
			//~ fputfmt(fout, ".%04x: ", i);

		//~ if (mode & 0x20)		// TODO: remove stack size
			//~ fputfmt(fout, "ss(%03d): ", ss);

		fputopc(fout, (void*)ip, mode & 0xf, mode & 0x10 ? i : -1);

		fputc('\n', fout);
	}
}

void dumpasmdbg(FILE *fout, vmEnv s, int mark, int mode) {
	fputasm(fout, (unsigned char*)getip(s, mark), s->cs - mark, mode);
}


void vm_fputval(FILE *fout, symn var, stkval* ref, int flgs) {
	typedef enum {
		//~ flg_type = 0x010000,
		flg_hex  = 0x020000,
		//~ flg_offs = 0x040000,
	} flagbits;

	//~ int type
	symn typ = var->type;

	if (typ && typ->kind == TYPE_enu)
		typ = typ->type;

	switch(typ ? typ->kind : 0) {
		case TYPE_bit:
		case TYPE_int: {
			int64t val;
			switch (castId(typ)) {
				case TYPE_bit:
				case TYPE_u32: switch (typ->size) {
					case 1: val = ref->u1; break;
					case 2: val = ref->u2; break;
					case 4: val = ref->u4; break;
				} break;
				case TYPE_i32: switch (typ->size) {
					case 1: val = ref->i1; break;
					case 2: val = ref->i2; break;
					case 4: val = ref->i4; break;
				} break;
				case TYPE_i64: val = ref->i8; break;
				default: goto TYPE_XXX;
			}
			if (!var->pfmt || *var->pfmt != '%') {
				if (var->kind == TYPE_ref)
					fputfmt(fout, "%T:", var);
				if (flgs & flg_hex)
					fputfmt(fout, "%T[%08X](%D)", typ, val, val);
				else
					fputfmt(fout, "%T(%D)", typ, val);
			}
			else
				fputfmt(fout, var->pfmt + 1, val);
		} break;
		case TYPE_flt: {
			flt64t val = 0;
			switch (typ->size) {
				case 4: val = ref->f4; break;
				case 8: val = ref->f8; break;
			}
			if (!var->pfmt || *var->pfmt != '%') {
				if (var->kind == TYPE_ref)
					fputfmt(fout, "%T:", var);
				if (flgs & flg_hex)
					fputfmt(fout, "%T[%08X](%F)", typ, val, val);
				else
					fputfmt(fout, "%T(%F)", typ, val);
			}
			else
				fputfmt(fout, var->pfmt + 1, val);
		} break;
		case TYPE_rec: {
			symn tmp;
			fputfmt(fout, "%T:%T {", var, typ);
			for (tmp = typ->args; tmp; tmp = tmp->next) {
				if (tmp->kind != TYPE_ref)
					continue;

				if (tmp->pfmt && strcmp(tmp->pfmt, "%") == 0)
					continue;

				if (tmp != typ->args)
					fputfmt(fout, ", ");
				vm_fputval(fout, tmp, (void*)((char*)ref + tmp->offs), 0);
			}
			fputfmt(fout, "}");
		} break;
		case TYPE_arr: {
			int i = 0;
			struct astn tmp;
			symn base = typ->type;
			fputfmt(fout, "%I%T[", flgs & 0xff, var);
			if (eval(&tmp, typ->init) != TYPE_int) {
				fputfmt(fout, "Unknown Dimension]");
				break;
			}
			if (tmp.con.cint > 10)
				tmp.con.cint = 10;
			while (i < tmp.con.cint) {
				if (base->kind == TYPE_arr)
					fputfmt(fout, "\n");

				vm_fputval(fout, typ, (stkval*)((char*)ref + i * sizeOf(base)), ((flgs + 1) & 0xff));
				if (++i < tmp.con.cint)
					fputfmt(fout, ", ");//, isType(var) ? '\n': ' ');
			}
			if (base->kind == TYPE_arr)
				fputfmt(fout, "\n%I", flgs & 0xff);
			fputfmt(fout, "]");
		} break;
		default:
		TYPE_XXX:
			fputfmt(fout, "%T(error):%t, %t", typ, typ->kind, castId(typ));
			break;
	}
}

void vmTags(ccEnv s, char *sptr, int slen) {
	symn ptr;
	FILE *fout = stdout;
	for (ptr = s->defs; ptr; ptr = ptr->next) {
		if (ptr->kind == TYPE_ref && ptr->offs < 0) {
			int spos = slen + ptr->offs;
			stkval* sp = (stkval*)(sptr + 4 * spos);
			symn typ = ptr->type;
			if (ptr->file && ptr->line)
				fputfmt(fout, "%s:%d:", ptr->file, ptr->line);
			fputfmt(fout, "sp(%d)", spos);
			if (spos < 0) {
				fputs(": cannot evaluate", fout);
			}
			else switch(typ ? typ->kind : 0) {
				case TYPE_enu:
					vm_fputval(fout, ptr, sp, 0);
					break;
				default:
					fputs(" = ", fout);
					vm_fputval(fout, ptr, sp, 0);
					break;
			}
			fputc('\n', fout);
		}
	}
}

void vmInfo(FILE* out, vmEnv vm) {
	int i;
	fprintf(out, "stack minimum size: %d\n", vm->sm);
	i = vm->ds; fprintf(out, "data(@.%04x) size: %dM, %dK, %dB\n", 0, i >> 20, i >> 10, i);
	i = vm->cs; fprintf(out, "code(@.%04x) size: %dM, %dK, %dB, %d instructions\n", vm->pc, i >> 20, i >> 10, i, vm->ic);
}

int vmTest() {
	int e = 0;
	struct bcde opc, *ip = &opc;
	opc.arg.i8 = 0;
	for (opc.opc = 0; opc.opc < opc_last; opc.opc++) {
		int err = 0;
		if (opc_tbl[opc.opc].size == 0) continue;
		if (opc_tbl[opc.opc].code != opc.opc) {
			fprintf(stderr, "invalid '%s'[%02x]\n", opc_tbl[opc.opc].name, opc.opc);
			e += err = 1;
		}
		else switch (opc.opc) {
			error_len: e += 1; fputfmt(stderr, "opcode size 0x%02x: '%A'\n", opc.opc, ip); break;
			error_chk: e += 1; fputfmt(stderr, "stack check 0x%02x: '%A'\n", opc.opc, ip); break;
			error_dif: e += 1; fputfmt(stderr, "stack difference 0x%02x: '%A'\n", opc.opc, ip); break;
			error_opc: e += 1; fputfmt(stderr, "unimplemented opcode 0x%02x: '%A'\n", opc.opc, ip); break;
			#define NEXT(__IP, __CHK, __DIF) {\
				if (opc_tbl[opc.opc].size != 0 && opc_tbl[opc.opc].size != (__IP)) goto error_len;\
				if (opc_tbl[opc.opc].chck != 9 && opc_tbl[opc.opc].chck != (__CHK)) goto error_chk;\
				if (opc_tbl[opc.opc].diff != 9 && opc_tbl[opc.opc].diff != (__DIF)) goto error_dif;\
			}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "code.h"
		}
	}
	return e;
}

