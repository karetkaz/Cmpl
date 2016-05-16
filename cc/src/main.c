/* command line options
application [global options] [local options]...

<global options>:
  -std<file>            specify custom standard library file (empty file name disables std library compilation).

  -mem<int>[kKmMgG]     override memory usage for compiler and runtime(heap)

  -log[*] <file>        set logger for: compilation errors and warnings, runtime debug messages
    /a                  append to the log file

  -dump[?] <file>       set output for: dump(symbols, assembly, abstract syntax tree, coverage, call tree)
    .scite              dump api for scite text editor
    .json               Turn on javascript output.

  -api[*]               dump symbols
    /a                  include builtin symbols
    /d                  dump details
    /p                  dump params
    /u                  dump usages

  -asm[<hex>]           dump assembled code
    /a                  use global address: (@0x003d8c)
    /n                  prefer names over addresses: <main+80>
    /s                  print source code statements

  -ast[<hex>]           ?dump syntax tree

  -run                  run without: debug information, stacktrace, bounds checking, ...

  -debug[*]             run with attached debugger, pausing on uncaught errors and break points
    /s                  pause on startup
    /a                  pause on caught errors
    /l                  print cause of caught errors
    /L                  print stack trace of caught errors
                        profiler options ?
    /g                  dump global variable values
    /G                  dump global variable values including functions
    /h                  dump allocated heap memory chunks
    /H                  dump allocated heap memory chunks including free chunks
    /p                  dump function statistics
    /P                  dump function statistics including statement stats

  -profile[*]           run code with profiler: coverage, method tracing
    /g                  dump global variable values
    /G                  dump global variable values including functions
    /h                  dump allocated heap memory chunks
    /H                  dump allocated heap memory chunks including free chunks
    /p                  dump function statistics
    /P                  dump function statistics including statement stats

<local options>: filename followed by swithes
  <file>                  if file extension is (.so|.dll) load as library else compile

  -w[a|x|<hex>]           set or disable warning level for current file

  -b[*]<int>              break point on <int> line in current file
    /l                    print only, do not pause
    /L                    print stack trace, do not pause
    /o                    one shot breakpoint, disable after first hit

examples:
>app -dump.json api.json
    dump builtin symbols to `api.json` file (types, functions, variables, aliases)

>app -run test.tracing.ccc
    compile and execute the file `test.tracing.ccc`

>app -debug gl.so -w0 gl.ccc -w0 test.ccc -wx -b12 -b15 -b/t19
    execute in debug mode
    import `gl.so`
        with no warnings
    compile `gl.ccc`
        with no warnings
    compile `test.ccc`
        treating all warnings as errors
        break execution on lines 12 and 15
        print message when line 19 is hit
*/

//~ (wcl386 -cc -q -ei -6s -d0  -fe=../main *.c) && (rm -f *.o *.obj *.err)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "internal.h"

// default values
static const int wl = 15;           // default warning level
static char mem[8 << 20];           // 8MB runtime memory
const char* STDLIB = "stdlib.cvx";  // standard library

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

static inline int streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}
static inline int strbeg(const char *a, const char *b) {
	return strncmp(a, b, strlen(b)) == 0;
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
	fputfmt(out, "    -log <file>    logger\n");
	fputfmt(out, "    -dump <file>   output\n");
	fputfmt(out, "    -std<file>     override standard library path\n");
	fputfmt(out,"\n");
	fputfmt(out, "    -run           execute\n");
	fputfmt(out, "    -debug         execute with debugerr\n");
	fputfmt(out, "    -profile       execute with code profiling\n");
	fputfmt(out,"\n");
	fputfmt(out,"    -api[<hex>]     dump symbols\n");
	fputfmt(out,"    -asm[<hex>]     dump assembly\n");
	fputfmt(out,"    -ast[<hex>]     dump syntax tree\n");
	fputfmt(out,"\n");
	fputfmt(out,"  <local options>: filename followed by swithes\n");
	fputfmt(out,"    <filename>      if file extension is (.so|.dll) import else compile\n");
	fputfmt(out,"    -w[a|x|<int>]   set warning level\n");
	fputfmt(out,"    -b<int>         set break point on line\n");
	fputfmt(out,"    -t<int>         set trace point on line\n");
}

static inline int hasAssembly(symn sym) {
	if (sym->call && sym->kind == TYPE_ref) {
		// exclude references to functions.
		if (sym->cast != TYPE_ref) {
			return 1;
		}
	}
	return 0;
}

// forward functions
extern int vmTest();
extern int vmHelp();

enum {
	// break point flags
	dbg_pause = 0x01,
	dbg_print = 0x02,
	dbg_trace = 0x04,
	dbg_1shot = 0x08,
};
struct userContextRec {
	rtContext rt;
	FILE *out;
	int indent;

	int dumpAst;

	unsigned dmpApi:1;      // dump symbols
	unsigned dmpApiAll:1;   // include builtin symbols
	unsigned dmpApiInfo:1;  // dump detailed info
	unsigned dmpApiPrms:1;  // dump parameters
	unsigned dmpApiUsed:1;  // dump usages

	unsigned dmpAsm:1;      // dump disasembly
	unsigned dmpAsmAddr:1;  // use global address: (@0x003d8c)
	unsigned dmpAsmName:1;  // prefer names over addresses: <main+80>
	unsigned dmpAsmStmt:1;  // print source code statements
	//unsigned dmpAsmCode:4;  // print code bytes (0-15)

	// execution stats
	unsigned dmpProf:2;
	unsigned dmpHeap:2;
	unsigned dmpGlob:2;
	unsigned hasOut:1;
	signed padFlags:16;

	// debug
	int breakCaught;    // pause debugger on caught error
	int printCaught;    // print message on caught errors
	int traceCaught;    // print stack trace for caught errors

	size_t breakNext;   // break on executing instruction
	int dbgCommand;     // last debuger command
};

