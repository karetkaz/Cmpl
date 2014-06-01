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
static int f64sin(libcArgs args) {
	float64_t x = argf64(args, 0);
	retf64(args, sin(x));
	return 0;
}
static int f64cos(libcArgs args) {
	float64_t x = argf64(args, 0);
	retf64(args, cos(x));
	return 0;
}
static int f64tan(libcArgs args) {
	float64_t x = argf64(args, 0);
	retf64(args, tan(x));
	return 0;
}
static int f64log(libcArgs args) {
	float64_t x = argf64(args, 0);
	retf64(args, log(x));
	return 0;
}
static int f64exp(libcArgs args) {
	float64_t x = argf64(args, 0);
	retf64(args, exp(x));
	return 0;
}
static int f64pow(libcArgs args) {
	float64_t x = argf64(args, 0);
	float64_t y = argf64(args, 8);
	retf64(args, pow(x, y));
	//~ debug("pow(%g, %g) := %g", x, y, pow(x, y));
	return 0;
}
static int f64sqrt(libcArgs args) {
	float64_t x = argf64(args, 0);
	retf64(args, sqrt(x));
	return 0;
}
static int f64atan2(libcArgs args) {
	float64_t x = argf64(args, 0);
	float64_t y = argf64(args, 8);
	retf64(args, atan2(x, y));
	return 0;
}
//#}#endregion

//#{#region bit operations
static int b64not(libcArgs args) {
	uint64_t x = argi64(args, 0);
	reti64(args, ~x);
	return 0;
}
static int b64and(libcArgs args) {
	uint64_t x = argi64(args, 0);
	uint64_t y = argi64(args, 8);
	reti64(args, x & y);
	return 0;
}
static int b64ior(libcArgs args) {
	uint64_t x = argi64(args, 0);
	uint64_t y = argi64(args, 8);
	reti64(args, x | y);
	return 0;
}
static int b64xor(libcArgs args) {
	uint64_t x = argi64(args, 0);
	uint64_t y = argi64(args, 8);
	reti64(args, x ^ y);
	return 0;
}
static int b64shl(libcArgs args) {
	uint64_t x = argi64(args, 0);
	int32_t y = argi32(args, 8);
	reti64(args, x << y);
	return 0;
}
static int b64shr(libcArgs args) {
	uint64_t x = argi64(args, 0);
	int32_t y = argi32(args, 8);
	reti64(args, x >> y);
	return 0;
}
static int b64sar(libcArgs args) {
	int64_t x = argi64(args, 0);
	int32_t y = argi32(args, 8);
	reti64(args, x >> y);
	return 0;
}

