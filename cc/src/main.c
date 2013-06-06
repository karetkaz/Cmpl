/* command line options
application <global options> <local options>*

	<global options>:
		-O<int>					optimize

		-l <file>				logger
		-o <file>				output

		-x[v][d<hex>]			execute
			v:					print global variables
			d<hex>:				debug using level <hex>

		-api[<hex>][.<sym>]		output symbols
		-asm[<hex>][.<fun>]		output assembly
		-ast[<hex>]				output syntax tree

		--[s|c]*				skip: stdlib | code <eration

	<local options>:
		-w[a|x|<hex>]			set warning level
		-L<file>				import plugin (.so|.dll)
		-C<file>				compile source
		<file>					if file extension is (.so|.dll) import else compile
*/

//~ (wcl386 -cc -q -ei -6s -d2  -fe=../main *.c) & (rm -f *.obj)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "core.h"

// default values
static const int wl = 9;			// warning level
static const int ol = 2;			// optimize level
static int dl = 0x19;				// disassambly level

//~ static const int cc = 1;			// execution cores
#define memsize (4 << 20)			// runtime size(4M)
static const int ss = memsize / 4;	// stack size
static char mem[memsize];

char* STDLIB = "../stdlib.cvx";		// standard library

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

char* parsei32(const char* str, int32_t* res, int radix) {
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

	*res = (int32_t)(sign * result);

	return (char*)str;
}
char* matchstr(const char* t, const char* p, int ic) {
	int i = 0;//, ic = flgs & 1;

	for ( ; *t && p[i]; ++t, ++i) {
		if (p[i] == '*') {
			if (matchstr(t, p + i + 1, ic))
				return (char*)(t - i);
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

	return (char*)(p[i] ? 0 : t - i);
}
char* parsecmd(char* ptr, char* cmd, char* sws) {
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

void usage(char* prog) {
	FILE *out = stdout;
	fputfmt(out, "Usage: %s =<eval expression>\n", prog);
	fputfmt(out, "Usage: %s <global options> <local options>*\n", prog);

	fputfmt(out, "	<global options>:\n");
	fputfmt(out, "		-O<int>		optimize\n");
	fputfmt(out,"\n");
	fputfmt(out, "		-l <file>	logger\n");
	fputfmt(out, "		-o <file>	output\n");
	fputfmt(out,"\n");
	fputfmt(out, "		-x[v][d<hex>]	execute\n");
	fputfmt(out, "			v: print global variables\n");
	fputfmt(out, "			d<hex>: debug using level <hex>\n");
	fputfmt(out, "		--[s|c]*		skip stdlib / code generation\n");
	fputfmt(out,"\n");
	fputfmt(out,"		-api[<hex>][.<sym>]		output symbols\n");
	fputfmt(out,"		-asm[<hex>][.<fun>]		output assembly\n");
	fputfmt(out,"		-ast[<hex>]	output syntax tree\n");
	fputfmt(out,"\n");
	fputfmt(out,"	<local options>:\n");
	fputfmt(out,"		-w[a|x|<hex>]		set warning level\n");
	fputfmt(out,"		-L<file>	import plugin (.so|.dll)\n");
	fputfmt(out,"		-C<file>	compile source\n");
	fputfmt(out,"		<file>		if file extension is (.so|.dll) import else compile\n");
}

int evalexp(ccState cc, char* text) {
	struct astn res;
	astn ast;
	symn typ;
	int tid;

	ccOpen(cc->s, NULL, 0, text);

	ast = expr(cc, TYPE_any);
	typ = typecheck(cc, NULL, ast);
	tid = eval(&res, ast);

	if (peek(cc))
		error(cc->s, cc->file, cc->line, "unexpected: `%k`", peek(cc));

	fputfmt(cc->s->logf, "expr: %+K", ast);
	fputfmt(cc->s->logf, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(cc->s->logf, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(cc->s->logf, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);

	return -1;
}

int reglibs(state rt, char* stdlib) {
	int err = 0;

	err = err || install_stdc(rt, stdlib, 0);
	err = err || install_file(rt);
	//~ err = err || install_bits(s);

	return err;
}

#if defined(USEPLUGINS)

typedef struct pluginLib* pluginLib;
static const char* pluginLibInstall = "ccvmInit";
static const char* pluginLibDestroy = "ccvmDone";

static int installDll(state rt, int ccApiMain(state rt)) {
	rt->api.ccBegin = ccBegin;
	rt->api.ccDefInt = ccDefInt;
	rt->api.ccDefFlt = ccDefFlt;
	rt->api.ccDefStr = ccDefStr;
	rt->api.ccAddType = ccAddType;
	rt->api.ccAddCall = ccAddCall;
	rt->api.ccAddCode = ccAddCode;
	rt->api.ccEnd = ccEnd;

	//~ rt->api.ccSymFind = ccSymFind;

	rt->api.rtAlloc = rtAlloc;

	rt->api.invoke = vmCall;
	rt->api.symfind = symfind;
	return ccApiMain(rt);
}

#if (defined(WIN32) || defined(_WIN32))

#include <windows.h>
static struct pluginLib {
	void (*onClose)();
	pluginLib next;			// next plugin
	HANDLE lib;				// 
} *pluginLibs = NULL;
static void closeLibs() {
	while (pluginLibs != NULL) {
		if (pluginLibs->onClose) {
			pluginLibs->onClose();
		}
		if (pluginLibs->lib) {
			FreeLibrary((HINSTANCE)pluginLibs->lib);
		}
		free(pluginLibs);
		pluginLibs = pluginLibs->next;
	}
}
static int importLib(state rt, const char* path) {
	int result = -1;
	HANDLE lib = LoadLibraryA(path);
	if (lib != NULL) {
		int (*install)(state api) = (void*)GetProcAddress(lib, pluginLibInstall);
		void (*destroy)() = (void*)GetProcAddress(lib, pluginLibDestroy);
		if (install != NULL) {
			pluginLib lib = malloc(sizeof(struct pluginLib));
			result = installDll(rt, install);
			lib->onClose = destroy;
			lib->next = pluginLibs;
			pluginLibs = lib;
		}
		else {
			result = -2;
		}
	}
	return result;
}

#elif (defined(__linux__))

#include <dlfcn.h>
static struct pluginLib {
	void (*onClose)();
	pluginLib next;			// next plugin
	void* lib;				// 
} *pluginLibs = NULL;
static void closeLibs() {
	while (pluginLibs != NULL) {
		if (pluginLibs->onClose) {
			pluginLibs->onClose();
		}
		if (pluginLibs->lib) {
			dlclose(pluginLibs->lib);
		}
		free(pluginLibs);
		pluginLibs = pluginLibs->next;
	}
}
static int importLib(state rt, const char* path) {
	int result = -1;
	void* lib = dlopen(path, RTLD_NOW);
	if (lib != NULL) {
		void* install = dlsym(lib, pluginLibInstall);
		void* destroy = dlsym(lib, pluginLibDestroy);
		if (install != NULL) {
			pluginLib lib = malloc(sizeof(struct pluginLib));
			result = installDll(rt, install);
			lib->onClose = destroy;
			lib->next = pluginLibs;
			pluginLibs = lib;
		}
		else {
			result = -2;
		}
	}
	return result;
}
#endif

#else

static void closeLibs() {}
static int importLib(state rt, const char* path) {
	(void)rt;
	(void)path;
	error(rt, path, 1, "dynamic linking is not available in this build.");
	return -1;
}

#endif

static symn printvars = NULL;
static int dbgCon(state, int pu, void* ip, long* bp, int ss);
static int libCallHaltDebug(state rt, void* _) {
	symn arg = rt->libc.libc->args;
	int argc = (char*)rt->libc.retv - (char*)rt->libc.argv;

	for ( ; arg; arg = arg->next) {
		char* ofs;

		if (arg->call)
			continue;

		if (arg->kind != TYPE_ref)
			continue;

		if (arg->file && arg->line)
			fputfmt(stdout, "%s:%d:", arg->file, arg->line);
		else
			fputfmt(stdout, "var: ");

		fputfmt(stdout, "@0x%06x[size: %d]: ", arg->offs, arg->size);

		if (arg->stat) {
			// static variable.
			ofs = (void*)(rt->_mem + arg->offs);
		}
		else {
			// argument or local variable.
			ofs = ((char*)rt->libc.argv) + argc - arg->offs;
		}

		vm_fputval(rt, stdout, arg, (stkval*)ofs, 0);
		fputc('\n', stdout);
	}

	rtAlloc(rt, NULL, 0);

	return 0;
	(void)_;
}

int program(int argc, char* argv[]) {
	state rt = rtInit(mem, sizeof(mem));

	char* stdl = STDLIB;

	char* prg = argv[0];
	dbgf dbg = NULL;

	if (argc < 2) {
		usage(prg);
	}
	else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		usage(prg);
	}
	else if (argc == 2 && *argv[1] == '=') {
		return evalexp(ccInit(rt, creg_def, NULL), argv[1] + 1);
	}
	else {		// compile
		int level = -1, argi;
		int opti = ol;

		int gen_code = 1;	// cgen: true/false
		int run_code = 0;	// exec: true/false
		char* stk_dump = NULL;

		int out_tree = -1;	// walk: level
		char* str_tree = NULL;
		int out_tags = -1;	// tags: level
		char* str_tags = NULL;
		int out_dasm = -1;	// dasm: level
		char* str_dasm = NULL;

		char* logf = 0;			// logger filename
		char* outf = 0;			// output filename

		int warn = wl;

		int (*onHalt)(state, void*) = NULL;	// print variables and values on exit

		// global options
		for (argi = 1; argi < argc; ++argi) {
			char* arg = argv[argi];

			// optimize code
			if (strncmp(arg, "-O", 2) == 0) {			// optimize level
				if (strcmp(arg, "-O") == 0)
					opti = 1;
				else if (strcmp(arg, "-Oa") == 0)
					opti = 3;
				else if (!parsei32(arg + 2, &opti, 10)) {
					error(rt, NULL, 0, "invalid level '%s'", arg + 2);
					return 0;
				}
			}

			// execute code
			else if (strncmp(arg, "-x", 2) == 0) {		// exec(&| debug)
				char* str = arg + 2;

				if (*str == 'v') {
					onHalt = libCallHaltDebug;
					str += 1;
				}
				if (*str == 'd') {
					dbg = dbgCon;
					if (str[1] == ':') {
						stk_dump = str + 2;
						str += 1;
					}
				}
				run_code = 1;
			}

			// output file
			else if (strcmp(arg, "-l") == 0) {			// log
				if (++argi >= argc || logf) {
					error(rt, NULL, 0, "log file not specified");
					return -1;
				}
				logf = argv[argi];
			}
			else if (strcmp(arg, "-o") == 0) {			// out
				if (++argi >= argc || outf) {
					error(rt, NULL, 0, "output file not specified");
					return -1;
				}
				outf = argv[argi];
			}

			// output what
			else if (strncmp(arg, "-api", 4) == 0) {	// tags
				level = 2;
				if (arg[4]) {
					char* ptr = parsei32(arg + 4, &level, 16);
					if (*ptr == '.') {
						str_tags = ptr + 1;
					}
					else if (*ptr) {
						error(rt, NULL, 0, "invalid argument '%s'\n", arg);
						return 0;
					}
				}
				out_tags = level;
			}
			else if (strncmp(arg, "-ast", 4) == 0) {	// tree
				level = 0;
				if (arg[4]) {
					char* ptr = parsei32(arg + 4, &level, 16);
					if (*ptr == '.') {
						str_tree = ptr + 1;
					}
					else if (*ptr) {
						error(rt, NULL, 0, "invalid argument '%s'\n", arg);
						return 0;
					}
				}
				out_tree = level;
			}
			else if (strncmp(arg, "-asm", 4) == 0) {	// dasm
				level = 0;
				if (arg[4]) {
					char* ptr = parsei32(arg + 4, &level, 16);
					if (*ptr == '.') {
						str_dasm = ptr + 1;
					}
					else if (*ptr) {
						error(rt, NULL, 0, "invalid argument '%s'\n", arg);
						return 0;
					}
				}
				out_dasm = level;
			}

			// temp
			else if (strncmp(arg, "-std", 4) == 0) {	// redefine stdlib
				stdl = arg + 4;
			}
			else if (strncmp(arg, "--", 2) == 0) {		// exclude: do not gen code
				if (strchr(arg, 'c'))
					gen_code = 0;
				if (strchr(arg, 's'))
					stdl = NULL;
			}

			else break;
		}

		// open logger
		if (logf && logfile(rt, logf) != 0) {
			error(rt, NULL, 0, "can not open file `%s`\n", logf);
			return -1;
		}

		// intstall basic type system.
		if (!ccInit(rt, creg_def, onHalt)) {
			error(rt, NULL, 0, "error registering types");
			logfile(rt, NULL);
			return -6;
		}

		// intstall standard libraries.
		if (reglibs(rt, stdl) != 0) {
			error(rt, NULL, 0, "error registering lib calls");
			logfile(rt, NULL);
			return -6;
		}

		// compile files and import
		for (; argi < argc; ++argi) {
			char* arg = argv[argi];

			if (strncmp(arg, "-w", 2) == 0) {			// warning level
				if (strcmp(arg, "-wx") == 0)
					warn = -1;
				else if (strcmp(arg, "-wa") == 0)
					warn = 32;
				else if (*parsei32(arg + 2, &warn, 10)) {
					error(rt, NULL, 0, "invalid warning level '%s'\n", arg + 2);
					return 0;
				}
			}
			else if (strncmp(arg, "-L", 2) == 0) {		// import library
				char* str = arg + 2;
				if (importLib(rt, str) != 0) {
					error(rt, NULL, 0, "error importing library `%s`", str);
				}
			}
			else if (strncmp(arg, "-C", 2) == 0) {		// compile source
				char* str = arg + 2;
				if (ccAddCode(rt, warn, str, 1, NULL) != 0) {
					error(rt, NULL, 0, "error compiling `%s`", str);
				}
			}
			else if (*arg != '-') {
				char* ext = strrchr(arg, '.');
				if (ext && strcmp(ext, ".so") == 0) {
					if (importLib(rt, arg) != 0) {
						error(rt, NULL, 0, "error importing library `%s`", arg);
					}
					continue;
				}
				if (ext && strcmp(ext, ".dll") == 0) {
					if (importLib(rt, arg) != 0) {
						error(rt, NULL, 0, "error importing library `%s`", arg);
					}
					continue;
				}
				if (ccAddCode(rt, warn, arg, 1, NULL) != 0) {
					error(rt, NULL, 0, "error compiling `%s`", arg);
				}
			}
			else {
				error(rt, NULL, 0, "invalid option: `%s`", arg);
			}
		}

		// print top of stack as a type or var.
		if (stk_dump != NULL) {
			ccState cc = ccOpen(rt, NULL, 0, stk_dump);
			if (cc != NULL) {
				astn ast = decl_var(cc, NULL, TYPE_def);
				printvars = linkOf(ast);
				if (printvars != NULL) {
					printvars->name = "";
				}
				else {
					error(rt, NULL, 0, "error in debug print format `%s`", stk_dump);
				}
			}
		}

		if (rt->errc == 0) {

			// generate variables and vm code.
			if ((gen_code || run_code) && gencode(rt, opti) != 0) {
				logfile(rt, NULL);
				closeLibs();
				return rt->errc;
			}

			// dump to log or execute
			if (outf == NULL) {
				logFILE(rt, stdout);
			}
			else {
				logfile(rt, outf);
			}

			if (out_tags >= 0) {
				symn sym = NULL;
				if (str_tags != NULL) {
					sym = ccFindSym(rt->cc, NULL, str_tags);
					if (sym == NULL) {
						info(rt, NULL, 0, "symbol not found: %s", str_tags);
					}
				}
				dump(rt, dump_sym | (out_tags & 0x0ff), sym, "\ntags:\n");
			}
			if (out_tree >= 0) {
				symn sym = NULL;
				if (str_tree != NULL) {
					sym = ccFindSym(rt->cc, NULL, str_tree);
					if (sym == NULL) {
						info(rt, NULL, 0, "symbol not found: %s", str_tree);
					}
				}
				dump(rt, dump_ast | (out_tree & 0x0ff), sym, "\ncode:\n");
			}
			if (out_dasm >= 0) {
				symn sym = NULL;
				if (str_dasm != NULL) {
					sym = ccFindSym(rt->cc, NULL, str_dasm);
					if (sym == NULL) {
						info(rt, NULL, 0, "symbol not found: %s", str_dasm);
					}
				}
				dl = out_dasm & 0x0ff;
				dump(rt, dump_asm | dl, NULL, "\ndasm:\n");
			}
			if (run_code != 0) {
				logFILE(rt, stdout);
				vmExec(rt, dbg, ss);
			}
		}

		// close log file
		logfile(rt, NULL);
		closeLibs();
		return 0;
	}
	return 0;
}

extern int vmTest();
extern int vmHelp();

int main(int argc, char* argv[]) {
	//setbuf(stdout, NULL);
	//setbuf(stderr, NULL);

	//~ return vmTest();
	//~ return vmHelp();
	return program(argc, argv);
}

static int dbgCon(state rt, int pu, void* ip, long* bp, int ss) {
	static char buff[1024];
	static char cmd = 'N';
	int IP;//, SP;
	char* arg;

	if (ip == NULL) {
		return 0;
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	IP = ((char*)ip) - ((char*)rt->_mem);
	//~ SP = ((char*)rt->_ptr) - ((char*)bp);

	fputfmt(stdout, ">exec:[sp(%02d)", ss);
	if (printvars != NULL) {
		stkval* sp = (stkval*)((char*)bp);
		vm_fputval(rt, stdout, printvars, sp, 0);
	}
	//~ fputfmt(stdout, "]@%9.*A\n", IP, ip);
	fputfmt(stdout, "]");
	fputopc(stdout, ip, dl & 0xf, IP, rt);fputfmt(stdout, "\n");
	//~ fputasm(stdout, rt, IP, IP + 1, dl);

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
			if (*arg == 0) {	// step = step over
				cmd = 'n';
			}
			else if (strcmp(arg, "over") == 0) {
				cmd = 'n';
			}
			else if (strcmp(arg, "out") == 0) {
				cmd = 'n';
			}
			else if (strcmp(arg, "in") == 0) {
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
					symn sym = ccFindSym(rt->cc, NULL, arg);
					debug("arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && !sym->stat) {
						stkval* sp = (stkval*)((char*)bp + ss + sym->offs);
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
	return 0;
	(void)pu;
}
