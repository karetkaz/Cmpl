//~ wcl386 -zq -ei -6s -d2  -fe=../main code.c clog.c parse.c tree.c type.c ccvm.c main.c
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "ccvm.h"

// default values
static const int wl = 9;		// warning level
static const int ol = 2;		// optimize level
static const int cc = 1;		// execution cores
#define memsize (4 << 20)		// runtime size(4M)
static char mem[memsize];

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

char *parsei32(const char *str, int32_t *res, int radix) {
	int64_t result = 0;
	int sign = 1;

	//~ ('+' | '-')?
	switch (*str) {
		case '-':
			sign = -1;
		case '+':
			str += 1;
	}

	if (radix == 0) {		// detect
		radix = 10;
		if (*str == '0') {
			str += 1;

			switch (*str) {

				default:
					radix = 8;
					break;

				case 'b':
				case 'B':
					radix = 2;
					str += 1;
					break;

				case 'o':
				case 'O':
					radix = 8;
					str += 1;
					break;

				case 'x':
				case 'X':
					radix = 16;
					str += 1;
					break;

			}
		}
	}

	//~ ([0-9])*
	while (*str) {
		int value = *str;
		if (value >= '0' && value <= '9')
			value -= '0';
		else if (value >= 'a' && value <= 'z')
			value -= 'a' - 10;
		else if (value >= 'A' && value <= 'Z')
			value -= 'A' - 10;
		else break;

		if (value > radix)
			break;

		result *= radix;
		result += value;
		str += 1;
	}

	*res = sign * result;

	return (char *)str;
}
char *matchstr(const char *t, const char *p, int ic) {
	int i = 0;//, ic = flgs & 1;

	for ( ; *t && p[i]; ++t, ++i) {
		if (p[i] == '*') {
			if (matchstr(t, p + i + 1, ic))
				return (char *)(t - i);
			return 0;
		}

		if (p[i] == '?' || p[i] == *t)		// skip | match
			continue;

		if (ic && p[i] == tolower(*t))		// ignore case
			continue;

		t -= i;
		i = -1;
	}

	while (p[i] == '*')			// "a***" is "a"
		++p;					// keep i for return

	return (char *)(p[i] ? 0 : t - i);
}
char *parsecmd(char *ptr, char *cmd, char *sws) {
	while (*cmd && *cmd == *ptr)
		cmd++, ptr++;

	if (*cmd != 0)
		return 0;

	if (*ptr == 0)
		return ptr;

	while (strchr(sws, *ptr))
		++ptr;

	return ptr;
}

/*static int cctext(state s, int wl, char *file, int line, char *buff) {
	if (!ccOpen(s, srcText, buff))
		return -2;

	ccSource(s->cc, file, line);
	return parse(s->cc, 0, wl);
}// */

