#include <math.h>
#include <string.h>
#include "ccvm.h"

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
		/*struct {				// extended3: 4 bytes `res := lhs OP rhs`
			uns32t opc:4;		// 0 ... 15

			uns32t mem:2;		// mem
			uns32t res:6;		// res
			uns32t lhs:6;		// lhs
			uns32t rhs:6;		// rhs
			//{ --8<-------------------------------------------
			void *res = sp + ip->ext.res;
			void *lhs = sp + ip->ext.lhs;
			void *rhs = sp + ip->ext.rhs;
			switch (ip->ext.mem) {
				case 1: res = *(void**)res; break;
				case 2: lhs = *(void**)lhs; break;
				case 3: rhs = *(void**)rhs; break;
			}
			switch (ip->ext.opc) {
				case ext_add: ...add(res, lhs, rhs); break;
				case ext_...:
			}
			//} --8<-------------------------------------------
		}ext;// */
		struct {				// extended3: 5 - 8 bytes
			uns32t opc:4;		// 0 ... 15

			uns32t imm:1;		// 
			uns32t tmp:1;		// 
			uns32t mem:2;		// ???
			union {
				struct {				// indexed: sp[res] := sp[lhs] OP sp[rhs]
					uns32t res:8;		// res
					uns32t lhs:8;		// lhs
					uns32t rhs:8;		// rhs
					//~ uns32t xxx:8;		// ???
				};
				struct {				// immediate sp(0) := sp(0) OP (val | [val])
					uns32t val:32;		// imm
				};
			};
			/*{ --8<-------------------------------------------
			void *res, *lhs, *rhs;
			if (imm) {
				res = sp + ip->ext.res;
				rhs = ip->ext.val;
				switch (ip->ext.mem) {
					case 1: res = memAt(s, res); break;
					case 2: 
					case 3: rhs = memAt(s, rhs); break;
				}
				lhs = res;
			}
			else {
				res = sp + ip->ext.res;
				lhs = sp + ip->ext.lhs;
				rhs = sp + ip->ext.rhs;
				switch (ip->ext.mem) {
					case 1: res = memAt(s, res); break;
					case 2: lhs = memAt(s, lhs); break;
					case 3: rhs = memAt(s, rhs); break;
				}
			}
			switch (ip->ext.opc) {
				case ext_add: ...add(res, lhs, rhs); break;
				case ext_...:
			}
			// */
		}ext;// */
		/*struct {				// extended ?(x86)?
			uint8_t opc:4;		// 0 ... 15
			uint8_t opt:2;		// idx(3) / rev(3) / imm(6) / mem(6)
			uint8_t mem:2;		// mem size
			union {
				uint8_t si[4];
				val32t	arg;
			};
		/+extended opcodes ???argc = 1: [size:2][type:2][code:4]
			type:
			ext_set = 0,		// 3 / sp(n) = opr(sp(n), sp(0)); pop(size & 1)				// a += b
			ext_dup = 1,		// 3 / sp(0) = opr(sp(0), sp(n)); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c
			ext_sto = 2,		// 6 / mp[n] = opr(mp[n], sp(0)); pop(size & 1)				// a += b
			ext_lod = 3,		// 6 / sp(0) = opr(sp(0), mp[n]); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c

			switch (type) {
				case ext_idx : ip += 3; {
					lhs = sp + u1[ip + 2];
					rhs = sp + 0;
					switch (size) {
						case 0 : dst = (sp -= 1);	// new result	push(opr(sp(n) sp(0)))
						case 1 : 					sp(0) = opr(sp(0) sp(n))
						case 2 : dst = (sp -= 1);	// get result	sp(n) = opr(sp(n) sp(0))
						case 3 : dst = (lhs);		// set result	sp(n) = opr(sp(n) sp(0)); pop
					}
				}
				dst = ip + 2
				lhs = sp(ip[2])
				rhs = ip(0)
			}

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

//~ #define useBuiltInFuncs
//{ libc.c ---------------------------------------------------------------------
//~ #pragma intrinsic(log,cos,sin,tan,sqrt,fabs,pow,atan2,fmod)
//~ #pragma intrinsic(acos,asin,atan,cosh,exp,log10,sinh,tanh)

#ifdef useBuiltInFuncs
static void f64abs(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, abs(x));
}
static void f64sin(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sin(x));
}
static void f64cos(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, cos(x));
}
static void f64tan(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, tan(x));
}

static void f64log(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, log(x));
}
static void f64exp(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, exp(x));
}
static void f64pow(state s) {
	flt64t x = poparg(s, flt64t);
	flt64t y = poparg(s, flt64t);
	setret(flt64t, s, pow(x, y));
}
static void f64sqrt(state s) {
	flt64t x = poparg(s, flt64t);
	setret(flt64t, s, sqrt(x));
}

static void f64atan2(state s) {
	flt64t x = poparg(s, flt64t);
	flt64t y = poparg(s, flt64t);
	setret(flt64t, s, atan2(x, y));
}

/*static void f64lg2(state s) {
	double log2(double);
	flt64t *sp = stk;
	*sp = log2(*sp);
}
static void f64_xp2(state s) {
	double exp2(double);
	flt64t *sp = stk;
	*sp = exp2(*sp);
}*/

