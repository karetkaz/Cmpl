//~ wcl386 -zq -ei -6s -d2  -fe=../main *.c
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
char *STDLIB = "../stdlib.cvx";		// standard library

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

static int cctext(state rt, char *file, int line, int wl, char *buff) {
	if (!ccOpen(rt, srcText, buff))
		return -2;

	ccSource(rt->cc, file, line);
	return parse(rt->cc, 0, wl);
}

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

int evalexp(ccState cc, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	source(cc, 0, text);

	ast = expr(cc, 0);
	typ = typecheck(cc, 0, ast);
	tid = eval(&res, ast);

	if (peek(cc))
		error(cc->s, cc->file, cc->line, "unexpected: `%k`", peek(cc));

	fputfmt(stdout, "expr: %+K", ast);
	fputfmt(stdout, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(stdout, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(stdout, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);

	return -1;
}

//{ int64 ext
static int b64shl(state s) {
	uint64_t x = popi64(s);
	int32_t y = popi32(s);
	setret(s, uint64_t, x << y);
	return 0;
}
static int b64shr(state s) {
	uint64_t x = popi64(s);
	int32_t y = popi32(s);
	setret(s, uint64_t, x >> y);
	return 0;
}
static int b64sar(state s) {
	int64_t x = popi64(s);
	int32_t y = popi32(s);
	setret(s, uint64_t, x >> y);
	return 0;
}
/* unused
static int b64and(state s) {
	uint64_t x = popi64(s);
	uint64_t y = popi64(s);
	setret(s, uint64_t, x & y);
	return 0;
}
static int b64ior(state s) {
	uint64_t x = popi64(s);
	uint64_t y = popi64(s);
	setret(s, uint64_t, x | y);
	return 0;
}
static int b64xor(state s) {
	uint64_t x = popi64(s);
	uint64_t y = popi64(s);
	setret(s, uint64_t, x ^ y);
	return 0;
}

static int b64bsf(state s) {
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
	return 0;
}
static int b64bsr(state s) {
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
	return 0;
}
static int b64hib(state s) {
	uint64_t x = popi64(s);
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	setret(s, uint64_t, x - (x >> 1));
	return 0;
}
static int b64lob(state s) {
	uint64_t x = popi64(s);
	setret(s, uint64_t, x & -x);
	return 0;
}*/
//}

int reglibs(state rt, char *stdlib) {
	int err = 0;

	err = err || install_stdc(rt, stdlib, wl);
	//~ enter(rt->cc, NULL);
		libcall(rt, b64shl, 0, "int64 Shl(int64 Value, int Count);");
		libcall(rt, b64shr, 0, "int64 Shr(int64 Value, int Count);");
		libcall(rt, b64sar, 0, "int64 Sar(int64 Value, int Count);");
	//~ extend(type_i64, leave(rt->cc, type_i64, 1));
	//~ err = err || install_bits(s);

	return err;
}

int compile(state rt, int wl, char *file) {
	int result;
	#if DEBUGGING > 1
	int size = 0;
	#endif

	if (!ccOpen(rt, srcFile, file))
		return -1;

	#if DEBUGGING > 1
	//~ debug("after init:%d Bytes", s->cc->_beg - s->_mem);
	size = (rt->cc->_beg - (char*)rt->_mem);
	#endif

	result = parse(rt->cc, 0, wl);

	#if DEBUGGING > 1
	//~ debug("%s: init(%.2F); scan(%.2F) KBytes", file, size / 1024., (s->cc->_beg - s->_mem) / 1024.);
	(void)size;
	#endif

	return result;
}

int installDll(state rt, int ccApiMain(stateApi api)) {
	static struct stateApi api;

	api.rt = rt;
	api.ccBegin = ccBegin;
	api.ccEnd = ccEnd;
	api.libcall = libcall;
	//~ api.install = installtyp;
	api.ccDefineInt = ccDefineInt;
	api.ccAddText = cctext;
	api.rtAlloc = rtAlloc;
	api.invoke = vmCall;
	//~ api.findsym = findsym;
	api.findref = findref;
	return ccApiMain(&api);

}

#if defined(__linux__)
#include <dlfcn.h>
static int importLib(state rt, const char *path, const char *init) {
	int result = 0;
	void *lib = dlopen(path, RTLD_NOW);
	if (lib != NULL) {
		void *sym = dlsym(lib, init);
		if (sym != NULL) {
			result = installDll(rt, sym);
		}
		else {
			result = -2;
		}
		//~ dlclose(lib);
	}
	else {
		result = -1;
	}

	fprintf(stdout, "imported: %s.%s(): %d `%s`\n", path, init, result, dlerror());
	fflush(stdout);

	return result;
}
#else
#include <windows.h>
static int importLib(state rt, const char *path, const char *init) {
	int result = 0;
	typedef int (*ccApiMain)(stateApi api);
	HANDLE lib = LoadLibrary(path);
	if (lib != NULL) {
		ccApiMain sym = (ccApiMain)GetProcAddress(lib, init);
		if (sym != NULL) {
			result = installDll(rt, sym);
		}
		else {
			result = -2;
		}
		//~ FreeLibrary((HINSTANCE)lib);
	}
	else {
		result = -1;
	}
	fprintf(stdout, "imported: %s.%s(): %d\n", path, init, result);
	fflush(stdout);
	return result;
}

#endif

static int printvars = 0;
static int dbgCon(state, int pu, void *ip, long* bp, int ss);
void vm_fputval(state, FILE *fout, symn var, stkval* ref, int flgs);
static int libCallExitDebug(state rt) {
	symn arg = rt->libc->args;
	int argc = (char*)rt->retv - (char*)rt->argv;

	//~ for (arg = rt->gdef ;arg; arg = arg->gdef) {
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
		if (arg->call)
			continue;

		if (arg->kind != TYPE_ref)
			continue;

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

	rtAlloc(rt, NULL, 0);

	return 0;
}

int program(int argc, char *argv[]) {
	state rt = rtInit(mem, sizeof(mem));

	char *stdl = STDLIB;

	char *prg, *cmd;
	dbgf dbg = NULL;

	prg = argv[0];
	cmd = argv[1];
	if (argc < 2) {
		usage(prg, NULL);
	}
	else if (argc == 2 && *cmd == '=') {	// eval
		return evalexp(ccInit(rt, creg_def, NULL), cmd + 1);
	}
	else if (strcmp(cmd, "-api") == 0) {
		ccState env = ccInit(rt, creg_def, NULL);
		const int level = 2;
		symn glob;
		int i;

		if (!env)
			return -33;

		if (reglibs(rt, stdl) != 0) {
			error(rt, NULL, 0, "error registering lib calls");
			logfile(rt, NULL);
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
		return ccDone(rt);
	}
	else if (stricmp(cmd, "-c") == 0) {		// compile
		int level = -1, argi;
		int warn = wl;
		int opti = ol;
		//~ int outc = 0;			// output

		int gen_code = 1;	// cgen: true/false
		int run_code = 0;	// exec: true/false
		int out_tags = -1;	// tags: level
		int out_tree = -1;	// walk: level
		int out_dasm = -1;	// dasm: level

		char *srcf = 0;			// source
		char *logf = 0;			// logger
		char *outf = 0;			// output
		int (*onExit)(state) = NULL;	// print variables and values on exit

		/*enum {
			gen_code = 0x0010,
			out_tags = 0x0001,	// tags
			out_tree = 0x0002,	// walk
			out_dasm = 0x0013,	// dasm
			run_code = 0x0014,	// exec
		};*/

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
				//~ outc = gen_code | out_tags;
				out_tags = level;
			}
			else if (strncmp(arg, "-T", 2) == 0) {		// tags / no cgen
				level = 2;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				//~ outc = out_tags;
				out_tags = level;
				gen_code = 0;
			}
			else if (strncmp(arg, "-c", 2) == 0) {		// ast
				level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				//~ outc = gen_code | out_tree;
				out_tree = level;
			}
			else if (strncmp(arg, "-C", 2) == 0) {		// ast / no cgen
				//~ level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				//~ outc = out_tree;
				out_tree = level;
				gen_code = 0;
			}
			else if (strncmp(arg, "-s", 2) == 0) {		// asm
				level = 0;
				if (arg[2] && *parsei32(arg + 2, &level, 16)) {
					fputfmt(stderr, "invalid argument '%s'\n", arg);
					return 0;
				}
				//~ outc = gen_code | out_dasm;
				out_dasm = level;
			}

			else if (strncmp(arg, "-x", 2) == 0) {		// exec(&| debug)
				char *str = arg + 2;

				if (*str == 'v') {
					onExit = libCallExitDebug;
					str += 1;
				}
				if (*str == 'd' || *str == 'D') {
					printvars = *str == 'D';
					dbg = dbgCon;
					str += 1;
				}

				level = 1;
				if (*str && *parsei32(str, &level, 16)) {
					fputfmt(stderr, "invalid level '%s'\n", str);
					debug("invalid level '%s'", str);
					return 0;
				}
				//~ outc = gen_code | run_code;
				run_code = 1;
			}

			// Override settings
			else if (strncmp(arg, "-i", 2) == 0) {}		// import library
			else if (strncmp(arg, "-w", 2) == 0) {		// warning level
				if (strcmp(arg, "-wx") == 0)
					warn = -1;
				else if (strcmp(arg, "-wa") == 0)
					warn = 32;
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
		if (logf && logfile(rt, logf) != 0) {
			fputfmt(stderr, "can not open file `%s`\n", logf);
			return -1;
		}

		// initialize compiler: type sysyem, emit, ...
		if (!ccInit(rt, creg_def, onExit)) {
			error(rt, NULL, 0, "error registering types");
			logfile(rt, NULL);
			return -6;
		}

		// intstall standard library and others.
		if (reglibs(rt, stdl) != 0) {
			error(rt, NULL, 0, "error registering lib calls");
			logfile(rt, NULL);
			return -6;
		}// */

		// iomports
		for (argi = 2; argi < argc; ++argi) {
			char *arg = argv[argi];
			if (strncmp(arg, "-i", 2) == 0) {		// import library
				char *str = arg + 2;
				importLib(rt, str, "apiMain");
			}
		}

		// compile the given source code.
		if (compile(rt, warn, srcf) != 0) {
			error(rt, NULL, 0, "error compiling `%s`", srcf);
			logfile(rt, NULL);
			return rt->errc;
		}

		// generate variables and vm code.
		if ((gen_code || run_code) && gencode(rt, opti) != 0) {
			logfile(rt, NULL);
			return rt->errc;
		}

		// dump to log or execute
		if (outf)
			logfile(rt, outf);
		else
			logFILE(rt, stdout);

		if (out_tags >= 0) {
			dump(rt, dump_sym | (out_tags & 0x0ff), NULL, "\ntags:\n");
		}
		if (out_tree >= 0) {
			dump(rt, dump_ast | (out_tree & 0x0ff), NULL, "\ncode:\n");
		}
		if (out_dasm >= 0) {
			dump(rt, dump_asm | (out_dasm & 0x0ff), NULL, "\ndasm:\n");
		}
		if (run_code) {
			vmExec(rt, dbg);
		}

		// close log file
		logfile(rt, NULL);
		return 0;
	}
	else if (strcmp(cmd, "-h") == 0) {		// help
		usage(prg, argc >= 2 ? argv[2] : NULL);
		//~ else if (strcmp(t, "compile") == 0) {}
	}
	else fputfmt(stderr, "invalid option: '%s'\n", cmd);

	return 0;
}

//extern int vmTest();

int main(int argc, char *argv[]) {
	int result = 0;
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	//~ return vmTest();
	result = program(argc, argv);
	return result;
}

static int dbgCon(state rt, int pu, void *ip, long* bp, int ss) {
	static char buff[1024];
	static char cmd = 'N';
	int IP;//, SP;
	char *arg;

	if (ip == NULL) {
		return 0;
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	IP = ((char*)ip) - ((char*)rt->_mem);
	//~ SP = ((char*)rt->_ptr) - ((char*)bp);

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
	//~ fputfmt(stdout, ">exec:[sp%02d:%008x]@%9.*A\n", ss, bp + ss, IP, ip);
	fputfmt(stdout, ">exec:[sp%02d:%008x]@", ss, bp + ss, IP, ip);
	fputopc(stdout, ip, 0x09, IP, rt);
	fputfmt(stdout, "\n");

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

		if (*buff == 0) {}		// no command, use last
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
			debug("invalid command %rt", buff);
			arg = buff + 1;
			cmd = buff[0];
			buff[1] = 0;
			continue;
		}
		if (!arg) {
			debug("invalid command %rt", buff);
			arg = NULL;
			//cmd = 0;
			continue;
		}

		switch (cmd) {
			default:
				debug("invalid command '%c'", cmd);
				break;
			case 0 :
				break;

			case 'r' :		// resume
			//~ case 'c' :		// step in
			//~ case 'C' :		// step out
			case 'n' :		// step over
				return 0;
			case 'p' : if (rt->cc) {		// print
				if (!*arg) {
					// vmTags(rt, (void*)sptr, slen, 0);
				}
				else {
					symn sym = findsym(rt->cc, NULL, arg);
					debug("arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && sym->offs >= 0) {
						stkval* sp = (stkval*)((char*)bp + ss + sym->offs);
						void vm_fputval(state rt, FILE *fout, symn var, stkval* ref, int flgs);
						vm_fputval(rt, stdout, sym, sp, 0);
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
	//~ (void)SP;
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
