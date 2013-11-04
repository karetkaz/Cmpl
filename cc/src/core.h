/*******************************************************************************
 *   File: core.h
 *   Date: 2007/04/20
 *   Desc: internal header
 *******************************************************************************
 *
 *
*******************************************************************************/
#ifndef CC_CORE_H
#define CC_CORE_H 2
#include "pvmc.h"
#include <stdlib.h>

/* debug level:
	0: no debug info
	1: print debug messages
	2: print trace messages
	3: print generated assembly for statements with errors;
	4: print generated assembly
	5: print non pre-mapped strings, non static types
	6: print static casts generated with emit
*/
//~ #define DEBUGGING 1

// enable dynamic dll/so lib loading
#define USEPLUGINS

// enable paralell stuff
//~ #define MAXPROCSEXEC 1

// maximum elements to print from an array
#define MAX_ARR_PRINT 100

// maximum tokens in expressions & nest level
#define TOKS 2048

// symbol & hash table size
#define TBLS 512


#define prerr(__DBG, __MSG, ...) do { fputfmt(stdout, "%s:%d: "__DBG": %s: "__MSG"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)

#ifdef DEBUGGING
#define logif(__EXP, msg, ...) do {if (__EXP) prerr("info", msg, ##__VA_ARGS__);} while(0)
#if DEBUGGING > 0	// enable debug
#define debug(msg, ...) do { prerr("debug", msg, ##__VA_ARGS__); } while(0)
#else
#define debug(msg, ...) do {} while(0)
#endif
#if DEBUGGING > 1	// enable trace
#define trace(msg, ...) do { prerr("trace", msg, ##__VA_ARGS__); } while(0)
#define trloop(msg, ...) //do { prerr("trace", msg, ##__VA_ARGS__); } while(0)
#else
#define trace(msg, ...) do {} while(0)
#define trloop(msg, ...) do {} while(0)
#endif

#else
#define logif(__EXP, msg, ...) do {} while(0)
#define debug(msg, ...) do {} while(0)
#define trace(msg, ...) do {} while(0)
#define trloop(msg, ...) do {} while(0)

#endif

// internal errors
#define fatal(msg, ...) do { prerr("fatal", msg, ##__VA_ARGS__); abort(); } while(0)
#define dieif(__EXP, msg, ...) do { if (__EXP) { fatal(msg, ##__VA_ARGS__); } } while(0)

// compilation errors
#define error(__ENV, __FILE, __LINE, msg, ...) do { perr(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__); debug(msg, ##__VA_ARGS__); } while(0)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)
#define info(__ENV, __FILE, __LINE, msg, ...) do { warn(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)

#define lengthOf(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))
#define offsetOf(__TYPE, __FIELD) ((size_t) &((__TYPE)0)->__FIELD)


// Tokens - CC(tokens)
typedef enum {
	#define TOKDEF(NAME, TYPE, SIZE, STR) NAME,
	#include "defs.inl"
	tok_last,

	TOKN_err = TYPE_any,
	TYPE_int = TYPE_i64,
	TYPE_flt = TYPE_f64,
	TYPE_str = TYPE_ptr,

	stmt_NoDefs = 0x100,		// disable typedefs in stmt.
	stmt_NoRefs = 0x200,		// disable variables in stmt.

	decl_NoDefs = 0x100,		// disable typedefs in decl.
	decl_NoInit = 0x200,		// disable initializer.
	decl_ItDecl = 0x400,		// enable ':' after declaration: for(int a : range(0, 12))

	ATTR_const  = 0x100,		// constant
	ATTR_stat   = 0x200			// static
	/*? enum Kind {
		//inline     = 0x000000;	// these are not available at runtime.
		typename     = 0x000001;	//
		function     = 0x000002;	//
		variable     = 0x000003;	// functions and typenames are also variables

		attr_const   = 0x000004;
		attr_static  = 0x000008;
	}*/
} ccToken;
typedef struct tok_inf {
	int const	type;
	int const	argc;
	char *const	name;
} tok_inf;
extern const tok_inf tok_tbl[255];

