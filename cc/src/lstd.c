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
static int f64sin(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sin(x));
	return 0;
	(void)_;
}
static int f64cos(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, cos(x));
	return 0;
	(void)_;
}
static int f64tan(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, tan(x));
	return 0;
	(void)_;
}
static int f64log(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, log(x));
	return 0;
	(void)_;
}
static int f64exp(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, exp(x));
	return 0;
	(void)_;
}
static int f64pow(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, pow(x, y));
	//~ debug("pow(%g, %g) := %g", x, y, pow(x, y));
	return 0;
	(void)_;
}
static int f64sqrt(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sqrt(x));
	return 0;
	(void)_;
}
static int f64atan2(state rt, void* _) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, atan2(x, y));
	return 0;
	(void)_;
}
//#}#endregion

//#{#region bit operations
static int b64not(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	reti64(rt, ~x);
	return 0;
	(void)_;
}
static int b64and(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	uint64_t y = argi64(rt, 8);
	reti64(rt, x & y);
	return 0;
	(void)_;
}
static int b64ior(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	uint64_t y = argi64(rt, 8);
	reti64(rt, x | y);
	return 0;
	(void)_;
}
static int b64xor(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	uint64_t y = argi64(rt, 8);
	reti64(rt, x ^ y);
	return 0;
	(void)_;
}
static int b64shl(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	int32_t y = argi32(rt, 8);
	reti64(rt, x << y);
	return 0;
	(void)_;
}
static int b64shr(state rt, void* _) {
	uint64_t x = argi64(rt, 0);
	int32_t y = argi32(rt, 8);
	reti64(rt, x >> y);
	return 0;
	(void)_;
}
static int b64sar(state rt, void* _) {
	int64_t x = argi64(rt, 0);
	int32_t y = argi32(rt, 8);
	reti64(rt, x >> y);
	return 0;
	(void)_;
}

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

static int bitFunctions(state rt, void *function) {
	switch ((bitOperation)function) {

		case b32_btc: {
			uint32_t x = argi32(rt, 0);
			x -= ((x >> 1) & 0x55555555);
			x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
			x = (((x >> 4) + x) & 0x0f0f0f0f);
			x += (x >> 8) + (x >> 16);
			reti32(rt, x & 0x3f);
			return 0;
		}
		//case b64_btc:

		case b32_bsf: {
			int result = 0;
			uint32_t x = argi32(rt, 0);
			if ((x & 0x0000ffff) == 0) { result += 16; x >>= 16; }
			if ((x & 0x000000ff) == 0) { result +=  8; x >>=  8; }
			if ((x & 0x0000000f) == 0) { result +=  4; x >>=  4; }
			if ((x & 0x00000003) == 0) { result +=  2; x >>=  2; }
			if ((x & 0x00000001) == 0) { result +=  1; }
			reti32(rt, x ? result : -1);
			return 0;
		}

		case b64_bsf: {
			int result = -1;
			uint64_t x = argi64(rt, 0);
			if (x != 0) {
				result = 0;
				if ((x & 0x00000000ffffffffULL) == 0) { result += 32; x >>= 32; }
				if ((x & 0x000000000000ffffULL) == 0) { result += 16; x >>= 16; }
				if ((x & 0x00000000000000ffULL) == 0) { result +=  8; x >>=  8; }
				if ((x & 0x000000000000000fULL) == 0) { result +=  4; x >>=  4; }
				if ((x & 0x0000000000000003ULL) == 0) { result +=  2; x >>=  2; }
				if ((x & 0x0000000000000001ULL) == 0) { result +=  1; }
			}
			reti32(rt, result);
			return 0;
		}

		case b32_bsr: {
			uint32_t x = argi32(rt, 0);
			unsigned ans = 0;
			if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
			if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
			if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
			if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000002) != 0) { ans +=  1; }
			reti32(rt, x ? ans : -1);
			return 0;
		}

		case b64_bsr: {
			uint64_t x = argi64(rt, 0);
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

		case b32_bhi: {
			uint32_t x = argi32(rt, 0);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			reti32(rt, x - (x >> 1));
			return 0;
		}

		case b64_bhi: {
			uint64_t x = argi64(rt, 0);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			x |= x >> 32;
			reti64(rt, x - (x >> 1));
			return 0;
		}

		case b32_blo: {
			uint32_t x = argi32(rt, 0);
			reti32(rt, x & -x);
			return 0;
		}

		case b64_blo: {
			uint64_t x = argi64(rt, 0);
			reti64(rt, x & -x);
			return 0;
		}

		case b32_swp: {
			uint32_t x = argi32(rt, 0);
			x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
			x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
			x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
			x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
			reti64(rt, (x >> 16) | (x << 16));
			return 0;
		}
		//case b64_swp:

		/*case b32_zxt: {
			uint32_t val = popi32(rt);
			int32_t ofs = popi32(rt);
			int32_t cnt = popi32(rt);
			val <<= 32 - (ofs + cnt);
			reti32(rt, val >> (32 - cnt));
			return 0;
		}
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
		// */
	}
	return -1;
}
//#}#endregion