// text output
static void conDumpMem(rtContext rt, void* ptr, size_t size, int used) {
	userContext ctx = rt->dbg->extra;
	if (ctx == NULL) {
		return;
	}
	FILE *out = ctx->out;
	if (!used && ctx->dmpHeap == 1) {
		return;
	}
	fputfmt(out, "!%s chunk @%06x; size: %d\n", used ? "used" : "free", vmOffset(rt, ptr), size);
}
static void conDumpAsm(userContext extra, size_t offs, void* ip) {
	FILE *out = extra->out;

	if (extra->dmpAsmStmt && extra->rt->cc) {
		dbgn dbg = mapDbgStatement(extra->rt, offs);
		if (dbg != NULL && dbg->stmt != NULL && dbg->start == offs) {
			fputfmt(out, "%I%s:%u: %t\n", extra->indent, dbg->file, dbg->line, dbg->stmt);
			//fputFmt(out, NULL, "%I%s:%u: [%06x-%06x): %t\n", extra->indent, dbg->file, dbg->line, dbg->start, dbg->end, dbg->stmt);
		}
	}

	int mode = 9;	// TODO: hardcoded: use 9 bytes of code
	if (extra->dmpAsmAddr) {
		mode |= prAsmAddr;
	}
	if (extra->dmpAsmName) {
		mode |= prAsmAddr | prAsmName;
	}
	//~ fputFmt(out, NULL, "%I%.*A\n", extra->indent, mode, ip);
	fputFmt(out, NULL, "%I", extra->indent);
	fputAsm(out, NULL, extra->rt, ip, mode);
	fputFmt(out, NULL, "\n");
}
static void conDumpRun(userContext cctx) {
	rtContext rt = cctx->rt;
	dbgContext dbg = rt->dbg;
	FILE *out = cctx->out;

	if (cctx->dmpProf != 0) {
		int covFunc = 0, nFunc = dbg->functions.cnt;
		int covStmt = 0, nStmt = dbg->statements.cnt;
		dbgn fun = (dbgn) dbg->functions.ptr;
		int i, all = cctx->dmpProf > 1;

		fputfmt(out, "\n/*-- Profile:\n");

		for (i = 0; i < nFunc; ++i, fun++) {
			symn sym = fun->decl;
			if (fun->hits == 0) {
				continue;
			}
			covFunc += 1;
			if (sym == NULL) {
				sym = rtFindSym(rt, fun->start, 1);
			}
			fputfmt(out,
				"%s:%u:[.%06x, .%06x): <%?T> hits(%D/%D), time(%D%?+D / %.3F%?+.3F ms)\n", fun->file,
				fun->line, fun->start, fun->end, sym, (int64_t) fun->hits, (int64_t) fun->exec,
				(int64_t) fun->total, (int64_t) -(fun->total - fun->self),
				fun->total / (double) CLOCKS_PER_SEC, -(fun->total - fun->self) / (double) CLOCKS_PER_SEC
			);
		}

		if (all) {
			fputfmt(out, "\n//-- statements:\n");
		}
		fun = (dbgn) rt->dbg->statements.ptr;
		for (i = 0; i < nStmt; ++i, fun++) {
			size_t symOffs = 0;
			symn sym = fun->decl;
			if (fun->hits == 0) {
				continue;
			}
			covStmt += 1;
			if (sym == NULL) {
				sym = rtFindSym(rt, fun->start, 1);
			}
			if (sym != NULL) {
				symOffs = fun->start - sym->offs;
			}
			if (all) {
				fputfmt(out,
					"%s:%u:[.%06x, .%06x): <%?T+%d> hits(%D/%D), time(%D%?+D / %.3F%?+.3F ms)\n", fun->file,
					fun->line, fun->start, fun->end, sym, symOffs, (int64_t) fun->hits, (int64_t) fun->exec,
					(int64_t) fun->total, (int64_t) -(fun->total - fun->self),
					fun->total / (double) CLOCKS_PER_SEC, -(fun->total - fun->self) / (double) CLOCKS_PER_SEC
				);
			}
		}

		fputfmt(out, "\n//-- coverage(functions: %.2f%%(%d/%d), statements: %.2f%%(%d/%d))\n",
			covFunc * 100. / nFunc, covFunc, nFunc, covStmt * 100. / nStmt, covStmt, nStmt);

		fputfmt(out, "// */\n");
	}

	if (cctx->dmpGlob != 0) {
		symn var;
		int all = cctx->dmpGlob > 1;
		fputfmt(out, "\n/*-- Globals:\n");
		for (var = rt->vars; var; var = var->next) {
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

			fputVal(out, NULL, rt, var, (stkval *) ofs, prSymQual | prType, 0);
			fputc('\n', out);
		}
		fputfmt(out, "// */\n");
	}

	if (cctx->dmpHeap != 0) {
		// show allocated memory chunks.
		fputfmt(out, "\n/*-- Allocations:\n");
		rtAlloc(rt, NULL, 0, conDumpMem);
		fputfmt(out, "// */\n");
	}
}

static void conDumpApi(userContext extra, symn sym) {
	int dmpAsm = 0;
	int dumpAst = -1;
	int identExt = 0;//extra->indent;
	int dumpExtraData = 0;
	FILE *out = extra->out;

	if (sym == NULL) {
		// last symbol
		return;
	}

	if (extra->dumpAst >= 0 && extra->rt->cc && sym->init != NULL) {
		// dump valid syntax trees only
		dumpAst = extra->dumpAst;
	}
	if (extra->dmpAsm && hasAssembly(sym)) {
		dmpAsm = 1;
	}

	if (sym->file == NULL && sym->line == 0 && !extra->dmpApiAll) {
		return;
	}
	if (!extra->dmpApi && !dmpAsm && dumpAst < 0) {
		return;
	}

	// print qualified name with arguments and type
	fputfmt(out, "%+T: %+T", sym, sym->type);

	// print symbol info (kind, size, offset, ...)
	if (extra->dmpApiInfo) {
		if (!dumpExtraData) {
			fputfmt(out, " {\n");
			dumpExtraData = 1;
		}
		// print symbol definition location
		if (sym->file != NULL && sym->line > 0) {
			fputfmt(out, "%I.definition: %s:%u\n", identExt, sym->file, sym->line);
		}

		fputfmt(out, "%I.kind:%?s%?s %K\n", identExt
			, sym->stat ? " static" : ""
			, sym->cnst ? " const" : ""
			, sym->kind
		);
		if (sym->cast != TYPE_any) {
			fputfmt(out, "%I.casts: %K\n", identExt, sym->cast);
		}
		fputfmt(out, "%I.offset: %06x\n", identExt, sym->offs);
		fputfmt(out, "%I.size: %d\n", identExt, sym->size);
	}

	// explain params of the function
	if (extra->dmpApiPrms) {
		symn param;
		if (!dumpExtraData) {
			fputfmt(out, " {\n");
			dumpExtraData = 1;
		}
		for (param = sym->prms; param; param = param->next) {
			fputfmt(out, "%I.param %.T: %?T (@%06x+%d->%K)\n", identExt, param, param->type, param->offs, param->size, param->cast);
		}
	}

	if (dumpAst >= 0) {
		if (!dumpExtraData) {
			fputfmt(out, " {\n");
			dumpExtraData = 1;
		}
		fputfmt(out, "%I.syntaxTree: %?.*t", identExt, dumpAst, sym->init);
		if (sym->init->kind != STMT_beg) {
			fputfmt(out, "\n");
		}
	}

	// print disassembly of the function
	if (dmpAsm != 0) {
		if (!dumpExtraData) {
			fputfmt(out, " {\n");
			dumpExtraData = 1;
		}
		fputfmt(out, "%I.assembly: [@%06x: %d]\n", identExt, sym->offs, sym->size);
		dumpAsm(extra->rt, sym->offs, sym->offs + sym->size, extra, conDumpAsm);
	}

	// print usages of symbol
	if (extra->dmpApiUsed) {
		astn usage;
		int extUsages = 0;
		if (!dumpExtraData) {
			fputfmt(out, " {\n");
			dumpExtraData = 1;
		}
		fputfmt(out, "%I.references:\n", identExt);
		for (usage = sym->used; usage; usage = usage->ref.used) {
			if (usage->file && usage->line) {
				int referenced = !(usage->file == sym->file && usage->line == sym->line);
				fputfmt(out, "%I%s:%u: %s as `%+t`\n", identExt + 1, usage->file, usage->line, referenced ? "referenced" : "defined", usage);
			}
			else {
				extUsages += 1;
			}
		}
		if (extUsages > 0) {
			fputfmt(out, "%Iexternal references: %d\n", identExt + 1, extUsages);
		}
	}
	if (dumpExtraData) {
		fputfmt(out, "}");
	}
	fputfmt(out, "\n");
}

