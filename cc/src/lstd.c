#include "ccvm.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>

static int f64abs(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, fabs(x));
	return 0;
}
static int f64sin(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, sin(x));
	return 0;
}
static int f64cos(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, cos(x));
	return 0;
}
static int f64tan(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, tan(x));
	return 0;
}

static int f64log(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, log(x));
	return 0;
}
static int f64exp(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, exp(x));
	return 0;
}
static int f64pow(state s) {
	float64_t x = popf64(s);
	float64_t y = popf64(s);
	setret(s, float64_t, pow(x, y));
	//~ debug("pow(%g, %g) := %g", x, y, pow(x, y));
	return 0;
}
static int f64sqrt(state s) {
	float64_t x = popf64(s);
	setret(s, float64_t, sqrt(x));
	return 0;
}

static int f64atan2(state s) {
	float64_t x = popf64(s);
	float64_t y = popf64(s);
	setret(s, float64_t, atan2(x, y));
	return 0;
}

enum miscCalls {

	miscOpRand32,
	miscOpTime32,
	miscOpClock32,
	miscOpClocksPS,

	timeOpProc64,		// spent in cpu
	timeOpClck64,		// spent 

	//~ miscOpPutX64,
	//~ miscOpPutI64,
	//~ miscOpPutF64,
	//~ miscOpPutChr,
	miscOpPutStr,
	miscOpPutFmt,

	miscOpMemMgr,
};

/*static int miscCall(state s) {
	switch (s->func) {
		default: return -1;

		//~ case miscOpArgc:
			//~ setret(s, int32_t, s->ev.argc);
			//~ break;

		//~ case miscOpArgv: // returning a string ?
			//~ setret(s, int32_t, s->ev.???);
			//~ break;

		case miscOpRand32: {
			static int initialized = 0;
			int result;
			if (!initialized) {
				srand(time(NULL));
				initialized = 1;
			}
			result = rand() * rand();	// if it gives a 16 bit int
			setret(s, int32_t, result & 0x7fffffff);
		} break;
		case miscOpTime32: {
			setret(s, int32_t, time(NULL));
		} break;
		case miscOpClock32: {
			setret(s, int32_t, clock());
		} break;
		case miscOpClocksPS: {
			float64_t ticks = popi32(s);
			setret(s, float64_t, ticks / CLOCKS_PER_SEC);
		} break;

		case miscOpPutX64: {
			fputfmt(stdout, "%X", popi64(s));
		} break;
		case miscOpPutI64: {
			fputfmt(stdout, "%D", popi64(s));
		} break;
		case miscOpPutF64: {
			fputfmt(stdout, "%F", popf64(s));
		} break;
		case miscOpPutChr: {
			fputfmt(stdout, "%c", popi32(s));
		} break;
		case miscOpPutStr: {
			// TODO: check bounds
			fputfmt(stdout, "%s", popstr(s));
		} break;

		case miscOpMemMgr: {
			void *old = popref(s);
			int size = popi32(s);
			void *res = rtAlloc(s, old, size);
			setret(s, int32_t, vmOffset(s, res));
			//~ debug("memmgr(%06x, %d): %06x", vmOffset(s, old), size, vmOffset(s, res));
		} break;
	}

	return 0;
}
// */

static inline int64_t clockCpu() {
	uint64_t now = clock();
	return (now << 32) / CLOCKS_PER_SEC;
	//~ return (now / CLOCKS_PER_SEC) << 32;
}

#ifdef __WATCOMC__
static inline int64_t clockNow() {
	return clockCpu();
}
#else
#include <sys/time.h>
static inline int64_t clockNow() {
	int64_t now;
	struct timeval tv_now;
	//~ static int64_t progbeg = 0;
	gettimeofday(&tv_now, NULL);

	now = ((uint64_t)(tv_now.tv_sec)) << 32;
	now |= ((uint64_t)tv_now.tv_usec << 32) / 1000000;

	//~ if (progbeg == 0) {
		//~ progbeg = now - timeproc();
	//~ }

	return now;// - progbeg;
}
#endif

static int miscCall(state s) {
	switch (s->func) {
		default: return -1;

		case miscOpRand32: {
			static int initialized = 0;
			int result;
			if (!initialized) {
				srand(time(NULL));
				initialized = 1;
			}
			result = rand() * rand();	// if it gives a 16 bit int
			setret(s, int32_t, result & 0x7fffffff);
		} break;
		case miscOpTime32: {
			setret(s, int32_t, time(NULL));
		} break;
		case miscOpClock32: {
			setret(s, int32_t, clock());
		} break;
		case miscOpClocksPS: {
			float64_t ticks = popi32(s);
			setret(s, float64_t, ticks / CLOCKS_PER_SEC);
		} break;

		case timeOpProc64: {
			setret(s, int64_t, clockCpu());
		} break;
		case timeOpClck64: {
			setret(s, int64_t, clockNow());
		} break;

		case miscOpPutStr: {
			// TODO: check bounds
			fputfmt(stdout, "%s", popref(s));
		} break;
		case miscOpPutFmt: {
			char *fmt = popref(s);
			int64_t arg = popi64(s);
			fputfmt(stdout, fmt, arg);
		} break;

		case miscOpMemMgr: {
			void *old = popref(s);
			int size = popi32(s);
			void *res = rtAlloc(s, old, size);
			setret(s, int32_t, vmOffset(s, res));
			//~ debug("memmgr(%06x, %d): %06x", vmOffset(s, old), size, vmOffset(s, res));
		} break;
	}

	return 0;
}