//{#region bit operations
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
static int bits_call(state s) {
	switch (s->libc->offs) {
		case b64_cmt: {
			uint64_t x = popi64(s);
			setret(s, uint64_t, ~x);
		} return 0;
		case b64_and: {
			uint64_t x = popi64(s);
			uint64_t y = popi64(s);
			setret(s, uint64_t, x & y);
		} return 0;
		case b64_or:  {
			uint64_t x = popi64(s);
			uint64_t y = popi64(s);
			setret(s, uint64_t, x | y);
		} return 0;
		case b64_xor: {
			uint64_t x = popi64(s);
			uint64_t y = popi64(s);
			setret(s, uint64_t, x ^ y);
		} return 0;
		case b64_shl: {
			uint64_t x = popi64(s);
			uint32_t y = popi32(s);
			setret(s, uint64_t, x << y);
		} return 0;
		case b64_shr: {
			uint64_t x = popi64(s);
			uint32_t y = popi32(s);
			setret(s, uint64_t, x >> y);
			//~ debug("%D >> %d = %D", x, y, x >> y);
		} return 0;
		case b64_sar: {
			int64_t x = popi64(s);
			uint32_t y = popi32(s);
			setret(s, uint64_t, x >> y);
		} return 0;

		case b32_btc: {
			uint32_t x = popi32(s);
			x -= ((x >> 1) & 0x55555555);
			x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
			x = (((x >> 4) + x) & 0x0f0f0f0f);
			x += (x >> 8) + (x >> 16);
			setret(s, uint32_t, x & 0x3f);
		} return 0;
		//case b64_btc:

		case b32_bsf: {
			uint32_t x = popi32(s);
			int ans = 0;
			if ((x & 0x0000ffff) == 0) { ans += 16; x >>= 16; }
			if ((x & 0x000000ff) == 0) { ans +=  8; x >>=  8; }
			if ((x & 0x0000000f) == 0) { ans +=  4; x >>=  4; }
			if ((x & 0x00000003) == 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000001) == 0) { ans +=  1; }
			setret(s, uint32_t, x ? ans : -1);
		} return 0;
		case b64_bsf: {
			uint64_t x = popi64(s);
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
			setret(s, int32_t, ans);
		} return 0;

		case b32_bsr: {
			uint32_t x = popi32(s);
			unsigned ans = 0;
			if ((x & 0xffff0000) != 0) { ans += 16; x >>= 16; }
			if ((x & 0x0000ff00) != 0) { ans +=  8; x >>=  8; }
			if ((x & 0x000000f0) != 0) { ans +=  4; x >>=  4; }
			if ((x & 0x0000000c) != 0) { ans +=  2; x >>=  2; }
			if ((x & 0x00000002) != 0) { ans +=  1; }
			setret(s, uint32_t, x ? ans : -1);
		} return 0;
		case b64_bsr: {
			uint64_t x = popi64(s);
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
			setret(s, int32_t, ans);
		} return 0;

		case b32_bhi: {
			uint32_t x = popi32(s);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			setret(s, uint32_t, x - (x >> 1));
		} return 0;
		case b64_bhi: {
			uint64_t x = popi64(s);
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			x |= x >> 32;
			setret(s, uint64_t, x - (x >> 1));
		} return 0;

		case b32_blo: {
			uint32_t x = popi32(s);
			setret(s, uint32_t, x & -x);
		} return 0;
		case b64_blo: {
			uint64_t x = popi64(s);
			setret(s, uint64_t, x & -x);
		} return 0;

		case b32_swp: {
			uint32_t x = popi32(s);
			x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
			x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
			x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
			x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
			setret(s, uint32_t, (x >> 16) | (x << 16));
		} return 0;
		//case b64_swp:

		case b32_zxt: {
			uint32_t val = popi32(s);
			int32_t ofs = popi32(s);
			int32_t cnt = popi32(s);
			val <<= 32 - (ofs + cnt);
			setret(s, int32_t, val >> (32 - cnt));
		} return 0;
		case b64_zxt: {
			uint64_t val = popi64(s);
			int32_t ofs = popi32(s);
			int32_t cnt = popi32(s);
			val <<= 64 - (ofs + cnt);
			setret(s, int64_t, val >> (64 - cnt));
		} return 0;

		case b32_sxt: {
			int32_t val = popi32(s);
			int32_t ofs = popi32(s);
			int32_t cnt = popi32(s);
			val <<= 32 - (ofs + cnt);
			setret(s, int32_t, val >> (32 - cnt));
		} return 0;
		case b64_sxt: {
			int64_t val = popi64(s);
			int32_t ofs = popi32(s);
			int32_t cnt = popi32(s);
			val <<= 64 - (ofs + cnt);
			setret(s, int64_t, val >> (64 - cnt));
		} return 0;
	}
	return -1;
}
static int install_bits(state rt) {
	int err = 0;
	symn cls;
	if ((cls = ccBegin(rt, "bits"))) {
		// libcall will return 0 on failure, 
		// 'err = err || !libcall...' <=> 'if (!err) err = !libcall...'
		// will skip forward libcalls if an error ocurred

		err = err || !libcall(rt, bits_call, b64_cmt, "int64 cmt(int64 x);");
		err = err || !libcall(rt, bits_call, b64_and, "int64 and(int64 x, int64 y);");
		err = err || !libcall(rt, bits_call, b64_or,  "int64 or(int64 x, int64 y);");
		err = err || !libcall(rt, bits_call, b64_xor, "int64 xor(int64 x, int64 y);");
		err = err || !libcall(rt, bits_call, b64_shl, "int64 shl(int64 x, int32 y);");
		err = err || !libcall(rt, bits_call, b64_shr, "int64 shr(int64 x, int32 y);");
		err = err || !libcall(rt, bits_call, b64_sar, "int64 sar(int64 x, int32 y);");

		//~ err = err || !libcall(rt, bits_call, b64_btc, "int32 btc(int64 x);");
		err = err || !libcall(rt, bits_call, b64_bsf, "int32 bsf(int64 x);");
		err = err || !libcall(rt, bits_call, b64_bsr, "int32 bsr(int64 x);");
		err = err || !libcall(rt, bits_call, b64_bhi, "int32 bhi(int64 x);");
		err = err || !libcall(rt, bits_call, b64_blo, "int32 blo(int64 x);");
		//~ err = err || !libcall(rt, bits_call, b64_swp, "int64 swp(int64 x);");
		err = err || !libcall(rt, bits_call, b64_zxt, "int64 zxt(int64 val, int offs, int bits);");
		err = err || !libcall(rt, bits_call, b64_sxt, "int64 sxt(int64 val, int offs, int bits);");

		/// 32 bits
		err = err || !libcall(rt, bits_call, b32_btc, "int32 btc(int32 x);");
		err = err || !libcall(rt, bits_call, b32_bsf, "int32 bsf(int32 x);");
		err = err || !libcall(rt, bits_call, b32_bsr, "int32 bsr(int32 x);");
		err = err || !libcall(rt, bits_call, b32_bhi, "int32 bhi(int32 x);");
		err = err || !libcall(rt, bits_call, b32_blo, "int32 blo(int32 x);");
		err = err || !libcall(rt, bits_call, b32_swp, "int32 swp(int32 x);");
		err = err || !libcall(rt, bits_call, b32_zxt, "int32 zxt(int32 val, int offs, int bits);");
		err = err || !libcall(rt, bits_call, b32_sxt, "int32 sxt(int32 val, int offs, int bits);");
		ccEnd(rt, cls);
	}
	return err;
}
//}#endregion

