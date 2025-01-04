/*******************************************************************************
 *   File: lstd.c
 *   Date: 2011/06/23
 *   Desc: standard library
 *******************************************************************************
 * basic math functions
 * standard functions
 */

#include <math.h>
#include <time.h>
#include <cmpl.h>
#include "../../src/compiler.h"
#include "../../src/debuger.h"

enum PrintFlags {
	minus = 1 << 0,  // '-' Left-justify within the given field width; Right justification is the default (see width sub-specifier).
	plus = 1 << 1,   // '+' Forces to preceed the result with a plus or minus sign (+ or -) even for positive numbers. By default, only negative numbers are preceded with a - sign.
	space = 1 << 2,  // ' ' If no sign is going to be written, a blank space is inserted before the value.
	hash = 1 << 3,   // '#' Used with o, x or X specifiers the value is preceeded with 0, 0x or 0X respectively for values different than zero. Used with a, A, e, E, f, F, g or G it forces the written output to contain a decimal point even if no more digits follow. By default, if no digits follow, no decimal point is written.
	zero = 1 << 4,   // `0` Left-pads the number with zeroes (0) instead of spaces when padding is specified (see width sub-specifier).
	// https://cplusplus.com/reference/cstdio/printf/
	// warning: flag ' ' is ignored when flag '+' is present [-Wformat]
	// warning: flag '0' is ignored when flag '-' is present [-Wformat]
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ float64 functions
static vmError f64ldexp(nfcContext ctx) {
	float64_t x = argF64(ctx, sizeof(int32_t));
	int32_t y = argI32(ctx, 0);
	retF64(ctx, ldexp(x, y));
	return noError;
}
static vmError f64frexp(nfcContext ctx) {
	float64_t x = argF64(ctx, sizeof(vmOffs));
	int32_t *y = vmPointer(ctx->rt, argRef(ctx, 0));
	retF64(ctx, frexp(x, y));
	return noError;
}
static vmError f64log2(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, log2(x));
	return noError;
}
static vmError f64exp2(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, exp2(x));
	return noError;
}
static vmError f64ln(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, log(x));
	return noError;
}
static vmError f64exp(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, exp(x));
	return noError;
}
static vmError f64pow(nfcContext ctx) {
	float64_t x = argF64(ctx, 8);
	float64_t y = argF64(ctx, 0);
	retF64(ctx, pow(x, y));
	return noError;
}
static vmError f64sqrt(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, sqrt(x));
	return noError;
}

static vmError f64sin(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, sin(x));
	return noError;
}
static vmError f64cos(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, cos(x));
	return noError;
}
static vmError f64tan(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, tan(x));
	return noError;
}

static vmError f64sinh(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, sinh(x));
	return noError;
}
static vmError f64cosh(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, cosh(x));
	return noError;
}
static vmError f64tanh(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, tanh(x));
	return noError;
}

static vmError f64asin(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, asin(x));
	return noError;
}
static vmError f64acos(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, acos(x));
	return noError;
}
static vmError f64atan(nfcContext ctx) {
	float64_t x = argF64(ctx, 0);
	retF64(ctx, atan(x));
	return noError;
}

