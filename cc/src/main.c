/* command line options
application [global options] [local options]...

<global options>:
    -std<file>              specify custom standard library file.
                            empty file name disables std library compilation.

    -log <file>             set logger for: errors, warnings, runtime logging
    +out <file>             set output for: api, assembly, abstract syntax tree, coverage, call tree

    +doc[<hex>]             dump documentation
    -api[<hex>]             dump symbols
    -ast[<hex>]             dump syntax tree
    -asm[<hex>]             dump assembly
    +cov[<hex>]             dump profiling: coverage, method, call tree

    -run                    run without: debug information, stacktrace, bounds checking, ...
    -dbg[<hex>][*]          run with attached debugger, break on uncaught errors
        a                   break on caught errors
        s                   break on startup
        t                   print profiler results
        h                   print allocated heap memory
        v                   print global variable values

<local options>:
    <file>                  if file extension is (.so|.dll) load as library else compile
    -b<int>                 break point on <int> line in current file
    -d<int>                 trace point on <int> line in current file
    -w[a|x|<hex>]           set or disable warning level for current file

examples:
>app -out api.js -api
    dump builtin symbols to api.js file (types, functions, variables, aliases)

>app -run test.tracing.ccc
    dump builtin symbols to api.js file (types, functions, variables, aliases)

?
>app -dbg -lgl.so -w0 gl.ccc -w0 test.ccc -wx -b12 -b15 -d19
        execute in debug mode
        import gl.so
            with no warnings
        compile gl.ccc
            with no warnings
        compile test.ccc
            treating all warnings as errors
            break execution on lines 12 and 15
            print stacktrace when line 19 is executing
*/

//~ (wcl386 -cc -q -ei -6s -d0  -fe=../main *.c) && (rm -f *.o *.obj *.err)
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core.h"

// enable dynamic dll/so lib loading
#define USEPLUGINS

// default values
static const int wl = 15;           // default warning level
static const int ol = 2;            // default optimization level
static char mem[8 << 20];           // 8MB runtime memory
const char* STDLIB = "stdlib.cvx";  // standard library

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

static inline int streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

