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
#include "../../src/cmpl.h"
#include "../../src/internal.h"
#include "../../src/utils.h"

enum PrintFlags {
	minus = 1 << 0,  // '-'  Left-justify within the given field width; Right justification is the default (see width sub-specifier).
	plus = 1 << 1,   // '+'  Forces to preceed the result with a plus or minus sign (+ or -) even for positive numbers. By default, only negative numbers are preceded with a - sign.
	space = 1 << 2,  // ' '  If no sign is going to be written, a blank space is inserted before the value.
	hash = 1 << 3,   // '#'  Used with o, x or X specifiers the value is preceeded with 0, 0x or 0X respectively for values different than zero. Used with a, A, e, E, f, F, g or G it forces the written output to contain a decimal point even if no more digits follow. By default, if no digits follow, no decimal point is written.
	zero = 1 << 4,   // `0`  Left-pads the number with zeroes (0) instead of spaces when padding is specified (see width sub-specifier).
	// https://cplusplus.com/reference/cstdio/printf/
	// warning: flag ' ' is ignored when flag '+' is present [-Wformat]
	// warning: flag '0' is ignored when flag '-' is present [-Wformat]
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ math functions
static vmError f64sin(nfcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, sin(x));
	return noError;
}
static vmError f64cos(nfcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, cos(x));
	return noError;
}
static vmError f64tan(nfcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, tan(x));
	return noError;
}
static vmError f64log(nfcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, log(x));
	return noError;
}
static vmError f64exp(nfcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, exp(x));
	return noError;
}
static vmError f64pow(nfcContext args) {
	float64_t x = argf64(args, 8);
	float64_t y = argf64(args, 0);
	retf64(args, pow(x, y));
	return noError;
}
static vmError f64sqrt(nfcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, sqrt(x));
	return noError;
}
static vmError f64atan2(nfcContext args) {
	float64_t y = argf64(args, 8);
	float64_t x = argf64(args, 0);
	retf64(args, atan2(y, x));
	return noError;
}
static vmError f64ldexp(nfcContext args) {
	float64_t x = argf64(args, sizeof(int32_t));
	int32_t y = argi32(args, 0);
	retf64(args, ldexp(x, y));
	return noError;
}
static vmError f64frexp(nfcContext args) {
	float64_t x = argf64(args, sizeof(vmOffs));
	int32_t *y = vmPointer(args->rt, argref(args, 0));
	retf64(args, frexp(x, y));
	return noError;
}
static vmError f64parse(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	const char *str = rt->api.nextArg(&al)->ref;
	float64_t *out = rt->api.nextArg(&al)->ref;
	char *end = NULL;
	*out = strtod(str, &end);
	retref(ctx, end - str);
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
	format[i++] = 0;

#ifdef __WATCOMC__ // fixme: watcom compiler does not link snprintf from this file ???
	return nativeCallError;
#else
	int res = snprintf((char *) out.ref, out.length, format, width, precision, val);
	retref(ctx, res);
	return noError;
#endif
}

