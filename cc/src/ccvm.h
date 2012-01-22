/*******************************************************************************
 *   File: ccvm.h
 *   Date: 2007/04/20
 *   Desc: main header
 *******************************************************************************
 *
 *
*******************************************************************************/
#ifndef __CCVM_H
#define __CCVM_H
#include "pvmc.h"
#include <stdlib.h>

// maximum tokens in expressions & nest level
#define TOKS 2048

// symbol & hash table size
#define TBLS 512

// maximum elements to print from an array
#define MAX_ARR_PRINT 20

#define DO_PRAGMA(x) _Pragma(#x)
#define TODO(x) //DO_PRAGMA(message("TODO: " #x))

#ifdef _MSC_VER
#define pdbg(__DBG, __FILE, __LINE, msg, ...) do {fputfmt(stderr, "%s:%d: "__DBG": %s: "msg"\n", __FILE, __LINE, __FUNCTION__, ##__VA_ARGS__); fflush(stdout); fflush(stderr);} while(0)
#else
#define pdbg(__DBG, __FILE, __LINE, msg, ...) do {fputfmt(stderr, "%s:%d: "__DBG": %s: "msg"\n", __FILE, __LINE, __func__, ##__VA_ARGS__); fflush(stdout); fflush(stderr);} while(0)
#endif

#define prerr(msg, ...) do {pdbg("FixMe", __FILE__, __LINE__, msg, ##__VA_ARGS__); } while(0)
#define fatal(msg, ...) do {prerr(msg, ##__VA_ARGS__); abort();} while(0)
#define dieif(__EXP, msg, ...) do {if (__EXP) fatal(msg, ##__VA_ARGS__);} while(0)
#define logif(__EXP, msg, ...) do {if (__EXP) prerr(msg, ##__VA_ARGS__);} while(0)

#ifdef _MSC_VER

#if DEBUGGING > 1
#define debug(msg, ...) pdbg("debug", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define trace(msg, ...) pdbg("trace", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); debug(msg, ##__VA_ARGS__); } while(0)
#else // catch the position error raised
#define debug(msg, ...)
#define trace(msg, ...)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)
#endif
#define error(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)

#elif defined __WATCOMC__

#if DEBUGGING > 1
#define debug(msg, ...) pdbg("debug", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define trace(msg, ...) pdbg("trace", __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); debug(msg, ##__VA_ARGS__); } while(0)
#else // catch the position error raised
#define debug(msg, ...)
#define trace(msg, ...)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); } while(0)
#endif
#define error(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)

#else

#if DEBUGGING > 1
#define debug(msg...) pdbg("debug", __FILE__, __LINE__, msg)
#define trace(msg...) pdbg("trace", __FILE__, __LINE__, msg)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg); debug(msg); } while(0)
#else // catch the position error raised
#define debug(msg...)
#define trace(msg...)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg); debug(msg); } while(0)
#endif
#define error(__ENV, __FILE, __LINE, msg...) PERR(__ENV, -1, __FILE, __LINE, msg)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg...) PERR(__ENV, __LEVEL, __FILE, __LINE, msg)
#define info(__ENV, __FILE, __LINE, msg...) PERR(__ENV, 0, __FILE, __LINE, msg)
#endif

// Symbols - CC(tokens)
typedef enum {
	#define TOKDEF(NAME, TYPE, SIZE, STR) NAME,
	#include "defs.i"
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
typedef struct {
	int const	type;
	int const	argc;
	char const	*name;
} tok_inf;
extern const tok_inf tok_tbl[255];

// Opcodes - VM(opcodes)
typedef enum {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) Name = Code,
	#include "defs.i"
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

	//~ opc_line,		// line info

	b32_bit_and = 0 << 6,
	b32_bit_shl = 1 << 6,
	b32_bit_shr = 2 << 6,
	b32_bit_sar = 3 << 6,

	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,
	max_reg = 255,	// maximum registers for dup, set, pop, ...
	//~ data_size = 4,	// 
} vmOpcode;
typedef struct {
	int const	code;
	int const	size;
	int const	chck;
	int const	diff;
	char const *name;
	//~ char const *help;
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
	struct {void* data; int length;} dA;	// dinamic array
} stkval;

//~ typedef struct symn *symn;		// Symbol Node
typedef struct astn *astn;		// Abstract Syntax Tree Node
typedef struct list *list;
typedef unsigned int uint;

