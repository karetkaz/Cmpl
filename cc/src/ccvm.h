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

#define DEBUGGING 2

// maximum tokens in expressions & nest level
#define TOKS 2048

// symbol & hash table size
#define TBLS 2048

#define pdbg(__DBG, msg, ...) do{fputfmt(stderr, "%s:%d:"__DBG":%s: "msg"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); fflush(stdout); fflush(stderr);}while(0)

#define debug(msg, ...) pdbg("debug", msg, ##__VA_ARGS__)
#define fatal(msg, ...) {pdbg("fatal", msg, ##__VA_ARGS__); abort(); }
#define dieif(__EXP, msg, ...) {if (__EXP) fatal(msg, ##__VA_ARGS__)}

#if 1
#define PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ...) do { perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__); perr(__ENV, __LEVEL, __FILE__, __LINE__, msg, ##__VA_ARGS__); }while(0)
#define error(__ENV, __LINE, msg, ...) PERR(__ENV, -1, NULL, __LINE, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) PERR(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) PERR(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)
#else // catch the position error raised 
#define error(__ENV, __LINE, msg, ...) perr(__ENV, -1, NULL, __LINE, msg, ##__VA_ARGS__)
#define warn(__ENV, __LEVEL, __FILE, __LINE, msg, ...) perr(__ENV, __LEVEL, __FILE, __LINE, msg, ##__VA_ARGS__)
#define info(__ENV, __FILE, __LINE, msg, ...) perr(__ENV, 0, __FILE, __LINE, msg, ##__VA_ARGS__)
#endif

// Symbols - CC
enum {
	#define TOKDEF(NAME, TYPE, SIZE, STR) NAME,
	#include "incl/defs.h"
	tok_last,
	// vm part
	// xml dump needs these
	//~ TYPE_u32,
	//~ TYPE_i32,
	//~ TYPE_f32,
	//~ TYPE_i64,
	//~ TYPE_f64,
	//~ TYPE_p4x,

	TOKN_err = TYPE_any,
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
	#include "code.h"
	opc_last,

	opc_neg,
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

	//~ opc_ldc,
	opc_ldi,
	opc_sti,
	opc_inc,	// increment

	get_ip,		// instruction pointer
	get_sp,		// get stack position
	set_sp,		// set stack pointer
	// TODO: remove
	seg_code,	// pc = ptr - beg; ptr += code->cs;
	loc_data,

	opc_line,		// line info
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

typedef union {		// stack value type
	int8_t		i1;
	uint8_t		u1;
	int16_t		i2;
	uint16_t	u2;
	int32_t		i4;
	uint32_t	u4;
	int64_t		i8;
	//uint64_t	u8;
	flt32_t		f4;
	flt64_t		f8;
	//~ flt32_t		pf[4];
	//~ flt32_t		pd[2];
	//~ struct {flt32_t x, y, z, w;} pf;
	//~ struct {flt64_t x, y;} pd;
} stkval;

//~ typedef struct listn_t *list, **strt;
typedef struct symn **symt;
typedef struct symn *symn;		// symbols
typedef struct astn *astn;

typedef struct list {			// linked list
	struct list		*next;
	unsigned char	*data;
	//~ unsigned int	size;	// := ((char*)&node) - data;
} *list, **strt;

struct astn {				// tree
	uint32_t	line;				// token on line / code offset
	uint8_t		kind;				// code: TYPE_ref, OPER_???, CNST_???
	uint8_t		cst2;				// casts to
	uint16_t	XXXX;				// OPER: (priority level / dupplicate of, on stack)
	union {
		union  {					// CNST_xxx: constant
			int64_t	cint;			// cnst: integer
			flt64_t	cflt;			// cnst: float
			char*	cstr;			// cnst: string
			//~ int32_t	cpi4[4];		// rgb
			//~ int64_t	cpi2[2];		// rat
			//~ flt32_t	cpf4[4];		// vec
			//~ flt64_t	cpf2[2];		// cpl
		} con;
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
			astn	nuse;			// next used
			char*	name;			// name of identifyer
			uint32_t	hash;			// hash code for 'name'
			symn	link;			// func body / replacement
		} id;
	};
	symn		type;				// typeof() return type of operator ... base type of IDTF
	astn		next;				// next statement, do not use for preorder
};
struct symn {				// type
	char*	file;
	int		line;

