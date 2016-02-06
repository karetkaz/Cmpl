/*******************************************************************************
 *   File: lstd.c
 *   Date: 2011/06/23
 *   Desc: standard library
 *******************************************************************************
basic math, debug, and system functions
*******************************************************************************/
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "internal.h"

//#{#region math functions
static vmError f64sin(libcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, sin(x));
	return 0;
}
static vmError f64cos(libcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, cos(x));
	return 0;
}
static vmError f64tan(libcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, tan(x));
	return 0;
}
static vmError f64log(libcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, log(x));
	return 0;
}
static vmError f64exp(libcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, exp(x));
	return 0;
}
static vmError f64pow(libcContext args) {
	float64_t x = argf64(args, 0);
	float64_t y = argf64(args, 8);
	retf64(args, pow(x, y));
	return 0;
}
static vmError f64sqrt(libcContext args) {
	float64_t x = argf64(args, 0);
	retf64(args, sqrt(x));
	return 0;
}
static vmError f64atan2(libcContext args) {
	float64_t x = argf64(args, 0);
	float64_t y = argf64(args, 8);
	retf64(args, atan2(x, y));
	return 0;
}

static vmError f32sin(libcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, sinf(x));
	return 0;
}
static vmError f32cos(libcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, cosf(x));
	return 0;
}
static vmError f32tan(libcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, tanf(x));
	return 0;
}
static vmError f32log(libcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, logf(x));
	return 0;
}
static vmError f32exp(libcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, expf(x));
	return 0;
}
static vmError f32pow(libcContext args) {
	float32_t x = argf32(args, 0);
	float32_t y = argf32(args, 4);
	retf32(args, powf(x, y));
	return 0;
}
static vmError f32sqrt(libcContext args) {
	float32_t x = argf32(args, 0);
	retf32(args, sqrtf(x));
	return 0;
}
static vmError f32atan2(libcContext args) {
	float32_t x = argf32(args, 0);
	float32_t y = argf32(args, 4);
	retf32(args, atan2f(x, y));
	return 0;
}
//#}#endregion

//#{#region bit operations
static vmError b64not(libcContext args) {
	int64_t x = argi64(args, 0);
	reti64(args, ~x);
	return 0;
}
static vmError b64and(libcContext args) {
	int64_t x = argi64(args, 0);
	int64_t y = argi64(args, 8);
	reti64(args, x & y);
	return 0;
}
static vmError b64ior(libcContext args) {
	int64_t x = argi64(args, 0);
	int64_t y = argi64(args, 8);
	reti64(args, x | y);
	return 0;
}
static vmError b64xor(libcContext args) {
	int64_t x = argi64(args, 0);
	int64_t y = argi64(args, 8);
	reti64(args, x ^ y);
	return 0;
}
static vmError b64shl(libcContext args) {
	int64_t x = argi64(args, 0);
	int32_t y = argi32(args, 8);
	reti64(args, x << y);
	return 0;
}
static vmError b64shr(libcContext args) {
	uint64_t x = (uint64_t) argi64(args, 0);
	int32_t y = argi32(args, 8);
	reti64(args, (int64_t) (x >> y));
	return 0;
}
static vmError b64sar(libcContext args) {
	int64_t x = argi64(args, 0);
	int32_t y = argi32(args, 8);
	reti64(args, x >> y);
	return 0;
}