typedef struct libc {
	struct libc		*next;	// next
	//~ unsigned int pos;
	int (*call)(state);
	const char* proto;
	symn sym;
	int8_t chk, pop;
	int16_t pos;//__pad[2];
} *libc;
struct list {				// linked list: stringlist, memgr, ...
	struct list		*next;
	unsigned char	*data;
	unsigned int	size;	// := ((char*)&node) - data;
	unsigned int	_pad;
	//~ unsigned int	offs;	// offset in file ?
};
struct astn {				// tree node (code)
	symn		type;				// typeof() return type of operator ... base type of IDTF
	//~ ccToken		kind;				// code: TYPE_ref, OPER_???
	//~ ccToken		cst2;				// casts to basic type: (i32, f32, i64, f64, ref, bool, void)
	//~ astn		next;				// next statement, do not use for preorder

	uint8_t	kind;		// TYPE_ref / TYPE_def / TYPE_rec / TYPE_arr
	uint8_t	cst2;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	uint16_t	_pad;				// unused

	union {
		union  {					// TYPE_xxx: constant
			int64_t	cint;			// const: integer
			float64_t	cflt;			// const: float
			//~ char*	cstr;		// const: use instead: '.id.name'
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
			//~ uint32_t _pad;
		} op;
		struct {					// TYPE_ref: identifyer
			char*	name;			// name of identifyer
			int32_t hash;			// hash code for 'name'
			symn	link;			// variable
			astn	args;			// next used
		} id;
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

	astn		next;				// next statement, do not use for preorder
	uint32_t	temp;				// token on line
};
struct symn {				// type node (data)
	char*	name;		// symbol name
	char*	file;		// declared in file
	int		line;		// declared on line

	int32_t	size;		// sizeof(TYPE_xxx)

	//~ NOTE: negative offst means global
	int32_t	offs;		// addrof(TYPE_xxx)


	symn	type;		// base type of TYPE_ref/TYPE_arr/function (void, int, float, struct, ...)

	//~ TODO: temporarly array variable base type
	symn	args;		// struct members / function paramseters

	//~ TODO: should be the tail of args
	symn	sdef;		// static members 

	symn	decl;		// declared in namespace/struct/class, function, ...

	symn	next;		// symbols on table / next param / next field / next symbol

	//~ ccToken	kind;		// TYPE_ref / TYPE_def / TYPE_rec / TYPE_arr
	//~ ccToken	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	//~ uint16_t __castkindpadd;

	uint8_t	kind;		// TYPE_ref / TYPE_def / TYPE_rec / TYPE_arr
	uint8_t	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	union {				// Attributes
	struct {
	uint16_t	call:1;		// callable(function/definition) <=> (kind == TYPE_ref && args)
	uint16_t	cnst:1;		// constant
	uint16_t	stat:1;		// static ?
	uint16_t	glob:1;		// global

	//~ uint8_t	load:1;		// indirect reference: cast == TYPE_ref
	//~ uint8_t	priv:1;		// private
	//~ uint8_t	read:1;		// const / no override
	//~ uint8_t	used:1;		// 
	uint16_t _atr:12;		// attributes (const static /+ private, ... +/).
	};
	uint16_t	Attr;
	};

	int		nest;		// declaration level
	//~ int	refc;		// how many times was referenced

	symn	defs;		// global variables and functions / while_compiling variables of the block in reverse order
	symn	gdef;		// static variables and functions / while_compiling ?

	astn	init;		// VAR init / FUN body, this shuld be null after codegen
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
	//~ symn	gdef;		// definitions
	astn	root;		// statements

	// lists
	astn	jmps;		// jumps
	symn	free;		// free these variables

	list	strt[TBLS];		// string table
	symn	deft[TBLS];		// definitions: hashStack;

	//~ int		verb;		// verbosity
	int		warn;		// warning level
	int		maxlevel;		// max nest level: modified by ?
	int		nest;		// nest level: modified by (enter/leave)
	//~ int		funl;		// function nest
	int		siff:1;		// inside a static if false
	int		sini:1;		// initialize static variables ?
	int		_pad:30;	// 

	char*	file;	// current file name
	int		line;	// current line number
	//~ int		_pad;

	// Warning set to -1 to record.
	symn	pfmt;
	//~ char	*doc;