// Opcodes - VM(opcodes)
typedef enum {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) Name = Code,
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

	vm_size = sizeof(int),	// size of data element on stack
	vm_regs = 255	// maximum registers for dup, set, pop, ...
} vmOpcode;
typedef struct opc_inf {
	int const code;		// opcode value (0..255)
	int const size;		// length of opcode with args
	int const chck;		// minimum elements on stack before execution
	int const diff;		// stack size difference after execution
	char *const name;	// mnemonic for the opcode
} opc_inf;
extern const opc_inf opc_tbl[255];

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
	struct {void* data; long length;} arr;	// slice
} stkval;

typedef struct list {				// linked list: stringlist, ...
	struct list*	next;
	unsigned char*	data;
	unsigned int	size;
	unsigned int	_pad;
} *list;
typedef struct libc {
	struct libc *next;	// next
	int (*call)(libcArgs);
	void *data;	// user data for this function
	symn sym;
	int chk, pop;
	int pos;
} *libc;

typedef struct astNode *astn;		// Abstract Syntax Tree Node

struct symNode {					// type node (meta)
	char*	name;		// symbol name
	char*	file;		// declared in file
	int32_t	line;		// declared on line
	int32_t nest;		// declaration level

	int32_t	size;		// sizeof(TYPE_xxx)
	int32_t	offs;		// addrof(TYPE_xxx)

	symn	type;		// base type of TYPE_ref/TYPE_arr/function (void, int, float, struct, ...)

	symn	flds;		// all fields: static + nonstatic fields / function return value + parameters
	//~ TODO: temporarly array variable base type
	symn	prms;		// tail of flds: struct nonstatic fields / function paramseters

	symn	decl;		// declaring symbol: struct, function, ...
	symn	next;		// next symbol: field / param / ... /(in scope table)

	ccToken	kind;		// TYPE_def / TYPE_rec / TYPE_ref / TYPE_arr
	ccToken	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).

	union {				// Attributes
		uint64_t	pad;
		uint32_t	Attr;
	struct {
		uint32_t	memb:1;		// member operator (push the object by ref first)
		uint32_t	call:1;		// callable(function/definition) <=> (kind == TYPE_ref && args)
		uint32_t	cnst:1;		// constant
		uint32_t	stat:1;		// static ?
		uint32_t	glob:1;		// global
		uint32_t _padd:27;		// declaration level
	};
	};

	symn	defs;		// global variables and functions / while_compiling variables of the block in reverse order
	symn	gdef;		// all static variables and functions

	astn	used;		// usage references
	astn	init;		// VAR init / FUN body, this shuld be null after codegen
	char*	pfmt;		// TEMP: print format
};

struct astNode {					// tree node (code)
	ccToken		kind;				// code: TYPE_ref, OPER_???
	ccToken		cst2;				// casts to: (void, bool, int32, int64, float32, float64, reference, value)
	symn		type;				// typeof() return type of operator
	astn		next;				// next statement, do not use for preorder
	union {
		union {						// TYPE_xxx: constant
			int64_t     cint;		// const: integer
			float64_t	cflt;		// const: float
			//~ char*	cstr;		// const: use instead: '.ref.name'
		} con;
		struct {					// STMT_xxx: statement
			astn	stmt;			// statement / then block
			astn	step;			// increment / else block
			astn	test;			// condition: if, for
			astn	init;			// for statement init
		} stmt;
		struct {					// OPER_xxx: operator
			astn	rhso;			// right hand side operand
			astn	lhso;			// left hand side operand
			astn	test;			// ?: operator condition
			uint32_t prec;			// precedence
		} op;
		struct {					// TYPE_ref: identifyer
			char*	name;			// name of identifyer
			symn	link;			// variable
			astn	used;			// next used
			int32_t hash;			// hash code for 'name'
		} ref;
		struct {					// STMT_brk, STMT_con
			long offs;
			long stks;				// stack size
		} go2;
		struct {					// STMT_beg: list
			astn head;
			astn tail;
		} list;
	};
	char*		file;				// token in file
	uint32_t	line;				// token on line
};

