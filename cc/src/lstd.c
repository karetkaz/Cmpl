/*******************************************************************************
 *   File: lstd.c
 *   Date: 2011/06/23
 *   Desc: standard library
 *******************************************************************************
math, print, time libcall functions
*******************************************************************************/
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "core.h"

//{#region math functions

/*static int f64abs(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, fabs(x));
	return 0;
}// */
static int f64sin(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, sin(x));
	return 0;
}
static int f64cos(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, cos(x));
	return 0;
}
static int f64tan(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, tan(x));
	return 0;
}

static int f64log(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, log(x));
	return 0;
}
static int f64exp(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, exp(x));
	return 0;
}
static int f64pow(state rt) {
	float64_t x = popf64(rt);
	float64_t y = popf64(rt);
	retf64(rt, pow(x, y));
	//~ debug("pow(%g, %g) := %g", x, y, pow(x, y));
	return 0;
}
static int f64sqrt(state rt) {
	float64_t x = popf64(rt);
	retf64(rt, sqrt(x));
	return 0;
}

static int f64atan2(state rt) {
	float64_t x = popf64(rt);
	float64_t y = popf64(rt);
	retf64(rt, atan2(x, y));
	return 0;
}
//}#endregion

//{ int64 ext
static int b64shl(state rt) {
	uint64_t x = popi64(rt);
	int32_t y = popi32(rt);
	reti64(rt, x << y);
	return 0;
}
static int b64shr(state rt) {
	uint64_t x = popi64(rt);
	int32_t y = popi32(rt);
	reti64(rt, x >> y);
	return 0;
}
static int b64sar(state rt) {
	int64_t x = popi64(rt);
	int32_t y = popi32(rt);
	reti64(rt, x >> y);
	return 0;
}
static int b64and(state rt) {
	uint64_t x = popi64(rt);
	uint64_t y = popi64(rt);
	reti64(rt, x & y);
	return 0;
}
static int b64ior(state rt) {
	uint64_t x = popi64(rt);
	uint64_t y = popi64(rt);
	reti64(rt, x | y);
	return 0;
}
static int b64xor(state rt) {
	uint64_t x = popi64(rt);
	uint64_t y = popi64(rt);
	reti64(rt, x ^ y);
	return 0;
}
/* unused
static int b64bsf(state rt) {
	uint64_t x = popi64(rt);
	int ans = -1;
	if (x != 0) {
		ans = 0;
		if ((x & 0x00000000ffffffffULL) == 0) { ans += 32; x >>= 32; }
		if ((x & 0x000000000000ffffULL) == 0) { ans += 16; x >>= 16; }
		if ((x & 0x00000000000000ffULL) == 0) { ans +=  8; x >>=  8; }
		if ((x & 0x000000000000000fULL) == 0) { ans +=  4; x >>=  4; }
		if ((x & 0x0000000000000003ULL) == 0) { ans +=  2; x >>=  2; }
		if ((x & 0x0000000000000001ULL) == 0) { ans +=  1; }
	}
	reti32(rt, ans);
	return 0;
}
static int b64bsr(state rt) {
	uint64_t x = popi64(rt);
	int ans = -1;
	if (x != 0) {
		ans = 0;
		if ((x & 0xffffffff00000000ULL) != 0) { ans += 32; x >>= 32; }
		if ((x & 0x00000000ffff0000ULL) != 0) { ans += 16; x >>= 16; }
		if ((x & 0x000000000000ff00ULL) != 0) { ans +=  8; x >>=  8; }
		if ((x & 0x00000000000000f0ULL) != 0) { ans +=  4; x >>=  4; }
		if ((x & 0x000000000000000cULL) != 0) { ans +=  2; x >>=  2; }
		if ((x & 0x0000000000000002ULL) != 0) { ans +=  1; }
	}
	reti32(rt, ans);
	return 0;
}
static int b64hib(state rt) {
	uint64_t x = popi64(rt);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	reti64(rt, x - (x >> 1));
	return 0;
}
static int b64lob(state s) {
	uint64_t x = popi64(s);
	reti64(rt, x & -x);
	return 0;
}*/
//}

