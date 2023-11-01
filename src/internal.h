/*******************************************************************************
 *   File: core.h
 *   Date: 2007/04/20
 *   Desc: internal header
 *******************************************************************************
 *
 *
*******************************************************************************/

#ifndef CC_INTERNAL_H
#define CC_INTERNAL_H

#include <stdarg.h>
#include "cmpl.h"
#include "parser.h"
#include "printer.h"

/* Debugging the compiler:
	0: show where errors were raised
	1: trace errors to their root
	2: include debug messages
	3: include debug messages from code generator
	4: include debug messages from code emitter
	if not defined, no extra messages and extra checks are performed.
*/
//#define DEBUGGING 0

// native function call
typedef struct libc {
	vmError (*call)(nfcContext);
	const char *proto;
	symn sym;
	size_t offs;
	size_t in, out;		// stack parameters
} *libc;

/// Bit scan forward: position of the Least Significant Bit
static inline int32_t bitsf(uint32_t value) {
	if (value == 0) {
		return -1;
	}
	int32_t result = 0;
	if ((value & 0x0000ffff) == 0) {
		result += 16;
		value >>= 16;
	}
	if ((value & 0x000000ff) == 0) {
		result += 8;
		value >>= 8;
	}
	if ((value & 0x0000000f) == 0) {
		result += 4;
		value >>= 4;
	}
	if ((value & 0x00000003) == 0) {
		result += 2;
		value >>= 2;
	}
	if ((value & 0x00000001) == 0) {
		result += 1;
	}
	return result;
}

/// Utility function to align offset
static inline size_t padOffset(size_t offs, size_t align) {
	return (offs + (align - 1)) & -align;
}

/// Utility function to align pointer
static inline void *padPointer(void *offs, size_t align) {
	return (void*)padOffset((size_t)offs, align);
}

/// Utility function to swap memory
static inline void memSwap(void *_a, void *_b, size_t size) {
	char *a = _a;
	char *b = _b;
	char *end = a + size;
	while (a < end) {
		char c = *a;
		*a = *b;
		*b = c;
		a += 1;
		b += 1;
	}
}

