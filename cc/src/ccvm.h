/*******************************************************************************
 *   File: main.c
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

#define DEBUGGING 15

// maximum tokens in expressions & nest level
#define TOKS 2048

// symbol & hash table size
#define TBLS 512

#define pdbg(__DBG, __FILE, __LINE, msg, ...) do {fputfmt(stderr, "%s:%d: "__DBG": %s: "msg"\n", __FILE, __LINE, __func__, ##__VA_ARGS__); fflush(stdout); fflush(stderr);} while(0)
#define fatal(msg...) do {pdbg("fatal", __FILE__, __LINE__, msg); abort();} while(0)
#define dieif(__EXP, msg...) do {if (__EXP) fatal(msg);} while(0)

#if DEBUGGING > 1
#define debug(msg...) pdbg("debug", __FILE__, __LINE__, msg)
#define trace(msg...) pdbg("trace", __FILE__, __LINE__, msg)
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg); debug(msg); } while(0)
#define error(__ENV, __LINE, msg...) PERR(__ENV, -1, NULL, __LINE, msg)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg...) PERR(__ENV, __LEVEL, __FILE, __LINE, msg)
#define info(__ENV, __FILE, __LINE, msg...) PERR(__ENV, 0, __FILE, __LINE, msg)
#else // catch the position error raised
#define debug(msg...)
#define trace(msg...)
#define error(__ENV, __LINE, msg...) perr(__ENV, -1, NULL, __LINE, msg)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg...) perr(__ENV, __LEVEL, __FILE, __LINE, msg)
#define info(__ENV, __FILE, __LINE, msg...) perr(__ENV, 0, __FILE, __LINE, msg)
#endif

// Symbols - CC
enum {
	#define TOKDEF(NAME, TYPE, SIZE, STR) NAME,
	#include "incl/defs.h"
	tok_last,
	// xml dump cast needs STR

	TOKN_err = TYPE_any,
	//~ ATTR_fun = 0x0100,		// callable
	//~ ATTR_ind = 0x0200,		// indirect

	decl_NoDefs = 0x100,		// disable type defs in decl.
	decl_NoInit = 0x200,		// disable initialization. (disable)
	decl_Ref = decl_NoDefs|decl_NoInit,

	symn_call = 0x100,
	symn_read = 0x200,
};
typedef struct {
	int const	type;
	int const	argc;
	char const	*name;
} tok_inf;
extern const tok_inf tok_tbl[255];

// Opcodes - VM
enum {
	#define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) Name = Code,
	#include "incl/defs.h"
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

	// TODO: remove
	opc_inc,		// increment
	opc_line,		// line info

	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,
	max_reg = 255,	// maximum dup, set, pop, ...
};
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
	struct {int64_t lo, hi;} x16;
} stkval;

//~ typedef struct symn *symn;		// Symbol Node
//~ typedef struct astn *astn;		// Abstract Syntax Tree Node
typedef struct list *list;

struct list {				// linked list
	struct list		*next;
	unsigned char	*data;
	//~ long	offs;
	//~ void	*data;
	//~ unsigned int	size;	// := ((char*)&node) - data;
	//~ unsigned int	offs;	// offset in file ?
};
struct astn {				// tree node
	uint32_t	line;				// token on line (* file offset or what *)
	uint8_t		kind;				// code: TYPE_ref, OPER_???
	uint8_t		cst2;				// casts to basic type: (i32, f32, i64, f64)
	uint16_t	_XXX;				// unused
	union {
		union  {					// TYPE_xxx: constant
			int64_t	cint;			// const: integer
			float64_t	cflt;			// const: float
			//~ char*	cstr;		// const: use: '.id.name'
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
			//int32_t _pad;
			symn	link;			// variable
			astn	args;			// next used
			//~ astn	nuse;			// next used
		} id;
		struct {					// STMT_brk, STMT_con
			long offs;
			long stks;				// stack size
		} go2;
	};
	symn		type;				// typeof() return type of operator ... base type of IDTF
	astn		next;				// next statement, do not use for preorder
};
struct symn {				// type node
	char*	name;
	char*	file;
	int		line;
	union {
	int32_t	size;	// sizeof(TYPE_xxx)
	int32_t	offs;	// addrof(TYPE_ref)
	};
	symn	decl;		// declared in
	symn	type;		// base type of TYPE_ref (void, int, float, struct, ...)
	symn	args;		// REC fields / FUN args
	symn	next;		// symbols on table/args

	uint8_t	cast;		// casts to type(TYPE_(ref, u32, i32, i64, f32, f64, p4x)).
	uint8_t	pack;		// alignment
	uint8_t	kind;		// TYPE_ref || TYPE_xxx

	uint8_t	call:1;		// function / callable => (kind == TYPE_ref && args)
	uint8_t	stat:1;		// static ?
	//~ uint8_t	load:1;		// indirect reference / eval param: "&": cast == TYPE_ref

	//~ uint8_t	priv:1;		// private
	uint8_t	read:1;		// const / no override

	//~ uint8_t	onst:1;		// stack  (val): replaced with (offs <= 0) ?(!stat)

	uint32_t	xx_1:5;		// -Wpadded
	uint16_t	xx_2;		// align
	uint16_t	nest;		// declaration level
	astn	init;		// VAR init / FUN body

	// list(scoping)
	astn	used;
	symn	defs;		// symbols on stack/all
	//~ symn	uses;		// declared in
	char*	pfmt;		// print format
};

struct ccState {
	state	s;
	//~ symn	defs;		// definitions
	astn	root;		// statements
	astn	jmps;		// jumps

	symn	all;		// all symbols TODO:Temp
	list	strt[TBLS];		// string table
	symn	deft[TBLS];		// definitions: hashStack;

	int		verb;		// verbosity
	int		warn;		// warning level
	int		nest;		// nest level: modified by (enter/leave)

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
			//~ int		_pad;		// next char
		};// file[TOKS];

		/*struct {		// current decl
			symn	csym;
			//~ symn	cref;
			//~ astn	stmt;

			//~ astn	_brk;	// list: break
			//~ astn	_con;	// list: continue
		} scope[TOKS];// */
	};

	astn	argz;		// no parameter of type void
	char	*_beg;
	char	*_end;
};
struct vmState {
	state	s;
	int		opti;			// optimization levevel

