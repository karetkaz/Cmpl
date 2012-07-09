/*******************************************************************************
 *   File: core.h
 *   Date: 2007/04/20
 *   Desc: main header
 *******************************************************************************
 *
 *
*******************************************************************************/
#ifndef __CORE_H
#define __CORE_H
#include "pvmc.h"
#include <stdlib.h>

// debug level
#define DEBUGGING 15

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

//~ #define DO_PRAGMA(x) _Pragma(#x)
//~ #define TODO(x) //DO_PRAGMA(message("TODO: " #x))

#define pdbg(__DBG, __FILE, __LINE, msg, ...) do {fputfmt(stderr, "%s:%d: "__DBG": %s: "msg"\n", __FILE, __LINE, __FUNCTION__, ##__VA_ARGS__); fflush(stdout); fflush(stderr);} while(0)

#if DEBUGGING > 1
#define debug(msg, ...) pdbg("debug", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#if DEBUGGING > 15
#define trace(msg, ...) pdbg("trace", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define trloop(msg, ...) //pdbg("trace", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#else
#define trace(msg, ...)
#define trloop(msg, ...)
#endif

#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); trace(msg, ##__VA_ARGS__); } while(0)

#else // catch the position error raised

#define debug(msg, ...)
#define trace(msg, ...)
#define trloop(msg, ...)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)
#endif

// error
#define error(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)

// internal errors
#define prerr(msg, ...) do {pdbg("FixMe", __FILE__, __LINE__, msg, ##__VA_ARGS__); } while(0)
#define fatal(msg, ...) do {prerr(msg, ##__VA_ARGS__); abort();} while(0)

#define dieif(__EXP, msg, ...) do {if (__EXP) fatal(msg, ##__VA_ARGS__);} while(0)
#define logif(__EXP, msg, ...) do {if (__EXP) prerr(msg, ##__VA_ARGS__);} while(0)

#define offsetof(__TYPE, __FIELD) ((size_t) &((__TYPE)0)->__FIELD)
#define lengthof(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))

// Symbols - CC(tokens)
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

	decl_NoDefs = 0x100,		// disable type defs in decl.
	decl_NoInit = 0x200,		// disable initializer.
	decl_Colon  = 0x400,		// enable ':' after variable name: for(int a : range(0, 12))

	ATTR_const = 0x0100,		// constant
	ATTR_stat  = 0x0200,		// static

	//~ ATTR_byref = 0x00000002,		// indirect
	//~ ATTR_glob  = 0x00000008,		// global
	//~ ATTR_used  = 0x00000080,		// used
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
	opc_cge,		// argument is the type


	opc_ldi,		// argument is the size
	opc_sti,		// argument is the size

	markIP,

	b32_bit_and = 0 << 6,
	b32_bit_shl = 1 << 6,
	b32_bit_shr = 2 << 6,
	b32_bit_sar = 3 << 6,

	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,

	vm_size = sizeof(int),	// size of data on stack
	vm_regs = 255,	// maximum registers for dup, set, pop, ...
} vmOpcode;
typedef struct opc_inf {
	int const code;
	int const size;
	int const chck;
	int const diff;
	char *const name;
	//~ const char *help;
} opc_inf;
extern const opc_inf opc_tbl[255];

typedef union {		// value type
	int8_t		i1;
	uint8_t		u1;
	int16_t		i2;
	uint16_t	u2;
	int32_t		i4;
	uint32_t	u4;
	int64_t		i8;
	//uint64_t	u8;
	float32_t		f4;
	float64_t		f8;
	//~ float32_t		pf[4];
	//~ float32_t		pd[2];
	//~ struct {float32_t x, y, z, w;} pf;
	//~ struct {float64_t x, y;} pd;
	//~ struct {int64_t lo, hi;} x16;
	int32_t		rel:24;
	struct {void* data; long length;} arr;	// slice
} stkval;

//~ typedef struct symn *symn;		// Symbol Node
typedef struct astn *astn;		// Abstract Syntax Tree Node
typedef struct list *list;
//typedef unsigned int uint;