void usage(char* prog, char* cmd) {
	if (cmd == NULL) {
		fputfmt(stdout, "Usage: %s <command> [options] ...\n", prog);
		fputfmt(stdout, "<Commands>\n");
		fputfmt(stdout, "\t-c: compile\n");
		fputfmt(stdout, "\t-e: execute\n");
		fputfmt(stdout, "\t-d: diassemble\n");
		fputfmt(stdout, "\t-h: help\n");
		fputfmt(stdout, "\t=<expression>: eval\n");
	}
	else if (strcmp(cmd, "-c") == 0) {
		fputfmt(stdout, "compile: %s -c [options] file ...args\n", prog);
		fputfmt(stdout, "Options:\n");

		fputfmt(stdout, "\t[Output]\n");
		fputfmt(stdout, "\t-t Output is tags\n");
		fputfmt(stdout, "\t-c Output is ast code\n");
		fputfmt(stdout, "\t-s Output is asm code\n");
		fputfmt(stdout, "\telse output is binary code\n");
		fputfmt(stdout, "\t-o <file> Put output into <file>. [default=stdout]\n");

		fputfmt(stdout, "\t[Loging]\n");
		fputfmt(stdout, "\t-wa all warnings\n");
		fputfmt(stdout, "\t-wx treat warnings as errors\n");
		fputfmt(stdout, "\t-w<num> set warning level to <num> [default=%d]\n", wl);
		fputfmt(stdout, "\t-l <file> Put errors into <file>. [default=stderr]\n");

		fputfmt(stdout, "\t[Execute and Debug]\n");
		fputfmt(stdout, "\t-x<n> execute on <n> procs [default=%d]\n", cc);
		fputfmt(stdout, "\t-xd<n> debug on <n> procs [default=%d]\n", cc);

		//~ fputfmt(stdout, "\t[Debug & Optimization]\n");
	}
	else if (strcmp(cmd, "-h") == 0) {
		fputfmt(stdout, "command: '-h': help\n");
	}
	else {
		fputfmt(stdout, "invalid help for: '%s'\n", cmd);
	}
}

