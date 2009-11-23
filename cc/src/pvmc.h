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
#include "api.h"

//~ #include <stdio.h>
#include <stdlib.h>

#define TODO_REM_TEMP

// maximum tokens in expressions & nest level
#define TOKS 2048

// symbol & hash table size
#define TBLS 1024

// debug, errors

#define pdbg(__DBG, msg, ...) {fputfmt(stderr, "%s:%d:"__DBG":%s: "msg"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); fflush(stderr);}
//~ #define pdbg(__DBG, msg, ...) {fputfmt(stderr, "%s:%d:"__DBG": "msg"\n", __FILE__, __LINE__, ##__VA_ARGS__); fflush(stderr);}
#define trace(__LEV, msg, ...) //pdbg("trace", ##__VA_ARGS__)
#define debug(msg, ...) pdbg("debug", msg, ##__VA_ARGS__)
#define fatal(msg, ...) {pdbg("fatal", msg, ##__VA_ARGS__); exit(-2);}
#define dieif(__EXP, msg, ...) {if (__EXP) fatal(msg, ##__VA_ARGS__)}

#if 0
#define error(__ENV, __LINE, msg, ...) perr(__ENV, -1, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) perr(__ENV, __LEVEL, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, 0, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#else
#define error(__ENV, __LINE, msg, ...) perr(__ENV, -1, (__ENV)->cc->file, __LINE, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)
#endif