typedef struct libc {
	struct libc *next;	// next
	int (*call)(state, void*);
	void *data;	// user data for this function
	int chk, pop;
	int pos;
	symn sym;
} *libc;
struct list {				// linked list: stringlist, memgr, ...
	struct list		*next;
	unsigned char	*data;
	unsigned int	size;	// := ((char*)&node) - data;
	unsigned int	_pad;
	//~ unsigned int	offs;	// offset in file ?
};
struct astn {				// tree node (code)
	ccToken		kind;				// code: TYPE_ref, OPER_???
	ccToken		cst2;				// casts to basic type: (i32, f32, i64, f64, ref, bool, void)
	symn		type;				// typeof() return type of operator ... base type of IDTF
	astn		next;				// next statement, do not use for preorder
	union {
		union {						// TYPE_xxx: constant
			int64_t	cint;			// const: integer
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
			int32_t hash;			// hash code for 'name'
			symn	link;			// variable
			astn	used;			// next used
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
struct symn {				// type node (data)
	char*	name;		// symbol name
	char*	file;		// declared in file
	int		line;		// declared on line

	int32_t	size;		// sizeof(TYPE_xxx)

	//~ NOTE: negative offset means global
	int32_t	offs;		// addrof(TYPE_xxx)

	symn	type;		// base type of TYPE_ref/TYPE_arr/function (void, int, float, struct, ...)

	//~ TODO: temporarly array variable base type
	symn	args;		// struct members / function paramseters
	symn	sdef;		// static members (is the tail of args) / function return value(out value)

	symn	decl;		// declared in namespace/struct/class, function, ...
	symn	next;		// symbols on table / next param / next field / next symbol

	#ifdef DEBUGGING
	ccToken	kind;		// TYPE_ref / TYPE_def / TYPE_rec / TYPE_arr
	ccToken	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	uint16_t __castkindpadd;
	#else
	uint8_t	kind;		// TYPE_ref / TYPE_def / TYPE_rec / TYPE_arr
	uint8_t	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	#endif

	union {				// Attributes
	struct {
	// TODO: remove call
	uint16_t	call:1;		// callable(function/definition) <=> (kind == TYPE_ref && args)
	uint16_t	cnst:1;		// constant
	uint16_t	priv:1;		// private
	uint16_t	stat:1;		// static ?
	uint16_t	glob:1;		// global

	//~ uint8_t	load:1;		// indirect reference: cast == TYPE_ref
	//~ uint8_t	used:1;		// 
	uint16_t _padd:11;		// attributes (const static /+ private, ... +/).
	};
	uint16_t	Attr;
	};

	int		nest;		// declaration level

	symn	defs;		// global variables and functions / while_compiling variables of the block in reverse order
	symn	gdef;		// static variables and functions / while_compiling ?

	astn	init;		// VAR init / FUN body, this shuld be null after codegen
	astn	used;		// how many times was referenced by lookup
	char*	pfmt;		// TEMP: print format
};

typedef struct arrBuffer {
	char *ptr;
	int esz;		// element size
	int cap;		// capacity
	int cnt;		// length
} arrBuffer;

void* getBuff(struct arrBuffer* buff, int idx);
void* setBuff(struct arrBuffer* buff, int idx, void* data);
void* addBuff(arrBuffer* buff, void* data);
void initBuff(struct arrBuffer* buff, int initsize, int elemsize);
void freeBuff(struct arrBuffer* buff);

struct ccState {
	state	s;
	symn	defs;		// all definitions
	libc	libc;		// installed libcalls
	symn	func;		// functions level stack
	astn	root;		// statements

	// lists
	astn	jmps;		// jumps
	symn	free;		// free these variables

	list	strt[TBLS];		// string table
	symn	deft[TBLS];		// definitions: hashStack;

