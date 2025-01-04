/**
 * Formatted printing of:
 * Symbols
 * Tokens
 * Instructions
 */
#ifndef CMPL_PRINTER_H
#define CMPL_PRINTER_H

#include <cmpl.h>
/* Debugging the compiler:
	if not defined, no extra messages and extra checks are performed.
	0: show where errors were raised
	1: trace errors to their root
	2: include debug messages
	3: include debug messages from code-generator
	4: include debug messages from code-emitter
*/
//#define DEBUGGING 0

typedef enum {
	prAsmCode = 0x00000f,   // print code bytes (0-15)
	prAsmOffs = 0x000010,   // print offset of the instruction
	prRelOffs = 0x000020,   // print relative offsets: <main+80>
	prAbsOffs = 0x000040,   // print absolute offset: <main@041048>

	prSymQual = 0x000100,   // print qualified symbol names.
	prSymAttr = 0x000200,   // print attributes: const, static, member
	prSymType = 0x000400,   // print variable type, function return type, typename base type.
	prSymInit = 0x000800,   // print variable initializer, function body, typename fields.
	prSymArgs = 0x001000,   // print functions parameters, typename fields.

	prAstType = 0x010000,   // print type cast of each subexpression
	nlAstBody = 0x020000,   // print compound statements on new line (like in cs)
	nlAstElIf = 0x040000,   // don't keep `else if` constructs on the same line.
	prMinified= 0x100000,   // enable minified printing as: `} else {`, `}, {`, `1, 2`
	prOneLine = 0x200000,   // force on a single line (skip: function body, typename fields, statement blocks, ...)

	prName = 0,  // print operator or symbol name only: %.t, %.T
	prShort = prOneLine | prSymQual | prSymArgs ,	// %t, %T
	prDetail = prSymAttr | prSymQual | prSymArgs | prSymType | prSymInit,		// %±t, %±T

	prGlobal = prMinified | prSymQual | prSymType,  // mode to print values of global variable
	prMember = prMinified,  // mode to print values of record members
	prArgs = prMinified,  // mode to print values of arguments in stack trace
	prDbg = prSymAttr | prAstType | prSymQual | prSymArgs | prSymType | prSymInit | prRelOffs | 9 | prOneLine
} dmpMode;

/**
 * Print the given symbol to the output stream.
 *
 * @param out Output stream.
 * @param esc Escape translation (format string will be not escaped).
 * @param sym the symbol to be printed.
 * @param mode How to print.
 * @param indent Indentation level. (Used for struct members)
 */
void printSym(FILE *out, const char **esc, symn sym, dmpMode mode, int indent);

/**
 * Print the given abstract syntax tree to the output stream.
 *
 * @param out Output stream.
 * @param esc Escape translation (format string will be not escaped).
 * @param ast the abstract syntax tree to be printed.
 * @param mode How to print.
 * @param indent Indentation level. (Used for statements)
 */
void printAst(FILE *out, const char **esc, astn ast, dmpMode mode, int indent);

/**
 * Print an instruction to the output stream.
 *
 * @param out Output stream.
 * @param esc Escape translation (format string will be not escaped).
 * @param ctx optional runtime context.
 * @param ptr Instruction pointer.
 * @param mode How to print.
 */
void printAsm(FILE *out, const char **esc, rtContext ctx, void *ptr, dmpMode mode);

/**
 * Print an instruction to the output stream.
 *
 * @param out Output stream.
 * @param esc Escape translation (format string will be not escaped).
 * @param opc Operation code of the instruction
 * @param args Argument of the instruction
 */
void printOpc(FILE *out, const char **esc, int opc, int64_t args);

/**
 * Print the offset of a variable.
 *
 * @param out Output stream.
 * @param esc Escape output.
 * @param ctx runtime context.
 * @param sym Offset is relative to this symbol (can be null).
 * @param offs Offset to be printed.
 * @param mode Flags to modify the output format.
 */
void printOfs(FILE *out, const char **esc, rtContext ctx, symn sym, size_t offs, dmpMode mode);

/**
 * Print the value of a variable.
 *
 * @param out Output stream.
 * @param esc Escape output.
 * @param ctx runtime context.
 * @param var Variable to be printed.
 * @param ref Base offset of variable.
 * @param mode Flags to modify the output format.
 * @param indent Indentation level. (Used for members and arrays)
 */
void printVal(FILE *out, const char **esc, rtContext ctx, symn var, void *ref, dmpMode mode, int indent);