static void b32btc(state s) {
	uns32t x = poparg(s, uns32t);
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8) + (x >> 16);
	setret(uns32t, s, x & 0x3f);
}
static void b32bsf(state s) {
	uns32t x = poparg(s, uns32t);
	int ans = 0;
	if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { ans +=  1; }
	setret(uns32t, s, x ? ans : -1);
}
static void b32bsr(state s) {
	uns32t x = poparg(s, uns32t);
	unsigned ans = 0;
	if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { ans +=  1; }
	setret(uns32t, s, x ? ans : -1);
}
static void b32swp(state s) {
	uns32t x = poparg(s, uns32t);
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	setret(uns32t, s, (x >> 16) | (x << 16));
}
static void b32hib(state s) {
	uns32t x = poparg(s, uns32t);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	setret(uns32t, s, x - (x >> 1));
}
//~ static void b32lob(state s) := x & -x;

static void b64shl(state s) {
	uns64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x << y);
}
static void b64shr(state s) {
	uns64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x >> y);
}
static void b64sar(state s) {
	int64t x = poparg(s, uns64t);
	int32t y = poparg(s, int32t);
	setret(uns64t, s, x >> y);
}

static void b32zxt(state s) {
	uns32t val = poparg(s, int32t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, s, val >> (32 - cnt));
}
static void b32sxt(state s) {
	int32t val = poparg(s, int32t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, s, val >> (32 - cnt));
}
/*static void b64zxt(state s) {
	uns64t val = poparg(s, int64t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, s, val >> (64 - cnt));
}// */
static void b64sxt(state s) {
	int64t val = poparg(s, int64t);
	int32t ofs = poparg(s, int32t);
	int32t cnt = poparg(s, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, s, val >> (64 - cnt));
}

#endif

static struct lfun {
	void (*call)(state);
	const char* proto;
	symn sym;
	// TODO: check these values
	int08t chk, pop, pad[2];
}
libcfnc[256] = {
	{NULL},
#ifdef useBuiltInFuncs
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

	{b32btc, "int32 btc(uns32 x)"},		// bitcount
	//~ {b32btc, "int32 btc(uns32 x)"},
	{b32bsf, "int32 bsf(int32 x)"},
	{b32bsr, "int32 bsr(int32 x)"},
	{b32swp, "int32 swp(int32 x)"},
	{b32hib, "int32 bhi(int32 x)"},
	{b32hib, "uns32 bhi(uns32 x)"},

	{b64shl, "int64 shl(int64 x, int32 y)"},
	//~ {b64shl, "int64 shl(uns64 x, int32 y)"},
	{b64shr, "int64 shr(int64 x, int32 y)"},
	//~ {b64shr, "int64 shr(uns64 x, int32 y)"},
	{b64sar, "int64 sar(int64 x, int32 y)"},

	//~ {i32i64, "int64 i32i64(int32 val)"},

	{b64sxt, "int64 ext(int64 val, int32 offs, int32 bits)"},
	//~ {b64zxt, "int64 ext(uns64 val, int32 offs, int32 bits)"},
	{b32sxt, "int32 ext(int32 val, int32 offs, int32 bits)"},
	{b32zxt, "int32 ext(uns32 val, int32 offs, int32 bits)"},
#endif
	{NULL},
};