//#{#region file operations
static inline int argpos(int *argp, int size) {
	int result = *argp;
	*argp += padded(size || 1, vm_size);
	return result;
}

//~ #define logFILE(msg, ...) prerr(msg, ##__VA_ARGS__)
#define logFILE(msg, ...)

static int FILE_open(state rt, void* mode) {	// void Open(char filename[]);
	int argc = 0;
	char *name = argref(rt, argpos(&argc, vm_size));
	// int slen = argi32(rt, argpos(&argc, vm_size));
	FILE *file = fopen(name, mode);
	rethnd(rt, file);

	logFILE("Name: %s, Mode: %s, File: %x", name, mode, file);
	return 0;
}
static int FILE_close(state rt, void* _) {	// void close(File file);
	FILE *file = arghnd(rt, 0);
	logFILE("File: %x", file);
	fclose(file);
	return 0;
	(void)_;
}

static int FILE_getc(state rt, void* _) {
	FILE *file = arghnd(rt, 0);
	reti32(rt, fgetc(file));
	return 0;

	(void)_;
}
static int FILE_peek(state rt, void* _) {
	FILE *file = arghnd(rt, 0);
	int chr = ungetc(getc(file), file);
	reti32(rt, chr);
	return 0;

	(void)_;
}
static int FILE_read(state rt, void* _) {	// int read(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(rt, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(rt, argpos(&argc, vm_size));
	int len = argi32(rt, argpos(&argc, vm_size));
	len = fread(buff, len, 1, file);
	reti32(rt, len);
	return 0;
	(void)_;
}
static int FILE_gets(state rt, void* _) {	// int fgets(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(rt, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(rt, argpos(&argc, vm_size));
	int len = argi32(rt, argpos(&argc, vm_size));
	logFILE("Buff: %08x[%d], File: %x", buff, len, file);
	if (feof(file)) {
		reti32(rt, -1);
	}
	else {
		long pos = ftell(file);
		char *unused = fgets(buff, len, file);
		reti32(rt, ftell(file) - pos);
		(void)unused;
	}
	return 0;
	(void)_;
}

static int FILE_putc(state rt, void* _) {
	int argc = 0;
	FILE *file = arghnd(rt, argpos(&argc, sizeof(FILE *)));
	int data = argi32(rt, argpos(&argc, vm_size));
	logFILE("Data: %c, File: %x", data, file);
	reti32(rt, putc(data, file));
	return 0;
	(void)_;
}
static int FILE_write(state rt, void* _) {	// int write(File &f, uint8 buff[])
	int argc = 0;
	FILE *file = arghnd(rt, argpos(&argc, sizeof(FILE *)));
	char *buff = argref(rt, argpos(&argc, vm_size));
	int len = argi32(rt, argpos(&argc, vm_size));
	len = fwrite(buff, len, 1, file);
	reti32(rt, len);
	return 0;
	(void)_;
}

static int FILE_flush(state rt, void* _) {
	FILE *file = arghnd(rt, 0);
	logFILE("File: %x", file);
	fflush(file);
	return 0;
	(void)_;
}
//#}#endregion

//#{#region reflection operations
static int typenameGetName(state rt, void* _) {
	symn sym = mapsym(rt, argref(rt, 0));
	reti32(rt, vmOffset(rt, sym->name));
	return 0;
	(void)_;
}
static int typenameGetFile(state rt, void* _) {
	symn sym = mapsym(rt, argref(rt, 0));
	reti32(rt, vmOffset(rt, sym->file));
	return 0;
	(void)_;
}
static int typenameGetBase(state rt, void* _) {
	symn sym = mapsym(rt, argref(rt, 0));
	reti32(rt, vmOffset(rt, sym->type));
	return 0;
	(void)_;
}
//#}#endregion

