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

#include "ccstd.h"
#include <stdlib.h> // abort

/* debug level:
	0: show where errors were raised
	1: trace errors to their root
	2: include debug messages
	3: print generated assembly for statements with errors
*/
//~ #define DEBUGGING 1

// enable paralell execution stuff
//~ #define VM_MAX_PROCS 1

// maximum elements to print (references, stacktrace, arrayEelements)
// undefine to print everithing
#define LOG_MAX_ITEMS 100

// maximum tokens in expressions & nesting level
#define TOKS 1024

// hash table size
#define TBLS 512

#define prerr(__DBG, __MSG, ...) do { fputfmt(stdout, "%s:%u: "__DBG": %s: "__MSG"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); _break(); } while(0)

#ifndef DEBUGGING	// disable compiler debuging

#define debug(msg, ...) do {} while(0)
#define trace(msg, ...) do {} while(0)
#define traceAst(__AST) do {} while(0)
#define traceLoop(msg, ...) do {} while(0)
#define logif(__EXP, msg, ...) do {} while(0)

#else

#if DEBUGGING >= 2	// enable debug
#define debug(msg, ...) do { prerr("debug", msg, ##__VA_ARGS__); } while(0)
#else
#define debug(msg, ...) do {} while(0)
#endif
#if DEBUGGING >= 1	// enable trace
#define trace(msg, ...) do { prerr("trace", msg, ##__VA_ARGS__); _break(); } while(0)
#else
#define trace(msg, ...) do {} while(0)
#endif
#define traceAst(__AST) do { trace("%+t", __AST); _break(); } while(0)
#define traceLoop(msg, ...) //do { trace("trace", msg, ##__VA_ARGS__); } while(0)
#define logif(__EXP, msg, ...) do {if (__EXP) prerr("todo", msg, ##__VA_ARGS__);} while(0)

#endif

// internal errors
#define fatal(msg, ...) do { prerr("internal error", msg, ##__VA_ARGS__); _abort(); } while(0)
#define dieif(__EXP, msg, ...) do {if (__EXP) { prerr("internal error("#__EXP")", msg, ##__VA_ARGS__); _abort(); }} while(0)

// compilation errors
#define error(__ENV, __FILE, __LINE, msg, ...) do { perr(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__); trace(msg, ##__VA_ARGS__); } while(0)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)
#define info(__ENV, __FILE, __LINE, msg, ...) do { warn(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)

// helper macros
#define lengthOf(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))
#define offsetOf(__TYPE, __FIELD) ((size_t) &((__TYPE)0)->__FIELD)

enum Format {
	nlAstBody = 0x0400,
	nlAstElIf = 0x0800,

	// sym & ast & val
	prType = 0x0010,

	prSymQual = 0x0020,
	prAstCast = 0x0020,

	prSymInit = 0x0040,

	prAsmCode = 0x000f, // print code bytes (0-15)
	prAsmAddr = 0x0010, // print global address: (@0x003d8c)
	prAsmName = 0x0040, // use symbol names instead of addresses: <main+80>

//	prArgs = 0x0080,
//	prXMLLine = 0x0080
};

// Tokens - CC
typedef enum {
	#define TOKDEF(NAME, TYPE, SIZE, STR) NAME,
	#include "defs.inl"
	tok_last,

	OPER_beg = OPER_fnc,
	OPER_end = OPER_com,

	TOKN_err = TYPE_any,
	TYPE_int = TYPE_i64,
	TYPE_flt = TYPE_f64,
	TYPE_str = TYPE_ptr,

	KIND_mask   = 0x0ff,		// mask
	ATTR_mask   = 0xf00,		// mask
	ATTR_stat   = 0x400,		// static
	ATTR_const  = 0x800,		// constant
	ATTR_paral  = 0x100,		// parallel

	stmt_NoDefs = 0x100,		// disable typedefs in stmt.
	stmt_NoRefs = 0x200,		// disable variables in stmt.

	decl_NoDefs = 0x100,		// disable typedefs in decl.
	decl_NoInit = 0x200,		// disable initialization.
	decl_ItDecl = 0x400,		// enable ':' after declaration: for(int a : range(0, 12))

	/*? enum Kind {
		CAST_any    = 0x000000;		// invalid, error, ...
		CAST_vid    = 0x000001;		// void;
		CAST_bit    = 0x000002;		// bool;
		CAST_i32    = 0x000003;		// int32, int16, int8

		// ...
		// alias    = 0x000000;		// invalid at runtime.

		KIND_typ
		typename    = 0x000010;		// struct metadata info.

		KIND_fun
		function    = 0x000020;		// function

		KIND_var
		variable    = 0x000030;		// functions and typenames are also variables

		ATTR_const  = 0x000040;
		ATTR_static = 0x000080;
	}*/
} ccToken;
struct tok_inf {
	int const	type;
	int const	argc;
	char *const	name;
};
extern const struct tok_inf tok_tbl[255];

