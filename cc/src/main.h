/*******************************************************************************
 *   File: main.c
 *   Date: 2007/04/20
 *   Desc: main header
 *******************************************************************************
 *
 *
*******************************************************************************/
#include "pvm.h"

#ifndef __CCVM_H
#define __CCVM_H

#include <stdio.h>
#include <stdlib.h>

#define TODO_REM_TEMP 3

// maximum tokens in expressions & nest level
#define TOKS 2048

// symbol & hash table size
#define TBLS 1024

// static memory size for state
// TODO: malloc-it
#define bufmaxlen (128<<20)

#define SRCPOS "src\\" __FILE__, __LINE__, __func__

#define trace(__LEV, msg, ...)// {fputfmt(stderr, "%s:%d:trace:%s: "msg"\n", SRCPOS, ##__VA_ARGS__);fflush(stderr);}
#define debug(msg, ...) {fputfmt(stdout, "%s:%d:debug:%s: "msg"\n", SRCPOS, ##__VA_ARGS__);fflush(stderr);}
#define fatal(msg, ...) { fputfmt(stderr, "%s:%d:fatal:%s: "msg"\n", SRCPOS, ##__VA_ARGS__); fflush(stderr); exit(-2);}
#define dieif(__EXP, msg, ...) {if (__EXP) {fputfmt(stderr, "%s:%d:fatal(`"#__EXP"`):%s: "msg"\n", SRCPOS, ##__VA_ARGS__); fflush(stderr); exit(-1);}}

#if 0
#define error(__ENV, __LINE, msg, ...) perr(__ENV, -1, SRCPTH __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) perr(__ENV, __LEVEL, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, 0, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#else
#define error(__ENV, __LINE, msg, ...) perr(__ENV, -1, (__ENV)->file, __LINE, msg, ##__VA_ARGS__)
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

	//~ opc_ldc = 0x0100,
	//~ opc_ldi,
	//~ opc_sti,

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

	//~ opc_cz = 0x02000 << 0,
	opc_ceq,// = 0x02001 << 0,
	opc_clt,// = 0x02002 << 0,
	opc_cle,// = 0x02003 << 0,
	//~ cmp_i32,// = 0x02000 << 2,
	//~ cmp_f32,// = 0x02001 << 2,
	//~ cmp_i64,// = 0x02002 << 2,
	//~ cmp_f64 = 0x02003 << 2,
	//~ cmp_tun = 0x02001 << 4,		// unsigned / unordered
	//~ opc_not,// = 0x02001 << 5,		// 
	//~ opc_cnz,// = opc_not + opc_c2z,
	opc_cne,// = opc_not + opc_ceq,
	opc_cgt,// = opc_not + opc_cle,
	opc_cge,// = opc_not + opc_clt,

	//~ opc_ldcr = opc_ldc4,
	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,

	opc_line,		// line info
	get_ip,		// instruction pointer
	get_sp,		// stack pointer
	seg_code,	// pc = ptr - beg; ptr += code->cs;
	loc_data,
	//~ seg_data,
	//~ vminf_ip,		// instruction pointer
	//~ vminf_sp,		// stack pointer
	//~ vminf_pos,
	//~ vminf_cnt,
	//~ opc_file,		// file info
	//~ opc_docc,		// doc comment
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