int install_libc(state s, void libc(state), const char* proto) {
	static int libccnt = 0;
	int stdiff = 0;
	symn arg, sym;

	if (!libc && proto && strcmp(proto, "print") == 0) {
		int i;
		for (i = 0; libcfnc[i].proto; ++i) {
			fputfmt(stdout, "%s\n", libcfnc[i].proto);
		}
	}

	if (!libccnt && !libc/*  && strcmp(proto, "defaults") == 0 */) {
		libccnt = 1;
		if (!libc) while (libcfnc[libccnt].proto) {
			int i = libccnt;
			install_libc(s, libcfnc[i].call, libcfnc[i].proto);
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

	sym = install(s->cc, -1, libcfnc[libccnt].proto, 0);

	if (sym == NULL) {
		error(s, 0, "install('%s')", proto);
		return -2;
	}

	for (arg = sym->args; arg; arg = arg->next)
		stdiff += arg->type->size;
	libcfnc[libccnt].chk = stdiff / 4;
	stdiff -= sym->type->size;
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
		return s->ss = arg.i4;
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
		default: return 0;
	}
	else if (opc == opc_add) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_add; break;
		case TYPE_i64: opc = i64_add; break;
		case TYPE_f32: opc = f32_add; break;
		case TYPE_f64: opc = f64_add; break;
		//~ case TYPE_pf2: opc = p4d_add; break;
		//~ case TYPE_pf4: opc = p4f_add; break;
		default: return 0;
	}
	else if (opc == opc_sub) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = i32_sub; break;
		case TYPE_i64: opc = i64_sub; break;
		case TYPE_f32: opc = f32_sub; break;
		case TYPE_f64: opc = f64_sub; break;
		//~ case TYPE_pf2: opc = p4d_sub; break;
		//~ case TYPE_pf4: opc = p4f_sub; break;
		default: return 0;
	}
	else if (opc == opc_mul) switch (arg.i4) {
		case TYPE_u32: opc = u32_mul; break;
		case TYPE_i32: opc = i32_mul; break;
		case TYPE_i64: opc = i64_mul; break;
		case TYPE_f32: opc = f32_mul; break;
		case TYPE_f64: opc = f64_mul; break;
		//~ case TYPE_pf2: opc = p4d_mul; break;
		//~ case TYPE_pf4: opc = p4f_mul; break;
		default: return 0;
	}
	else if (opc == opc_div) switch (arg.i4) {
		case TYPE_u32: opc = u32_div; break;
		case TYPE_i32: opc = i32_div; break;
		case TYPE_i64: opc = i64_div; break;
		case TYPE_f32: opc = f32_div; break;
		case TYPE_f64: opc = f64_div; break;
		//~ case TYPE_pf2: opc = p4d_div; break;
		//~ case TYPE_pf4: opc = p4f_div; break;
		default: return 0;
	}
	else if (opc == opc_mod) switch (arg.i4) {
		//~ case TYPE_u32: opc = u32_mod; break;
		case TYPE_i32: opc = i32_mod; break;
		case TYPE_i64: opc = i64_mod; break;
		case TYPE_f32: opc = f32_mod; break;
		case TYPE_f64: opc = f64_mod; break;
		//~ case TYPE_pf2: opc = p4d_mod; break;
		//~ case TYPE_pf4: opc = p4f_mod; break; p4f_crs ???
		default: return 0;
	}

	// cmp
	else if (opc == opc_ceq) switch (arg.i4) {
		case TYPE_u32: //opc = u32_ceq; break;
		case TYPE_i32: opc = i32_ceq; break;
		case TYPE_f32: opc = f32_ceq; break;
		case TYPE_i64: opc = i64_ceq; break;
		case TYPE_f64: opc = f64_ceq; break;
		default: return 0;
	}
	else if (opc == opc_clt) switch (arg.i4) {
		case TYPE_u32: opc = u32_clt; break;
		case TYPE_i32: opc = i32_clt; break;
		case TYPE_f32: opc = f32_clt; break;
		case TYPE_i64: opc = i64_clt; break;
		case TYPE_f64: opc = f64_clt; break;
		default: return 0;
	}
	else if (opc == opc_cgt) switch (arg.i4) {
		case TYPE_u32: opc = u32_cgt; break;
		case TYPE_i32: opc = i32_cgt; break;
		case TYPE_f32: opc = f32_cgt; break;
		case TYPE_i64: opc = i64_cgt; break;
		case TYPE_f64: opc = f64_cgt; break;
		default: return 0;
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
		default: return 0;
	}
	else if (opc == opc_shr) switch (arg.i4) {
		case TYPE_i32: opc = b32_sar; break;
		case TYPE_u32: opc = b32_shr; break;
		default: return 0;
	}
	else if (opc == opc_and) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_and; break;
		default: return 0;
	}
	else if (opc == opc_ior) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_ior; break;
		default: return 0;
	}
	else if (opc == opc_xor) switch (arg.i4) {
		case TYPE_u32:
		case TYPE_i32: opc = b32_xor; break;
		default: return 0;
	}

	else if (opc == opc_ldi) switch (arg.i4) {
		case  1: opc = opc_ldi1; break;
		case  2: opc = opc_ldi2; break;
		case  4: opc = opc_ldi4; break;
		case  8: opc = opc_ldi8; break;
		case 16: opc = opc_ldiq; break;
		default: return 0;
	}
	else if (opc == opc_sti) switch (arg.i4) {
		case  1: opc = opc_sti1; break;
		case  2: opc = opc_sti2; break;
		case  4: opc = opc_sti4; break;
		case  8: opc = opc_sti8; break;
		case 16: opc = opc_stiq; break;
		default: return 0;
	}

	/*~:)) ?? rollbacks
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
	else if (opc == opc_dup1) {			// TODO 
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
	/ *
	else if (opc == b32_not) {
		ip = getip(s, s->pc);
		if (ip->opc == b32_not) {
			s->cs = s->pc;
			return s->pc;
		}
	}
	else if (opc == opc_jnz || opc == opc_jz) {
		ip = getip(s, s->pc);
		if (ip->opc == b32_not && ip->arg.u1 == 1) {
			opc = (opc == opc_jnz) ? opc_jz : opc_jnz;
			s->cs = s->pc;
		}
	}// * /
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

	if (opc > opc_last) {
		fatal("invalid opc_0x%x", opc);
		return 0;
	}

	if (opc  == opc_pop && !arg.i4)
		return s->pc;

	ip = getip(s, s->cs);
	ip->opc = opc;
	ip->arg = arg;

	s->pc = s->cs;
	//~ debug("@%04x[ss:%03d]: %09A", s->pc, s->ss, ip);
	switch (opc) {		//#exec.c
		error_opc: error(s->s, 0, "invalid opcode: [%02x] %A", ip->opc, ip); return 0;
		error_stc: error(s->s, 0, "stack underflow(%d): near opcode %A", s->ss, ip); return 0;
		#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
		#define NEXT(__IP, __CHK, __SP)\
			STOP(error_stc, s->ss < (__CHK));\
			s->ss += (__SP);\
			s->cs += (__IP);
		//~ #define EXEC(__STMT)
		//~ #define MEMP(__MPTR)
		//~ #define CHECK()
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
	tmp.i4 = s->ss + arg;
	if (opc == opc_ldsp) {
		if (!emit(s, opc))
			return 0;
		if (!emiti32(s, 4*tmp.i4))
			return 0;
		return emit(s, i32_add);
	}
	if (tmp.u4 > 255) {
		error(s->s, 0, "0x%02x(%d)", opc, tmp.u4);
		fputasm(stdout, s->_ptr, s->cs, 0x31);
		return 0;
	}
	if (tmp.u4 > s->ss) {
		error(s->s, 0, "%d", tmp.u4);
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
	bcde ip = getip(s, src);

	if (src >= 0) switch (ip->opc) {
		default: fatal("NoIpHere");
		case opc_jmp:
		case opc_jnz:
		case opc_jz: ip->jmp = dst - src; break;
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

void vm_fputval(FILE *fout, symn typ, stkval* sp, int flgs) {
	typedef enum {
		flg_type = 0x01,
		flg_hex = 0x02,
		flg_offs = 0x04,
	} flagbits;
	switch(typ ? typ->kind : 0) {
		case TYPE_p4x: {
			if (typ == type_f32x4) {
				flt32t *val = (flt32t*)sp;
				fputfmt(fout, "f32x4(%g, %g, %g, %g)", val[0], val[1], val[2], val[3]);
				//~ fputfmt(fout, "f32x4(%g, %g, %g, %g)", sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w);
			}
			else if (typ == type_f64x2) {
				flt64t *val = (flt64t*)sp;
				fputfmt(fout, "f64x2(%G, %G)", val[0], val[1]);
				//~ fputfmt(fout, "f64x2(%G, %G)", sp->pd.x, sp->pd.y);
			}
		} break;
		case TYPE_bit: switch (typ->size) {
			case 1:
				if (flgs & flg_type) fputfmt(fout, "u32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i1);
				fputfmt(fout, "(%u)", sp->i1);
				break;
			case 2:
				if (flgs & flg_type) fputfmt(fout, "u32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i2);
				fputfmt(fout, "(%u)", sp->i2);
				break;
			case 4: 
				if (flgs & flg_type) fputfmt(fout, "u32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i4);
				fputfmt(fout, "(%u)", sp->i4);
				break;
			case 8:
				if (flgs & flg_type) fputfmt(fout, "u64");
				if (flgs & flg_hex) fputfmt(fout, "[%016X]", sp->i8);
				fputfmt(fout, "(%U)", sp->i8);
				break;
			default: goto TYPE_XXX;
		} break;
		case TYPE_int: switch (typ->size) {
			case 1:
				if (flgs & flg_type) fputfmt(fout, "i32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i1);
				fputfmt(fout, "(%d)", sp->i1);
				break;
			case 2:
				if (flgs & flg_type) fputfmt(fout, "i32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i2);
				fputfmt(fout, "(%d)", sp->i2);
				break;
			case 4: 
				if (flgs & flg_type) fputfmt(fout, "i32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i4);
				fputfmt(fout, "(%d)", sp->i4);
				break;
			case 8:
				if (flgs & flg_type) fputfmt(fout, "i64");
				if (flgs & flg_hex) fputfmt(fout, "[%016X]", sp->i8);
				fputfmt(fout, "(%D)", sp->i8);
				break;
			default: goto TYPE_XXX;
		} break;
		case TYPE_flt: switch (typ->size) {
			case 4: 
				if (flgs & flg_type) fputfmt(fout, "f32");
				if (flgs & flg_hex) fputfmt(fout, "[%008x]", sp->i4);
				fputfmt(fout, "(%.30g)", sp->f4);
				break;
			case 8:
				if (flgs & flg_type) fputfmt(fout, "f64");
				if (flgs & flg_hex) fputfmt(fout, "[%016X]", sp->i8);
				fputfmt(fout, "(%.30G)", sp->f8);
				break;
			default: goto TYPE_XXX;
		} break;
		case TYPE_rec: {
			symn tmp;
			fputfmt(fout, "rec{");
			for (tmp = typ->args; tmp; tmp = tmp->next) {
				fputfmt(fout, "%T:", tmp);
				vm_fputval(fout, tmp->type, sp, 1);
				if (tmp->next)
					fputfmt(fout, ", ");
				sp = (void*)((char*)sp + tmp->type->size);
			}
			fputfmt(fout, "}");
		} break;
		case TYPE_ar3: {
			int i = 0;
			struct astn tmp;
			if (!eval(&tmp, typ->init, TYPE_int)) {
				fputfmt(fout, ":array = error");
				break;
			}
			fputfmt(fout, "arr[");
			while (i < tmp.con.cint) {
				vm_fputval(fout, typ->type, sp, 1);
				if (++i < tmp.con.cint)
					fputfmt(fout, ", ");
				sp = (void*)((char*)sp + typ->type->size);
			}
			fputfmt(fout, "]");
			//~ fputfmt(fout, "array[]");
		} break;
		default:
		TYPE_XXX:
			fputfmt(fout, "error", typ);
			break;
	}
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
			error_len: e += 1; debug("opcode size 0x%02x: '%A'", opc.opc, ip); break;
			error_chk: e += 1; debug("stack check 0x%02x: '%A'", opc.opc, ip); break;
			error_dif: e += 1; debug("stack difference 0x%02x: '%A'", opc.opc, ip); break;
			error_opc: e += 1; debug("unimplemented opcode 0x%02x: '%A'", opc.opc, ip); break;
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

void vmTags(ccEnv s, char *sptr, int slen) {
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
				/*case TYPE_rec: {
					symn ttt;
					while()
					fputfmt(fout, ":struct");
				} break;
				case TYPE_ar3: {
					int i = 0;
					struct astn ns;
					if (!eval(&ns, typ->init, TYPE_int)) {
						fputfmt(fout, ":array = error");
						break;
					}
					fputfmt(fout, ":array [");
					while (i < ns.con.cint) {
						vm_fputval(fout, typ->type, sp, 1);
						if (++i < ns.con.cint)
							fputfmt(fout, ", ");
						sp = (void*)((char*)sp + typ->type->size);
					}
					fputfmt(fout, "]");
				} break;*/

				default:
					fputs(" = ", fout);
					vm_fputval(fout, typ, sp, -1);
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
static int execpu(vmEnv vm, cell pu) {
	//~ unsigned char *bp = pu->sp;
	unsigned char *mp = NULL;//vm->_mem;

	while (pu->ip) {

		register bcde ip = (bcde)pu->ip;
		register unsigned char *st = pu->sp;

		switch (ip->opc) {		// exec
			error_opc: error(vm->s, 0, "invalid opcode: [%02x]", ip->opc); return -1;
			error_ovf: error(vm->s, 0, "stack overflow: op[%A]", ip); return -2;
			error_div: error(vm->s, 0, "division by zero: op[%A]", ip); break;//return -3;
			//~ error_mem: error(vm->s, 0, "segmentation fault: op[%A]", ip); return -4;
			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			//~ #define FLGS
			#include "code.h"
		}
	}

	return 0;
}