// Opcodes - VM
typedef enum {
	#define OPCDEF(Name, Code, Size, Args, Push, Mnem) Name = Code,
	#include "defs.inl"
	opc_last,

	opc_neg,		// argument is the type
	opc_add,
	opc_sub,
	opc_mul,
	opc_div,
	opc_mod,

	opc_cmt,
	opc_shl,
	opc_shr,
	opc_and,
	opc_ior,
	opc_xor,

	opc_ceq,
	opc_cne,
	opc_clt,
	opc_cle,
	opc_cgt,
	opc_cge,

	opc_ldi,		// argument is the size
	opc_sti,		// argument is the size
	opc_drop,		// convert this to opcode.spc

	markIP,

	b32_bit_and = 0 << 6,
	b32_bit_shl = 1 << 6,
	b32_bit_shr = 2 << 6,
	b32_bit_sar = 3 << 6,

	vm_size = 4,//sizeof(int),	// size of data element on stack
	px_size = 5,// size in bytes of the exit instruction exit(0)
	rt_size = 8,//sizeof(void*),
	vm_regs = 255	// maximum registers for dup, set, pop, ...
} vmOpcode;
struct opc_inf {
	signed int const code;		// opcode value (0..255)
	unsigned int const size;	// length of opcode with args
	signed int const chck;		// minimum elements on stack before execution
	signed int const diff;		// stack size difference after execution
	char *const name;	// mnemonic for the opcode
};
extern const struct opc_inf opc_tbl[255];

typedef union {		// on stack value type
	int8_t		i1;
	int16_t		i2;
	int32_t		i4;
	int64_t		i8;
	uint8_t		u1;
	uint16_t	u2;
	uint32_t	u4;
	//uint64_t	u8;
	float32_t	f4;
	float64_t	f8;
	int32_t		rel:24;
	struct {int32_t data; int32_t length;} arr;	// slice
	struct {int32_t value; int32_t type;} var;	// variant
	struct {int32_t func; int32_t vars;} del;	// delegate
} stkval;

typedef struct list {	// linked list
	struct list*    next;
	unsigned char*  data;
} *list;
typedef struct libc {	// library call
	struct libc *next;	// next
	int (*call)(libcContext);
	void *data;	// user data for this function
	symn sym;
	size_t pos, chk;
	int pop;
} *libc;

/// Abstract Syntax Tree Node
typedef struct astNode *astn;

// TODO: split into 2 structs one for runtime and another for compiler.
/// Symbol node types and variables
struct symNode {
	char*	name;		// symbol name
	char*	file;		// declared in file
	int32_t	line;		// declared on line
	int32_t nest;		// declaration level
	int32_t	colp;		// declared on column
	size_t	size;		// variable or function size.
	size_t	offs;		// address of variable or function.

	symn	type;		// base type of TYPE_ref/TYPE_arr/function (void, int, float, struct, ...)

	symn	flds;		// all fields: static + nonstatic fields / function return value + parameters
	//~ TODO: temporarly array variable base type
	symn	prms;		// tail of flds: struct nonstatic fields / function paramseters

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
		uint32_t	cnst:1;		// constant
		uint32_t	stat:1;		// static
		uint32_t _padd:28;		// declaration level
	};
	};

	symn	defs;		// global variables and functions / while_compiling variables of the block in reverse order
	symn	gdef;		// all static variables and functions
	astn	init;		// VAR init / FUN body, this shuld be null after codegen

	astn	used;		// TEMP: usage references
	char*	pfmt;		// TEMP: print format
};