/*typedef union temps{
	uns08t _b[16];
	uns16t _w[8];
	uns32t _l[4];
	uns64t _q[2];
	flt32t _s[4];
	flt64t _d[2];
} pd128t;

typedef union {
	uint8_t _b[16];
	uint16_t _w[8];
	uint32_t _l[4];
	uint64_t _q[2];
	flt32t _s[4];
	flt64t _d[2];
} XMMReg;

typedef union {
	uint8_t _b[8];
	uint16_t _w[2];
	uint32_t _l[1];
	uint64_t q;
} MMXReg;

//{ list
//~ 0000: .. .. .. ..
//~ ....: .. .. .. ..
//~ 8000: .. .. .. ..: [data+00]
//~ 8004: .. .. .. ..: [data+04]
//~ 8008: .. .. .. ..: [data+08]
//~ 800C: .. .. .. ..: [data+0C]
//~ 8010: .. .. .. ..: [data+10]
//~ 8014: .. .. .. ..: [data+14]
//~ 8018: .. .. .. ..: [data+18]
//~ ....: .. .. .. ..: 
//~ 8040: 00 00 00 00: next
//~ 8044: 00 00 80 04: data => list.dlen = list.data - &list.next;
//}
//{ packed data
//~ typedef union    p2f64t;
//~ typedef union    p4f32t;
//~ typedef union    p8f16t;

//~ typedef union    p2i64t;
//~ typedef union    p4i32t;
//~ typedef union    p8i16t;
//~ typedef union    p16i8t;

//~ typedef union    p2u64t;
//~ typedef union    p4u32t;
//~ typedef union    p8u16t;
//~ typedef union    p16u8t;
//} */

typedef struct listn_t *list, **strt;
typedef struct symn **symt;		// sym (astn, table)

struct listn_t {			// linked list astn
	struct listn_t	*next;
	unsigned char	*data;
	//~ unsigned int	size;
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
		int64t		cint;			// cnst: integer
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
		/*
		union  {					// CNST_xxx: constant
			int64t	cint;			// cnst: integer
			flt64t	cflt;			// cnst: float
			//~ int32t	cpi4[2];		// ???
			//~ int64t	cpi2[2];		// rat
			//~ flt32t	cpf4[4];		// vec
			//~ flt64t	cpf2[2];		// cpl
		} cnst;
		struct {				// TYPE_xxx: typename
			symn	decl;			// link to reference
			astn	link;			// func body / replacement
			char*	name;			// name of identifyer
			uns32t	hash;			// hash code for 'name'
		} idtf;
		struct {				// OPER_xxx: operator
			astn	rhso;			// right hand side operand
			astn	lhso;			// left hand side operand
			astn	test;			// ?: operator condition
			symn	call;			// assigned operator
		} oper;
		struct {				// STMT_xxx: statement
			astn	stmt;			// statement / then block
			astn	step;			// increment / else block
			astn	test;			// condition: if, for
			astn	init;			// for statement init
		} stmt;
		/ *struct {					// list of next-linked nodes
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
	uns08t	kind;		// TYPE_ref || TYPE_xxx
	uns16t	nest;		// declaration level

	//~ uns08t	priv:1;		// private
	//~ uns08t	stat:1;		// static
	//~ uns08t	read:1;		// const
	//~ uns08t	used:1;		// used(ref/def)
	//~ uns08t	onst:1;		// temp / on stack(val) (probably better (offs < 0))
	uns08t	libc:1;		// native (fun)
	uns08t	call:1;		// is a function
	//~ uns08t	asgn:1;		// assigned a value

	union {int32t	size, offs;};	// addrof(TYPE_ref) / sizeof(TYPE_xxx)
	symn	type;		// base type of TYPE_ref (void, int, float, struct, ...)
	symn	args;		// REC fields / FUN args
	astn	init;		// VAR init / FUN body

	char*	name;
	char*	file;
	int		line;

	// list(scoping)
	symn	defs;		// simbols on stack/all
	symn	next;		// simbols on table/args
};

/* symbol for references/reflection
struct symn {
	char*	name;
	char*	file;
	int		line;

	int32t	offs;		// addrof(TYPE_ref) / sizeof(TYPE_ref)

	symn	type;		// base type
	symn	args[];		// REC fields / FUN args
};// */

struct state_t {			// enviroment (context)
	int		errc;			// error count
	FILE*	logf;			// log file (errors + warnings)
	symn	glob;			// symbols
	symn	all;			// symbols