int evalexp(ccState s, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	source(s, 0, text);

	ast = expr(s, 0);
	typ = typecheck(s, 0, ast);
	tid = eval(&res, ast);

	if (peek(s))
		error(s->s, s->file, s->line, "unexpected: `%k`", peek(s));

	fputfmt(stdout, "expr: %+K", ast);
	fputfmt(stdout, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(stdout, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(stdout, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);

	return -1;
}

int reglibs(state s, char *stdlib) {
	int err = 0;
	//~ symn App;

	//~ if (!libcall(s, libCallExitDebug, 0, "void debug1(int x, int y, int z);")) err = 1;
	//~ if (!libcall(s, libCallExitDebug, 0, "void debug1(int x, int y, int z, int u, int v, int w);")) err = 1;

	/*
	if ((App = ccBegin(s, "App"))) {
		symn App_xxx;
		if ((App_xxx = ccBegin(s, "Xxx"))) {
			err = err || !ccDefineInt(s, "width", 2);
			err = err || !ccDefineStr(s, "width2", "alma a fa melett");
			ccEnd(s, App_xxx);
		}
		err = err || !ccDefineInt(s, "width", 2);
		err = err || !ccDefineInt(s, "height", 8);
		ccEnd(s, App);
	}// */

	err = err || install_stdc(s, stdlib, wl);
	err = err || install_bits(s);

	return err;
}

int compile(state s, int wl, char *file) {
	int result;
	#if DEBUGGING > 1
	int size = 0;
	#endif

	if (!ccOpen(s, srcFile, file))
		return -1;

	#if DEBUGGING > 1
	//~ debug("after init:%d Bytes", s->cc->_beg - s->_mem);
	size = (s->cc->_beg - (char*)s->_mem);
	#endif

	result = parse(s->cc, 0, wl);

	#if DEBUGGING > 1
	//~ debug("%s: init(%.2F); scan(%.2F) KBytes", file, size / 1024., (s->cc->_beg - s->_mem) / 1024.);
	#endif

	return result;
}

int installDll(state s, int ccApiMain(state s, stateApi api)) {
	struct stateApi api;
	api.ccBegin = ccBegin;
	api.ccEnd = ccEnd;
	api.libcall = libcall;
	api.install = installtyp;
	return ccApiMain(s, &api);

}

#if __linux__
#include <dlfcn.h>
static int importLib(state s, const char *path, const char *init) {
	int result = 0;
	void *lib = dlopen(path, RTLD_NOW);
	if (lib != NULL) {
		void *sym = dlsym(lib, init);
		if (sym != NULL) {
			result = installDll(s, sym);
		}
		else {
			result = -2;
		}
		dlclose(lib);
	}
	else {
		result = -1;
	}
	return result;
}

#else

#include <windows.h>
static int importLib(state s, const char *path, const char *init) {
	int result = 0;
	typedef int (*ccApiMain)(state s, stateApi api);
	HANDLE lib = LoadLibrary(path);
	if (lib != NULL) {
		ccApiMain sym = (ccApiMain)GetProcAddress(lib, init);
		if (sym != NULL) {
			result = installDll(s, sym);
		}
		else {
			result = -2;
		}
		//~ FreeLibrary((HINSTANCE)lib);
	}
	else {
		result = -1;
	}
	return result;
}

#endif

static int printvars = 0;
static int dbgCon(state s, int pu, void *ip, long* bp, int ss);
void vm_fputval(state, FILE *fout, symn var, stkval* ref, int flgs);
static int libCallExitDebug(state rt) {
	symn arg = rt->libc->args;
	int argc = (char*)rt->retv - (char*)rt->argv;

	for ( ;arg; arg = arg->next) {
		char *ofs;

		if (arg->offs <= 0) {
			// global variable.
			ofs = (void*)(rt->_mem - arg->offs);
		}
		else {
			// argument or local variable.
			ofs = ((char*)rt->argv) + argc - arg->offs;
		}

		//~ debug("argv: %08x", rt->argv);
		//~ if (arg->kind != TYPE_ref && rt->args == rt->defs) continue;
		if (arg->kind != TYPE_ref) continue;

		if (arg->file && arg->line)
			fputfmt(stdout, "%s:%d:",arg->file, arg->line);
		else
			fputfmt(stdout, "var: ",arg->file, arg->line);

		fputfmt(stdout, "@%d[0x%08x]\t: ", arg->offs < 0 ? -1 : arg->offs, ofs);

		//~ fputfmt(stdout, "@%d[0x%08x]\t: ", arg->offs, ofs);
		//~ fputfmt(stdout, "@%d[0x%08x]\t: ", rt->argc - arg->offs, ofs);
		vm_fputval(rt, stdout, arg, (stkval*)ofs, 0);
		fputc('\n', stdout);
	}
	if (1) {	// memory info
		list mem;

		perr(rt, 0, NULL, 0, "memory info");
		for (mem = rt->_free; mem; mem = mem->next) {
			perr(rt, 0, NULL, 0, "free mem@%06x[%d]", mem->data - rt->_mem, mem->size);
		}
		for (mem = rt->_used; mem; mem = mem->next) {
			perr(rt, 0, NULL, 0, "used mem@%06x[%d]", mem->data - rt->_mem, mem->size);
		}
		//~ fatal("FixMe: %s", "try to defrag free memory");
	}

	return 0;
}

int program(int argc, char *argv[]) {
	state s = rtInit(mem, sizeof(mem));

	char *stdl = "../stdlib.cvx";		// standard library

	char *prg, *cmd;
	dbgf dbg = NULL;

	prg = argv[0];
	cmd = argv[1];
	if (argc < 2) {
		usage(prg, NULL);
	}
	else if (argc == 2 && *cmd == '=') {	// eval
		return evalexp(ccInit(s, creg_def, NULL), cmd + 1);
	}
	else if (strcmp(cmd, "-api") == 0) {
		ccState env = ccInit(s, creg_def, NULL);
		const int level = 2;
		symn glob;
		int i;

		if (!env)
			return -33;

		if (reglibs(s, stdl) != 0) {
			error(s, NULL, 0, "error registering lib calls");
			logfile(s, NULL);
			return -6;
		}

		glob = leave(env, NULL, 1);
		if (argc == 2) {
			dumpsym(stdout, glob, level);
		}
		else if (argc == 3 && strcmp(argv[2], ".") == 0) {
			while (glob) {
				dumpsym(stdout, glob, 0);
				glob = glob->defs;
			}
		}
		else for (i = 2; i < argc; i += 1) {
			symn sym = findsym(env, glob, argv[i]);
			if (sym) {
				dumpsym(stdout, sym, 0);
				dumpsym(stdout, sym->args, level);
			}
			else
				fputfmt(stderr, "symbol not found `%s`\n", argv[i]);
		}
		return ccDone(s);
	}
	else if (stricmp(cmd, "-c") == 0) {		// compile
		int level = -1, argi;
		int warn = wl;
		int opti = ol;
		int outc = 0;			// output
		char *srcf = 0;			// source
		char *logf = 0;			// logger
		char *outf = 0;			// output

		enum {
			gen_code = 0x0010,
			out_tags = 0x0001,	// tags
			out_tree = 0x0002,	// walk
			out_dasm = 0x0013,	// dasm
			run_code = 0x0014,	// exec
		};

		// no std lib
		if (cmd[1] == 'C')
			stdl = NULL;

		// options
		for (argi = 2; argi < argc; ++argi) {
			char *arg = argv[argi];

			// source file
			if (*arg != '-') {
				if (srcf) {
					fputfmt(stderr, "multiple source files are not suported\n");
					return -1;
				}
				srcf = argv[argi];
			}

			// output file
			else if (strcmp(arg, "-l") == 0) {		// log
				if (++argi >= argc || logf) {
					fputfmt(stderr, "logger error\n");
					return -1;
				}
				logf = argv[argi];
			}
			else if (strcmp(arg, "-o") == 0) {		// out
				if (++argi >= argc || outf) {
					fputfmt(stderr, "output error\n");
					return -1;
				}
				outf = argv[argi];
			}

			// output what
			else if (strncmp(arg, "-t", 2) == 0) {		// tags
				level = 2;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = gen_code | out_tags;
			}
			else if (strncmp(arg, "-T", 2) == 0) {		// tags / no cgen
				level = 2;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_tags;
			}
			else if (strncmp(arg, "-c", 2) == 0) {		// ast
				//~ level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = gen_code | out_tree;
			}
			else if (strncmp(arg, "-C", 2) == 0) {		// ast / no cgen
				//~ level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = out_tree;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// asm
				//~ level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				outc = gen_code | out_dasm;
			}

			else if (strncmp(arg, "-x", 2) == 0) {		// exec(&| debug)
				char *str = arg + 2;
				outc = gen_code | run_code;

				if (*str == 'd' || *str == 'D') {
					dbg = dbgCon;
					printvars = *str == 'D';
					str += 1;
				}

				level = 1;

				if (*str && *parsei32(str, &level, 16)) {
					fputfmt(stderr, "invalid level '%s'\n", str);
					debug("invalid level '%s'", str);
					return 0;
				}
			}

			// Override settings
			else if (strncmp(arg, "-i", 2) == 0) {}		// import library
			else if (strncmp(arg, "-w", 2) == 0) {		// warning level
				if (strcmp(arg, "-wx") == 0)
					warn = -1;
				else if (strcmp(arg, "-wa") == 0)
					warn = 9;
				else if (*parsei32(arg + 2, &warn, 10)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-O", 2) == 0) {		// optimize level
				if (strcmp(arg, "-O") == 0)
					opti = 1;
				else if (strcmp(arg, "-Oa") == 0)
					opti = 3;
				else if (!parsei32(arg + 2, &opti, 10)) {
					fputfmt(stderr, "invalid level '%s'\n", arg + 2);
					debug("invalid level '%s'\n", arg + 2);
					return 0;
				}
			}

			else {
				fputfmt(stderr, "invalid option '%s' for -compile\n", arg);
				return -1;
			}
		}

		// open logger
		if (logf && logfile(s, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", logf);
			return -1;
		}

		// initialize compiler: type sysyem, emit, ...
		if (!ccInit(s, creg_def, libCallExitDebug)) {
			error(s, NULL, 0, "error registering types");
			logfile(s, NULL);
			return -6;
		}

		// intstall standard library and others.
		if (reglibs(s, stdl) != 0) {
			error(s, NULL, 0, "error registering lib calls");
			logfile(s, NULL);
			return -6;
		}// */

		// iomports
		for (argi = 2; argi < argc; ++argi) {
			char *arg = argv[argi];
			if (strncmp(arg, "-i", 2) == 0) {		// import library
				char *str = arg + 2;
				importLib(s, str, "apiMain");
			}
		}

		// compile the given source code.
		if (compile(s, warn, srcf) != 0) {
			error(s, NULL, 0, "error compiling `%s`", srcf);
			logfile(s, NULL);
			return s->errc;
		}

		// generate variables and vm code.
		if ((outc & gen_code) && gencode(s, opti) != 0) {
			logfile(s, NULL);
			return s->errc;
		}

		// dump to log or execute
		if (outf)
			logfile(s, outf);
		else
			logFILE(s, stdout);

		if (outc) switch (outc) {

			case out_tags + gen_code:
			case out_tags: dump(s, dump_sym | (level & 0x0ff), NULL, NULL); break;

			case out_tree + gen_code:
			case out_tree: dump(s, dump_ast | (level & 0x0ff), NULL, NULL); break;

			case out_dasm: dump(s, dump_asm | (level & 0xfff), NULL, NULL); break;

			case run_code: vmExec(s, dbg); break;

			default: fatal("FixMe");
		}

		logfile(s, NULL);
		return 0;
	}
	else if (strcmp(cmd, "-h") == 0) {		// help
		usage(prg, argc >= 2 ? argv[2] : NULL);
		//~ else if (strcmp(t, "compile") == 0) {}
	}
	else fputfmt(stderr, "invalid option: '%s'\n", cmd);

	return 0;
}

extern int vmTest();

int main(int argc, char *argv[]) {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	//~ return vmTest();
	return program(argc, argv);
}

static int dbgCon(state s, int pu, void *ip, long* bp, int ss) {
	static char buff[1024];
	static char cmd = 'N';
	int IP, SP;
	char *arg;

	if (ip == NULL) {
		return 0;
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	IP = ((char*)ip) - ((char*)s->_mem);
	SP = ((char*)s->_ptr) - ((char*)bp);

	//~ fputfmt(stdout, ">exec:[pu%02d][sp%02d]@%9.*A\n", pu, ss, IP, ip);
	if (printvars) {
		typedef struct {double x, y;} vec2d;
		typedef struct {float x, y, z, w;} vec4f;
		stkval* sp = (stkval*)((char*)bp);
		vec4f* v4 = (vec4f*)sp;
		vec2d* v2 = (vec2d*)sp;
		fputfmt(stdout, "\tsp(%d): {i32(%d), i64(%D), f32(%g), f64(%G), vec4f(%g, %g, %g, %g), vec2d(%G, %G)}\n", ss, sp->i4, sp->f4, sp->i8, sp->f8, v4->x, v4->y, v4->z, v4->w, v2->x, v2->y);
	}
	//~ fputfmt(stdout, ">exec:[sp%02d]@%9.*A\n", ss, IP, ip);
	fputfmt(stdout, ">exec:[sp%02d:%008x]@%9.*A\n", ss, bp + ss, IP, ip);

	if (cmd != 'N') for ( ; ; ) {
		if (fgets(buff, 1024, stdin) == NULL) {
			// Can not read from stdin
			// continue
			cmd = 'N';
			return 0;
		}

		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0);		// no command, use last
		else if ((arg = parsecmd(buff, "print", " "))) {
			cmd = 'p';
		}
		else if ((arg = parsecmd(buff, "step", " "))) {

			if (!*arg || strcmp(arg, "over") == 0) {
				cmd = 'n';
			}

			else if (strcmp(arg, "in") == 0) {
				cmd = 'n';
			}

			else if (strcmp(arg, "out") == 0) {
				cmd = 'n';
			}

		}
		else if ((arg = parsecmd(buff, "sp", " "))) {
			cmd = 's';
		}
		else if (buff[1] == 0) {
			arg = buff + 1;
			cmd = buff[0];
		}
		else {
			debug("invalid command %s", buff);
			arg = buff + 1;
			cmd = buff[0];
			buff[1] = 0;
			continue;
		}
		if (!arg) {
			debug("invalid command %s", buff);
			arg = NULL;
			//cmd = 0;
			continue;
		}

		switch (cmd) {
			default:
				debug("invalid command '%c'", cmd);
			case 0 :
				break;

			case 'r' :		// resume
			//~ case 'c' :		// step in
			//~ case 'C' :		// step out
			case 'n' :		// step over
				return 0;
			case 'p' : if (s->cc) {		// print
				if (!*arg) {
					// vmTags(s, (void*)sptr, slen, 0);
				}
				else {
					symn sym = findsym(s->cc, NULL, arg);
					debug("arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && sym->offs >= 0) {
						stkval* sp = (stkval*)((char*)bp + ss + sym->offs);
						void vm_fputval(state s, FILE *fout, symn var, stkval* ref, int flgs);
						vm_fputval(s, stdout, sym, sp, 0);
					}
				}
			} break;
			case 's' : {
				int i;
				stkval* sp = (stkval*)bp;
				if (*arg == 0) for (i = 0; i < ss; i++) {
					fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp[i].i4, sp[i].f4, sp[i].i8, sp[i].f8);
				}
				else if (*parsei32(arg, &i, 10) == '\0') {
					if (i < ss)
						fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, sp[i].i4, sp[i].f4, sp[i].i8, sp[i].f8);
					//~ fputfmt(stdout, "\tsp: {i32(%d), i64(%D), f32(%g), f64(%G), p4f(%g, %g, %g, %g), p2d(%G, %G)}\n", sp->i4, sp->i8, sp->f4, sp->f8, sp->pf.x, sp->pf.y, sp->pf.z, sp->pf.w, sp->pd.x, sp->pd.y);
				}
				else if (strcmp(arg, "i32") == 0)
					fputfmt(stdout, "\tsp: i32(%d)\n", sp->i4);
				else if (strcmp(arg, "f32") == 0)
					fputfmt(stdout, "\tsp: f32(%d)\n", sp->i8);
				else if (strcmp(arg, "i64") == 0)
					fputfmt(stdout, "\tsp: i64(%d)\n", sp->f4);
				else if (strcmp(arg, "f64") == 0)
					fputfmt(stdout, "\tsp: f64(%d)\n", sp->f8);
			} break;
		}
	}
	return 0;
}


/* temp

int vmHelp(char *cmd) {
	FILE *out = stdout;
	int i, k = 0, n = 0;
	for (i = 0; i < opc_last; ++i) {
		char *opc = (char*)opc_tbl[i].name;
		if (opc && matchstr(opc, cmd, 1)) {
			fputfmt(out, "Instruction: %s\n", opc);
			n += 1;
			k = i;
		}
	}
	if (n == 1 && strcmp(cmd, opc_tbl[k].name) == 0) {
		fputfmt(out, "Opcode: 0x%02x\n", opc_tbl[k].code);
		fputfmt(out, "Length: %d\n", opc_tbl[k].size);
		fputfmt(out, "Stack check: %d\n", opc_tbl[k].chck);
		fputfmt(out, "Stack diff: %+d\n", opc_tbl[k].diff);
		//~ fputfmt(out, "0x%02x	%d		\n", opc_tbl[k].code, opc_tbl[k].size-1);
		//~ fputfmt(out, "\nDescription\n");
		//~ fputfmt(out, "The '%s' instruction %s\n", opc_tbl[k].name, "#");
		//~ fputfmt(out, "\nOperation\n");
		//~ fputfmt(out, "#\n");
		//~ fputfmt(out, "\nExceptions\n");
		//~ fputfmt(out, "None#\n");
		//~ fputfmt(out, "\n");		// end of text
	}
	else if (n == 0) {
		fputfmt(out, "No Entry for: '%s'\n", cmd);
	}
	return n;
}
// */