/// Abstract Syntax Tree Node
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

struct arrBuffer {
	char *ptr;		// data
	int esz;		// element size
	int cap;		// capacity
	int cnt;		// length
};
int initBuff(struct arrBuffer* buff, int initsize, int elemsize);
void* setBuff(struct arrBuffer* buff, int idx, void* data);
void* insBuff(struct arrBuffer* buff, int idx, void* data);
void* getBuff(struct arrBuffer* buff, int idx);
void freeBuff(struct arrBuffer* buff);

typedef struct dbgInfo {
	// the statement tree.
	astn stmt;
	// the declared symbol.
	symn decl;

	// position in file.
	char* file;
	int line;

	// break execution?
	int bp;

	// trace information
	int64_t total, self;  // time spent executing functions
	int64_t hits, exec;  // count function executions

	// position in code
	size_t start, end;
} *dbgInfo;

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

	//~ symn	libc_mem;		// memory manager libcall: memmgr(pointer oldOffset, int newSize);
	symn	libc_dbg;		// debug function libcall: debug(string message, variant inspect, int level, int maxTrace);
	size_t	libc_dbg_idx;	// debug function index
};

/// Debuger context
struct dbgContextRec {
	rtContext rt;
	userContext extra;		// extra data for debuger
	int checked;			// execution is inside an try catch
	struct arrBuffer functions;
	struct arrBuffer statements;
	int (*function)(dbgContext ctx, vmError, size_t ss, void* sp, void* caller, void* callee);
};

/**
 * @brief Print formatted text to the output stream.
 * @param fout Output stream.
 * @param msg Format text.
 * @param ... Format variables.
 * @note %(\?)?[+-]?[0 ]?([1-9][0-9]*)?(.(\*)|([1-9][0-9]*))?[tTkKAIbBoOxXuUdDfFeEsScC]
 *    skip: (\?)? skip printing if variable is null or zero (prints pad character if specified.)
 *    sign: [+-]? sign flag / alignment.
 *    padd: [0 ]? padding character.
 *    len:  ([1-9][0-9]*)? length
 *    offs: (.(*)|([1-9][0-9]*))? percent or offset.
 *
 *    T: symbol
 *      +: expand function
 *      -: qualified name only
 *    k: ast
 *      +: expand statements
 *      -: ?
 *    K: kind
 *    A: instruction (asm)
 *    I: indent
 *    b: 32 bit bin value
 *    B: 64 bit bin value
 *    o: 32 bit oct value
 *    O: 64 bit oct value
 *    x: 32 bit hex value
 *    X: 64 bit hex value
 *    u: 32 bit unsigned value
 *    U: 64 bit unsigned value
 *    d: 32 bit signed value (decimal)
 *    D: 64 bit signed value (decimal)
 *    f: 32 bit floating point value
 *    F: 64 bit floating point value
 *    e: 32 bit floating point value (Scientific notation (mantissa/exponent))
 *    E: 64 bit floating point value (Scientific notation (mantissa/exponent))
 *    s: ansi string
 *    c: ansi character
 *    S: ?wide string
 *    C: ?wide character
 * flags
 *    +-
 *        oOuU: ignored
 *        bBxX: lowerCase / upperCase
 *        dDfFeE: forceSign /
 *
 */
void fputfmt(FILE *fout, const char *msg, ...);
void fputesc(FILE* fout, const char *esc[], const char* msg, ...);

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
void fputasm(FILE* fout, void *ptr, int mode, rtContext rt);

/**
 * @brief Dump all accessible symbols
 * TODO: DocMe
 */
void iterateApi(rtContext rt, userContext ctx, void action(userContext, symn));

/**
 * @brief Iterate over instructions.
 * @param Runtime context.
 * @param offsBegin First instruction offset.
 * @param offsEnd Last instruction offset.
 * @param extra Extra arguments for callback.
 * @param action Callback executed on each instruction.
 */
void iterateAsm(rtContext rt, size_t offsBegin, size_t offsEnd, userContext extra, void action(userContext, size_t offs, void* ip));

