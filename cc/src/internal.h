/*******************************************************************************
 *   File: core.h
 *   Date: 2007/04/20
 *   Desc: internal header
 *******************************************************************************
 *
 *
*******************************************************************************/
#ifndef CC_INTERNAL_H
#define CC_INTERNAL_H 2

#include "vmCore.h"
#include "ccCore.h"

#include <stdlib.h> //4 abort

/* debugging the compiler:
	0: show where errors were raised
	1: trace errors to their root
	2: include debug messages
	3: print generated assembly for statements with errors
 	if not defined no extra messages and extra checks are performed.
*/
#define DEBUGGING 0

// limit the count of printed elements(stacktrace, array elements)
#define LOG_MAX_ITEMS 100

// enable parallel execution stuff
#define VM_MAX_PROCS 0

// maximum tokens in expressions & nesting level
#define TOKS 1024

// hash table size
#define TBLS 512

// helper macros
#define lengthOf(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))
#define offsetOf(__TYPE, __FIELD) ((size_t) &((__TYPE)0)->__FIELD)

typedef struct list {	// linked list
	struct list*    next;
	unsigned char*  data;
} *list;
typedef struct libc {	// library call
	struct libc *next;	// next
	vmError (*call)(libcContext);
	void *data;	// user data for this function
	symn sym;
	size_t pos, chk;
	int pop;
} *libc;

// A simple dynamic array
struct arrBuffer {
	char *ptr;		// data
	int esz;		// element size
	int cap;		// capacity
	int cnt;		// length
};
int initBuff(struct arrBuffer* buff, int initsize, int elemsize);
void freeBuff(struct arrBuffer* buff);
static inline void* getBuff(struct arrBuffer* buff, int idx) {
	int pos = idx * buff->esz;
	if (pos >= buff->cap || !buff->ptr)
		return NULL;
	return buff->ptr + pos;
}

/// Symbol node types and variables
struct symNode {
	char*	name;		// symbol name
	char*	file;		// declared in file
	int32_t	line;		// declared on line
	size_t	size;		// variable or function size.
	size_t	offs;		// address of variable or function.

	symn	type;		// base type of TYPE_ref/TYPE_arr/function (void, int, float, struct, ...)
	symn	flds;		// all fields: static + non static fields / function return value + parameters
	symn	prms;		// tail of flds: struct non static fields / function parameters
	symn	decl;		// declaring symbol (defined in ...): struct, function, ...
	symn	next;		// next symbol: field / param / ... / (in scope table)

	ccToken	kind;		// TYPE_def / TYPE_rec / TYPE_ref / TYPE_arr
	ccToken	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).

	union {				// Attributes
		uint32_t	Attr;
	struct {
		//~ uint32_t	kind:8;
		//~ uint32_t	cast:8;
		uint32_t	memb:1;		// member operator (push the object by ref first)
		uint32_t	call:1;		// callable (function/definition) <=> TODO: prms != NULL
		uint32_t	cnst:1;		// constant TODO: kind & ATTR_const
		uint32_t	stat:1;		// static TODO: kind & ATTR_stat
		uint32_t _padd:28;		// declaration level
	};
	};

	symn	defs;		// global variables and functions / while_compiling variables of the block in reverse order
	symn	gdef;		// all static variables and functions
	astn	init;		// VAR init / FUN body, this shuld be null after codegen

	astn	used;		// TEMP: usage references
	char*	pfmt;		// TEMP: print format

	//~ TODO: deprecated members
	symn	arrB;		// array variable base type
	int32_t nest, colp; // declared on column and level
};