/*/{#region "bit operations"
enum bits_funs {
	b64_cmt,
	b64_and,
	b64_or,
	b64_xor,
	b64_shl,	// 
	b64_shr,	// 
	b64_sar,	// 

	b32_btc,	// count ones
	//~ b64_btc,

	b32_bsf,	// scan forward
	b64_bsf,

	b32_bsr,	// scan reverse
	b64_bsr,

	b32_bhi,	// highes bit only
	b64_bhi,

	b32_blo,	// lowest bit only
	b64_blo,

	b32_swp,	// 
	//~ b64_swp,

	b32_zxt,	// zero extend
	b64_zxt,

	b32_sxt,	// sign extend
	b64_sxt,
};
static int bits_call(state rt, int function) {
	switch (function) {
		case b64_cmt: {
			uint64_t x = popi64(rt);
			setret(rt, uint64_t, ~x);
		} return 0;
		case b64_and: {
			uint64_t x = popi64(rt);
			uint64_t y = popi64(rt);
			setret(rt, uint64_t, x & y);
		} return 0;
		case b64_or:  {
			uint64_t x = popi64(rt);
			uint64_t y = popi64(rt);
			setret(rt, uint64_t, x | y);
		} return 0;
		case b64_xor: {
			uint64_t x = popi64(rt);
			uint64_t y = popi64(rt);
			setret(rt, uint64_t, x ^ y);
		} return 0;
		case b64_shl: {
			uint64_t x = popi64(rt);
			uint32_t y = popi32(rt);
			setret(rt, uint64_t, x << y);
		} return 0;
		case b64_shr: {
			uint64_t x = popi64(rt);
			uint32_t y = popi32(rt);
			setret(rt, uint64_t, x >> y);
			//~ debug("%D >> %d = %D", x, y, x >> y);
		} return 0;
		case b64_sar: {
			int64_t x = popi64(rt);
			uint32_t y = popi32(rt);
			setret(rt, uint64_t, x >> y);
		} return 0;

		case b32_btc: {
			uint32_t x = popi32(rt);
			x -= ((x >> 1) & 0x55555555);
			x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
			x = (((x >> 4) + x) & 0x0f0f0f0f);
			x += (x >> 8) + (x >> 16);
			setret(rt, uint32_t, x & 0x3f);
		} return 0;
		//case b64_btc:

		case b32_bsf: {
			uint32_t x = popi32(rt);
			int ans = 0;
			if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
			if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
			if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
			if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000001) == 0) { ans +=  1; }
			setret(rt, uint32_t, x ? ans : -1);
		} return 0;
		case b64_bsf: {
			uint64_t x = popi64(rt);
			int ans = -1;
			if (x != 0) {
				ans = 0;
				if ((x & 0x00000000ffffffffULL) == 0) { ans += 32; x >>= 32; }
				if ((x & 0x000000000000ffffULL) == 0) { ans += 16; x >>= 16; }
				if ((x & 0x00000000000000ffULL) == 0) { ans +=  8; x >>=  8; }
				if ((x & 0x000000000000000fULL) == 0) { ans +=  4; x >>=  4; }
				if ((x & 0x0000000000000003ULL) == 0) { ans +=  2; x >>=  2; }
				if ((x & 0x0000000000000001ULL) == 0) { ans +=  1; }
			}
			reti32(rt, ans);
		} return 0;

		case b32_bsr: {
			uint32_t x = popi32(rt);
			unsigned ans = 0;
			if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
			if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
			if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
			if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000002) != 0) { ans +=  1; }
			setret(rt, uint32_t, x ? ans : -1);
		} return 0;
		case b64_bsr: {
			uint64_t x = popi64(rt);
			int ans = -1;
			if (x != 0) {
				ans = 0;
				if ((x & 0xffffffff00000000ULL) != 0) { ans += 32; x >>= 32; }
				if ((x & 0x00000000ffff0000ULL) != 0) { ans += 16; x >>= 16; }
				if ((x & 0x000000000000ff00ULL) != 0) { ans +=  8; x >>=  8; }
				if ((x & 0x00000000000000f0ULL) != 0) { ans +=  4; x >>=  4; }
				if ((x & 0x000000000000000cULL) != 0) { ans +=  2; x >>=  2; }
				if ((x & 0x0000000000000002ULL) != 0) { ans +=  1; }
			}
			reti32(rt, ans);
		} return 0;

		case b32_bhi: {
			uint32_t x = popi32(rt);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			setret(rt, uint32_t, x - (x >> 1));
		} return 0;
		case b64_bhi: {
			uint64_t x = popi64(rt);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			x |= x >> 32;
			setret(rt, uint64_t, x - (x >> 1));
		} return 0;

		case b32_blo: {
			uint32_t x = popi32(rt);
			setret(rt, uint32_t, x & -x);
		} return 0;
		case b64_blo: {
			uint64_t x = popi64(rt);
			setret(rt, uint64_t, x & -x);
		} return 0;

		case b32_swp: {
			uint32_t x = popi32(rt);
			x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
			x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
			x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
			x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
			setret(rt, uint32_t, (x >> 16) | (x << 16));
		} return 0;
		//case b64_swp:

		case b32_zxt: {
			uint32_t val = popi32(rt);
			int32_t ofs = popi32(rt);
			int32_t cnt = popi32(rt);
			val <<= 32 - (ofs + cnt);
			reti32(rt, val >> (32 - cnt));
		} return 0;
		case b64_zxt: {
			uint64_t val = popi64(rt);
			int32_t ofs = popi32(rt);
			int32_t cnt = popi32(rt);
			val <<= 64 - (ofs + cnt);
			reti64(rt, val >> (64 - cnt));
		} return 0;

		case b32_sxt: {
			int32_t val = popi32(rt);
			int32_t ofs = popi32(rt);
			int32_t cnt = popi32(rt);
			val <<= 32 - (ofs + cnt);
			reti32(rt, val >> (32 - cnt));
		} return 0;
		case b64_sxt: {
			int64_t val = popi64(rt);
			int32_t ofs = popi32(rt);
			int32_t cnt = popi32(rt);
			val <<= 64 - (ofs + cnt);
			reti64(rt, val >> (64 - cnt));
		} return 0;
	}
	return -1;
}
//}#endregion */

