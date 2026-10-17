#include "type.h"
#include "code.h"
#include "debuger.h"
#include "lexer.h"
#include "compiler.h"
#include "printer.h"
#include <time.h>
#include <limits.h>

// default values
static const char * const CMPL_HOME = "CMPL_HOME";    // Home environment path variable name

/**
 * One row of the CLI usage/help reference.
 *
 * `flag == NULL && desc == NULL`: blank separator line.
 * `flag == NULL && desc != NULL`: a free-text line (already carries its own
 *   leading indentation, if any), printed as-is: section headers, the
 *   preamble, and the free-form "examples" walkthrough.
 * `flag != NULL`: a flag/sub-option row (`flag` already carries its own
 *   leading indentation: two spaces for a top-level flag, four for a
 *   sub-option), printed left-justified to a fixed column so the
 *   description lines up; `sub` distinguishes a sub-option (1) of the
 *   preceding top-level flag (0) for the structured "usage" JSON dump.
 *
 * This single table is the source of truth for both the `--help` console
 * output (usage(), below) and the `-dump.json`'s `"usage"` array
 * (dumpUsageJSON(), below) -- no more hand-kept-in-sync duplicate of this
 * reference in `extras/docs/700 - Usage.md`.
 */
static const struct {
	const char *flag;
	const char *desc;
	int sub;
} usage_opts[] = {
	{NULL, NULL, 0},
	{NULL, "<global options>:", 0},
	{NULL, NULL, 0},
	{"  -run[*]", "run at full speed, no debug information available", 0},
	{"    /g /G", "dump global variable values (/G includes builtin types and functions)", 1},
	{"    /m /M", "dump memory usage (/M includes heap allocations)", 1},
	{"    /t", "add debug information for stack-traces", 1},
	{NULL, NULL, 0},
	{"  -debug[*]", "run with attached debugger, pausing on uncaught errors and break points", 0},
	{"    /g /G", "dump global variable values (/G includes builtin types and functions)", 1},
	{"    /m /M", "dump memory usage (/M includes heap allocations)", 1},
	{"    /p /P", "dump caught errors (/P includes stacktrace)", 1},
	{"    /t /T", "dump executed instructions (/T includes values on stack)", 1},
	{"    /s /S", "pause on any error (/S pauses on startup)", 1},
	{NULL, NULL, 0},
	{"  -profile[*]", "run code with profiler: coverage, method tracing", 0},
	{"    /g /G", "dump global variable values (/G includes builtin types and functions)", 1},
	{"    /m /M", "dump memory usage (/M includes heap allocations)", 1},
	{"    /p /P", "dump executed statements (/P include all functions and statements)", 1},
	{"    /t /T", "trace execution with timestamps (/T includes instructions)", 1},
	{NULL, NULL, 0},
	{"  -std<file>", "specify custom standard library file (empty file name disables std library compilation).", 0},
	{NULL, NULL, 0},
	{"  -mem<int>[kKmMgG]", "override memory usage for compiler and runtime(heap)", 0},
	{NULL, NULL, 0},
	{"  -log[*]<int> <file>", "set logger for: compilation errors and warnings, runtime debug messages", 0},
	{"    <int>", "set the default log(warning) level 0 - 15", 1},
	{"    /a", "append to the log file", 1},
	{"    /d", "dump to the log file", 1},
	{NULL, NULL, 0},
	{"  -dump[?] <file>", "set output for: dump(symbols, assembly, abstract syntax tree, coverage, call tree)", 0},
	{"    .scite", "dump api for SciTE text editor", 1},
	{"    .json", "dump api and profile data in javascript object notation format", 1},
	{"    .md", "dump documentation in markdown format", 1},
	{NULL, NULL, 0},
	{"  -api[*]", "dump symbols", 0},
	{"    /a /A", "include all library symbols(/A includes builtins)", 1},
	{"    /d", "dump details of symbol", 1},
	{"    /p", "dump params and fields", 1},
	{NULL, NULL, 0},
	{"  -doc[*]", "dump documentation", 0},
	{"    /a /A", "include all library symbols(/A includes builtins)", 1},
	{"    /d", "dump details of symbol", 1},
	{"    /p", "dump params and fields", 1},
	{NULL, NULL, 0},
	{"  -use[*]", "dump usages", 0},
	{"    /a /A", "include all library symbols(/A includes builtins)", 1},
	{"    /d", "dump details of symbol", 1},
	{"    /p", "dump params and fields", 1},
	{NULL, NULL, 0},
	{"  -asm[*]<int>", "dump assembled code: jmp +80", 0},
	{"    /a /A", "include all library symbols(/A includes builtins)", 1},
	{"    /d", "dump details of symbol", 1},
	{"    /p", "dump params and fields", 1},
	{"    /g", "use global address: jmp @0x003d8c", 1},
	{"    /n", "prefer names over addresses: jmp <main+80>", 1},
	{"    /s", "print source code statements", 1},
	{NULL, NULL, 0},
	{"  -ast[*]", "dump syntax tree", 0},
	{"    /a /A", "include all library symbols(/A includes builtins)", 1},
	{"    /d", "dump details of symbol", 1},
	{"    /p", "dump params and fields", 1},
	{"    /t", "dump sub-expression type information", 1},
	{"    /l", "do not expand statements (print on single line)", 1},
	{"    /b", "don't keep braces ('{') on the same line", 1},
	{"    /e", "don't keep `else if` constructs on the same line", 1},
	{NULL, NULL, 0},
	{NULL, "<files with options>: filename followed by switches", 0},
	{"  <file>", "if file extension is (.so|.dll|.wasm) load as library else compile", 0},
	{"  -w[a|<int>]", "set or disable warning level for current file", 0},
	{"  -b[*]<int>", "break point on <int> line in current file", 0},
	{"    /[p|P]", "print only, do not pause (/P includes stack trace)", 1},
	{"    /[v]", "after execution evaluate the top of the stack (partially implemented)", 1},
	{"    /o", "one shot breakpoint, disable after first hit", 1},
	{NULL, NULL, 0},
	{NULL, "examples:", 0},
	{NULL, NULL, 0},
	{NULL, ">app test.cmpl", 0},
	{NULL, "    compile only the file `test.cmpl`, no global option (run, debug or profile) is specified", 0},
	{NULL, NULL, 0},
	{NULL, ">app -run test.cmpl", 0},
	{NULL, "    compile and execute the file `test.cmpl` at full speed", 0},
	{NULL, NULL, 0},
	{NULL, ">app -debug test.cmpl", 0},
	{NULL, "    compile and execute the file `test.cmpl` with additional checks", 0},
	{NULL, "    in case of an error, the debugger pauses the execution at the fault location", 0},
	{NULL, NULL, 0},
	{NULL, ">app -profile test.cmpl", 0},
	{NULL, "    compile, execute and measure the file `test.cmpl`", 0},
	{NULL, "    when the execution finishes, the collected data is dumped to the console, or to the specified file", 0},
	{NULL, NULL, 0},
	{NULL, ">app -debug gl.so -w0 gl.cmpl -w0 test.cmpl -wa -b12 -b/o15 -b/p19", 0},
	{NULL, "    execute in debug mode", 0},
	{NULL, "    import `gl.so`", 0},
	{NULL, "        with no warnings", 0},
	{NULL, "    compile `gl.cmpl`", 0},
	{NULL, "        with no warnings", 0},
	{NULL, "    compile `test.cmpl`", 0},
	{NULL, "        show all warnings", 0},
	{NULL, "        break execution each time line 12 is hit", 0},
	{NULL, "        break execution when line 15 is first hit", 0},
	{NULL, "        print message when line 19 is hit", 0},
	{NULL, NULL, 0},
	{NULL, ">app -profile/t -api/a/d/p -dump.json test.json test.cmpl", 0},
	{NULL, "    execute in profile mode, including the call tree in the dump", 0},
	{NULL, "    include all symbols with details, parameters and fields in the dump", 0},
	{NULL, "    dump in json format to the file `test.json`, which can be opened with the web inspector", 0},
	{NULL, NULL, 0},
	{NULL, ">app -api -dump.json api.json", 0},
	{NULL, "    dump builtin symbols to `api.json` file (types, functions, variables, aliases)", 0},
	{NULL, NULL, 0},
	{NULL, ">app -doc/a -dump.md Cmpl.md cmplDoc/Cmpl.cmpl", 0},
	{NULL, "    generate this documentation in markdown format to the file `Cmpl.md`", 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
};

static inline int strEquals(const char *str, const char *with) {
	if (str == NULL || with == NULL) {
		return str == with;
	}
	return strcmp(str, with) == 0;
}
static inline int strBegins(const char *str, const char *with) {
	return strncmp(str, with, strlen(with)) == 0;
}

static const char *parseInt(const char *str, int32_t *res, int radix) {
	int64_t result = 0;
	int sign = 1;

	//~ ('+' | '-')?
	switch (*str) {
		case '-':
			sign = -1;
			// fall through
		case '+':
			str += 1;
			// fall through
		default:
			break;
	}

	if (radix == 0) {		// auto detect
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

	*res = (uint32_t) (sign * result);

	return str;
}

static const char **escapeXML() {
	static const char *escape[256] = {0};
	if (escape[0] == NULL) {
		escape[0] = "&#0;";
		escape['\''] = "&apos;";
		escape['"'] = "&quot;";
		escape['&'] = "&amp;";
		escape['<'] = "&lt;";
		escape['>'] = "&gt;";
	}
	return escape;
}
static const char **escapeJSON() {
	static const char *escape[256] = {0};
	if (escape[0] == NULL) {
		escape[0] = "\\u0000";
		escape['\n'] = "\\n";
		escape['\r'] = "\\r";
		escape['\t'] = "\\t";
		escape['\"'] = "\\\"";
		escape['\\'] = "\\\\";
	}
	return escape;
}

// Debug commands
typedef enum {
	dbgAbort = 'q',		// stop debugging
	dbgResume = 'r',	// break on next breakpoint or error

	dbgStepNext = 'a',	// break on next instruction
	dbgStepLine = 'n',	// break on next statement
	dbgStepInto = 'i',	// break on next function
	dbgStepOut = 'o',	// break on next return

	dbgPrintStackTrace = 't',	// print stack trace
	dbgPrintStackValues = 's',	// print values on stack
	dbgPrintInstruction = 'p',	// print current instruction
} dbgMode;

typedef struct callStackItem {
	clock_t time;       // time when the function was invoked
	size_t ss;          // Stack size
	symn fun;
	dbgn dbg;
} *callStackItem;

typedef struct breakPointItem {
	char *file;
	int line;
	int type;
} *breakPointItem;

struct userContextRec {
	rtContext rt;

	// override settings
	const char *std;             // standard library unit
	int warnLevel;               // compile log level
	int raiseLevel;              // runtime log level
	size_t memory;               // memory to use
	clock_t rtTime;              // time of execution

	// debug ...
	FILE *in;                    // debugger input
	dbgMode dbgCommand;          // last debugger command
	brkMode dbgOnError;          // on uncaught exception
	brkMode dbgOnCaught;         // on caught exception
	size_t dbgNextBreak;         // offset of the next break (used for step in, out, over, instruction)
	dbgn dbgNextValue;

	dbgn cachedDebugNode;

	struct arrBuffer callStack;
	struct arrBuffer breakPoints;

	// profile ...
	size_t funcCov;
	size_t funcCnt;
	size_t stmtCov;
	size_t stmtCnt;
	size_t trapCnt;

	// dump & output
	FILE *out;                   // dump file
	int indent;                  // dump indentation
	const char **esc;            // escape characters
	const char *steps;           // dump compilation steps
	dmpMode dmpMode;             // dump flags

	const char *mdFilter;        // markdown dump: filter for symbol or instruction
	const char *mdFile;          // markdown dump: the file of the current library section
	symn mdOwner;                // markdown dump: the record of the current library section
	int mdCount;                 // markdown dump: count of listed library symbols

	unsigned closeOut:1;         // close `out` file.

	// dumps
	unsigned dmpApi:1;           // dump symbols
	unsigned dmpDoc:1;           // dump documentation
	unsigned dmpAsm:1;           // dump instructions
	unsigned dmpAst:1;           // dump abstract syntax tree
	unsigned dmpUse:1;           // dump usages

	unsigned dmpDetails:1;       // dump detailed info
	unsigned dmpParams:1;        // dump parameters and fields
	unsigned dmpAsmStmt:1;       // print source code position/statements
	unsigned hideOffsets:1;      // remove offsets, bytecode, and durations from dumps
	unsigned dmpSymbols:2;       // 0: from file, 1: from unit; 2: all compiled; 3: builtin symbols
	unsigned dmpGlobals:2;       // dump global variable, [function, [typename]] values
	unsigned dmpMemoryUse:2;     // dump memory allocation [used, [free] heap memory chunks]

	unsigned traceMethods:1;     // dump function call and return
	unsigned traceOpcodes:1;     // dump executing instruction
	unsigned traceLocals:1;      // dump the content of the stack
	unsigned traceTime:1;        // dump timestamp before `traceMethods` and `traceOpcodes`
	unsigned profFunctions:1;    // measure function execution times
	unsigned profStatements:1;   // measure statement execution times
	unsigned profNotExecuted:1;  // include not executed functions and statements in the dump

	// optimizations
	unsigned foldConstants:1;
	unsigned foldInstruction:1;
	unsigned genStaticGlobals:1;
	unsigned errPrivateAccess:1;
	unsigned errUninitialized:1;
	unsigned checkArrayBounds:1;
	unsigned runsInTestMode:1;
	unsigned preferNativeCalls:1;
	unsigned genDebugInformation:1;
};

static void dumpAstXML(FILE *out, const char **esc, astn ast, dmpMode mode, int indent, const char *text) {
	if (ast == NULL) {
		return;
	}

	printFmt(out, esc, "%I<%s token=\"%k\"", indent, text, ast->kind);

	if ((mode & (prSymType | prAstType)) != 0) {
		printFmt(out, esc, " type=\"%T\"", ast->type);
		if (ast->type != NULL) {
			printFmt(out, esc, " kind=\"%K\"", ast->type->kind);
		}
	}

	if (ast->kind == STMT_beg && ast->file != NULL && ast->line > 0) {
		printFmt(out, esc, " file=\"%s:%d\"", ast->file, ast->line);
	}

	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return;

		//#{ STATEMENTS
		case STMT_beg:
			printFmt(out, esc, ">\n");
			for (astn list = ast->stmt.stmt; list; list = list->next) {
				dumpAstXML(out, esc, list, mode, indent + 1, "stmt");
			}
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		case STMT_end:
			printFmt(out, esc, " stmt=\"%?t\">\n", ast);
			dumpAstXML(out, esc, ast->stmt.stmt, mode, indent + 1, "expr");
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		case STMT_if:
		case STMT_sif:
			printFmt(out, esc, " stmt=\"%?t\">\n", ast);
			dumpAstXML(out, esc, ast->stmt.test, mode, indent + 1, "test");
			dumpAstXML(out, esc, ast->stmt.stmt, mode, indent + 1, "then");
			dumpAstXML(out, esc, ast->stmt.step, mode, indent + 1, "else");
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		case STMT_for:
			printFmt(out, esc, " stmt=\"%?t\">\n", ast);
			dumpAstXML(out, esc, ast->stmt.init, mode, indent + 1, "init");
			dumpAstXML(out, esc, ast->stmt.test, mode, indent + 1, "test");
			dumpAstXML(out, esc, ast->stmt.step, mode, indent + 1, "step");
			dumpAstXML(out, esc, ast->stmt.stmt, mode, indent + 1, "stmt");
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		case STMT_con:
		case STMT_brk:
			printFmt(out, esc, " stmt=\"%?t\" />\n", ast);
			break;

		case STMT_ret:
			if (ast->ret.value != NULL) {
				printFmt(out, esc, " stmt=\"%?t\">\n", ast);
				dumpAstXML(out, esc, ast->ret.value, mode & ~prSymInit, indent + 1, "expr");
				printFmt(out, esc, "%I</%s>\n", indent, text);
			} else {
				printFmt(out, esc, " stmt=\"%?t\" />\n", ast);
			}
			break;

		//#}
		//#{ OPERATORS
		case OPER_fnc:        // '()'
			printFmt(out, esc, " value=\"%?t\">\n", ast);
			for (astn list = chainArgs(ast->op.rhso); list != NULL; list = list->next) {
				dumpAstXML(out, esc, list, mode & ~prSymInit, indent + 1, "push");
			}
			dumpAstXML(out, esc, ast->op.lhso, mode & ~prSymInit, indent + 1, "call");
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		case OPER_dot:        // '.'
		case OPER_idx:        // '[]'

		case OPER_adr:        // '&'
		case PNCT_dot3:       // '...'
		case OPER_pls:        // '+'
		case OPER_mns:        // '-'
		case OPER_cmt:        // '~'
		case OPER_not:        // '!'

		case OPER_add:        // '+'
		case OPER_sub:        // '-'
		case OPER_mul:        // '*'
		case OPER_div:        // '/'
		case OPER_mod:        // '%'

		case OPER_shl:        // '>>'
		case OPER_shr:        // '<<'
		case OPER_and:        // '&'
		case OPER_ior:        // '|'
		case OPER_xor:        // '^'

		case OPER_ceq:        // '=='
		case OPER_cne:        // '!='
		case OPER_clt:        // '<'
		case OPER_cle:        // '<='
		case OPER_cgt:        // '>'
		case OPER_cge:        // '>='

		case OPER_all:        // '&&'
		case OPER_any:        // '||'

		case OPER_sel:         // '?'
		case OPER_cln:         // ':'

		case OPER_com:        // ','

		case INIT_set:        // '='
		case ASGN_set:        // '='
			printFmt(out, esc, " value=\"%?t\">\n", ast);
			dumpAstXML(out, esc, ast->op.lhso, mode & ~prSymInit, indent + 1, "left");
			dumpAstXML(out, esc, ast->op.rhso, mode & ~prSymInit, indent + 1, "right");
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		//#}
		//#{ VALUES
		case TOKEN_var:
			if (ast->id.link == NULL || (mode & prSymInit) == 0) {
				printFmt(out, esc, " value=\"%?t\"/>\n", ast);
				break;
			}

			printFmt(out, esc, " value=\"%?t\">\n", ast);
			if (isInline(ast->id.link)) {
				break;
			}
			if (ast->id.link->init) {
				dumpAstXML(out, esc, ast->id.link->init, mode, indent + 1, "init");
			}
			if (isTypename(ast->id.link)) {
				// declaration
				for (symn field = ast->id.link->fields; field; field = field->next) {
					dumpAstXML(out, esc, field->tag, mode, indent + 1, "field");
				}
			}
			printFmt(out, esc, "%I</%s>\n", indent, text);
			break;

		case TOKEN_opc:
		case TOKEN_val:
		case RECORD_kwd:
		case INLINE_kwd:
			printFmt(out, esc, " value=\"%?t\" />\n", ast);
			break;

		//#}
	}
}

static inline int canDump(userContext ctx, symn sym) {
	if (ctx->dmpSymbols == 3) {
		// include builtin symbols (everything)
		return 1;
	}

	if (ctx->dmpSymbols == 2) {
		// include external symbols
		return sym->file != NULL && sym->line > 0;
	}

	if (ctx->dmpSymbols == 1) {
		// include internal symbols (current unit)
		return strEquals(sym->unit, ctx->rt->main->unit);
	}

	// include only symbols from the current compiled file
	return strEquals(sym->file, ctx->rt->main->unit);
}

static callStackItem enterMethod(userContext usr, size_t ss, dbgn callee) {
	callStackItem top = addBuff(&usr->callStack, NULL);
	top->time = clock();
	top->fun = callee->fun;
	top->dbg = callee;
	top->ss = ss;

	if (callee->group) {
		// offset of native function calls are never executed
		// increment hit count (call count) here
		callee->hits += 1;
	}
	return top;
}
static clock_t leaveMethod(userContext usr) {
	callStackItem top = popBuff(&usr->callStack, NULL);
	dbgn callee = top->dbg;

	clock_t time = clock();
	int recursive = 0;

	callStackItem data = (callStackItem) usr->callStack.ptr;
	for (size_t i = 0; i < usr->callStack.cnt; i++) {
		if (data[i].fun == callee->fun) {
			recursive = 1;
			break;
		}
	}

	clock_t ticks = time - top->time;
	if (!recursive) {
		callee->total += ticks;
	}

	callee->time += ticks;

	callStackItem caller = topBuff(&usr->callStack);
	if (caller != NULL) {
		caller->dbg->time -= ticks;
	}

	return time;
}
static dbgn nextDbgNode(dbgContext ctx, size_t pc, dbgn hint) {
	dbgn base = (void *)ctx->statements.ptr;
	size_t size = ctx->statements.cnt;

	// still on the same leftmost node (statement with multiple instructions).
	if (hint > base && pc >= hint->start && pc < hint->end && hint[-1].end <= pc) {
		return hint;
	}

	// cold path: full binary search on [0, size).
	size_t lo = 0;
	size_t hi = size;
	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		if (base[mid].end > pc) {
			hi = mid;
		} else {
			lo = mid + 1;
		}
	}
	while (lo < size && base[lo].end > pc) {
		if (base[lo].start <= pc) {
			return base + lo;
		}
		lo += 1;
	}
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ json output
static const char * const JSON_KEY_VERSION = "version";
static const char * const JSON_KEY_SYMBOLS = "symbols";

static const char * const JSON_OBJ_ARR_START = "%I, \"%s\": [{\n";
static const char * const JSON_OBJ_START = "%I, \"%s\": {\n";
static const char * const JSON_ARR_START = "%I, \"%s\": [\n";
static const char * const JSON_OBJ_NEXT = "%I}, {\n";
static const char * const JSON_OBJ_ARR_END = "%I}]\n";
static const char * const JSON_OBJ_END = "%I}\n";
static const char * const JSON_ARR_END = "%I]\n";

static const char * const JSON_KEY_FILE = "file";
static const char * const JSON_KEY_LINE = "line";
static const char * const JSON_KEY_UNIT = "unit";
static const char * const JSON_KEY_NAME = "name";
static const char * const JSON_KEY_OFFS = "offs";
static const char * const JSON_KEY_SIZE = "size";

static void jsonDumpSym(FILE *out, const char **esc, symn ptr, const char *kind, int indent) {
	static const char *const JSON_KEY_PROTO = "";
	static const char *const JSON_KEY_KIND = "kind";
	static const char *const JSON_KEY_CAST = "cast";
	static const char *const JSON_KEY_OWNER = "owner";
	static const char *const JSON_KEY_TYPE = "type";
	static const char *const JSON_KEY_ARGS = "args";
	static const char *const JSON_KEY_CNST = "const";
	static const char *const JSON_KEY_STAT = "static";

	static const char *const JSON_VAL_TRUE = "true";
	static const char *const JSON_VAL_FALSE = "false";

	if (ptr == NULL) {
		return;
	}
	if (kind != NULL) {
		printFmt(out, esc, JSON_OBJ_START, indent - 1, kind, ptr);
	}

	printFmt(out, esc, "%I\"%s\": \"%T\"\n", indent, JSON_KEY_PROTO, ptr);
	if (strEquals(ptr->name, doc_tag_inline) || strEquals(ptr->name, doc_tag_statement)) {
		// synthetic doc-placeholder symbol (see declareDocSymbol, parser.c):
		// distinguishable from a real symbol by "kind":"doc" alone, no new
		// field needed to check.
		printFmt(out, esc, "%I, \"%s\": \"doc\"\n", indent, JSON_KEY_KIND);
	} else {
		printFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent, JSON_KEY_KIND, ptr->kind & MASK_kind);
		printFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent, JSON_KEY_CAST, ptr->kind & MASK_cast);
	}
	printFmt(out, esc, "%I, \"%s\": \"%.T\"\n", indent, JSON_KEY_NAME, ptr);
	if (ptr->owner != NULL) {
		printFmt(out, esc, "%I, \"%s\": \"%T\"\n", indent, JSON_KEY_OWNER, ptr->owner);
	}
	if (ptr->type != NULL) {
		printFmt(out, esc, "%I, \"%s\": \"%T\"\n", indent, JSON_KEY_TYPE, ptr->type);
	}
	if (ptr->file != NULL && ptr->line != 0) {
		printFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent, JSON_KEY_FILE, ptr->file);
		printFmt(out, esc, "%I, \"%s\": %u\n", indent, JSON_KEY_LINE, ptr->line);
	}
	if (ptr->unit != NULL) {
		printFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent, JSON_KEY_UNIT, ptr->unit);
	}

	if (isInvokable(ptr)) {
		printFmt(out, esc, JSON_OBJ_ARR_START, indent, JSON_KEY_ARGS);
		for (symn arg = ptr->params; arg; arg = arg->next) {
			if (arg != ptr->params) {
				printFmt(out, esc, JSON_OBJ_NEXT, indent);
			}
			jsonDumpSym(out, esc, arg, NULL, indent + 1);
		}
		printFmt(out, esc, JSON_OBJ_ARR_END, indent);
	}
	printFmt(out, esc, "%I, \"%s\": %u\n", indent, JSON_KEY_SIZE, ptr->size);
	printFmt(out, esc, "%I, \"%s\": %u\n", indent, JSON_KEY_OFFS, ptr->offs);
	printFmt(out, esc, "%I, \"%s\": %s\n", indent, JSON_KEY_STAT, isStatic(ptr) ? JSON_VAL_TRUE : JSON_VAL_FALSE);
	printFmt(out, esc, "%I, \"%s\": %s\n", indent, JSON_KEY_CNST, !isMutable(ptr) ? JSON_VAL_TRUE : JSON_VAL_FALSE);
	if (kind != NULL) {
		printFmt(out, esc, JSON_OBJ_END, indent - 1);
	}
}
static void jsonDumpAst(FILE *out, const char **esc, astn ast, const char *kind, int indent) {
	static const char *const JSON_KEY_PROTO = "";
	static const char *const JSON_KEY_KIND = "kind";
	static const char *const JSON_KEY_TYPE = "type";
	static const char *const JSON_KEY_STMT = "stmt";
	static const char *const JSON_KEY_INIT = "init";
	static const char *const JSON_KEY_TEST = "test";
	static const char *const JSON_KEY_THEN = "then";
	static const char *const JSON_KEY_STEP = "step";
	static const char *const JSON_KEY_ELSE = "else";
	static const char *const JSON_KEY_ARGS = "args";
	static const char *const JSON_KEY_LHSO = "left";
	static const char *const JSON_KEY_RHSO = "right";
	static const char *const JSON_KEY_VALUE = "value";

	if (ast == NULL) {
		return;
	}
	if (kind != NULL) {
		printFmt(out, esc, JSON_OBJ_START, indent, kind);
	}

	printFmt(out, esc, "%I\"%s\": \"%t\"\n", indent + 1, JSON_KEY_PROTO, ast);

	printFmt(out, esc, "%I, \"%s\": \"%k\"\n", indent + 1, JSON_KEY_KIND, ast->kind);
	if (ast->type != NULL) {
		printFmt(out, esc, "%I, \"%s\": \"%T\"\n", indent + 1, JSON_KEY_TYPE, ast->type);
	}
	if (ast->file != NULL) {
		printFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 1, JSON_KEY_FILE, ast->file);
	}
	if (ast->line != 0) {
		printFmt(out, esc, "%I, \"%s\": %u\n", indent + 1, JSON_KEY_LINE, ast->line);
	}
	switch (ast->kind) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return;

		//#{ STATEMENTS
		case STMT_beg: {
			printFmt(out, esc, JSON_OBJ_ARR_START, indent + 1, JSON_KEY_STMT);
			for (astn list = ast->stmt.stmt; list; list = list->next) {
				if (list != ast->stmt.stmt) {
					printFmt(out, esc, JSON_OBJ_NEXT, indent + 1, list);
				}
				jsonDumpAst(out, esc, list, NULL, indent + 1);
			}
			printFmt(out, esc, JSON_OBJ_ARR_END, indent + 1);
			break;
		}

		case STMT_if:
		case STMT_sif:
			jsonDumpAst(out, esc, ast->stmt.test, JSON_KEY_TEST, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.stmt, JSON_KEY_THEN, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.step, JSON_KEY_ELSE, indent + 1);
			break;

		case STMT_for:
			jsonDumpAst(out, esc, ast->stmt.init, JSON_KEY_INIT, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.test, JSON_KEY_TEST, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.step, JSON_KEY_STEP, indent + 1);
			jsonDumpAst(out, esc, ast->stmt.stmt, JSON_KEY_STMT, indent + 1);
			break;

		case STMT_con:
		case STMT_brk:
			break;

		case STMT_end:
		case STMT_ret:
			jsonDumpAst(out, esc, ast->ret.value, JSON_KEY_STMT, indent + 1);
			break;
		//#}
		//#{ OPERATORS
		case OPER_fnc: {    // '()'
			astn args = chainArgs(ast->op.rhso);
			printFmt(out, esc, JSON_OBJ_ARR_START, indent + 1, JSON_KEY_ARGS);
			while (args != NULL) {
				if (args != ast->op.rhso) {
					printFmt(out, esc, JSON_OBJ_NEXT, indent + 1);
				}
				jsonDumpAst(out, esc, args, NULL, indent + 1);
				args = args->next;
			}
			printFmt(out, esc, JSON_OBJ_ARR_END, indent + 1);
			break;
		}

		case OPER_dot:      // '.'
		case OPER_idx:      // '[]'

		case OPER_adr:      // '&'
		case PNCT_dot3:     // '...'
		case OPER_pls:      // '+'
		case OPER_mns:      // '-'
		case OPER_cmt:      // '~'
		case OPER_not:      // '!'

		case OPER_add:      // '+'
		case OPER_sub:      // '-'
		case OPER_mul:      // '*'
		case OPER_div:      // '/'
		case OPER_mod:      // '%'

		case OPER_shl:      // '>>'
		case OPER_shr:      // '<<'
		case OPER_and:      // '&'
		case OPER_ior:      // '|'
		case OPER_xor:      // '^'

		case OPER_ceq:      // '=='
		case OPER_cne:      // '!='
		case OPER_clt:      // '<'
		case OPER_cle:      // '<='
		case OPER_cgt:      // '>'
		case OPER_cge:      // '>='

		case OPER_all:      // '&&'
		case OPER_any:      // '||'

		case OPER_sel:       // '?'
		case OPER_cln:       // ':'

		case OPER_com:      // ','

		case INIT_set:      // '='
		case ASGN_set:      // '='
			jsonDumpAst(out, esc, ast->op.lhso, JSON_KEY_LHSO, indent + 1);
			jsonDumpAst(out, esc, ast->op.rhso, JSON_KEY_RHSO, indent + 1);
			break;
		//#}
		//#{ VALUES
		case TOKEN_opc:
		case TOKEN_val:
		case TOKEN_var:
			printFmt(out, esc, "%I, \"%s\": \"%t\"\n", indent + 1, JSON_KEY_VALUE, ast);
			break;
		//#}
	}

	if (kind != NULL) {
		printFmt(out, esc, JSON_OBJ_END, indent);
	}
}
static void jsonDumpAsm(FILE *out, const char **esc, symn sym, rtContext ctx, int indent) {
	static const char *const JSON_KEY_OPC = "instruction";
	static const char *const JSON_KEY_CODE = "code";

	size_t end = sym->offs + sym->size;
	for (size_t pc = sym->offs, n = pc; pc < end; pc = n) {
		unsigned char *ip = nextOpc(ctx, &n, NULL);
		if (ip == NULL) {
			// invalid instruction
			break;
		}

		if (pc != sym->offs) {
			printFmt(out, esc, JSON_OBJ_NEXT, indent, indent);
		}
		printFmt(out, esc, "%I\"%s\": \"%.A\"\n", indent + 1, JSON_KEY_OPC, ip);
		printFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 1, JSON_KEY_NAME, opcode_tbl[*ip].name);
		printFmt(out, esc, "%I, \"%s\": \"0x%02x\"\n", indent + 1, JSON_KEY_CODE, opcode_tbl[*ip].code);
		printFmt(out, esc, "%I, \"%s\": %u\n", indent + 1, JSON_KEY_OFFS, pc);
		printFmt(out, esc, "%I, \"%s\": %u\n", indent + 1, JSON_KEY_SIZE, opcode_tbl[*ip].size);
	}
}