/// Abstract syntax tree node
struct astNode {
	ccToken		kind;				// code: TYPE_ref, OPER_???
	ccToken		cst2;				// casts to: (void, bool, int32, int64, float32, float64, reference, value)
	symn		type;				// typeof() return type of operator
	astn		next;				// next statement, next usage, do not use for preorder
	char*		file;				// file name of the token belongs to
	int32_t		line;				// line position of token
	int32_t		colp;				// column position
	union {
		char *cstr;
		int64_t cint;				// constant integer value
		float64_t cflt;				// constant floating point value
		struct {			// STMT_xxx: statement
			astn	stmt;			// statement / then block
			astn	step;			// increment / else block
			astn	test;			// condition: if, for
			astn	init;			// for statement init
		} stmt;
		struct {			// TYPE_ref: identifyer
			char*	name;			// name of identifyer
			unsigned hash;			// hash code for 'name'
			symn	link;			// variable
			astn	used;			// next used
		} ref;
		struct {			// OPER_xxx: operator
			astn	rhso;			// right hand side operand
			astn	lhso;			// left hand side operand
			astn	test;			// ?: operator condition
			size_t	prec;			// precedence
		} op;
		struct {			// STMT_brk, STMT_con
			size_t offs;
			size_t stks;			// stack size
		} go2;
		struct {			// STMT_beg: list
			astn head;
			astn tail;
		} lst;
	};
};

/// Debug info node
struct dbgNode {
	// the statement tree.
	astn stmt;
	// the declared symbol.
	symn decl;

	// position in code
	size_t start, end;

	// position in file.
	char* file;
	int line;

	// break execution?
	int bp;

	// execution information
	int64_t total, self;  // time spent executing function / statement
	int64_t hits, exec;  // hit count and successful executions
};

/// Compiler context
struct ccContextRec {
	rtContext	rt;
	symn	gdef;		// all static variables and functions
	symn	defs;		// all definitions
	libc	libc;		// installed libcalls
	symn	func;		// functions level stack
	astn	root;		// statements

	// lists and tables
	astn	jmps;		// list of break and continue statements to fix
	//~ symn	free;		// variables that needs to be freed
	list	strt[TBLS];		// string hash table
	symn	deft[TBLS];		// symbol hash stack
	//~ astn	scope[TBLS];		// current scope + expression scope, string literal, ...
	int		nest;		// nest level: modified by (enter/leave)

	int		warn;		// warning level
	int		maxlevel;		// max nest level: disables lookup of variables outside the current function.
	int		siff:1;		// inside a static if false
	int		init:1;		// initialize static variables ?
	int		_padd:30;	//

	// Parser
	char*	file;		// current file name
	int		line;		// current line number
	int		lpos;		// current line position
	size_t	fpos;		// current file position
	//~ current column = fpos - lpos

	// Lexer
	struct {
		symn	pfmt;			// Warning set to -1 to record.
		astn	tokp;			// list of reusable tokens
		astn	_tok;			// one token look-ahead
		int		_chr;			// one char look-ahead
		struct {				// Input
			int		nest;		// nesting level on open.
			int		_fin;		// file handle
			size_t	_cnt;		// chars left in buffer
			char*	_ptr;		// pointer parsing trough source
			uint8_t	_buf[1024];	// memory file buffer
		} fin;
	};

	// Type cache
	astn	void_tag;		// no parameter of type void
	astn	emit_tag;		// "emit"

	symn	type_rec;		// typename
	symn	type_vid;		// void
	symn	type_bol;		// boolean
	symn	type_i32;		// 32bit signed integer
	symn	type_u32;		// 32bit unsigned integer
	symn	type_i64;		// 64bit signed integer
	//~ symn	type_u64;		// 64bit unsigned integer
	symn	type_f32;		// 32bit floating point
	symn	type_f64;		// 64bit floating point
	symn	type_ptr;		// pointer
	symn	type_var;		// variant

	symn	type_str;		// TODO: string should be replaced with pointer or char* or cstr

	symn	null_ref;		// variable null
	symn	emit_opc;		// emit intrinsic function, or whatever it is.

	symn	libc_dbg;		// debug function libcall: debug(string message, variant inspect, int level, int maxTrace);
	size_t	libc_dbg_idx;	// debug function index
};