static int conProfile(dbgContext ctx, size_t ss, void* caller, void* callee, clock_t ticks) {
	if (callee != NULL) {
		userContext cc = ctx->extra;
		if ((ptrdiff_t) callee < 0) {
			fputfmt(cc->out, "% I< %d\n", ss, ticks);
		}
		else {
			rtContext rt = ctx->rt;
			size_t offs = vmOffset(rt, callee);
			fputfmt(cc->out, "% I> %d,0x%06x %?T\n", ss, ticks, offs, rtFindSym(rt, offs, 1));
		}
	}
	(void)caller;
	(void)ticks;
	return 0;
}
static int conDebug(dbgContext dbg, const vmError error, size_t ss, void* sp, void* ip) {
	enum {
		dbgAbort = 'q',		// stop debuging
		dbgResume = 'r',	// break on next breakpoint or error
		dbgStepNext = 'a',	// break on next opcode
		dbgStepLine = 'l',	// break on next line
		dbgStepInto = 'i',	// break on next function call
		dbgStepOut = 'o',	// break on next return
		dbgPrintTrace = 280,	// print stack trace
		dbgPrintOpcode,		// print current opcode
		dbgPrintValues,		// print values on sack
	};

	char buff[1024];
	rtContext rt = dbg->rt;
	userContext usr = dbg->extra;

	FILE *out = stdout;
	int breakMode = 0;
	char *breakCause = NULL;
	size_t offs = vmOffset(rt, ip);
	symn fun = rtFindSym(rt, offs, 0);
	dbgn dbgStmt = mapDbgStatement(rt, offs);

	/*if (callee != NULL && error == noError) {
		int returns = (ptrdiff_t) callee < 0;
		// last return
		if (returns && ss == 0) {
		}
		/ * first call
		else if (ss == 0) {
		}* /

		switch (usr->dbgCommand) {
			// TODO: test implementation: (step in / step out)
			case dbgStepInto:
				if (!returns) {
					usr->breakNext = (size_t)-1;
				}
				break;

			case dbgStepOut:
				if (returns) {
					usr->breakNext = (size_t)-1;
				}
				break;
		}
		return 0;
	}
	else */
	if (error != noError) {
		switch (error) {
			default:
				breakCause = "Unknown error";
				break;

			case noError:
				breakCause = "Breakpoint";
				break;

			case invalidIP:
				breakCause = "Invalid instruction pointer";
				break;

			case invalidSP:
				breakCause = "Invalid stack pointer";
				break;

			case illegalInstruction:
				breakCause = "Invalid instruction";
				break;

			case stackOverflow:
				breakCause = "Stack Overflow";
				break;

			case divisionByZero:
				breakCause = "Division by Zero";
				break;

			case libCallAbort:
				breakCause = "External call abort";
				break;

			case memReadError:
				breakCause = "Access violation reading memory";
				break;

			case memWriteError:
				breakCause = "Access violation writing memory";
				break;
		}
		if (!dbg->checked) {
			// break on uncaught errors (what a debugger should do ;)
			breakMode = dbg_pause;
		}
		else {
			if (usr->breakCaught) {
				breakMode |= dbg_pause;
			}
			if (usr->printCaught) {
				breakMode |= dbg_print;
			}
			if (usr->traceCaught) {
				breakMode |= dbg_trace;
			}
		}
	}

	else if (usr->breakNext == (size_t)-1) {
		breakMode = dbg_pause;
		breakCause = "Break";
	}
	else if (usr->breakNext == offs) {
		breakMode = dbg_pause;
		breakCause = "Break";
	}
	else if (dbgStmt != NULL) {
		if (offs == dbgStmt->start) {
			breakMode = dbgStmt->bp;
			if (breakMode & dbg_pause) {
				breakCause = "Break point";
			}
			if (breakMode & dbg_print) {
				breakCause = "Break point";
			}
			if (breakMode & dbg_trace) {
				breakCause = "Trace point";
			}
			if (breakMode & dbg_1shot) {
				// remove breakpoint
				dbgStmt->bp = 0;
			}
		}
	}

	// print error type
	if (breakMode & (dbg_pause | dbg_print | dbg_trace)) {
		size_t funOffs = offs;
		if (fun != NULL) {
			funOffs = offs - fun->offs;
		}
		// print current file position
		if (dbgStmt != NULL && dbgStmt->file != NULL && dbgStmt->line > 0) {
			fputfmt(out, "%s:%u: ", dbgStmt->file, dbgStmt->line);
		}
		fputfmt(out, "%s in function: <%T+%06x>\n", breakCause, fun, funOffs);
		if (breakMode & dbg_trace) {
			logTrace(dbg, out, 1, 0, 20);
		}
	}

	// pause execution in debuger
	for ( ; breakMode & dbg_pause; ) {
		int cmd = -1;
		char* arg = NULL;
		fputfmt(out, ">dbg[%?c] %.A: ", usr->dbgCommand, ip);
		if (fgets(buff, sizeof(buff), stdin) == NULL) {
			// Can not read from stdin
			fputfmt(out, "can not read from standard input, resuming\n");
			cmd = usr->dbgCommand;
			//return 0;
		}
		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff == 0) {
			// no new command, use last one
			cmd = usr->dbgCommand;
		}
		else if (buff[1] == 0) {
			// one char command
			arg = buff + 1;
			cmd = buff[0];
		}
		else if ((arg = parsecmd(buff, "resume", " "))) {
			if (*arg == 0) {
				cmd = dbgResume;
			}
		}
		else if ((arg = parsecmd(buff, "step", " "))) {
			if (*arg == 0) {	// step == step over
				cmd = dbgStepNext;
			}
			else if (strcmp(arg, "opcode") == 0) {
				cmd = dbgStepNext;
			}
			else if (strcmp(arg, "over") == 0) {
				cmd = dbgStepLine;
			}
			else if (strcmp(arg, "out") == 0) {
				cmd = dbgStepOut;
			}
			else if (strcmp(arg, "in") == 0) {
				cmd = dbgStepInto;
			}
		}

		else if ((arg = parsecmd(buff, "trace", " "))) {
			if (*arg == 0) {
				cmd = dbgPrintTrace;
			}
		}
		else if ((arg = parsecmd(buff, "print", " "))) {
			if (!*arg || streq(arg, "values")) {
				cmd = dbgPrintValues;
			}
			else if (streq(arg, "trace")) {
				cmd = dbgPrintTrace;
			}
			else if (streq(arg, "opcode")) {
				cmd = dbgPrintOpcode;
			}
		}

		switch (cmd) {
			default:
				fputfmt(out, "invalid command '%s'\n", buff);
				break;

			case dbgAbort:
				return executionAborted;

			case dbgResume:
			case dbgStepInto:
			case dbgStepOut:
				usr->breakNext = 0;
				usr->dbgCommand = cmd;
				return 0;

			case dbgStepNext:
				usr->breakNext = (size_t)-1;
				usr->dbgCommand = cmd;
				return 0;

			case dbgStepLine:
				usr->breakNext = dbgStmt ? dbgStmt->end : 0;
				usr->dbgCommand = cmd;
				return 0;

			case dbgPrintTrace:
				logTrace(dbg, out, 1, 0, 20);
				break;

			case dbgPrintOpcode:
				conDumpAsm(usr, vmOffset(rt, ip), ip);
				break;

			case dbgPrintValues:
				if (*arg == 0) {
					// print top of stack
					for (offs = 0; offs < ss; offs++) {
						stkval* v = (stkval*)&((long*)sp)[offs];
						fputfmt(out, "\tsp(%d): {0x%08x, i32(%d), f32(%f), i64(%D), f64(%F)}\n", offs, v->i4, v->i4, v->f4, v->i8, v->f8);
					}
				}
				else {
					symn sym = ccLookupSym(rt->cc, NULL, arg);
					fputfmt(out, "arg:%T", sym);
					if (sym && sym->kind == TYPE_ref && !sym->stat) {
						fputVal(out, NULL, rt, sym, (stkval *) sp, prType, 0);
					}
				}
				break;
		}
	}
	return 0;
}