/* unused bit operations
static int b32cnt(libcContext args) {
	uint32_t x = (uint32_t) argi32(args, 0);
	x -= ((x >> 1) & 0x55555555);
	x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
	x = (((x >> 4) + x) & 0x0f0f0f0f);
	x += (x >> 8) + (x >> 16);
	reti32(args, x & 0x3f);
	return 0;
}
static int b32sf(libcContext args) {
	int32_t result = 0;
	uint32_t x = (uint32_t) argi32(args, 0);
	if ((x & 0x0000ffff) == 0) { result += 16; x >>= 16; }
	if ((x & 0x000000ff) == 0) { result +=  8; x >>=  8; }
	if ((x & 0x0000000f) == 0) { result +=  4; x >>=  4; }
	if ((x & 0x00000003) == 0) { result +=  2; x >>=  2; }
	if ((x & 0x00000001) == 0) { result +=  1; }
	reti32(args, x ? result : -1);
	return 0;
}
static int b32sr(libcContext args) {
	int32_t result = 0;
	uint32_t x = (uint32_t) argi32(args, 0);
	if ((x & 0xffff0000) != 0) { result += 16; x >>= 16; }
	if ((x & 0x0000ff00) != 0) { result +=  8; x >>=  8; }
	if ((x & 0x000000f0) != 0) { result +=  4; x >>=  4; }
	if ((x & 0x0000000c) != 0) { result +=  2; x >>=  2; }
	if ((x & 0x00000002) != 0) { result +=  1; }
	reti32(args, x ? result : -1);
	return 0;
}
static int b32hi(libcContext args) {
	uint32_t x = (uint32_t) argi32(args, 0);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	reti32(args, x - (x >> 1));
	return 0;
}
static int b32lo(libcContext args) {
	int32_t x = argi32(args, 0);
	reti32(args, x & -x);
	return 0;
}
static int b32swap(libcContext args) {
	uint32_t x = (uint32_t) argi32(args, 0);
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	reti64(args, (x >> 16) | (x << 16));
	return 0;
}
static int b32zxt(libcContext args) {
	uint32_t val = (uint32_t) argi32(args, 0);
	int32_t ofs = (uint32_t) argi32(args, 4);
	int32_t cnt = (uint32_t) argi32(args, 8);
	reti32(args, (val << 32 - (ofs + cnt)) >> (32 - cnt));
	return 0;
}
static int b32sxt(libcContext args) {
	int32_t val = (uint32_t) argi32(args, 0);
	int32_t ofs = (uint32_t) argi32(args, 4);
	int32_t cnt = (uint32_t) argi32(args, 8);
	reti32(args, (val << 32 - (ofs + cnt)) >> (32 - cnt));
	return 0;
}
// */
//#}#endregion

//#{#region file operations
static inline int argpos(int *argp, int size) {
	int result = *argp;
	*argp += padded(size ? size : 1, vm_size);
	return result;
}

#define debugFILE(msg, ...) //prerr("debug", msg, ##__VA_ARGS__)

static vmError FILE_open(libcContext args) {       // File Open(char filename[]);
	int argc = 0;
	char *mode = args->data;
	char *name = argref(args, argpos(&argc, vm_size));
	// int slen = argi32(args, argpos(&argc, vm_size));
	FILE *file = fopen(name, mode);
	rethnd(args, file);
	debugFILE("Name: %s, Mode: %s, File: %x", name, mode, file);
	return file == NULL;
}
static vmError FILE_close(libcContext args) {      // void close(File file);
	FILE *file = arghnd(args, 0);
	debugFILE("File: %x", file);
	fclose(file);
	return 0;
}
static vmError FILE_stream(libcContext args) {     // File std[in, out, err];
	size_t stream = (size_t) args->data;
	switch (stream) {
		default:
			break;

		case 0:
			rethnd(args, stdin);
			return 0;

		case 1:
			rethnd(args, stdout);
			return 0;

		case 2:
			rethnd(args, stderr);
			return 0;

		case 3:
			rethnd(args, args->rt->logFile);
			return 0;
	}

	debugFILE("error opening stream: %x", stream);
	return 1;
}

static vmError FILE_getc(libcContext rt) {
	FILE *file = arghnd(rt, 0);
	reti32(rt, fgetc(file));
	return 0;
}
static vmError FILE_peek(libcContext rt) {
	FILE *file = arghnd(rt, 0);
	int chr = ungetc(getc(file), file);
	reti32(rt, chr);
	return 0;
}
static vmError FILE_read(libcContext rt) {         // int read(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(rt, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(rt, argpos(&argc, vm_size));
	int len = argi32(rt, argpos(&argc, vm_size));
	reti32(rt, fread(buff, (size_t) len, 1, file));
	return 0;
}
static vmError FILE_gets(libcContext args) {       // int fgets(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(args, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(args, argpos(&argc, vm_size));
	int len = argi32(args, argpos(&argc, vm_size));
	debugFILE("Buff: %08x[%d], File: %x", buff, len, file);
	if (feof(file)) {
		reti32(args, -1);
	}
	else {
		long pos = ftell(file);
		char *unused = fgets(buff, len, file);
		reti32(args, ftell(file) - pos);
		(void)unused;
	}
	return 0;
}

