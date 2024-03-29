/*******************************************************************************
 *   File: main.c
 *   Date: 2011/06/23
 *   Desc: command line executable
 *******************************************************************************
 * command line argument parsing
 * console debugger
 * json / text dump
 */

#include <time.h>
#include "cmplDbg.h"
#include "internal.h"
#include "utils.h"

// default values
static const char * const STDLIB = "/cmplStd/lib.ci";   // standard library
static const char * const CMPL_HOME = "CMPL_HOME";    // Home environment path variable name

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
	dbgPrintStackValues = 's',	// print values on sack
	dbgPrintInstruction = 'p',	// print current instruction
} dbgMode;

struct userContextRec {
	rtContext rt;
	FILE *out;              // dump file
	const char **esc;       // escape characters
	int indent;             // dump indentation
	dmpMode dmpMode;        // dump flags

	unsigned dmpApi:1;           // dump symbols
	unsigned dmpDoc:1;           // dump documentation
	unsigned dmpAsm:1;           // dump instructions
	unsigned dmpAst:1;           // dump abstract syntax tree
	unsigned dmpUse:1;           // dump usages

	unsigned dmpDetails:1;       // dump detailed info
	unsigned dmpParams:1;        // dump parameters and fields
	unsigned dmpAsmStmt:1;       // print source code position/statements

	// exclude by location file, unit, library, builtins
	unsigned dmpSymbols:2;       // 0: from file, 1: from unit; 2: all compiled; 3: builtin symbols

	// dump global variable, [function, [typename]] values
	unsigned dmpGlobals:2;

	// dump memory allocation [used, [free] heap memory chunks] information
	unsigned dmpMemoryUse:2;

	unsigned traceMethods:1;     // dump function call and return
	unsigned traceOpcodes:1;     // dump executing instruction
	unsigned traceLocals:1;      // dump the content of the stack
	unsigned traceTime:1;        // dump timestamp before `traceMethods` and `traceOpcodes`
	unsigned profFunctions:1;    // measure function execution times
	unsigned profStatements:1;   // measure statement execution times
	unsigned profNotExecuted:1;  // include not executed functions and statements in the dump

	unsigned closeOut:1;         // close `out` file.
	unsigned hideOffsets:1;      // remove offsets, bytecode, and durations from dumps
	char *compileSteps;     // dump compilation steps

	// debugger
	FILE *in;               // debugger input
	dbgMode dbgCommand;     // last debugger command
	brkMode dbgOnError;     // on uncaught exception
	brkMode dbgOnCaught;    // on caught exception
	size_t dbgNextBreak;    // offset of the next break (used for step in, out, over, instruction)
	dbgn dbgNextValue;

	clock_t rtTime;         // time of execution
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
		case OPER_sel:        // '?:'

		case OPER_com:        // ','

