//~ #include "ccvm.h"
//~ #include <math.h>
//~ libc.c ---------------------------------------------------------------------
#define argsize(__TYPE) ((sizeof(__TYPE) | 3) & ~3)
// 
#define poparg(__ARGV, __TYPE) ((__TYPE*)((__ARGV->argv += argsize(__TYPE))))[-1]
#define setret(__TYPE, __ARGV, __VAL) (*((__TYPE*)(__ARGV->retv)) = (__TYPE)(__VAL))

inline flt32t lcargf32(struct libcargv *args) {
	flt32t result = *((flt32t*)args->argv);
	args->argv += ((sizeof(flt32t) | 3) & ~3);
	return result;
}

void f32abs(struct libcargv *args) {
	flt32t x = poparg(args, flt32t);
	setret(flt32t, args, fabs(x));
}
void f32sin(struct libcargv *args) {
	flt32t x = poparg(args, flt32t);
	setret(flt32t, args, sin(x));
}
void f32cos(struct libcargv *args) {
	flt32t x = poparg(args, flt32t);
	setret(flt32t, args, cos(x));
}
void f32tan(struct libcargv *args) {
	flt32t x = poparg(args, flt32t);
	setret(flt32t, args, tan(x));
}

void f64abs(struct libcargv *args) {
	flt64t x = poparg(args, flt64t);
	setret(flt64t, args, fabs(x));
}
void f64sin(struct libcargv *args) {
	flt64t x = poparg(args, flt64t);
	setret(flt64t, args, sin(x));
}
void f64cos(struct libcargv *args) {
	flt64t x = poparg(args, flt64t);
	setret(flt64t, args, cos(x));
}
void f64atan2(struct libcargv *args) {
	flt64t x = poparg(args, flt64t);
	flt64t y = poparg(args, flt64t);
	setret(flt64t, args, atan2(x, y));
	//~ debug("flt64t atan2(flt64t x(%G), flt64t y(%G)) = %G", x, y, atan2(x, y));
}

/*void f64lg2(struct libcargv *args) {
	double log2(double);
	flt64t *sp = stk;
	*sp = log2(*sp);
}
void f64_xp2(struct libcargv *args) {
	double exp2(double);
	flt64t *sp = stk;
	*sp = exp2(*sp);
}*/
void f64log(struct libcargv *args) {
	flt64t x = poparg(args, flt64t);
	setret(flt64t, args, log(x));
}
void f64exp(struct libcargv *args) {
	flt64t x = poparg(args, flt64t);
	setret(flt64t, args, exp(x));
}