/* unused bit operations
typedef enum {
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

	//~ b32_zxt,	// zero extend
	//~ b64_zxt,

	//~ b32_sxt,	// sign extend
	//~ b64_sxt,
} bitOperation;

static int bitFunctions(libcArgs args) {
	switch ((bitOperation)args->data) {

		case b32_btc: {
			uint32_t x = argi32(args, 0);
			x -= ((x >> 1) & 0x55555555);
			x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
			x = (((x >> 4) + x) & 0x0f0f0f0f);
			x += (x >> 8) + (x >> 16);
			reti32(args, x & 0x3f);
			return 0;
		}
		//case b64_btc:

		case b32_bsf: {
			int result = 0;
			uint32_t x = argi32(args, 0);
			if ((x & 0x0000ffff) == 0) { result += 16; x >>= 16; }
			if ((x & 0x000000ff) == 0) { result +=  8; x >>=  8; }
			if ((x & 0x0000000f) == 0) { result +=  4; x >>=  4; }
			if ((x & 0x00000003) == 0) { result +=  2; x >>=  2; }
			if ((x & 0x00000001) == 0) { result +=  1; }
			reti32(args, x ? result : -1);
			return 0;
		}

		case b64_bsf: {
			int result = -1;
			uint64_t x = argi64(args, 0);
			if (x != 0) {
				result = 0;
				if ((x & 0x00000000ffffffffULL) == 0) { result += 32; x >>= 32; }
				if ((x & 0x000000000000ffffULL) == 0) { result += 16; x >>= 16; }
				if ((x & 0x00000000000000ffULL) == 0) { result +=  8; x >>=  8; }
				if ((x & 0x000000000000000fULL) == 0) { result +=  4; x >>=  4; }
				if ((x & 0x0000000000000003ULL) == 0) { result +=  2; x >>=  2; }
				if ((x & 0x0000000000000001ULL) == 0) { result +=  1; }
			}
			reti32(args, result);
			return 0;
		}

		case b32_bsr: {
			uint32_t x = argi32(args, 0);
			signed ans = 0;
			if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
			if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
			if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
			if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000002) != 0) { ans +=  1; }
			reti32(args, x ? ans : -1);
			return 0;
		}

		case b64_bsr: {
			uint64_t x = argi64(args, 0);
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
			reti32(args, ans);
			return 0;
		}

		case b32_bhi: {
			uint32_t x = argi32(args, 0);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			reti32(args, x - (x >> 1));
			return 0;
		}

		case b64_bhi: {
			uint64_t x = argi64(args, 0);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			x |= x >> 32;
			reti64(args, x - (x >> 1));
			return 0;
		}

		case b32_blo: {
			int32_t x = argi32(args, 0);
			reti32(args, x & -x);
			return 0;
		}

		case b64_blo: {
			int64_t x = argi64(args, 0);
			reti64(args, x & -x);
			return 0;
		}

		case b32_swp: {
			uint32_t x = argi32(args, 0);
			x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
			x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
			x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
			x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
			reti64(args, (x >> 16) | (x << 16));
			return 0;
		}
		/ *case b64_swp:
			???

		case b32_zxt: {
			uint32_t val = popi32(args);
			int32_t ofs = popi32(args);
			int32_t cnt = popi32(args);
			val <<= 32 - (ofs + cnt);
			reti32(args, val >> (32 - cnt));
			return 0;
		}
		case b64_zxt: {
			uint64_t val = popi64(args);
			int32_t ofs = popi32(args);
			int32_t cnt = popi32(args);
			val <<= 64 - (ofs + cnt);
			reti64(args, val >> (64 - cnt));
		} return 0;

		case b32_sxt: {
			int32_t val = popi32(args);
			int32_t ofs = popi32(args);
			int32_t cnt = popi32(args);
			val <<= 32 - (ofs + cnt);
			reti32(args, val >> (32 - cnt));
		} return 0;
		case b64_sxt: {
			int64_t val = popi64(args);
			int32_t ofs = popi32(args);
			int32_t cnt = popi32(args);
			val <<= 64 - (ofs + cnt);
			reti64(args, val >> (64 - cnt));
		} return 0;
		* /
	}
	return -1;
}*/

//#}#endregion

//#{#region file operations
static inline int argpos(int *argp, int size) {
	int result = *argp;
	*argp += padded(size || 1, vm_size);
	return result;
}

#define debugFILE(msg, ...) //prerr("debug", msg, ##__VA_ARGS__)

static int FILE_open(libcArgs args) {	// File Open(char filename[]);
	int argc = 0;
	char *mode = args->data;
	char *name = argref(args, argpos(&argc, vm_size));
	// int slen = argi32(args, argpos(&argc, vm_size));

	FILE *file = fopen(name, mode);
	rethnd(args, file);

	debugFILE("Name: %s, Mode: %s, File: %x", name, mode, file);
	return file == NULL;
}
static int FILE_close(libcArgs args) {	// void close(File file);
	FILE *file = arghnd(args, 0);
	debugFILE("File: %x", file);
	fclose(file);

	return 0;
}
static int FILE_stream(libcArgs args) {	// File std[in, out, err];
	//~ int argc = 0;
	int stream = (int)(size_t)args->data;

	//~ FILE *file = fdopen(name, mode);
	switch (stream) {
		case 0:
			rethnd(args, stdin);
			return 0;

		case 1:
			rethnd(args, stdout);
			return 0;

		case 2:
			rethnd(args, stderr);
			return 0;
	}

	debugFILE("error opening stream: %x", stream);
	return 1;
}

