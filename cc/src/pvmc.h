#ifndef __CCVM_API
#define __CCVM_API

#include <stdio.h>
#include <stdint.h>
#include <string.h>
typedef float float32_t;
typedef double float64_t;

//~ typedef struct astn *astn;		// tree
typedef struct symn *symn;		// symbols
typedef struct ccState *ccState;

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
	creg_def = creg_eopc,
};

typedef struct state {
	FILE* logf;		// log file
	int   closelog;	// close log file

	symn  defs;		// definitions
	symn  gdef;		// definitions

	int   errc;		// error count

	symn  libc;		// library call symbol
	int   func;		// library call function
	void* retv;		// return value
	char* argv;		// first argument
	void* data;		// user data for execution

	ccState cc;		// compiler enviroment
	struct {
		int		opti;			// optimization levevel

		unsigned int	pc;			// entry point / prev program counter
		unsigned int	px;			// exit point / program counter

		unsigned int	ss;			// stack size / current stack size
		unsigned int	sm;			// stack minimum size

		unsigned int	ro;			// <= ro : read only region(meta data)
		unsigned int	pos;		// current positin in buffer
	}vm;
	struct {
		int argc;
		char* argv[];
	}pe;

	long _size;		// size of total memory
	void *_free;	// list of free memory
	void *_used;	// list of used memory

	unsigned char *_ptr;	// cc: used memory; vm: max stack when calling vmCall from a libcall
	unsigned char _mem[];
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
	dump_sym = 0x0000100,
	dump_asm = 0x0000200,
	dump_ast = 0x0000400,
	dumpMask = 0x0000f00,
} srcType, dumpMode;

// run-time
state rtInit(void* mem, unsigned size);

/** return the internal offset of a reference
 *  aborts if ptr not inside the context.
 */
int vmOffset(state s, void *ptr);

int logfile(state, char *file);				// logger
int logFILE(state, FILE *file);				// logger

// compile
//~ int srcfile(state, char *file);				// source
//~ int srctext(state, char *file, int line, char *src);				// source
//~ int compile(state, int level);				// warning level
int gencode(state, int level);				// optimize level
//~ int execute(state, int cc, int ss, dbgf dbg);
int parse(ccState, srcType, int wl);

symn libcall(state, int libc(state), int pass, const char* proto);
//~ symn install(state, int libc(state), int pass, const char* proto);
symn installtyp(state s, const char* name, unsigned size);
//~ symn installvar(state s, const char* name, symn type, unsigned offset);

// execute
int libCallExitDebug(state s);
int libCallExitQuiet(state s);
int (*libcSwapExit(state s, int libc(state)))(state);
typedef int (*dbgf)(state s, int pu, void *ip, long* sptr, int scnt);
//~ int dbgCon(state, int, void*, long*, int);				// 

int vmExec(state, dbgf dbg);
int vmCall(state, symn fun, ...);

// output
void fputfmt(FILE *fout, const char *msg, ...);

void dump(state, dumpMode, symn, char* text, ...);

// Level 1 Functions: use less these
ccState ccInit(state, int mode);
ccState ccOpen(state, srcType, char* source);

int ccDone(state);

//~ state vmInit(state);
//~ int vmOpen(state, char* source);
//~ int vmDone(state s);		// what should do this

void ccSource(ccState, char *file, int line);

symn ccBegin(state rt, char *cls);
void ccEnd(state rt, symn cls);

symn findref(state s, void *ptr);
symn findsym(ccState s, symn in, char *name);

int findnzv(ccState s, char *name);
int findint(ccState s, char *name, int* res);
int findflt(ccState s, char *name, double* res);

#define poparg(__ARGV, __SIZE) ((void*)(((__ARGV)->argv += (((__SIZE) | 3) & ~3)) - (__SIZE)))
#define popaty(__ARGV, __TYPE) (*(__TYPE*)(poparg((__ARGV), sizeof(__TYPE))))

#define retptr(__ARGV, __TYPE) ((__TYPE*)(__ARGV->retv))
#define getret(__ARGV, __TYPE) (*retptr(__ARGV, __TYPE))
#define setret(__ARGV, __TYPE, __VAL) (*retptr(__ARGV, __TYPE) = (__TYPE)(__VAL))

static inline int32_t popi32(state s) { return *(int32_t*)poparg(s, sizeof(int32_t)); }
static inline int64_t popi64(state s) { return *(int64_t*)poparg(s, sizeof(int64_t)); }
static inline float32_t popf32(state s) { return *(float32_t*)poparg(s, sizeof(float32_t)); }
static inline float64_t popf64(state s) { return *(float64_t*)poparg(s, sizeof(float64_t)); }
static inline void* popref(state s) { int32_t p = popi32(s); return p ? s->_mem + p : NULL; }
static inline char* popstr(state s) { return popref(s); }

static inline void* popval(state s, void* dst, int size) { return memcpy(dst, poparg(s, size), size); }
static inline void* popvar(state s, void* dst, int size) { return memcpy(dst, popref(s), size); }

//~ static inline void reti32(state s, int32_t val) { setret(int32_t, s, val); }
//~ static inline void reti64(state s, int64_t val) { setret(int64_t, s, val); }
//~ static inline void retf32(state s, float32_t val) { setret(float32_t, s, val); }
//~ static inline void retf64(state s, float64_t val) { setret(float64_t, s, val); }
#endif

#define DEBUGGING 1