void b32btc(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8);
	x += (x >> 16);
	setret(uns32t, args, x & 0x3f);
}
void b32bsf(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	int ans = 0;
	if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { ans +=  1; }
	setret(uns32t, args, x ? ans : -1);
}
void b32bsr(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	unsigned ans = 0;
	if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { ans +=  1; }
	setret(uns32t, args, x ? ans : -1);
}
void b32swp(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	setret(uns32t, args, (x >> 16) | (x << 16));
}
void b32hib(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	setret(uns32t, args, x - (x >> 1));
}
void b32lob(struct libcargv *args) ;
void b32shl(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	uns32t y = poparg(args, uns32t);
	setret(uns32t, args, x << y);
	//~ debug("uns32 shl(uns32t x(%d), uns32t y(%d)) = %d", x, y, x << y);
}
void b32shr(struct libcargv *args) {
	uns32t x = poparg(args, uns32t);
	uns32t y = poparg(args, uns32t);
	setret(uns32t, args, x >> y);
	//~ debug("uns32 shl(uns32 x(%d), uns32 y(%d)) = %d", x, y, x >> y);
}
void b32sar(struct libcargv *args) {
	int32t x = poparg(args, int32t);
	int32t y = poparg(args, int32t);
	setret(int32t, args, x >> y);
	//~ debug("int32 shl(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}

void b32zxt(struct libcargv *args) {
	uns32t val = poparg(args, int32t);
	int32t ofs = poparg(args, int32t);
	int32t cnt = poparg(args, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, args, val >> (32 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}
void b32sxt(struct libcargv *args) {
	int32t val = poparg(args, int32t);
	int32t ofs = poparg(args, int32t);
	int32t cnt = poparg(args, int32t);
	val <<= 32 - (ofs + cnt);
	setret(int32t, args, val >> (32 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}
void b64zxt(struct libcargv *args) {
	uns64t val = poparg(args, int64t);
	int32t ofs = poparg(args, int32t);
	int32t cnt = poparg(args, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, args, val >> (64 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}
void b64sxt(struct libcargv *args) {
	int64t val = poparg(args, int64t);
	int32t ofs = poparg(args, int32t);
	int32t cnt = poparg(args, int32t);
	val <<= 64 - (ofs + cnt);
	setret(int64t, args, val >> (64 - cnt));
	//~ debug("int32 zxt(int32 x(%d), int32 y(%d)) = %d", x, y, x >> y);
}

void b64shl(struct libcargv *args) {
	uns64t x = poparg(args, uns64t);
	int32t y = poparg(args, int32t);
	setret(uns64t, args, x << y);
	//~ debug("int64 shl(int64 x(%D), int32 y(%D)) = %D", x, y, x << y);
}
void b64shr(struct libcargv *args) {
	uns64t x = poparg(args, uns64t);
	int32t y = poparg(args, int32t);
	setret(uns64t, args, x >> y);
	//~ debug("int64 shl(int64 x(%D), int32 y(%D)) = %D", x, y, x << y);
}
void b64sar(struct libcargv *args) {
	int64t x = poparg(args, uns64t);
	int32t y = poparg(args, int32t);
	setret(uns64t, args, x >> y);
	//~ debug("int64 shl(int64 x(%D), int32 y(%D)) = %D", x, y, x << y);
}

static struct lfun {
	void (*call)(struct libcargv*);
	const char* proto;//, *name;
	defn sym;
	uns08t ret, arg, pop;
}
libcfnc[] = {
	{f32abs, "flt32 abs(flt32 x)", 0},
	{f32sin, "flt32 sin(flt32 x)", 0},
	{f32cos, "flt32 cos(flt32 x)", 0},
	{f32tan, "flt32 tan(flt32 x)", 0},

	{f64abs, "flt64 abs(flt64 x)", 0},
	{f64sin, "flt64 sin(flt64 x)", 0},
	{f64cos, "flt64 cos(flt64 x)", 0},
	{f64atan2, "flt64 atan2(flt64 x, flt64 y)", 0},
	//~ {f64_lg2, "flt64 log2(flt64 x)", 0},
	//~ {f64_xp2, "flt64 exp2(flt64 x)", 0},
	{f64log, "flt64 log(flt64 x)", 0},
	{f64exp, "flt64 exp(flt64 x)", 0},

	{b32btc, "int32 btc(uns32 x)", 0},		// bitcount
	{b32btc, "int32 btc(uns32 x)", 0},
	{b32bsf, "int32 bsf(uns32 x)", 0},
	{b32bsr, "int32 bsr(uns32 x)", 0},
	{b32bsf, "int32 bsf(int32 x)", 0},
	{b32bsr, "int32 bsr(int32 x)", 0},
	{b32swp, "int32 bsw(int32 x)", 0},
	{b32swp, "uns32 bsw(uns32 x)", 0},
	{b32hib, "int32 bhi(int32 x)", 0},
	{b32hib, "uns32 bhi(uns32 x)", 0},

	{b32shl, "int32 shl(uns32 x, int32 y)", 0},
	{b32shl, "int32 shl(int32 x, int32 y)", 0},
	{b32shr, "int32 shr(uns32 x, int32 y)", 0},
	{b32sar, "int32 shr(int32 x, int32 y)", 0},

	{b64shl, "int64 shl(int64 x, int32 y)", 0},
	//~ {b64shl, "int64 shl(uns64 x, int32 y)", 0},
	{b64shr, "int64 shr(int64 x, int32 y)", 0},
	{b64sar, "int64 sar(int64 x, int32 y)", 0},
	//~ {b64shr, "int64 shr(uns64 x, int32 y)", 0},
};
