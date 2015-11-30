/* command line options
application [global options] [local options]...

<global options>:
  -std<file>            specify custom standard library file.
                        empty file name disables std library compilation.

  -log[/ *] <file>      set logger for: compilation errors and warnings, runtime debug messages
    /a                  append to the log file

  -dump[.?] <file>      set output for: dump(symbols, assembly, abstract syntax tree, coverage, call tree)
    .scite              dump api for scite text editor
    .json               Turn on javascript output.

  -api[<hex>]           dump symbols
  -ast[<hex>]           dump syntax tree
  -asm[<hex>]           dump assembled code

  -run                  run without: debug information, stacktrace, bounds checking, ...

  -debug[<hex>][/ *]    run with attached debugger, break on uncaught errors
    <hex>               custom optimization level for code generation
    /a                  break on caught errors
    /s                  break on startup
    /v                  print global variable values

  -profile[/ *]         run code with profiler: coverage, method tracing
+   /c                  dump call tree
+   /m                  dump allocated heap memory
    without c or m dump will contain only execution times and hit count of functions and statements.

<local options>: filename followed by swithes
  <file>                  if file extension is (.so|.dll) load as library else compile
  -b<int>                 break point on <int> line in current file
? -d<int>                 trace point on <int> line in current file
  -w[a|x|<hex>]           set or disable warning level for current file

examples:
>app -dump.json api.json
    dump builtin symbols to `api.json` file (types, functions, variables, aliases)

>app -run test.tracing.ccc
    compile and execute the file `test.tracing.ccc`

?
>app -debug -gl.so -w0 gl.ccc -w0 test.ccc -wx -b12 -b15 -d19
        execute in debug mode
        import `gl.so`
            with no warnings
        compile `gl.ccc`
            with no warnings
        compile `test.ccc`
            treating all warnings as errors
            break execution on lines 12 and 15
            print stacktrace when line 19 is hit
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

	fputfmt(out, "  <global options>:\n");
	fputfmt(out, "    -log<file>     logger\n");
	fputfmt(out, "    -out<file>     output\n");
	fputfmt(out, "    -std<file>     override standard library path\n");
	fputfmt(out,"\n");
	fputfmt(out, "    -run           execute\n");
	fputfmt(out, "    -debug         execute with debugerr\n");
	fputfmt(out, "    -profile       execute\n");
	fputfmt(out,"\n");
	fputfmt(out,"    -api[<hex>]     dump symbols\n");
	fputfmt(out,"    -asm[<hex>]     dump assembly\n");
	fputfmt(out,"    -ast[<hex>]     dump syntax tree\n");
	fputfmt(out,"\n");
	fputfmt(out,"  <local options>: filename followed by swithes\n");
	fputfmt(out,"    <file>          if file extension is (.so|.dll) import else compile\n");
	fputfmt(out,"    -w[a|x|<int>]   set warning level\n");
	fputfmt(out,"    -b<int>         set break point on line\n");
	//~ fputfmt(out,"    -L<file>      import plugin (.so|.dll)\n");
	//~ fputfmt(out,"    -C<file>      compile source\n");
}

#if defined(USEPLUGINS)
static struct pluginLib *pluginLibs = NULL;
static const char* pluginLibInstall = "ccvmInit";
static const char* pluginLibDestroy = "ccvmDone";

static int installDll(state rt, int ccApiMain(state)) {
	return ccApiMain(rt);
}

#if (defined(WIN32) || defined(_WIN32))
#include <windows.h>
struct pluginLib {
	struct pluginLib *next;		// next plugin
	void(*onClose)();
	HANDLE lib;					//
};
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
#include <stddef.h>

struct pluginLib {
	struct pluginLib *next;		// next plugin
	void (*onClose)();
	void *lib;					//
};
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
extern int vmTest();
extern int vmHelp();

enum {
	sym_attr = 0x01,   // size, kind, attributes, initializer, ...
	sym_refs = 0x02,   // dump usage references of symbol
	sym_prms = 0x04,   // print params

	asm_addr = prAsmAddr,   // use global address: (@0x003d8c)
	asm_syms = prAsmSyms,   // use symbol names instead of addresses: <main+80>
	asm_stmt = prAsmStmt,   // print source code statements
	//asm_data = prAsmCode,   // print 0-15 bytes as hex code

	brk_fatal = 0x010,	// break on uncaught error
	brk_error = 0x020,	// break on caught error
	brk_start = 0x040,	// break on startup

	dmp_prof  = 0x100,	// print profiler results
	dmp_heap  = 0x400,	// print allocated heap memory
	dmp_vars  = 0x200,	// print global variable values
	dmp_tree  = 0x800,	// print call tree
};
struct customContext {
	state rt;
	FILE *out;
	int indent;
	int sym_dump;
	int ast_dump;
	int asm_dump;
	int run_dump;
	size_t breakAt;		// break if pc is equal
	size_t breakLt;		// break if pc is less than
	size_t breakGt;		// break if pc is greater than
};

// text output
static void conDumpProfile(FILE* out, state rt, int all) {
	fputfmt(out, "\n/*-- trace:\n");
	if (rt->dbg) {
		int i;
		int covFunc = 0, nFunc = rt->dbg->functions.cnt;
		int covStmt = 0, nStmt = rt->dbg->statements.cnt;
		dbgInfo dbg = (dbgInfo) rt->dbg->functions.ptr;
		for (i = 0; i < nFunc; ++i, dbg++) {
			symn sym = dbg->decl;
			if (dbg->hits == 0) {
				continue;
			}
			covFunc += 1;
			if (sym == NULL) {
				sym = mapsym(rt, dbg->start, 1);
			}
			fputfmt(out,
					"%s:%u:[.%06x, .%06x): <%?T> hits(%D/%D), time(%D%?+D / %.3F%?+.3F ms)\n", dbg->file,
					dbg->line, dbg->start, dbg->end, sym, (int64_t) dbg->hits, (int64_t) dbg->exec,
					(int64_t) dbg->total, (int64_t) -(dbg->total - dbg->self),
					dbg->total / (double) CLOCKS_PER_SEC, -(dbg->total - dbg->self) / (double) CLOCKS_PER_SEC
			);
		}

		fputfmt(out, ">//-- trace.statements:\n");
		dbg = (dbgInfo) rt->dbg->statements.ptr;
		for (i = 0; all && i < nStmt; ++i, dbg++) {
			size_t symOffs = 0;
			symn sym = dbg->decl;
			if (dbg->hits == 0) {
				continue;
			}
			covStmt += 1;
			if (sym == NULL) {
				sym = mapsym(rt, dbg->start, 1);
			}
			if (sym != NULL) {
				symOffs = dbg->start - sym->offs;
			}
			fputfmt(out,
					"%s:%u:[.%06x, .%06x): <%?T+%d> hits(%D/%D), time(%D%?+D / %.3F%?+.3F ms)\n", dbg->file,
					dbg->line, dbg->start, dbg->end, sym, symOffs, (int64_t) dbg->hits, (int64_t) dbg->exec,
					(int64_t) dbg->total, (int64_t) -(dbg->total - dbg->self),
					dbg->total / (double) CLOCKS_PER_SEC, -(dbg->total - dbg->self) / (double) CLOCKS_PER_SEC
			);
		}

		fputfmt(out, "//-- coverage(functions: %.2f%%, statements: %.2f%%)\n",
				covFunc * 100. / nFunc, covStmt * 100. / nStmt);
		fputfmt(out, "//-- coverage(functions: %d/%d, statements: %d/%d)\n", covFunc, nFunc,
				covStmt, nStmt);
	}
	fputfmt(out, "// */\n");
}
static void conDumpGlobals(FILE* out, state rt, int all) {
	symn var;
	for (var = rt->defs; var; var = var->next) {
		char* ofs = NULL;

		// exclude typenames
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
static int conDumpAsm(customContext extra, size_t offs, void* ip) {
	FILE *fout = extra->out;
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
static void conDump(customContext extra, symn sym) {
	int dumpAsm = 0;
	int dumpAst = 0;
	int identExt = extra->indent;
	FILE *out = extra->out;

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

	if (extra->sym_dump == 0) {
		fputfmt(out, "%+T: %+T\n", sym, sym->type);
		return;
	}

	// print qualified name with arguments
	fputfmt(out, "%+T: %+T {\n", sym, sym->type);

	// print symbol info (kind, size, offset, ...)
	if (extra->sym_dump >= 0 && extra->sym_dump & sym_attr) {
		// print symbol definition location
		if (sym->file != NULL && sym->line > 0) {
			fputfmt(out, "%I.definition: %s:%u\n", identExt, sym->file, sym->line);
		}

		fputfmt(out, "%I.kind:%?s%?s %K->%K\n", identExt
			, sym->stat ? " static" : ""
			, sym->cnst ? " const" : ""
			, sym->kind, sym->cast
		);
		fputfmt(out, "%I.offset: %06x\n", identExt, sym->offs);
		fputfmt(out, "%I.size: %d\n", identExt, sym->size);
	}

	// explain params of the function
	if (extra->sym_dump >= 0 && extra->sym_dump & sym_prms) {
		symn param;
		for (param = sym->prms; param; param = param->next) {
			fputfmt(out, "%I.param %T: %?+T (@%06x+%d->%K)\n", identExt, param, param->type, param->offs, param->size, param->cast);
		}
	}

	if (dumpAst != 0) {
		fputfmt(out, "%I.syntaxTree: %?-1.*t", identExt, dumpAst, sym->init);
		if (sym->init->kind != STMT_beg) {
			fputfmt(out, "\n");
		}
	}

	// print disassembly of the function
	if (dumpAsm != 0) {
		fputfmt(out, "%I.assembly [@%06x: %d] {\n", identExt, sym->offs, sym->size);
		iterateAsm(extra->rt, sym->offs, sym->offs + sym->size, extra, conDumpAsm);
		fputfmt(out, "%I}\n", identExt);
	}

	// print usages of symbol
	if (extra->sym_dump >= 0 && extra->sym_dump & sym_refs) {
		astn usage;
		int usages = 0;
		int extUsages = 0;
		fputfmt(out, "%I.references: {\n", identExt);
		for (usage = sym->used; usage; usage = usage->ref.used) {
			if (usage->file && usage->line) {
				int referenced = !(usage->file == sym->file && usage->line == sym->line);
				fputfmt(out, "%I%s:%u: %s as `%+t`\n", identExt, usage->file, usage->line, referenced ? "referenced" : "defined", usage);
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
			fputfmt(out, "%Iexternal references: %d\n", identExt, extUsages);
		}
		fputfmt(out, "%I}\n", identExt);
		(void)usages;
	}

	fputfmt(out, "}\n");
}
static int conDebug(customContext ctx, vmError error, size_t ss, void* sp, void* caller, void* callee) {
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

	int brk = 0;
	state rt = ctx->rt;
	FILE *out = stdout;//ctx->out;
	size_t i = vmOffset(rt, caller);
	symn fun = mapsym(rt, i, 0);
	dbgInfo dbg = mapDbgStatement(rt, i);

	if (callee != NULL && error == noError) {
		// TODO: enter or leave a function (step in / step out)
	}
	else if (error != noError) {
		brk = '!';
	}
	else if (i == ctx->breakAt) {
		// scheduled break
		brk = '=';
	}
	else if (i < ctx->breakLt) {
		// scheduled break
		brk = '<';
	}
	else if (i > ctx->breakGt) {
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
		switch (error) {
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
			size_t funOffs = i;
			if (fun != NULL) {
				funOffs = i - fun->offs;
			}
			// print current file position
			if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
				fputfmt(out, "%s:%u: ", dbg->file, dbg->line);
			}
			fputfmt(out, "%s in function: <%T+%06x>\n", errorType, fun, funOffs);
		}
	}
	for ( ; ; ) {
		char* arg = NULL;
		char cmd = lastCommand;

		fputfmt(out, ">dbg[%?c%?c]: ", brk, cmd);
		if (fgets(buff, sizeof(buff), stdin) == NULL) {
			// Can not read from stdin
			fputfmt(out, "can not read from standard input, continue\n");
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
				fputfmt(out, "invalid command '%s'\n", buff);
				break;

			case 0:
				break;

			case 'q':		// abort
				return -1;
			case 'r':		// resume
				ctx->breakAt = 0;
				ctx->breakLt = rt->vm.ro;
				ctx->breakGt = rt->vm.px + px_size;
				lastCommand = 'r';
				return 0;
			case 'a':		// step over opcode
				ctx->breakAt = 0;
				ctx->breakLt = dbg->start;
				ctx->breakGt = dbg->start;
				lastCommand = 'a';
				return 0;
			case 'n':		// step over line
				ctx->breakAt = 0;
				ctx->breakLt = dbg->start;
				ctx->breakGt = dbg->end;
				lastCommand = 'n';
				return 0;

			// print
			case 'p':
				if (*arg == 0) {
					// print top of stack
					for (i = 0; i < ss; i++) {
						stkval* v = (stkval*)&((long*)sp)[i];
						fputfmt(out, "\tsp(%d): {0x%08x, i32(%d), f32(%f), i64(%D), f64(%F)}\n", i, v->i4, v->i4, v->f4, v->i8, v->f8);
					}
				}
				else {
					symn sym = ccFindSym(rt->cc, NULL, arg);
					fputfmt(out, "arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && !sym->stat) {
						fputval(rt, out, sym, (stkval*)sp, 0, prType);
					}
				}
				break;

			// instruction
			case 's':
				conDumpAsm(ctx, i, caller);
				break;

			// trace
			case 't':
				logTrace(rt, out, 1, 0, 20);
				break;
		}
	}
	return 0;
}
static int conProfile(customContext ctx, vmError error, size_t ss, void* sp, void* caller, void* callee) {
	if (callee != NULL) {
		state rt = ctx->rt;
		FILE *out = ctx->out;
		//* trace every enter and leave
		clock_t now = clock();
		if ((ptrdiff_t) callee < 0) {
			if (callee == (void *) -1) {
				fputfmt(out, "% I< %d\n", ss, now);
			}
			else {
				fputfmt(out, "% I<< %d / %d\n", ss, now, callee);
			}
		}
		else {
			size_t offs = vmOffset(rt, callee);
			fputfmt(out, "% I> %d,0x%06x %?T\n", ss, now, offs, mapsym(rt, offs, 1));
		}
		// */
		// last call
		if (ss == 0 && caller == NULL && (ptrdiff_t) callee < 0) {

			conDumpProfile(out, rt, 1);

			if (ctx->run_dump & dmp_heap) {
				// show allocated memory chunks.
				fputfmt(out, "\n/*-- heap:\n");
				rtAlloc(rt, NULL, 0);
				fputfmt(out, "// */\n");
			}

			if (ctx->run_dump & dmp_vars) {
				fputfmt(out, "\n/*-- vars:\n");
				conDumpGlobals(out, rt, 0);
				fputfmt(out, "// */\n");
			}
		}
	}
	return 0;
}

static void dmpSciTEApi(customContext extra, symn sym) {
	FILE *out = extra->out;
	if (sym->prms != NULL && sym->call) {
		fputfmt(out, "%-T", sym);
	}
	else {
		fputfmt(out, "%T", sym);
	}
	fputfmt(out, "\n");
}

// json output
static void jsonDumpSym(FILE *out, const char **esc, symn ptr, const char *kind, int indent) {
	static const char* KEY_KIND = "kind";
	static const char* KEY_FILE = "file";
	static const char* KEY_LINE = "line";
	static const char* KEY_NAME = "name";
	static const char* KEY_PROTO = "proto";
	//~ static const char* KEY_DECL = "declaredIn";
	static const char* KEY_TYPE = "type";
	//~ static const char* KEY_INIT = "init";
	//~ static const char* KEY_CODE = "code";
	static const char* KEY_ARGS = "args";
	static const char* KEY_CONST = "const";
	static const char* KEY_STAT = "static";
	//~ static const char* KEY_PARLEL = "parallel";
	static const char* KEY_CAST = "cast";
	static const char* KEY_SIZE = "size";
	static const char* KEY_OFFS = "offs";

	static const char* VAL_TRUE = "true";
	static const char* VAL_FALSE = "false";

	//~ static const char* KIND_PARAM = "param";
	//~ static const char* FMT_COMMENT = "{ // %T: %T\n";

	if (kind != NULL) {
		fputesc(out, esc, "%I\"%s\": ", indent - 1, kind);
	}
	fputesc(out, NULL, "{\n");

	fputesc(out, esc, "%I\"%s\": \"%K\",\n", indent, KEY_KIND, ptr->kind);
	fputesc(out, esc, "%I\"%s\": \"%.T\",\n", indent, KEY_NAME, ptr);
	fputesc(out, esc, "%I\"%s\": \"%-T\",\n", indent, KEY_PROTO, ptr);

	//~ fputesc(out, esc, "%I\"%s\": \"%?+T\",\n", indent, KEY_DECL, ptr->decl);

	if (ptr->type != NULL) {
		fputesc(out, esc, "%I\"%s\": \"%T\",\n", indent, KEY_TYPE, ptr->type);
	}
	if (ptr->file != NULL) {
		fputesc(out, esc, "%I\"%s\": \"%s\",\n", indent, KEY_FILE, ptr->file);
	}
	if (ptr->line != 0) {
		fputesc(out, esc, "%I\"%s\": %u,\n", indent, KEY_LINE, ptr->line);
	}
	if (ptr->init != NULL) {
		//~ fputesc(out, esc, "%I\"%s\": \"%+t\",\n", indent, KEY_CODE, ptr->init);
	}

	if (ptr->call && ptr->prms) {
		symn arg;
		fputesc(out, NULL, "%I\"%s\": [", indent, KEY_ARGS);
		for (arg = ptr->prms; arg; arg = arg->next) {
			if (arg != ptr->prms) {
				fputesc(out, NULL, ", ");
			}
			//~ fputesc(out, NULL, FMT_COMMENT, arg, arg->type);
			jsonDumpSym(out, esc, arg, NULL, indent + 1);
		}
		fputesc(out, NULL, "],\n");
	}
	if (ptr->cast != 0) {
		fputesc(out, NULL, "%I\"%s\": \"%K\",\n", indent, KEY_CAST, ptr->cast);
	}
	fputesc(out, NULL, "%I\"%s\": %u,\n", indent, KEY_SIZE, ptr->size);
	fputesc(out, NULL, "%I\"%s\": %u,\n", indent, KEY_OFFS, ptr->offs);
	fputesc(out, NULL, "%I\"%s\": %s,\n", indent, KEY_CONST, ptr->cnst ? VAL_TRUE : VAL_FALSE);
	fputesc(out, NULL, "%I\"%s\": %s\n", indent, KEY_STAT, ptr->stat ? VAL_TRUE : VAL_FALSE);
	// no parallel symbols!!! fputesc(out, NULL, "%I\"%s\": %s,\n", indent, KEY_PARLEL, ptr->stat ? VAL_TRUE : VAL_FALSE);
	fputesc(out, esc, "%I}", indent - 1);
}
static void jsonDumpAst(FILE *out, const char **esc, astn ast, const char *kind, int indent) {
	static const char* KEY_KIND = "kind";
	static const char* KEY_CAST = "cast";
	static const char* KEY_FILE = "file";
	static const char* KEY_LINE = "line";
	static const char* KEY_TYPE = "type";
	static const char* KEY_STMT = "stmt";
	static const char* KEY_INIT = "init";
	static const char* KEY_TEST = "test";
	static const char* KEY_THEN = "then";
	static const char* KEY_STEP = "step";
	static const char* KEY_ELSE = "else";
	static const char* KEY_ARGS = "args";
	static const char* KEY_LHSO = "lval";
	static const char* KEY_RHSO = "rval";
	static const char* KEY_VALUE = "value";
	//~ static const char* FMT_COMMENT = " // %t\n";

	if (ast == NULL) {
		return;
	}
	if (kind != NULL) {
		fputesc(out, esc, "%I\"%s\": {", indent - 1, kind);
		//~ fputesc(out, NULL, FMT_COMMENT, ast);
	}

	fputesc(out, esc, "%I\"%s\": \"%K\",\n", indent, KEY_KIND, ast->kind);
	if (ast->type != NULL) {
		fputesc(out, esc, "%I\"%s\": \"%T\",\n", indent, KEY_TYPE, ast->type);
	}
	if (ast->cst2 != TYPE_any) {
		fputesc(out, esc, "%I\"%s\": \"%K\",\n", indent, KEY_CAST, ast->cst2);
	}
	if (ast->file != NULL) {
		fputesc(out, esc, "%I\"%s\": \"%s\",\n", indent, KEY_FILE, ast->file);
	}
	if (ast->line != 0) {
		fputesc(out, esc, "%I\"%s\": %u,\n", indent, KEY_LINE, ast->line);
	}
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			fatal("%K", ast->kind);
			return;
		//#{ STMT
		case STMT_beg: {
			astn list;
			fputesc(out, esc, "%I\"%s\": [{", indent, KEY_STMT);
			for (list = ast->stmt.stmt; list; list = list->next) {
				if (list != ast->stmt.stmt) {
					fputesc(out, esc, "%I}, {", indent, list);
				}
				//~ fputesc(out, NULL, FMT_COMMENT, list);
				jsonDumpAst(out, esc, list, NULL, indent + 1);
			}
			fputesc(out, esc, "%I}],\n", indent);
			break;
		}

		case STMT_if:
			jsonDumpAst(out, esc, ast->stmt.test, KEY_TEST, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.stmt, KEY_THEN, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.step, KEY_ELSE, indent + 1);
			break;

		case STMT_for:
			jsonDumpAst(out, esc, ast->stmt.init, KEY_INIT, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.test, KEY_TEST, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.step, KEY_STEP, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.stmt, KEY_STMT, indent + 1);
			break;

		case STMT_con:
		case STMT_brk:
			//fputesc(out, esc, "%I\"%s\": %d,\n", indent, KEY_OFFS, ast->go2.offs);
			//fputesc(out, esc, "%I\"%s\": %d,\n", indent, KEY_ARGS, ast->go2.stks);
			break;

		case STMT_end:
		case STMT_ret:
			jsonDumpAst(out, esc, ast->stmt.stmt, KEY_STMT, indent + 1);
			break;

		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			astn args = ast->op.rhso;
			fputesc(out, esc, "%I\"%s\": [{", indent, KEY_ARGS);
			if (args != NULL) {
				while (args && args->kind == OPER_com) {
					if (args != ast->stmt.stmt) {
						fputesc(out, esc, "%I}, {", indent);
					}
					//~ fputesc(out, NULL, FMT_COMMENT, args->op.rhso);
					jsonDumpAst(out, esc, args->op.rhso, NULL, indent + 1);
					args = args->op.lhso;
				}
				if (args != ast->stmt.stmt) {
					fputesc(out, esc, "%I}, {", indent, args);
				}
				//~ fputesc(out, NULL, FMT_COMMENT, args);
				jsonDumpAst(out, esc, args, NULL, indent + 1);
			}
			fputesc(out, esc, "%I}],\n", indent);
			break;
		}

		case OPER_dot:		// '.'
		case OPER_idx:		// '[]'

		case OPER_adr:		// '&'
		case OPER_pls:		// '+'
		case OPER_mns:		// '-'
		case OPER_cmt:		// '~'
		case OPER_not:		// '!'

		case OPER_add:		// '+'
		case OPER_sub:		// '-'
		case OPER_mul:		// '*'
		case OPER_div:		// '/'
		case OPER_mod:		// '%'

		case OPER_shl:		// '>>'
		case OPER_shr:		// '<<'
		case OPER_and:		// '&'
		case OPER_ior:		// '|'
		case OPER_xor:		// '^'

		case OPER_equ:		// '=='
		case OPER_neq:		// '!='
		case OPER_lte:		// '<'
		case OPER_leq:		// '<='
		case OPER_gte:		// '>'
		case OPER_geq:		// '>='

		case OPER_lnd:		// '&&'
		case OPER_lor:		// '||'
		case OPER_sel:		// '?:'

		case OPER_com:		// ','

		case ASGN_set:		// '='
			jsonDumpAst(out, esc, ast->op.test, KEY_TEST, indent + 1);	// not null for operator ?:
			jsonDumpAst(out, esc, ast->op.lhso, KEY_LHSO, indent + 1);
			jsonDumpAst(out, esc, ast->op.rhso, KEY_RHSO, indent + 1);
			break;

		//#}
		//#{ TVAL
		case EMIT_opc:
			//~ fputesc(out, escape, " />\n", text);
			//~ break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:

		case TYPE_ref:
		case TYPE_def:	// TODO: see dumpxml
			fputesc(out, esc, "%I\"%s\": \"%t\"\n", indent, KEY_VALUE, ast);
			break;

		//#}
	}

	if (kind != NULL) {
		fputesc(out, esc, "%I},\n", indent - 1);
	}
}
static int jsonDumpAsm(customContext ctx, size_t offs, void *ip) {
	static const char* KEY_OFFS = "offs";
	static const char* FMT_COMMENT = "{ // %.9A\n";

	FILE *out = ctx->out;
	int indent = ctx->indent;
	fputesc(out, NULL, FMT_COMMENT, ip);
	fputesc(out, NULL, "%I\"%s\": %u,\n", indent, KEY_OFFS, offs);
	fputesc(out, NULL, "%I}, ", indent);
	return 0;
}
static void jsonDump(customContext ctx, symn sym) {
	static const char **esc = NULL;
	FILE *fout = ctx->out;
	if (esc == NULL) {
		// layzy initialize on first function call
		static const char *esc_js[256];
		memset(esc_js, 0, sizeof(esc_js));
		esc_js['\n'] = "\\n";
		esc_js['\r'] = "\\r";
		esc_js['\t'] = "\\t";
		esc_js['\''] = "\\'";
		esc_js['\"'] = "\\\"";
		esc = esc_js;
	}

	if (sym != ctx->rt->defs) {
		// not the first symbol
		fputfmt(fout, ", ");
	}
	jsonDumpSym(fout, esc, sym, NULL, 1);

	// export syntax tree
	if (0 && ctx->ast_dump != -1 && sym->init) {
		fputfmt(fout, "%Iast :[", 1);
		jsonDumpAst(fout, esc, sym->init, NULL, 1);
		fputfmt(fout, "%I],\n", 1);
	}

	// export assembly
	if (0 && ctx->asm_dump != -1 && sym->call) {
	//~ if (sym->init && sym->call) {
		fputfmt(fout, "%Iasm :[\n", 1);
		iterateAsm(ctx->rt, sym->offs, sym->offs + sym->size, ctx, jsonDumpAsm);
		fputfmt(fout, "%I],\n", 1);
	}

	// export usages
	if (0 && ctx->sym_dump & sym_refs) {
		astn usage;
		int usages = 0;
		int extUsages = 0;
		fputfmt(fout, "%I.references: {\n", 1);
		for (usage = sym->used; usage; usage = usage->ref.used) {
			if (usage->file && usage->line) {
				int referenced = !(usage->file == sym->file && usage->line == sym->line);
				fputfmt(fout, "%I%s:%u: %s as `%+t`\n", 1, usage->file, usage->line, referenced ? "referenced" : "defined", usage);
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
			fputfmt(fout, "%Iexternal references: %d\n", 1, extUsages);
		}
		fputfmt(fout, "%I}\n", 1);
		(void)usages;
	}
	// */
}
static int jsonProfile(customContext ctx, vmError error, size_t ss, void* sp, void* caller, void* callee) {
	if (callee != NULL) {
		FILE *out = ctx->out;
		time_t now = clock();
		const int indent = 0;
		const char **esc = NULL;
		if ((ptrdiff_t)callee < 0) {
			fputfmt(out, "% I% 6d,-1%s\n", ss, now, ss ? ", " : " ");
			if (ss == 0) {
				fputfmt(out, "],\n");
				if (ctx->rt->dbg) {
					int i;
					int covFunc = 0, nFunc = ctx->rt->dbg->functions.cnt;
					int covStmt = 0, nStmt = ctx->rt->dbg->statements.cnt;
					dbgInfo dbg = (dbgInfo) ctx->rt->dbg->functions.ptr;
					fputesc(out, esc, "%I\"%s\": [{\n", indent, "functions");
					for (i = 0; i < nFunc; ++i, dbg++) {
						symn sym = dbg->decl;
						if (dbg->hits == 0) {
							// skip functions not invoked
							continue;
						}
						covFunc += 1;
						if (sym == NULL) {
							sym = mapsym(ctx->rt, dbg->start, 1);
						}
						if (covFunc > 1) {
							fputesc(out, esc, "}, {\n");
						}
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "offs", dbg->start);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "time", dbg->self);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "total", dbg->total);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "hits", dbg->hits);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "fails", dbg->exec - dbg->hits);
						if (dbg->file != NULL && dbg->line > 0) {
							fputesc(out, esc, "%I\"%s\": \"%s\",\n", indent + 1, "file", dbg->file);
							fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "line", dbg->line);
						}
						jsonDumpSym(out, esc, sym, "symbol", indent + 2);
					}

					fputesc(out, esc, "%I}],\n", indent);
					fputesc(out, esc, "%I\"%s\": [{\n", indent, "statements");
					dbg = (dbgInfo) ctx->rt->dbg->statements.ptr;
					for (i = 0; i < nStmt; ++i, dbg++) {
						size_t symOffs = 0;
						symn sym = dbg->decl;
						if (dbg->hits == 0) {
							continue;
						}
						covStmt += 1;
						if (sym == NULL) {
							sym = mapsym(ctx->rt, dbg->start, 1);
						}
						if (sym != NULL) {
							symOffs = dbg->start - sym->offs;
						}
						if (covStmt > 1) {
							fputesc(out, esc, "}, {\n");
						}
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "offs", dbg->start);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "time", dbg->self);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "total", dbg->total);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "hits", dbg->hits);
						fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "fails", dbg->exec - dbg->hits);
						if (dbg->file != NULL && dbg->line > 0) {
							fputesc(out, esc, "%I\"%s\": \"%s\",\n", indent + 1, "file", dbg->file);
							fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "line", dbg->line);
						}
						fputesc(out, esc, "%I\"%s\": \"%?T+%d\"\n", indent + 1, "func", sym, symOffs);
					}
					fputesc(out, esc, "%I}],\n", indent);
					fputesc(out, esc, "\"%s\": %d,\n", "ticksPerSec", CLOCKS_PER_SEC);
					fputesc(out, esc, "\"%s\": %d,\n", "functionCount", ctx->rt->dbg->functions.cnt);
					fputesc(out, esc, "\"%s\": %d\n", "statementCount", ctx->rt->dbg->statements.cnt);

					/*fputesc(out, esc, "%I\"%s\": {\n", indent, "coverage");
					fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "hitFunctions", covFunc);
					fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "allFunctions", nFunc);
					fputesc(out, esc, "%I\"%s\": %d,\n", indent + 1, "hitStatements", covStmt);
					fputesc(out, esc, "%I\"%s\": %d\n", indent + 1, "allStatements", nStmt);
					fputesc(out, esc, "%I}\n", indent);*/
					//~ conDumpProfile(stdout, ctx->rt, 0);
				}
				fputfmt(out, "}\n");
			}
		}
		else {
			size_t offs = vmOffset(ctx->rt, callee);
			if (ss == 0) {
				fputfmt(out, "\"profile\": {\n");
				fputfmt(out, "\"description\": \"callTree array is constructed from a tick(timestamp) followed by a function offset, if the offset is negative it represents a return from a function instead of a call.\",\n");
				fputfmt(out, "\"callTree\": [\n");
			}
			fputfmt(out, "% I% 6d,%d,\n", ss, now, offs);
		}
		(void)ss;
	}
	return 0;
}