static int dbugpu(vmEnv vm, cell pu, dbgf dbg) {
	unsigned char *bp = pu->sp;
	unsigned char *mp = NULL;//vm->_mem;

	while (pu->ip) {

		register bcde ip = (bcde)pu->ip;
		register unsigned char *st = pu->sp;

		// we should call here mtt(vm, doBrk, ...)
		if (dbg(vm, 0, ip, (long*)st, (bp - st) / 4))
			return -9;

		switch (ip->opc) {		// exec
			error_opc: error(vm->s, 0, "invalid opcode: [%02x]", ip->opc); return -1;
			error_ovf: error(vm->s, 0, "stack overflow: op[%A]", ip); return -2;
			error_div: error(vm->s, 0, "division by zero: op[%A]", ip); break;//return -3;
			//~ error_mem: error(vm->s, 0, "segmentation fault: op[%A]", ip); return -4;
			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			//~ #define FLGS
			#include "code.h"
		}
	}

	dbg(vm, 0, NULL, (long*)pu->sp, (vm->_end - pu->sp) / 4);
	return 0;
}

int exec(vmEnv vm, unsigned ss, dbgf dbg) {
	struct cell pu[1];

	pu->ip = vm->_mem + vm->pc;
	pu->bp = vm->_end - ss;
	pu->sp = vm->_end;

	return dbg ? dbugpu(vm, pu, dbg) : execpu(vm, pu);
}

