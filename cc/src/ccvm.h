#ifndef __CCVM_H
#define __CCVM_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

// maximum tokens in expressions
// maximum nest level
#define TOKS 2048
// symbol & hash table size
#define TBLS 1024

// static memory size for state
// TODO : should be malloc-ed or not
#define bufmaxlen (128<<20)

#define trace(__LEV, msg, ...)// {fputmsg(stderr, "%s:%d:trace[%04x]:"msg"\n", __FILE__, __LINE__, __LEV, ##__VA_ARGS__);fflush(stderr);}
#define debug(msg, ...) {fputmsg(stderr, "%s:%d:debug:"msg"\n", __FILE__, __LINE__, ##__VA_ARGS__);fflush(stderr);}
#define fatal(__ENV, msg, ...) {fputmsg(stderr, "%s:%d:debug:"msg"\n", __FILE__, __LINE__, ##__VA_ARGS__); fflush(stderr); /*exit(-1);*/}

#define error(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, -1, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) perr(__ENV, __LEVEL, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, 0, __FILE__, __LINE__, msg, ##__VA_ARGS__)

//~ #define error(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, -1, __FILE, __LINE, msg, ##__VA_ARGS__)
//~ #define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
//~ #define info(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)


// Symbols - CC
enum {
	OP = 0, ID = 1, TY = 2, XX = -1,
	#define TOKDEF(NAME, PREC, ARGC, KIND, STR) NAME,
	#include "incl/defs.inl"
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
	#include "incl/defs.inl"
	opc_last,

	opc_ldc = 0x0100,
	opc_ldi,
	opc_sti,

	opc_add,
	opc_sub,
	opc_mul,
	opc_div,
	opc_mod,

	//~ opc_c2z = 0x02000 << 0,
	opc_ceq = 0x02001 << 0,
	opc_clt = 0x02002 << 0,
	opc_cle = 0x02003 << 0,
	cmp_i32 = 0x02000 << 2,
	cmp_f32 = 0x02001 << 2,
	cmp_i64 = 0x02002 << 2,
	cmp_f64 = 0x02003 << 2,
	cmp_tun = 0x02001 << 4,		// unsigned / unordered
	opc_not = 0x02001 << 5,		// 
	//~ opc_c2z = opc_not + opc_c2z,
	opc_cne = opc_not + opc_ceq,
	opc_cgt = opc_not + opc_cle,
	opc_cge = opc_not + opc_clt,

	opc_shl,
	opc_shr,
	opc_and,
	opc_ior,
	opc_xor,

	//~ opc_ldcr = opc_ldc4,
	//~ opc_ldcf = opc_ldc4,
	//~ opc_ldcF = opc_ldc8,

	opc_line = 0x1000,		// line info
	//~ opc_file = 0x1001,		// file info
	//~ opc_docc = 0x1001,		// doc comment
};

typedef struct {
	int const	code;
	int const	size;
	char const *name;
} opc_inf;
extern const opc_inf opc_tbl[255];

struct libcargv {
	unsigned char* retv;	// retval
	unsigned char* argv;	// 
};

typedef int8_t   int08t;
typedef int16_t  int16t;
typedef int32_t  int32t;
typedef int64_t  int64t;
typedef uint8_t  uns08t;
typedef uint16_t uns16t;
typedef uint32_t uns32t;
typedef uint64_t uns64t;
typedef float    flt32t;
typedef double   flt64t;
typedef union {
	uns08t _b[16];
	uns16t _w[8];
	uns32t _l[4];
	uns64t _q[2];
	flt32t _s[4];
	flt64t _d[2];
} pd128t;

//~ typedef signed char int08t;
//~ typedef signed short int16t;
//~ typedef signed long int32t;
//~ typedef signed long long int64t;
//~ typedef unsigned char uns08t;
//~ typedef unsigned short uns16t;
//~ typedef unsigned long uns32t;
//~ typedef unsigned long long uns64t;

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
//}

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

typedef struct list_t *list, **strT;

typedef struct symn_t *defn, **defT;

typedef struct astn_t *node;

typedef struct bcdc_t *bcde;

struct list_t {				// list
	struct list_t*	next;		// link
	unsigned char*	data;		// data
};