static int program(int argc, char* argv[]) {
	char* stdlib = (char*)STDLIB;

	int i, errors = 0;

	// compile, optimize, run, debug, profile, ...
	int gen_code = cgen_info | ol;	// optimize_level, debug_info, ...
	int run_code = -1;	// run, debug, profile

	int sym_dump = -1;	// dump variables and functions.
	int ast_dump = -1;	// dump abstract syntax tree.
	int asm_dump = -1;	// dump assembled code.

	int logAppend = 0;
	char* logf = NULL;		// logger filename
	FILE* dmpf = NULL;		// dump filename
	void (*dump_fun)(customContext, symn) = NULL;

	char* cccf = NULL;		// compile filename
	int warn;

	state rt = rtInit(mem, sizeof(mem));
	//~ state rt = rtInit(NULL, 8 << 20);

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

		//~ if (strncmp(arg, "-mem", 4) == 0) {}	// override heap size

		// logger filename
		else if (strncmp(arg, "-log", 4) == 0) {
			char *arg2 = arg + 4;
			if (++i >= argc || logf) {
				error(rt, NULL, 0, "log file not or double specified");
				return -1;
			}
			logf = argv[i];
			if (*arg2 == '/') {
				while (*arg2) {
					if (*arg2 == '/') {}
					else if (*arg2 == 'a') {
						logAppend = 1;
					}
					else {
						break;
					}
					arg2 += 1;
				}
			}
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}

		// output api, ast, asm
		else if (strncmp(arg, "-api", 4) == 0) {
			char* arg2 = arg + 4;
			if (sym_dump != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			sym_dump = 0xff;
			if (*arg2) {
				arg2 = parsei32(arg2, &sym_dump, 16);
			}
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-ast", 4) == 0) {
			char* arg2 = arg + 4;
			if (ast_dump != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			ast_dump = 0x7f;
			if (*arg2) {
				arg2 = parsei32(arg2, &ast_dump, 16);
			}
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-asm", 4) == 0) {
			char* arg2 = arg + 4;
			if (asm_dump != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
				return -1;
			}
			asm_dump = asm_addr | 0x09;	// use global offset, 9 characters for bytecode hexview
			if (*arg2) {
				arg2 = parsei32(arg2, &asm_dump, 16);
			}
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}

		// output format and filename
		else if (strncmp(arg, "-dump", 5) == 0) {
			if (++i >= argc || dmpf) {
				error(rt, NULL, 0, "dump file not or double specified");
				return -1;
			}
			dmpf = fopen(argv[i], "w");
			if (dmpf == NULL) {
				error(rt, NULL, 0, "error opening dump file: %s", argv[i]);
			}
			if (strcmp(arg, "-dump") == 0) {
				dump_fun = conDump;
			}
			else if (strcmp(arg, "-dump.json") == 0) {
				dump_fun = jsonDump;
			}
			else if (strcmp(arg, "-dump.scite") == 0) {
				dump_fun = dmpSciTEApi;
			}
			else {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
			}
		}

		// run, debug or profile
		else if (strncmp(arg, "-run", 4) == 0) {		// execute code in release mode
			char* arg2 = arg + 4;
			if (run_code != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
			}
			gen_code = ol;
			run_code = 0;
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-debug", 6) == 0) {		// break into debuger on ...
			char *arg2 = arg + 6;
			if (run_code != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
			}
			run_code = brk_fatal | dmp_vars;
			if (*arg2 && *arg2 != '/') {
				arg2 = parsei32(arg2, &gen_code, 10);
			}
			if (*arg2 == '/') {
				while (*arg2) {
					if (*arg2 == '/') {}
					else if (*arg2 == 'a') {
						run_code |= brk_error;
					}
					else if (*arg2 == 's') {
						run_code |= brk_start;
					}
					else {
						break;
					}
					arg2 += 1;
				}
			}
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-profile", 8) == 0) {	// run code profiler
			char *arg2 = arg + 8;
			if (run_code != -1) {
				error(rt, NULL, 0, "argument specified multiple times: %s", arg);
			}
			run_code = dmp_prof | dmp_heap;
			if (*arg2 != '/') {
				while (*arg2) {
					if (*arg2 == '/') {}
					else if (*arg2 == 'v') {
						run_code |= dmp_vars;
					}
					else if (*arg2 == 'h') {
						run_code |= dmp_heap;
					}
					else if (*arg2 == 't') {
						run_code |= dmp_tree;
					}
					else {
						break;
					}
					arg2 += 1;
				}
			}
			if (*arg2) {
				error(rt, NULL, 0, "invalid argument '%s'\n", arg);
				return -1;
			}
		}

		// no more global options
		else break;
	}

	// open log file (global option)
	if (logf && logfile(rt, logf, logAppend) != 0) {
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
	if (!ccAddUnit(rt, install_stdc, wl, stdlib)) {
		error(rt, NULL, 0, "error registering standard library");
		logfile(rt, NULL, 0);
		return -6;
	}

	// intstall file operations.
	if (!ccAddUnit(rt, install_file, wl, NULL)) {
		error(rt, NULL, 0, "error registering file library");
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
	if (errors == 0 && gen_code != -1) {
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

	struct customContext dumpExtra = {
		.breakAt = 0,
		.breakGt = 0,
		.breakLt = 0,
		.sym_dump = sym_dump,
		.ast_dump = ast_dump,
		.asm_dump = asm_dump,
		.run_dump = run_code,	// runtime dump
		.out = dmpf ? dmpf : stdout,
		.indent = 1,
		.rt = rt
	};

	if (sym_dump >= 0 || ast_dump >= 0 || asm_dump >= 0) {
		if (dump_fun == NULL) {
			dump_fun = conDump;
		}
	}

	if (dump_fun == jsonDump) {
		fputfmt(dumpExtra.out, "{");
		fputfmt(dumpExtra.out, "\n\"symbols\": [");
		dump(rt, &dumpExtra, dump_fun);
		fputfmt(dumpExtra.out, "],\n");
	}
	else if (dump_fun != NULL) {
		dump(rt, &dumpExtra, dump_fun);
	}

	// run code if there are no compilation errors.
	if (errors == 0 && run_code != -1) {
		if (rt->dbg != NULL) {
			rt->dbg->context = &dumpExtra;
			// set debugger of profiler
			if (run_code & dmp_prof) {
				// set call tree dump method
				rt->dbg->dbug = (dump_fun == jsonDump) ? &jsonProfile : &conProfile;
			}
			else if (run_code & (brk_fatal|brk_error|brk_start)) {
				rt->dbg->dbug = conDebug;
				if (run_code & brk_start) {
					// break on first instruction
					dumpExtra.breakAt = rt->vm.pc;
					dumpExtra.breakLt = rt->vm.ro;
					dumpExtra.breakGt = rt->vm.px + px_size;
				}
				else {
					// break on invalid instruction
					dumpExtra.breakAt = 0;
					dumpExtra.breakLt = rt->vm.ro;
					dumpExtra.breakGt = rt->vm.px + px_size;
				}
			}
		}
		errors = execute(rt, rt->_size / 4, NULL);
	}

	if (dump_fun == jsonDump) {
		fputfmt(dumpExtra.out, "}\n");
	}
	if (dmpf != NULL) {
		fclose(dmpf);
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