/*int execute_emu(vmEnv vm, unsigned cc, unsigned ss, dbgf dbg) {
	struct cell procs[32], *pu = procs;

	register bcde ip;
	register void *mp = NULL;
	register unsigned char *st;

	if (cc > sizeof(procs) / sizeof(*procs)) {
		debug("cells overrun");
		return -1;
	}
	/ *if (ss < env->sm) {
		debug("stack overrun(%d, %d)", ss, env->sm);
		return -1;
	}
	/ *if ((env->cs + ss * cc) >= ?) {
		debug("memory overrun");
		return -1;
	}
	* /

	pu->ip = vm->_mem + vm->pc;
	pu->bp = vm->_end - ss;
	pu->sp = vm->_end;

	if (!mtt(vm, doInit, pu, cc)) {
		debug("thread creation failed");
		return -1;
	}

	while (pu->ip) {

		//~ vm->ic += 1;
		st = pu->sp;			// stack
		ip = (bcde)pu->ip;

		if (dbg && dbg(vm, 0, ip, (long*)st, (vm->_end - st) / 4))
			return -9;

		switch (ip->opc) {		// exec
			error_opc: error(vm->s, 0, "invalid opcode: [%02x]", ip->opc); return -1;
			error_ovf: error(vm->s, 0, "stack overflow: op[%A]", ip); return -2;
			error_div: error(vm->s, 0, "division by zero: op[%A]", ip); return -3;
			//~ error_mem: error(vm->s, 0, "segmentation fault: op[%A]", ip); return -5;
			#define NEXT(__IP, __CHK, __SP) {pu->sp -= 4*(__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#define EXEC
			//~ #define FLGS
			#include "code.h"
		}

		//~ pu = procs + mtt(vm, getNext, procs, pu - procs);
	}

	if (dbg) dbg(vm, 0, NULL, (long*)pu->sp, (vm->_end - pu->sp) / 4);
	return 0;
}*/