/**
 * Print formatted text to the output stream.
 *
 * @param out Output stream.
 * @param esc Escape translation (format string will be not escaped).
 * @param fmt Format text.
 * @param ... Format variables.
 * @note %[?]?[+-]?[0 ]?[0-9]*(\.([*]|[0-9]*))?[tTKAIbBoOxXuUdDfFeEsScC]
 *    skip: [?]? skip printing `(null)` or `0` (may print pad character if specified)
 *    sign: [+-]? sign flag / alignment.
 *    pad:  [0 ]? padding character.
 *    size: [0-9]* length / indent.
 *    mode: (\.[*]|[0-9]*)? precision / mode.
 *
 *    T: symbol (inline, typename, function or variable)
 *      %T: print qualified symbol name with arguments
 *      %.T: print simple symbol name
 *      %+T: expands fields on multiple lines
 *      %-T: same as '%+T', but skips first indent
 *
 *    t: token (abstract syntax tree)
 *      %t: print statement or expression
 *      %.t: print only the token
 *      %+t: expand statements on multiple lines
 *      %-t: same as '%+t', but skip first indent
 *
 *    K: symbol kind
 *    k: token kind
 *
 *    A: instruction (asm)
 *    a: offset (address)
 *
 *    I: indent
 *    b: 32-bit bin value
 *    B: 64-bit bin value
 *    o: 32-bit oct value
 *    O: 64-bit oct value
 *    x: 32-bit hex value
 *    X: 64-bit hex value
 *    u: 32-bit unsigned value
 *    U: 64-bit unsigned value
 *    d: 32-bit signed value (decimal)
 *    D: 64-bit signed value (decimal)
 *    f: 32-bit floating point value
 *    F: 64-bit floating point value
 *    e: 32-bit floating point value (Scientific notation (mantissa/exponent))
 *    E: 64-bit floating point value (Scientific notation (mantissa/exponent))
 *
 *    s: ansi string
 *    c: ansi character
 *    S: ?wide string
 *    C: ?wide character
 *
 * flags
 *    +-
 *        oOuU: ignored
 *        bBxX: lowerCase / upperCase
 *        dDfFeE: forceSign /
 */
void printFmt(FILE *out, const char **esc, const char *fmt, ...);

/**
 * Print compilation and runtime errors.
 */
void printLog(rtContext ctx, raiseLevel level, const char *file, int line, const char *msg, ...);

/**
 * Dump all accessible symbols.
 */
void dumpApi(rtContext ctx, userContext extra, void action(userContext, symn));

/**
 * Print the current stacktrace.
 *
 * @param ctx Debugger context
 * @param out The file to be used as output
 * @param indent indentation
 * @param maxCalls limit the size of the stacktrace
 * @param skipCalls offset of the first call in the stacktrace
 */
void traceCalls(dbgContext ctx, FILE *out, int indent, size_t maxCalls, size_t skipCalls);

const char **escapeStr();

// open log file for both compiler and runtime
void logFILE(rtContext ctx, FILE *file);

void print_log(rtContext ctx, raiseLevel level, const char *file, int line, struct nfcArgArr details, const char *msg, va_list vaList);

///////

#define dieif(__EXP, __FMT, ...) do { if (__EXP) { printLog(NULL, raiseFatal, __FILE__, __LINE__, "%s(" #__EXP "): " __FMT, __FUNCTION__, ##__VA_ARGS__); }} while(0)
#define logif(__EXP, __FMT, ...) do { } while(0)

#define fatal(__FMT, ...) dieif("fatal", __FMT, ##__VA_ARGS__)
#define error(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseError, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)
#define warn(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseWarn, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)
#define debug(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseDebug, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)
#define info(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raisePrint, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)
#define dbgTrace(__FMT, ...) do { } while(0)
#define dbgInfo(__FMT, ...) do { } while(0)
#define dbgCgen(__FMT, ...) do { } while(0)
#define dbgEmit(__FMT, ...) do { } while(0)
#define dbgTraceAst(__AST) dbgTrace("%.*t", prDbg, __AST)

#ifdef DEBUGGING
#undef error
#undef logif
#define error(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseError, __FILE__, __LINE__, "%?s:%?u: "__FMT, __FILE, __LINE, ##__VA_ARGS__); } while(0)
#define logif(__EXP, __FMT, ...) do { if (__EXP) { printLog(NULL, raisePrint, __FILE__, __LINE__, "%s(" #__EXP "): " __FMT, __FUNCTION__, ##__VA_ARGS__); }} while(0)