	struct {				// Scanner
		int		warn;		// warning level
		int		copt;		// optimization levevel
		int		nest;		// nest level: modified by (enterblock/leaveblock)
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
			// BUFFER
		};// file[TOKS];

		astn	root;		// program

		strt	strt;		// string table
		symt	deft;		// definitions / declarations

		symn	defs;		// definitions / declarations stack

		struct {		// current decl
			symn	csym;
			//~ symn	cref;
			//~ astn	stmt;

			//~ astn	_brk;	// list: break
			//~ astn	_con;	// list: continue
		} scope[TOKS];
	};
	vmEnv	code;
	//~ struct scan_t *scan;
	//~ struct code_t *code;

	char	*buffp;					// ???
	//~ struct buff alma;
	char	buffer[bufmaxlen+2];	// buffer base (!!! static)
};

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
int parseint(const char *str, int *res, int hexchr);
char *strfindstr(const char *t, const char *p, int flags);

//~ clog
void fputfmt(FILE *fout, const char *msg, ...);
void perr(state s, int level, const char *file, int line, const char *msg, ...);

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
//~ void fputopc(FILE *fout, bcde opc, int len, int offset);

//~ void fputasm(FILE *fout, void *ip, int len, int length);

void dumpasm(FILE *fout, vmEnv s, int offs);
void dumpsym(FILE *fout, symn sym, int mode);
//~ void dumpast(FILE *fout, astn ast, int mode);

//~ void dumpxml(FILE *fout, astn ast, int mode);
//~ void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

typedef enum {
	//~ dump_bin = 0x0000100,
	dump_sym = 0x0000200,
	dump_asm = 0x0000300,
	dump_ast = 0x0000400,
} dumpWhat;

void dump(state, FILE*, dumpWhat);

astn newnode(state s, int kind);
void eatnode(state s, astn ast);
astn intnode(state s, int64t v);
//~ astn fltnode(state s, flt64t v);
astn strnode(state s, char *v);
astn fh8node(state s, uns64t v);

symn newdefn(state s, int kind);
symn install(state s, int kind, const char* name, unsigned size);
symn declare(state s, int kind, astn tag, symn rtyp, symn args);
symn lookup(state s, symn loc, astn ast);

int32t constbol(astn ast);
//~ int64t constint(astn ast);
//~ flt64t constflt(astn ast);

//~ int source(state, int mode, char* text);		// mode: file/text

astn peek(state);
//~ astn next(state, int kind);
//~ void back(state, astn ast);
//~ int  skip(state, int kind);
//~ int  test(state, int kind);

astn expr(state, int mode);		// parse expression	(mode: ?)
//~ astn decl(state, int mode);		// parse declaration	(mode: enable expr)
//~ astn stmt(state, int mode);		// parse statement	(mode: enable decl)
astn scan(state, int mode);		// parse program	(mode: script mode)

void enter(state s, symn def);
symn leave(state s, symn def);

/** try to evaluate an expression
 * @return CNST_xxx if expression can be folded 0 otherwise
 * @param ast: tree to be evaluated
 * @param res: where to put the result
 * @param get: one of TYPE_xxx
 */
int eval(astn res, astn ast, int get);

/** generate byte code for tree
 * @return TYPE_xxx, 0 on error
 * @param ast: tree to be generated
 * @param get: one of TYPE_xxx
 */
int cgen(state s, astn ast, int get);

int istype(astn ast);
int isemit(astn ast);
int islval(astn ast);
symn linkOf(astn ast);

int castId(symn ast);
int castOp(symn lhs, symn rhs);

void vm_info(vmEnv);
int cc_init(state);
int cc_done(state);

vmEnv vmGetEnv(state env, int size, int ss);

int fixjump(vmEnv s, int src, int dst, int stc);
void installlibc(state, void call(libcarg), const char* proto);

int rehash(const char* str, unsigned size);
//~ int align(int offs, int pack, int size);
char *mapstr(state s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

#endif