static vmError f64atan2(nfcContext ctx) {
	float64_t y = argF64(ctx, 8);
	float64_t x = argF64(ctx, 0);
	retF64(ctx, atan2(y, x));
	return noError;
}
static vmError f64parse(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	const char *str = rt->api.nextArg(&al)->ref;
	float64_t *out = rt->api.nextArg(&al)->ref;
	char *end = NULL;
	*out = strtod(str, &end);
	retRef(ctx, end - str);
	return noError;
}
static vmError f64print(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	struct nfcArgArr out = rt->api.nextArg(&al)->arr;
	double val = rt->api.nextArg(&al)->f64;
	int32_t flags = rt->api.nextArg(&al)->i32;
	int32_t width = rt->api.nextArg(&al)->i32;
	int32_t precision = rt->api.nextArg(&al)->i32;
	int i = 0;
	char format[16] = {0};

	format[i++] = '%';
	if (flags & minus) {
		format[i++] = '-';
	}
	if (flags & plus) {
		format[i++] = '+';
	}
	if (flags & space) {
		format[i++] = ' ';
	}
	if (flags & hash) {
		format[i++] = '#';
	}
	if (flags & zero) {
		format[i++] = '0';
	}
	format[i++] = '*';
	format[i++] = '.';
	format[i++] = '*';
	format[i++] = 'l';
	format[i++] = 'f';
	format[i] = 0;

#ifdef __WATCOMC__ // fixme: watcom compiler does not link snprintf from this file ???
	return nativeCallError;
#else
	int res = snprintf((char *) out.ref, out.length, format, width, precision, val);
	retRef(ctx, res);
	return noError;
#endif
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ float32 functions
static vmError f32ln(nfcContext ctx) {
	float32_t x = argF32(ctx, 0);
	retF32(ctx, logf(x));
	return noError;
}
static vmError f32exp(nfcContext ctx) {
	float32_t x = argF32(ctx, 0);
	retF32(ctx, expf(x));
	return noError;
}
static vmError f32pow(nfcContext ctx) {
	float32_t x = argF32(ctx, 4);
	float32_t y = argF32(ctx, 0);
	retF32(ctx, powf(x, y));
	return noError;
}
static vmError f32sqrt(nfcContext ctx) {
	float32_t x = argF32(ctx, 0);
	retF32(ctx, sqrtf(x));
	return noError;
}
static vmError f32sin(nfcContext ctx) {
	float32_t x = argF32(ctx, 0);
	retF32(ctx, sinf(x));
	return noError;
}
static vmError f32cos(nfcContext ctx) {
	float32_t x = argF32(ctx, 0);
	retF32(ctx, cosf(x));
	return noError;
}
static vmError f32tan(nfcContext ctx) {
	float32_t x = argF32(ctx, 0);
	retF32(ctx, tanf(x));
	return noError;
}
static vmError f32atan2(nfcContext ctx) {
	float32_t y = argF32(ctx, 4);
	float32_t x = argF32(ctx, 0);
	retF32(ctx, atan2f(y, x));
	return noError;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bit operations

// count population of bits set to 1
static vmError b32pop(nfcContext ctx) {
	uint32_t x = argU32(ctx, 0);
	retU32(ctx, bitPop32(x));
	return noError;
}
// count the number of bits needed to represent this value
static vmError b32len(nfcContext ctx) {
	uint32_t x = argU32(ctx, 0);
	retU32(ctx, bitLen32(x));
	return noError;
}

// count population of bits set to 1
static vmError b64pop(nfcContext ctx) {
	uint64_t x = argU64(ctx, 0);
	retU32(ctx, bitPop64(x));
	return noError;
}
// count the number of bits needed to represent this value
static vmError b64len(nfcContext ctx) {
	uint64_t x = argU64(ctx, 0);
	retU32(ctx, bitLen64(x));
	return noError;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ system functions (exit, rand, clock, debug)
static vmError sysExit(nfcContext ctx) {
	exit(argI32(ctx, 0));
	return noError;
}

static vmError sysRand(nfcContext ctx) {
	retI32(ctx, rand());
	return noError;
}
static vmError sysSRand(nfcContext ctx) {
	int seed = argI32(ctx, 0);
	srand((unsigned) seed);
	return noError;
}

static vmError sysTime(nfcContext ctx) {
	retI32(ctx, time(NULL));
	return noError;
}
static vmError sysClock(nfcContext ctx) {
	retI32(ctx, clock());
	return noError;
}
static vmError sysMillis(nfcContext ctx) {
	retU64(ctx, timeMillis());
	return noError;
}
static vmError sysMSleep(nfcContext ctx) {
	sleepMillis(argI64(ctx, 0));
	return noError;
}

// pointer alloc(pointer ptr, int32 size);
static vmError sysMemMgr(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *old = rt->api.nextArg(&al)->ref;
	size_t size = rt->api.nextArg(&al)->i64;
	void *res = rt->api.alloc(ctx, old, size);
	retRef(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// pointer fill(pointer dst, int value, int32 size)
static vmError sysMemFill(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	int value = rt->api.nextArg(&al)->i32;
	size_t size = rt->api.nextArg(&al)->i64;
	void *res = memset(dst, value, size);
	retRef(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// pointer copy(pointer dst, pointer src, int32 size);
static vmError sysMemCopy(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	void *src = rt->api.nextArg(&al)->ref;
	size_t size = rt->api.nextArg(&al)->i64;
	void *res = memcpy(dst, src, size);
	retRef(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// pointer swap(pointer dst, pointer src, int32 size);
static vmError sysMemSwap(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	void *src = rt->api.nextArg(&al)->ref;
	size_t size = rt->api.nextArg(&al)->i64;
	memSwap(dst, src, size);
	retRef(ctx, vmOffset(ctx->rt, dst));
	return noError;
}
// pointer move(pointer dst, pointer src, int32 size);
static vmError sysMemMove(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	void *src = rt->api.nextArg(&al)->ref;
	size_t size = rt->api.nextArg(&al)->i64;
	void *res = memmove(dst, src, size);
	retRef(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// int compare(pointer dst, pointer src, int32 size);
static vmError sysMemCmp(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	void *src = rt->api.nextArg(&al)->ref;
	size_t size = rt->api.nextArg(&al)->i64;
	int res = memcmp(dst, src, size);
	retRef(ctx,  res);
	return noError;
}


static int installPlatform(rtContext rt, ccContext cc) {
	int err = 0;
	symn nsp = NULL;		// namespace

	if ((nsp = rt->api.ccBegin(cc, "Platform"))) {
		int isWebAssembly = 0;
		int isWindows = 0;
		int isMacOs = 0;
		int isLinux = 0;
		int isUnix = 0;
#if defined(__EMSCRIPTEN__)
		isWebAssembly = 8 * sizeof(void*);
#endif
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
		isWindows = 8 * sizeof(void*);
#endif
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
		isWindows = 8 * sizeof(void*);
#endif
#if defined(__OSX__) || defined(__MACOSX__) || defined(__APPLE__)
		isMacOs = 8 * sizeof(void*);
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
		isLinux = 8 * sizeof(void*);
#endif
#if defined(unix)        || defined(__unix)      || defined(__unix__) \
|| defined(sun)         || defined(__sun) \
|| defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
|| defined(__FreeBSD__) || defined (__DragonFly__) \
|| defined(sgi)         || defined(__sgi) \
|| defined(__CYGWIN__)
		isUnix = 8 * sizeof(void*);
#endif
		err = err || !rt->api.ccDefInt(cc, "WebAssembly", isWebAssembly);
		err = err || !rt->api.ccDefInt(cc, "Windows", isWindows);
		err = err || !rt->api.ccDefInt(cc, "MacOS", isMacOs);
		err = err || !rt->api.ccDefInt(cc, "Linux", isLinux);
		err = err || !rt->api.ccDefInt(cc, "Unix", isUnix);
		//err = err || !rt->api.ccDefInt(cc, "Android", 0);
		//err = err || !rt->api.ccDefInt(cc, "iOS", 0);
		//err = err || !rt->api.ccDefInt(cc, "Dos", 0);
		rt->api.ccEnd(cc, nsp);
	}
	return err;
}

static int ccLibSys(rtContext rt, ccContext cc) {
	symn nsp = NULL;		// namespace
	int err = 0;

	struct {
		vmError (*fun)(nfcContext);
		char *def;
	}
	sys[] = {       // IO/MEM/EXIT
		{sysExit,   "void exit(int32 code)"},
		{sysSRand,  "void srand(int32 seed)"},

		{sysTime,   "int32 time()"},
		{sysMillis, "int64 millis()"},
		{sysMSleep, "void sleep(int64 millis)"},
	};

	// alloc, free, memset, memcpy, memcmp
	if (rt->api.ccExtend(cc, cc->type_ptr)) {
		cc->libc_mal = rt->api.ccAddCall(cc, sysMemMgr, "pointer alloc(pointer ptr, int size)");
		if (cc->libc_mal == NULL) {
			err = 3;
		}
		err = err || 3 * !rt->api.ccAddCall(cc, sysMemFill, "pointer fill(pointer dst, uint8 value, int size)");
		err = err || 3 * !rt->api.ccAddCall(cc, sysMemCopy, "pointer copy(pointer dst, pointer src, int size)");
		err = err || 3 * !rt->api.ccAddCall(cc, sysMemSwap, "pointer swap(pointer dst, pointer src, int size)");
		err = err || 3 * !rt->api.ccAddCall(cc, sysMemMove, "pointer move(pointer dst, pointer src, int size)");
		err = err || 3 * !rt->api.ccAddCall(cc, sysMemCmp, "int compare(pointer dst, pointer src, int size)");
		rt->api.ccEnd(cc, cc->type_ptr);
	}

	if (!err && (nsp = rt->api.ccBegin(cc, "System")) != NULL) {
		for (size_t i = 0; i < lengthOf(sys); i += 1) {
			if (!rt->api.ccAddCall(cc, sys[i].fun, sys[i].def)) {
				err = 4;
				break;
			}
		}

		symn rand = rt->api.ccAddCall(cc, sysRand, "int32 rand()");
		if (!err && rt->api.ccExtend(cc, rand) != NULL) {
			if (!rt->api.ccDefInt(cc, "max", RAND_MAX)) {
				err = 1;
			}
			rt->api.ccEnd(cc, rand);
		}
		else {
			err = 2;
		}

		symn clock = rt->api.ccAddCall(cc, sysClock, "int32 clock()");
		if (!err && rt->api.ccExtend(cc, clock) != NULL) {
			if (!rt->api.ccDefInt(cc, "perSec", CLOCKS_PER_SEC)) {
				err = 1;
			}
			rt->api.ccEnd(cc, clock);
		}
		else {
			err = 2;
		}

		installPlatform(rt, cc);
		//~ install(cc, "Arguments", CAST_arr, 0, 0);	// string Args[];
		//~ install(cc, "Environment", KIND_def, 0, 0);	// string Env[string];
		rt->api.ccEnd(cc, nsp);
	}
	return err;
}

static int ccLibStd(rtContext rt, ccContext cc) {
	struct {
		vmError (*fun)(nfcContext);
		char *def;
	}
	bit32[] = {      // 32-bit operations
		{b32len,    "int32 len(int32 value)"},
		{b32pop,    "int32 pop(int32 value)"},
	},
	bit64[] = {      // 64-bit operations
		{b64len,    "int32 len(int64 value)"},
		{b64pop,    "int32 pop(int64 value)"},
	},
	flt32[] = {      // sin, cos, sqrt, ...
		{f32ln,     "float32 ln(float32 x)"},
		{f32exp,    "float32 exp(float32 x)"},
		{f32pow,    "float32 pow(float32 x, float32 y)"},

		{f32sin,    "float32 sin(float32 x)"},
		{f32cos,    "float32 cos(float32 x)"},
		{f32tan,    "float32 tan(float32 x)"},
		{f32sqrt,   "float32 sqrt(float32 x)"},
		{f32atan2,  "float32 atan2(float32 x, float32 y)"},
	},
	flt64[] = {      // sin, cos, sqrt, ...
		{f64ldexp,  "float64 ldexp(float64 x, int exp)"},
		{f64frexp,  "float64 frexp(float64 x, int exp&)"},

		{f64log2,   "float64 log2(float64 x)"},
		{f64exp2,   "float64 exp2(float64 x)"},
		{f64ln,     "float64 ln(float64 x)"},
		{f64exp,    "float64 exp(float64 x)"},
		{f64pow,    "float64 pow(float64 x, float64 y)"},
		{f64sqrt,   "float64 sqrt(float64 x)"},

		{f64sin,    "float64 sin(float64 x)"},
		{f64sinh,    "float64 sinh(float64 x)"},
		{f64asin,    "float64 asin(float64 x)"},
		{f64cos,    "float64 cos(float64 x)"},
		{f64cosh,    "float64 cosh(float64 x)"},
		{f64acos,    "float64 acos(float64 x)"},
		{f64tan,    "float64 tan(float64 x)"},
		{f64tanh,    "float64 tanh(float64 x)"},
		{f64atan,    "float64 atan(float64 x)"},
		{f64atan2,  "float64 atan2(float64 x, float64 y)"},

		{f64parse,  "int parse(char value[], float64 out&)"},
	};

	int err = 0;
	// Add extra operations to int32
	if (!err && rt->api.ccExtend(cc, cc->type_u32)) {
		for (size_t i = 0; i < lengthOf(bit32); i += 1) {
			if (!rt->api.ccAddCall(cc, bit32[i].fun, bit32[i].def)) {
				err = 5;
				break;
			}
		}
		rt->api.ccEnd(cc, cc->type_u32);
	}
	// Add extra operations to int64
	if (!err && rt->api.ccExtend(cc, cc->type_u64)) {
		for (size_t i = 0; i < lengthOf(bit64); i += 1) {
			if (!rt->api.ccAddCall(cc, bit64[i].fun, bit64[i].def)) {
				err = 5;
				break;
			}
		}
		rt->api.ccEnd(cc, cc->type_u64);
	}
	// add math functions to float32
	if (!err && rt->api.ccExtend(cc, cc->type_f32)) {
		for (size_t i = 0; i < lengthOf(flt32); i += 1) {
			if (!rt->api.ccAddCall(cc, flt32[i].fun, flt32[i].def)) {
				err = 7;
				break;
			}
		}
		rt->api.ccEnd(cc, cc->type_f32);
	}
	// add math functions to float64
	if (!err && rt->api.ccExtend(cc, cc->type_f64)) {
		for (size_t i = 0; i < lengthOf(flt64); i += 1) {
			if (!rt->api.ccAddCall(cc, flt64[i].fun, flt64[i].def)) {
				err = 6;
				break;
			}
		}
		symn print = rt->api.ccAddCall(cc, f64print, "int print(char out[], float64 value, int32 flags, int32 width, int32 precision)");
		if (rt->api.ccExtend(cc, print)) {
			rt->api.ccDefInt(cc, "flagMinus", minus);
			rt->api.ccDefInt(cc, "flagPlus", plus);
			rt->api.ccDefInt(cc, "flagSpace", space);
			rt->api.ccDefInt(cc, "flagHash", hash);
			rt->api.ccDefInt(cc, "flagZero", zero);
			rt->api.ccEnd(cc, print);
		}
		rt->api.ccEnd(cc, cc->type_f64);
	}
	return err;
}

const char cmplUnit[] = "cmplStd/lib.cmpl";
int cmplInit(rtContext ctx, ccContext cc) {
	int err = 0;
	err = err || ccLibSys(ctx, cc);
	err = err || ccLibStd(ctx, cc);
	return err;
}

// void cmplClose(rtContext ctx) {/* nothing to do here */}