static void dumpSciTEApi(userContext extra, symn sym) {
	FILE *out = extra->out;
	if (sym == NULL) {
		// last symbol
		return;
	}
	if (sym->prms != NULL && sym->call) {
		fputfmt(out, "%-T", sym);
	}
	else {
		fputfmt(out, "%T", sym);
	}
	fputfmt(out, "\n");
}

// json output
static const char *JSON_KEY_API = "symbols";
static const char *JSON_KEY_RUN = "profile";
static void jsonDumpSym(FILE *out, const char **esc, symn ptr, const char *kind, int indent) {
	//~ static const char* FMT_START = "%I, \"%s\": { \"\": \"%T\"\n";
	static const char* FMT_START = "%I, \"%s\": {\n";
	static const char* FMT_NEXT = "%I}, {\n";
	static const char* FMT_END = "%I}\n";

	static const char* KEY_PROTO = "proto";

	static const char* KEY_KIND = "kind";
	static const char* KEY_FILE = "file";
	static const char* KEY_LINE = "line";
	static const char* KEY_NAME = "name";
	static const char* KEY_DECL = "declaredIn";
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

	if (ptr == NULL) {
		return;
	}
	if (kind != NULL) {
		fputFmt(out, esc, FMT_START, indent - 1, kind, ptr);
	}

	fputFmt(out, esc, "%I\"%s\": \"%-T\"\n", indent, KEY_PROTO, ptr);
	fputFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent, KEY_KIND, ptr->kind);
	fputFmt(out, esc, "%I, \"%s\": \"%.T\"\n", indent, KEY_NAME, ptr);
	if (ptr->decl != NULL) {
		fputFmt(out, esc, "%I, \"%s\": \"%?T\"\n", indent, KEY_DECL, ptr->decl);
	}

	if (ptr->type != NULL) {
		fputFmt(out, esc, "%I, \"%s\": \"%T\"\n", indent, KEY_TYPE, ptr->type);
	}
	if (ptr->file != NULL) {
		fputFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent, KEY_FILE, ptr->file);
	}
	if (ptr->line != 0) {
		fputFmt(out, esc, "%I, \"%s\": %u\n", indent, KEY_LINE, ptr->line);
	}
	if (ptr->init != NULL) {
		//~ fputFmt(out, esc, "%I, \"%s\": \"%+t\"\n", indent, KEY_CODE, ptr->init);
	}

	if (ptr->call && ptr->prms) {
		symn arg;
		fputFmt(out, NULL, "%I, \"%s\": [{\n", indent, KEY_ARGS);
		for (arg = ptr->prms; arg; arg = arg->next) {
			if (arg != ptr->prms) {
				fputFmt(out, NULL, FMT_NEXT, indent);
			}
			//~ fputFmt(out, NULL, FMT_COMMENT, arg, arg->type);
			jsonDumpSym(out, esc, arg, NULL, indent + 1);
		}
		fputFmt(out, NULL, "%I}]\n", indent);
	}
	if (ptr->cast != 0) {
		fputFmt(out, NULL, "%I, \"%s\": \"%K\"\n", indent, KEY_CAST, ptr->cast);
	}
	fputFmt(out, esc, "%I, \"%s\": %u\n", indent, KEY_SIZE, ptr->size);
	fputFmt(out, esc, "%I, \"%s\": %u\n", indent, KEY_OFFS, ptr->offs);
	fputFmt(out, esc, "%I, \"%s\": %s\n", indent, KEY_CONST, ptr->cnst ? VAL_TRUE : VAL_FALSE);
	fputFmt(out, esc, "%I, \"%s\": %s\n", indent, KEY_STAT, ptr->stat ? VAL_TRUE : VAL_FALSE);
	// no parallel symbols!!! fputFmt(out, NULL, "%I, \"%s\": %s\n", indent, KEY_PARLEL, ptr->stat ? VAL_TRUE : VAL_FALSE);
	if (kind != NULL) {
		fputFmt(out, esc, FMT_END, indent - 1);
	}
}
static void jsonDumpAst(FILE *out, const char **esc, astn ast, const char *kind, int indent) {
	static const char* KEY_PROTO = "proto";

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

	if (ast == NULL) {
		return;
	}
	if (kind != NULL) {
		fputFmt(out, esc, "%I, \"%s\": {\n", indent, kind);
	}

	fputFmt(out, esc, "%I\"%s\": \"%t\"\n", indent + 1, KEY_PROTO, ast);

	fputFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent + 1, KEY_KIND, ast->kind);
	if (ast->type != NULL) {
		fputFmt(out, esc, "%I, \"%s\": \"%T\"\n", indent + 1, KEY_TYPE, ast->type);
	}
	if (ast->cst2 != TYPE_any) {
		fputFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent + 1, KEY_CAST, ast->cst2);
	}
	if (ast->file != NULL) {
		fputFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 1, KEY_FILE, ast->file);
	}
	if (ast->line != 0) {
		fputFmt(out, esc, "%I, \"%s\": %u\n", indent + 1, KEY_LINE, ast->line);
	}
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			fatal("%K", ast->kind);
			return;
		//#{ STMT
		case STMT_beg: {
			astn list;
			fputFmt(out, esc, "%I, \"%s\": [{\n", indent + 1, KEY_STMT);
			for (list = ast->stmt.stmt; list; list = list->next) {
				if (list != ast->stmt.stmt) {
					fputFmt(out, esc, "%I}, {\n", indent + 1, list);
				}
				//~ fputFmt(out, NULL, FMT_COMMENT, list);
				jsonDumpAst(out, esc, list, NULL, indent + 1);
			}
			fputFmt(out, esc, "%I}]\n", indent + 1);
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
			//fputFmt(out, esc, "%I, \"%s\": %d\n", indent, KEY_OFFS, ast->go2.offs);
			//fputFmt(out, esc, "%I, \"%s\": %d\n", indent, KEY_ARGS, ast->go2.stks);
			break;

		case STMT_end:
		case STMT_ret:
			jsonDumpAst(out, esc, ast->stmt.stmt, KEY_STMT, indent + 1);
			break;

		//#}
		//#{ OPER
		case OPER_fnc: {	// '()'
			astn args = ast->op.rhso;
			fputFmt(out, esc, "%I, \"%s\": [{\n", indent + 1, KEY_ARGS);
			if (args != NULL) {
				while (args && args->kind == OPER_com) {
					if (args != ast->stmt.stmt) {
						fputFmt(out, esc, "%I}, {\n", indent + 1);
					}
					jsonDumpAst(out, esc, args->op.rhso, NULL, indent + 1);
					args = args->op.lhso;
				}
				if (args != ast->stmt.stmt) {
					fputFmt(out, esc, "%I}, {\n", indent + 1, args);
				}
				jsonDumpAst(out, esc, args, NULL, indent + 1);
			}
			fputFmt(out, esc, "%I}]\n", indent + 1);
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
			//~ fputFmt(out, escape, " />\n", text);
			//~ break;

		case TYPE_int:
		case TYPE_flt:
		case TYPE_str:

		case TYPE_ref:
		case TYPE_def:	// TODO: see dumpxml
			fputFmt(out, esc, "%I, \"%s\": \"%t\"\n", indent + 1, KEY_VALUE, ast);
			break;

		//#}
	}

	if (kind != NULL) {
		fputFmt(out, esc, "%I}\n", indent);
	}
}
static void jsonDumpAsm(userContext ctx, size_t offs, void *ip) {
	static const char* FMT_START = "%I, { \"\": \"%.9A\"\n";
	//static const char* FMT_NEXT = "%I}, { null: \"%.9A\"\n";
	static const char* FMT_END = "%I}\n";

	static const char* KEY_OFFS = "offs";
	static const char* KEY_INSN = "instruction";

	FILE *out = ctx->out;
	int indent = ctx->indent + 1;
	fputFmt(out, NULL, FMT_START, indent, ip);
	fputFmt(out, NULL, "%I, \"%s\": %u\n", indent + 1, KEY_OFFS, offs);
	fputFmt(out, NULL, "%I, \"%s\": \"%.A\"\n", indent + 1, KEY_INSN, ip);
	fputFmt(out, NULL, FMT_END, indent);
}
static void jsonDumpApi(userContext ctx, symn sym) {
	FILE *fout = ctx->out;
	int indent = ctx->indent;

	// lazy initialization of escape characters
	static const char **esc = NULL;
	if (esc == NULL) {
		// layzy initialize on first function call
		static const char *esc_json[256];
		memset(esc_json, 0, sizeof(esc_json));
		esc_json['\n'] = "\\n";
		esc_json['\r'] = "\\r";
		esc_json['\t'] = "\\t";
		//~ esc_json['\''] = "\\'";
		esc_json['\"'] = "\\\"";
		esc = esc_json;
	}

	if (!ctx->dmpApi && !ctx->dmpAsm && ctx->dumpAst < 0) {
		return;
	}

	if (sym == ctx->rt->vars) {
		// first symbol
		fputFmt(fout, esc, "\n%I\"%s\": [{\n", indent, JSON_KEY_API);
		ctx->hasOut = 1;
	}
	else if (sym != NULL) {
		// not the first symbol
		fputfmt(fout, "%I}, {\n", indent);
	}
	else {
		// last symbol
		fputfmt(fout, "%I}]", indent);
		return;
	}

	jsonDumpSym(fout, esc, sym, NULL, indent + 1);

	// export syntax tree
	if (ctx->dumpAst != -1 && sym->init) {
		jsonDumpAst(fout, esc, sym->init, "ast", indent + 1);
	}

	// export assembly
	if (ctx->dmpAsm && hasAssembly(sym)) {
		fputfmt(fout, "%I, \"%s\": [null\n", indent + 1, "asm");
		dumpAsm(ctx->rt, sym->offs, sym->offs + sym->size, ctx, jsonDumpAsm);
		fputfmt(fout, "%I]\n", indent + 1);
	}

	// export usages
	/*if (ctx->dmpApiUsed) {
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
	}*/
}