static symn miscOpExit = NULL;
static symn miscOpRand32 = NULL;
static symn miscOpTime32 = NULL;
static symn miscOpClock32 = NULL;
static symn miscOpClocksPS = NULL;
static symn timeOpProc64 = NULL;
static symn timeOpClck64 = NULL;
static symn miscOpPutStr = NULL;
static symn miscOpPutFmt = NULL;


static inline int64_t clockCpu() {
	uint64_t now = clock();
	return (now << 32) / CLOCKS_PER_SEC;
	//~ return (now / CLOCKS_PER_SEC) << 32;
}

#if (defined __WATCOMC__) || (defined _MSC_VER)
static inline int64_t clockNow() {
	return clockCpu();
}
#else
#include <sys/time.h>
static inline int64_t clockNow() {
	int64_t now;
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);

	now = ((uint64_t)(tv_now.tv_sec)) << 32;
	now |= ((uint64_t)tv_now.tv_usec << 32) / 1000000;

	return now;
}
#endif

static int miscCall(state rt) {
	if (rt->libc == miscOpExit) {
		exit(popi32(rt));
	}
	if (rt->libc == miscOpRand32) {
		static int initialized = 0;
		int result;
		if (!initialized) {
			srand(time(NULL));
			initialized = 1;
		}
		result = rand() * rand();	// if it gives a 16 bit int
		reti32(rt, result & 0x7fffffff);
		return 0;
	}
	if (rt->libc == miscOpTime32) {
		reti32(rt, time(NULL));
		return 0;
	}
	if (rt->libc == miscOpClock32) {
		reti32(rt, clock());
		return 0;
	}
	if (rt->libc == miscOpClocksPS) {
		float64_t ticks = popi32(rt);
		retf64(rt, ticks / CLOCKS_PER_SEC);
		return 0;
	}

	if (rt->libc == timeOpProc64) {
		reti64(rt, clockCpu());
		return 0;
	}
	if (rt->libc == timeOpClck64) {
		reti64(rt, clockNow());
		return 0;
	}

	if (rt->libc == miscOpPutStr) {
		// TODO: check bounds
		fputfmt(stdout, "%s", popref(rt));
		return 0;
	}
	if (rt->libc == miscOpPutFmt) {
		char *fmt = popref(rt);
		int64_t arg = popi64(rt);
		fputfmt(stdout, fmt, arg);
		return 0;
	}

	return -1;
}