typedef struct arrBuffer {
	char *ptr;		// data
	int esz;		// element size
	int cap;		// capacity
	int cnt;		// length
} arrBuffer;

int initBuff(struct arrBuffer* buff, int initsize, int elemsize);
void* setBuff(struct arrBuffer* buff, int idx, void* data);
void* insBuff(struct arrBuffer* buff, int idx, void* data);
void* getBuff(struct arrBuffer* buff, int idx);
void freeBuff(struct arrBuffer* buff);

typedef struct dbgInfo {
	// the statement tree
	astn stmt;

	// position in file
	char* file;
	int line;

	// position in code
	int start;
	int end;

} *dbgInfo;

struct dbgStateRec {
	int (*dbug)(state, int pu, void* ip, long* sptr, int scnt);

	struct {
		void* cf;
		void* ip;
		void* sp;
		symn sym;
		int pos;
	} trace[512];

	int tracePos;

	struct arrBuffer codeMap;
};

struct ccStateRec {
	state	s;
	symn	defs;		// all definitions
	libc	libc;		// installed libcalls
	symn	func;		// functions level stack
	astn	root;		// statements

	// lists
	astn	jmps;		// jumps
	//~ symn	free;		// free these variables

	list	strt[TBLS];		// string table
	symn	deft[TBLS];		// definitions: hashStack;

	int		warn;		// warning level
	int		nest;		// nest level: modified by (enter/leave)
	int		maxlevel;		// max nest level: modified by ?
	int		siff:1;		// inside a static if false
	int		sini:1;		// initialize static variables ?
	int		_pad:30;	//

	char*	file;	// current file name
	int		line;	// current line number

	struct {			// Lexer
		symn	pfmt;		// Warning set to -1 to record.
		astn	tokp;		// list of reusable tokens
		astn	_tok;		// one token look-ahead
		int		_chr;		// one char look-ahead
		struct {		// INPUT
			int		_fin;		// file handle (-1) for cc_buff()
			int		_cnt;		// chars left in buffer
			char*	_ptr;		// pointer parsing trough source
			uint8_t	_buf[1024];	// cache
		} fin;
	};

	astn	void_tag;		// no parameter of type void
	astn	emit_tag;		// "emit"

	symn	type_rec;		// typename
	symn	type_vid;		// void
	symn	type_bol;		// boolean
	symn	type_u32;		// 32bit unsigned integer
	symn	type_i32;		// 32bit integer
	symn	type_i64;		// 64bit integer
	symn	type_f32;		// 32bit floating point
	symn	type_f64;		// 64bit floating point
	symn	type_ptr;		// pointer
	symn	type_var;		// variant

	symn	type_str;		// TODO: string should be replaced with pointer or char* or cstr

	symn	null_ref;
	symn	emit_opc;

	symn	libc_mem;		// memory manager libcall
	symn	libc_dbg;
};

dbgInfo getCodeMapping(state rt, int position);
dbgInfo addCodeMapping(state rt, astn ast, int start, int end);

//~ clog
void fputopc(FILE *fout, unsigned char* ptr, int len, int offs, state rt);
void fputval(state, FILE *fout, symn var, stkval* ref, int flgs);
void fputasm(state, FILE *fout, int beg, int end, int mode);
void fputfmt(FILE *fout, const char *msg, ...);

// program error
void perr(state rt, int level, const char *file, int line, const char *msg, ...);

// creating syms and nodes
symn newdefn(ccState, int kind);
astn newnode(ccState, int kind);