/// Check if the pointer is inside the vm.
static inline int isValidOffset(rtContext rt, void *ptr) {
	if ((unsigned char*)ptr > rt->_mem + rt->_size) {
		return 0;
	}
	if ((unsigned char*)ptr < rt->_mem) {
		return 0;
	}
	return 1;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const char * const pluginLibImport;
extern const char * const pluginLibInstall;
extern const char * const pluginLibDestroy;

extern const char * const type_doc_public;
extern const char * const type_fmt_signed32;
extern const char * const type_fmt_signed64;
extern const char * const type_fmt_unsigned32;
extern const char * const type_fmt_unsigned64;
extern const char * const type_fmt_float32;
extern const char * const type_fmt_float64;
extern const char * const type_fmt_character;
extern const char * const type_fmt_string;
extern const char * const type_fmt_typename;
extern const char type_fmt_string_chr;
extern const char type_fmt_character_chr;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *absolutePath(const char *path, char *buff, size_t size);

char *relativeToCWD(const char *path);

/** Calculate hash of string.
 * @brief calculate the hash of a string.
 * @param str string to be hashed.
 * @param len length of string.
 * @return hash code of the input.
 */
unsigned rehash(const char *str, size_t size);

/** Add string to string table.
 * @brief add string to string table.
 * @param cc compiler context.
 * @param str the string to be mapped.
 * @param len the length of the string, -1 recalculates using strlen.
 * @param hash pre-calculated hashcode, -1 recalculates.
 * @return the mapped string in the string table.
 */
const char *ccUniqueStr(ccContext cc, const char *str, size_t size/* = -1*/, unsigned hash/* = -1*/);

// open log file for both compiler and runtime
void logFILE(rtContext ctx, FILE *file);
FILE *logFile(rtContext ctx, char *file, int append);

/**
 * Construct reference node.
 *
 * @param cc compiler context.
 * @param name name of the node.
 * @return the new node.
 */
static inline astn tagNode(ccContext cc, const char *name) {
	astn ast = NULL;
	if (cc != NULL && name != NULL) {
		ast = newNode(cc, TOKEN_var);
		if (ast != NULL) {
			size_t len = strlen(name);
			ast->file = cc->file;
			ast->line = cc->line;
			ast->type = NULL;
			ast->id.link = NULL;
			ast->id.hash = rehash(name, len + 1) % hashTableSize;
			ast->id.name = ccUniqueStr(cc, name, len + 1, ast->id.hash);
		}
	}
	return ast;
}

/**
 * Construct arguments.
 *
 * @param cc compiler context.
 * @param lhs arguments or first argument.
 * @param rhs next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn argNode(ccContext cc, astn lhs, astn rhs) {
	if (lhs == NULL) {
		return rhs;
	}
	return opNode(cc, cc->type_vid, OPER_com, lhs, rhs);
}

/**
 * Chain the arguments through ast.next link.
 * @param args root node of arguments tree.
 */
static inline astn chainArgs(astn args) {
	if (args == NULL) {
		return NULL;
	}
	if (args->kind == OPER_com) {
		astn lhs = chainArgs(args->op.lhso);
		astn rhs = chainArgs(args->op.rhso);
		args = lhs;
		while (lhs->next != NULL) {
			lhs = lhs->next;
		}
		lhs->next = rhs;
	} else {
		args->next = NULL;
	}
	return args;
}

/**
 * Optimize an assignment by removing extra copy of the value if it is on the top of the stack.
 * replace `dup x`, `set y` with a single `mov x, y` instruction
 * replace `dup 0`, `set x` with a single `set x` instruction
 *
 * @param Runtime context.
 * @param offsBegin Begin of the byte code.
 * @param offsEnd End of the byte code.
 * @return non zero if the code was optimized.
 */
int foldAssignment(rtContext rt, size_t offsBegin, size_t offsEnd);

/**
 * Test the virtual machine instruction set(compare implementation with definition `OPCODE_DEF`).
 *
 * @param cb callback executed for each instruction.
 * @return number of errors found during the test.
 */
int vmSelfTest(void cb(const char *error, const struct opcodeRec *info));

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Plugins
int importLib(rtContext rt, const char *path);
void closeLibs(rtContext rt);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Error and warning messages
void print_log(rtContext rt, raiseLevel level, const char *file, int line, struct nfcArgArr details, const char *msg, va_list vaList);

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
#define ERR_INVALID_BASE_TYPE "invalid struct base type, got `%t`"
#define ERR_INVALID_PACK_SIZE "invalid struct pack size, got `%t`"
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
#define ERR_MULTIPLE_OVERLOADS "there are %d overloads for `%t`"
#define ERR_UNIMPLEMENTED_FUNCTION "unimplemented function `%T`"
#define ERR_UNINITIALIZED_CONSTANT "uninitialized constant `%T`"
#define ERR_UNINITIALIZED_VARIABLE "uninitialized variable `%T`"
#define ERR_UNINITIALIZED_MEMBER "uninitialized member `%.T.%.T`"
#define ERR_PARTIAL_INITIALIZE_UNION "partial union initialization with `%.T.%.T`"

// Code generator errors
#define ERR_CAST_EXPRESSION "can not emit expression: %t, invalid cast(%K -> %K)"
#define ERR_EMIT_LENGTH "can not emit length of array: %t"
#define ERR_EMIT_VARIABLE "can not emit variable: %T"
#define ERR_EMIT_FUNCTION "can not emit function: %T"
#define ERR_EMIT_STATEMENT "can not emit statement: %t"
#define ERR_INVALID_JUMP "`%t` statement is invalid due to previous variable declaration within loop"
#define ERR_INVALID_OFFSET "invalid reference: %06x"

// Code execution errors
#define ERR_EXEC_INSTRUCTION "%s at .%06x in function: <%?.T%?+d> executing instruction: %.A"
#define ERR_EXEC_NATIVE_CALL "%s at .%06x in function: <%?.T%?+d> executing native call: %T"
#define ERR_EXEC_FUNCTION "can not execute function: %T"

#define WARN_EMPTY_STATEMENT "empty statement `;`"
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


#define prerr(__TAG, __FMT, ...) do { printFmt(stdout, NULL, "%?s:%?u: %s(" __TAG "): " __FMT "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)
#define fatal(__FMT, ...) do { prerr("fatal", __FMT, ##__VA_ARGS__); abort(); } while(0)
#define dieif(__EXP, __FMT, ...) do { if (__EXP) { prerr(#__EXP, __FMT, ##__VA_ARGS__); abort(); } } while(0)

// compilation errors
#define warn(__ENV, __LEVEL, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, __LEVEL, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)
#define info(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raisePrint, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)

#ifndef DEBUGGING
#define error(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseError, __FILE, __LINE, __FMT, ##__VA_ARGS__); } while(0)
#else
// show also the line where the error is raised
#define error(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseError, __FILE__, __LINE__, "%?s:%?u: "__FMT, __FILE, __LINE, ##__VA_ARGS__); } while(0)
#define logif(__EXP, __FMT, ...) do { if (__EXP) { prerr(#__EXP, __FMT, ##__VA_ARGS__); } } while(0)

#if DEBUGGING >= 1	// enable trace
#define trace(__FMT, ...) do { prerr("trace", __FMT, ##__VA_ARGS__); } while(0)
#endif

#if DEBUGGING >= 2	// enable debug
#define debug(__FMT, ...) do { prerr("debug", __FMT, ##__VA_ARGS__); } while(0)
#endif

#if DEBUGGING >= 3	// enable debug cgen
#define dbgCgen(__FMT, ...) do { prerr("debug", __FMT, ##__VA_ARGS__); } while(0)
#endif

#if DEBUGGING >= 4	// enable debug emit
#define dbgEmit(__FMT, ...) do { prerr("debug", __FMT, ##__VA_ARGS__); } while(0)
#endif

#endif

#ifndef logif
#define logif(__EXP, __FMT, ...) do {} while(0)
#endif
#ifndef trace
#define trace(__FMT, ...) do {} while(0)
#endif
#ifndef debug
#define debug(__FMT, ...) do {} while(0)
#endif
#ifndef dbgCgen
#define dbgCgen(__FMT, ...) do {} while(0)
#endif
#ifndef dbgEmit
#define dbgEmit(__FMT, ...) do {} while(0)
#endif

#define traceAst(__AST) do { trace("%.*t", prDbg, __AST); } while(0)

// Compiler specific settings
#ifdef __WATCOMC__
#pragma disable_message(136);	// Warning! W136: Comparison equivalent to 'unsigned == 0'

#include <math.h>
static inline float fmodf(float x, float y) { return (float) fmod(x, y); }
static inline float sinf(float x) { return (float) sin((float) x); }
static inline float cosf(float x) { return (float) cos((float) x); }
static inline float tanf(float x) { return (float) tan((float) x); }
static inline float logf(float x) { return (float) log((float) x); }
static inline float expf(float x) { return (float) exp((float) x); }
static inline float powf(float x, float y) { return (float) pow((float) x, (float) y); }
static inline float sqrtf(float x) { return (float) sqrt((float) x); }
static inline float atan2f(float x, float y) { return (float) atan2((float) x, (float) y); }
#endif

#endif