		case INIT_set:        // '='
		case ASGN_set:        // '='
			printFmt(out, esc, " value=\"%?t\">\n", ast);
			dumpAstXML(out, esc, ast->op.test, mode & ~prSymInit, indent + 1, "test");
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
	printFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent, JSON_KEY_KIND, ptr->kind & MASK_kind);
	printFmt(out, esc, "%I, \"%s\": \"%K\"\n", indent, JSON_KEY_CAST, ptr->kind & MASK_cast);
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

	if (isInvokable(ptr)) {
		printFmt(out, NULL, JSON_OBJ_ARR_START, indent, JSON_KEY_ARGS);
		for (symn arg = ptr->params; arg; arg = arg->next) {
			if (arg != ptr->params) {
				printFmt(out, NULL, JSON_OBJ_NEXT, indent);
			}
			jsonDumpSym(out, esc, arg, NULL, indent + 1);
		}
		printFmt(out, NULL, JSON_OBJ_ARR_END, indent);
	}
	printFmt(out, esc, "%I, \"%s\": %u\n", indent, JSON_KEY_SIZE, ptr->size);
	printFmt(out, esc, "%I, \"%s\": %u\n", indent, JSON_KEY_OFFS, ptr->offs);
	printFmt(out, esc, "%I, \"%s\": %s\n", indent, JSON_KEY_STAT, isStatic(ptr) ? JSON_VAL_TRUE : JSON_VAL_FALSE);
	printFmt(out, esc, "%I, \"%s\": %s\n", indent, JSON_KEY_CNST, isConst(ptr) ? JSON_VAL_TRUE : JSON_VAL_FALSE);
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
		case OPER_sel:      // '?:'

		case OPER_com:      // ','

		case INIT_set:      // '='
		case ASGN_set:      // '='
			jsonDumpAst(out, esc, ast->op.test, JSON_KEY_TEST, indent + 1);
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
static void jsonDumpAsm(FILE *out, const char **esc, symn sym, rtContext rt, int indent) {
	static const char *const JSON_KEY_OPC = "instruction";
	static const char *const JSON_KEY_CODE = "code";

	size_t end = sym->offs + sym->size;
	for (size_t pc = sym->offs, n = pc; pc < end; pc = n) {
		unsigned char *ip = nextOpc(rt, &n, NULL);
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
static void dumpApiJSON(userContext ctx, symn sym) {
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

static vmError jsonProfile(dbgContext ctx, vmError error, size_t ss, void *stack, size_t caller, size_t callee) {
	if (error == noError && callee != 0) {
		clock_t ticks = clock();
		userContext usr = ctx->rt->usr;
		FILE *out = usr->out;
		const char **esc = usr->esc;
		if ((ssize_t)callee < 0) {
			printFmt(out, esc, "%d,%d,%d%s", ticks, ctx->usedMem, -1, ss > 0 ? "," : "");
		} else {
			printFmt(out, esc, "%d,%d,%d,", ticks, ctx->usedMem, (vmOffs) callee);
		}
	}
	(void)caller;
	(void)stack;
	return error;
}
static void jsonPostProfile(dbgContext ctx) {
	static const char *const JSON_KEY_FUNC = "functions";
	static const char *const JSON_KEY_STMT = "statements";
	static const char *const JSON_KEY_TIME = "time";
	static const char *const JSON_KEY_TOTAL = "total";
	static const char *const JSON_KEY_HITS = "hits";
	static const char *const JSON_KEY_FAILS = "fails";

	userContext usr = ctx->rt->usr;
	FILE *out = usr->out;
	const int indent = usr->indent;
	const char **esc = usr->esc;

	size_t covFunc = 0, nFunc = ctx->functions.cnt;
	size_t covStmt = 0, nStmt = ctx->statements.cnt;
	dbgn dbg = (dbgn) ctx->functions.ptr;

	printFmt(out, esc, JSON_ARR_END, indent);

	printFmt(out, esc, JSON_OBJ_ARR_START, indent, JSON_KEY_FUNC);
	for (size_t i = 0; i < nFunc; ++i, dbg++) {
		symn sym = dbg->func;
		if (dbg->hits == 0) {
			// skip functions not invoked
			continue;
		}
		covFunc += 1;
		if (sym == NULL) {
			sym = rtLookup(ctx->rt, dbg->start, 0);
		}
		if (covFunc > 1) {
			printFmt(out, esc, JSON_OBJ_NEXT, indent);
		}
		jsonDumpSym(out, esc, sym, NULL, indent + 1);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_HITS, dbg->hits);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TIME, dbg->time);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TOTAL, dbg->total);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_FAILS, dbg->fails);
	}
	printFmt(out, esc, JSON_OBJ_ARR_END, indent);

	printFmt(out, esc, JSON_OBJ_ARR_START, indent, JSON_KEY_STMT);
	dbg = (dbgn) ctx->statements.ptr;
	for (size_t i = 0; i < nStmt; ++i, dbg++) {
		size_t symOffs = 0;
		symn sym = dbg->func;
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
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_HITS, dbg->hits);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_TOTAL, dbg->total);
		printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_FAILS, dbg->fails);
		if (dbg->file != NULL && dbg->line > 0) {
			printFmt(out, esc, "%I, \"%s\": \"%s\"\n", indent + 1, JSON_KEY_FILE, dbg->file);
			printFmt(out, esc, "%I, \"%s\": %d\n", indent + 1, JSON_KEY_LINE, dbg->line);
		}
	}
	printFmt(out, esc, JSON_OBJ_ARR_END, indent);

	printFmt(out, esc, "%I, \"%s\": %d\n", indent, "ticksPerSec", CLOCKS_PER_SEC);
	printFmt(out, esc, "%I, \"%s\": %d\n", indent, "functionCount", ctx->functions.cnt);
	printFmt(out, esc, "%I, \"%s\": %d\n", indent, "statementCount", ctx->statements.cnt);
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

	printFmt(out, esc, JSON_ARR_START, indent, "callTree");
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
static void textDumpMem(dbgContext dbg, void *ptr, size_t size, char *kind) {
	rtContext rt = dbg->rt;
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
			value = (char *) rt->vm.cell - var->offs;
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

static void textPostProfile(userContext usr) {
	static const double CLOCKS_PER_MILLIS = CLOCKS_PER_SEC / 1000.;
	const char **esc = usr->esc;
	FILE *out = usr->out;
	rtContext rt = usr->rt;
	const char *prefix = usr->compileSteps ? usr->compileSteps : "\n---------- ";

	if (usr != rt->usr) {
		fatal(ERR_INTERNAL_ERROR);
		return;
	}

	if (rt->dbg != NULL) {
		dbgContext dbg = rt->dbg;
		if (usr->profFunctions) {
			size_t cov = 0, err = 0;
			size_t cnt = dbg->functions.cnt;
			for (size_t i = 0; i < cnt; ++i) {
				dbgn ptrFunc = (dbgn) dbg->functions.ptr + i;
				cov += ptrFunc->hits > 0;
				err += ptrFunc->fails > 0;
			}

			printFmt(out, esc, "%?sProfile functions: %d/%d, coverage: %.2f%%, failures: %d\n",
				prefix, cov, cnt, cov * 100. / (cnt ? cnt : 1), err
			);
			for (size_t i = 0; i < cnt; ++i) {
				dbgn ptr = (dbgn) dbg->functions.ptr + i;
				if (ptr->hits == 0 && !usr->profNotExecuted) {
					continue;
				}
				symn sym = ptr->func;
				if (sym == NULL) {
					sym = rtLookup(rt, ptr->start, 0);
				}
				if (sym == NULL || !canDump(usr, sym)) {
					continue;
				}
				double timeTotal = ptr->total / CLOCKS_PER_MILLIS;
				double timeOther = (ptr->total - ptr->time) / CLOCKS_PER_MILLIS;
				printFmt(out, esc, "%?s:%?u:[.%06x, .%06x): hits(%D%?+D), time(%.3F%?.3F ms): %?T\n",
					ptr->file, ptr->line, ptr->start, ptr->end, ptr->hits, -ptr->fails, timeTotal, -timeOther, sym
				);
			}
		}
		if (usr->profStatements) {
			size_t cov = 0, err = 0;
			size_t cnt = dbg->statements.cnt;
			dbgn ptrStmt = (dbgn) dbg->statements.ptr;
			for (size_t i = 0; i < cnt; ++i, ptrStmt++) {
				cov += ptrStmt->hits > 0;
				err += ptrStmt->fails > 0;
			}
			printFmt(out, esc, "%?sProfile statements: %d/%d, coverage: %.2f%%, failures: %d\n",
				prefix, cov, cnt, cov * 100. / (cnt ? cnt : 1), err
			);
			for (size_t i = 0; i < cnt; ++i) {
				dbgn ptr = (dbgn) dbg->statements.ptr + i;
				if (ptr->hits == 0 && !usr->profNotExecuted) {
					continue;
				}
				symn sym = ptr->func;
				if (sym == NULL) {
					sym = rtLookup(rt, ptr->start, 0);
				}
				size_t symOffs = 0;
				if (sym != NULL) {
					symOffs = ptr->start - sym->offs;
				}
				if (sym != NULL && !canDump(usr, sym)) {
					continue;
				}
				printFmt(out, esc, "%?s:%?u:[.%06x, .%06x) hits(%D), instructions(%D%?+D): <%?.T+%d>\n",
					ptr->file, ptr->line, ptr->start, ptr->end, ptr->hits, ptr->total, -ptr->fails, sym, symOffs
				);
			}
		}
	}

	if (usr->dmpGlobals) {
		printFmt(out, esc, "%?sGlobals:\n", prefix);
		printFields(out, esc, rt->main, usr);
	}

	if (usr->dmpMemoryUse > 0) {
		struct dbgContextRec tmp = {0};
		dbgContext dbg = rt->dbg;
		if (dbg == NULL) {
			tmp.rt = rt;
			dbg = &tmp;
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

		if (usr->dmpMemoryUse > 1) {
			// show allocated and free memory chunks.
			printFmt(out, esc, "%?sHeap memory:\n", prefix);
			rtAlloc(rt, NULL, 0, textDumpMem);
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
	static const char *escape[256] = {0};
	if (escape[0] == NULL) {
		escape[0] = "\0";
		escape['\n'] = "\\n";
		escape['\r'] = "\\r";
		escape['\t'] = "\\t";
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
		printSym(out, escape, sym, prSymQual|prSymArgs|prSymType, 0);
	}
	else {
		printSym(out, escape, sym, prSymQual|prSymArgs, 0);
	}
	if (sym->doc != NULL && *sym->doc && sym->doc != sym->name) {
		printFmt(out, escape, " # %s", sym->doc);
	}
	printFmt(out, escape, "\n");
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ console debugger and profiler
static void traceMethods(userContext usr, size_t ss, size_t callee) {
	if (!usr->traceMethods) {
		return;
	}

	rtContext rt = usr->rt;
	const char **esc = usr->esc;
	FILE *out = usr->out;

	if (usr->traceTime) {
		double now = clock() * 1000. / CLOCKS_PER_SEC;
		printFmt(out, esc, "[% 3.2F]", now);
	}
	if ((ssize_t) callee > 0) {
		printFmt(out, esc, "% I > %T\n", ss, rtLookup(rt, callee, 0));
	}
	else {
		printFmt(out, esc, "% I < return\n", ss);
	}
}
static void traceOpcodes(userContext usr, size_t ss, void *stack, size_t caller, dbgn dbg) {
	if (!usr->traceOpcodes) {
		return;
	}

	rtContext rt = usr->rt;
	const char **esc = usr->esc;
	FILE *out = usr->out;

	if (usr->traceLocals && caller != rt->main->offs) {
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

	if (usr->dmpAsmStmt && dbg && dbg->start == caller) {
		textDumpDbg(out, esc, usr, NULL, dbg, 0);
	}
	if (usr->traceTime) {
		double now = clock() * 1000. / CLOCKS_PER_SEC;
		printFmt(out, esc, "[% 3.2F]: ", now);
	}
	textDumpAsm(out, esc, caller, usr, 0);
}

static vmError dbgProfile(dbgContext ctx, vmError error, size_t ss, void *sp, size_t caller, size_t callee) {
	rtContext rt = ctx->rt;
	userContext usr = rt->usr;

	dbgn dbg = mapDbgStatement(rt, caller, NULL);
	if (error != noError) {
		if (dbg != NULL) {
			dbg->fails += 1;
		}
		// abort the execution
		return error;
	}

	// function call or return.
	if (callee != 0) {
		/* TODO: measure execution time here
		clock_t now = clock();
		if (callee < 0) {
			dbgn calleeFunc = mapDbgFunction(ctx->rt, -callee);
			if (calleeFunc != NULL) {
				clock_t ticks = now - tp->func;
				if (callee == -1) {
					calleeFunc->fails += 1;
				}
				if (calleeFunc->hits > calleeFunc->returns) {
					// we are inside a recursive function
					calleeFunc->total += ticks;
				}
				calleeFunc->time += ticks;
				calleeFunc->returns -= 1;
			}

			dbgn callerFunc = mapDbgFunction(ctx->rt, caller);
			if (callerFunc != NULL) {
				clock_t ticks = now - tp->func;
				callerFunc->time -= ticks;
			}
			return dbg;
		}

		dbgn calleeFunc = mapDbgFunction(ctx->rt, callee);
		if (calleeFunc != NULL) {
			calleeFunc->hits += 1;
			calleeFunc->time = now;
		}// */
		traceMethods(usr, ss, callee);
		return noError;
	}

	// statement / instruction execution.
	if (dbg != NULL) {
		if (caller == dbg->start) {
			dbg->hits += 1;
		}
		dbg->total += 1;
	}

	traceOpcodes(usr, ss, sp, caller, dbg);
	return noError;
}
static vmError dbgDebug(dbgContext ctx, vmError error, size_t ss, void *sp, size_t caller, size_t callee) {
	rtContext rt = ctx->rt;
	userContext usr = rt->usr;

	if (callee != 0) {
		// TODO: enter will break after executing call?
		// TODO: leave will break after executing return?
		if (usr->dbgCommand == dbgStepInto && (ssize_t) callee > 0) {
			usr->dbgNextBreak = (size_t) -1;
		}
		else if (usr->dbgCommand == dbgStepOut && (ssize_t) callee < 0) {
			usr->dbgNextBreak = (size_t) -1;
		}
		traceMethods(usr, ss, callee);
		return noError;
	}

	dbgn dbg = mapDbgStatement(rt, caller, NULL);
	brkMode breakMode = brkSkip;
	char *breakCause = NULL;

	if (error == divisionByZero) {
		// keep quiet performing floating point division by zero
		if (testOcp(rt, caller, f64_div, NULL)) {
			error = noError;
		}
		if (testOcp(rt, caller, f32_div, NULL)) {
			error = noError;
		}
	}
	if (error != noError) {
		breakCause = vmErrorMessage(error);
		if (isChecked(ctx)) {
			breakMode = usr->dbgOnCaught;
		}
		else {
			breakMode = usr->dbgOnError;
		}
	}
	else if (usr->dbgNextBreak == (size_t) -1) {
		breakMode = brkPause;
		breakCause = "Break";
	}
	else if (usr->dbgNextBreak == caller) {
		breakMode = brkPause;
		breakCause = "Break";
	}
	else if (dbg != NULL) {
		if (caller == dbg->start) {
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
	if (valPrint && valPrint->end == caller) {
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
	traceOpcodes(usr, ss, sp, caller, dbg);

	// print error type
	if (breakMode & brkPrint) {
		symn fun = rtLookup(rt, caller, 0);
		size_t funOffs = caller;
		if (fun != NULL) {
			funOffs -= fun->offs;
		}
		// print current file position
		if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
			printFmt(out, esc, "%s:%u: ", dbg->file, dbg->line);
		}
		printFmt(out, esc, ERR_EXEC_INSTRUCTION"\n", breakCause, caller, fun, funOffs, vmPointer(rt, caller));
		if (out != stdout && out != stderr) {
			// print the error message also to the console
			if (dbg != NULL && dbg->file != NULL && dbg->line > 0) {
				printFmt(con, esc, "%s:%u: ", dbg->file, dbg->line);
			}
			printFmt(con, esc, ERR_EXEC_INSTRUCTION"\n", breakCause, caller, fun, funOffs, vmPointer(rt, caller));
		}
	}
	if (breakMode & brkTrace) {
		traceCalls(ctx, out, 1, rt->traceLevel, 0);
	}

	// pause execution in debugger
	for ( ; breakMode & brkPause; ) {
		dbgMode cmd = usr->dbgCommand;
		char buff[1024], *arg = NULL;

		printFmt(con, esc, ">dbg[%?c] %.A: ", cmd, vmPointer(rt, caller));
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
				traceCalls(ctx, con, 1, rt->traceLevel, 0);
				break;

			case dbgPrintInstruction:
				textDumpAsm(con, esc, caller, usr, 0);
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
					symn sym = ccLookup(rt, NULL, arg);
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
static void dumpVmOpc(const char *error, const struct opcodeRec *info) {
	FILE *out = stdout;
	printFmt(out, NULL, "### Instruction `%s`\n", info->name);
	if (error != NULL) {
		printFmt(out, NULL, "Error: `%s`\n\n", error);
		return;
	}
	printFmt(out, NULL, "\nPerforms [todo]\n");
	printFmt(out, NULL, "\n* Instruction code: `0x%02x`", info->code);
	printFmt(out, NULL, "\n* Instruction length: %d byte%?c", info->size, info->size == 1 ? 0 : 's');

	printFmt(out, NULL, "\n* Requires %d operand%?c: […", info->stack_in, info->stack_in == 1 ? 0 : 's');
	for (unsigned i = 0; i < info->stack_in; ++i) {
		printFmt(out, NULL, ", `%c`", 'a' + i);
	}
	printFmt(out, NULL, ", [todo]\n");

	printFmt(out, NULL, "* Returns %d value%?c: […", info->stack_out, info->stack_in == 1 ? 0 : 's');
	for (unsigned i = 0; i < info->stack_out; ++i) {
		printFmt(out, NULL, ", `%c`", 'a' + i);
	}
	printFmt(out, NULL, ", [todo]\n");

	printFmt(out, NULL, "\n```\n[todo]\n```\n");
	printFmt(out, NULL, "\n");
}

static int usage() {
	const char *USAGE = \
		"cmpl [global options] [files with options]..."
		"\n"
		"\n<global options>:"
		"\n"
		"\n  -run[*]               run at full speed, but without: debug information, stacktrace, bounds checking, ..."
		"\n    /g /G               dump global variable values (/G includes builtin types and functions)"
		"\n    /m /M               dump memory usage (/M includes heap allocations)"
		"\n"
		"\n  -debug[*]             run with attached debugger, pausing on uncaught errors and break points"
		"\n    /g /G               dump global variable values (/G includes builtin types and functions)"
		"\n    /m /M               dump memory usage (/M includes heap allocations)"
		"\n    /p /P               dump caught errors (/P includes stacktrace)"
		"\n    /t /T               dump executed instructions (/T includes values on stack)"
		"\n    /s /S               pause on any error (/S pauses on startup)"
		"\n"
		"\n  -profile[*]           run code with profiler: coverage, method tracing"
		"\n    /g /G               dump global variable values (/G includes builtin types and functions)"
		"\n    /m /M               dump memory usage (/M includes heap allocations)"
		"\n    /p /P               dump executed statements (/P include all functions and statements)"
		"\n    /t /T               trace execution with timestamps (/T includes instructions)"
		"\n"
		"\n  -std<file>            specify custom standard library file (empty file name disables std library compilation)."
		"\n"
		"\n  -mem<int>[kKmMgG]     override memory usage for compiler and runtime(heap)"
		"\n"
		"\n  -log[*]<int> <file>   set logger for: compilation errors and warnings, runtime debug messages"
		"\n    <int>               set the default log(warning) level 0 - 15"
		"\n    /a                  append to the log file"
		"\n    /d                  dump to the log file"
		"\n"
		"\n  -dump[?] <file>       set output for: dump(symbols, assembly, abstract syntax tree, coverage, call tree)"
		"\n    .scite              dump api for SciTE text editor"
		"\n    .json               dump api and profile data in javascript object notation format"
		"\n"
		"\n  -api[*]               dump symbols"
		"\n    /a /A               include all library symbols(/A includes builtins)"
		"\n    /d                  dump details of symbol"
		"\n    /p                  dump params and fields"
		"\n"
		"\n  -doc[*]               dump documentation"
		"\n    /a /A               include all library symbols(/A includes builtins)"
		"\n    /d                  dump details of symbol"
		"\n    /p                  dump params and fields"
		"\n"
		"\n  -use[*]               dump usages"
		"\n    /a /A               include all library symbols(/A includes builtins)"
		"\n    /d                  dump details of symbol"
		"\n    /p                  dump params and fields"
		"\n"
		"\n  -asm[*]<int>          dump assembled code: jmp +80"
		"\n    /a /A               include all library symbols(/A includes builtins)"
		"\n    /d                  dump details of symbol"
		"\n    /p                  dump params and fields"
		"\n    /g                  use global address: jmp @0x003d8c"
		"\n    /n                  prefer names over addresses: jmp <main+80>"
		"\n    /s                  print source code statements"
		"\n"
		"\n  -ast[*]               dump syntax tree"
		"\n    /a /A               include all library symbols(/A includes builtins)"
		"\n    /d                  dump details of symbol"
		"\n    /p                  dump params and fields"
		"\n    /t                  dump sub-expression type information"
		"\n    /l                  do not expand statements (print on single line)"
		"\n    /b                  don't keep braces ('{') on the same line"
		"\n    /e                  don't keep `else if` constructs on the same line"
		"\n"
		"\n<files with options>: filename followed by switches"
		"\n  <file>                if file extension is (.so|.dll|.wasm) load as library else compile"
		"\n  -w[a|<int>]           set or disable warning level for current file"
		"\n  -b[*]<int>            break point on <int> line in current file"
		"\n    /[p|P]              print only, do not pause (/P includes stack trace)"
		"\n    /[v]                after execution evaluate the top of the stack (partially implemented)"
		"\n    /o                  one shot breakpoint, disable after first hit"
		"\n"
		"\nexamples:"
		"\n"
		"\n>app -api -dump.json api.json"
		"\n    dump builtin symbols to `api.json` file (types, functions, variables, aliases)"
		"\n"
		"\n>app -run test.all.ci"
		"\n    compile and execute the file `test.all.ci`"
		"\n"
		"\n>app -debug gl.so -w0 gl.ci -w0 test.ci -wa -b12 -b15 -b/t/19"
		"\n    execute in debug mode"
		"\n    import `gl.so`"
		"\n        with no warnings"
		"\n    compile `gl.ci`"
		"\n        with no warnings"
		"\n    compile `test.ci`"
		"\n        show all warnings"
		"\n        break execution on lines 12 and 15"
		"\n        print message when line 19 is hit"
		"\n\n";
	fputs(USAGE, stdout);
	fprintf(stdout, "Version: %d, more details at: https://karetkaz.github.io/Cmpl\n", cmplVersion());
	return 0;
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
		if (strcmp(argv[1], "--test-vm") == 0) {
			return vmSelfTest(dumpVmOpc);
		}
	}

	struct userContextRec extra = {0};
	extra.in = stdin;
	extra.out = stdout;
	extra.dmpSymbols = 1;  // by default print symbols from inlined files also
	extra.dbgCommand = dbgResume;	// last command: resume
	extra.dbgOnError = brkPause | brkPrint | brkTrace;
	extra.dbgOnCaught = brkSkip;

	struct {
		// optimizations
		int foldConstants;
		int foldInstruction;
		int genStaticGlobals;
		int errPrivateAccess;
		int errUninitialized;
		int preferNativeCalls;
		int warnLevel;	// compile log level
		int raiseLevel;	// runtime log level

		size_t memory;
	} settings = {
		// use all optimizations by default
		.foldConstants = 1,
		.foldInstruction = 1,
		.genStaticGlobals = 1,
		.errPrivateAccess = 1,
		.errUninitialized = 1,
		.preferNativeCalls = 1,
		.warnLevel = 5,
		.raiseLevel = 15,

		// 128 Mb memory compiler + runtime
		.memory = 128 << 20
	};

	const char *stdlib = STDLIB;
	const char *cmpl_home = getenv(CMPL_HOME);

	ccInstall install = install_def;
	char *ccFile = NULL;			// compiled filename

	char *logFileName = NULL;		// logger filename
	int logAppend = 0;				// do not clear the log file

	char *dumpFileName = NULL;      // dump file
	char *pathAstDumpXml = NULL;    // dump ast to xml file before code generation
	void (*dumpFun)(userContext, symn) = NULL;
	enum { run, trace, debug, profile, compile } run_code = compile;

	// TODO: max 32 break points ?
	brkMode bp_type[32];
	char *bp_file[32];
	int bp_line[32];
	int bp_size = 0;

	int i;
	// process global options
	for (i = 1; i < argc; ++i) {
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
						// this slows down execution only 20 times,
						// debug and profile are 200 times slower
						run_code = trace;
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
						extra.profFunctions = 1;
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
				stdlib = arg + 4;
			}
			else {
				// disable standard library
				stdlib = NULL;
			}
		}
		// override heap size
		else if (strncmp(arg, "-mem", 4) == 0) {
			int value = -1;
			const char *arg2 = arg + 4;
			if (*arg2) {
				arg2 = parseInt(arg2, &value, 10);
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
			if (*arg2 && *parseInt(arg2, &settings.warnLevel, 10)) {
				fatal("invalid argument '%s'", arg);
				return -1;
			}
			if (settings.warnLevel > 15) {
				settings.warnLevel = 15;
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
				dumpFun = dumpApiJSON;
			}
			else if (strEquals(arg, "-dump.scite")) {
				dumpFun = dumpApiSciTE;
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
					settings.foldConstants = on;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "fast")) {
					settings.foldInstruction = on;
					arg2 += 5;
				}
				else if (strBegins(arg2 + 1, "glob")) {
					settings.genStaticGlobals = on;
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
				else if (strBegins(arg2 + 1, "init")) {
					settings.errUninitialized = !on;
					arg2 += 8;
				}
				else if (strBegins(arg2 + 1, "public")) {
					settings.errPrivateAccess = !on;
					arg2 += 7;
				}
				else if (strBegins(arg2 + 1, "offsets")) {
					extra.hideOffsets = !on;
					arg2 += 8;
				}
				else if (strBegins(arg2 + 1, "steps")) {
					extra.compileSteps = on ? "\n---------- " : NULL;
					arg2 += 6;
				}
				else if (strBegins(arg2 + 1, "native")) {
					settings.preferNativeCalls = on;
					arg2 += 7;
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

	if (dumpFun == dumpApiSciTE && !extra.dmpDoc) {
		// record documentation for api
		extra.dmpDoc = 1;
	}
	if (extra.hideOffsets) {
		extra.dmpMode &= ~(prAsmCode|prRelOffs|prAbsOffs);
	}

	// initialize runtime context
	rtContext rt = rtInit(NULL, settings.memory);
	if (rt == NULL) {
		fatal("initializing runtime context");
		return -1;
	}

	extra.rt = rt;
	rt->usr = &extra;

	// override log level
	rt->logLevel = settings.warnLevel;

	// override constant folding
	rt->foldCasts = settings.foldConstants != 0;
	rt->foldConst = settings.foldConstants != 0;

	// override instruction folding
	rt->foldInstr = settings.foldInstruction != 0;
	rt->foldMemory = settings.foldInstruction != 0;
	rt->foldAssign = settings.foldInstruction != 0;

	// open the log file first (enabling dump appending)
	if (logFileName && !logFile(rt, logFileName, logAppend)) {
		info(rt, NULL, 0, ERR_OPENING_FILE, logFileName);
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
	ccContext cc = ccInit(rt, install, NULL);
	if (cc == NULL) {
		fatal("initializing compiler context");
		logFile(rt, NULL, 0);
		return -6;
	}
	cc->genDocumentation = extra.dmpDoc;
	cc->genStaticGlobals = settings.genStaticGlobals != 0;
	cc->errPrivateAccess = settings.errPrivateAccess != 0;
	cc->errUninitialized = settings.errUninitialized != 0;
	cc->home = ccUniqueStr(cc, cmpl_home, -1, -1);

	if (install & installLibs) {
		// install standard library.
		if (extra.compileSteps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sCompile: `%?s`", extra.compileSteps, stdlib);}
		char buffer[1024];
		if (stdlib != NULL) {
			snprintf(buffer, sizeof(buffer), "inline \"%s\";", stdlib);
		} else {
			buffer[0] = 0;
		}
		if (!ccDefInt(cc, "preferNativeCalls", settings.preferNativeCalls)) {
			error(rt, NULL, 0, "error registering `preferNativeCalls`");
			return -1;
		}
		if (cmplInit(rt) != 0) {
			error(rt, NULL, 0, "error registering standard library");
			return -1;
		}
		if (!ccAddUnit(cc, stdlib, 0, buffer)) {
			error(rt, NULL, 0, "error registering standard library");
			return -1;
		}
	}

	// compile and import files / modules
	for (; i <= argc; ++i) {
		char *arg = argv[i];
		if (i == argc || *arg != '-') {
			if (ccFile != NULL) {
				char *ext = strrchr(ccFile, '.');
				if (ext && (strEquals(ext, ".so") || strEquals(ext, ".dll") || strEquals(ext, ".wasm") || strEquals(ext, ".dylib"))) {
					if (extra.compileSteps != NULL && ccFile != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sLibrary: `%?s`", extra.compileSteps, ccFile);}
					int resultCode = importLib(rt, ccFile);
					if (resultCode != 0) {
						fatal("error(%d) importing library `%s`", resultCode, ccFile);
					}
				}
				else {
					if (extra.compileSteps != NULL && ccFile != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sCompile: `%?s`", extra.compileSteps, ccFile);}
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
			rt->logLevel = settings.warnLevel;
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
				if (bp_size > (int)lengthOf(bp_file)) {
					info(rt, NULL, 0, "can not add more than %d breakpoints.", lengthOf(bp_file));
				}
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
				bp_type[bp_size] = type;
				bp_file[bp_size] = ccFile;
				bp_line[bp_size] = line;
				bp_size += 1;
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
			return -1;
		}
		dumpAstXML(xmlFile, escapeXML(), cc->root, prDbg, 0, "main");
		fclose(xmlFile);
	}

	// generate code only if there are no compilation errors
	if (rt->errors == 0) {
		if (extra.compileSteps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sGenerate: byte-code", extra.compileSteps);}
		if (ccGenCode(cc, run_code != run || extra.dmpAsmStmt) != 0) {
			trace("error generating code");
		}
		// set breakpoints
		for (i = 0; i < bp_size; ++i) {
			char *file = bp_file[i];
			int line = bp_line[i];
			brkMode type = bp_type[i];
			dbgn dbg = getDbgStatement(rt, file, line);
			if (dbg == NULL) {
				info(rt, file, line, "invalid breakpoint");
				continue;
			}

			if (type & brkValue && dbg->stmt && dbg->stmt->kind == TOKEN_var) {
				dbg->decl = dbg->stmt->id.link;
			}
			dbg->bp = type;
		}
	}

	if (dumpFun == NULL) {
		if (extra.dmpApi || extra.dmpDoc || extra.dmpAsm || extra.dmpAst || extra.dmpUse) {
			dumpFun = dumpApiText;
		}
	}

	if (rt->errors == 0) {
		if (dumpFun == dumpApiJSON) {
			extra.esc = escapeJSON();
			printFmt(extra.out, NULL, "{\n");
			printFmt(extra.out, extra.esc, "%I\"%s\": \"%d\"\n", extra.indent, JSON_KEY_VERSION, cmplVersion());
			printFmt(extra.out, extra.esc, JSON_OBJ_ARR_START, extra.indent, JSON_KEY_SYMBOLS);
			dumpApi(rt, &extra, dumpApiJSON);
			printFmt(extra.out, extra.esc, JSON_OBJ_ARR_END, extra.indent);
		}
		else if (dumpFun != NULL) {
			extra.esc = NULL;
			if (extra.compileSteps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sSymbols:", extra.compileSteps);}
			dumpApi(rt, &extra, dumpFun);
		}
	}

	// run code if there are no compilation errors.
	if (rt->errors == 0 && run_code != compile) {
		rt->logLevel = settings.raiseLevel;
		if (rt->dbg != NULL) {
			// set debugger or profiler
			if (run_code == trace) {
				rt->dbg->debug = &dbgError;
			}
			else if (run_code == debug) {
				rt->dbg->debug = &dbgDebug;
				if (extra.dbgNextBreak != 0) {
					// break on first instruction
					extra.dbgNextBreak = rt->vm.pc;
				}
			}
			else if (run_code == profile) {
				// set call tree dump method
				if (dumpFun == dumpApiJSON) {
					rt->dbg->debug = &jsonProfile;
					jsonPreProfile(rt->dbg);
				}
				else {
					rt->dbg->debug = &dbgProfile;
				}
			}
		}
		if (!extra.dmpAsm && extra.traceOpcodes) {
			extra.dmpMode |= prRelOffs | 9;
			extra.dmpAsmStmt = 1;
		}
		if (extra.compileSteps != NULL) {printLog(extra.rt, raisePrint, NULL, 0, "%sExecute: byte-code", extra.compileSteps);}
		long start = clock();
		if (execute(rt, 0, NULL, NULL) == noError) {
			// clear caught errors
			rt->errors = 0;
		}
		else {
			rt->errors = 1;
		}
		extra.rtTime = clock() - start;
	}

	if (rt->errors == 0) {
		if (dumpFun == dumpApiJSON) {
			if (rt->dbg != NULL) {
				jsonPostProfile(rt->dbg);
			}
			printFmt(extra.out, NULL, "}\n");
		}
		else if (run_code != compile) {
			// dump results: global variables, memory allocations
			textPostProfile(&extra);
		}
	}

	if (extra.compileSteps != NULL) {
		clock_t runTime = extra.hideOffsets ? 0 : extra.rtTime;
		printLog(extra.rt, raisePrint, NULL, 0, "%sExitcode: %d, time: %.3F ms",
			extra.compileSteps, rt->errors,
			runTime  * 1000. / CLOCKS_PER_SEC
		);
	}

	if (extra.out && extra.closeOut) {
		fclose(extra.out);
	}

	return rtClose(rt) != 0;
	(void) cmplUnit;
	(void) cmplClose;
}