int install_stdc(state rt, char* file, int level) {
	symn nsp = NULL;		// namespace
	int i, err = 0;
	struct {
		int (*fun)(state);
		symn *fsym;
		char *def;
	}
	math[] = {
		//~ {f64abs, 0, "float64 abs(float64 x);"},
		{f64sin,   NULL, "float64 sin(float64 x);"},
		{f64cos,   NULL, "float64 cos(float64 x);"},
		{f64tan,   NULL, "float64 tan(float64 x);"},
		{f64log,   NULL, "float64 log(float64 x);"},
		{f64exp,   NULL, "float64 exp(float64 x);"},
		{f64pow,   NULL, "float64 pow(float64 x, float64 y);"},
		{f64sqrt,  NULL, "float64 sqrt(float64 x);"},
		{f64atan2, NULL, "float64 atan2(float64 x, float64 y);"},

		//~ {f64lg2, "float64 log2(float64 x);"},
		//~ {f64xp2, "float64 exp2(float64 x);"},
	},
	/*bits[] = {
		{bits_call, b64_cmt, "int64 cmt(int64 x);"},
		{bits_call, b64_and, "int64 and(int64 x, int64 y);"},
		{bits_call, b64_or,  "int64 or(int64 x, int64 y);"},
		{bits_call, b64_xor, "int64 xor(int64 x, int64 y);"},
		{bits_call, b64_shl, "int64 shl(int64 x, int32 y);"},
		{bits_call, b64_shr, "int64 shr(int64 x, int32 y);"},
		{bits_call, b64_sar, "int64 sar(int64 x, int32 y);"},
		//~ {bits_call, b64_btc, "int32 btc(int64 x);"},
		{bits_call, b64_bsf, "int32 bsf(int64 x);"},
		{bits_call, b64_bsr, "int32 bsr(int64 x);"},
		{bits_call, b64_bhi, "int32 bhi(int64 x);"},
		{bits_call, b64_blo, "int32 blo(int64 x);"},
		//~ {bits_call, b64_swp, "int64 swp(int64 x);"},
		{bits_call, b64_zxt, "int64 zxt(int64 val, int offs, int bits);"},
		{bits_call, b64_sxt, "int64 sxt(int64 val, int offs, int bits);"},
		{bits_call, b32_btc, "int32 btc(int32 x);"},
		{bits_call, b32_bsf, "int32 bsf(int32 x);"},
		{bits_call, b32_bsr, "int32 bsr(int32 x);"},
		{bits_call, b32_bhi, "int32 bhi(int32 x);"},
		{bits_call, b32_blo, "int32 blo(int32 x);"},
		{bits_call, b32_swp, "int32 swp(int32 x);"},
		{bits_call, b32_zxt, "int32 zxt(int32 val, int offs, int bits);"},
		{bits_call, b32_sxt, "int32 sxt(int32 val, int offs, int bits);"},
	},// */
	misc[] = {
		//{ IO/MEM/EXIT

		//~ {miscCall, &miscOpArgc,			"int32 argc();"},
		//~ {miscCall, &miscOpArgv,			"string arg(int arg);"},

		{miscCall, &miscOpRand32,		"int32 rand();"},
		//~ {miscCall, &timeOpTime32,		"int32 time();"},
		//~ {miscCall, &timeOpClock32,		"int32 clock();"},
		//~ {miscCall, &timeOpClocksPS,		"float64 clocksPerSec(int32 ticks);"},

		{miscCall, &timeOpClck64,		"int64 timeNow();"},
		{miscCall, &timeOpProc64,		"int64 timeCpu();"},

		{miscCall, &miscOpPutStr,		"void print(string val);"},
		{miscCall, &miscOpPutFmt,		"void print(string fmt, int64 val);"},
		//}

		// TODO: include some of the compiler functions
		// for reflection. (lookup, import, logger, assert, exec?, ...)

	};
	if (rt->cc->type_i64 != NULL && rt->cc->type_i64->args == NULL) {
		enter(rt->cc, NULL);
		err = err || !libcall(rt, b64shl, "int64 Shl(int64 Value, int Count);");
		err = err || !libcall(rt, b64shr, "int64 Shr(int64 Value, int Count);");
		err = err || !libcall(rt, b64sar, "int64 Sar(int64 Value, int Count);");
		err = err || !libcall(rt, b64and, "int64 And(int64 Lhs, int64 Rhs);");
		err = err || !libcall(rt, b64ior, "int64  Or(int64 Lhs, int64 Rhs);");
		err = err || !libcall(rt, b64xor, "int64 Xor(int64 Lhs, int64 Rhs);");
		rt->cc->type_i64->args = leave(rt->cc, rt->cc->type_i64, 1);
	}
	for (i = 0; i < sizeof(math) / sizeof(*math); i += 1) {
		symn libc = libcall(rt, math[i].fun, math[i].def);
		if (libc == NULL) {
			return -1;
		}
		if (math[i].fsym != NULL) {
			*math[i].fsym = libc;
		}
	}
	/*if ((nsp = ccBegin(rt, "bits"))) {
		for (i = 0; i < sizeof(bits) / sizeof(*bits); i += 1) {
			if (!libcall(rt, bits[i].fun, bits[i].n, bits[i].def)) {
				return -1;
			}
		}
		ccEnd(rt, nsp);
	}*/
	for (i = 0; i < sizeof(misc) / sizeof(*misc); i += 1) {
		symn libc = libcall(rt, misc[i].fun, misc[i].def);
		if (libc == NULL) {
			return -1;
		}
		if (misc[i].fsym != NULL) {
			*misc[i].fsym = libc;
		}
	}

	if ((nsp = ccBegin(rt, "system"))) {
		// libcall will return 0 on failure,
		// 'err = err || !libcall...' <=> 'if (!err) err = !libcall...'
		// will skip forward libcalls if an error ocurred

		err = err || !(miscOpExit = libcall(rt, miscCall, "void Exit(int Code);"));
		ccEnd(rt, nsp);
	}

	if (!err && file && ccOpen(rt, file, 1, NULL)) {
		return parse(rt->cc, 0, level);
	}

	return err;
}