/** TODO: to be renamed and moved.
 * @brief Print the value of a variable at runtime.
 * @param rt Runtime context.
 * @param fout Output stream.
 * @param var Variable to be printed. (May be a typename also)
 * @param ref Base offset of variable.
 * @param level Identation level. (Used for members and arrays)
 */
void fputval(rtContext, FILE *fout, symn var, stkval* ref, int level, int mode);

// program error
void perr(rtContext rt, int level, const char *file, int line, const char *msg, ...);

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
 * @brief check if a value can be assigned to a symbol.
 * @param rhs variable to be assigned to.
 * @param val value to be assigned.
 * @param strict Strict mode: casts are not enabled.
 * @return cast of the assignmet if it can be done.
 */
ccToken canAssign(ccContext, symn rhs, astn val, int strict);

/**
 * @brief Lookup an identifier.
 * @param sym List of symbols to search in.
 * @param ast Identifier node to search for.
 * @param args List of arguments, use null for variables.
 * @param raise may raise error if more than one result found.
 * @return Symbol found.
 */
symn lookup(ccContext, symn sym, astn ast, astn args, int raise);

/** TODO: to be renamed to something like `fixtype`.
 * @brief Type Check an expression.
 * @param loc Override scope.
 * @param ast Tree to check.
 * @return Type of expression.
 */
symn typecheck(ccContext, symn loc, astn ast);

/**
 * @brief Fix structure member offsets / function arguments.
 * @param sym Symbol of struct or function.
 * @param align Alignment of members.
 * @param base Size of base / return.
 * @return Size of struct / functions param size.
 */
size_t fixargs(symn sym, unsigned int align, size_t base);

/**
 * @brief Add usage of the symbol.
 * @param sym Symbol typename or variable.
 * @param tag The node of the usage.
 */
void addUsage(symn sym, astn tag);

ccToken castOf(symn typ);
ccToken castTo(astn ast, ccToken castTo);

/// skip the next token.
astn next(ccContext, ccToken kind);
int skip(ccContext, ccToken kind);
ccToken test(ccContext);
void backTok(ccContext, astn tok);
astn peekTok(ccContext, ccToken kind);
ccToken skiptok(ccContext, ccToken kind, int raise);
int source(ccContext, int isFile, char* src);

//~ astn expr(ccContext, int mode);		// parse expression	(mode: typecheck)
//~ astn decl(ccContext, int mode);		// parse declaration	(mode: enable defs(: struct, define, ...))
//~ astn stmt(ccContext, int mode);		// parse statement	(mode: enable decl/enter new scope)
astn decl_var(ccContext, astn *args, int mode);

/// Enter a new scope.
void enter(ccContext, astn ast);
/// Leave current scope.
symn leave(ccContext, symn def, int mkstatic);

/**
 * @brief Open a file or text for compilation.
 * @param rt runtime context.
 * @param file file name of input.
 * @param line first line of input.
 * @param text if not null, this will be compiled instead of the file.
 * @return compiler context or null on error.
 * @note invokes ccInit if not initialized.
 */
ccContext ccOpen(rtContext rt, char* file, int line, char* source);

/**
 * @brief Close input file, ensuring it ends correctly.
 * @param cc compiler context.
 * @return number of errors.
 */
int ccDone(ccContext cc);


/** TODO: to be deleted; lookup should handle static expressions.
 * @brief Check if an expression is static.
 * @param Compiler context.
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isStatic(ccContext, astn ast);

/**
 * @brief Check if an expression is constant.
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isConst(astn ast);

/**
 * @brief Check if an expression is a type.
 * @param ast Abstract syntax tree to be checked.
 * @return true or false.
 */
int isType(astn ast);

/**
 * @brief Check if a symbol is a type.
 * @param sym Symbol to be checked.
 * @return true or false.
 */
int istype(const symn sym);

/**
 * @brief Get how many time a symbol was referenced.
 * @param sym Symbol to be checked.
 * @return Usage count including declaration.
 */
int usages(symn sym);

/** TODO: to be deleted; sym->size should be used instead.
 * @brief Get the size of the symbol.
 * @param sym Symbol to be checked.
 * @param varSize if the type is reference type return the size of a reference.
 * @return Size of type or variable.
 */
