/* command line options
application [global options] [local options]...

	<global options>:
		-g[d][-]<int>			generate
			d:					generate debug info
			-:					generate globals on stack
			int:				optimization level, max 255

		-x[v][d|D[:<type>]]		execute
			v:					print global variable values
			d|D:				debug application
			<type>				print top of stack as it would be a variable of type
				ex: -xd:int32[20]

		-l <file>				logger
		-o <file>				output

		-api[<hex>][.<sym>]		output symbols
		-asm[<hex>][.<fun>]		output assembly
		-ast[<hex>]				output syntax tree

		--[s|c]*				skip: stdlib | code geration

	<local options>:
		-w[a|x|<hex>]			set warning level
		-L<file>				import plugin (.so|.dll)
		-C<file>				compile source
		<file>					if file extension is (.so|.dll) import else compile
*/

//~ (wcl386 -cc -q -ei -6s -d2  -fe=../main *.c) && (rm -f *.o *.obj *.err)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "core.h"

// enable dynamic dll/so lib loading
#define USEPLUGINS

// default values
static const int wl = 9;			// warning level
static const int ol = 2;			// optimize level
static char mem[16 << 20];			// runtime memory of (16M)

const char* STDLIB = "stdlib.cvx";		// standard library

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

static inline int streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

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
char* parsecmd(char* ptr, const char* cmd, const char* sws) {
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

static void usage(char* prog) {
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

int evalexp(state rt, char* text) {
	struct astNode res;
	astn ast = NULL;
	symn typ = NULL;
	int tid = 0;

	ccState cc = ccInit(rt, creg_def, NULL);
	ccOpen(cc->s, NULL, 0, text);

	//ast = expr(cc, TYPE_def);
	typ = typecheck(cc, NULL, ast);
	tid = eval(&res, ast);

	ccDone(cc);

	fputfmt(cc->s->logf, "expr: %+K", ast);
	fputfmt(cc->s->logf, "eval(`%+k`) = ", ast);

	if (ast && typ && tid) {
		fputfmt(cc->s->logf, "%T(%k)\n", typ, &res);
		return 0;
	}

	fputfmt(cc->s->logf, "ERROR(typ:`%T`, tid:%d)\n", typ, tid);

	return -1;
}

static int testFunction(libcArgs rt) {		// void testFunction(void cb(int n), int n)
	symn cb = mapsym(rt->rt, argref(rt, 0));
	if (cb != NULL) {
		int n = argi32(rt, 4);
		struct {int n;} args;// = {n};
		args.n = n;
		return invoke(rt->rt, cb, NULL, &args, NULL);
	}
	return 0;
}

#if defined(USEPLUGINS)

typedef struct pluginLib* pluginLib;
static const char* pluginLibInstall = "ccvmInit";
static const char* pluginLibDestroy = "ccvmDone";

static int installDll(state rt, int ccApiMain(state rt)) {
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
#else

static void closeLibs() {}
static int importLib(state rt, const char* path) {
	(void)rt;
	(void)path;
	error(rt, path, 1, "dynamic linking is not available in this build.");
	return -1;
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
static int dbgCon(state, int pu, void* ip, long* sp, int ss);

int program(int argc, char* argv[]) {
	char* stdlib = (char*)STDLIB;

	//~ char* prg = argv[0];
	int (*dbg)(state, int pu, void *ip, long* sp, int ss) = NULL;

	// compile, run, debug, ...
	int level = -1, argi;
	int gen_code = ol;	// optimize_level, debug_info, ...
	int run_code = 0;	// true/false: exec
	char* stk_dump = NULL;

	int out_tree = -1;
	char* str_tree = NULL;

	int out_tags = -1;
	char* str_tags = NULL;

	int out_dasm = -1;
	char* str_dasm = NULL;

	char* logf = 0;			// logger filename
	char* outf = 0;			// output filename

	int warn = wl;
	int result = 0;

	int (*onHalt)(libcArgs) = NULL;	// print variables and values on exit?

	state rt = rtInit(mem, sizeof(mem));

	if (rt == NULL) {
		fatal("initializing runtime context.");
		return -1;
	}

	// evaluate constant expression.
	if (argc == 2 && *argv[1] == '=') {
		return evalexp(rt, argv[1] + 1);
	}

	// global options
	for (argi = 1; argi < argc; ++argi) {
		char* arg = argv[argi];

		// generate code
		if (strncmp(arg, "-g", 2) == 0) {			// generate code
			char* str = arg + 2;

			if (*str == 'd') {
				// generate debug info
				gen_code |= cgen_info;
				str += 1;
			}
			if (*str == '-') {
				// generate globals on stack
				gen_code |= cgen_glob;
				str += 1;
			}
			if (*str) {
				if (!parsei32(str, &level, 10)) {
					error(rt, NULL, 0, "invalid level '%s'", str);
					return 0;
				}
				gen_code |= level & cgen_opti;
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

				gen_code |= cgen_info;
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
			stdlib = arg + 4;
		}
		else if (strncmp(arg, "--", 2) == 0) {		// exclude: do not gen code
			if (strchr(arg, 'c'))
				gen_code = 0;
			if (strchr(arg, 's'))
				stdlib = NULL;
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
		error(rt, NULL, 0, "error registering base types");
		logfile(rt, NULL);
		return -6;
	}

	// intstall standard libraries.
	if (!ccAddUnit(rt, install_stdc, 0, stdlib)) {
		error(rt, NULL, 0, "error registering standard libs");
		logfile(rt, NULL);
		return -6;
	}

	// intstall file libraries.
	if (!ccAddUnit(rt, install_file, 0, NULL)) {
		error(rt, NULL, 0, "error registering file libs");
		logfile(rt, NULL);
		return -6;
	}

	// TODO: remove test function
	ccAddCall(rt, testFunction, NULL, "void testFunction(void cb(pointer args), pointer args);");

	// compile and import files / modules
	for (; argi < argc; ++argi) {
		char* arg = argv[argi];
		if (*arg != '-') {
			char* ext = strrchr(arg, '.');
			if (ext && (streq(ext, ".so") || streq(ext, ".dll"))) {
				int resultCode = importLib(rt, arg);
				if (resultCode != 0) {
					error(rt, NULL, 0, "error(%d) importing library `%s`", resultCode, arg);
				}
			}
			else if (!ccAddCode(rt, warn, arg, 1, NULL)) {
				error(rt, NULL, 0, "error compiling `%s`", arg);
			}
		}
		else if (strncmp(arg, "-w", 2) == 0) {		// warning level for 
			if (strcmp(arg, "-wx") == 0)
				warn = -1;
			else if (strcmp(arg, "-wa") == 0)
				warn = 32;
			else if (*parsei32(arg + 2, &warn, 10)) {
				error(rt, NULL, 0, "invalid warning level '%s'\n", arg + 2);
				return 1;
			}
		}
		else if (strncmp(arg, "-L", 2) == 0) {		// import library
			char* str = arg + 2;
			int resultCode = importLib(rt, str);
			if (resultCode != 0) {
				error(rt, NULL, 0, "error(%d) importing library `%s`", resultCode, arg);
			}
		}
		else if (strncmp(arg, "-C", 2) == 0) {		// compile source
			char* str = arg + 2;
			if (!ccAddCode(rt, warn, str, 1, NULL)) {
				error(rt, NULL, 0, "error compiling `%s`", str);
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
				printvars->name = "sp";	// stack pointer
			}
			else {
				error(rt, NULL, 0, "error in debug print format `%s`", stk_dump);
			}
			ccDone(cc);
		}
	}

	// if no error generate code and execute
	if (rt->errc == 0) {

		// generate variables and vm code.
		if ((gen_code || run_code) && !gencode(rt, gen_code)) {
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
			dump(rt, dump_sym | (out_tags & 0x0ff), sym, "\ntags:\n#api: replace(`^([^:]*).*$`, `\\1`)\n");
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
			dump(rt, dump_asm | (out_dasm & 0x0ff), NULL, "\ndasm:\n");
		}
		if (run_code != 0) {
			if (dbg != NULL && rt->dbg != NULL) {
				rt->dbg->dbug = dbg;
			}
			result = execute(rt, NULL, rt->_size / 4);
		}
	}

	// close log file
	logfile(rt, NULL);
	closeLibs();
	return result;
}

extern int vmTest();
extern int vmHelp();

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage(*argv);
		return 0;
	}
	if (argc == 2) {
		#if DEBUGGING > 0
		if (strcmp(argv[1], "-vmTest") == 0) {
			return vmTest();
		}
		if (strcmp(argv[1], "-vmHelp") == 0) {
			return vmHelp();
		}
		#endif
		if (strcmp(argv[1], "--help") == 0) {
			usage(*argv);
			return 0;
		}
	}
	//setbuf(stdout, NULL);
	//setbuf(stderr, NULL);
	return program(argc, argv);
}

static int dbgCon(state rt, int pu, void* ip, long* sp, int ss) {
	static char buff[1024];
	static char cmd = 'N';
	dbgInfo dbg;
	char* arg;
	int IP;

	if (ip == NULL) {
		return 0;
	}

	if (cmd == 'r') {	// && !breakpoint(vm, ip)
		return 0;
	}

	if (ss > 0 && printvars != NULL) {
		fputval(rt, stdout, printvars, (stkval*)sp, 0);
		fputfmt(stdout, "\n");
	}

	IP = ((char*)ip) - ((char*)rt->_mem);
	dbg = getCodeMapping(rt, IP);
	if (dbg != NULL) {
		fputfmt(stdout, "%s:%d:exec:[sp(%02d)] %9.*A\n", dbg->file, dbg->line, ss, IP, ip);
	}
	else {
		fputfmt(stdout, ">exec:[sp(%02d)] %9.*A\n", ss, IP, ip);
	}

	if (cmd != 'N') for ( ; ; ) {
		if (fgets(buff, sizeof(buff), stdin) == NULL) {
			// Can not read from stdin
			// continue
			cmd = 'N';
			return 0;
		}

		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0) {}		// no command, use last one
		else if ((arg = parsecmd(buff, "continue", " "))) {
			cmd = 'N';
		}
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
						fputval(rt, stdout, sym, (stkval*)sp, 0);
					}
				}
			} break;

			// trace
			case 't' : {
			} break;

			case 's' : {
				int i;
				for (i = 0; i < ss; i++) {
					stkval* v = (stkval*)&sp[i];
					fputfmt(stdout, "\tsp(%d): {i32(%d), f32(%g), i64(%D), f64(%G)}\n", i, v->i4, v->f4, v->i8, v->f8);
				}
			} break;
		}
	}
	return 0;
	(void)pu;
}
