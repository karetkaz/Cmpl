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
// TODO : malloc-it
#define bufmaxlen (128<<20)

#define trace(__LEV, msg, ...)// {fputmsg(stderr, "%s:%d:trace[%04x]:"msg"\n", __FILE__, __LINE__, __LEV, ##__VA_ARGS__);fflush(stderr);}
#define debug(msg, ...) {fputmsg(stdout, "%s:%d:%s:debug:"msg"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);fflush(stderr);}
#define debug2(__FILE, __LINE, msg, ...) {fputmsg(stderr, "%s:%d:%s:debug:"msg"\n", __FILE, __LINE, __func__, ##__VA_ARGS__);fflush(stderr);}
#define fatal(__ENV, msg, ...) { fputmsg(stderr, "%s:%d:%s:fatal:"msg"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); fflush(stderr);}
#define dieif(__EXP) if (__EXP) { fputmsg(stderr, "%s:%d:debug: `"#__EXP"`\n", __FILE__, __LINE__); fflush(stderr); exit(-1);}

#if 0
#define error(__ENV, __LINE, msg, ...) perr(__ENV, -1, __FILE__, __LINE__, msg, ##__VA_ARGS__)
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

	opc_ldc = 0x0100,
	opc_ldi,
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
	//~ opc_cle,// = 0x02003 << 0,
	//~ cmp_i32,// = 0x02000 << 2,
	//~ cmp_f32,// = 0x02001 << 2,
	//~ cmp_i64,// = 0x02002 << 2,
	//~ cmp_f64 = 0x02003 << 2,
	//~ cmp_tun = 0x02001 << 4,		// unsigned / unordered
	//~ opc_not,// = 0x02001 << 5,		// 
	//~ opc_cnz,// = opc_not + opc_c2z,
	//~ opc_cne,// = opc_not + opc_ceq,
	opc_cgt,// = opc_not + opc_cle,
	//~ opc_cge,// = opc_not + opc_clt,

	//~ opc_ldcr = opc_ldc4,
	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,

	opc_line,		// line info
	//~ opc_file,		// file info
	//~ opc_docc,		// doc comment
};

typedef struct {
	int const	code;
	int const	size;
	int const	chck;
	int const	pops;
	char const *name;
} opc_inf;
extern const opc_inf opc_tbl[255];

struct libcargv {
	unsigned char* retv;	// retval
	unsigned char* argv;	// 
};

typedef struct {
	uns32t x;
	uns32t y;
	uns32t z;
	uns32t w;
} u32x4t;
typedef struct {
	uns32t x;
	uns32t y;
} u32x2t;

typedef struct {
	flt32t x;
	flt32t y;
	flt32t z;
	flt32t w;
} f32x4t;
typedef struct {
	flt64t x;
	flt64t y;
} f64x2t;