static vmError f32sin(nfcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, sinf(x));
	return noError;
}
static vmError f32cos(nfcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, cosf(x));
	return noError;
}
static vmError f32tan(nfcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, tanf(x));
	return noError;
}
static vmError f32log(nfcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, logf(x));
	return noError;
}
static vmError f32exp(nfcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, expf(x));
	return noError;
}
static vmError f32pow(nfcContext args) {
	float32_t x = argf32(args, 4);
	float32_t y = argf32(args, 0);
	retf32(args, powf(x, y));
	return noError;
}
static vmError f32sqrt(nfcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, sqrtf(x));
	return noError;
}
static vmError f32atan2(nfcContext args) {
	float32_t y = argf32(args, 4);
	float32_t x = argf32(args, 0);
	retf32(args, atan2f(y, x));
	return noError;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ bit operations
// sign extend
static vmError b32sxt(nfcContext args) {
	int32_t val = argi32(args, 8);
	int32_t ofs = argi32(args, 4);
	int32_t cnt = argi32(args, 0);
	reti32(args, (val << (32 - (ofs + cnt))) >> (32 - cnt));
	return noError;
}
// zero extend
static vmError b32zxt(nfcContext args) {
	uint32_t val = argu32(args, 8);
	int32_t ofs = argi32(args, 4);
	int32_t cnt = argi32(args, 0);
	retu32(args, (val << (32 - (ofs + cnt))) >> (32 - cnt));
	return noError;
}
// count population of bits set to 1
static vmError b32pop(nfcContext args) {
	uint32_t val = argu32(args, 0);
	val -= (val >> 1) & 0x55555555;
	val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
	val = (val + (val >> 4)) & 0x0f0f0f0f;
	val += val >> 8;
	val += val >> 16;
	retu32(args, val & 0x3f);
	return noError;
}
// swap bits
static vmError b32swp(nfcContext args) {
	uint32_t val = argu32(args, 0);
	val = ((val >> 1) & 0x55555555) | ((val & 0x55555555) << 1);
	val = ((val >> 2) & 0x33333333) | ((val & 0x33333333) << 2);
	val = ((val >> 4) & 0x0F0F0F0F) | ((val & 0x0F0F0F0F) << 4);
	val = ((val >> 8) & 0x00FF00FF) | ((val & 0x00FF00FF) << 8);
	retu32(args, (val >> 16) | (val << 16));
	return noError;
}
// bit scan reverse: position of the Most Significant Bit
static vmError b32sr(nfcContext args) {
	uint32_t value = argu32(args, 0);
	if (value == 0) {
		reti32(args, -1);
		return noError;
	}
	int32_t result = 0;
	if (value & 0xffff0000) {
		result += 16;
		value >>= 16;
	}
	if (value & 0x0000ff00) {
		result +=  8;
		value >>= 8;
	}
	if (value & 0x000000f0) {
		result += 4;
		value >>= 4;
	}
	if (value & 0x0000000c) {
		result += 2;
		value >>= 2;
	}
	if (value & 0x00000002) {
		result += 1;
	}
	reti32(args, result);
	return noError;
}
// bit scan forward: position of the Least Significant Bit
static vmError b32sf(nfcContext args) {
	uint32_t value = argu32(args, 0);
	if (value == 0) {
		reti32(args, -1);
		return noError;
	}
	int32_t result = 0;
	if ((value & 0x0000ffff) == 0) {
		result += 16;
		value >>= 16;
	}
	if ((value & 0x000000ff) == 0) {
		result += 8;
		value >>= 8;
	}
	if ((value & 0x0000000f) == 0) {
		result += 4;
		value >>= 4;
	}
	if ((value & 0x00000003) == 0) {
		result += 2;
		value >>= 2;
	}
	if ((value & 0x00000001) == 0) {
		result += 1;
	}
	reti32(args, result);
	return noError;
}
// keep the highest: Most Significant Bit
static vmError b32hi(nfcContext args) {
	uint32_t val = argu32(args, 0);
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	retu32(args, val - (val >> 1));
	return noError;
}
// keep the lowest: Least Significant Bit
static vmError b32lo(nfcContext args) {
	uint32_t val = argu32(args, 0);
	retu32(args, val & -val);
	return noError;
}

static vmError b64sxt(nfcContext args) {
	int64_t val = argi64(args, 8);
	int32_t ofs = argi32(args, 4);
	int32_t cnt = argi32(args, 0);
	reti64(args, (val << (64 - (ofs + cnt))) >> (64 - cnt));
	return noError;
}
static vmError b64zxt(nfcContext args) {
	uint64_t val = argu64(args, 8);
	int32_t ofs = argi32(args, 4);
	int32_t cnt = argi32(args, 0);
	retu64(args, (val << (64 - (ofs + cnt))) >> (64 - cnt));
	return noError;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ system functions (exit, rand, clock, debug)
static vmError sysExit(nfcContext args) {
	exit(argi32(args, 0));
	return noError;
}

static vmError sysRand(nfcContext args) {
	reti32(args, rand());
	return noError;
}
static vmError sysSRand(nfcContext args) {
	int seed = argi32(args, 0);
	srand((unsigned) seed);
	return noError;
}

static vmError sysTime(nfcContext args) {
	reti32(args, time(NULL));
	return noError;
}
static vmError sysClock(nfcContext args) {
	reti32(args, clock());
	return noError;
}
static vmError sysMillis(nfcContext args) {
	retu64(args, timeMillis());
	return noError;
}
static vmError sysMSleep(nfcContext args) {
	sleepMillis(argi64(args, 0));
	return noError;
}

static void raiseStd(rtContext rt, raiseLevel level, const char *file, int line, struct nfcArgArr details, const char *msg, ...) {
	va_list vaList;
	va_start(vaList, msg);
	print_log(rt, level, file, line, details, msg, vaList);
	va_end(vaList);
}

// void raise(char file[*], int line, int level, int trace, char message[*], variant details...);
static vmError sysRaise(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	const char *file = rt->api.nextArg(&al)->ref;
	int line = rt->api.nextArg(&al)->i32;
	int logLevel = rt->api.nextArg(&al)->i32;
	int trace = rt->api.nextArg(&al)->i32;
	char *message = rt->api.nextArg(&al)->ref;
	struct nfcArgArr details = rt->api.nextArg(&al)->arr;

	// logging is disabled or log level not reached.
	if (rt->logFile == NULL || logLevel > (int)rt->logLevel) {
		return noError;
	}

	raiseStd(rt, logLevel, file, line, details, "%?s", message);

	// print stack trace excluding this function
	if (rt->dbg != NULL && trace > 0) {
		traceCalls(rt->dbg, rt->logFile, 1, trace - 1, 1);
	}

	// abort the execution
	if (logLevel == raiseFatal) {
		return nativeCallError;
	}

	return noError;
}

// int tryExec(variant outError, pointer args, void action(pointer args));
static vmError sysTryExec(nfcContext ctx) {
	vmError result;
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	size_t args = argref(ctx, rt->api.nextArg(&al)->offset);
	size_t actionOffs = argref(ctx, rt->api.nextArg(&al)->offset);
	symn action = rt->api.rtLookup(rt, actionOffs, KIND_fun);

	if (action != NULL && action->offs == actionOffs) {
		#pragma pack(push, 4)
		struct { vmOffs ptr; } cbArg;
		cbArg.ptr = (vmOffs) args;
		#pragma pack(pop)
		result = rt->api.invoke(rt, action, NULL, &cbArg, ctx->extra);
	}
	else {
		result = illegalState;
	}
	reti32(ctx, result);
	return noError;
}

// pointer alloc(pointer ptr, int32 size);
static vmError sysMemMgr(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *old = rt->api.nextArg(&al)->ref;
	int size = rt->api.nextArg(&al)->i32;
	void *res = rtAlloc(rt, old, (size_t) size, NULL);
	retref(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// pointer fill(pointer dst, int value, int32 size)
static vmError sysMemSet(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	int value = rt->api.nextArg(&al)->i32;
	int size = rt->api.nextArg(&al)->i32;
	void *res = memset(dst, value, (size_t) size);
	retref(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// pointer copy(pointer dst, pointer src, int32 size);
static vmError sysMemCpy(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	void *src = rt->api.nextArg(&al)->ref;
	int size = rt->api.nextArg(&al)->i32;
	void *res = memcpy(dst, src, (size_t) size);
	retref(ctx, vmOffset(ctx->rt, res));
	return noError;
}
// pointer move(pointer dst, pointer src, int32 size);
static vmError sysMemMove(nfcContext ctx) {
	rtContext rt = ctx->rt;
	struct nfcArgs al = rt->api.nfcArgs(ctx);
	void *dst = rt->api.nextArg(&al)->ref;
	void *src = rt->api.nextArg(&al)->ref;
	int size = rt->api.nextArg(&al)->i32;
	void *res = memmove(dst, src, (size_t) size);
	retref(ctx, vmOffset(ctx->rt, res));
	return noError;
}


static int installPlatform(ccContext cc) {
	int err = 0;
	symn nsp = NULL;		// namespace

	if ((nsp = ccBegin(cc, "Platform"))) {
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
		err = err || !ccDefInt(cc, "WebAssembly", isWebAssembly);
		err = err || !ccDefInt(cc, "Windows", isWindows);
		err = err || !ccDefInt(cc, "MacOS", isMacOs);
		err = err || !ccDefInt(cc, "Linux", isLinux);
		err = err || !ccDefInt(cc, "Unix", isUnix);
		//err = err || !ccDefInt(cc, "Android", 0);
		//err = err || !ccDefInt(cc, "iOS", 0);
		//err = err || !ccDefInt(cc, "Dos", 0);
		ccEnd(cc, nsp);
	}
	return err;
}

static int ccLibSys(ccContext cc) {
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
	struct {
		int64_t value;
		char *name;
	}
	logLevels[] = {
			{raiseFatal, "abort"},
			{raiseError, "error"},
			{raiseWarn, "warn"},
			{raiseInfo, "info"},
			{raiseDebug, "debug"},
			{raiseVerbose, "verbose"},
			// trace levels
			{0, "noTrace"},
			{128, "defTrace"},
	};

	// raise(fatal, error, warn, info, debug, trace)
	if (cc->type_var != NULL) {
		cc->libc_dbg = ccAddCall(cc, sysRaise, "void raise(const char file[*], int32 line, int32 level, int32 trace, const char message[*], const variant details...)");
		if (cc->libc_dbg == NULL) {
			err = 2;
		}
		if (!err && ccExtend(cc, cc->libc_dbg)) {
			cc->libc_dbg->doc = "Report messages or raise errors.";
			for (size_t i = 0; i < lengthOf(logLevels); i += 1) {
				if (!ccDefInt(cc, logLevels[i].name, logLevels[i].value)) {
					err = 1;
					break;
				}
			}
			ccEnd(cc, cc->libc_dbg);
		}
	}

	// tryExecute
	if (!err && cc->type_ptr != NULL) {
		cc->libc_try = ccAddCall(cc, sysTryExec, "int32 tryExec(pointer args, void action(pointer args))");
		if (cc->libc_try == NULL) {
			err = 2;
		}
	}

	// alloc, free, memset, memcpy
	if (!err && ccExtend(cc, cc->type_ptr)) {
		cc->libc_mal = ccAddCall(cc, sysMemMgr, "pointer alloc(pointer ptr, int32 size)");
		if (cc->libc_mal == NULL) {
			err = 3;
		}
		if (!ccAddCall(cc, sysMemSet, "pointer fill(pointer dst, uint8 value, int32 size)")) {
			err = 3;
		}
		if (!ccAddCall(cc, sysMemCpy, "pointer copy(pointer dst, pointer src, int32 size)")) {
			err = 3;
		}
		if (!ccAddCall(cc, sysMemMove, "pointer move(pointer dst, pointer src, int32 size)")) {
			err = 3;
		}
		ccEnd(cc, cc->type_ptr);
	}

	if (!err && (nsp = ccBegin(cc, "System"))) {
		for (size_t i = 0; i < lengthOf(sys); i += 1) {
			if (!ccAddCall(cc, sys[i].fun, sys[i].def)) {
				err = 4;
				break;
			}
		}

		symn rand = ccAddCall(cc, sysRand, "int32 rand()");
		if (!err && ccExtend(cc, rand) != NULL) {
			if (!ccDefInt(cc, "max", RAND_MAX)) {
				err = 1;
			}
			ccEnd(cc, rand);
		}
		else {
			err = 2;
		}

		symn clock = ccAddCall(cc, sysClock, "int32 clock()");
		if (!err && ccExtend(cc, clock) != NULL) {
			if (!ccDefInt(cc, "perSec", CLOCKS_PER_SEC)) {
				err = 1;
			}
			ccEnd(cc, clock);
		}
		else {
			err = 2;
		}

		installPlatform(cc);
		//~ install(cc, "Arguments", CAST_arr, 0, 0);	// string Args[];
		//~ install(cc, "Environment", KIND_def, 0, 0);	// string Env[string];
		ccEnd(cc, nsp);
	}
	return err;
}

static int ccLibStd(ccContext cc) {

	struct {
		vmError (*fun)(nfcContext);
		char *def;
	}
	bit32[] = {      // 32 bit operations
		{b32zxt,    "int32 zxt(int32 value, int32 offs, int32 count)"},
		{b32sxt,    "int32 sxt(int32 value, int32 offs, int32 count)"},
		{b32pop,    "int32 pop(int32 value)"},
		{b32swp,    "int32 swap(int32 value)"},
		{b32sr,     "int32 bsr(int32 value)"},
		{b32sf,     "int32 bsf(int32 value)"},
		{b32hi,     "int32 hib(int32 value)"},
		{b32lo,     "int32 lob(int32 value)"},
	},
	bit64[] = {      // 64 bit operations
		{b64zxt,    "int64 zxt(int64 value, int32 offs, int32 count)"},
		{b64sxt,    "int64 sxt(int64 value, int32 offs, int32 count)"},
	},
	flt32[] = {      // sin, cos, sqrt, ...
		{f32sin,    "float32 sin(float32 x)"},
		{f32cos,    "float32 cos(float32 x)"},
		{f32tan,    "float32 tan(float32 x)"},
		{f32log,    "float32 log(float32 x)"},
		{f32exp,    "float32 exp(float32 x)"},
		{f32pow,    "float32 pow(float32 x, float32 y)"},
		{f32sqrt,   "float32 sqrt(float32 x)"},
		{f32atan2,  "float32 atan2(float32 x, float32 y)"},
	},
	flt64[] = {      // sin, cos, sqrt, ...
		{f64sin,    "float64 sin(float64 x)"},
		{f64cos,    "float64 cos(float64 x)"},
		{f64tan,    "float64 tan(float64 x)"},
		{f64log,    "float64 log(float64 x)"},
		{f64exp,    "float64 exp(float64 x)"},
		{f64pow,    "float64 pow(float64 x, float64 y)"},
		{f64sqrt,   "float64 sqrt(float64 x)"},
		{f64atan2,  "float64 atan2(float64 x, float64 y)"},
		{f64ldexp,  "float64 ldexp(float64 x, int exp)"},
		{f64frexp,  "float64 frexp(float64 x, int exp&)"},
		{f64parse,  "int parse(const char value[], float64 out&)"},
	};

	int err = 0;
	// Add extra operations to int32
	if (!err && ccExtend(cc, cc->type_u32)) {
		for (size_t i = 0; i < lengthOf(bit32); i += 1) {
			if (!ccAddCall(cc, bit32[i].fun, bit32[i].def)) {
				err = 5;
				break;
			}
		}
		ccEnd(cc, cc->type_u32);
	}
	// Add extra operations to int64
	if (!err && ccExtend(cc, cc->type_u64)) {
		for (size_t i = 0; i < lengthOf(bit64); i += 1) {
			if (!ccAddCall(cc, bit64[i].fun, bit64[i].def)) {
				err = 5;
				break;
			}
		}
		ccEnd(cc, cc->type_u64);
	}
	// add math functions to float32
	if (!err && ccExtend(cc, cc->type_f32)) {
		for (size_t i = 0; i < lengthOf(flt32); i += 1) {
			if (!ccAddCall(cc, flt32[i].fun, flt32[i].def)) {
				err = 7;
				break;
			}
		}
		ccEnd(cc, cc->type_f32);
	}
	// add math functions to float64
	if (!err && ccExtend(cc, cc->type_f64)) {
		for (size_t i = 0; i < lengthOf(flt64); i += 1) {
			if (!ccAddCall(cc, flt64[i].fun, flt64[i].def)) {
				err = 6;
				break;
			}
		}
		symn print = ccAddCall(cc, f64print, "int print(char out[], float64 value, int32 flags, int32 width, int32 precision)");
		if (ccExtend(cc, print)) {
			ccDefInt(cc, "flagMinus", minus);
			ccDefInt(cc, "flagPlus", plus);
			ccDefInt(cc, "flagSpace", space);
			ccDefInt(cc, "flagHash", hash);
			ccDefInt(cc, "flagZero", zero);
			ccEnd(cc, print);
		}
		ccEnd(cc, cc->type_f64);
	}
	return err;
}

int cmplInit(rtContext rt) {
	int err = 0;
	err = err || ccLibSys(rt->cc);
	err = err || ccLibStd(rt->cc);
	return err;
}