static vmError FILE_putc(libcContext args) {
	int argc = 0;
	FILE *file = arghnd(args, argpos(&argc, sizeof(FILE *)));
	int data = argi32(args, argpos(&argc, vm_size));
	debugFILE("Data: %c, File: %x", data, file);
	reti32(args, putc(data, file));
	return 0;
}
static vmError FILE_write(libcContext args) {      // int write(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(args, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(args, argpos(&argc, vm_size));
	int len = argi32(args, argpos(&argc, vm_size));
	debugFILE("Buff: %08x[%d], File: %x", buff, len, file);
	len = fwrite(buff, (size_t) len, 1, file);
	reti32(args, len);
	return 0;
}

static vmError FILE_flush(libcContext args) {
	FILE *file = arghnd(args, 0);
	debugFILE("File: %x", file);
	fflush(file);
	return 0;
}
//#}#endregion

//#{#region system functions (exit, rand, clock, debug)

#if (defined __WATCOMC__) || (defined _MSC_VER) || (defined __WIN32)
#include <Windows.h>
static inline int64_t timeMillis() {
	static const int64_t kTimeEpoc = 116444736000000000LL;
	static const int64_t kTimeScaler = 10000;  // 100 ns to us.

	// Although win32 uses 64-bit integers for representing timestamps,
	// these are packed into a FILETIME structure. The FILETIME
	// structure is just a struct representing a 64-bit integer. The
	// TimeStamp union allows access to both a FILETIME and an integer
	// representation of the timestamp. The Windows timestamp is in
	// 100-nanosecond intervals since January 1, 1601.
	typedef union {
		FILETIME ftime;
		int64_t time;
	} TimeStamp;
	TimeStamp time;
	GetSystemTimeAsFileTime(&time.ftime);
	return (time.time - kTimeEpoc) / kTimeScaler;
}
static inline void sleepMillis(int64_t milliseconds) {
	Sleep(milliseconds);
}
#else
#include <sys/time.h>
static inline int64_t timeMillis() {
	int64_t now;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	now = tv.tv_sec * (uint64_t)1000;
	now += tv.tv_usec / (uint64_t)1000;
	return now;
}
static inline void sleepMillis(int64_t milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
#endif

static vmError sysExit(libcContext args) {
	exit(argi32(args, 0));
	return 0;
}

static vmError sysRand(libcContext args) {
	reti32(args, rand());
	return 0;
}
static vmError sysSRand(libcContext args) {
	int seed = argi32(args, 0);
	srand((unsigned) seed);
	return 0;
}

static vmError sysTime(libcContext args) {
	reti32(args, (int) time(NULL));
	return 0;
}
static vmError sysClock(libcContext args) {
	reti32(args, clock());
	return 0;
}
static vmError sysMillis(libcContext args) {
	reti64(args, timeMillis());
	return 0;
}
static vmError sysMSleep(libcContext args) {
	sleepMillis(argi64(args, 0));
	return 0;
}

enum {
	raise_error = -1,	// log and raise executionAborted.
	raise_skip = 0,	// continue execution.
	raise_warn = 1,	// log and continue execution.
	raise_info = 2,	// log and continue execution.
	raise_debug = 3,	// log only if executing with debuger.
	raise_verbose = 4	// log only if executing with debuger.
};
// void raise(int logLevel, string message, variant inspect, int logTrace);
static vmError sysRaise(libcContext args) {
	rtContext rt = args->rt;
	char* file = argref(args, 0 * vm_size);
	int line = argi32(args, 1 * vm_size);
	int logLevel = argi32(args, 2 * vm_size);
	char* message = argref(args, 3 * vm_size);
	void* varRef = argref(args, 4 * vm_size);
	symn varType = argref(args, 5 * vm_size);
	int maxTrace = argi32(args, 6 * vm_size);
	int isOutput = 0;

	// logging disabled or log level not reached.
	if (rt->logFile == NULL || logLevel > rt->logLevel || !logLevel) {
		return noError;
	}
	/* skip debug logs if not in debug mode.
	if (rt->dbg == NULL && logLevel >= raise_debug) {
		return noError;
	}*/

	//~ long* argv = (long*)(long(*)[7])args->argv;

	// print valid code position (where the function was invoked).
	if (file != NULL && line > 0) {
		fputfmt(rt->logFile, "%s:%u", file, line);
		isOutput = 1;
	}

	// print the message
	if (message != NULL) {
		if (isOutput) {
			fputfmt(rt->logFile, ": ");
		}
		fputfmt(rt->logFile, "%s", message);
		isOutput = 1;
	}

	// print the value of the object (handy to inspect values).
	if (varType != NULL && varRef != NULL) {
		if (isOutput) {
			fputfmt(rt->logFile, ": ");
		}
		fputVal(rt->logFile, NULL, rt, varType, varRef, prType, 0);
		isOutput = 1;
	}

	// add a line ending to the output.
	if (isOutput) {
		fputfmt(rt->logFile, "\n");
	}

	// print stack trace skipping this function
	if (maxTrace > 0) {
		logTrace(rt, NULL, 1, 0, maxTrace);
	}

	// abort the execution
	if (logLevel < 0) {
		return executionAborted;
	}

	return noError;
}

static vmError sysTryExec(libcContext args) {
	rtContext rt = args->rt;
	dbgContext dbg = rt->dbg;
	int argval = argi32(args, 0 * vm_size);
	symn action = argsym(args, 1 * vm_size);
	if (action != NULL) {
		int result;
		#pragma pack(push, 4)
		struct { int32_t ptr; } cbArg = { .ptr = argval };
		#pragma pack(pop)
		// TODO: checked and unchecked errors should be handled the same way in debug and release mode.
		if (dbg != NULL) {
			int oldValue = dbg->checked;
			dbg->checked = 1;
			result = invoke(rt, action, NULL, &cbArg, args->extra);
			dbg->checked = oldValue;
		}
		else {
			result = invoke(rt, action, NULL, &cbArg, NULL);
		}
		reti32(args, result);
	}
	return noError;
}

static vmError sysMemMgr(libcContext rt) {
	void* old = argref(rt, 0);
	int size = argi32(rt, 4);
	void* res = rtAlloc(rt->rt, old, (size_t) size, NULL);
	reti32(rt, vmOffset(rt->rt, res));
	return noError;
}
static vmError sysMemCpy(libcContext rt) {
	void* dest = argref(rt, 0 * vm_size);
	void* src = argref(rt, 1 * vm_size);
	int size = argi32(rt, 2 * vm_size);
	void* res = memcpy(dest, src, (size_t) size);
	reti32(rt, vmOffset(rt->rt, res));
	return noError;
}
static vmError sysMemSet(libcContext rt) {
	void* dest = argref(rt, 0 * vm_size);
	int value = argi32(rt, 1 * vm_size);
	int size = argi32(rt, 2 * vm_size);
	void* res = memset(dest, value, (size_t) size);
	reti32(rt, vmOffset(rt->rt, res));
	return noError;
}
//#}#endregion

int ccUnitStdc(rtContext rt) {
	symn nsp = NULL;		// namespace
	int err = 0;
	size_t i;

	struct {
		vmError (*fun)(libcContext);
		char* def;
	}
	flt64[] = {		// sin, cos, sqrt, ...
		{f64sin,   "float64 sin(float64 x);"},
		{f64cos,   "float64 cos(float64 x);"},
		{f64tan,   "float64 tan(float64 x);"},
		{f64log,   "float64 log(float64 x);"},
		{f64exp,   "float64 exp(float64 x);"},
		{f64pow,   "float64 pow(float64 x, float64 y);"},
		{f64sqrt,  "float64 sqrt(float64 x);"},
		{f64atan2, "float64 atan2(float64 x, float64 y);"},
	},
	flt32[] = {		// sin, cos, sqrt, ...
		{f32sin,   "float32 sin(float32 x);"},
		{f32cos,   "float32 cos(float32 x);"},
		{f32tan,   "float32 tan(float32 x);"},
		{f32log,   "float32 log(float32 x);"},
		{f32exp,   "float32 exp(float32 x);"},
		{f32pow,   "float32 pow(float32 x, float32 y);"},
		{f32sqrt,  "float32 sqrt(float32 x);"},
		{f32atan2, "float32 atan2(float32 x, float32 y);"},
	},
	bit64[] = {			// 64 bit integer operations
		{b64not, "int64 Not(int64 Value);"},
		{b64and, "int64 And(int64 Lhs, int64 Rhs);"},
		{b64ior, "int64  Or(int64 Lhs, int64 Rhs);"},
		{b64xor, "int64 Xor(int64 Lhs, int64 Rhs);"},
		{b64shl, "int64 Shl(int64 Value, int Count);"},
		{b64shr, "int64 Shr(int64 Value, int Count);"},
		{b64sar, "int64 Sar(int64 Value, int Count);"},
	},
	misc[] = {			// IO/MEM/EXIT

		{sysExit,		"void exit(int32 code);"},
		{sysSRand,		"void srand(int32 seed);"},
		{sysRand,		"int32 rand();"},

		{sysTime,		"int32 time();"},
		{sysClock,		"int32 clock();"},
		{sysMillis,		"int64 millis();"},
		{sysMSleep,		"void sleep(int64 millis);"},

		// TODO: include some of the compiler functions for reflection. (lookup, exec?, ...)
	};
	struct {
		int64_t value;
		char* name;
	}
	constants[] = {
			{CLOCKS_PER_SEC, "CLOCKS_PER_SEC"},
			{RAND_MAX, "RAND_MAX"},
	},
	logLevels[] = {
		{raise_error, "error"},
		{raise_skip, "skip"},
		{raise_warn, "warn"},
		{raise_info, "info"},
		{raise_debug, "debug"},
		{raise_verbose, "verbose"},
		// trace levels
		{0, "noTrace"},
		{128, "defTrace"},
	};

	for (i = 0; i < lengthOf(constants); i += 1) {
		if (!ccDefInt(rt, constants[i].name, constants[i].value)) {
			err = 1;
			break;
		}
	}

	if (!err && rt->cc->type_var != NULL) {		// debug, trace, assert, fatal, ...
		rt->cc->libc_dbg = ccDefCall(rt, sysRaise, NULL, "void raise(int level, string message, variant inspect, int maxTrace);");
		rt->cc->libc_dbg_idx = rt->cc->libc_dbg->offs;
		if (rt->cc->libc_dbg == NULL) {
			err = 2;
		}
		if (!err && rt->cc->libc_dbg) {
			nsp = ccBegin(rt, "raise");
			for (i = 0; i < lengthOf(logLevels); i += 1) {
				if (!ccDefInt(rt, logLevels[i].name, logLevels[i].value)) {
					err = 1;
					break;
				}
			}
			ccEnd(rt, nsp, ATTR_stat);
		}
	}

	if (!err && rt->cc->type_ptr != NULL) {		// tryExecute
		if(!ccDefCall(rt, sysTryExec, NULL, "int tryExec(pointer args, void action(pointer args));")) {
			err = 2;
		}
	}

	if (!err && rt->cc->type_ptr != NULL) {		// realloc, malloc, free, memset, memcpy
		if(!ccDefCall(rt, sysMemMgr, NULL, "pointer memmgr(pointer ptr, int32 size);")) {
			err = 3;
		}
		if(!ccDefCall(rt, sysMemSet, NULL, "pointer memset(pointer dest, int value, int32 size);")) {
			err = 3;
		}
		if(!ccDefCall(rt, sysMemCpy, NULL, "pointer memcpy(pointer dest, pointer src, int32 size);")) {
			err = 3;
		}
	}

	// System.Exit(int code), ...
	if (!err && (nsp = ccBegin(rt, "System"))) {
		for (i = 0; i < lengthOf(misc); i += 1) {
			if (!ccDefCall(rt, misc[i].fun, NULL, misc[i].def)) {
				err = 4;
				break;
			}
		}
		//~ install(cc, "Arguments", TYPE_arr, 0, 0);	// string Args[];
		//~ install(cc, "Enviroment", TYPE_def, 0, 0);	// string Env[string];
		ccEnd(rt, nsp, ATTR_stat);
	}

	// Add bitwise operations to int64 as functions
	if (!err && rt->cc->type_i64 && !rt->cc->type_i64->flds) {
		ccBegin(rt, NULL);
		for (i = 0; i < lengthOf(bit64); i += 1) {
			if (!ccDefCall(rt, bit64[i].fun, NULL, bit64[i].def)) {
				err = 5;
				break;
			}
		}
		ccEnd(rt, rt->cc->type_i64, ATTR_stat);
	}

	// add math functions to float64 as functions
	if (!err && rt->cc->type_f64 && !rt->cc->type_f64->flds) {
		ccBegin(rt, NULL);
		for (i = 0; i < lengthOf(flt64); i += 1) {
			if (!ccDefCall(rt, flt64[i].fun, NULL, flt64[i].def)) {
				err = 6;
				break;
			}
		}
		ccEnd(rt, rt->cc->type_f64, ATTR_stat);
	}
	// add math functions to float32 as functions
	if (!err && rt->cc->type_f32 && !rt->cc->type_f32->flds) {
		ccBegin(rt, NULL);
		for (i = 0; i < lengthOf(flt32); i += 1) {
			if (!ccDefCall(rt, flt32[i].fun, NULL, flt32[i].def)) {
				err = 7;
				break;
			}
		}
		ccEnd(rt, rt->cc->type_f32, ATTR_stat);
	}

	return err;
}

int ccUnitFile(rtContext rt) {
	symn file_nsp = ccDefType(rt, "File", sizeof(FILE*), 0);
	int err = file_nsp == NULL;

	if (file_nsp != NULL) {
		ccBegin(rt, NULL);

		err = err || !ccDefCall(rt, FILE_open, "r", "File Open(char path[]);");
		err = err || !ccDefCall(rt, FILE_open, "w", "File Create(char path[]);");
		err = err || !ccDefCall(rt, FILE_open, "a", "File Append(char path[]);");

		err = err || !ccDefCall(rt, FILE_peek, NULL, "int Peek(File file);");
		err = err || !ccDefCall(rt, FILE_getc, NULL, "int Read(File file);");
		err = err || !ccDefCall(rt, FILE_read, NULL, "int Read(File file, uint8 buff[]);");
		err = err || !ccDefCall(rt, FILE_gets, NULL, "int ReadLine(File file, uint8 buff[]);");

		err = err || !ccDefCall(rt, FILE_putc, NULL, "int Write(File file, uint8 byte);");
		err = err || !ccDefCall(rt, FILE_write, NULL, "int Write(File file, uint8 buff[]);");

		//~ err = err || !ccDefCall(rt, FILE_puts, NULL, "int puts(char buff[], File file);");
		//~ err = err || !ccDefCall(rt, FILE_ungetc, NULL, "int ungetc(int chr, File file);");

		err = err || !ccDefCall(rt, FILE_flush, NULL, "void Flush(File file);");
		err = err || !ccDefCall(rt, FILE_close, NULL, "void Close(File file);");

		//~ err = err || !ccDefCall(rt, FILE_memory, (void*)0, "File MemIn(char data[]);");
		//~ err = err || !ccDefCall(rt, FILE_memory, (void*)1, "File MemOut(char data[]);");

		err = err || !ccDefCall(rt, FILE_stream, (void *) 0, "File StdIn;");
		err = err || !ccDefCall(rt, FILE_stream, (void *) 1, "File StdOut;");
		err = err || !ccDefCall(rt, FILE_stream, (void *) 2, "File StdErr;");
		err = err || !ccDefCall(rt, FILE_stream, (void *) 3, "File DbgOut;");

		//~ err = err || !ccDefCall(rt, FILE_mkdirs, NULL, "bool mkdirs(char path[]);");
		//~ err = err || !ccDefCall(rt, FILE_exists, NULL, "bool Exists(char path[]);");
		//~ err = err || !ccDefCall(rt, FILE_delete, NULL, "bool Delete(char path[]);");

		/* struct Path& {
		 *		int64 size;		// size of file or directory
		 *		int32 mode;		// attributes
		 *		string name;
		 *		string path;
		 *		string user;	// owner
		 *		string group;
		 *		define Exists = size > 0;
		 *		...
		 * }

		err = err || !ccDefCall(rt, FILE_properties, NULL, "Path Path(char path[]);");

		 */

		ccEnd(rt, file_nsp, ATTR_stat);
	}

	return err;
}