static int FILE_getc(libcArgs rt) {
	FILE *file = arghnd(rt, 0);
	reti32(rt, fgetc(file));

	return 0;
}
static int FILE_peek(libcArgs rt) {
	FILE *file = arghnd(rt, 0);

	int chr = ungetc(getc(file), file);
	reti32(rt, chr);

	return 0;
}
static int FILE_read(libcArgs rt) {	// int read(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(rt, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(rt, argpos(&argc, vm_size));
	int len = argi32(rt, argpos(&argc, vm_size));

	reti32(rt, fread(buff, len, 1, file));

	return 0;
}
static int FILE_gets(libcArgs args) {	// int fgets(File &f, uint8 buff[])
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

static int FILE_putc(libcArgs args) {
	int argc = 0;
	FILE *file = arghnd(args, argpos(&argc, sizeof(FILE *)));
	int data = argi32(args, argpos(&argc, vm_size));

	debugFILE("Data: %c, File: %x", data, file);
	reti32(args, putc(data, file));

	return 0;
}
static int FILE_write(libcArgs args) {	// int write(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(args, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(args, argpos(&argc, vm_size));
	int len = argi32(args, argpos(&argc, vm_size));

	debugFILE("Buff: %08x[%d], File: %x", buff, len, file);
	len = fwrite(buff, len, 1, file);
	reti32(args, len);

	return 0;
}

static int FILE_flush(libcArgs args) {
	FILE *file = arghnd(args, 0);

	debugFILE("File: %x", file);
	fflush(file);

	return 0;
}
//#}#endregion

//#{#region reflection operations
//#}#endregion

//#{#region io functions (exit, rand, clock, print, debug)
typedef enum {
	miscOpExit,
	miscOpRand32,

	timeOpTime32,		// current time in seconds since 1970
	timeOpMillis64,		// current time millis.

	timeOpClock32,		// process time spent in processor.
	timeOpClocks2s,

	miscOpPutStr,
	miscOpPutFmt
} miscOperation;

#if (defined __WATCOMC__) || (defined _MSC_VER)
/**
int64_t TimerUtils::GetCurrentTimeMilliseconds() {
  return GetCurrentTimeMicros() / 1000;
}

int64_t TimerUtils::GetCurrentTimeMicros() {
  static const int64_t kTimeEpoc = 116444736000000000LL;
  static const int64_t kTimeScaler = 10;  // 100 ns to us.

  // Although win32 uses 64-bit integers for representing timestamps,
  // these are packed into a FILETIME structure. The FILETIME
  // structure is just a struct representing a 64-bit integer. The
  // TimeStamp union allows access to both a FILETIME and an integer
  // representation of the timestamp. The Windows timestamp is in
  // 100-nanosecond intervals since January 1, 1601.
  union TimeStamp {
    FILETIME ft_;
    int64_t t_;
  };
  TimeStamp time;
  GetSystemTimeAsFileTime(&time.ft_);
  return (time.t_ - kTimeEpoc) / kTimeScaler;
}
**/
//#include <time.h>
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
#endif

static int miscFunction(libcArgs args) {
	switch ((miscOperation)(size_t)args->data) {

		case miscOpExit:
			exit(argi32(args, 0));
			return 0;

		case miscOpRand32: {
			static int initialized = 0;
			int result;
			if (!initialized) {
				srand((unsigned int)time(NULL));
				initialized = 1;
			}
			result = rand() * rand();	// if it gives a 16 bit int
			reti32(args, result & 0x7fffffff);
			return 0;
		}

		case timeOpTime32:
			reti32(args, (int)time(NULL));
			return 0;

		case timeOpMillis64:
			reti64(args, timeMillis());
			return 0;

		case timeOpClock32:
			reti32(args, clock());
			return 0;

		case timeOpClocks2s: {
			float64_t ticks = argi32(args, 0);
			retf64(args, ticks / CLOCKS_PER_SEC);
			return 0;
		}

		case miscOpPutStr:
			// TODO: check bounds
			fputfmt(stdout, "%s", argref(args, 0));
			return 0;

		case miscOpPutFmt: {
			char* fmt = argref(args, 0);
			int64_t arg = argi64(args, 4);
			fputfmt(stdout, fmt, arg);
			return 0;
		}
	}
	return -1;
}

static int libCallDebug(libcArgs args) {
	state rt = args->rt;
	char* file = argref(args, 0 * vm_size);
	int   line = argi32(args, 1 * vm_size);
	//~ char* func = argstr(args, ? * vm_size);
	char* message = argref(args, 2 * vm_size);
	int loglevel = argi32(args, 3 * vm_size);
	int tracelevel = argi32(args, 4 * vm_size);
	void* objref = argref(args, 5 * vm_size);
	symn objtyp = argref(args, 6 * vm_size);

	// skip loglevel 0
	if (rt->logf != NULL && loglevel != 0) {
		int isOutput = 0;

		if (!(message || (objref && objtyp))) {
			message = "";
		}
		// position where the function was invoked
		if (file != NULL && line > 0) {
			fputfmt(rt->logf, "%s:%u", file, line);
			isOutput = 1;
		}

		// the message to be printed
		if (message != NULL) {
			fputfmt(rt->logf, ": %s", message);
			isOutput = 1;
		}

		// specified object
		if (objtyp != NULL && objref != NULL) {
			if (isOutput) {
				fputfmt(rt->logf, ": ");
			}
			fputval(rt, rt->logf, objtyp, objref, 0, prType);
			isOutput = 1;
		}

		if (isOutput) {
			fputfmt(rt->logf, "\n");
		}
		// print stack trace skipping this function
		if (tracelevel > 0) {
			logTrace(rt, 1, 1, tracelevel);
		}
	}

	// abort the execution
	if (loglevel < 0) {
		return -loglevel;
	}

	return 0;
}

static int libCallMemMgr(libcArgs rt) {
	void* old = argref(rt, 0);
	int size = argi32(rt, 4);

	void* res = rtAlloc(rt->rt, old, size);
	reti32(rt, vmOffset(rt->rt, res));

	return 0;
}
//#}#endregion

int install_stdc(state rt) {
	symn nsp = NULL;		// namespace
	unsigned int i, err = 0;
	struct {
		int (*fun)(libcArgs);
		miscOperation data;
		char* def;
	}
	math[] = {		// sin, cos, sqrt, ...
		{f64sin,   0, "float64 sin(float64 x);"},
		{f64cos,   0, "float64 cos(float64 x);"},
		{f64tan,   0, "float64 tan(float64 x);"},
		{f64log,   0, "float64 log(float64 x);"},
		{f64exp,   0, "float64 exp(float64 x);"},
		{f64pow,   0, "float64 pow(float64 x, float64 y);"},
		{f64sqrt,  0, "float64 sqrt(float64 x);"},
		{f64atan2, 0, "float64 atan2(float64 x, float64 y);"},
	},
	misc[] = {		// IO/MEM/EXIT

		{miscFunction, miscOpRand32,		"int32 rand();"},

		{miscFunction, timeOpTime32,		"int32 time();"},
		{miscFunction, timeOpMillis64,		"int64 millis();"},

		{miscFunction, timeOpClock32,		"int32 clock();"},
		{miscFunction, timeOpClocks2s,		"float64 clocks2Sec(int32 clock);"},

		{miscFunction, miscOpPutStr,		"void print(string val);"},
		{miscFunction, miscOpPutFmt,		"void print(string fmt, int64 val);"},

		// TODO: include some of the compiler functions for reflection. (lookup, exec?, ...)
	};

	if (rt->cc->type_ptr != NULL) {		// realloc, malloc, free.
		ccAddCall(rt, libCallMemMgr, NULL, "pointer memmgr(pointer ptr, int32 size);");
	}
	if (rt->cc->type_var != NULL) {		// debug.
		rt->cc->libc_dbg = ccAddCall(rt, libCallDebug, NULL, "void debug(string message, int level, int trace, variant object);");
		rt->cc->libc_dbg_idx = rt->cc->libc_dbg->offs;
	}

	// System.Exit(int code), ...
	if ((nsp = ccBegin(rt, "System"))) {
		err = err || !ccAddCall(rt, miscFunction, (void*)miscOpExit, "void Exit(int Code);");

		//~ install(cc, "Arguments", TYPE_arr, 0, 0);	// string Args[];
		//~ install(cc, "Enviroment", TYPE_def, 0, 0);	// string Env[string];

		ccEnd(rt, nsp);
	}

	// Add bitwise operations to int64 as functions
	if (rt->cc->type_i64 && !rt->cc->type_i64->flds) {
		ccBegin(rt, NULL);
		err = err || !ccAddCall(rt, b64not, NULL, "int64 Not(int64 Value);");
		err = err || !ccAddCall(rt, b64and, NULL, "int64 And(int64 Lhs, int64 Rhs);");
		err = err || !ccAddCall(rt, b64ior, NULL, "int64  Or(int64 Lhs, int64 Rhs);");
		err = err || !ccAddCall(rt, b64xor, NULL, "int64 Xor(int64 Lhs, int64 Rhs);");
		err = err || !ccAddCall(rt, b64shl, NULL, "int64 Shl(int64 Value, int Count);");
		err = err || !ccAddCall(rt, b64shr, NULL, "int64 Shr(int64 Value, int Count);");
		err = err || !ccAddCall(rt, b64sar, NULL, "int64 Sar(int64 Value, int Count);");
		ccEnd(rt, rt->cc->type_i64);
	}

	for (i = 0; i < lengthOf(math); i += 1) {
		symn libc = ccAddCall(rt, math[i].fun, (void*)math[i].data, math[i].def);
		if (libc == NULL) {
			return -1;
		}
	}

	for (i = 0; i < lengthOf(misc); i += 1) {
		symn libc = ccAddCall(rt, misc[i].fun, (void*)misc[i].data, misc[i].def);
		if (libc == NULL) {
			return -1;
		}
	}

	return err;
}

int install_file(state rt) {
	symn file_nsp = ccAddType(rt, "File", sizeof(FILE*), 0);
	int err = file_nsp == NULL;

	if (file_nsp != NULL) {
		ccBegin(rt, NULL);

		err = err || !ccAddCall(rt, FILE_open, "r", "File Open(char path[]);");
		err = err || !ccAddCall(rt, FILE_open, "w", "File Create(char path[]);");
		err = err || !ccAddCall(rt, FILE_open, "a", "File Append(char path[]);");

		err = err || !ccAddCall(rt, FILE_peek, NULL, "int Peek(File file);");
		err = err || !ccAddCall(rt, FILE_getc, NULL, "int Read(File file);");
		err = err || !ccAddCall(rt, FILE_read, NULL, "int Read(File file, uint8 buff[]);");
		err = err || !ccAddCall(rt, FILE_gets, NULL, "int ReadLine(File file, uint8 buff[]);");

		err = err || !ccAddCall(rt, FILE_putc, NULL, "int Write(File file, uint8 byte);");
		err = err || !ccAddCall(rt, FILE_write, NULL, "int Write(File file, uint8 buff[]);");

		//~ err = err || !ccAddCall(rt, FILE_puts, NULL, "int puts(char buff[], File file);");
		//~ err = err || !ccAddCall(rt, FILE_ungetc, NULL, "int ungetc(int chr, File file);");

		err = err || !ccAddCall(rt, FILE_flush, NULL, "void Flush(File file);");
		err = err || !ccAddCall(rt, FILE_close, NULL, "void Close(File file);");

		//~ err = err || !ccAddCall(rt, FILE_memory, (void*)0, "File MemIn(char data[]);");
		//~ err = err || !ccAddCall(rt, FILE_memory, (void*)1, "File MemOut(char data[]);");

		err = err || !ccAddCall(rt, FILE_stream, (void*)0, "File StdIn;");
		err = err || !ccAddCall(rt, FILE_stream, (void*)1, "File StdOut;");
		err = err || !ccAddCall(rt, FILE_stream, (void*)2, "File StdErr;");

		//~ err = err || !ccAddCall(rt, FILE_mkdirs, NULL, "bool mkdirs(char path[]);");
		//~ err = err || !ccAddCall(rt, FILE_exists, NULL, "bool Exists(char path[]);");
		//~ err = err || !ccAddCall(rt, FILE_delete, NULL, "bool Delete(char path[]);");

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

		err = err || !ccAddCall(rt, FILE_properties, NULL, "Path Path(char path[]);");

		 */

		ccEnd(rt, file_nsp);
	}

	return err;
}