astn opnode(ccState, int kind, astn lhs, astn rhs);
astn lnknode(ccState, symn ref);
astn tagnode(ccState s, char *str);

astn intnode(ccState, int64_t v);
astn fltnode(ccState, float64_t v);
astn strnode(ccState, char *v);

//~ astn cpynode(ccState, astn src);
void eatnode(ccState, astn ast);

/** install a new symbol: typename, function, variable or alias.
 * @param ccState compiler context
 * @param name symbol name
 * @param kind: type of sybol: (TYPE_ref, TYPE_def, TYPE_rec, TYPE_arr)
 * @param cast: casts to ...
 * @param size: size of symbol
 * @param type: type, base type, return type, of symbol.
 * @param init: initializer, function body, alias link.
 * 
 */
symn install(ccState, const char* name, ccToken kind, ccToken cast, unsigned size, symn type, astn init);

/** install a new symbol: typename, function, variable or alias.
 * like install, using tag node as refernce.
 */
symn declare(ccState, ccToken kind, astn tag, symn rtyp);

int canAssign(ccState, symn rhs, astn val, int strict);
symn lookup(ccState, symn sym, astn ast, astn args, int raise);

// TODO: this is the semantic-analyzer, typecheck should be renamed to something like `fixtype`.
symn typecheck(ccState, symn loc, astn ast);

/* fix structure member offsets / function arguments
 * returns the size of struct / functions param size.
 */
int fixargs(symn sym, int align, int spos);

ccToken castOf(symn typ);
int castTo(astn ast, ccToken tyId);

int32_t constbol(astn ast);
int64_t constint(astn ast);
float64_t constflt(astn ast);

astn peek(ccState);
astn next(ccState, ccToken kind);
int  skip(ccState, ccToken kind);

astn expr(ccState, int mode);		// parse expression	(mode: typecheck)
astn decl(ccState, int mode);		// parse declaration	(mode: enable defs(: struct, define, ...))
astn decl_var(ccState, astn *args, int mode);
//~ astn stmt(ccState, int mode);		// parse statement	(mode: enable decl/enter new scope)
//~ astn unit(ccState, int mode);		// parse program	(mode: script mode)

// scoping ...
void enter(ccState s, astn ast);
symn leave(ccState s, symn def, int mkstatic);

/** try to evaluate an expression
 * @param res: where to put the result
 * @param ast: tree to be evaluated
 * @return one of [TYPE_err, TYPE_bit, TYPE_int, TYPE_flt, ?TYPE_str]
 * @todo should use the vmExec funtion, for the generated code.
 */
int eval(astn res, astn ast);

int isStatic(ccState,astn ast);
int isConst(astn ast);

/** emit an opcode with args
 * @param opc: opcode
 * @param arg: argument
 * @return program counter
 */
int emit(state, vmOpcode opc, stkval arg);
int emitopc(state, vmOpcode opc);
int emiti32(state, int32_t arg);
int emiti64(state, int64_t arg);
int emitf32(state, float32_t arg);
int emitf64(state, float64_t arg);
int emitref(state, void* arg);		// arg should be a pointer inside the vm

int emitinc(state, int val);		// increment the top of stack by ...
int emitidx(state, vmOpcode opc, int pos);
int emitint(state, vmOpcode opc, int64_t arg);
int fixjump(state, int src, int dst, int stc);

// returns the stack size
int stkoffs(state rt, int size);

int istype(symn sym);
int isType(astn ast);
int usedCnt(symn sym);

symn linkOf(astn ast);
long sizeOf(symn typ);

ccState ccOpen(state rt, char* file, int line, char* source);

unsigned rehash(const char* str, unsigned size);
char* mapstr(ccState s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

/** return the internal offset of a reference
 * aborts if ptr is not null and not inside the context.
 */
int vmOffset(state, void *ptr);

//~ disable warning messages
#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#endif