#if DEBUGGING >= 1	// enable trace
#undef dbgTrace
#define dbgTrace(__FMT, ...) logif("trace", __FMT, ##__VA_ARGS__)
#endif
#if DEBUGGING >= 2	// enable debug
#undef dbgInfo
#define dbgInfo(__FMT, ...) logif("debug.info", __FMT, ##__VA_ARGS__)
#endif
#if DEBUGGING >= 3	// enable debug cgen
#undef dbgCgen
#define dbgCgen(__FMT, ...) logif("debug.cgen", __FMT, ##__VA_ARGS__)
#endif
#if DEBUGGING >= 4	// enable debug emit
#undef dbgEmit
#define dbgEmit(__FMT, ...) logif("debug.emit", __FMT, ##__VA_ARGS__)
#endif
#endif

#define ERR_INTERNAL_ERROR "Internal Error"
#define ERR_MEMORY_OVERRUN "Memory Overrun"
#define ERR_OPENING_FILE "can not open file: %s"
#define ERR_UNIMPLEMENTED_FEATURE "Unimplemented feature"

// Lexer errors
#define ERR_EMPTY_CHAR_CONSTANT "empty character constant"
#define ERR_INVALID_CHARACTER "invalid character: '%c'"
#define ERR_INVALID_COMMENT "unterminated block comment"
#define ERR_INVALID_DOC_COMMENT "comment does not belong to a declaration"
#define ERR_INVALID_ESC_SEQ "invalid escape sequence '\\%c'"
#define ERR_INVALID_EXPONENT "no exponent in numeric constant"
#define ERR_INVALID_HEX_SEQ "hex escape sequence invalid '%c'"
#define ERR_INVALID_LITERAL "unclosed '%c' literal"
#define ERR_INVALID_STATEMENT "unterminated block statement"
#define ERR_INVALID_SUFFIX "invalid suffix in numeric constant '%s'"

// Parser errors
#define ERR_EXPECTED_BEFORE_TOK "expected `%s` before token `%?.t`"
#define ERR_EXPR_TOO_COMPLEX "expression too complex"
#define ERR_SYNTAX_ERR_BEFORE "syntax error before token `%?.t`"
#define ERR_UNEXPECTED_ATTR "unexpected attribute `%?K`"
#define ERR_UNEXPECTED_QUAL "unexpected qualifier `%.t` declared more than once"
#define ERR_UNEXPECTED_TOKEN "unexpected token `%.t`"
#define ERR_UNMATCHED_TOKEN "unexpected token `%?.t`, matching `%.k`"
#define ERR_UNMATCHED_SEPARATOR "unexpected separator `%?.t`, matching `%.k`"
#define ERR_DECLARATION_EXPECTED "declaration expected, got: `%t`"
#define ERR_INITIALIZER_EXPECTED "initializer expected, got: `%t`"
#define ERR_STATEMENT_EXPECTED "statement expected, got: `%t`"

// Type checker errors
#define ERR_DECLARATION_COMPLEX "declaration too complex: `%T`"
#define ERR_DECLARATION_REDEFINED "redefinition of `%T`"
#define ERR_UNDEFINED_DECLARATION "reference `%t` is not defined"
#define ERR_PRIVATE_DECLARATION "declaration `%T` is not public"
#define ERR_INVALID_ARRAY_LENGTH "positive integer constant expected, got `%t`"
#define ERR_INVALID_ARRAY_VALUES "array initialized with too many values, only %t expected"
#define ERR_INVALID_INHERITANCE "type must be inherited from object, got `%t`"
#define ERR_INVALID_BASE_OR_PACK "invalid struct base type or allignment, got `%t`"
#define ERR_INVALID_BASE_TYPE "invalid struct base type, got `%t`"
#define ERR_INVALID_INITIALIZER "invalid initialization: `%t`"
#define ERR_INVALID_VALUE_ASSIGN "invalid assignment: `%T` := `%t`"
#define ERR_INVALID_CONST_ASSIGN "constants can not be assigned `%t`"
#define ERR_INVALID_CONST_EXPR "constant expression expected, got: `%t`"
#define ERR_INVALID_DECLARATION "invalid declaration: `%s`"
#define ERR_INVALID_FIELD_ACCESS "object reference is required to access the member `%T`"
#define ERR_INVALID_STATIC_FIELD_INIT "static field can not be re-initialized `%t`"
#define ERR_INVALID_TYPE "can not type check expression: `%t`"
#define ERR_INVALID_CAST "can not use cast to construct instance: `%t`"
#define ERR_INVALID_OPERATOR "invalid operator %.t(%T, %T)"
#define ERR_INVALID_OPERATOR_ARG "invalid operator(%k) overload, one or two arguments required, got: %d"
#define ERR_INVALID_OPERATOR_ARGC "invalid operator(%k) overload, %d arguments required, got: %d"
#define ERR_MULTIPLE_OVERLOADS "there are %d overloads for `%t`"
#define ERR_UNIMPLEMENTED_FUNCTION "unimplemented function `%T`"
#define ERR_UNINITIALIZED_CONSTANT "uninitialized constant `%T`"
#define ERR_UNINITIALIZED_VARIABLE "uninitialized variable `%T`"
#define ERR_UNINITIALIZED_ARRAY "uninitialized array `%T`"
#define ERR_UNINITIALIZED_MEMBER "uninitialized member `%.T.%.T`"
#define ERR_PARTIAL_INITIALIZE_UNION "partial union initialization with `%.T.%.T`"