static int jsonProfile(dbgContext ctx, size_t ss, void* caller, void* callee, clock_t ticks) {
	static const int prettyPrint = 1;
	if (callee != NULL) {
		userContext cc = ctx->extra;
		FILE *out = cc->out;
		const int indent = cc->indent;
		const char **esc = NULL;
		if ((ptrdiff_t)callee < 0) {
			if (prettyPrint) {
				fputfmt(out, "\n% I%d,-1%s", ss, ticks, ss ? "," : "");
			}
			else {
				fputfmt(out, "%d,-1%s", ticks, ss ? "," : "");
			}
			if (ss == 0) {
				int i;
				int covFunc = 0, nFunc = ctx->functions.cnt;
				int covStmt = 0, nStmt = ctx->statements.cnt;
				dbgn dbg = (dbgn) ctx->functions.ptr;

				fputfmt(out, "]\n");

				fputFmt(out, esc, "%I, \"%s\": [{\n", indent + 1, "functions");
				for (i = 0; i < nFunc; ++i, dbg++) {
					symn sym = dbg->decl;
					if (dbg->hits == 0) {
						// skip functions not invoked
						continue;
					}
					covFunc += 1;
					if (sym == NULL) {
						sym = rtFindSym(ctx->rt, dbg->start, 1);
					}
					if (covFunc > 1) {
						fputFmt(out, esc, "%I}, {\n", indent + 1);
					}
					jsonDumpSym(out, esc, sym, NULL, indent + 2);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "time", dbg->self);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "total", dbg->total);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "hits", dbg->hits);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "fails", dbg->exec - dbg->hits);
					if (dbg->file != NULL && dbg->line > 0) {
						fputFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 2, "file", dbg->file);
						fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "line", dbg->line);
					}
					//~ fputFmt(out, esc, "\n");
				}
				fputFmt(out, esc, "%I}]\n", indent + 1);

				fputFmt(out, esc, "%I, \"%s\": [{\n", indent + 1, "statements");
				dbg = (dbgn) ctx->statements.ptr;
				for (i = 0; i < nStmt; ++i, dbg++) {
					size_t symOffs = 0;
					symn sym = dbg->decl;
					if (dbg->hits == 0) {
						continue;
					}
					covStmt += 1;
					if (sym == NULL) {
						sym = rtFindSym(ctx->rt, dbg->start, 1);
					}
					if (sym != NULL) {
						symOffs = dbg->start - sym->offs;
					}
					if (covStmt > 1) {
						fputFmt(out, esc, "%I}, {\n", indent + 1);
					}
					fputFmt(out, esc, "%I\"%s\": \"%?T+%d\"\n", indent + 2, "", sym, symOffs);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "offs", dbg->start);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "time", dbg->self);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "total", dbg->total);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "hits", dbg->hits);
					fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "fails", dbg->exec - dbg->hits);
					if (dbg->file != NULL && dbg->line > 0) {
						fputFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 2, "file", dbg->file);
						fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, "line", dbg->line);
					}
				}
				fputFmt(out, esc, "%I}]\n", indent + 1);

				fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, "ticksPerSec", CLOCKS_PER_SEC);
				fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, "functionCount", ctx->functions.cnt);
				fputFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, "statementCount", ctx->statements.cnt);
				fputfmt(out, "%I}", indent);
			}
		}
		else {
			size_t offs = vmOffset(ctx->rt, callee);
			if (ss == 0) {
				fputFmt(out, esc, "\n%I%s\"%s\": {\n", indent, cc->hasOut ? ", " : "", JSON_KEY_RUN);
				fputFmt(out, esc, "%I\"\": \"%s\"\n", indent + 1,
						"callTree array is constructed from a tick(timestamp) followed by a function offset, if the offset is negative it represents a return from a function instead of a call.");
				fputFmt(out, esc, "%I, \"callTree\": [", indent + 1);
			}
			if (prettyPrint) {
				fputFmt(out, esc, "\n% I%d,%d,", ss, ticks, offs);
			}
			else {
				fputFmt(out, esc, "%d,%d,", ss, ticks, offs);
			}
		}
	}
	(void)caller;
	return 0;
}