/// Debugger context
struct dbgContextRec {
	rtContext rt;
	userContext extra;		// extra data for debuger
	int (*debug)(dbgContext ctx, vmError, size_t ss, void* sp, void* ip);
	int (*profile)(dbgContext ctx, size_t ss, void* caller, void* callee, time_t now);
	int checked;			// execution is inside an try catch
	struct arrBuffer functions;
	struct arrBuffer statements;
};


//             *** Type related functions
/// Enter a new scope.
void enter(ccContext cc, astn ast);

/// Leave current scope.
symn leave(ccContext cc, symn dcl, int mode);

/**
 * @brief Install a new symbol: alias, type, variable or function.
 * @param name Symbol name.
 * @param kind Kind of sybol: (TYPE_def, TYPE_ref,TYPE_rec, TYPE_arr)
 * @param cast Casts to ...
 * @param size Size of symbol
 * @param type Type of symbol (base type / return type).
 * @param init Initializer, function body, alias link.
 * @return The symbol.
 */
symn install(ccContext, const char* name, ccToken kind, ccToken cast, unsigned size, symn type, astn init);

/**
 * @brief Install a new symbol: alias, type, variable or function.
 * @param kind Kind of sybol: (TYPE_def, TYPE_ref,TYPE_rec, TYPE_arr)
 * @param tag Parsed tree node representing the sybol.
 * @param typ Type of symbol.
 * @return The symbol.
 */
symn declare(ccContext, ccToken kind, astn tag, symn typ);

/**
 * @brief Lookup an identifier.
 * @param sym List of symbols to search in.
 * @param ast Identifier node to search for.
 * @param args List of arguments, use null for variables.
 * @param raise may raise error if more than one result found.
 * @return Symbol found.
 */
symn lookup(ccContext, symn sym, astn ast, astn args, int raise);

/**
 * @brief Type Check an expression.
 * @param loc Override scope.
 * @param ast Tree to check.
 * @return Type of expression.
 */
// TODO: find a better name
symn typeCheck(ccContext, symn loc, astn ast);

/**
 * @brief check if a value can be assigned to a symbol.
 * @param rhs variable to be assigned to.
 * @param val value to be assigned.
 * @param strict Strict mode: casts are not enabled.
 * @return cast of the assignmet if it can be done.
 */
ccToken canAssign(ccContext, symn rhs, astn val, int strict);

/**
 * @brief Add usage of the symbol.
 * @param sym Symbol typename or variable.
 * @param tag The node of the usage.
 */
void addUsage(symn sym, astn tag);

/**
 * @brief Get how many time a symbol was referenced.
 * @param sym Symbol to be checked.
 * @return Usage count including declaration.
 */
int countUsages(symn sym);


//             *** Tree related functions
/// Allocate a symbol node.
symn newdefn(ccContext, ccToken kind);
/// Allocate a tree node.
astn newnode(ccContext, ccToken kind);
/// Consume node, so it may be reused.
void eatnode(ccContext, astn ast);

/// Allocate a constant integer node.
astn intnode(ccContext, int64_t v);

/// Allocate a constant float node.
astn fltnode(ccContext, float64_t v);

/// Allocate a constant string node.
astn strnode(ccContext, char *v);

/// Allocate node whitch is a link to a reference
astn lnknode(ccContext, symn ref);

/// Allocate an operator tree node.
astn opnode(ccContext, ccToken kind, astn lhs, astn rhs);

// return constant values of nodes
int32_t constbol(astn ast);
int64_t constint(astn ast);
float64_t constflt(astn ast);
/**
 * @brief Try to evaluate a constant expression.
 * @param res Place the result here.
 * @param ast Abstract syntax tree to be evaluated.
 * @return Type of result: [TYPE_err, TYPE_bit, TYPE_int, TYPE_flt, TYPE_str]
 */