void fputopc(FILE *fout, unsigned char* ptr, int len, int offs) {
	int i;
	//~ unsigned char* ptr = (unsigned char*)ip;
	bcde ip = (bcde)ptr;
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
		case opc_loc:
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
		case opc_libc: fputfmt(fout, " %-T", libcfnc[ip->idx].sym); break;
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

void fputasm(FILE *fout, unsigned char* beg, int len, int mode) {
	int is = 12345, ss = 0, i;
	for (i = 0; i < len; i += is) {
		bcde ip = (bcde)(beg + i);

		switch (ip->opc) {
			error_opc: error(NULL, 0, "invalid opcode: %02x '%A'", ip->opc, ip); return;
			#define NEXT(__IP, __CHK, __SP) {is = (__IP); ss += (__SP);}
			#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
			#include "code.h"
		}

		if (mode & 0x10)		// offset
			fputfmt(fout, ".%04x: ", i);

		if (mode & 0x20)		// TODO: remove stack
			fputfmt(fout, "ss(%03d): ", ss);

		fputopc(fout, (void*)ip, mode & 0xf, mode & 0x10 ? i : -1);

		fputc('\n', fout);
	}
}

void dumpasm(FILE *fout, vmEnv s, int mode) {
	vmInfo(fout, s);
	fputasm(fout, s->_mem + s->pc, s->cs, mode);
}

void dumpasmdbg(FILE *fout, vmEnv s, int mark, int mode) {
	fputasm(fout, (unsigned char*)getip(s, mark), s->cs, mode);
}