static void dumpJsonOpcodes(FILE *out, const char **esc, int indent) {
	static const char *const JSON_KEY_OPCODES = "opcodes";
	static const char *const JSON_KEY_NAME = "name";
	static const char *const JSON_KEY_CODE = "code";
	static const char *const JSON_KEY_SIZE = "size";
	static const char *const JSON_KEY_STACK_IN = "stack_in";
	static const char *const JSON_KEY_STACK_OUT = "stack_out";

	printFmt(out, esc, JSON_OBJ_ARR_START, indent, JSON_KEY_OPCODES);
	for (int i = 0; i < 256; ++i) {
		const struct opcodeRec *op = &opcode_tbl[i];
		if (op->name == NULL) {
			continue;
		}
		printFmt(out, esc, JSON_OBJ_NEXT, indent + 1);
		printFmt(out, esc, "%I\"%s\": \"%s\"\n", indent + 2, JSON_KEY_NAME, op->name);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 2, JSON_KEY_CODE, op->code);
		printFmt(out, esc, "%I, \"%s\": %u\n", indent + 2, JSON_KEY_SIZE, op->size);
		printFmt(out, esc, "%I, \"%s\": %u\n", indent + 2, JSON_KEY_STACK_IN, op->stack_in);
		printFmt(out, esc, "%I, \"%s\": %u\n", indent + 2, JSON_KEY_STACK_OUT, op->stack_out);
	}
	printFmt(out, esc, JSON_OBJ_ARR_END, indent + 1);
}
static void dumpJsonUsage(FILE *out, const char **esc, int indent) {
	static const char *const JSON_KEY_USAGE = "usage";
	static const char *const JSON_KEY_FLAG = "flag";
	static const char *const JSON_KEY_DESC = "desc";
	static const char *const JSON_KEY_SUBFLAGS = "subflags";

	// nested (flag -> subflags[]) structure doesn't fit the flat-list
	// ARR_START/NEXT/END convention used elsewhere in this file (that
	// convention closes-and-reopens exactly one brace per entry; here each
	// top-level flag needs to close its subflags array *and* its own
	// object before the next one can open), so brackets are managed
	// explicitly instead.
	printFmt(out, esc, "%I, \"%s\": [\n", indent, JSON_KEY_USAGE);
	int open = 0;
	int subOpen = 0;
	for (size_t i = 0; i < lengthOf(usage_opts); ++i) {
		const char *flag = usage_opts[i].flag;
		const char *desc = usage_opts[i].desc;
		if (flag == NULL) {
			continue;
		}
		if (!usage_opts[i].sub) {
			if (open) {
				if (subOpen) {
					printFmt(out, esc, "%I}\n", indent + 3);
				}
				printFmt(out, esc, "%I]\n", indent + 2);
				printFmt(out, esc, "%I}, {\n", indent + 1);
			} else {
				printFmt(out, esc, "%I{\n", indent + 1);
			}
			open = 1;
			subOpen = 0;
			printFmt(out, esc, "%I\"%s\": \"%?s\"\n", indent + 2, JSON_KEY_FLAG, flag);
			printFmt(out, esc, "%I, \"%s\": \"%?s\"\n", indent + 2, JSON_KEY_DESC, desc);
			printFmt(out, esc, "%I, \"%s\": [\n", indent + 2, JSON_KEY_SUBFLAGS);
		} else if (open) {
			printFmt(out, esc, "%I%s{\n", indent + 3, subOpen ? "}, " : "");
			subOpen = 1;
			printFmt(out, esc, "%I\"%s\": \"%?s\"\n", indent + 4, JSON_KEY_FLAG, flag);
			printFmt(out, esc, "%I, \"%s\": \"%?s\"\n", indent + 4, JSON_KEY_DESC, desc);
		}
	}
	if (open) {
		if (subOpen) {
			printFmt(out, esc, "%I}\n", indent + 3);
		}
		printFmt(out, esc, "%I]\n", indent + 2);
		printFmt(out, esc, "%I}\n", indent + 1);
	}
	printFmt(out, esc, "%I]\n", indent);
}
static void dumpJsonApi(userContext ctx, symn sym) {
	static const char *const JSON_KEY_DOC = "doc";
	static const char *const JSON_KEY_ASM = "asm";
	static const char *const JSON_KEY_AST = "ast";

	FILE *out = ctx->out;
	int indent = ctx->indent;
	const char **esc = ctx->esc;

	// symbols are always accessible
	int dmpApi = ctx->dmpApi;
	// dump documentation if requested
	int dmpDoc = ctx->dmpDoc && sym->doc;
	// instructions are available only for functions
	int dmpAsm = ctx->dmpAsm && isFunction(sym);
	// syntax tree is unavailable at runtime(compiler context is destroyed)
	int dmpAst = ctx->dmpAst && sym->init && ctx->rt->cc;

	if (!dmpApi && !dmpDoc && !dmpAsm && !dmpAst) {
		// nothing to dump
		return;
	}

	if (!canDump(ctx, sym)) {
		return;
	}

	if (sym != ctx->rt->main->fields) {
		// not the first symbol
		printFmt(out, esc, JSON_OBJ_NEXT, indent);
	}

	jsonDumpSym(out, esc, sym, NULL, indent + 1);

	// export documentation
	if (dmpDoc) {
		printFmt(out, esc, "%I, \"%s\": \"%?s\"\n", indent + 1, JSON_KEY_DOC, sym->doc);
	}

	// export valid syntax tree if we still have compiler context
	if (dmpAst) {
		jsonDumpAst(out, esc, sym->init, JSON_KEY_AST, indent + 1);
	}

	// export assembly
	if (dmpAsm) {
		printFmt(out, esc, JSON_OBJ_ARR_START, indent + 1, JSON_KEY_ASM);
		jsonDumpAsm(out, esc, sym, ctx->rt, indent + 1);
		printFmt(out, esc, JSON_OBJ_ARR_END, indent + 1);
	}
}