// TODO: to be deleted; use vm to evaluate constants.
ccToken eval(astn res, astn ast);

/**
 * @brief Get the symbol(variable) linked to expression.
 * @param ast Abstract syntax tree to be checked.
 * @return null if not an identifier.
 * @note if input is a the symbol for variable a is returned.
 * @note if input is a.b the symbol for member b is returned.
 * @note if input is a(1, 2) the symbol for function a is returned.
 */
symn linkOf(astn ast);

/**
 * @brief Check if a symbol is a type.
 * @param sym Symbol to be checked.
 * @return true or false.
 */
int isType(symn sym);

/**
 * @brief Check if an expression is a type.
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isTypeExpr(astn ast);

/**
 * @brief Check if an expression is constant.
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isConstExpr(astn ast);

/**
 * @brief Check if an expression is static.
 * @param Compiler context.
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
// TODO: remove function; lookup should handle static expressions.
int isStaticExpr(ccContext, astn ast);

/** Cast
 * returns one of (TYPE_bit, ref, u32, i32, i64, f32, f64)
**/
// TODO: check / remove function
ccToken castOf(symn typ);

// TODO: check / remove function
ccToken castTo(astn ast, ccToken castTo);

/**
 * @brief Get the size of the symbol.
 * @param sym Symbol to be checked.
 * @param varSize if the type is reference type return the size of a reference.
 * @return Size of type or variable.
 */
// TODO: remove function; use sym->size instead.
size_t sizeOf(symn sym, int varSize);


//             *** Parsing section
/// skip the next token.
astn next(ccContext, ccToken kind);
int skip(ccContext, ccToken kind);
ccToken test(ccContext);
void backTok(ccContext, astn tok);
astn peekTok(ccContext, ccToken kind);
ccToken skiptok(ccContext, ccToken kind, int raise);
int source(ccContext, int isFile, char* src);

//             *** Print and dump
enum Format {
	// fputAst
	nlAstBody = 0x0400,
	nlAstElIf = 0x0800,

	// sym & ast & val
	prType = 0x0010,
	prSymQual = 0x0020,
	prAstCast = 0x0020,
	prSymInit = 0x0040,

	// fputAsm
	prAsmCode = 0x000f, // print code bytes (0-15)
	prAsmAddr = 0x0010, // print global address: (@0x003d8c)
	prAsmName = 0x0040, // use symbol names instead of addresses: <main+80>

	// prArgs = 0x0080,
};

void fputFmt(FILE *out, const char *esc[], const char *fmt, ...);

void fputSym(FILE *out, const char *esc[], symn sym, int mode, int indent);

void fputAst(FILE *out, const char *esc[], astn ast, int mode, int indent);

/**
 * @brief Print an instruction to the output stream.
 * @param fout Output stream.
 * @param ptr Instruction pointer.
 * @param len Length to represent binary the instruction.
 * @param offs Base offset to be used for relative instructions.
 *    negative value forces relative offsets. (ex: jmp +5)
 *    positive or zero value forces absolute offsets. (ex: jmp @0255d0)
 * @param rt Runtime context (optional).
 */
void fputAsm(FILE *out, const char *esc[], rtContext, void *ptr, int mode);

/** TODO: to be renamed and moved.
 * @brief Print the value of a variable at runtime.
 * @param rt Runtime context.
 * @param fout Output stream.
 * @param var Variable to be printed. (May be a typename also)
 * @param ref Base offset of variable.
 * @param level Identation level. (Used for members and arrays)
 */
void fputVal(FILE *out, const char *esc[], rtContext, symn var, stkval *ref, int mode, int indent);

/**
 * @brief Dump all accessible symbols
 * TODO: DocMe
 */
void dumpApi(rtContext rt, userContext ctx, void action(userContext, symn));