	char*	name;
	union {
	int32_t	size;	// sizeof(TYPE_xxx)
	int32_t	offs;	// addrof(TYPE_ref)
	};
	symn	decl;		// declared in
	symn	type;		// base type of TYPE_ref (void, int, float, struct, ...)
	symn	args;		// REC fields / FUN args
	symn	next;		// symbols on table/args

	uint8_t	cast;		// casts to type(TYPE_(u32, i32, i64, f32, f64, p4x)).
	uint8_t	algn;		// alignment
	uint8_t	kind;		// TYPE_ref || TYPE_xxx

	uint8_t	call:1;		// function / callable
	//~ uint8_t	iref:1;		// indirect reference: "&"

	//~ uint8_t	priv:1;		// private
	//~ uint8_t	stat:1;		// static ?
	uint8_t	read:1;		// const / no override

	//~ uint8_t	onst:1;		// stack  (val): replaced with (offs < 0) ?(offs < 255)
	//~ uint8_t	libc:1;		// native (fun): replaced with (offs < 0) ?(offs < 255)

	uint32_t	xx_1:6;		// -Wpadded
	uint16_t	xx_2;		// align
	uint16_t	nest;		// declaration level
	astn	init;		// VAR init / FUN body

	// list(scoping)
	astn	used;
	symn	defs;		// symbols on stack/all
	char*	pfmt;		// print format
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

	char*	file;	// current file name
	int		line;	// current line number

	// Warning set to -1 to record.
	char	*doc;

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
	long	_cnt;
	char	*_ptr;
};
struct vmEnv {
	state	s;
	int		opti;			// optimization levevel

	unsigned int	pc;			// entry point / prev program counter
	unsigned int	ic;			// ?executed? / instruction count
	unsigned int	cs;			// code size
	//~ unsigned int	pi[2];		// previous instructions

	unsigned int	ss;			// stack size / current stack size
	unsigned int	sm;			// stack minimum size

	unsigned int	ds;			// data size

	//~ unsigned long _cnt; // return (_end - _ptr);
	//~ unsigned long _len; // return (_ptr - _mem);
	unsigned char *_ptr;
	unsigned char *_end;
	unsigned char _mem[];
};

//~ static inline int canscan(ccEnv env) {return env && env->_cnt > 0;}
//~ static inline int kindOf(astn ast) {return ast ? ast->kind : 0;}

extern symn type_vid;
extern symn type_bol;
extern symn type_u32;
extern symn type_i32;
extern symn type_i64;
extern symn type_f32;
extern symn type_f64;
extern symn type_str;

extern symn type_v4f;
extern symn type_v2d;

extern symn emit_opc;

//~ util
//~ int parseInt(const char *str, int *res, int hexchr);
//~ char* parsecmd(char *ptr, char *cmd, char *sws);

//~ clog
void fputfmt(FILE *fout, const char *msg, ...);

//~ void fputsym(FILE *fout, symn sym, int mode, int level);
//~ void fputast(FILE *fout, astn ast, int mode, int level);
//~ void fputopc(FILE *fout, bcde opc, int len, int offset);
void fputasm(FILE *fout, unsigned char *beg, int len, int rel, int mode);
//~ void fputasm(FILE *fout, void *ip, int len, int length);

// program error
void perr(state s, int level, const char *file, int line, const char *msg, ...);

//~ void dumpasm(FILE *fout, vmEnv s, int offs);
void dumpsym(FILE *fout, symn sym, int mode);
void dumpast(FILE *fout, astn ast, int mode);
void dumpxml(FILE *fout, astn ast, int lev, const char* text, int level);