// Symbols - CC
enum {
	OP = 0, ID = 1, TY = 2, XX = -1,
	#define TOKDEF(NAME, PREC, ARGC, KIND, STR) NAME,
	#include "incl/defs.h"
	tok_last,
};
typedef struct {
	int const	kind;
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

	opc_neg,
	opc_add,
	opc_sub,
	opc_mul,
	opc_div,
	opc_mod,

	opc_shl,
	opc_shr,
	opc_and,
	opc_ior,
	opc_xor,

	opc_ceq,
	opc_clt,
	opc_cle,
	opc_cne,
	opc_cgt,
	opc_cge,

	//~ opc_ldc,
	opc_ldi,
	opc_sti,

	opc_line,		// line info
	get_ip,		// instruction pointer
	get_sp,		// get stack pointer
	set_sp,		// set stack pointer
	seg_code,	// pc = ptr - beg; ptr += code->cs;
	loc_data,

	//~ seg_data,
	//~ vminf_ip,		// instruction pointer
	//~ vminf_sp,		// stack pointer
	//~ vminf_pos,
	//~ vminf_cnt,
	//~ opc_file,		// file info
	//~ opc_docc,		// doc comment

	//~ opc_ldcr = opc_ldc4,
	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,
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

typedef struct {	// i32x4
	uns32t x;
	uns32t y;
	uns32t z;
	uns32t w;
} u32x4t;
typedef struct {	// i64x2: rational
	uns64t x;
	uns64t y;
} u32x2t;
typedef struct {	// f32x4: vector
	flt32t x;
	flt32t y;
	flt32t z;
	flt32t w;
} f32x4t;
typedef struct {	// f64x2: complex
	flt64t x;
	flt64t y;
} f64x2t;

typedef union {		// stack value type
	int08t	i1;
	uns08t	u1;
	int16t	i2;
	uns16t	u2;
	int32t	i4;
	uns32t	u4;
	int64t	i8;
	uns64t	u8;
	flt32t	f4;
	flt64t	f8;
	//TODO: remove pf, pd, x4
	f32x4t	pf;
	f64x2t	pd;
	u32x4t	x4;
} stkval;

typedef struct listn_t *list, **strt;
typedef struct symn **symt;		// sym (astn, table)

struct listn_t {			// linked list astn
	struct listn_t	*next;
	unsigned char	*data;
	//~ unsigned int	size;	// := &this - data
};

/*struct buff {
	long cnt;
	char *ptr;
	char beg[];
};

//~ inline int getBuffChr(struct buff *b) {if (--b->cnt > 0) return *b->ptr++; return 0;}
//~ inline int getBuffChr(struct buff *b) {return *(--s->_cnt, s->_ptr++);}
// */

struct astn {				// ast astn
	uns32t		line;				// token on line / code offset
	uns08t		kind;				// code: TYPE_ref, OPER_???, CNST_???
	uns08t		cast;				// cast To castId(this->type);
	uns16t		XXXX;				// TYPE_ref hash / priority level
	union {
		/*int64t		cint;			// cnst: integer
		flt64t		cflt;			// cnst: float
		struct {
			union {
				astn	stmt;			// STMT: statement block
				astn	rhso;			// OPER: left hand side operand / true block / statement
			};
			union {
				astn	step;			// STMT: for statement increment expression / if else block
				astn	lhso;			// OPER: right hand side operand / false block / increment
				char*	name;			// IDTF
			};
			union {
				astn	test;			// OPER: ?: operator condition / if for while condition
				long	hash;			// IDTF
			};
			union {
				astn	init;			// STMT: just for statement 'for' & nop
				symn	link;			// IDTF/OPER: link ? TYPE_ref : TYPE_def
				// temp values
				long	pril;			// OPER: used temporarly by cast();
			};
		};// */
		//~ /*
		union  {					// CNST_xxx: constant
			int64t	cint;			// cnst: integer
			flt64t	cflt;			// cnst: float
			char*	cstr;			// cnst: float
			//~ int32t	cpi4[4];		// rgb
			//~ int64t	cpi2[2];		// rat
			//~ flt32t	cpf4[4];		// vec
			//~ flt64t	cpf2[2];		// cpl
		} cnst;
		struct {					// OPER_xxx: operator
			astn	rhso;			// right hand side operand
			astn	lhso;			// left hand side operand
			astn	test;			// ?: operator condition
			//~ symn	link;			// assigned operator convert to function call
		} op;
		struct {					// STMT_xxx: statement
			astn	stmt;			// statement / then block
			astn	step;			// increment / else block
			astn	test;			// condition: if, for
			astn	init;			// for statement init
		} stmt;
		struct {					// TYPE_xxx: identifyer
			//~ symn	decl;			// link to reference
			char*	name;			// name of identifyer
			uns32t	hash;			// hash code for 'name'
			symn	link;			// func body / replacement
		} id;
		/*struct {					// list of next-linked nodes
			astn	next;
			astn	tail;
			uns32t	count;
			uns32t	extra;
		} list;// */
	};
	symn		type;				// typeof() return type of operator ... base type of IDTF
	astn		next;				// 
};
struct symn {				// symbol
	char*	name;
	char*	file;
	int		line;

	union {
	int32t	size;	// addrof(TYPE_ref)
	int32t	offs;	// sizeof(TYPE_xxx)
	};
	symn	type;		// base type of TYPE_ref (void, int, float, struct, ...)
	symn	args;		// REC fields / FUN args
	// reflection ends here

	uns08t	kind;		// TYPE_ref || TYPE_xxx
	//~ uns08t	priv:1;		// private
	//~ uns08t	stat:1;		// static
	//~ uns08t	read:1;		// const
	//~ uns08t	used:1;		// used(ref/def)
	//~ uns08t	onst:1;		// temp / on stack(val) (probably better (offs < 0))
	uns08t	libc:1;		// native (fun)
	uns08t	call:1;		// is a function
	//~ uns08t	asgn:1;		// assigned a value
	uns16t	nest;		// declaration level
	astn	init;		// VAR init / FUN body

	// list(scoping)
	symn	defs;		// simbols on stack/all
	symn	next;		// simbols on table/args
};

struct ccEnv {
	state	s;
	int		warn;		// warning level
	astn	root;		// statements
	symn	defs;		// definitions

	symn	all;		// all symbols TODO:Temp
	strt	strt;		// string table
	symt	deft;		// definitions: hashStack;

	int		nest;		// nest level: modified by (enterblock/leaveblock)

	struct {
		struct {			// Lexer
			char*	file;	// current file name
			int		line;	// current line number
			// INPUT
			struct {
				int		_chr;		// saved char
				int		_fin;		// file handle (-1) for cc_buff()
				int		_cnt;		// chars left in buffer
				char*	_ptr;		// pointer parsing trough source
				uns08t	_buf[1024];	// cache
			};//fin;
			astn	tokp;		// token pool
			astn	_tok;		// next token
		};// file[TOKS];

		/*struct {		// current decl
			symn	csym;
			//~ symn	cref;
			//~ astn	stmt;

			//~ astn	_brk;	// list: break
			//~ astn	_con;	// list: continue
		} scope[TOKS];// */
	};

	char	*buffp;					// TODO: remove this
};
struct vmEnv {
	state	s;
	unsigned int	pc;			// entry point / prev program counter
	unsigned int	ic;			// ?executed? / instruction count
	unsigned int	cs;			// code size

	unsigned int	ss;			// stack size / current stack size
	unsigned int	sm;			// stack minimum size

	unsigned int	ds;			// data size

	/** memory mapping
	 *	data(RW):ds		:	initialized data
	 *	code(RX):cs		:	instructions
	 *	heap(RW):		:	
	 *	stack(RW):ss	:	
	 *
	+mp------------+ip------------+hp------------+bp---------------+sp
	|   code[cs]   |   data[ds]   |   heap[??]   |  stack[ss*cc]   |
	+--------------+--------------+--------------+-----------------+
	?      r-x     |      r--     |     rw-      |       rw-       |
	+--------------+--------------+--------------+-----------------+
	enum Segment
	{
		Seg_Data = 0,
		Seg_Init = 1,
		Seg_Code = 1,
	};
	**/

	//~ unsigned long _cnt; // return (_end - _ptr);
	//~ unsigned long _len; // return (_ptr - _mem);
	unsigned char *_ptr;
	unsigned char *_end;
	unsigned char _mem[];
};

static inline int kindOf(astn ast) {return ast ? ast->kind : 0;}

extern symn type_vid;
extern symn type_bol;
extern symn type_u32;
extern symn type_i32;
extern symn type_i64;
extern symn type_f32;
extern symn type_f64;
extern symn type_str;

extern symn emit_opc;
extern symn type_f32x4;
extern symn type_f64x2;

//~ util
int parseInt(const char *str, int *res, int hexchr);
char* parsecmd(char *ptr, char *cmd, char *sws);

char *strfindstr(const char *t, const char *p, int flags);

//~ clog
void fputfmt(FILE *fout, const char *msg, ...);
void perr(state s, int level, const char *file, int line, const char *msg, ...);

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
//~ void fputopc(FILE *fout, bcde opc, int len, int offset);
void fputasm(FILE *fout, unsigned char *beg, int len, int mode);
//~ void fputasm(FILE *fout, void *ip, int len, int length);

void dumpasm(FILE *fout, vmEnv s, int offs);
void dumpsym(FILE *fout, symn sym, int mode);
void dumpast(FILE *fout, astn ast, int mode);
//~ void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

void* getmem(state s, int size);

astn newnode(ccEnv s, int kind);
astn cpynode(ccEnv s, astn src);
void eatnode(ccEnv s, astn ast);
astn intnode(ccEnv s, int64t v);
//~ astn fltnode(ccEnv s, flt64t v);
astn strnode(ccEnv s, char *v);
astn fh8node(ccEnv s, uns64t v);

symn newdefn(ccEnv s, int kind);
symn installex(ccEnv s, int kind, const char* name, unsigned size, symn type, astn init);
symn install(ccEnv s, int kind, const char* name, unsigned size);
symn declare(ccEnv s, int kind, astn tag, symn rtyp);
symn lookin(ccEnv s, symn sym, astn ast, astn args);
symn lookup(ccEnv s, symn loc, astn ast);

int32t constbol(astn ast);
//~ int64t constint(astn ast);
//~ flt64t constflt(astn ast);

astn peek(ccEnv);
//~ astn next(ccEnv, int kind);
//~ void back(ccEnv, astn ast);
//~ int  skip(ccEnv, int kind);
//~ int  test(ccEnv, int kind);

astn expr(ccEnv, int mode);		// parse expression	(mode: lookup)
//~ astn decl(ccEnv, int mode);		// parse declaration	(mode: enable expr)
//~ astn stmt(ccEnv, int mode);		// parse statement	(mode: enable decl)
int scan(ccEnv, int mode);		// parse program	(mode: script mode)

void enter(ccEnv s, symn def);
symn leave(ccEnv s);

/** try to evaluate an expression
 * @return CNST_xxx if expression can be folded 0 otherwise
 * @param ast: tree to be evaluated
 * @param res: where to put the result
 * @param get: one of TYPE_xxx
 */

int eval(astn res, astn ast, int get);
int cgen(state s, astn ast, int get);		// ret: typeId(ast)

/** generate byte code for tree
 * @return TYPE_xxx, 0 on error
 * @param ast: tree to be generated
 * @param get: one of TYPE_xxx
 */
int emit(vmEnv, int opc, ...);			// ret: program counter
int emiti32(vmEnv, int32t arg);
int emiti64(vmEnv, int64t arg);
int emitf32(vmEnv, flt32t arg);
int emitf64(vmEnv, flt64t arg);
int emitidx(vmEnv, int opc, int stpos);
int emitint(vmEnv, int opc, int64t arg);
int fixjump(vmEnv s, int src, int dst, int stc);

int isType(symn sym);
int istype(astn ast);

int isemit(astn ast);
//~ int islval(astn ast);
symn linkOf(astn ast);

int castId(symn ast);
int castOp(symn lhs, symn rhs, int uns);
//~ int castTo(astn ast, symn typ);

int source(ccEnv, srcType mode, char* text);		// mode: file/text

//~ void installlibc(state, void call(state), const char* proto);

int rehash(const char* str, unsigned size);
//~ int align(int offs, int pack, int size);
char *mapstr(ccEnv s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);


void vm_tags(ccEnv s, char *sptr, int slen);
int dbgNone(vmEnv vm, int pu, void *ip, long* sptr, int sc);
int dbgCon(vmEnv vm, int pu, void *ip, long* sptr, int sc);
int nodbg(vmEnv vm, int pu, void *ip, long* sptr, int sc);

#endif