	int		warn;		// warning level
	int		nest;		// nest level: modified by (enter/leave)
	int		maxlevel;		// max nest level: modified by ?
	int		siff:1;		// inside a static if false
	int		sini:1;		// initialize static variables ?
	int		_pad:30;	// 
	//~ int		verb:1;		// verbosity

	char*	file;	// current file name
	int		line;	// current line number

	struct {			// Lexer
		symn	pfmt;		// Warning set to -1 to record.
		astn	tokp;		// list of reusable tokens
		astn	_tok;		// one token look-ahead
		int		_chr;		// one char look-ahead
		// INPUT
		struct {
			int		_fin;		// file handle (-1) for cc_buff()
			int		_cnt;		// chars left in buffer
			char*	_ptr;		// pointer parsing trough source
			uint8_t	_buf[1024];	// cache
		} fin;
	};

	astn	void_tag;		// no parameter of type void
	astn	emit_tag;		// "emit"

	symn	type_rec;		// typename
	symn	type_vid;
	symn	type_bol;
	symn	type_u32;
	symn	type_i32;
	symn	type_i64;
	symn	type_f32;
	symn	type_f64;
	symn	type_str;
	symn	type_ptr;

	symn	null_ref;
	symn	emit_opc;

	symn	libc_mem;		// memory manager libcall
	symn	libc_dbg;

	arrBuffer dbg;
};
static inline int kindOf(astn ast) {return ast ? ast->kind : 0;}

//~ clog
//~ void fputfmt(FILE *fout, const char *msg, ...);

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
void fputopc(FILE *fout, unsigned char* ptr, int len, int offs, state rt);

// program error
void perr(state rt, int level, const char *file, int line, const char *msg, ...);

//~ void dumpsym(FILE *fout, symn sym, int mode);
//~ void dumpast(FILE *fout, astn ast, int mode);
//~ void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

symn newdefn(ccState, int kind);
astn newnode(ccState, int kind);

astn opnode(ccState, int kind, astn lhs, astn rhs);
astn lnknode(ccState, symn ref);
astn tagnode(ccState s, char *str);

astn intnode(ccState, int64_t v);
astn fltnode(ccState, float64_t v);
astn strnode(ccState, char *v);
//~ astn fh8node(ccState, uint64_t v);

astn cpynode(ccState, astn src);
void eatnode(ccState, astn ast);

symn install(ccState, const char* name, int kind, int cast, unsigned size, symn type, astn init);

symn declare(ccState, int kind, astn tag, symn rtyp);
symn addarg(ccState, symn sym, const char* name, int kind, symn type, astn init);

int canAssign(ccState, symn rhs, astn val, int strict);
symn lookup(ccState, symn sym, astn ast/*, int deep*/, astn args, int raise);
// TODO: this is the semantic-analyzer, typecheck should be renamed.
symn typecheck(ccState, symn loc, astn ast);

/* fix structure member offsets / function arguments
 * returns the size of struct / functions param size.
 */
int fixargs(symn sym, int align, int spos);

int castOf(symn typ);
int castTo(astn ast, int tyId);

int32_t constbol(astn ast);
int64_t constint(astn ast);
float64_t constflt(astn ast);

astn peek(ccState);
astn next(ccState, int kind);
//~ void back(ccState, astn ast);
int  skip(ccState, int kind);
//~ int  test(ccState, int kind);

astn expr(ccState, int mode);		// parse expression	(mode: do typecheck)
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
 * @param ...: argument
 * @return: program counter
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

int isType(symn sym);
int istype(astn ast);

symn linkOf(astn ast);
long sizeOf(symn typ);	// should be typ->size

int source(ccState, int isFile, char* text);

unsigned rehash(const char* str, unsigned size);

char* mapstr(ccState s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

void fputasm(FILE *fout, state rt, int beg, int end, int mode);

/** return the internal offset of a reference
 * aborts if ptr is not null and not inside the context.
 */
int vmOffset(state, void *ptr);
void vm_fputval(state, FILE *fout, symn var, stkval* ref, int flgs);

//~ disable warning messages
#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif
#endif