/*typedef union temps{
	uns08t _b[16];
	uns16t _w[8];
	uns32t _l[4];
	uns64t _q[2];
	flt32t _s[4];
	flt64t _d[2];
} pd128t;

typedef union {
	flt32t d[4];
	struct {
		flt32t x;
		flt32t y;
		flt32t z;
		flt32t w;
	};
	struct {
		flt32t r;
		flt32t g;
		flt32t b;
		flt32t a;
	};
} f32x4t, *f32x4;

typedef union {
	flt64t d[2];
	struct {
		flt64t x;
		flt64t y;
	};
	struct {
		flt64t re;
		flt64t im;
	};
} f64x2t, *f64x2;

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

typedef struct listn_t *list, **strT;
typedef struct symn_t *defn, **defT;
//~ typedef struct state_t *state;
//~ typedef struct astn_t *node;
typedef struct bcdc_t *bcde;

typedef struct cell_t {			// processor
	//~ struct cell_t*		next;
	//~ unsigned int	ei;			// instruction count / exec
	//~ unsigned int	wi;			// instruction count / wait
	//~ unsigned int	ri;			// instruction count / halt

						// exec / emit
	//~ unsigned int	ds;			// data size
	//~ unsigned int	tc;			// instruction (ticks / count)

	//~ unsigned int	ss;			// ss / stack size
	unsigned int	cs;			// child start(==0 join) / code size (pc)
	unsigned int	pp;			// parent proc(==0 main) / ???

	// flags[?]
	unsigned	zf:1;			// zero flag
	unsigned	sf:1;			// sign flag
	unsigned	cf:1;			// carry flag
	unsigned	of:1;			// overflow flag

	unsigned char	*ip;			// Instruction pointer		/ prev1 ip
	unsigned char	*sp;			// Stack pointer(bp + ss)	/ prev2 ip
	unsigned char	*bp;			// Stack base			/ prev3 ip
} *cell;

struct listn_t {			// linked list node
	struct listn_t	*next;
	unsigned char	*data;
	//~ unsigned int	size;
};

struct astn_t {				// ast node
	uns32t		line;				// token on line / code offset
	uns08t		kind;				// code : TYPE_ref, OPER_???, CNST_???
	uns08t		cast;				// cast To castId(this->type);
	uns16t		XXXX;				// TYPE_ref hash / priority level
	union {
		int64t		cint;			// cnst : integer
		flt64t		cflt;			// cnst : float
		struct {
			union {
				node	stmt;			// STMT : statement block
				node	rhso;			// OPER : left hand side operand / true block / statement
			};
			union {
				node	step;			// STMT : for statement increment expression / if else block
				node	lhso;			// OPER : right hand side operand / false block / increment
				char*	name;			// IDTF
			};
			union {
				node	test;			// OPER : ?: operator condition / if for while condition
				long	hash;			// IDTF
			};
			union {
				node	init;			// STMT : just for statement 'for' & nop
				defn	link;			// IDTF/OPER : link ? TYPE_ref : TYPE_def
				// temp values
				long	pril;			// OPER : used temporarly by cast();
			};
		};// */
		/*
		union  {					// CNST_xxx : constant
			int64t	cint;			// cnst : integer
			flt64t	cflt;			// cnst : float
			//~ int32t	cpi4[2];		// ???
			//~ int64t	cpi2[2];		// rat
			//~ flt32t	cpf4[4];		// vec
			//~ flt64t	cpf2[2];		// cpl
		} cnst;
		struct {				// TYPE_xxx : typename
			defn	decl;			// link to reference
			node	link;			// func body / replacement
			char*	name;			// name of identifyer
			uns32t	hash;			// hash code for 'name'
		} idtf;
		struct {				// OPER_xxx : operator
			node	rhso;			// right hand side operand
			node	lhso;			// left hand side operand
			node	test;			// ?: operator condition
			defn	call;			// assigned operator
		} oper;
		struct {				// STMT_xxx : statement
			node	stmt;			// statement / then block
			node	step;			// increment / else block
			node	test;			// condition : if, for
			node	init;			// for statement init
		} stmt;
		/ *struct {					// list of next-linked nodes
			node	next;
			node	tail;
			uns32t	count;
			uns32t	extra;
		} list;// */
	};
	defn		type;				// typeof() return type of operator ... base type of IDTF
	node		next;				// 
};
struct symn_t {				// symbol
	uns08t	kind;		// TYPE_ref || TYPE_xxx
	uns16t	nest;		// declaration level

	//~ uns08t	priv:1;		// private
	//~ uns08t	stat:1;		// static
	//~ uns08t	read:1;		// final? (const)
	uns08t	used:1;		// used(ref/def)
	uns08t	onst:1;		// temp / on stack(val)
	uns08t	libc:1;		// native (fun / var)
	uns08t	call:1;		// is a function
	//~ uns08t	asgn:1;		// assigned a value

	union {uns32t	size, offs;};	// addrof(TYPE_ref) / sizeof(TYPE_xxx)
	defn	type;		// base type of TYPE_ref (void, int, float, struct, ...)
	defn	args;		// REC fields / FUN args
	node	init;		// VAR init / FUN body

	char*	name;
	char*	file;
	int		line;

	// list(scoping)
	defn	defs;		// simbols on stack
	defn	next;		// simbols on table/args

};

struct buff_t {
	char *ptr;
	char *end;
	char beg[];
};

struct state_t {			// enviroment (context)
	int		errc;			// error count
	FILE*	logf;			// log file (errors + warnings)

	struct {				// Scanner
		int		warn;		// warning level
		int		copt;		// optimization levevel
		int		nest;		// nest level : modified by (enterblock/leaveblock)
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
			node	tokp;		// token pool
			node	_tok;		// next token
			// BUFFER
		};// file[TOKS];