int install_stdc(state rt, char* file, int level) {
	int i;//, err = 0;
	struct {
		int (*fun)(state s);
		int n;
		char *def;
	}
	defs[] = {
		//{ MATH
		{f64abs, 0, "float64 abs(float64 x);"},
		{f64sin, 0, "float64 sin(float64 x);"},
		{f64cos, 0, "float64 cos(float64 x);"},
		{f64tan, 0, "float64 tan(float64 x);"},
		{f64log, 0, "float64 log(float64 x);"},
		{f64exp, 0, "float64 exp(float64 x);"},
		{f64pow, 0, "float64 pow(float64 x, float64 y);"},
		{f64sqrt, 0, "float64 sqrt(float64 x);"},
		{f64atan2, 0, "float64 atan2(float64 x, float64 y);"},

		//~ {f64lg2, "float64 log2(float64 x);"},
		//~ {f64xp2, "float64 exp2(float64 x);"},
		//}

		/*/{ BITS
		{b32btc, "int32 btc(int32 x);"},		// bitcount
		{b32bsf, "int32 bsf(int32 x);"},
		{b32bsr, "int32 bsr(int32 x);"},
		{b32swp, "int32 swp(int32 x);"},
		{b32hib, "int32 bhi(int32 x);"},

		{b64bsf, "int32 bsf(int64 x);"},
		{b64bsr, "int32 bsr(int64 x);"},
		{b64lob, "int64 lobit(int64 x);"},
		{b64hib, "int64 hibit(int64 x);"},
		//~ {b64shl, "int64 shl(int64 x, int32 y);"},
		//~ {b64shr, "int64 shr(int64 x, int32 y);"},
		//~ {b64sar, "int64 sar(int64 x, int32 y);"},
		//~ {b64and, "int64 and(int64 x, int64 y);"},
		//~ {b64ior, "int64  or(int64 x, int64 y);"},
		//~ {b64xor, "int64 xor(int64 x, int64 y);"},
		{b64zxt, "int64 zxt(int64 val, int offs, int bits);"},
		{b64sxt, "int64 sxt(int64 val, int offs, int bits);"},
		//} */
	},
	misc[] = {
		//{ IO/MEM/EXIT
		//~ {memmgr, "pointer memmgr(pointer old, int32 cnt, int allign);"},		// allocate, reallocate, free
		//~ {memset, "pointer memset(pointer dst, byte src, int32 cnt);"},
		//~ {memcpy, "pointer memcpy(pointer dst, pointer src, int32 cnt);"},

		//~ {miscCall, miscOpArgc,			"int32 argc();"},
		//~ {miscCall, miscOpArgv,			"string arg(int arg);"},

		{miscCall, miscOpRand32,		"int32 rand();"},
		//~ {miscCall, timeOpTime32,		"int32 time();"},
		//~ {miscCall, timeOpClock32,		"int32 clock();"},
		//~ {miscCall, timeOpClocksPS,		"float64 clocksPerSec(int32 ticks);"},
		{miscCall, timeOpClck64,		"int64 timeNow();"},
		{miscCall, timeOpProc64,		"int64 timeCpu();"},
		//~ {miscCall, miscOpPutX64,		"void putx64(int64 val);"},
		//~ {miscCall, miscOpPutI64,		"void puti64(int64 val);"},
		//~ {miscCall, miscOpPutF64,		"void putf64(float64 val);"},
		//~ {miscCall, miscOpPutChr,		"void putchr(int val);"},

		{miscCall, miscOpPutStr,		"void print(string val);"},
		{miscCall, miscOpPutFmt,		"void print(string fmt, int64 val);"},
		{miscCall, miscOpPutFmt,		"void print(string fmt, float64 val);"},

		{miscCall, miscOpMemMgr,		"pointer realloc(pointer ptr, int32 size);"},		// reallocate, allocate, free
		//~ {NULL, miscOpMemMgr,		"define alloc(int32 size) = realloc(null, size);"},
		//~ {NULL, miscOpMemMgr,		"define free(pointer ptr) = realloc(ptr, 0);"},
		//~ {miscCall, miscOpMemMgr,		"int reallocInt(int ptr, int32 size);"},		// reallocate, allocate, free

		//}

		// TODO: include some of the compiler functions
		// for reflection. (lookup, import, logger, exec?)

	};
	for (i = 0; i < sizeof(defs) / sizeof(*defs); i += 1) {
		if (!libcall(rt, defs[i].fun, defs[i].n, defs[i].def)) {
			return -1;
		}
	}
	for (i = 0; i < sizeof(misc) / sizeof(*misc); i += 1) {
		if (!libcall(rt, misc[i].fun, misc[i].n, misc[i].def)) {
			return -1;
		}
	}

	if (file && ccOpen(rt, srcFile, file)) {
		return parse(rt->cc, 0, level) != 0;
	}

	return 0;
}