static int program(int argc, char* argv[]) {
	struct {

		int32_t logAppend: 1;

		int32_t foldConst: 1;
		int32_t fastInstr: 1;
		int32_t fastAssign: 1;
		int32_t genGlobals: 1;

		int32_t addPointer: 1;
		int32_t addVariant: 1;
		int32_t addObject: 1;
		int32_t addEmit: 1;
		int32_t stdLibs: 1;
		int32_t padStruct: 22;

		int32_t install;
		size_t memory;
	} settings = {
		.logAppend = 0,

		.foldConst = 1,
		.fastInstr = 1,
		.fastAssign = 1,
		.genGlobals = 1,

		.addPointer = (install_def & install_ptr) != 0,
		.addVariant = (install_def & install_var) != 0,
		.addObject = (install_def & install_obj) != 0,
		.addEmit = (install_def & installEmit) != 0,
		.stdLibs = 1,

		.install = install_def,
		.memory = sizeof(mem)
	};

	struct userContextRec extra = {
		.breakNext = 0,
		.breakCaught = 0,
		.printCaught = 0,
		.traceCaught = 0,
		.dbgCommand = 'r',	// last command: resume

		.dmpApi = 0,
		.dmpApiAll = 0,   // include builtin symbols
		.dmpApiInfo = 0,  // dump detailed info
		.dmpApiPrms = 0,  // dump parameters
		.dmpApiUsed = 0,  // dump usages
		.dmpAsm = 0,      // dump disasembly
		.dmpAsmAddr = 0,  // use global address: (@0x003d8c)
		.dmpAsmName = 0,  // prefer names over addresses: <main+80>
		.dmpAsmStmt = 0,  // print source code statements

		.dumpAst = -1,

		.out = stdout,
		.hasOut = 0,
		.indent = 0,

		.rt = NULL
	};

	rtContext rt = NULL;

	char *stdLib = (char*)STDLIB;	// standard library
	char *ccFile = NULL;			// compile filename
	char *logFile = NULL;			// logger filename
	FILE *dumpFile = NULL;			// dump file
	void (*dumpFun)(userContext, symn) = NULL;
	enum {run, debug, profile, skipExecution} run_code = skipExecution;

	int i, warn, errors = 0;

	// TODO: max 32 break points ?
	char* bp_file[32];
	int bp_line[32];
	int bp_type[32];
	int bp_size = 0;

	// porcess global options
	for (i = 1; i < argc; ++i) {
		char* arg = argv[i];

		if (strncmp(arg, "-std", 4) == 0) {		// override stdlib file
			if (stdLib != (char*)STDLIB) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
				return -1;
			}
			if (arg[4] != 0) {
				stdLib = arg + 4;
			}
			else {
				// diasble standard library
				stdLib = NULL;
			}
		}

		else if (strbeg(arg, "-X")) {		// enable or disable settings
			char *arg2 = arg + 2;
			while (*arg2 == '-' || *arg2 == '+') {
				int on = *arg2 == '+';
				if (strbeg(arg2 + 1, "fold")) {
					settings.foldConst = on;
					arg2 += 5;
				}
				else if (strbeg(arg2 + 1, "fast")) {
					settings.fastInstr = on;
					arg2 += 5;
				}
				else if (strbeg(arg2 + 1, "asgn")) {
					settings.fastAssign = on;
					arg2 += 5;
				}
				else if (strbeg(arg2 + 1, "glob")) {
					settings.genGlobals = on;
					arg2 += 5;
				}

				//
				else if (strbeg(arg2 + 1, "emit")) {
					settings.addEmit = on;
					arg2 += 5;
				}
				else if (strbeg(arg2 + 1, "ptr")) {
					settings.addPointer = on;
					arg2 += 4;
				}
				else if (strbeg(arg2 + 1, "var")) {
					settings.addVariant = on;
					arg2 += 4;
				}
				else if (strbeg(arg2 + 1, "obj")) {
					settings.addObject = on;
					arg2 += 4;
				}
				else if (strbeg(arg2 + 1, "std")) {
					settings.stdLibs = on;
					arg2 += 4;
				}
				else {
					break;
				}
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s' at %d\n", arg, arg2 - arg);
				return -1;
			}
		}

		else if (strncmp(arg, "-mem", 4) == 0) {        // override heap size
			int value = -1;
			char* arg2 = arg + 4;
			if (settings.memory != sizeof(mem)) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
				return -1;
			}
			settings.memory = 1;
			if (*arg2) {
				arg2 = parsei32(arg2, &value, 10);
				settings.memory = (size_t) value;
			}
			switch (*arg2) {
				default:
					break;

				case 'k':
				case 'K':
					settings.memory <<= 10;
					if (settings.memory >> 10 != (size_t)value) {
						break;
					}
					arg2 += 1;
					break;

				case 'm':
				case 'M':
					settings.memory <<= 20;
					if (settings.memory >> 20 != (size_t)value) {
						break;
					}
					arg2 += 1;
					break;

				case 'g':
				case 'G':
					settings.memory <<= 30;
					if (settings.memory >> 30 != (size_t)value) {
						break;
					}
					arg2 += 1;
					break;
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}

		// logger filename
		else if (strncmp(arg, "-log", 4) == 0) {
			char *arg2 = arg + 4;
			if (++i >= argc || logFile) {
				fputfmt(stdout, "log file not or double specified");
				return -1;
			}
			logFile = argv[i];
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					case 'a':
						settings.logAppend = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}

		// output api, ast, asm
		else if (strncmp(arg, "-api", 4) == 0) {
			char* arg2 = arg + 4;
			if (extra.dmpApi != 0) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpApi = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;
					case 'a':
						extra.dmpApiAll = 1;
						arg2 += 2;
						break;
					case 'd':
						extra.dmpApiInfo = 1;
						arg2 += 2;
						break;
					case 'p':
						extra.dmpApiPrms = 1;
						arg2 += 2;
						break;
					case 'u':
						extra.dmpApiUsed = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-asm", 4) == 0) {
			char* arg2 = arg + 4;
			if (extra.dmpAsm) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpAsm = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;
					case 's':
						extra.dmpAsmStmt = 1;
						arg2 += 2;
						break;
					case 'a':
						extra.dmpAsmAddr = 1;
						arg2 += 2;
						break;
					case 'n':
						extra.dmpAsmName = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-ast", 4) == 0) {
			char* arg2 = arg + 4;
			if (extra.dumpAst != -1) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dumpAst = 0x7f;
			if (*arg2) {
				arg2 = parsei32(arg2, &extra.dumpAst, 16);
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		// output format and filename
		else if (strncmp(arg, "-dump", 5) == 0) {
			if (++i >= argc || dumpFile) {
				fputfmt(stdout, "dump file not or double specified");
				return -1;
			}
			dumpFile = fopen(argv[i], "w");
			if (dumpFile == NULL) {
				fputfmt(stdout, "error opening dump file: %s", argv[i]);
			}
			if (streq(arg, "-dump.scite")) {
				dumpFun = dumpSciTEApi;
			}
			else if (streq(arg, "-dump.json")) {
				dumpFun = jsonDumpApi;
			}
			else if (streq(arg, "-dump")) {
				dumpFun = conDumpApi;
			}
			else {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
			}
		}

		// run, debug or profile
		else if (strncmp(arg, "-run", 4) == 0) {		// execute code in release mode
			char* arg2 = arg + 4;
			if (run_code != skipExecution) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
			run_code = run;
		}
		else if (strncmp(arg, "-debug", 6) == 0) {		// break into debuger on ...
			char *arg2 = arg + 6;
			if (run_code != skipExecution) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
			}
			run_code = debug;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					// break, print, trace ...
					case 's':
						extra.breakNext = (size_t)-1;
						arg2 += 2;
						break;
					case 'a':
						extra.breakCaught = 1;
						arg2 += 2;
						break;
					case 'l':
						extra.printCaught = 1;
						arg2 += 2;
						break;
					case 'L':
						extra.traceCaught = 1;
						arg2 += 2;
						break;

					// dump stats
					case 'g':
					case 'G':
						extra.dmpGlob = islower(arg2[1]) ? 1 : 2;
						arg2 += 2;
						break;
					case 'h':
					case 'H':
						extra.dmpHeap = islower(arg2[1]) ? 1 : 2;
						arg2 += 2;
						break;
					case 'p':
					case 'P':
						extra.dmpProf = islower(arg2[1]) ? 1 : 2;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-profile", 8) == 0) {	// run code profiler
			char *arg2 = arg + 8;
			if (run_code != skipExecution) {
				fputfmt(stdout, "argument specified multiple times: %s", arg);
			}
			run_code = profile;
			extra.dmpProf = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					// dump stats
					case 'g':
					case 'G':
						extra.dmpGlob = islower(arg2[1]) ? 1 : 2;
						arg2 += 2;
						break;
					case 'h':
					case 'H':
						extra.dmpHeap = islower(arg2[1]) ? 1 : 2;
						arg2 += 2;
						break;
					case 'p':
					case 'P':
						extra.dmpProf = islower(arg2[1]) ? 1 : 2;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fputfmt(stdout, "invalid argument '%s'\n", arg);
				return -1;
			}
		}

		// no more global options
		else break;
	}

	// initialize runtime context
	if (settings.memory > sizeof(mem)) {
		rt = rtInit(NULL, settings.memory);
	}
	else {
		rt = rtInit(mem, settings.memory);
	}

	if (rt == NULL) {
		fatal("initializing runtime context");
		return -1;
	}
	else {
		settings.install &= ~(install_ptr | install_var | install_obj | installEmit);
		settings.install |= install_ptr * settings.addPointer;
		settings.install |= install_var * settings.addVariant;
		settings.install |= install_obj * settings.addObject;
		settings.install |= installEmit * settings.addEmit;

		rt->foldConst = settings.foldConst;
		rt->fastInstr = settings.fastInstr;
		rt->fastAssign = settings.fastAssign;
		rt->genGlobals = settings.genGlobals;
	}

	// open log file (global option)
	if (logFile && logfile(rt, logFile, settings.logAppend) != 0) {
		error(rt, NULL, 0, "can not open log file: `%s`", logFile);
		return -1;
	}

	// intstall base type system.
	if (!ccInit(rt, settings.install, NULL)) {
		error(rt, NULL, 0, "error registering base types");
		logfile(rt, NULL, 0);
		return -6;
	}

	// intstall standard library.
	if (settings.stdLibs) {
		// intstall standard library.
		if (!ccAddUnit(rt, ccUnitStdc, wl, stdLib)) {
			error(rt, NULL, 0, "error registering standard library");
			logfile(rt, NULL, 0);
			return -6;
		}

		// intstall file operations.
		if (settings.stdLibs && !ccAddUnit(rt, ccUnitFile, wl, NULL)) {
			error(rt, NULL, 0, "error registering file library");
			logfile(rt, NULL, 0);
			return -6;
		}
	}

	// compile and import files / modules
	for (warn = -1; i <= argc; ++i) {
		char* arg = argv[i];
		if (i == argc || *arg != '-') {
			if (ccFile != NULL) {
				char* ext = strrchr(ccFile, '.');
				if (ext && (streq(ext, ".so") || streq(ext, ".dll"))) {
					int resultCode = importLib(rt, ccFile);
					if (resultCode != 0) {
						error(rt, NULL, 0, "error(%d) importing library `%s`", resultCode, ccFile);
					}
				}
				else if (!ccDefCode(rt, warn == -1 ? wl : warn, ccFile, 1, NULL)) {
					error(rt, NULL, 0, "error compiling source `%s`", arg);
				}
				ccFile = NULL;
			}
			ccFile = arg;
			warn = -1;
		}
		else {
			if (ccFile == NULL) {
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
			else if (arg[1] == 'b') {
				int line = 0;
				int type = dbg_pause;
				char *arg2 = arg + 2;
				if (bp_size > (int)lengthOf(bp_file)) {
					info(rt, NULL, 0, "can not add more than %d breakponts.", lengthOf(bp_file));
				}
				while (*arg2 == '/') {
					switch (arg2[1]) {
						default:
							arg2 += 1;
							break;

						case 'p': // print only
							type &= ~dbg_pause;
							type |= dbg_print;
							arg2 += 2;
							break;

						case 't': // trace only
							type &= ~dbg_pause;
							type |= dbg_trace;
							arg2 += 2;
							break;

						case 'o': // remove after first hit
							type |= dbg_1shot;
							arg2 += 2;
							break;
					}
				}

				if (*parsei32(arg2, &line, 10)) {
					error(rt, NULL, 0, "invalid line number `%s`", arg + 2);
				}
				bp_file[bp_size] = ccFile;
				bp_line[bp_size] = line;
				bp_type[bp_size] = type;
				bp_size += 1;
			}
			else {
				error(rt, NULL, 0, "invalid option: `%s`", arg);
			}
		}
	}

	errors = rt->errCount;

	extra.rt = rt;
	if (dumpFile != NULL) {
		extra.out = dumpFile;
	}

	// generate code only if needed and there are no compilation errors
	if (errors == 0) {
		if (!gencode(rt, run_code == debug || run_code == profile)) {
			trace("error generating code");
			errors += 1;
		}
		// set breakpoints
		for (i = 0; i < bp_size; ++i) {
			char *file = bp_file[i];
			int line = bp_line[i];
			int type = bp_type[i];
			dbgn dbg = getDbgStatement(rt, file, line);
			if (dbg != NULL) {
				dbg->bp = type;
				//info(rt, NULL, 0, "%s:%u: breakpoint", file, line);
			}
			else {
				info(rt, NULL, 0, "%s:%u: invalid breakpoint", file, line);
			}
		}
	}

	if (dumpFun == NULL) {
		if (extra.dmpApi != 0 || extra.dmpAsm != 0 || extra.dumpAst >= 0) {
			dumpFun = conDumpApi;
		}
	}

	if (dumpFun == jsonDumpApi) {
		fputfmt(extra.out, "{");
	}
	if (dumpFun != NULL) {
		dumpApi(rt, &extra, dumpFun);
	}

	// run code if there are no compilation errors.
	if (errors == 0 && run_code != skipExecution) {
		if (rt->dbg != NULL) {
			rt->dbg->extra = &extra;
			// set debugger of profiler
			if (run_code == debug) {
				rt->dbg->debug = conDebug;
				rt->dbg->profile = NULL;
				if (extra.breakNext != 0) {
					// break on first instruction
					extra.breakNext = rt->vm.pc;
				}
			}
			else if (run_code == profile) {
				rt->dbg->debug = NULL;
				// set call tree dump method
				rt->dbg->profile = (dumpFun == jsonDumpApi) ? &jsonProfile : &conProfile;
			}
		}
		errors = execute(rt, rt->_size / 4, NULL);
		if (dumpFun != jsonDumpApi) {
			conDumpRun(&extra);
		}
	}

	if (dumpFun == jsonDumpApi) {
		fputfmt(extra.out, "\n}\n");
	}
	if (dumpFile != NULL) {
		fclose(dumpFile);
	}

	// release resources
	closeLibs();
	rtClose(rt);

	return errors;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage(*argv);
		return 0;
	}
	if (argc == 2) {
		if (strcmp(argv[1], "-vmTest") == 0) {
			return vmTest();
		}
		if (strcmp(argv[1], "-vmHelp") == 0) {
			return vmHelp();
		}
		if (strcmp(argv[1], "--help") == 0) {
			usage(*argv);
			return 0;
		}
	}
	//setbuf(stdout, NULL);
	//setbuf(stderr, NULL);
	return program(argc, argv);
}