static vmError jsonProfile(dbgContext ctx, vmError error, size_t pc, size_t ss, void *sp, int mode) {
	rtContext rt = ctx->rt;
	userContext usr = rt->usr;

	FILE *out = usr->out;
	const char **esc = usr->esc;

	dbgn dbg = nextDbgNode(ctx, pc, usr->cachedDebugNode);
	usr->cachedDebugNode = dbg;

	if (error != noError) {
		callStackItem top = topBuff(&usr->callStack);
		while (top != NULL && top->ss >= ss) {
			printFmt(out, esc, ",%d,%d,%d", top->time, ctx->usedMem, -1);
			leaveMethod(usr);
			top = topBuff(&usr->callStack);
		}

		dbg->traps += 1;
		return error;
	}

	if (mode == 0) {
		if (pc == dbg->start) {
			// we just started executing a statement
			dbg->hits += 1;
		}

		dbg->exec += 1;
		// just an instruction, not a call or return
		return noError;
	}
	if (mode > 0) {
		callStackItem top = enterMethod(rt->usr, ss, dbg);
		printFmt(out, esc, ",%d,%d,%d", top->time, ctx->usedMem, pc);
	} else {
		clock_t time = leaveMethod(usr);
		printFmt(out, esc, ",%d,%d,%d", time, ctx->usedMem, -1);
	}

	(void) sp;
	return noError;
}
static void jsonPostProfile(dbgContext ctx) {
	if (ctx == NULL || ctx->debug != jsonProfile) {
		// not debugging
		return;
	}

	static const char *const JSON_KEY_FUNC = "functions";
	static const char *const JSON_KEY_STMT = "statements";
	static const char *const JSON_KEY_TIME = "time";
	static const char *const JSON_KEY_TOTAL = "total";
	static const char *const JSON_KEY_EXEC = "exec";
	static const char *const JSON_KEY_HITS = "hits";
	static const char *const JSON_KEY_FAILS = "fails";

	userContext usr = ctx->rt->usr;
	FILE *out = usr->out;
	const int indent = usr->indent;
	const char **esc = usr->esc;

	size_t covFunc = 0, nFunc = 0;
	size_t covStmt = 0, nStmt = 0;

	// end of call-tree, leave `main` function
	printFmt(out, esc, JSON_ARR_END, indent);

	printFmt(out, esc, JSON_OBJ_ARR_START, indent, JSON_KEY_FUNC);
	symn lastFun = NULL;
	size_t dataCnt = ctx->statements.cnt;
	dbgn data = (dbgn) ctx->statements.ptr;
	for (size_t i = 0; i < ctx->statements.cnt; ++i) {
		dbgn dbg = data + i;
		if (dbg->start != dbg->fun->offs) {
			// not a function
			continue;
		}
		nFunc += 1;

		symn fun = dbg->fun;
		if (dbg->hits == 0) {
			// skip functions not invoked
			continue;
		}
		if (fun == lastFun) {
			// skip functions not invoked
			continue;
		}
		covFunc += 1;

		dbgn firstStmt = dbg;
		// function start might not be the first statement in the list, find it
		for (size_t j = i; j < dataCnt; ++j) {
			dbgn d = data + j;
			if (d->fun != fun) {
				break;
			}
			if (d->group != 0) {
				// ignore groups
				continue;
			}
			if (d->start == fun->offs) {
				firstStmt = d;
			}
		}
		lastFun = fun;

		if (covFunc > 1) {
			printFmt(out, esc, JSON_OBJ_NEXT, indent);
		}
		jsonDumpSym(out, esc, fun, NULL, indent + 1);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_HITS, firstStmt->hits);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TIME, firstStmt->time);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TOTAL, firstStmt->total);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_FAILS, firstStmt->traps);
	}
	printFmt(out, esc, JSON_OBJ_ARR_END, indent);

	printFmt(out, esc, JSON_OBJ_ARR_START, indent, JSON_KEY_STMT);
	for (size_t i = 0; i < dataCnt; ++i) {
		dbgn dbg = data + i;
		nStmt += 1;
		size_t symOffs = 0;
		symn sym = dbg->fun;
		if (dbg->hits == 0) {
			continue;
		}
		covStmt += 1;
		if (sym == NULL) {
			sym = rtLookup(ctx->rt, dbg->start, 0);
		}
		if (sym != NULL) {
			symOffs = dbg->start - sym->offs;
		}
		if (covStmt > 1) {
			printFmt(out, esc, JSON_OBJ_NEXT, indent);
		}
		printFmt(out, esc, "%I\"%s\": \"%?.T+%d\"\n", indent + 1, "", sym, symOffs);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_OFFS, dbg->start);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_EXEC, dbg->exec);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_HITS, dbg->hits);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TIME, dbg->time);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TOTAL, dbg->total);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_FAILS, dbg->traps);
		if (dbg->file != NULL && dbg->line > 0) {
			printFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 1, JSON_KEY_FILE, dbg->file);
			printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_LINE, dbg->line);
		}
	}
	printFmt(out, esc, JSON_OBJ_ARR_END, indent);

	printFmt(out, esc, "%I, \"%s\": %d\n", indent, "ticksPerSec", CLOCKS_PER_SEC);
	printFmt(out, esc, "%I, \"%s\": %d\n", indent, "functionCount", nFunc);
	printFmt(out, esc, "%I, \"%s\": %d\n", indent, "statementCount", nStmt);
}
static void jsonPreProfile(dbgContext ctx) {
	userContext usr = ctx->rt->usr;
	FILE *out = usr->out;
	const int indent = usr->indent;
	const char **esc = usr->esc;

	printFmt(out, esc, JSON_ARR_START, indent, "callTreeData");
	printFmt(out, esc, "\"%s\", \"%s\", \"%s\"\n",
		"ctTickIndex", "ctHeapIndex", "ctFunIndex"
	);
	printFmt(out, esc, JSON_ARR_END, indent);

	// start of call-tree, enter `main` function
	printFmt(out, esc, JSON_ARR_START, indent, "callTree");
	// todo: remove hack and optionally add the first comma in jsonProfile
	printFmt(out, esc, "0,0,0\n");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ text output
static void textDumpOfs(FILE *out, const char **esc, userContext ctx, symn sym, size_t start, size_t end) {
	size_t size = end - start;
	printFmt(out, esc, "%d byte%?c", size, size > 1 ? 's' : 0);
	if (ctx->hideOffsets) {
		return;
	}
	dmpMode mode = ctx->dmpMode & (prRelOffs|prAbsOffs);
	printFmt(out, esc, ": ");
	printOfs(out, esc, ctx->rt, sym, start, mode);
	printFmt(out, esc, " - ");
	printOfs(out, esc, ctx->rt, sym, end, mode);
}
static void textDumpDbg(FILE *out, const char **esc, userContext ctx, symn sym, dbgn dbg, int indent) {
	dbgn outerStmt = mapDbgStatement(ctx->rt, dbg->start, dbg);
	if (outerStmt && outerStmt->start == dbg->start) {
		textDumpDbg(out, esc, ctx, sym, outerStmt, indent);
	}

	printFmt(out, esc, "%I%?s:%?u: (", indent, dbg->file, dbg->line);
	textDumpOfs(out, esc, ctx, sym, dbg->start, dbg->end);
	printFmt(out, esc, ")");

	if (dbg->stmt != NULL && ctx->rt->cc != NULL) {
		printFmt(out, esc, ": %.*t", prDetail | prOneLine, dbg->stmt);
	}
	printFmt(out, esc, "\n");
}
static void textDumpAsm(FILE *out, const char **esc, size_t offs, userContext ctx, int indent) {
	printFmt(out, esc, "%I", indent);
	printAsm(out, esc, ctx->rt, vmPointer(ctx->rt, offs), (ctx->dmpMode | prOneLine) & ~prSymQual);
	printFmt(out, esc, "\n");
}
static void textDumpMem(dbgContext ctx, void *ptr, size_t size, char *kind) {
	rtContext rt = ctx->rt;
	userContext usr = rt->usr;
	const char **esc = usr->esc;
	FILE *out = usr->out;

	char *unit = "bytes";
	double value = (double)size;

	if (size > (1 << 30)) {
		unit = "Gb";
		value /= 1 << 30;
	}
	else if (size > (1 << 20)) {
		unit = "Mb";
		value /= 1 << 20;
	}
	else if (size > (1 << 10)) {
		unit = "Kb";
		value /= 1 << 10;
	}

	printFmt(out, esc, "memory[%s] @%06x; size: %d(%?.1F %s)\n", kind, vmOffset(rt, ptr), size, value, unit);
}

static void printFields(FILE *out, const char **esc, symn sym, userContext ctx) {
	rtContext rt = ctx->rt;
	for (symn var = sym->fields; var; var = var->next) {
		if (var == rt->main || isInline(var)) {
			// alias and type names.
			continue;
		}
		if (isTypename(var)) {
			dieif(!isStatic(var), ERR_INTERNAL_ERROR);
			printFields(out, esc, var, ctx);
			if (ctx->dmpGlobals < 3) {
				continue;
			}
		}
		if (isFunction(var)) {
			dieif(!isStatic(var), ERR_INTERNAL_ERROR);
			printFields(out, esc, var, ctx);
			if (ctx->dmpGlobals < 2) {
				continue;
			}
		}

		if (!canDump(ctx, var)) {
			continue;
		}

		void *value = NULL;
		if (isStatic(var)) {
			// static variable.
			value = rt->_mem + var->offs;
		}
		else if (sym == rt->main) {
			// HACK: local main variable.
			value = (char *) rt->_end + rt->vm.ss - var->offs;
		}
		else {
			continue;
		}

		if (var->file && var->line) {
			printFmt(out, esc, "%s:%u: ", var->file, var->line);
		}
		printVal(out, esc, rt, var, value, prGlobal, 0);
		printFmt(out, esc, "\n");
	}
}

static void textPostProfile(userContext ctx) {
	static const double CLOCKS_PER_MILLIS = CLOCKS_PER_SEC / 1000.;
	const char **esc = ctx->esc;
	FILE *out = ctx->out;
	const rtContext rt = ctx->rt;
	dbgContext dbg = rt->dbg;
	const char *prefix = ctx->steps ? ctx->steps : "\n---------- ";

	if (dbg != NULL && (ctx->profFunctions || ctx->profStatements || ctx->profNotExecuted)) {
		size_t dataCnt = dbg->statements.cnt;
		dbgn data = (dbgn) dbg->statements.ptr;

		printFmt(out, esc, "%?sProfile functions: %d/%d, statements: %d/%d, coverage: %.2f%%, traps: %d\n",
			prefix, ctx->funcCov, ctx->funcCnt, ctx->stmtCov, ctx->stmtCnt,
			ctx->stmtCov * 100. / (ctx->stmtCnt ? ctx->stmtCnt : 1),
			ctx->trapCnt
		);

		symn lastFun = NULL;
		for (size_t i = 0; i < dataCnt; ++i) {
			dbgn stmt = data + i;
			if (stmt->hits == 0 && !ctx->profNotExecuted) {
				continue;
			}

			symn fun = stmt->fun;
			if (fun == NULL) {
				fatal("can not be null");
				continue;
			}

			if (ctx->profFunctions && fun != lastFun) {
				size_t traps = 0;
				dbgn firstStmt = stmt;
				// function start might not be the first statement in the list, find it
				for (size_t j = i; j < dataCnt; ++j) {
					dbgn d = data + j;
					if (d->fun != fun) {
						break;
					}
					if (d->group != 0) {
						// ignore groups
						continue;
					}
					if (d->start == fun->offs) {
						firstStmt = d;
					}
					traps += d->traps;
				}
				lastFun = fun;

				double timeTotal = firstStmt->total / CLOCKS_PER_MILLIS;
				double timeOther = (firstStmt->total - firstStmt->time) / CLOCKS_PER_MILLIS;

				printFmt(out, esc, "%?s:%?u:[.%06x, .%06x): hits(%D%?+D), time(%.3F%?.3F ms): %?T\n",
					fun->file, fun->line, fun->offs, fun->offs + fun->size, firstStmt->hits, -traps, timeTotal, -timeOther, fun
				);
			}

			if (ctx->profStatements) {
				size_t symOffs = stmt->start - fun->offs;
				if (stmt->group == 0) {
					printFmt(out, esc, "%?s:%?u:[.%06x, .%06x) hits(%D), instructions(%D%?+D): <%?.T+%d>\n",
						stmt->file, stmt->line, stmt->start, stmt->end, stmt->hits, stmt->exec, -stmt->traps, fun, symOffs
					);
				}
				else if (ctx->profNotExecuted) {
					printFmt(out, esc, "%?s:%?u:[.%06x, .%06x) group[%d], instructions(%D%?+D): <%?.T+%d>\n",
						stmt->file, stmt->line, stmt->start, stmt->end, stmt->group, stmt->exec, -stmt->traps, fun, symOffs
					);
				}
			}
		}
	}

	if (ctx->dmpGlobals) {
		printFmt(out, esc, "%?sGlobals:\n", prefix);
		printFields(out, esc, rt->main, ctx);
	}

	if (ctx->dmpMemoryUse > 0) {
		struct dbgContextRec dbgRec = {0};
		if (dbg == NULL) {
			dbgRec.rt = rt;
			dbg = &dbgRec;
		}
		printFmt(out, esc, "%?sMemory usage:\n", prefix);
		textDumpMem(dbg, rt->_mem, rt->_size, "all");
		textDumpMem(dbg, rt->_mem, rt->vm.px + px_size, "used");
		textDumpMem(dbg, rt->_beg, rt->_end - rt->_beg, "heap");
		textDumpMem(dbg, rt->_end - rt->vm.ss, rt->vm.ss, "stack");

		printFmt(out, esc, "%?sUsed memory:\n", prefix);
		textDumpMem(dbg, rt->_mem, rt->vm.ro, "meta");
		textDumpMem(dbg, rt->_mem, rt->vm.cs, "code");
		textDumpMem(dbg, rt->_mem, rt->vm.ds, "data");

		if (ctx->dmpMemoryUse > 1) {
			// show allocated and free memory chunks.
			printFmt(out, esc, "%?sHeap memory:\n", prefix);
			dbg->dbgAlloc = textDumpMem;
			struct nfcContextRec ctx = {0};
			*(rtContext*)&ctx.rt = rt;
			rt->api.alloc(&ctx, NULL, 0);
		}
	}
}

static void dumpApiText(userContext ctx, symn sym) {
	int dumpExtraData = 0;
	const char **esc = ctx->esc;
	FILE *out = ctx->out;
	int indent = ctx->indent;

	// symbols are always accessible
	int dmpApi = ctx->dmpApi;
	// dump documentation if requested
	int dmpDoc = ctx->dmpDoc && sym->doc;
	// instructions are available only for functions
	int dmpAsm = ctx->dmpAsm && isFunction(sym);
	// syntax tree is unavailable at runtime(compiler context is destroyed)
	int dmpAst = ctx->dmpAst && sym->init && ctx->rt->cc;
	// dump usages only if the symbol is used
	int dmpUse = ctx->dmpUse && sym->use && (dmpApi || sym->use != sym->tag);

	if (!dmpApi && !dmpDoc && !dmpAsm && !dmpAst && !dmpUse) {
		// nothing to dump
		return;
	}

	if (!canDump(ctx, sym)) {
		return;
	}

	// print qualified name with arguments and type
	printFmt(out, esc, "%I%T: %T", ctx->indent, sym, sym->type);

	// print symbol info (kind, size, offset, ...)
	if (ctx->dmpDetails) {
		if (!dumpExtraData) {
			printFmt(out, esc, " {\n");
			dumpExtraData = 1;
		}
		printFmt(out, esc, "%I.kind: %K\n", indent, sym->kind);
		printFmt(out, esc, "%I.base: `%T`\n", indent, sym->type);
		printFmt(out, esc, "%I.size: %d\n", indent, sym->size);
		if (!ctx->hideOffsets) {
			dmpMode ofsMode = isStatic(sym) ? prAbsOffs : prRelOffs;
			printFmt(out, esc, "%I.offset: %.*a\n", indent, ofsMode, sym->offs);
		}
		printFmt(out, esc, "%I.name: '%s'\n", indent, sym->name);
		if (sym->fmt != NULL) {
			printFmt(out, esc, "%I.print: '%s'\n", indent, sym->fmt);
		}
		if (sym->file != NULL && sym->line > 0) {
			printFmt(out, esc, "%I.file: '%s:%u'\n", indent, sym->file, sym->line);
		}
		if (sym->owner != NULL) {
			printFmt(out, esc, "%I.owner: %?T\n", indent, sym->owner);
		}
	}

	// dump params of the function
	if (ctx->dmpParams) {
		symn param;
		if (!dumpExtraData) {
			printFmt(out, esc, " {\n");
			dumpExtraData = 1;
		}
		if (ctx->dmpApi && sym->fields != sym->params) {
			for (param = sym->fields; param; param = param->next) {
				if (!ctx->hideOffsets) {
					dmpMode ofsMode = isStatic(param) ? prAbsOffs : prRelOffs;
					printFmt(out, esc, "%I.field %.T: %?T (size: %d, offs: %.*a, cast: %K)\n", indent, param, param->type, param->size, ofsMode, param->offs, param->kind);
				}
				else {
					printFmt(out, esc, "%I.field %.T: %?T (size: %d, cast: %K)\n", indent, param, param->type, param->size, param->kind);
				}
			}
		}
		for (param = sym->params; param; param = param->next) {
			if (!ctx->hideOffsets) {
				dmpMode ofsMode = isStatic(param) ? prAbsOffs : prRelOffs;
				printFmt(out, esc, "%I.param %.T: %?T (size: %d, offs: %.*a, cast: %K)\n", indent, param, param->type, param->size, ofsMode, param->offs, param->kind);
			}
			else {
				printFmt(out, esc, "%I.param %.T: %?T (size: %d, cast: %K)\n", indent, param, param->type, param->size, param->kind);
			}
		}
	}

	if (dmpDoc) {
		if (!dumpExtraData) {
			printFmt(out, esc, " {\n");
			dumpExtraData = 1;
		}
		if (!ctx->dmpDetails && sym->file != NULL && sym->line > 0) {
			printFmt(out, esc, "%I.file: '%s:%u'\n", indent, sym->file, sym->line);
		}
		printFmt(out, esc, "%I.doc: '%s'\n", indent, sym->doc);
	}

	if (dmpAst) {
		if (!dumpExtraData) {
			printFmt(out, esc, " {\n");
			dumpExtraData = 1;
		}
		printFmt(out, esc, "%I.value: ", indent);
		printAst(out, esc, sym->init, ctx->dmpMode, -indent);
		printFmt(out, esc, "\n");
	}

	// print function disassembled instructions
	if (dmpAsm) {
		size_t end = sym->offs + sym->size;

		if (!dumpExtraData) {
			printFmt(out, esc, " {\n");
			dumpExtraData = 1;
		}
		printFmt(out, esc, "%I.instructions: (", indent);
		textDumpOfs(out, esc, ctx, sym, sym->offs, sym->offs + sym->size);
		printFmt(out, esc, ")\n");

		for (size_t pc = sym->offs, n = pc; pc < end; pc = n) {
			if (!nextOpc(ctx->rt, &n, NULL)) {
				// invalid instruction
				break;
			}
			dbgn dbg = mapDbgStatement(ctx->rt, pc, NULL);
			if (ctx->dmpAsmStmt && dbg && dbg->start == pc) {
				textDumpDbg(out, esc, ctx, sym, dbg, indent + 1);
			}
			textDumpAsm(out, esc, pc, ctx, indent + 1);
		}
	}

	// print usages of symbol
	if (dmpUse) {
		int internalUsages = 0;
		if (!dumpExtraData) {
			printFmt(out, esc, " {\n");
			dumpExtraData = 1;
		}
		printFmt(out, esc, "%I.usages:\n", indent);
		for (astn usage = sym->use; usage; usage = usage->id.used) {
			if (usage->file && usage->line && usage != sym->tag) {
				printFmt(out, esc, "%I%?s:%?u: referenced as `%.t`\n", indent + 1, usage->file, usage->line, usage);
			}
			else if (usage != sym->tag) {
				internalUsages += 1;
			}
		}
		if (internalUsages > 0) {
			printFmt(out, esc, "%Iinternal usages: %d\n", indent + 1, internalUsages);
		}
	}

	if (dumpExtraData) {
		printFmt(out, esc, "%I}", ctx->indent);
	}

	printFmt(out, esc, "\n");
}
static void dumpApiSciTE(userContext ctx, symn sym) {
	static const char *esc[256] = {0};
	if (esc[0] == NULL) {
		esc[0] = "\0";
		esc['\n'] = "\\n";
		esc['\r'] = "\\r";
		esc['\t'] = "\\t";
	}

	FILE *out = ctx->out;
	if (sym->name == NULL) {
		return;
	}

	// skip hidden symbols
	switch (*sym->name) {
		default:
			break;

		case '.':
		case 0:
			return;
	}

	if (isInvokable(sym)) {
		printSym(out, esc, sym, prSymQual|prSymArgs|prSymType, 0);
	}
	else {
		printSym(out, esc, sym, prSymQual|prSymArgs, 0);
	}
	if (sym->doc != NULL && *sym->doc && (sym->doc != sym->name || sym->params != NULL)) {
		printFmt(out, esc, " # %s", sym->doc);
	}
	printFmt(out, esc, "\n");
}


/**
 * Copy lines of a file following the given line into a cmpl code block,
 * leading blank lines are skipped.
 * @param count number of lines to copy, 0 copies the rest of the file.
 * @return non-zero if the file could not be read.
 */
static int dumpMdExample(FILE *out, const char *path, int afterLine, int count) {
	FILE *in = fopen(path, "rb");
	if (in == NULL) {
		return -1;
	}
	fseek(in, 0, SEEK_END);
	long size = ftell(in);
	fseek(in, 0, SEEK_SET);
	char *text = malloc(size + 1);
	if (text == NULL) {
		fclose(in);
		return -1;
	}
	size = (long) fread(text, 1, size, in);
	text[size] = 0;
	fclose(in);

	char *ptr = text;
	for (int line = 0; line < afterLine && *ptr; ptr += 1) {
		if (*ptr == '\n') {
			line += 1;
		}
	}
	if (count > 0) {
		// cut the text after the given number of lines
		char *end = ptr;
		for (int line = 0; line < count && *end; end += 1) {
			if (*end == '\n') {
				line += 1;
			}
		}
		*end = 0;
	}
	// skip leading blank lines
	for (char *eol = ptr; *eol == ' ' || *eol == '\t' || *eol == '\r' || *eol == '\n'; eol += 1) {
		if (*eol == '\n') {
			ptr = eol + 1;
		}
	}

	size_t len = strlen(ptr);
	fputs("\n```cmpl\n", out);
	fputs(ptr, out);
	if (len > 0 && ptr[len - 1] != '\n') {
		fputc('\n', out);
	}
	fputs("```\n", out);
	free(text);
	return 0;
}
/**
 * Dump the documented public symbols, which are not declared by the main unit,
 * grouped by type (see `inline "*library";`).
 */
static void dumpMdApi(userContext ctx, symn sym) {
	FILE *out = ctx->out;
	if (sym->doc == NULL || sym->doc == sym->name || sym->file == NULL) {
		// undocumented or builtin symbol
		return;
	}
	if (sym->unit == ctx->rt->main->unit) {
		// declared by the documentation
		return;
	}
	if (ctx->mdFilter != NULL) {
		// only the symbols of the requested unit: `inline "*library:unit.cmpl";`
		size_t len = strlen(ctx->mdFilter);
		size_t unitLen = sym->unit != NULL ? strlen(sym->unit) : 0;
		if (unitLen < len || strcmp(sym->unit + unitLen - len, ctx->mdFilter) != 0) {
			return;
		}
	}
	char chr = sym->name != NULL ? *sym->name : 0;
	if (!(chr == '_' || (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z'))) {
		// hidden symbols and operators
		return;
	}

	ctx->mdCount += 1;
	if (isTypename(sym)) {
		printFmt(out, NULL, "\n## %.*T\n%s\n\n", prSymQual, sym, sym->doc);
		ctx->mdOwner = sym;
		ctx->mdFile = NULL;
		return;
	}

	if (sym->owner != NULL) {
		if (sym->owner != ctx->mdOwner) {
			// members are listed in the section of their type
			return;
		}
		printFmt(out, NULL, "- `");
		printSym(out, NULL, sym, prSymArgs | prSymType, 0);
		printFmt(out, NULL, "`: %s\n", sym->doc);
		return;
	}

	if (ctx->mdOwner != NULL || !strEquals(ctx->mdFile, sym->file)) {
		printFmt(out, NULL, "\n## %s\n", sym->file);
		ctx->mdOwner = NULL;
		ctx->mdFile = sym->file;
	}
	printFmt(out, NULL, "- `");
	printSym(out, NULL, sym, prSymQual | prSymArgs | prSymType, 0);
	printFmt(out, NULL, "`: %s\n", sym->doc);
}
/// Validate the instructions of the virtual machine, abort on the first invalid instruction.
static void dumpMdOpcode(rtContext ctx, const char *error, const struct opcodeRec *info) {
	FILE *out = ctx->usr->out;
	if (error != NULL) {
		printFmt(out, NULL, "\n* Instruction error: `%s`\n", error);
	}
	if (!strEquals(ctx->usr->mdFilter, info->name)) {
		return;
	}
	printFmt(out, NULL, "\nDetails:");
	printFmt(out, NULL, "\n* Instruction code: `0x%02x`", info->code);
	printFmt(out, NULL, "\n* Instruction length: `%d` byte%?c", info->size, info->size == 1 ? 0 : 's');
	printFmt(out, NULL, "\n* Stack inputs: `%d` slot%?c", info->stack_in, info->stack_in == 1 ? 0 : 's');
	printFmt(out, NULL, "\n* Stack outputs: `%d` slot%?c", info->stack_out, info->stack_out == 1 ? 0 : 's');
	ctx->usr->mdCount += 1;
}
/// Dump the command line options and examples (see `inline "*usage";`).
static void dumpMdUsage(FILE *out) {
	for (size_t i = 0; i < lengthOf(usage_opts); ++i) {
		const char *flag = usage_opts[i].flag;
		const char *desc = usage_opts[i].desc;
		if (flag == NULL && desc == NULL) {
			printFmt(out, NULL, "\n");
			continue;
		}
		if (flag != NULL) {
			// flag or sub-option of the previous flag
			while (*flag == ' ') {
				flag += 1;
			}
			printFmt(out, NULL, "%s- `%s`: %s\n", usage_opts[i].sub ? "\t" : "", flag, desc);
			continue;
		}
		if (strBegins(desc, ">app ")) {
			// example invocation
			printFmt(out, NULL, ">cmpl %s\n\n", desc + 5);
			continue;
		}
		if (*desc == ' ') {
			// explanation of an example, 4 spaces per nesting level
			int level = 0;
			while (strBegins(desc, "    ")) {
				desc += 4;
				level += 1;
			}
			for (int n = 1; n < level; ++n) {
				printFmt(out, NULL, "\t");
			}
			printFmt(out, NULL, "- %s\n", desc);
			continue;
		}

		// section header: `<name>: description` or `name:`
		const char *end = strchr(desc, ':');
		if (end == NULL) {
			printFmt(out, NULL, "%s\n", desc);
			continue;
		}
		const char *name = desc;
		if (*name == '<') {
			name += 1;
		}
		size_t len = end - name;
		if (len > 0 && name[len - 1] == '>') {
			len -= 1;
		}
		if (len == 0) {
			printFmt(out, NULL, "%s\n", desc);
			continue;
		}
		char chr = *name;
		if (chr >= 'a' && chr <= 'z') {
			chr = (char) (chr - 'a' + 'A');
		}
		printFmt(out, NULL, "## %c", chr);
		fwrite(name + 1, 1, len - 1, out);
		printFmt(out, NULL, "\n");
		for (end += 1; *end == ' '; end += 1) {}
		if (*end != 0) {
			printFmt(out, NULL, "\n%s\n", end);
		}
	}
}
/**
 * Insert the generated text of a documentation placeholder: `inline "*directive:argument";`
 * (the directives are validated by the parser, see checkPlaceholder).
 */
static void dumpMdPlaceholder(userContext ctx, astn ast) {
	FILE *out = ctx->out;
	const char *name = ast->id.name + 1;
	const char *arg = strchr(name, ':');
	size_t len = arg != NULL ? (size_t) (arg - name) : strlen(name);
	if (arg != NULL) {
		arg += 1;
	}

	if (len == 7 && strncmp(name, "example", len) == 0) {
		// `*example:tail`: the rest of the file, `*example:<n>`: the next n lines
		int count = 0;
		if (arg != NULL && !strEquals(arg, "tail")) {
			count = atoi(arg);
			if (count <= 0) {
				return;
			}
		}
		if (dumpMdExample(out, ast->file, ast->line, count) != 0) {
			warn(ctx->rt, ast->file, ast->line, ERR_OPENING_FILE, ast->file);
		}
		return;
	}
	if (len == 7 && strncmp(name, "library", len) == 0) {
		ctx->mdOwner = NULL;
		ctx->mdFile = NULL;
		ctx->mdCount = 0;
		ctx->mdFilter = arg;
		dumpApi(ctx->rt, ctx, dumpMdApi);
		if (ctx->mdCount == 0) {
			error(ctx->rt, ast->file, ast->line, WARN_EMPTY_PLACEHOLDER, ast->id.name);
		}
		ctx->mdFilter = NULL;
		return;
	}
	if (len == 6 && strncmp(name, "opcode", len) == 0 && arg != NULL) {
		ctx->mdCount = 0;
		ctx->mdFilter = arg;
		vmSelfTest(ctx->rt, dumpMdOpcode);
		if (ctx->mdCount == 0) {
			error(ctx->rt, ast->file, ast->line, WARN_EMPTY_PLACEHOLDER, ast->file);
		}
		ctx->mdFilter = NULL;
		return;
	}
	if (len == 5 && strncmp(name, "usage", len) == 0) {
		dumpMdUsage(out);
	}
}

/**
 * Dump the markdown documentation: the doc comments, which do not belong to a declaration
 * (see doc_tag_inline and doc_tag_statement), followed by the generated text of the
 * documentation placeholders: `inline "*example:tail";`, `inline "*library";`, ...
 * `inline "./file.cmpl";` preceded by `/// **Example**` uses the whole file as example.
 */
static void dumpApiMarkdown(userContext ctx, symn sym) {
	FILE *out = ctx->out;
	if (!strEquals(sym->name, doc_tag_inline) && !strEquals(sym->name, doc_tag_statement)) {
		return;
	}
	if (sym->doc == NULL || sym->unit != ctx->rt->main->unit) {
		return;
	}
	if (*sym->doc != 0) {
		// the documentation is printed as it was written
		size_t len = strlen(sym->doc);
		printFmt(out, NULL, "\n%s%s", sym->doc, sym->doc[len - 1] == '\n' ? "" : "\n");
	}

	astn ast = sym->init;
	if (ast == NULL || ast->kind != TOKEN_val || ast->type != ctx->rt->cc->type_str) {
		return;
	}

	const char *name = ast->id.name;
	if (*name == '*') {
		dumpMdPlaceholder(ctx, ast);
		return;
	}
	if (!strEquals(sym->doc, "**Example**")) {
		// the doc comment belongs to the inlined file, not an example
		return;
	}

	char path[PATH_MAX] = {0};
	if (strBegins(name, "./") || strBegins(name, "../")) {
		// relative to the directory of the including file
		strncpy(path, ast->file, sizeof(path) - 1);
		char *dir = strrchr(path, '/');
		dir = dir != NULL ? dir + 1 : path;
		*dir = 0;
		strncat(path, name, sizeof(path) - strlen(path) - 1);
	}
	else if (*name == '/') {
		strncpy(path, name, sizeof(path) - 1);
	}
	else {
		// relative to the home folder of the compiler
		snprintf(path, sizeof(path), "%s/%s", ctx->rt->cc->home, name);
	}

	if (dumpMdExample(out, path, 0, 0) != 0) {
		warn(ctx->rt, ast->file, ast->line, ERR_OPENING_FILE, path);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ console debugger and profiler
static void traceMethods(userContext ctx, int mode, symn fun) {
	if (!ctx->traceMethods) {
		return;
	}

	const char **esc = ctx->esc;
	FILE *out = ctx->out;

	libc *nativeCalls = ctx->rt->vm.nfc;
	if (fun == nativeCalls[0]->sym) {
		// exclude halt()
		return;
	}

	if (ctx->traceTime) {
		double now = clock() * 1000. / CLOCKS_PER_SEC;
		printFmt(out, esc, "[% 3.2F] ", now);
	}
	if (mode > 0) {
		printFmt(out, esc, "% I> %T\n", ctx->callStack.cnt, fun);
	} else {
		printFmt(out, esc, "% I< %T\n", ctx->callStack.cnt, fun);
	}
}
static void traceOpcodes(userContext ctx, size_t ss, void *stack, size_t pc, dbgn dbg) {
	if (!ctx->traceOpcodes) {
		return;
	}

	rtContext rt = ctx->rt;
	const char **esc = ctx->esc;
	FILE *out = ctx->out;

	if (ctx->traceLocals && pc != rt->main->offs) {
		size_t i = 0;
		printFmt(out, esc, "    stack: %7d: [", ss);
		if (ss > maxLogItems) {
			printFmt(out, esc, " …");
			i = ss - maxLogItems;
		}
		for (; i < ss; i++) {
			if (i > 0) {
				printFmt(out, esc, ", ");
			}
			symn sym = NULL;
			uint32_t val = ((uint32_t *) stack)[ss - i - 1];
			if (val > 0 && val <= rt->vm.px) {
				sym = rtLookup(rt, val, 0);
				if (sym && !isFunction(sym)) {
					if (sym->offs != val) {
						sym = NULL;
					}
				}
			}
			// TODO: use printOfs
			if (sym != NULL) {
				size_t symOffs = val - sym->offs;
				printFmt(out, esc, "<%?.T%?+d>", sym, symOffs);
			} else {
				printFmt(out, esc, "%d", val);
			}
		}
		printFmt(out, esc, "\n");
	}

	if (ctx->dmpAsmStmt && dbg && dbg->start == pc) {
		textDumpDbg(out, esc, ctx, NULL, dbg, 0);
	}
	if (ctx->traceTime) {
		double now = clock() * 1000. / CLOCKS_PER_SEC;
		printFmt(out, esc, "[% 3.2F]: ", now);
	}
	textDumpAsm(out, esc, pc, ctx, 0);
}

static vmError dbgProfile(dbgContext ctx, vmError error, size_t pc, size_t ss, void *sp, int mode) {
	rtContext rt = ctx->rt;
	userContext usr = rt->usr;

	dbgn dbg = nextDbgNode(ctx, pc, usr->cachedDebugNode);
	usr->cachedDebugNode = dbg;

	// dieif(dbg == NULL, ERR_INTERNAL_ERROR);
	// dieif(dbg != nextDbgNode(ctx, pc, NULL), "Not the same (leftmost) debug node returned");
	// dieif(dbg != mapDbgStatement(rt, pc, NULL), "nextDbgNode disagrees with mapDbgStatement");
	//if (mode == 0) { debug(rt, dbg->file, dbg->line, "%a: %A", pc, vmPointer(rt, pc)); }

	if (error != noError) {
		callStackItem top = topBuff(&usr->callStack);
		while (top != NULL && top->ss >= ss) {
			leaveMethod(usr);
			top = topBuff(&usr->callStack);
		}

		dbg->traps += 1;
		if (isChecked(rt)) {
			return error;
		}

		// abort the execution and print the stacktrace
		return dbgError(ctx, error, pc, ss, sp, mode);
	}

	if (mode == 0) {
		if (pc == dbg->start) {
			// we just started executing a statement
			dbg->hits += 1;
		}

		dbg->exec += 1;
		// just an instruction, not a call or return
		traceOpcodes(usr, ss, sp, pc, dbg);

		return noError;
	}
	if (mode > 0) {
		traceMethods(usr, mode, dbg->fun);
		enterMethod(rt->usr, ss, dbg);
	} else {
		callStackItem top = topBuff(&usr->callStack);
		dieif(top == NULL, ERR_INTERNAL_ERROR);
		dieif(top->fun != dbg->fun, ERR_INTERNAL_ERROR);

		leaveMethod(usr);
		traceMethods(usr, mode, top->fun);
	}

	(void) sp;
	return noError;
}
static vmError dbgDebug(dbgContext ctx, vmError error, size_t pc, size_t ss, void *sp, int mode) {
	rtContext rt = ctx->rt;
	userContext usr = rt->usr;

	if (mode != 0 && error == noError) {
		// errors are reported leaving the function (mode < 0), handle them as instructions
		// TODO: enter will break after executing call?
		// TODO: leave will break after executing return?
		if (usr->dbgCommand == dbgStepInto && mode > 0) {
			usr->dbgNextBreak = (size_t) -1;
		}
		else if (usr->dbgCommand == dbgStepOut && mode < 0) {
			usr->dbgNextBreak = (size_t) -1;
		}
		traceMethods(usr, mode, NULL);
		return noError;
	}

	dbgn dbg = nextDbgNode(ctx, pc, usr->cachedDebugNode);
	usr->cachedDebugNode = dbg;
	brkMode breakMode = brkSkip;
	char *breakCause = NULL;

	if (error == divisionByZero) {
		// keep quiet performing floating point division by zero
		if (testOcp(rt, pc, f64_div, NULL)) {
			error = noError;
		}
		if (testOcp(rt, pc, f32_div, NULL)) {
			error = noError;
		}
	}
	if (error != noError) {
		breakCause = vmErrorMessage(error);
		if (isChecked(rt)) {
			breakMode = usr->dbgOnCaught;
		} else {
			breakMode = usr->dbgOnError;
		}
	}
	else if (usr->dbgNextBreak == (size_t) -1) {
		breakMode = brkPause;
		breakCause = "Break";
	}
	else if (usr->dbgNextBreak == pc) {
		breakMode = brkPause;
		breakCause = "Break";
	}
	else if (dbg != NULL) {
		if (pc == dbg->start) {
			breakMode = dbg->bp;
			if (breakMode & (brkPause | brkPrint | brkTrace)) {
				breakCause = "Break point";
			}
			if (breakMode & brkValue) {
				usr->dbgNextValue = dbg;
			}
			if (breakMode & brkOnce) {
				// remove breakpoint
				dbg->bp = brkSkip;
			}
		}
	}

	const char **esc = usr->esc;
	FILE *out = usr->out;
	FILE *con = stderr;

	// TODO: this might not work on `if`, `for`, `return` and other jump statements,
	//  or if the initializer is a function call having also `value type breakpoints`.
	dbgn valPrint = usr->dbgNextValue;
	if (valPrint && valPrint->end == pc) {
		// print current file position
		printFmt(out, esc, "%s:%u: ", valPrint->file, valPrint->line);
		if (valPrint->decl != NULL) {
			printVal(out, esc, rt, valPrint->decl, sp, prSymType, 0);
		} else {
			vmValue *v = (vmValue *) sp;
			printFmt(out, esc, "{0x%08x, i32(%d), f32(%f), i64(%D), f64(%F)}", v->i32, v->i32, v->f32, v->i64, v->f64);
		}
		printFmt(out, esc, "\n");
		usr->dbgNextValue = NULL;
	}
	traceOpcodes(usr, ss, sp, pc, dbg);

	// print error type
	if (breakMode & brkPrint) {
		symn fun = rtLookup(rt, pc, 0);
		size_t funOffs = pc;
		if (fun != NULL) {
			funOffs -= fun->offs;
		}
		// print current file position
		if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
			printFmt(out, esc, "%s:%u: ", dbg->file, dbg->line);
		}
		printFmt(out, esc, ERR_EXEC_INSTRUCTION"\n", breakCause, pc, fun, funOffs, vmPointer(rt, pc));
		if (out != stdout && out != stderr) {
			// print the error message also to the console
			if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
				printFmt(con, esc, "%s:%u: ", dbg->file, dbg->line);
			}
			printFmt(con, esc, ERR_EXEC_INSTRUCTION"\n", breakCause, pc, fun, funOffs, vmPointer(rt, pc));
		}
	}
	if (breakMode & brkTrace) {
		traceCalls(rt, out, 1, maxLogItems, 0);
	}

	if (error != noError && !(breakMode & brkPause) && isChecked(rt)) {
		// the error is caught by `recover`, which should return it
		return error;
	}

	// pause execution in debugger
	for ( ; breakMode & brkPause; ) {
		dbgMode cmd = usr->dbgCommand;
		char buff[1024], *arg = NULL;

		printFmt(con, esc, ">dbg[%?c] %.A: ", cmd, vmPointer(rt, pc));
		if (usr->in == NULL || fgets(buff, sizeof(buff), usr->in) == NULL) {
			printFmt(con, esc, "can not read from standard input, aborting\n");
			return illegalState;
		}
		if ((arg = strchr(buff, '\n'))) {
			*arg = 0;		// remove new line char
		}

		if (*buff != 0) {
			// update last command
			cmd = (dbgMode) buff[0];
			arg = buff + 1;
			usr->dbgCommand = cmd;
		}

		switch (cmd) {
			default:
				printFmt(con, esc, "invalid command '%s'\n", buff);
				break;

			case dbgAbort:
				return illegalState;

			case dbgResume:
			case dbgStepInto:
			case dbgStepOut:
				usr->dbgNextBreak = 0;
				return noError;

			case dbgStepNext:
				usr->dbgNextBreak = (size_t)-1;
				return noError;

			case dbgStepLine:
				usr->dbgNextBreak = dbg ? dbg->end : 0;
				return noError;

			case dbgPrintStackTrace:
				traceCalls(rt, con, 1, maxLogItems, 0);
				break;

			case dbgPrintInstruction:
				textDumpAsm(con, esc, pc, usr, 0);
				break;

			case dbgPrintStackValues:
				if (*arg == 0) {
					size_t offs;
					// print top of stack
					for (offs = 0; offs < ss; offs++) {
						vmValue *v = (vmValue*)&((long*)sp)[offs];
						printFmt(con, esc, "\tsp(%d): {0x%08x, i32(%d), f32(%f), i64(%D), f64(%F)}\n", offs, v->i32, v->i32, v->f32, v->i64, v->f64);
					}
				}
				else {
					symn sym = ccLookup(rt->cc, NULL, arg);
					printFmt(con, esc, "arg:%T", sym);
					if (sym && isVariable(sym) && !isStatic(sym)) {
						printVal(con, esc, rt, sym, (vmValue *) sp, prSymType, 0);
					}
				}
				break;
		}
	}
	return noError;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// Private dummy on exit native function.
static vmError onHalt(nfcContext ctx) {
	(void)ctx;
	return noError;
}

static int usage() {
	fputs("cmpl [global options] [files with options]...", stdout);
	for (size_t i = 0; i < lengthOf(usage_opts); ++i) {
		const char *flag = usage_opts[i].flag;
		const char *desc = usage_opts[i].desc;
		if (flag == NULL && desc == NULL) {
			fputc('\n', stdout);
		}
		else if (flag == NULL) {
			fprintf(stdout, "\n%s", desc);
		}
		else {
			fprintf(stdout, "\n%-24s%s", flag, desc);
		}
	}
	fprintf(stdout, "Version: %d, more details at: https://karetkaz.github.io/Cmpl\n", cmplVersion());
	return 0;
}

static const char* detectHomeFolder(ccContext cc, const char *argv0) {
	char *home = getenv(CMPL_HOME);
	if (home != NULL) {
		return ccUniqueStr(cc, home, -1, -1);
	}

	char buffer[PATH_MAX];
	// path variable not set, try to guess based on executable path by going backward on parents,
	// until `cmplUnit` file exists (works only if the executable is inside the project folder)
	if (absolutePath(argv0, buffer, sizeof(buffer)) == NULL) {
		return NULL;
	}

	char *separator = buffer + strlen(buffer);
	while (--separator > buffer) {
		if (*separator != '/') {
			continue;
		}
		size_t max = sizeof(buffer) - (separator - buffer);
		strncpy(separator + 1, cmplUnit, max - 1);
		FILE *f = fopen(buffer, "r");
		if (f != NULL) {
			fclose(f);
			*separator = '\0';
			return ccUniqueStr(cc, buffer, -1, -1);
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	if (argc < 2) {
		return usage();
	}
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0) {
			return usage();
		}
	}

	struct userContextRec extra = {0};
	extra.in = stdin;
	extra.out = stdout;
	extra.std = cmplUnit;
	extra.dmpSymbols = 0;  // by default print symbols from main file only
	extra.dbgCommand = dbgResume;	// last command: resume
	extra.dbgOnError = brkPause | brkPrint | brkTrace;
	extra.dbgOnCaught = brkSkip;
	extra.foldConstants = 1;
	extra.foldInstruction = 1;
	extra.errPrivateAccess = 1;
	extra.errUninitialized = 1;
	extra.checkArrayBounds = 1;
	extra.preferNativeCalls = 1;
	extra.genDebugInformation = 1;
	extra.warnLevel = 5;
	extra.raiseLevel = 15;
	extra.memory = 128 << 20;// 128 MB memory compiler + runtime


	ccConfig install = install_def;
	char *ccFile = NULL;			// compiled filename

	char *logFileName = NULL;		// logger filename
	int logAppend = 0;				// do not clear the log file

	char *dumpFileName = NULL;      // dump file
	char *pathAstDumpXml = NULL;    // dump ast to xml file before code generation
	void (*dumpFun)(userContext, symn) = NULL;
	enum { run, debug, profile, compile } run_code = compile;

	int i = 1;
	// process global options
	for (; i < argc; ++i) {
		char *arg = argv[i];

		// not an option
		if (*arg != '-') {
			break;
		}

		// run, debug or profile
		else if (strncmp(arg, "-run", 4) == 0) {
			char *arg2 = arg + 4;
			if (run_code != compile) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			run_code = run;
			extra.genDebugInformation = 0;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					case 'G':
						extra.dmpGlobals = 3;
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'g':
						extra.dmpGlobals = 1;
						arg2 += 2;
						break;

					case 'M':
						extra.dmpMemoryUse = 3;
						arg2 += 2;
						break;
					case 'm':
						extra.dmpMemoryUse = 1;
						arg2 += 2;
						break;

					case 't':
						// debug and profile are many times slower, generate debug information,
						// and then search where the return address is on the stack when needed
						extra.genDebugInformation = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-debug", 6) == 0) {
			char *arg2 = arg + 6;
			if (run_code != compile) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			run_code = debug;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					case 'G':
						extra.dmpGlobals = 3;
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'g':
						extra.dmpGlobals = 1;
						arg2 += 2;
						break;

					case 'M':
						extra.dmpMemoryUse = 3;
						arg2 += 2;
						break;
					case 'm':
						extra.dmpMemoryUse = 1;
						arg2 += 2;
						break;

					case 'T':
						extra.traceLocals = 1;
						// fall through
					case 't':
						extra.traceOpcodes = 1;
						arg2 += 2;
						break;

					case 'P':
						extra.dbgOnCaught |= brkTrace;
						// fall through
					case 'p':
						extra.dbgOnCaught |= brkPrint;
						arg2 += 2;
						break;

					case 's':
						extra.dbgOnCaught |= brkPause;
						arg2 += 2;
						break;
					case 'S':
						extra.dbgNextBreak = (size_t)-1;
						extra.dbgOnCaught |= brkPause;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-profile", 8) == 0) {
			char *arg2 = arg + 8;
			if (run_code != compile) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			run_code = profile;
			extra.profFunctions = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					case 'G':
						extra.dmpGlobals = 3;
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'g':
						extra.dmpGlobals = 1;
						arg2 += 2;
						break;

					case 'M':
						extra.dmpMemoryUse = 3;
						arg2 += 2;
						break;
					case 'm':
						extra.dmpMemoryUse = 1;
						arg2 += 2;
						break;

					case 'T':
						extra.traceOpcodes = 1;
						// fall through
					case 't':
						extra.traceMethods = 1;
						extra.traceTime = 1;
						arg2 += 2;
						break;

					case 'P':
						extra.profNotExecuted = 1;
						// fall through
					case 'p':
						extra.profStatements = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}

		// override stdlib file
		else if (strncmp(arg, "-std", 4) == 0) {
			if (arg[4] != 0) {
				extra.std = arg + 4;
			}
			else {
				// disable standard library
				extra.std = NULL;
			}
		}
		// override heap size
		else if (strncmp(arg, "-mem", 4) == 0) {
			int value = -1;
			const char *arg2 = arg + 4;
			if (*arg2) {
				arg2 = parseInt(arg2, &value, 10);
				extra.memory = (size_t) value;
			}
			switch (*arg2) {
				default:
					break;

				case 'k':
				case 'K':
					extra.memory <<= 10;
					if (extra.memory >> 10 != (size_t)value) {
						break;
					}
					arg2 += 1;
					break;

				case 'm':
				case 'M':
					extra.memory <<= 20;
					if (extra.memory >> 20 != (size_t)value) {
						break;
					}
					arg2 += 1;
					break;

				case 'g':
				case 'G':
					extra.memory <<= 30;
					if (extra.memory >> 30 != (size_t)value) {
						break;
					}
					arg2 += 1;
					break;
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}

		// logger filename and level
		else if (strncmp(arg, "-log", 4) == 0) {
			char *arg2 = arg + 4;
			if (++i >= argc || logFileName) {
				fatal("log file not or double specified");
				return -1;
			}
			logFileName = argv[i];
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						// allow `-log/a/12`
						arg2 += 1;
						break;

					case 'a':
						logAppend = 1;
						arg2 += 2;
						break;

					case 'd':
						dumpFileName = logFileName;
						arg2 += 2;
						break;
				}
			}
			if (*arg2 && *parseInt(arg2, &extra.warnLevel, 10)) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
			if (extra.warnLevel > 15) {
				extra.warnLevel = 15;
			}
		}

		// dump output format and filename
		else if (strncmp(arg, "-dump", 5) == 0) {
			if (strEquals(arg, "-dump")) {
				dumpFun = dumpApiText;
			}
			else if (strEquals(arg, "-dump.ast.xml")) {
				if (++i >= argc || pathAstDumpXml) {
					fatal("dump file not or double specified");
					return -1;
				}
				pathAstDumpXml = argv[i];
				continue;
			}
			else if (strEquals(arg, "-dump.json")) {
				dumpFun = dumpJsonApi;
			}
			else if (strEquals(arg, "-dump.scite")) {
				dumpFun = dumpApiSciTE;
			}
			else if (strEquals(arg, "-dump.md")) {
				dumpFun = dumpApiMarkdown;
			}
			else {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
			if (++i >= argc || dumpFileName) {
				fatal("dump file not or double specified");
				return -1;
			}
			dumpFileName = argv[i];
		}
		else if (strncmp(arg, "-api", 4) == 0) {
			char *arg2 = arg + 4;
			if (extra.dmpApi) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpApi = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					// include extra symbols
					case 'A':
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'a':
						extra.dmpSymbols = 2;
						arg2 += 2;
						break;

					case 'd':
						extra.dmpDetails = 1;
						arg2 += 2;
						break;

					case 'p':
						extra.dmpParams = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-doc", 4) == 0) {
			char *arg2 = arg + 4;
			if (extra.dmpDoc) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpDoc = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					// include extra symbols
					case 'A':
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'a':
						extra.dmpSymbols = 2;
						arg2 += 2;
						break;

					case 'd':
						extra.dmpDetails = 1;
						arg2 += 2;
						break;

					case 'p':
						extra.dmpParams = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-asm", 4) == 0) {
			char *arg2 = arg + 4;
			if (extra.dmpAsm) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpAsm = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						// allow `-asm/a/9`
						arg2 += 1;
						break;

					case 'A':
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'a':
						extra.dmpSymbols = 2;
						arg2 += 2;
						break;

					case 'd':
						extra.dmpDetails = 1;
						arg2 += 2;
						break;

					case 'p':
						extra.dmpParams = 1;
						arg2 += 2;
						break;

					case 'g':
						extra.dmpMode |= prAsmOffs|prAbsOffs;
						arg2 += 2;
						break;

					case 'n':
						extra.dmpMode |= prAsmOffs|prRelOffs;
						arg2 += 2;
						break;

					case 's':
						extra.dmpAsmStmt = 1;
						arg2 += 2;
						break;
				}
			}
			int level = 9;
			if (*arg2 && *parseInt(arg2, &level, 10)) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
			if (level > 15) {
				level = 15;
			}
			extra.dmpMode |= prAsmCode & level;
		}
		else if (strncmp(arg, "-ast", 4) == 0) {
			char *arg2 = arg + 4;
			if (extra.dmpAst) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpAst = 1;
			extra.dmpMode |= prDetail;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					case 'A':
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'a':
						extra.dmpSymbols = 2;
						arg2 += 2;
						break;

					case 'd':
						extra.dmpDetails = 1;
						arg2 += 2;
						break;

					case 'p':
						extra.dmpParams = 1;
						arg2 += 2;
						break;

					case 't':
						extra.dmpMode |= prAstType;
						arg2 += 2;
						break;

					case 'l':
						extra.dmpMode |= prOneLine;
						arg2 += 2;
						break;

					case 'b':
						extra.dmpMode |= nlAstBody;
						arg2 += 2;
						break;

					case 'e':
						extra.dmpMode |= nlAstElIf;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}
		else if (strncmp(arg, "-use", 4) == 0) {
			char *arg2 = arg + 4;
			if (extra.dmpUse) {
				fatal("argument specified multiple times: %s", arg);
				return -1;
			}
			extra.dmpUse = 1;
			while (*arg2 == '/') {
				switch (arg2[1]) {
					default:
						arg2 += 1;
						break;

					case 'A':
						extra.dmpSymbols = 3;
						arg2 += 2;
						break;
					case 'a':
						extra.dmpSymbols = 2;
						arg2 += 2;
						break;

					case 'd':
						extra.dmpDetails = 1;
						arg2 += 2;
						break;

					case 'p':
						extra.dmpParams = 1;
						arg2 += 2;
						break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
		}

		// enable or disable hidden settings
		else if (strBegins(arg, "-X")) {
			char *arg2 = arg + 2;
			while (*arg2 == '-' || *arg2 == '+') {
				int on = *arg2 == '+';
				if (strBegins(arg2 + 1, "fold")) {
					extra.foldConstants = on;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "fast")) {
					extra.foldInstruction = on;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "glob")) {
					extra.genStaticGlobals = on;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "emit")) {
					install = (install & ~installEmit) | on * installEmit;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "ptr")) {
					install = (install & ~install_ptr) | on * install_ptr;
					arg2 += 4;
				}
				else if (strBegins(arg2 + 1, "var")) {
					install = (install & ~install_var) | on * install_var;
					arg2 += 4;
				}
				else if (strBegins(arg2 + 1, "obj")) {
					install = (install & ~install_obj) | on * install_obj;
					arg2 += 4;
				}
				else if (strBegins(arg2 + 1, "stdin")) {
					extra.in = on ? stdin : NULL;
					arg2 += 6;
				}
				else if (strBegins(arg2 + 1, "std")) {
					install = (install & ~installLibs) | on * installLibs;
					arg2 += 4;
				}
				else if (strBegins(arg2 + 1, "warn")) {
					extra.warnLevel *= on;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "wall")) {
					extra.warnLevel = 15;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "init")) {
					extra.errUninitialized = !on;
					arg2 += 8;
				}
				else if (strBegins(arg2 + 1, "public")) {
					extra.errPrivateAccess = !on;
					arg2 += 7;
				}
				else if (strBegins(arg2 + 1, "offsets")) {
					extra.hideOffsets = !on;
					arg2 += 8;
				}
				else if (strBegins(arg2 + 1, "steps")) {
					extra.steps = on ? "\n---------- " : NULL;
					arg2 += 6;
				}
				else if (strBegins(arg2 + 1, "bounds")) {
					extra.checkArrayBounds = on;
					arg2 += 7;
				}
				else if (strBegins(arg2 + 1, "native")) {
					extra.preferNativeCalls = on;
					arg2 += 7;
				}
				else if (strBegins(arg2 + 1, "test")) {
					extra.runsInTestMode = on;
					arg2 += 5;
				}
				else {
					break;
				}
			}
			if (*arg2) {
				fatal("invalid argument '%s' at %d", arg, arg2 - arg);
				return -1;
			}
		}

		// no more global options
		else break;
	}

	if ((dumpFun == dumpApiSciTE || dumpFun == dumpApiMarkdown) && !extra.dmpDoc) {
		// record documentation for api
		extra.dmpDoc = 1;
	}
	if (extra.hideOffsets) {
		extra.dmpMode &= ~(prAsmCode|prRelOffs|prAbsOffs);
	}

	// initialize runtime context
	rtContext rt = rtInit(NULL, extra.memory);
	if (rt == NULL) {
		fatal("initializing runtime context");
		return -1;
	}

	extra.rt = rt;
	rt->usr = &extra;

	// override log level
	rt->logLevel = extra.warnLevel;

	// override constant folding
	rt->foldCasts = extra.foldConstants != 0;
	rt->foldConst = extra.foldConstants != 0;

	// override instruction folding
	rt->foldInstr = extra.foldInstruction != 0;
	rt->foldInstr2 = extra.foldInstruction != 0;
	rt->foldMemory = extra.foldInstruction != 0;
	rt->foldAssign = extra.foldInstruction != 0;

	rt->checkBounds = extra.checkArrayBounds != 0;
	initBuff(&extra.callStack, 32, sizeof(struct callStackItem));
	initBuff(&extra.breakPoints, 32, sizeof(struct breakPointItem));

	// open the log file first (enabling dump appending)
	if (logFileName != NULL) {
		rt->logFile = fopen(logFileName, logAppend ? "ab" : "wb");
		rt->closeLog = rt->logFile != NULL;
		if (rt->logFile == NULL) {
			rt->logFile = stdout;
			info(rt, NULL, 0, ERR_OPENING_FILE, logFileName);
		}
	}

	// dump to log file (global option)
	if (dumpFileName && strEquals(dumpFileName, logFileName)) {
		// enable logging only as text to the log file
		if (dumpFun == NULL) {
			dumpFun = dumpApiText;
		}
		if (dumpFun == dumpApiText) {
			extra.out = rt->logFile;
		}
	}
	else if (dumpFileName != NULL) {
		extra.out = fopen(dumpFileName, "w");
		extra.closeOut = 1;
	}
	if (extra.out == NULL) {
		info(rt, NULL, 0, ERR_OPENING_FILE, dumpFileName);
		extra.out = stdout;
		extra.closeOut = 0;
	}

	// initialize compiler context, install builtin types
	ccContext cc = ccInit(rt, install, onHalt);
	if (cc == NULL) {
		fatal("initializing compiler context");
		freeBuff(&extra.breakPoints);
		freeBuff(&extra.callStack);
		logFILE(rt, NULL);
		return -6;
	}
	cc->runsInTestMode = extra.runsInTestMode != 0;
	cc->preferNativeCalls = extra.preferNativeCalls != 0;

	cc->genDocumentation = extra.dmpDoc;
	cc->genStaticGlobals = extra.genStaticGlobals != 0;
	cc->errPrivateAccess = extra.errPrivateAccess != 0;
	cc->errUninitialized = extra.errUninitialized != 0;
	cc->home = detectHomeFolder(cc, argv[0]);

	if (install & installLibs) {
		// install standard library.
		if (extra.steps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sCompile: `%?s`", extra.steps, extra.std);}
		if (cmplInit(rt, cc) != 0) {
			error(rt, NULL, 0, "error registering standard library");
			freeBuff(&extra.breakPoints);
			freeBuff(&extra.callStack);
			return -1;
		}
		if (extra.std != NULL) {
			char buffer[65536];
			snprintf(buffer, sizeof(buffer), "inline \"%s\";", extra.std);
			if (!ccAddUnit(cc, extra.std, 0, buffer)) {
				error(rt, NULL, 0, "error registering standard library");
				freeBuff(&extra.breakPoints);
				freeBuff(&extra.callStack);
				return -1;
			}
		}
	}

	// compile and import files / modules
	for (; i <= argc; ++i) {
		char *arg = argv[i];
		if (i == argc || *arg != '-') {
			if (ccFile != NULL) {
				cc->unit = ccFile;
				char *ext = strrchr(ccFile, '.');
				if (ext && (strEquals(ext, ".so") || strEquals(ext, ".dll") || strEquals(ext, ".wasm") || strEquals(ext, ".dylib"))) {
					char *name = strrchr(ccFile, '/');
					if (name == NULL) {
						name = ccFile;
					} else {
						name += 1;
					}
					if (extra.steps != NULL && ccFile != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sLibrary: `%?s`", extra.steps, name);}
					int resultCode = importLib(rt, cc, ccFile);
					if (resultCode != 0) {
						fatal("error(%d) importing library `%s`", resultCode, ccFile);
					}
				}
				else {
					char *name = ccFile;
					int homeLen = strlen(cc->home);
					if (strncmp(ccFile, cc->home, homeLen) == 0) {
						name += homeLen + 1;
					}
					if (extra.steps != NULL && ccFile != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sCompile: `%?s`", extra.steps, name);}
					int errors = rt->errors;
					if (!ccAddUnit(cc, ccFile, 1, NULL) && errors == rt->errors) {
						// show the error in case it was not raised, but returned
						error(rt, ccFile, 1, "error compiling source `%s`", ccFile);
					}
				}
			}
			if (i < argc) {
				ccFile = arg;
			}
			rt->logLevel = extra.warnLevel;
		}
		else {
			if (ccFile == NULL) {
				fatal("argument `%s` must be preceded by a file", arg);
			}
			if (arg[1] == 'w') {		// warning level for file (TODO: parser warnings only)
				int level = 0;
				if (strcmp(arg, "-wa") == 0) {
					level = 15;
				}
				else if (*parseInt(arg + 2, &level, 10)) {
					fatal("invalid warning level '%s'", arg + 2);
				}
				if (level > 15) {
					level = 15;
				}
				rt->logLevel = (unsigned) level;
			}
			else if (arg[1] == 'b') {
				int line = 0;
				brkMode type = brkPause;
				char *arg2 = arg + 2;
				while (*arg2 == '/') {
					switch (arg2[1]) {
						default:
							// allow `-b/p/10`
							arg2 += 1;
							break;

						case 'P': // trace only
							type |= brkTrace;
							// fall through
						case 'p': // print only
							type &= ~brkPause;
							type |= brkPrint;
							arg2 += 2;
							break;

						case 'v': // value only
							type &= ~brkPause;
							type |= brkValue;
							arg2 += 2;
							break;

						case 'o': // remove after first hit
							type |= brkOnce;
							arg2 += 2;
							break;
					}
				}

				if (*parseInt(arg2, &line, 10)) {
					fatal("invalid line number `%s`", arg + 2);
				}
				breakPointItem bp = addBuff(&extra.breakPoints, NULL);
				bp->file = ccFile;
				bp->line = line;
				bp->type = type;
			}
			else {
				fatal("invalid option: `%s`", arg);
			}
		}
	}

	if (pathAstDumpXml != NULL) {
		FILE *xmlFile = fopen(pathAstDumpXml, "w");
		if (xmlFile == NULL) {
			fatal(ERR_OPENING_FILE, pathAstDumpXml);
			freeBuff(&extra.breakPoints);
			freeBuff(&extra.callStack);
			return -1;
		}
		dumpAstXML(xmlFile, escapeXML(), cc->root, prDbg, 0, "main");
		fclose(xmlFile);
	}

	// generate code only if there are no compilation errors
	if (rt->errors == 0) {
		if (extra.steps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sGenerate: byte-code", extra.steps);}
		// debug metadata is always generated: it does not change the emitted
		// code, and enables stack traces with source positions in run mode
		if (ccGenCode(cc, extra.genDebugInformation) != 0) {
			if (rt->errors == 0) {
				error(rt, NULL, 0, "error generating code");
			}
			dbgTrace("error generating code");
		}
		// set breakpoints
		for (size_t i = 0; i < extra.breakPoints.cnt; ++i) {
			breakPointItem bp = getBuff(&extra.breakPoints, i);
			dbgn dbg = getDbgStatement(rt, bp->file, bp->line);
			if (dbg == NULL) {
				info(rt, bp->file, bp->line, "invalid breakpoint");
				continue;
			}

			if (bp->type & brkValue && dbg->stmt && dbg->stmt->kind == TOKEN_var) {
				dbg->decl = dbg->stmt->id.link;
			}
			dbg->bp = bp->type;
		}
	}

	if (dumpFun == NULL) {
		if (extra.dmpApi || extra.dmpDoc || extra.dmpAsm || extra.dmpAst || extra.dmpUse) {
			dumpFun = dumpApiText;
		}
	}

	if (rt->errors == 0) {
		if (dumpFun == dumpJsonApi) {
			extra.esc = escapeJSON();
			printFmt(extra.out, extra.esc, "{\n");
			printFmt(extra.out, extra.esc, "%I\"%s\": \"%d\"\n", extra.indent, JSON_KEY_VERSION, cmplVersion());
			printFmt(extra.out, extra.esc, JSON_OBJ_ARR_START, extra.indent, JSON_KEY_SYMBOLS);
			dumpApi(rt, &extra, dumpJsonApi);
			printFmt(extra.out, extra.esc, JSON_OBJ_ARR_END, extra.indent);

			printFmt(extra.out, extra.esc, JSON_OBJ_START, extra.indent, "main");
			jsonDumpSym(extra.out, extra.esc, rt->main, NULL, extra.indent + 1);
			printFmt(extra.out, extra.esc, JSON_OBJ_END, extra.indent);

			if (extra.dmpDoc) {
				// static VM/CLI metadata, independent of the compiled
				// program: only emitted alongside documentation, so routine
				// test/profile JSON dumps are unaffected.
				dumpJsonOpcodes(extra.out, extra.esc, extra.indent);
				dumpJsonUsage(extra.out, extra.esc, extra.indent);
			}
		}
		else if (dumpFun != NULL) {
			extra.esc = NULL;
			if (extra.steps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sSymbols:", extra.steps);}
			dumpApi(rt, &extra, dumpFun);
		}
	}

	// run code if there are no compilation errors.
	if (rt->errors == 0 && run_code != compile) {
		const dbgContext dbg = rt->dbg;
		rt->logLevel = extra.raiseLevel;
		if (dbg != NULL) {
			// set debugger or profiler
			switch (run_code) {
				case run:
				case compile:
					dbg->debug = NULL;
					break;

				case debug:
					dbg->debug = &dbgDebug;
					if (extra.dbgNextBreak != 0) {
						// break on first instruction
						extra.dbgNextBreak = rt->vm.pc;
					}
					break;

				case profile:
					// set call tree dump method
					if (dumpFun == dumpJsonApi) {
						dbg->debug = &jsonProfile;
						jsonPreProfile(dbg);
					} else {
						dbg->debug = &dbgProfile;
					}
					break;
			}

			// update debug statement list, and remove statements
			size_t dataCnt = dbg->statements.cnt;
			dbgn data = (dbgn) dbg->statements.ptr;
			for (size_t j = 0; j < dataCnt; ++j) {
				data[j].stmt = NULL;
			}
		}
		if (!extra.dmpAsm && extra.traceOpcodes) {
			extra.dmpMode |= prRelOffs | 9;
			extra.dmpAsmStmt = 1;
		}
		if (extra.steps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sExecute: byte-code", extra.steps);}
		long start = clock();
		if (execute(rt, NULL) == noError) {
			// clear caught errors
			rt->errors = 0;
		}
		else {
			rt->errors = 1;
		}
		if (dbg != NULL) {
			size_t dataCnt = dbg->statements.cnt;
			dbgn data = (dbgn) dbg->statements.ptr;
			for (size_t j = 0; j < dataCnt; ++j) {
				dbgn stmt = data + j;
				size_t group = j;
				while (group > 0) {
					group -= 1;
					dbgn d = data + group;
					if (d->start == stmt->start) {
						break;
					}
					if (d->start < stmt->start) {
						group = j; // not a group
						break;
					}
					if (d->end < stmt->start) {
						group = j; // not a group
						break;
					}
				}

				if (group != j) {
					stmt->hits = data[group].hits;
					stmt->group = group - j + data[group].group;
					for (size_t k = group; k < j; ++k) {
						dbgn d = data + k;
						stmt->exec += d->exec;
						stmt->traps += d->traps;
					}
					continue;
				}

				extra.stmtCnt += 1;
				extra.stmtCov += stmt->hits > 0;
				extra.trapCnt += stmt->traps > 0;

				symn fun = stmt->fun;
				if (fun == NULL) {
					fatal("can not be null");
					continue;
				}
				if (stmt->start == fun->offs) {
					extra.funcCnt += 1;
					extra.funcCov += stmt->hits > 0;
				}
			}
		}
		extra.rtTime = clock() - start;
	}

	if (rt->errors == 0) {
		if (dumpFun == dumpJsonApi) {
			jsonPostProfile(rt->dbg);
			printFmt(extra.out, extra.esc, "}\n");
		}
		else if (run_code != compile) {
			// dump results: global variables, memory allocations
			textPostProfile(&extra);
		}
	}

	if (extra.steps != NULL) {
		clock_t runTime = extra.hideOffsets ? 0 : extra.rtTime;
		printLog(extra.rt, raisePrint, NULL, 0, "%sExitcode: %d, time: %.3F ms",
			extra.steps, rt->errors, runTime  * 1000. / CLOCKS_PER_SEC
		);
	}

	if (extra.out && extra.closeOut) {
		fclose(extra.out);
	}

	freeBuff(&extra.breakPoints);
	freeBuff(&extra.callStack);
	return rtClose(rt) != 0;
}