// Code generator errors
#define ERR_CAST_EXPRESSION "can not emit expression: %t, invalid cast(%K -> %K)"
#define ERR_EMIT_LENGTH "can not emit length of array: %t"
#define ERR_EMIT_VARIABLE "can not emit variable: %T"
#define ERR_EMIT_FUNCTION "can not emit function: %T"
#define ERR_EMIT_FUNCTION_NO_RET "can not emit function, missing return statement: %T"
#define ERR_EMIT_STATEMENT "can not emit statement: %t"
#define ERR_INVALID_JUMP "`%t` statement is invalid due to previous variable declaration within loop"
#define ERR_INVALID_OFFSET "invalid reference: %06x"
#define ERR_INVALID_EMIT_SIZE "can not emit assignment `%t`: variable size: %d, emited size: %d"

// Code execution errors
#define ERR_EXEC_INSTRUCTION "%s at .%06x in function: <%?.T%?+d> executing instruction: %.A"
#define ERR_EXEC_NATIVE_CALL "%s at .%06x in function: <%?.T%?+d> executing native call: %T"
#define ERR_EXEC_FUNCTION "can not execute function: %T"

#define WARN_EMPTY_STATEMENT "empty statement `%?.0t`"
#define WARN_USE_BLOCK_STATEMENT "statement should be a block statement {%t}"
//TODO: #define WARN_LOCAL_MIGHT_ESCAPE "local variable `%t` can not be referenced outside of its scope"
#define WARN_PASS_ARG_NO_CAST "argument `%t` is passed to emit without cast as `%T`"
#define WARN_PASS_ARG_BY_REF "argument `%t` is passed by reference to `%T`"
#define WARN_NO_CODE_GENERATED "no code will be generated for statement: %t"
#define WARN_PADDING_ALIGNMENT "padding `%?T` with %d bytes: (%d -> %d)"
#define WARN_ADDING_IMPLICIT_CAST "adding implicit cast %T(%t: %T)"
#define WARN_ADDING_TEMPORARY_VAR "adding temporary variable %T := %t"
#define WARN_USING_SIGNED_CAST "using signed cast for unsigned value: `%t`"
#define WARN_STATIC_FIELD_ACCESS "accessing static member using instance variable `%T`/ %T"

#define WARN_COMMENT_MULTI_LINE "multi-line comment: `%s`"
#define WARN_COMMENT_NESTED "ignoring nested comment"
#define WARN_NO_NEW_LINE_AT_END "expected <new line> before end of input"

#define WARN_MULTI_CHAR_CONSTANT "multi character constant"
#define WARN_CHR_CONST_OVERFLOW "character constant truncated"
#define WARN_OCT_ESC_SEQ_OVERFLOW "octal escape sequence overflow"
#define WARN_VALUE_OVERFLOW "value overflow"
#define WARN_EXPONENT_OVERFLOW "exponent overflow"

#define WARN_VARIABLE_TYPE_INCOMPLETE "emitted variable has incomplete type: %T"
#define WARN_USING_BEST_OVERLOAD "using overload `%T` of %d declared symbols"
#define WARN_USING_DEF_TYPE_INITIALIZER "using default type initializer: %T := %t"
#define WARN_USING_DEF_FIELD_INITIALIZER "using default field initializer: %T := %t"
#define WARN_DECLARATION_REDEFINED "variable `%T` hides previous declaration"
#define WARN_FUNCTION_TYPENAME "function name `%.t` is a type, but returns `%T`"
#define WARN_FUNCTION_OVERRIDE "Overriding virtual function: %T"
#define WARN_FUNCTION_OVERLOAD "Overwriting forward function: %T"
#define WARN_EXTENDING_NAMESPACE "Extending static namespace: %T"
#define WARN_INLINE_FILE "inline file: `%s`"
#define INFO_PREVIOUS_DEFINITION "previously defined here as `%T`"

#endif