size_t sizeOf(symn sym, int varSize);

/**
 * @brief Get the symbol(variable) linked to expression.
 * @param ast Abstract syntax tree to be checked.
 * @return null if not an identifier.
 * @note if input is a the symbol for variable a is returned.
 * @note if input is a.b the symbol for member b is returned.
 * @note if input is a(1, 2) the symbol for function a is returned.
 */
symn linkOf(astn ast);

/** TODO: to be deleted; use vm to evaluate constants.
 * @brief Try to evaluate a constant expression.
 * @param res Place the result here.
 * @param ast Abstract syntax tree to be evaluated.
 * @return Type of result: [TYPE_err, TYPE_bit, TYPE_int, TYPE_flt, TYPE_str]
 */
int eval(astn res, astn ast);

/// Allocate a symbol node.
symn newdefn(ccContext, ccToken kind);
/// Allocate a tree node.
astn newnode(ccContext, ccToken kind);
/// Consume node, so it may be reused.
void eatnode(ccContext, astn ast);

/// Allocate an operator tree node.
astn opnode(ccContext, ccToken kind, astn lhs, astn rhs);
/// Allocate node whitch is a link to a reference
astn lnknode(ccContext, symn ref);

/// Allocate a constant integer node.
astn intnode(ccContext, int64_t v);
/// Allocate a constant float node.
astn fltnode(ccContext, float64_t v);
/// Allocate a constant string node.
astn strnode(ccContext, char *v);


int32_t constbol(astn ast);
int64_t constint(astn ast);
float64_t constflt(astn ast);


unsigned rehash(const char* str, size_t size);
char* mapstr(ccContext cc, char *str, size_t size/* = -1*/, unsigned hash/* = -1*/);

// TODO: to be removed.
/// returns a pointer to an offset inside the vm.
static inline void* getip(rtContext rt, size_t offset) {
	if (offset == 0) {
		return NULL;
	}
	return (void*)(rt->_mem + offset);
}

//  VM: execution + debuging.

/**
 * @brief Get the internal offset of a reference.
 * @param ptr Memory location.
 * @return The internal offset.
 * @note Aborts if ptr is not null and not inside the context.
 */
size_t vmOffset(rtContext, void *ptr);

/**
 * @brief Optimize an assigment by removing extra copy of the value if it is on the top of the stack.
 * @param Runtime context.
 * @param offsBegin Begin of the byte code.
 * @param offsEnd End of the byte code.
 * @return non zero if the code was optimized.
 */
int optimizeAssign(rtContext, size_t offsBegin, size_t offsEnd);

/**
 * @brief Emit an instruction.
 * @param Runtime context.
 * @param opc Opcode.
 * @param arg Argument.
 * @return Program counter.
 */
size_t emitarg(rtContext, vmOpcode opc, stkval arg);

/**
 * @brief Emit an instruction with int argument.
 * @param Runtime context.
 * @param opc Opcode.
 * @param arg Integer argument.
 * @return Program counter.
 */
size_t emitint(rtContext, vmOpcode opc, int64_t arg);

/**
 * @brief Fix a jump instruction.
 * @param Runtime context.
 * @param src Location of the instruction.
 * @param dst Absolute position to jump.
 * @param stc Fix also stack size.
 * @return
 */
int fixjump(rtContext, int src, int dst, int stc);


// Debuging.

dbgInfo getDbgStatement(rtContext rt, char* file, int line);
dbgInfo mapDbgStatement(rtContext rt, size_t position);
dbgInfo addDbgStatement(rtContext rt, size_t start, size_t end, astn tag);
dbgInfo mapDbgFunction(rtContext rt, size_t position);
dbgInfo addDbgFunction(rtContext rt, symn fun);

void logTrace(rtContext rt, FILE *out, int ident, int startlevel, int tracelevel);
//~ disable warning messages
#ifdef _MSC_VER
// C4996: The POSIX ...
#pragma warning(disable: 4996)
#endif

static inline void _abort() {
#ifndef DEBUGGING	// abort on first internal error
	abort();
#endif
}

static inline void _break() {}

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

#endif