struct astn_t {				// ast node
	node		next;				// 
	uns08t		kind;				// TYPE_ref, OPER_???, CNST_???
	uns08t		cast;				// lhs & rhs type
	uns16t		xxx2;				// ???
	uns32t		line;				// token on line / code offset
	defn		type;				// typeof() return type of operator ... base type of IDTF
	union {
		int64t		cint;			// cnst : integer
		flt64t		cflt;			// cnst : float
		struct {
			union {
				struct astn_t*	stmt;			// STMT : statement block
				struct astn_t*	rhso;			// OPER : left hand side operand / true block / statement
			};
			union {
				struct astn_t*	step;			// STMT : for statement increment expression / if else block
				struct astn_t*	lhso;			// OPER : right hand side operand / false block / increment
				struct symn_t*	link;			// IDTF : link ? TYPE_ref : TYPE_def
			};
			union {
				struct astn_t*	test;			// OPER : ?: operator condition / if for while condition
				char*		name;			// IDTF
			};
			union {
				struct astn_t*	init;			// STMT : just for statement 'for' & nop
				unsigned	pril;			// OPER : used temporarly by cast();
				unsigned	hash;			// IDTF
			};
		};
		/*union  {					// CNST_xxx : constant
			int64t	cint;			// cnst : integer
			flt64t	cflt;			// cnst : float
			//~ int32t	cpi4[2];		// ???
			//~ int64t	cpi2[2];		// rat
			//~ flt32t	cpf4[4];		// vec
			//~ flt64t	cpf2[2];		// cpl
		} cnst;
		struct {					// TYPE_xxx : typename
			defn	decl;			// link to reference
			node	link;			// func body, 
			char*	name;			// name of identifyer
			uns32t	hash;			// hash code for 'name'
		} idtf;
		struct {					// OPER_xxx : operator
			node	rhso;			// right hand side operand
			node	lhso;			// left hand side operand
			node	test;			// ?: operator condition
			//~ unsigned int	cast;			// cast lhsO & rhsO to type
		} oper;
		struct {					// STMT_xxx : statement
			node	stmt;			// statement / then block
			node	step;			// increment / else block
			node	test;			// condition : if, for
			node	init;			// for statement init
		} stmt;
		struct {					// list of next-linked nodes
			node	next;
			node	tail;
			uns32t	count;
			uns32t	extra;
		} list;// */
		struct {					// CODE_inf : codeinfo
			node	stmt;			// statement
			uns08t*	cbeg;			// code start
			uns08t*	cend;			// code end
			uns32t	spos;			// stack size
		} cinf;
	};
};

struct symn_t {				// symbol
	defn	deft;		// simbols on table
	defn	next;		// link

	uns16t	nest;		// declaration level
	uns08t	kind;		// TYPE_xxx / TYPE_ref

	uns08t	used:1;		// qual: used(ref/def)
	uns08t	onst:1;		// qual: on stack(val) / native(call)???
	uns08t	libc:1;		// native
	//~ uns08t	asgn:1;		// assigned a value
	//~ uns08t	temp:1;		// qual: remove valiable (!static && level > 0)
	//~ uns08t	call:1;		// qual: used(ref/def)

	uns32t	size;		// sizeof(TYPE_ref) / sizeof(TYPE_def)
	uns32t	offs;		// addrof(TYPE_ref) / packof(TYPE_def)

	defn	type;		// base type of TYPE_ref (void, int, float, struct, ...)
	char*	name;		// name of symbol
	defn	args;		// FUNCTION args / STRUCT fields
	node	init;		// TYPE_ref init / TYPE_fnc body
	node	link;		// declaration link
};

extern defn type_vid;
extern defn type_u32;
extern defn type_i32;
extern defn type_i64;
extern defn type_f32;
extern defn type_f64;
extern defn type_str;

typedef struct cell_t {			// processor
	struct cell_t*		next;
	//~ struct cpu*		task;
	//~ unsigned int	ei;			// instruction count / exec
	//~ unsigned int	wi;			// instruction count / wait
	//~ unsigned int	ri;			// instruction count / halt
	//~ unsigned		id;			// id

	// flags[?]
	//~ unsigned	zf:1;			// zero flag
	//~ unsigned	sf:1;			// sign flag
	//~ unsigned	cf:1;			// carry flag
	//~ unsigned	of:1;			// overflow flag

	unsigned char*	ip;			// Instruction pointer
	unsigned char*	sp;			// Stack pointer := bp + ss
	unsigned char	bp[];			// Stack base
} *cell;