/**
 * @brief Iterate over instructions.
 * @param Runtime context.
 * @param offsBegin First instruction offset.
 * @param offsEnd Last instruction offset.
 * @param extra Extra arguments for callback.
 * @param action Callback executed on each instruction.
 */
void dumpAsm(rtContext rt, size_t start, size_t end, userContext extra, void action(userContext, size_t offs, void *ip));

// TODO: remove: print compiler error
void ccError(rtContext rt, int level, const char *file, int line, const char *msg, ...);

//             *** General
unsigned rehash(const char* str, size_t size);
char* mapstr(ccContext cc, const char *str, size_t size/* = -1*/, unsigned hash/* = -1*/);

static inline void* paddptr(void *offs, unsigned align) {
	return (void*)(((ptrdiff_t)offs + (align - 1)) & ~(ptrdiff_t)(align - 1));
}

static inline size_t padded(size_t offs, unsigned align) {
	return (offs + (align - 1)) & ~(align - 1);
}

//             *** Plugins
void closeLibs();
int importLib(rtContext rt, const char* path);

static inline void _abort() {
#ifndef DEBUGGING	// abort on first internal error
	abort();
#endif
}

// TODO: Extract all error and warnings messages.
#define ERR_INTERNAL_ERROR "Internal Error"
#define ERR_MEMORY_OVERRUN "Memory Overrun"

#define ERR_ASSIGN_TO_CONST "assignment of constant variable `%+t`"
#define ERR_SYNTAX_ERR_BEFORE "syntax error before token '%t'"
#define ERR_CONST_INIT "invalid constant initialization `%+t`"

#define WARN_USE_BLOCK_STATEMENT "statement should be a block statement {%+t}."
#define WARN_EMPTY_STATEMENT "empty statement `;`."
#define WARN_TRAILING_COMMA "skipping trailing comma before `%t`"
#define WARN_INVALID_EXPRESSION_STATEMENT "expression statement expected"

#define FATAL_UNIMPLEMENTED_OPERATOR "operator %t (%T, %T): %+t"

//~ disable warning messages
#ifdef _MSC_VER
// C4996: The POSIX ...
#pragma warning(disable: 4996)
#endif

#define prerr(__DBG, __MSG, ...) do { fputfmt(stdout, "%s:%u: "__DBG": %s: "__MSG"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)

#define fatal(msg, ...) do { prerr("internal error", msg, ##__VA_ARGS__); _abort(); } while(0)
#define dieif(__EXP, msg, ...) do {if (__EXP) { prerr("internal error("#__EXP")", msg, ##__VA_ARGS__); _abort(); }} while(0)

// compilation errors
#define error(__ENV, __FILE, __LINE, msg, ...) do { ccError(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__); trace(msg, ##__VA_ARGS__); } while(0)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { ccError(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)
#define info(__ENV, __FILE, __LINE, msg, ...) do { ccError(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)

#ifndef DEBUGGING	// disable compiler debugging
#define logif(__EXP, msg, ...) do {} while(0)
#define debug(msg, ...) do {} while(0)
#define trace(msg, ...) do {} while(0)
#define traceAst(__AST) do {} while(0)
#define traceLoop(msg, ...) do {} while(0)
#else
#define logif(__EXP, msg, ...) do {if (__EXP) prerr("todo", msg, ##__VA_ARGS__);} while(0)
#if DEBUGGING >= 2	// enable debug
#define debug(msg, ...) do { prerr("debug", msg, ##__VA_ARGS__); } while(0)
#else
#define debug(msg, ...) do {} while(0)
#endif
#if DEBUGGING >= 1	// enable trace
#define trace(msg, ...) do { prerr("trace", msg, ##__VA_ARGS__); } while(0)
#else
#define trace(msg, ...) do {} while(0)
#endif
#define traceAst(__AST) do { trace("%+t", __AST); } while(0)
#define traceLoop(msg, ...) //do { trace("trace", msg, ##__VA_ARGS__); } while(0)
#endif

#endif