//#{#region io functions (exit, rand, clock, print, debug)
typedef enum {
	miscOpExit,
	miscOpRand32,

	timeOpTime32,
	timeOpClock32,
	timeOpClocksPS,

	timeOpProc64,
	timeOpClck64,

	miscOpPutStr,
	miscOpPutFmt,
} miscOperation;

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

static int miscFunction(state rt, void* data) {
	switch ((miscOperation)data) {
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
		case timeOpTime32: {
			reti32(rt, time(NULL));
			return 0;
		}
		case timeOpClock32: {
			reti32(rt, clock());
			return 0;
		}
		case timeOpClocksPS: {
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

static int libCallHalt(state rt, void* _) {
	return 0;
	(void)_;
	(void)rt;
}
static int libCallDebug(state rt, void* _) {
	// debug(string message, int level, int trace, bool abort, typename objtyp, pointer objref);

	int arg = 0;
	#define poparg(__TYPE) (arg += sizeof(__TYPE)) - sizeof(__TYPE)
	char* file = argstr(rt, poparg(int32_t));
	int   line = argi32(rt, poparg(int32_t));
	//~ char* func = argstr(rt, poparg(char*));

	char* message = argstr(rt, poparg(int32_t));
	int loglevel = argi32(rt, poparg(int32_t));
	int tracelevel = argi32(rt, poparg(int32_t));
	void* objref = argref(rt, poparg(int32_t));
	symn objtyp = argref(rt, poparg(int32_t));

	// skip loglevel 0
	if (loglevel != 0) {
		FILE* logf = rt ? rt->logf : stdout;
		if (logf) {
			int isOutput = 0;

			// position where the function was invoked
			if (file && line) {
				fputfmt(rt->logf, "%s:%u", file, line);
				isOutput = 1;
			}

			// the message to be printed
			if (message != NULL) {
				fputfmt(rt->logf, ": %s", message);
				isOutput = 1;
			}

			// specified object
			if (objtyp && objref) {
				fputfmt(rt->logf, ": ");
				//~ fputfmt(rt->logf, "typ@%x, ref@%x: ", (unsigned char*)objtyp - rt->_mem, (unsigned char*)objref - rt->_mem);
				fputval(rt, rt->logf, objtyp, objref, 0);
				isOutput = 1;
			}

			// print stack trace
			if (rt->dbg && tracelevel > 0) {
				int i, pos = rt->dbg->tracePos;
				if (tracelevel > pos) {
					tracelevel = pos;
				}
				// i = 1: skip debug function.
				for (i = 1; i < tracelevel; ++i) {
					dbgInfo trInfo = getCodeMapping(rt, rt->dbg->trace[pos - i].pos);
					symn fun = rt->dbg->trace[pos - i - 1].sym;

					if (trInfo) {
						file = trInfo->file;
						line = trInfo->line;
					}
					else {
						file = NULL;
						line = 0;
					}

					if (fun == NULL) {
						fun = mapsym(rt, rt->dbg->trace[pos - i - 1].cf);
					}

					file = file ? file : "eternal.code";
					fputfmt(rt->logf, "\n\t%s:%u: %?+T", file, line, fun);
					isOutput = 1;
				}
				if (i < pos) {
					fputfmt(rt->logf, "\n\t... %d more", pos - i);
				}
				//~ perr(rt, 1, NULL, 0, ": stack trace[%d] not supported yet", tracelevel);
			}
			if (isOutput) {
				fputfmt(logf, "\n");
			}
		}
	}

	// abor the application
	if (loglevel < 0) {
		//~ abort();
		return -loglevel;
	}

	return 0;
	(void)_;
}
static int libCallMemMgr(state rt, void* _) {
	void* old = argref(rt, 0);
	int size = argi32(rt, 4);
	void* res = rtAlloc(rt, old, size);
	reti32(rt, vmOffset(rt, res));
	return 0;
	(void)_;
}

/*
 *
 */
int libCallHaltDebug(state rt, void* _) {
	symn var = rt->libc.libc->prms;
	int argc = (char*)rt->libc.retv - (char*)rt->libc.argv;

	for ( ; var; var = var->next) {
		char* ofs;

		if (var->call)
			continue;

		if (var->kind != TYPE_ref)
			continue;

		if (var->file && var->line)
			fputfmt(stdout, "%s:%d:", var->file, var->line);
		else
			fputfmt(stdout, "var: ");

		fputfmt(stdout, "@0x%06x[size: %d]: ", var->offs, var->size);

		if (var->stat) {
			// static variable.
			ofs = (void*)(rt->_mem + var->offs);
		}
		else {
			// argument or local variable.
			ofs = ((char*)rt->libc.argv) + argc - var->offs;
		}

		fputval(rt, stdout, var, (stkval*)ofs, 0);
		fputc('\n', stdout);
	}

	rtAlloc(rt, NULL, 0);

	return 0;
	(void)_;
}

//#}#endregion

int install_base(state rt, int mode, int onHalt(state, void*)) {
	int error = 0;
	ccState cc = rt->cc;
	ccAddCall(rt, onHalt ? onHalt : libCallHalt, NULL, "void Halt(int Code);");

	if (cc->type_ptr && (mode & creg_tptr)) {
		cc->libc_mem = ccAddCall(cc->s, libCallMemMgr, NULL, "pointer memmgr(pointer ptr, int32 size);");
	}
	if (cc->type_var && (mode & creg_tvar)) {
		cc->libc_dbg = ccAddCall(rt, libCallDebug, NULL, "void debug(string message, int level, int trace, variant objref);");
	}

	//~ TODO: temporarly add null to variant type.
	if (rt->cc->type_var && !rt->cc->type_var->prms) {
		ccBegin(rt, NULL);
		//~ install(cc, "null", ATTR_const | ATTR_stat | TYPE_def, TYPE_i64, 2 * vm_size, rt->cc->type_var, intnode(cc, 0));
		install(cc, "null", ATTR_const | ATTR_stat | TYPE_def, TYPE_any, 2 * vm_size, rt->cc->type_var, NULL);
		ccEnd(rt, rt->cc->type_var);
	}

	// 4 reflection
	if (cc->type_rec && (mode & creg_tvar)) {
		symn arg = NULL;
		enter(cc, NULL);
		if ((arg = install(cc, "line", ATTR_const | TYPE_ref, TYPE_any, vm_size, cc->type_i32, NULL))) {
			arg->offs = offsetOf(symn, line);
		}
		else {
			error = 1;
		}

		if ((arg = install(cc, "size", ATTR_const | TYPE_ref, TYPE_any, vm_size, cc->type_i32, NULL))) {
			arg->offs = offsetOf(symn, size);
		}
		else {
			error = 1;
		}

		if ((arg = install(cc, "offset", ATTR_const | TYPE_ref, TYPE_any, vm_size, cc->type_i32, NULL))) {
			arg->offs = offsetOf(symn, offs);
			arg->pfmt = "@%06x";
		}
		else {
			error = 1;
		}

		if ((arg = ccAddCall(rt, typenameGetFile, NULL, "string file;"))) {
			arg->stat = 0;
			arg->memb = 1;
			rt->cc->libc->chk += 1;
			rt->cc->libc->pop += 1;
		}
		else {
			error = 1;
		}

		if ((arg = ccAddCall(rt, typenameGetName, NULL, "string name;"))) {
			arg->stat = 0;
			arg->memb = 1;
			rt->cc->libc->chk += 1;
			rt->cc->libc->pop += 1;
		}
		else {
			error = 1;
		}

		error = error || !ccAddCall(rt, typenameGetBase, NULL, "typename base(typename type);");

		ccExtEnd(rt, cc->type_rec, 0);

		/* TODO: more 4 reflection
		enum BindingFlags {
			//inline     = 0x000000;	// this is not available at runtime.
			typename     = 0x000001;
			function     = 0x000002;	//
			variable     = 0x000003;	// functions and typenames are also variables
			attr_static  = 0x000004;
			attr_const   = 0x000008;
		}
		//
		error = error || !ccAddCall(rt, typeFunction, (void*)typeOpGetFile, "variant setValue(typename field, variant value)");
		error = error || !ccAddCall(rt, typeFunction, (void*)typeOpGetFile, "variant getValue(typename field)");

		//~ install(cc, "typename[] lookup(variant &obj, int options, string name, variant args...)");
		//~ install(cc, "variant invoke(variant &obj, int options, string name, variant args...)");
		//~ install(cc, "bool canassign(typename toType, variant value, bool canCast)");
		//~ install(cc, "bool instanceof(typename &type, variant obj)");

		//~ */
	}
	return error;
}

int install_stdc(state rt, char* file, int level) {
	symn nsp = NULL;		// namespace
	int i, err = 0;
	struct {
		int (*fun)(state, void* data);
		miscOperation data;
		char* def;
	}
	math[] = {		// sin, cos, sqrt, ...
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
	misc[] = {		// IO/MEM/EXIT

		//~ {miscFunction, &miscOpArgc,			"int32 argc();"},
		//~ {miscFunction, &miscOpArgv,			"string arg(int arg);"},

		{miscFunction, miscOpRand32,		"int32 rand();"},

		{miscFunction, timeOpTime32,		"int32 time();"},
		{miscFunction, timeOpClck64,		"int32 ticks();"},
		{miscFunction, timeOpClocksPS,		"float64 ticks(int32 ticks);"},

		{miscFunction, timeOpClck64,		"int64 uTimeHp();"},
		{miscFunction, timeOpProc64,		"int64 sTimeHp();"},

		{miscFunction, miscOpPutStr,		"void print(string val);"},
		{miscFunction, miscOpPutFmt,		"void print(string fmt, int64 val);"},

		// TODO: include some of the compiler functions
		// for reflection. (lookup, import, logger, assert, exec?, ...)

	};

	// Add bitwise operations to int64 as functions
	if (rt->cc->type_i64 && !rt->cc->type_i64->prms) {
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

	// System.Exit(int code), ...
	if ((nsp = ccBegin(rt, "System"))) {
		// libcall will return 0 on failure,
		// 'err = err || !libcall...' <=> 'if (!err) err = !libcall...'
		// will skip forward libcalls if an error ocurred

		err = err || !ccAddCall(rt, miscFunction, (void*)miscOpExit, "void Exit(int Code);");

		//~ install(cc, "Args", TYPE_arr, 0, 0);// string Args[];
		//~ install(cc, "Env", TYPE_def, 0, 0);	// string Env[string];

		ccEnd(rt, nsp);
	}

	if (err == 0 && file != NULL) {
		return ccAddCode(rt, level, file, 1, NULL);
	}

	(void)bitFunctions;
	return err;
}

int install_file(state rt) {
	symn file_nsp = ccAddType(rt, "File", sizeof(FILE*), 0);
	int err = file_nsp == NULL;
	if (file_nsp != NULL) {
		ccBegin(rt, NULL);

		err = err || !ccAddCall(rt, FILE_open,  "r", "File Open(char path[]);");
		err = err || !ccAddCall(rt, FILE_open,  "w", "File Create(char path[]);");
		err = err || !ccAddCall(rt, FILE_open,  "a", "File Append(char path[]);");

		err = err || !ccAddCall(rt, FILE_peek, NULL, "int Peek(File file);");
		err = err || !ccAddCall(rt, FILE_getc, NULL, "int Read(File file);");
		err = err || !ccAddCall(rt, FILE_read, NULL, "int Read(File file, uint8 buff[]);");
		err = err || !ccAddCall(rt, FILE_gets, NULL, "int ReadLine(File file, uint8 buff[]);");

		err = err || !ccAddCall(rt, FILE_putc, NULL, "int Write(File file, uint8 byte);");
		err = err || !ccAddCall(rt, FILE_write, NULL, "int Write(File file, uint8 buff[]);");
		err = err || !ccAddCall(rt, FILE_flush, NULL, "void Flush(File file);");

		//~ err = err || !ccAddCall(rt, FILE_puts, NULL, "int puts(char buff[], File file);");
		//~ err = err || !ccAddCall(rt, FILE_ungetc, NULL, "int ungetc(int chr, File file);");

		err = err || !ccAddCall(rt, FILE_close, NULL, "void Close(File file);");

		//~ err = err || !ccAddCall(rt, FILE_delete, NULL, "bool Delete(char path[]);");
		//~ err = err || !ccAddCall(rt, FILE_delete, NULL, "bool Exists(char path[]);");

		ccEnd(rt, file_nsp);
	}
	return err;
}