typedef struct state_t {		// enviroment
	time_t		time;			// run time
	int		errc;			// error count
	int		warn;			// warning level

	char*		file;			// current file name
	int		line;			// current line number
	FILE*		logf;			// log file (errors + warnings + output)

	//~ char*		host;		// current "dos"/"win"/"linux"?
	//~ char*		cwdp;		// path of directory
	//~ char*		binp;		// path of program
	//~ char*		binn;		// name of program

	strT	strt;		// string table (identifyers || string constants)
	int		copt;		// optimization levevel

	struct {			// Lexer
		node		_tok;		// saved token
		int		_chr;		// saved char
		int		_fin;		// file handle (-1) for cc_buff()
		int		_cnt;		// chars left in buffer
		char*		_ptr;		// pointer parsing trough source
		uns08t		_buf[1024];	// cache
	};// file[TOKS];
	struct {			// Scanner
		//~ list	lstp;		// lits node pool
		node	tokp;		// token pool
		node	root;		// token code

		defT	deft;		// definitions / declarations
		defn	defs;		// definitions / declarations stack
		defn	glob;		// global

		int	nest;		// nest level : modified by (enterblock/leaveblock)
		struct {		// current decl
			defn	csym;
			defn	cdef;
			defn	cref;
			//~ node	stmt;
			//~ node	_brk;	// break
			//~ node	_con;	// continue
			//~ node	_jumplist;
		} scope[TOKS];
	};
	struct {			// bytecode
		unsigned char*	memp;			// memory == buffp
		unsigned long	mems;			// memory size ?

		//~ unsigned int	tick;			// 
		unsigned int	mins;			// stack min size
		unsigned int	rets;			// stack size on return
		unsigned int	pc;			// prev / program counter
		unsigned int	ic;			// instruction count / executed

		unsigned int	cs;			// code size / program counter
		unsigned int	ds;			// data size
		cell		pu;			// procs

		//~ unsigned char*	ip;			// Instruction pointer
		/** memory mapping
		 *	code(RX):cs		:	instructions
		 *	data(RW):ds		:	initialized data
		 *	heap(RW):		:	???
		 *	cell(RO):		:	
		 *
		+mp------------+ip------------+hp------------+---------------ms+
		|   code[cs]   |   data[ds]   |   heap[??]   |   cell[ss*cc]   |
		+--------------+--------------+--------------+-----------------+
		?      --x     |      ro-     |     rw-      |       rw-       |
		+--------------+--------------+--------------+-----------------+
		enum Segment
		{
			Seg_Data = 0,
			Seg_Init = 1,
			Seg_Code = 2,
		};
		**/
	} code;
	/*struct {			// memory manager
		unsigned char*	_ptr;
		unsigned char*	_end;
		//~ unsigned long	_cnt;		// := _end - _ptr;
		//~ unsigned long	_len;		// := _end - _buf;
		unsigned char*	_buf[bufmaxlen+2];
		list	used, free;
	} mmgr;// */
	long	buffs;			// size of buffer
	char*	buffp;			// ???
	char	buffer[bufmaxlen+2];	// buffer base (!!! static)
} *state;

#pragma pack(push, 1)
typedef union {				// stack value type
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
	//~ f32x4t	p4;
	f32x4t	pf;
	f64x2t	pd;
} stkval;
struct bcdc_t {			// byte code decoder
	uns08t opc;
	union {
		stkval arg;		// argument (load const)
		uns08t idx;		// usualy index for stack
		int32t jmp:24;		// jump relative
		struct {		// when starting a task
			uns08t	dl;	// data to copy to stack
			uns16t	cl;	// code to exec paralel
		};
		/*struct {				// extended
			uint8_t opc:4;		// 0-15
			uint8_t opt:2;		// idx(3) / rev(3) / imm(6) / mem(6)
			uint8_t mem:2;		// mem size
			union {
				uint8_t si[4];
				val32t	arg;
			};
		}ext;*/
	};
};
#pragma pack(pop)

void fputmsg(FILE *fout, char *msg, ...);
void fputsym(FILE *fout, defn sym, int mode);
void fputast(FILE *fout, node ast, int mode, int pri);
void fputasm(FILE *fout, bcde ip, int len, int offs);

void perr(state s, int level, char *file, int line, char *msg, ...);

