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

//#{#region math functions

/*static int f64abs(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, fabs(x));
	return 0;
}// */
static int f64sin(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sin(x));
	return 0;
}
static int f64cos(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, cos(x));
	return 0;
}
static int f64tan(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, tan(x));
	return 0;
}

static int f64log(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, log(x));
	return 0;
}
static int f64exp(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, exp(x));
	return 0;
}
static int f64pow(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, pow(x, y));
	//~ debug("pow(%g, %g) := %g", x, y, pow(x, y));
	return 0;
}
static int f64sqrt(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sqrt(x));
	return 0;
}

static int f64atan2(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, atan2(x, y));
	return 0;
}
//#}#endregion

//#{ int64 ext
static int b64shl(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	int32_t y = argi32(rt, 8);
	reti64(rt, x << y);
	return 0;
}
static int b64shr(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	int32_t y = argi32(rt, 8);
	reti64(rt, x >> y);
	return 0;
}
static int b64sar(state rt, void* _) {
	int64_t x = argi64(rt, 0);
	int32_t y = argi32(rt, 8);
	reti64(rt, x >> y);
	return 0;
}
static int b64and(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	uint64_t y = argi64(rt, 8);
	reti64(rt, x & y);
	return 0;
}
static int b64ior(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	uint64_t y = argi64(rt, 8);
	reti64(rt, x | y);
	return 0;
}
static int b64xor(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	uint64_t y = argi64(rt, 8);
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
//#}

/*/#{#region "bit operations"
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
//#}#endregion */

typedef enum {
	miscOpExit,
	miscOpRand32,
	miscOpTime32,
	miscOpClock32,
	miscOpClocksPS,
	timeOpProc64,
	timeOpClck64,
	miscOpPutStr,
	miscOpPutFmt,
} miscFunc;

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

static int miscCall(state rt, void* data) {
	switch ((miscFunc)(int)data) {
		case miscOpExit: {
			exit(argi32(rt, 0));
			return 0;
		}
		case miscOpRand32: {
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
		case miscOpTime32: {
			reti32(rt, time(NULL));
			return 0;
		}
		case miscOpClock32: {
			reti32(rt, clock());
			return 0;
		}
		case miscOpClocksPS: {
			float64_t ticks = argi32(rt, 0);
			retf64(rt, ticks / CLOCKS_PER_SEC);
			return 0;
		}

		case timeOpProc64: {
			reti64(rt, clockCpu());
			return 0;
		}
		case timeOpClck64: {
			reti64(rt, clockNow());
			return 0;
		}

		case miscOpPutStr: {
			// TODO: check bounds
			fputfmt(stdout, "%s", argref(rt, 0));
			return 0;
		}
		case miscOpPutFmt: {
			char* fmt = argref(rt, 0);
			int64_t arg = argi64(rt, 4);
			fputfmt(stdout, fmt, arg);
			return 0;
		}
	}
	return -1;
}

int install_stdc(state rt, char* file, int level) {
	symn nsp = NULL;		// namespace
	int i, err = 0;
	struct {
		int (*fun)(state, void* data);
		miscFunc data;
		char* def;
	}
	math[] = {
		//~ {f64abs, 0, "float64 abs(float64 x);"},
		{f64sin,   0, "float64 sin(float64 x);"},
		{f64cos,   0, "float64 cos(float64 x);"},
		{f64tan,   0, "float64 tan(float64 x);"},
		{f64log,   0, "float64 log(float64 x);"},
		{f64exp,   0, "float64 exp(float64 x);"},
		{f64pow,   0, "float64 pow(float64 x, float64 y);"},
		{f64sqrt,  0, "float64 sqrt(float64 x);"},
		{f64atan2, 0, "float64 atan2(float64 x, float64 y);"},

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
		// IO/MEM/EXIT

		//~ {miscCall, &miscOpArgc,			"int32 argc();"},
		//~ {miscCall, &miscOpArgv,			"string arg(int arg);"},

		{miscCall, miscOpRand32,		"int32 rand();"},
		//~ {miscCall, timeOpTime32,		"int32 time();"},
		//~ {miscCall, timeOpClock32,		"int32 clock();"},
		//~ {miscCall, timeOpClocksPS,		"float64 clocksPerSec(int32 ticks);"},

		{miscCall, timeOpClck64,		"int64 timeNow();"},
		{miscCall, timeOpProc64,		"int64 timeCpu();"},

		{miscCall, miscOpPutStr,		"void print(string val);"},
		{miscCall, miscOpPutFmt,		"void print(string fmt, int64 val);"},

		// TODO: include some of the compiler functions
		// for reflection. (lookup, import, logger, assert, exec?, ...)

	};
	if (rt->cc->type_i64 != NULL && rt->cc->type_i64->args == NULL) {
		enter(rt->cc, NULL);
		err = err || !ccAddCall(rt, b64shl, NULL, "int64 Shl(int64 Value, int Count);");
		err = err || !ccAddCall(rt, b64shr, NULL, "int64 Shr(int64 Value, int Count);");
		err = err || !ccAddCall(rt, b64sar, NULL, "int64 Sar(int64 Value, int Count);");
		err = err || !ccAddCall(rt, b64and, NULL, "int64 And(int64 Lhs, int64 Rhs);");
		err = err || !ccAddCall(rt, b64ior, NULL, "int64  Or(int64 Lhs, int64 Rhs);");
		err = err || !ccAddCall(rt, b64xor, NULL, "int64 Xor(int64 Lhs, int64 Rhs);");
		rt->cc->type_i64->args = leave(rt->cc, rt->cc->type_i64, 1);
	}
	for (i = 0; i < sizeof(math) / sizeof(*math); i += 1) {
		symn libc = ccAddCall(rt, math[i].fun, (void*)math[i].data, math[i].def);
		if (libc == NULL) {
			return -1;
		}
	}
	/*if ((nsp = ccBegin(rt, "bits"))) {
		for (i = 0; i < sizeof(bits) / sizeof(*bits); i += 1) {
			if (!ccAddCall(rt, bits[i].fun, bits[i].n, bits[i].def)) {
				return -1;
			}
		}
		ccEnd(rt, nsp);
	}*/
	for (i = 0; i < sizeof(misc) / sizeof(*misc); i += 1) {
		symn libc = ccAddCall(rt, misc[i].fun, (void*)misc[i].data, misc[i].def);
		if (libc == NULL) {
			return -1;
		}
	}

	if ((nsp = ccBegin(rt, "System"))) {
		// libcall will return 0 on failure,
		// 'err = err || !libcall...' <=> 'if (!err) err = !libcall...'
		// will skip forward libcalls if an error ocurred

		err = err || !ccAddCall(rt, miscCall, (void*)miscOpExit, "void Exit(int Code);");
		ccEnd(rt, nsp);
	}

	if (!err && file) {
		return ccAddCode(rt, level, file, 1, NULL);
	}
	/*if (!err && file && ccOpen(rt, file, 1, NULL)) {
		return parse(rt->cc, 0, level);
	}*/

	return err;
}