		node	root;		// program
		defn	glob;		// definitions : leave();

		strT	strt;		// string table
		defT	deft;		// definitions / declarations
		defn	defs;		// definitions / declarations stack


		struct {		// current decl
			defn	csym;
			//~ defn	cref;
			//~ node	stmt;

			//~ node	_brk;	// list: break
			//~ node	_con;	// list: continue
		} scope[TOKS];
	};
	struct {				// bytecode
		unsigned char*	mem;			// memory == buffp
		unsigned long	mems;			// memory size ?

		//~ unsigned int	tick;			// 
		unsigned int	sm;			// stack min size
		unsigned int	pc;			// prev / program counter
		unsigned int	ic;			// instruction count / executed

		unsigned int	ss;			// stack size on return
		unsigned int	cs;			// code size / program counter
		unsigned int	ds;			// data size
		//~ cell		pu;			// procs

		//~ unsigned char*	ip;			// Instruction pointer
		/** memory mapping
		 *	data(RW):ds		:	initialized data
		 *	code(RX):cs		:	instructions
		 *	heap(RW):		:	hs
		 *	stack(RW):		:	
		 *
		+mp------------+ip------------+hp------------+bp---------------+sp
		|   code[cs]   |   data[ds]   |   heap[??]   |  stack[ss*cc]   |
		+--------------+--------------+--------------+-----------------+
		?      r-x     |      r--     |     rw-      |       rw-       |
		+--------------+--------------+--------------+-----------------+
		enum Segment
		{
			Seg_Data = 0,
			Seg_Code = 1,
		};
		**/
	} code;
	//~ struct scan_t *scan;
	//~ struct code_t *code;
	//~ unsigned char *ptr;
	//~ unsigned char *end;
	//~ unsigned char buff[];
	char	*buffp;					// ???
	char	buffer[bufmaxlen+2];	// buffer base (!!! static)
};

extern defn type_vid;
extern defn type_bol;
extern defn type_u32;
extern defn type_i32;
extern defn type_i64;
extern defn type_f32;
extern defn type_f64;
extern defn type_str;

extern defn emit_opc;

void fputmsg(FILE *fout, const char *msg, ...);
//~ void fputsym(FILE *fout, defn sym, int mode);
void fputast(FILE *fout, node ast, int mode, int pri);
void fputasm(FILE *fout, bcde ip, int len, int offs);
void perr(state s, int level, const char *file, int line, const char *msg, ...);

defn newdefn(state s, int kind);
node newnode(state s, int kind);
void eatnode(state s, node ast);

int source(state, int mode, char* text);		// mode: file/text

defn lookup(state s, defn loc, node ast);
defn install(state s, int kind, const char* name, unsigned size);
defn declare(state s, int kind, node tag, defn rtyp, defn args);

//~ node fltnode(state s, flt64t v);
node intnode(state s, int64t v);
node fh8node(state s, uns64t v);
node strnode(state s, char *v);

int32t constbol(node ast);
//~ int64t constint(node ast);
//~ flt64t constflt(node ast);

//~ int cast(state s, node ptr, node arg);
int istype(node ast);
//~ int typeId(node ast);
//~ int castid(node lhs, node rhs);

node peek(state);
node next(state, int kind);
void back(state, node ast);

int  skip(state, int kind);
int  test(state, int kind);

node expr(state, int mode);		// parse expression	(mode : ?)
//~ node decl(state, int mode);		// parse declaration	(mode : enable expr)
//~ node stmt(state, int mode);		// parse statement	(mode : enable decl)
node scan(state, int mode);		// parse program	(mode : script mode)

void enter(state s, defn def);
defn leave(state s);

/** try to evaluate an expression
 * @return CNST_xxx if expression can be folded 0 otherwise
 * @param ast: tree to be evaluated
 * @param res: where to put the result
 * @param get: one of TYPE_xxx
 */
int eval(node res, node ast, int get);
/** generate byte code for tree
 * @return TYPE_xxx, 0 on error
 * @param ast: tree to be generated
 * @param get: one of TYPE_xxx
 */
int cgen(state s, node ast, int get);

int cc_init(state);
int cc_done(state);

int rehash(const char* str, unsigned size);
//~ int align(int offs, int pack, int size);
char *mapstr(state s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

#endif
