#ifndef __CCVM_API
#define __CCVM_API

#include <stdio.h>
#include <stdint.h>
#include <string.h>
typedef float float32_t;
typedef double float64_t;

typedef struct astn *astn;		// tree
typedef struct symn *symn;		// symbols
typedef struct ccState *ccState;
typedef struct vmState *vmState;

enum CompilerRegister {
	creg_base = 0x0000,				// type system only
	creg_emit = 0x0008,				// the emit thingie : emit(...)
	//~ creg_etyp = 0x0001 | creg_emit,	// emit types : emit.i32
	creg_eopc = 0x0002 | creg_emit,	// emit opcodes : emit.i32.add
	creg_swiz = 0x0004 | creg_eopc,	// swizzle constants : emit.swz.(xxxx ... xyzw ... wwww)

	// this are in main
	//~ creg_stdc = 0x0010 | creg_emit,	// std library calls : sin(float64 x) = emit(float64, libc(3), f64(x));
	//~ creg_bits = 0x0020 | creg_emit,	// bitwize operations: bits.shr(int64 x, int32 cnt)

	//~ creg_math = 0x000?,
	//~ creg_rtty = 0x000?,
	creg_all  = -1,
	creg_def = creg_all,
};

typedef struct state {
	FILE* logf;		// log file
	int   closelog;	// close log file

	symn  defs;		// definitions

	int   errc;		// error count

	int   func;		// library call function
	void* data;		// user data for execution
	symn  libc;		// library call symbol
	void* retv;		// TODO: RemMe: retval
	char* argv;		// first arg
	//~ symn  args;		// argument symbols
	//~ int   argc;		// number of args in bytes

	ccState cc;		// compiler enviroment	// TODO: RemMe cc has the state
	vmState vm;		// execution enviroment	// TODO: RemMe vm has the state

	long _cnt;
	char *_ptr;		// write here symbols and string constants
	char _mem[];
} *state;
typedef enum {
	srcText = 0x00,		// file / buffer
	srcFile = 0x10,		// file / buffer

	//~ srcUnit = 0x01,
	//~ srcDecl = 0x02,
	//~ srcExpr = 0x03,

	// unit / script (ask the file what is it ? (: first tokens : 'package' 'name'))

	//~ copt_eval = 1;
	//~ copt_memstack = 2;
	//~ copt_noGlobaStatic = 0x100,

	// dump()
	//~ dump_bin = 0x0000100,
	dump_txt = 0x0000800,
	dump_sym = 0x0000200,
	dump_asm = 0x0000300,
	dump_ast = 0x0000400,
	dumpMask = 0x0000f00,
} srcType, dumpMode;

// run-time
state rtInit(void* mem, unsigned size);

int logfile(state, char *file);				// logger
int logFILE(state, FILE *file);				// logger

// compile
//~ int srcfile(state, char *file);				// source
//~ int srctext(state, char *file, int line, char *src);				// source
//~ int compile(state, int level);				// warning level
int gencode(state, int level);				// optimize level
//~ int execute(state, int cc, int ss, dbgf dbg);
int parse(ccState, srcType);

symn libcall(state, int libc(state), int pass, const char* proto);
//~ symn install(state, int libc(state), int pass, const char* proto);
symn installtyp(state s, const char* name, unsigned size);
//~ symn installvar(state s, const char* name, symn type, unsigned offset);

// execute
int (*libcSwapExit(state s, int libc(state)))(state);
typedef int (*dbgf)(state s, int pu, void *ip, long* sptr, int scnt);
//~ int dbgCon(state, int, void*, long*, int);				// 

//~ int exec(vmState, unsigned cc, dbgf dbg);
int exec(vmState, dbgf dbg);

// output
void dump(state, dumpMode, char* text, ...);

// Level 1 Functions: use less these
ccState ccInit(state, int mode);
ccState ccOpen(state, srcType, char* source);

int ccDone(state);

vmState vmInit(state);
int vmCall(state, symn fun, ...);
//~ int vmOpen(state, char* source);
void vmInfo(FILE*, vmState s);
//~ int vmDone(state s);		// what should do this

void ccSource(ccState, char *file, int line);

symn findref(state s, void *ptr);
symn findsym(ccState s, symn in, char *name);

int findnzv(ccState s, char *name);
int findint(ccState s, char *name, int* res);
int findflt(ccState s, char *name, double* res);

#define popargsize(__ARGV, __SIZE) ((void*)(((__ARGV)->argv += (((__SIZE) | 3) & ~3)) - (__SIZE)))
//~ #define popargtype(__ARGV, __TYPE) ((__TYPE*)((__ARGV)->argv += ((sizeof(__TYPE) | 3) & ~3)))[-1]
#define popargtype(__ARGV, __TYPE) ((__TYPE*)(popargsize((__ARGV), sizeof(__TYPE))))[0]

#define retptr(__TYPE, __ARGV) ((__TYPE*)(__ARGV->retv))
//~ #define setret(__TYPE, __ARGV, __VAL) (retptr(__TYPE, __ARGV)[-1] = (__TYPE)(__VAL))
#define setret(__TYPE, __ARGV, __VAL) (*retptr(__TYPE, __ARGV) = (__TYPE)(__VAL))

#define poparg(__ARGV, __TYPE) popargtype(__ARGV, __TYPE)

static inline float32_t popf32(state s) { return popargtype(s, float32_t); }
static inline int32_t popi32(state s) { return popargtype(s, int32_t); }
static inline float64_t popf64(state s) { return popargtype(s, float64_t); }
static inline int64_t popi64(state s) { return popargtype(s, int64_t); }

static inline void* popref(state s) { int32_t p = popi32(s); return p ? s->_mem + p : NULL; }
static inline char* popstr(state s) { return s->_mem + popi32(s); }

static inline void* popval(state s, void* dst, int size) { return memcpy(dst, popargsize(s, size), size); }
//~ static inline void* poprecord(state s, int size) { return popargsize(s, size); }

static inline void reti32(state s, int32_t val) { setret(int32_t, s, val); }
static inline void reti64(state s, int64_t val) { setret(int64_t, s, val); }
static inline void retf32(state s, float32_t val) { setret(float32_t, s, val); }
static inline void retf64(state s, float64_t val) { setret(float64_t, s, val); }
#endif

/* example ccTags
 *int ccTags(state s, char *srcfile) {
 *	if (logfile(s, "tags.out") != 0) {	// log in file instead of stderr
 *		return ccDone(s);
 *	}
 *	if (!ccOpen(s, srcFile, srcfile)) {	// will cal ccInit
 *		logfile(s, NULL);
 *		return ccDone(s);
 *	}
 *	if (compile(s, warnLevel) != 0) {
 *		logfile(s, NULL);
 *		return s->errc;
 *	}
 *	logdump(s, dump_sym, NULL);
 *	logfile(s, NULL);
 *	return 0;
 *}
**/