void dumpdag(FILE *fout, node ptr, int lev);

defn newdefn(state s, int kind);
//~ void eatnode(state s, node ref);
//~ node cpynode(state s, node ast);
//~ node cpytree(state s, node ast);

char* lookupstr(state s, char *name, unsigned size, unsigned hash);

defn install(state s, char* name, unsigned size, int kind);
defn declare(state s, int kind, node tag, defn rtyp, defn args);
defn lookup(state s, node ast, node arg);

void enter(state s, defn def);
void leave(state s, int mode, defn *d);

node newnode(state s, int kind);
void delnode(state s, node ast);

node fltnode(state s, flt64t v);
node intnode(state s, int64t v);
node fh8node(state s, uns64t v);
node strnode(state s, char *v);

int32t constbol(node ast);
int64t constint(node ast);
flt64t constflt(node ast);

int istype(node ast);
int typeId(node ast);
int castid(node lhs, node rhs);

node peek(state);
node next(state, int kind);
int  skip(state, int kind);
int  test(state, int kind);

node expr(state, int mode);		// parse expression	(mode : ?)
//+ node decl(state, int mode);		// parse declaration	(mode : enable expr)
//~ node stmt(state, int mode);		// parse statement	(mode : enable decl)
node scan(state, int mode);		// parse a file 	(mode : script mode)

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
//~ int emit(state s, int opc, stkval arg);

int cc_init(state);
int cc_done(state);
int cc_srcf(state, char *file);
int cc_logf(state, char *file);

int vm_init(state);
int vm_done(state);
//~ int vm_open(state, char *file);
//~ int vm_dump(state, char *file);
int vm_exec(state, unsigned cc, unsigned ss, int dbg);

//~ int cc_file(state, int mode, char *file);
//~ int cc_buff(state, int mode, char *buff);

//~ int tags(state, char *file);
//~ int dasm(state, char *file);

int rehash(register char* str, unsigned size);
int align(int offs, int pack, int size);
#endif

#if 0
struct JSCodeGenerator {
    JSTreeContext   treeContext;    /* base state: statement info stack, etc. */

    JSArenaPool     *codePool;      /* pointer to thread code arena pool */
    JSArenaPool     *notePool;      /* pointer to thread srcnote arena pool */
    void            *codeMark;      /* low watermark in cg->codePool */
    void            *noteMark;      /* low watermark in cg->notePool */
    void            *tempMark;      /* low watermark in cx->tempPool */

    struct {
        jsbytecode  *base;          /* base of JS bytecode vector */
        jsbytecode  *limit;         /* one byte beyond end of bytecode */
        jsbytecode  *next;          /* pointer to next free bytecode */
        jssrcnote   *notes;         /* source notes, see below */
        uintN       noteCount;      /* number of source notes so far */
        uintN       noteMask;       /* growth increment for notes */
        ptrdiff_t   lastNoteOffset; /* code offset for last source note */
        uintN       currentLine;    /* line number for tree-based srcnote gen */
    } prolog, main, *current;

    const char      *filename;      /* null or weak link to source filename */
    uintN           firstLine;      /* first line, for js_NewScriptFromCG */
    JSPrincipals    *principals;    /* principals for constant folding eval */
    JSAtomList      atomList;       /* literals indexed for mapping */

    intN            stackDepth;     /* current stack depth in script frame */
    uintN           maxStackDepth;  /* maximum stack depth so far */

    JSTryNote       *tryBase;       /* first exception handling note */
    JSTryNote       *tryNext;       /* next available note */
    size_t          tryNoteSpace;   /* # of bytes allocated at tryBase */

    JSSpanDep       *spanDeps;      /* span dependent instruction records */
    JSJumpTarget    *jumpTargets;   /* AVL tree of jump target offsets */
    JSJumpTarget    *jtFreeList;    /* JT_LEFT-linked list of free structs */
    uintN           numSpanDeps;    /* number of span dependencies */
    uintN           numJumpTargets; /* number of jump targets */
    ptrdiff_t       spanDepTodo;    /* offset from main.base of potentially
                                       unoptimized spandeps */

    uintN           arrayCompSlot;  /* stack slot of array in comprehension */

    uintN           emitLevel;      /* js_EmitTree recursion level */
    JSAtomList      constList;      /* compile time constants */
    JSCodeGenerator *parent;        /* Enclosing function or global context */
};

#endif