static char* parsei32(const char* str, int32_t* res, int radix) {
	int64_t result = 0;
	int sign = 1;

	//~ ('+' | '-')?
	switch (*str) {
		case '-':
			sign = -1;
		case '+':
			str += 1;
		default:
			break;
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
	while (*str != 0) {
		int value = *str;
		if (value >= '0' && value <= '9') {
			value -= '0';
		}
		else if (value >= 'a' && value <= 'z') {
			value -= 'a' - 10;
		}
		else if (value >= 'A' && value <= 'Z') {
			value -= 'A' - 10;
		}
		else {
			break;
		}

		if (value > radix) {
			break;
		}

		result *= radix;
		result += value;
		str += 1;
	}

	*res = (int32_t)(sign * result);

	return (char*)str;
}
static char* parsecmd(char* ptr, const char* cmd, const char* sws) {
	while (*cmd && *cmd == *ptr) {
		cmd++, ptr++;
	}

	if (*cmd != 0) {
		return 0;
	}

	if (*ptr == 0) {
		return ptr;
	}

	while (strchr(sws, *ptr)) {
		++ptr;
	}

	return ptr;
}

static void usage(char* app) {
	FILE *out = stdout;
	fputfmt(out, "Usage: %s <global options> <local options>*\n", app);

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

#if defined(USEPLUGINS)
static const char* pluginLibInstall = "ccvmInit";
static const char* pluginLibDestroy = "ccvmDone";

static int installDll(state rt, int ccApiMain(state)) {
	return ccApiMain(rt);
}

#if (defined(WIN32) || defined(_WIN32))
#include <windows.h>
static struct pluginLib {
	struct pluginLib *next;		// next plugin
	void(*onClose)();
	HANDLE lib;					//
} *pluginLibs = NULL;
static void closeLibs() {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose) {
			lib->onClose();
		}
		if (lib->lib) {
			FreeLibrary((HINSTANCE)lib->lib);
		}
		free(lib);
	}
}
static int importLib(state rt, const char* path) {
	int result = -1;
	HANDLE library = LoadLibraryA(path);
	if (library != NULL) {
		void *install = (void*)GetProcAddress(library, pluginLibInstall);
		void *destroy = (void*)GetProcAddress(library, pluginLibDestroy);
		if (install != NULL) {
			struct pluginLib *lib = malloc(sizeof(struct pluginLib));
			if (lib != NULL) {
				result = installDll(rt, install);
				lib->lib = library;
				lib->onClose = destroy;
				lib->next = pluginLibs;
				pluginLibs = lib;
			}
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
	struct pluginLib *next;		// next plugin
	void (*onClose)();
	void *lib;					//
} *pluginLibs = NULL;
static void closeLibs() {
	while (pluginLibs != NULL) {
		struct pluginLib *lib = pluginLibs;
		pluginLibs = lib->next;

		if (lib->onClose) {
			lib->onClose();
		}
		if (lib->lib) {
			dlclose(lib->lib);
		}
		free(lib);
	}
}
static int importLib(state rt, const char* path) {
	int result = -1;
	void* library = dlopen(path, RTLD_NOW);
	if (library != NULL) {
		void* install = dlsym(library, pluginLibInstall);
		void* destroy = dlsym(library, pluginLibDestroy);
		if (install != NULL) {
			struct pluginLib *lib = malloc(sizeof(struct pluginLib));
			result = installDll(rt, install);
			lib->onClose = destroy;
			lib->next = pluginLibs;
			lib->lib = library;
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
	(void)pluginLibInstall;
	(void)pluginLibDestroy;
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

// forward functions
static void printGlobals(FILE* out, state rt, int all);
static int dbgConsole(state, int pu, void* ip, void* sp, size_t ss, vmError err, size_t fp);
extern int vmTest();
extern int vmHelp();

enum {
	sym_attr = 0x01,   // size, kind, attributes, initializer, ...
	sym_refs = 0x02,   // dump usage references of variable
	sym_prms = 0x04,   // print params

	//~ asm_locl = 0x10,   // local offsets
	//~ asm_glob = 0x20,   // global offsets
	//~ asm_data = 0x0f,   // print 0-15 bytes as hex code

	brk_fatal = 0x010,	// break on uncaught error
	brk_error = 0x020,	// break on caught error
	brk_start = 0x040,	// break on start
	dmp_time  = 0x100,	// print profiler results
	dmp_heap  = 0x400,	// print allocated heap memory
	dmp_vars  = 0x200,	// print global variable values
};
typedef struct {
	state rt;
	int indent;
	int sym_dump;
	int ast_dump;
	int asm_dump;
} customPrinterExtra;
static int dumpAsmCb(customPrinterExtra *extra, size_t offs, void* ip) {
	FILE *fout = extra->rt->logf;
	dbgInfo dbg = mapDbgStatement(extra->rt, offs);
	if (dbg != NULL && dbg->stmt != NULL && dbg->start == offs) {
		if ((extra->asm_dump & 0x30) != 0) {
			fputfmt(fout, "%I%s:%u: [%06x-%06x): %t\n", extra->indent, dbg->file, dbg->line, dbg->start, dbg->end, dbg->stmt);
		}
		else {
			fputfmt(fout, "%I%s:%u: %t\n", extra->indent, dbg->file, dbg->line, dbg->stmt);
		}
	}

	fputfmt(fout, "%I", extra->indent);
	fputasm(fout, ip, extra->asm_dump, extra->rt);
	fputfmt(fout, "\n");
	return 1;
}

static void customPrinter(customPrinterExtra *extra, symn sym) {
	int dumpAsm = 0;
	int dumpAst = 0;
	int identExt = extra->indent;
	FILE *fout = extra->rt->logf;

	if (extra->asm_dump >= 0 && sym->call && sym->kind == TYPE_ref) {
		if (sym->cast != TYPE_ref) {
			// function reference may point to null or another function.
			dumpAsm = extra->asm_dump;
		}
	}
	if (extra->ast_dump >= 0 && sym->init != NULL) {
		dumpAst = extra->ast_dump;
	}

	// filter symbols by request.
	if (extra->sym_dump < 0) {
		// skip builtin symbols if not requested (with '-api').
		if (sym->file == NULL && sym->line == 0) {
			return;
		}
		// skip symbol if there is nothing to dump.
		if (!dumpAsm && !dumpAst) {
			return;
		}
	}

	// print qualified name with arguments
	fputfmt(fout, "%+T: %+T {\n", sym, sym->type);

	// print symbol info (kind, size, offset, ...)
	if (extra->sym_dump >= 0 && extra->sym_dump & sym_attr) {
		// print symbol definition location
		if (sym->file != NULL && sym->line > 0) {
			fputfmt(fout, "%I.definition: %s:%u\n", identExt, sym->file, sym->line);
		}

		fputfmt(fout, "%I.kind:%?s%?s %K->%K\n", identExt
			, sym->stat ? " static" : ""
			, sym->cnst ? " const" : ""
			, sym->kind, sym->cast
		);
		fputfmt(fout, "%I.offset: %06x\n", identExt, sym->offs);
		fputfmt(fout, "%I.size: %d\n", identExt, sym->size);
	}

	// explain params of the function
	if (extra->sym_dump >= 0 && extra->sym_dump & sym_prms) {
		symn param;
		for (param = sym->prms; param; param = param->next) {
			fputfmt(fout, "%I.param %T: %?+T (@%06x+%d->%K)\n", identExt, param, param->type, param->offs, param->size, param->cast);
		}
	}

	if (dumpAst != 0) {
		fputfmt(fout, "%I.syntaxTree: %?-1.*t", identExt, dumpAst, sym->init);
		if (sym->init->kind != STMT_beg) {
			fputfmt(fout, "\n");
		}
	}

	// print disassembly of the function
	if (dumpAsm != 0) {
		fputfmt(fout, "%I.assembly [@%06x: %d] {\n", identExt, sym->offs, sym->size);
		iterateAsm(extra->rt, sym->offs, sym->offs + sym->size, extra, (void*)dumpAsmCb);
		fputfmt(fout, "%I}\n", identExt);
	}

	// print usages of symbol
	if (extra->sym_dump >= 0 && extra->sym_dump & sym_refs) {
		astn usage;
		int usages = 0;
		int extUsages = 0;
		fputfmt(fout, "%I.references: {\n", identExt);
		for (usage = sym->used; usage; usage = usage->ref.used) {
			if (usage->file && usage->line) {
				int referenced = !(usage->file == sym->file && usage->line == sym->line);
				fputfmt(fout, "%I%s:%u: %s as `%+t`\n", identExt, usage->file, usage->line, referenced ? "referenced" : "defined", usage);
				#ifdef LOG_MAX_ITEMS
				if ((usages += 1) > LOG_MAX_ITEMS) {
					break;
				}
				#endif
			}
			else {
				extUsages += 1;
			}
		}
		if (extUsages > 0) {
			fputfmt(fout, "%Iexternal references: %d\n", identExt, extUsages);
		}
		fputfmt(fout, "%I}\n", identExt);
		(void)usages;
	}

	fputfmt(fout, "}\n");
}

static int program(int argc, char* argv[]) {
	char* stdlib = (char*)STDLIB;

	int i, errors = 0;

	// compile, run, debug, profile, ...
	int gen_code = cgen_info | ol;	// optimize_level, debug_info, ...
	int run_code = -1;	// quit, exec, debug
	int sym_dump = -1;	// dump variables and functions.
	int ast_dump = -1;	// dump abstract syntax tree.
	int asm_dump = -1;	// dump disassembly.

	char* logf = NULL;			// logger filename
	FILE* dmpf = stdout;		// dump file

	char* cccf = NULL;			// compile filename
	int warn;

	char* amem = paddptr(mem, rt_size);
	state rt = rtInit(amem, sizeof(mem) - (amem - mem));

	// TODO: max 32 break points ?
	char* bp_file[32];
	int bp_line[32];
	int bp_type[32];
	int bp_size = 0;

	if (rt == NULL) {
		fatal("initializing runtime context");
		return -1;
	}

	// porcess global options
	for (i = 1; i < argc; ++i) {
		char* arg = argv[i];

		if (strncmp(arg, "-std", 4) == 0) {		// override stdlib file
			if (stdlib != (char*)STDLIB) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			if (arg[4] != 0) {
				stdlib = arg + 4;
			}
			else {
				// diasble standard library
				stdlib = NULL;
			}
		}

		// output file
		else if (strcmp(arg, "-log") == 0) {		// log
			if (++i >= argc || logf) {
				error(rt, NULL, 0, "log file not specified");
				return -1;
			}
			logf = argv[i];
		}
		/*else if (strcmp(arg, "-out") == 0) {		// out
			if (++i >= argc || outf) {
				error(rt, NULL, 0, "dump file not specified");
				return -1;
			}
			outf = argv[i];
		}*/

		// output what
		else if (strncmp(arg, "-api", 4) == 0) {	// tags
			if (sym_dump != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			sym_dump = -2;
			if (arg[4] != 0) {
				char* ptr = parsei32(arg + 4, &sym_dump, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return -1;
				}
			}
		}
		else if (strncmp(arg, "-ast", 4) == 0) {	// tree
			if (ast_dump != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			ast_dump = 0x7f;
			if (arg[4] != 0) {
				char* ptr = parsei32(arg + 4, &ast_dump, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return -1;
				}
			}
		}
		else if (strncmp(arg, "-asm", 4) == 0) {	// dasm
			if (asm_dump != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			asm_dump = 0x29;	// 2 use global offset, 9 characters for bytecode hexview
			if (arg[4] != 0) {
				char* ptr = parsei32(arg + 4, &asm_dump, 16);
				if (*ptr) {
					error(rt, NULL, 0, "invalid argument '%s'\n", arg);
					return -1;
				}
			}
		}

		else if (strncmp(arg, "-dbg", 4) == 0) {	// break into debuger if ...
			char *brk = arg + 4;
			if (run_code != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
			}
			if (*brk >= '0' && *brk <= '9') {
				brk = parsei32(brk, &gen_code, 16);
			}
			run_code = brk_fatal;
			if (strchr(brk, 'a')) {
				run_code |= brk_error;
			}
			if (strchr(brk, 's')) {
				run_code |= brk_start;
			}
			if (strchr(brk, 'v')) {
				run_code |= dmp_vars;
			}
			if (strchr(brk, 'h')) {
				run_code |= dmp_heap;
			}
			if (strchr(brk, 't')) {
				run_code |= dmp_time;
			}
		}
		else if (strcmp(arg, "-run") == 0) {		// execute code in release mode
			if (run_code != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
			}
			gen_code = ol;
			run_code = 0;
		}

		// no more global options
		else break;
	}

	// open log file (global option)
	if (logf && logfile(rt, logf, 0) != 0) {
		error(rt, NULL, 0, "can not open log file: `%s`", logf);
		return -1;
	}

	// intstall base type system.
	if (!ccInit(rt, creg_def, NULL)) {
		error(rt, NULL, 0, "error registering base types");
		logfile(rt, NULL, 0);
		return -6;
	}

	// intstall standard library.
	if (!ccAddUnit(rt, install_stdc, sym_dump == -2 ? 0 : 3, stdlib)) {
		error(rt, NULL, 0, "error registering standard libs");
		logfile(rt, NULL, 0);
		return -6;
	}

	// intstall file operations.
	if (!ccAddUnit(rt, install_file, 0, NULL)) {
		error(rt, NULL, 0, "error registering file libs");
		logfile(rt, NULL, 0);
		return -6;
	}

	// compile and import files / modules
	for (warn = -1; i <= argc; ++i) {
		char* arg = argv[i];
		if (i == argc || *arg != '-') {
			if (cccf != NULL) {
				char* ext = strrchr(cccf, '.');
				if (ext && (streq(ext, ".so") || streq(ext, ".dll"))) {
					int resultCode = importLib(rt, cccf);
					if (resultCode != 0) {
						error(rt, NULL, 0, "error(%d) importing library `%s`", resultCode, cccf);
					}
				}
				else if (!ccAddCode(rt, warn == -1 ? wl : warn, cccf, 1, NULL)) {
					error(rt, NULL, 0, "error compiling source `%s`", arg);
				}
				cccf = NULL;
			}
			cccf = arg;
			warn = -1;
		}
		else {
			if (cccf == NULL) {
				error(rt, NULL, 0, "argument `%s` must be preceded by a file", arg);
			}
			if (arg[1] == 'w') {		// warning level for file
				if (warn != -1) {
					info(rt, NULL, 0, "argument overwrites previous value: %d", warn);
				}
				if (strcmp(arg, "-wx") == 0) {
					warn = -2;
				}
				else if (strcmp(arg, "-wa") == 0) {
					warn = 32;
				}
				else if (*parsei32(arg + 2, &warn, 10)) {
					error(rt, NULL, 0, "invalid warning level '%s'", arg + 2);
				}
			}
			else if (arg[1] == 'b' || arg[1] == 't') {
				int line = 0;
				if (bp_size > lengthOf(bp_file)) {
					info(rt, NULL, 0, "can not add more than %d breakponts.", lengthOf(bp_file));
				}
				if (*parsei32(arg + 2, &line, 10)) {
					error(rt, NULL, 0, "invalid line number `%s`", arg + 2);
				}
				bp_file[bp_size] = cccf;
				bp_line[bp_size] = line;
				bp_type[bp_size] = arg[1];
				bp_size += 1;
			}
			else {
				error(rt, NULL, 0, "invalid option: `%s`", arg);
			}
		}
	}

	errors = rt->errc;

	// generate code only if needed and there are no compilation errors
	if (errors == 0 && (gen_code || run_code >= 0)) {
		if (!gencode(rt, gen_code)) {
			// show dump, but do not execute broken code.
			//error(rt, NULL, 0, "error generating code");
			errors += 1;
		}
		// set breakpoints
		for (i = 0; i < bp_size; ++i) {
			char *file = bp_file[i];
			int line = bp_line[i];
			int type = bp_type[i];
			dbgInfo dbg = getDbgStatement(rt, file, line);
			if (dbg != NULL) {
				dbg->bp = type;
				//info(rt, NULL, 0, "%s:%u: breakpoint", file, line);
			}
			else {
				info(rt, NULL, 0, "%s:%u: invalid breakpoint", file, line);
			}
		}
	}

	// TODO: dump and log file should be different.
	if (rt->logf != NULL) {
		dmpf = rt->logf;
	}

	if (sym_dump != -1 || ast_dump >= 0 || asm_dump >= 0) {
		if (sym_dump == -2 && (ast_dump >= 0 || asm_dump >= 0)) {
			// invalidate api dump if assembly or syntax tree dump is requested.
			sym_dump = -1;
		}
		if (sym_dump == -2) {
			dump(rt, NULL, NULL);
		}
		else {
			customPrinterExtra dumpExtra = {
				.sym_dump = sym_dump,
				.ast_dump = ast_dump,
				.asm_dump = asm_dump,
				.indent = 1,
				.rt = rt
			};
			dump(rt, &dumpExtra, (void*)customPrinter);
		}
	}

	// run code if there are no compilation errors.
	if (errors == 0 && run_code >= 0) {
		if (rt->dbg != NULL && run_code > 1) {
			if (run_code & (brk_fatal|brk_error|brk_start)) {
				rt->dbg->dbug = dbgConsole;
			}
			if (run_code & brk_start) {
				// break on first instruction
				rt->dbg->breakAt = rt->vm.pc;
				rt->dbg->breakLt = rt->vm.ro;
				rt->dbg->breakGt = rt->vm.px + px_size;
			}
			else {
				rt->dbg->breakAt = 0;
				rt->dbg->breakLt = rt->vm.ro;
				rt->dbg->breakGt = rt->vm.px + px_size;
			}
		}
		fputfmt(dmpf, "\n>/*-- exec:\n");
		errors = execute(rt, NULL, rt->_size / 4);
		fputfmt(dmpf, "\n// */\n");
		if (run_code & dmp_time) {
			fputfmt(dmpf, "\n>/*-- trace:\n");
			if (rt->dbg) {
				int n = rt->dbg->functions.cnt;
				dbgInfo dbg = (dbgInfo)rt->dbg->functions.ptr;
				while (n > 0) {
					if (dbg->hits) {
						symn sym = dbg->decl;
						if (sym == NULL) {
							sym = mapsym(rt, dbg->start, 1);
						}
						fputfmt(dmpf, "%s:%u:[.%06x, .%06x): <%?T> hits(%D), time(%D%?+D / %.3F%?+.3F ms)\n"
							, dbg->file, dbg->line, dbg->start, dbg->end, sym
							, (int64_t)dbg->hits, (int64_t)dbg->funcTime, (int64_t)dbg->diffTime
							, dbg->funcTime / (double)CLOCKS_PER_SEC, dbg->diffTime / (double)CLOCKS_PER_SEC
						);
					}
					dbg++;
					n--;
				}
				fputfmt(dmpf, ">//-- trace.statements:\n");
				n = rt->dbg->statements.cnt;
				dbg = (dbgInfo)rt->dbg->statements.ptr;
				while (n > 0) {
					int symOffs = 0;
					symn sym = dbg->decl;
					if (sym == NULL) {
						sym = mapsym(rt, dbg->start, 1);
					}
					if (sym != NULL) {
						symOffs = dbg->start - sym->offs;
					}
					fputfmt(dmpf, "%s:%u:[.%06x, .%06x): <%?T+%d> hits(%D), time(%D%?+D / %.3F%?+.3F ms)\n"
						, dbg->file, dbg->line, dbg->start, dbg->end, sym, symOffs
						, (int64_t)dbg->hits, (int64_t)dbg->funcTime, (int64_t)dbg->diffTime
						, dbg->funcTime / (double)CLOCKS_PER_SEC, dbg->diffTime / (double)CLOCKS_PER_SEC
					);
					dbg++;
					n--;
				}
			}
			fputfmt(dmpf, "// */\n");
		}
		if (run_code & dmp_heap) {
			// show allocated memory chunks.
			fputfmt(dmpf, "\n>/*-- heap:\n");
			rtAlloc(rt, NULL, 0);
			fputfmt(dmpf, "// */\n");
		}
		if (run_code & dmp_vars) {
			fputfmt(dmpf, "\n>/*-- vars:\n");
			printGlobals(dmpf, rt, 0);
			fputfmt(dmpf, "// */\n");
		}
	}

	// close log file
	logfile(rt, NULL, 0);
	closeLibs();
	return errors;
}

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

static void printGlobals(FILE* out, state rt, int all) {
	symn var;
	for (var = rt->defs; var; var = var->next) {
		char* ofs = NULL;

		// exclude types
		if (var->kind != TYPE_ref)
			continue;

		// exclude functions
		if (var->call && !all)
			continue;

		// exclude null
		if (var->offs == 0)
			continue;

		if (var->file && var->line) {
			fputfmt(out, "%s:%u:%u: ", var->file, var->line, var->colp);
		}
		else {
			//~ fputfmt(out, "var: ");
		}

		if (var->stat) {
			// static variable.
			ofs = (char*)rt->_mem + var->offs;
		}
		else {
			// argument or local variable.
			// in case if: var != rt->defs.
			//~ ofs = (char*)rt->retv + rt->fun->prms->offs - var->offs;
			continue;
		}

		fputval(rt, out, var, (stkval*)ofs, 0, prSymQual |prType);
		fputc('\n', out);
	}
}

static int dbgConsole(state rt, int pu, void* ip, void* sp, size_t ss, vmError err, size_t fp) {
	/* commands
	 *   \0: break and read command
	 *   r: break on next breakpoint (continue)
	 *   n: break on next line (step over line)
	 *   i: break on next call (step into function)
	 *   o: break on next return (step out function)
	 *   a: break on next opcode (step over opcode)

	 *   p: print (print top of stack)
	 *   t: trace (print callstack)
	 */
	static char lastCommand = 'r';
	char buff[1024];
	customPrinterExtra dumpExtra = {
		.asm_dump = 0x30,
		.sym_dump = -1,
		.ast_dump = -1,
		.indent = 1,
		.rt = rt
	};

	int brk = 0;
	size_t i = vmOffset(rt, ip);
	symn fun = mapsym(rt, i, 0);
	dbgInfo dbg = mapDbgStatement(rt, i);

	if (err != noError) {
		brk = '!';
	}
	else if (i == rt->dbg->breakAt) {
		// scheduled break
		brk = '=';
	}
	else if (i < rt->dbg->breakLt) {
		// scheduled break
		brk = '<';
	}
	else if (i > rt->dbg->breakGt) {
		// scheduled break
		brk = '>';
	}
	else if (dbg != NULL) {
		// check breakpoint hit
		if (dbg->bp && i == dbg->start) {
			brk = '+';
		}
	}

	// no need to break, continue execution.
	if (brk == 0) {
		return 0;
	}

	// print the error message in case of unhandled errors
	if (1 || !rt->dbg->checked) {
		char *errorType = NULL;
		switch (err) {
			case noError:
				errorType = "Breakpoint";
				break;

			case invalidIP:
				errorType = "Invalid instruction pointer";
				break;

			case invalidSP:
				errorType = "Invalid stack pointer";
				break;

			case illegalInstruction:
				errorType = "Invalid instruction";
				break;

			case traceOverflow:
			case stackOverflow:
				errorType = "Stack Overflow";
				break;

			case divisionByZero:
				errorType = "Division by Zero";
				break;

			case libCallAbort:
				errorType = "External call abort";
				break;

			case memReadError:
				errorType = "Access violation reading memory";
				break;

			case memWriteError:
				errorType = "Access violation writing memory";
				break;

			default:
				//Unhandled exception at 0x0140AA67 in cc.exe : 0xC00000FD : Stack overflow(parameters : 0x00000000, 0x00F52000).
				//Unhandled exception at 0x00F3AA67 in cc.exe : 0xC0000005 : Access violation reading location 0x00C90000.
				//Unhandled exception at 0x010D409C in cc.exe : 0xC0000005 : Access violation writing location 0x00000000.
				errorType = "Unknown error";
				break;
		}

		// print error type
		if (errorType != NULL) {
			int funOffs = i;
			if (fun != NULL) {
				funOffs = i - fun->offs;
			}
			// print current file position
			if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
				fputfmt(stdout, "%s:%u: ", dbg->file, dbg->line);
			}
			fputfmt(stdout, "%s in function: <%T+%06x>\n", errorType, fun, funOffs);
		}
	}
	for ( ; ; ) {
		char* arg = NULL;
		char cmd = lastCommand;

		fputfmt(stdout, ">dbg[%?c%?c]: ", brk, cmd);
		if (fgets(buff, sizeof(buff), stdin) == NULL) {
			// Can not read from stdin
			fputfmt(stdout, "can not read from standard input, continue\n");
			cmd = 'r';
			return 0;
		}
		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0) {}		// no command, use last one
		else if (buff[1] == 0) {// one char command
			arg = buff + 1;
			cmd = buff[0];
		}
		else if ((arg = parsecmd(buff, "step", " "))) {
			if (*arg == 0) {	// step == step over
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
			else {
				cmd = -1;	// error
			}
		}
		else if ((arg = parsecmd(buff, "continue", " "))) {
			if (*arg != 0) {
				cmd = -1;	// error
			}
			else {
				cmd = 'N';
			}
		}

		else if ((arg = parsecmd(buff, "trace", " "))) {
			if (*arg != 0) {
				cmd = -1;	// error
			}
			else {
				cmd = 't';
			}
		}
		else if ((arg = parsecmd(buff, "print", " "))) {
			if (*arg != 0) {
				cmd = -1;	// error
			}
			else {
				cmd = 'p';
			}
		}
		else {
			cmd = -1;	// error
		}
		if (arg == NULL) {
			cmd = -1;
		}

		switch (cmd) {
			default:
				fputfmt(stdout, "invalid command '%s'\n", buff);
				break;

			case 0:
				break;

			case 'q':		// abort
				return -1;
			case 'r':		// resume
				if (rt->dbg) {
					rt->dbg->breakAt = 0;
					rt->dbg->breakLt = rt->vm.ro;
					rt->dbg->breakGt = rt->vm.px + px_size;
				}
				lastCommand = 'r';
				return 0;
			case 'a':		// step over opcode
				if (rt->dbg && dbg) {
					rt->dbg->breakAt = 0;
					rt->dbg->breakLt = dbg->start;
					rt->dbg->breakGt = dbg->start;
				}
				lastCommand = 'a';
				return 0;
			case 'n':		// step over line
				if (rt->dbg && dbg) {
					rt->dbg->breakAt = 0;
					rt->dbg->breakLt = dbg->start;
					rt->dbg->breakGt = dbg->end;
				}
				lastCommand = 'n';
				return 0;

			// print
			case 'p':
				if (*arg == 0) {
					// print top of stack
					for (i = 0; i < ss; i++) {
						stkval* v = (stkval*)&((long*)sp)[i];
						fputfmt(stdout, "\tsp(%d): {0x%08x, i32(%d), f32(%f), i64(%D), f64(%F)}\n", i, v->i4, v->i4, v->f4, v->i8, v->f8);
					}
				}
				else {
					symn sym = ccFindSym(rt->cc, NULL, arg);
					fputfmt(stdout, "arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && !sym->stat) {
						fputval(rt, stdout, sym, (stkval*)sp, 0, prType);
					}
				}
				break;

			// instruction
			case 's':
				dumpAsmCb(&dumpExtra, i, ip);
				break;

			// trace
			case 't':
				logTrace(rt, stdout, 1, 0, 20);
				break;
		}
	}
	return 0;
	(void)pu;
	(void)fp;
}