	unsigned int	px;			// exit point
	unsigned int	pc;			// entry point / prev program counter
	unsigned int	cs;			// code size
	unsigned int	ds;			// data size

	//~ unsigned int	ic;			// ?executed? / instruction count
	//~ unsigned int	pi[2];		// previous instructions

	unsigned int	ss;			// stack size / current stack size
	unsigned int	sm;			// stack minimum size

	unsigned int	pos;		// current positin in buffer
	//~ unsigned int	mark;
	//~ unsigned int	_pad;
	//~ unsigned long _cnt; // return (_end - _ptr);
	//~ unsigned long _len; // return (_ptr - _mem);
	//~ unsigned char *_ptr;
	unsigned char *_end;
	unsigned char *_mem;
	unsigned char _beg[];
};

static inline int kindOf(astn ast) {return ast ? ast->kind : 0;}

extern symn type_vid;
extern symn type_bol;
extern symn type_u32;
extern symn type_i32;
extern symn type_i64;
extern symn type_f32;
extern symn type_f64;
//~ extern symn type_chr;		// should be for printing only
extern symn type_str;
extern symn type_ref;
extern symn null_ref;

//~ extern symn type_v4f;
//~ extern symn type_v2d;

//~ extern symn void_arg;
extern symn emit_opc;


//~ clog
void fputfmt(FILE *fout, const char *msg, ...);

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
//~ void fputopc(FILE *fout, struct bcde* opc, int len, int offset);

// program error
void perr(state s, int level, const char *file, int line, const char *msg, ...);

//~ void dumpasm(FILE *fout, vmState s, int offs);
void dumpsym(FILE *fout, symn sym, int mode);
void dumpast(FILE *fout, astn ast, int mode);
void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

void dump2file(state s, char *file, dumpMode mode, char *text);

// Runtime / global state
void* getmem(state s, int size, unsigned clear);

symn newdefn(ccState s, int kind);
astn newnode(ccState s, int kind);
astn intnode(ccState s, int64_t v);
//~ astn fltnode(ccState s, float64_t v);
astn strnode(ccState s, char *v);
astn fh8node(ccState s, uint64_t v);
astn cpynode(ccState s, astn src);
void eatnode(ccState s, astn ast);

//~ symn installex(ccState s, const char* name, int kind, unsigned size, symn type, astn init);
symn installex2(ccState s, const char* name, int kind, unsigned size, symn type, astn init);
symn installex(ccState s, const char* name, int kind, symn type, int cast, unsigned size, astn init);
symn install(ccState s, const char* name, int kind, int cast, unsigned size);
symn declare(ccState s, int kind, astn tag, symn rtyp);

//~ TODO: !!! args are linked in a list by next !!??
symn lookup(ccState s, symn sym, astn ast/*, int deep*/, astn args);

//~ symn findsym(ccState s, symn in, char *name);
//~ int findnzv(ccState s, char *name);
//~ int findint(ccState s, char *name, int* res);
//~ int findflt(ccState s, char *name, double* res);

symn typecheck(ccState, symn loc, astn ast);

int argsize(symn sym, int align);
int fixargs(symn sym, int align, int spos);

int castOf(symn typ);
int castTo(astn ast, int tyId);
//~ int castTy(astn ast, symn type);
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
symn leave(ccState s, symn def);

/** try to evaluate an expression
 * @param res: where to put the result
 * @param ast: tree to be evaluated
 * @return one of [TYPE_err, TYPE_bit, TYPE_int, TYPE_flt, ?TYPE_str]
 * @todo should use the exec funtion, for the generated code.
 */
int eval(astn res, astn ast);

/** generate byte code for tree
 * @param ast: tree to be generated
 * @param get: one of TYPE_xxx
 * @return TYPE_xxx, 0 on error
int cgen(state s, astn ast, int get);
 */

/** emit an opcode with args
 * @param opc: opcode
 * @param ...: argument
 * @return: program counter
 */
int emit(vmState, int opc, stkval arg);
int emitopc(vmState, int opc);
int emiti32(vmState, int32_t arg);
int emiti64(vmState, int64_t arg);
int emitf32(vmState, float32_t arg);
int emitf64(vmState, float64_t arg);
int emitptr(vmState, void* arg);

int emitidx(vmState, int opc, int pos);
int emitint(vmState, int opc, int64_t arg);
int fixjump(vmState s, int src, int dst, int stc);

// returns the stack size
int stkoffs(vmState s, int size);
//int testopc(vmState s, int opc);

int isType(symn sym);
int istype(astn ast);

symn linkOf(astn ast);
long sizeOf(symn typ);

int source(ccState, srcType mode, char* text);		// mode: file/text

unsigned rehash(const char* str, unsigned size);

//TODO: Rename: reg str
char *mapstr(ccState s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

void fputasm(FILE *fout, vmState s, int mark, int len, int mode);

//~ void printargcast(astn arg);

int libCallExitDebug(state s);

#endif