void dump2file(state s, char *file, dumpMode mode, char *text);

// ccvm
void* getmem(state s, int size, unsigned clear);

symn newdefn(ccEnv s, int kind);

astn newnode(ccEnv s, int kind);
astn intnode(ccEnv s, int64_t v);
//~ astn fltnode(ccEnv s, flt64_t v);
astn strnode(ccEnv s, char *v);
astn fh8node(ccEnv s, uint64_t v);
astn cpynode(ccEnv s, astn src);
void eatnode(ccEnv s, astn ast);

symn installex(ccEnv s, const char* name, int kind, unsigned size, symn type, astn init);
symn install(ccEnv s, const char* name, int kind, int cast, unsigned size);
symn declare(ccEnv s, int kind, astn tag, symn rtyp);
symn lookup(ccEnv s, symn sym, astn ast, astn args);

symn findsym(ccEnv s, symn in, char *name);
int findnzv(ccEnv s, char *name);
int findint(ccEnv s, char *name, int* res);
int findflt(ccEnv s, char *name, double* res);

symn typecheck(ccEnv s, symn loc, astn ast);
int castId(symn typ);
int castTo(astn ast, int tyId);

int32_t constbol(astn ast);
int64_t constint(astn ast);
flt64_t constflt(astn ast);

astn peek(ccEnv);
//~ astn next(ccEnv, int kind);
//~ void back(ccEnv, astn ast);
//~ int  skip(ccEnv, int kind);
//~ int  test(ccEnv, int kind);

astn expr(ccEnv, int mode);		// parse expression	(mode: lookup)
//~ astn decl(ccEnv, int mode);		// parse declaration	(mode: enable expr)
//~ astn stmt(ccEnv, int mode);		// parse statement	(mode: enable decl)
int scan(ccEnv, int mode);		// parse program	(mode: script mode)

// scoping ...
void enter(ccEnv s, symn def);
symn leave(ccEnv s, symn def);

/** try to evaluate an expression
 * @param res: where to put the result
 * @param ast: tree to be evaluated
 * @return one of [TYPE_err, TYPE_bit, TYPE_int, TYPE_flt]
 */
int eval(astn res, astn ast);

/** generate byte code for tree
 * @param ast: tree to be generated
 * @param get: one of TYPE_xxx
 * @return TYPE_xxx, 0 on error
 */
int cgen(state s, astn ast, int get);

/** emit an opcode with args
 * @param opc: opcode
 * @param ...: argument
 * @return: program counter
 */
int emit(vmEnv, int opc, ...);
int emiti32(vmEnv, int32_t arg);
int emiti64(vmEnv, int64_t arg);
int emitf32(vmEnv, flt32_t arg);
int emitf64(vmEnv, flt64_t arg);
int emitref(vmEnv, int opc, int offset);
int emitptr(vmEnv, int opc, void* arg);
//int emitref(vmEnv, int offset);

int emitidx(vmEnv, int opc, int pos);
int emitint(vmEnv, int opc, int64_t arg);
int fixjump(vmEnv s, int src, int dst, int stc);
//int testopc(vmEnv s, int opc);

int isType(symn sym);
int istype(astn ast);

//~ int isemit(astn ast);
//~ int islval(astn ast);
symn linkOf(astn ast, int notjustrefs);
long sizeOf(symn typ);

int castId(symn ast);
int castOp(symn lhs, symn rhs, int uns);

int source(ccEnv, srcType mode, char* text);		// mode: file/text

int rehash(const char* str, unsigned size);
//~ int align(int offs, int pack, int size);
char *mapstr(ccEnv s, char *name, unsigned size/* = -1U*/, unsigned hash/* = -1U*/);

/**
 * param flags:
 * bit 1: skip errors;
 * TODO: bit 2: print hex values;
**/
void vmTags(ccEnv s, char *sptr, int slen, int flags);

void dumpasmdbg(FILE *fout, vmEnv s, int mark, int len, int mode);

#endif