	struct {
		struct {			// Lexer
			// INPUT
			struct {
				int		_fin;		// file handle (-1) for cc_buff()
				int		_cnt;		// chars left in buffer
				char*	_ptr;		// pointer parsing trough source
				uint8_t	_buf[1024];	// cache
			}fin;
			astn	tokp;		// token pool
			astn	_tok;		// next token
			int		_chr;		// next char
			//~ int		_pad;		//
		};
	};

	astn	void_tag;		// no parameter of type void
	astn	emit_tag;		// "emit"


	//~ symn	type_vid;
	//~ symn	type_bol;
	//~ symn	type_u32;
	//~ symn	type_i32;
	//~ symn	type_i64;
	//~ symn	type_f32;
	//~ symn	type_f64;
	//~ symn	type_str;
	//~ symn	type_ptr;

	//~ symn	null_ref;
	//~ symn	emit_opc;
	//~ symn	emit_val;

	symn	libc_mem;		// memory manager libcall

	char	*_beg;
	char	*_end;
	arrBuffer dbg;
};
static inline int kindOf(astn ast) {return ast ? ast->kind : 0;}

TODO("these should go to ccState or runtime state")
extern symn type_vid;
extern symn type_bol;
extern symn type_u32;
extern symn type_i32;
extern symn type_i64;
extern symn type_f32;
extern symn type_f64;

extern symn type_str;
extern symn type_ptr;
extern symn null_ref;

extern symn emit_opc;
extern symn emit_val;

//~ clog
//~ void fputfmt(FILE *fout, const char *msg, ...);

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
void fputopc(FILE *fout, unsigned char* ptr, int len, int offs, state rt);

// program error
void perr(state rt, int level, const char *file, int line, const char *msg, ...);

void dumpsym(FILE *fout, symn sym, int mode);
//~ void dumpast(FILE *fout, astn ast, int mode);
//~ void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

symn newdefn(ccState, int kind);
astn newnode(ccState, int kind);
astn opnode(ccState, int kind, astn lhs, astn rhs);
astn lnknode(ccState, symn ref);
astn newIden(ccState, char* id);

astn intnode(ccState, int64_t v);
astn fltnode(ccState, float64_t v);
astn strnode(ccState, char *v);
//~ astn fh8node(ccState, uint64_t v);

astn cpynode(ccState, astn src);
void eatnode(ccState, astn ast);

symn installex(ccState, const char* name, int kind, unsigned size, symn type, astn init);
//~ symn installex(ccState, const char* name, int kind, symn type, int cast, unsigned size, astn init);
symn install(ccState, const char* name, int kind, int cast, unsigned size);
symn declare(ccState, int kind, astn tag, symn rtyp);
void extend(symn type, symn args);
symn addarg(ccState, symn sym, const char* name, int kind, symn type, astn init);

//~ symn findsym(ccState, symn in, char *name);
//~ int findnzv(ccState, char *name);
//~ int findint(ccState, char *name, int* res);
//~ int findflt(ccState, char *name, double* res);

int canAssign(symn rhs, astn val, int strict);
symn lookup(ccState, symn sym, astn ast/*, int deep*/, astn args, int raise);
// TODO: this is the semantic-analyzer, typecheck should be renamed.
symn typecheck(ccState, symn loc, astn ast);

/* fix structure member offsets / function arguments
 * returns the size of struct / functions param size.
 */
int fixargs(symn sym, int align, int spos);

int castOf(symn typ);
int castTo(astn ast, int tyId);
symn promote(symn lht, symn rht);

int32_t constbol(astn ast);
int64_t constint(astn ast);
float64_t constflt(astn ast);

astn peek(ccState);
astn next(ccState, int kind);
//~ void back(ccState, astn ast);
//~ int  skip(ccState, int kind);
//~ int  test(ccState, int kind);

astn expr(ccState, int mode);		// parse expression	(mode: lookup)
astn decl(ccState, int mode);		// parse declaration	(mode: enable defs(: struct, define, ...))
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
int mkcon(astn ast, symn type);
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
int padded(int offs, int align);

int isType(symn sym);
int istype(astn ast);

symn linkOf(astn ast);
long sizeOf(symn typ);	// should be typ->size

int source(ccState, srcType mode, char* text);		// mode: file/text

unsigned rehash(const char* str, unsigned size);

char *mapstr(ccState s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

void fputasm(FILE *fout, state rt, int beg, int end, int mode);

#endif
