#ifndef __CCVM_API
#define __CCVM_API

#include <stdio.h>
#include <stdint.h>

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

typedef struct astn *astn;
typedef struct symn *symn;		// symbols
typedef struct ccEnv *ccEnv;
typedef struct vmEnv *vmEnv;

typedef struct state {
	FILE*	logf;			// log file (errors + warnings)
	int		errc;			// error count
	//~ int		copt;		// optimization levevel

	ccEnv cc;		// compiler enviroment
	vmEnv vm;		// execution enviroment

	// lib call data
	void* data;		// user data
	void* retv;		// retval
	char* argv;		// first arg

	long _cnt;
	char *_ptr;
	char _mem[4 << 20];
} *state;

typedef enum {
	srcFile = 0x10,		// file / buffer
	srcUnit = 0x01,		// unit / script
	srcScript =  0,		// !srcUnit
	//~ srcDefs, srcMake;

	//~ skipCgen = 0x02,		// gen Code

	// dump()
	//~ dump_bin = 0x0000100,
	//~ outTags = 0x0000200,
	//~ outDasm = 0x0000300,
	//~ outCode = 0x0000300,
	//~ outMask = 0x0000f00,
	dump_sym = 0x0000200,
	dump_asm = 0x0000300,
	dump_ast = 0x0000400,
	dumpMask = 0x0000f00,
	dump_new = 0x0001000,		// do not append (!stdout)

} srcType, dumpMode;

// compile
int logfile(state, char *file);				// logger
int srcfile(state, char *file);				// source
int srctext(state, char *file, int line, char *src);				// source
int compile(state, srcType mode);			// script, unit; //, decl, make
int gencode(state, int level);

// execute
typedef int (*dbgf)(vmEnv vm, int pu, void *ip, long* sptr, int sc);
int dbgInfo(vmEnv, int, void*, long*, int);				// if compiled file will print results azt the end
int dbgCon(vmEnv, int, void*, long*, int);				// 
int nodbg(vmEnv, int, void*, long*, int);				// or pass NULL to execute
int exec(vmEnv, unsigned cc, unsigned ss, dbgf dbg);

// output
void dump(state, char*, dumpMode, char* text);

// use less these
ccEnv ccInit(state s);
ccEnv ccOpen(state, srcType, char* source);
//~ int ccScan(ccEnv, srcType);		// scan the source file into ast
int ccDone(state s);

//~ gencode()

vmEnv vmInit(state s);
//~ int vmOpen(state, srcType, char* source);
void vmInfo(FILE*, vmEnv s);
int vmDone(state s);

int installlibc(state, void call(state), const char* proto);
void installvar(state, void* ref, const char* proto);

#define poparg(__ARGV, __TYPE) ((__TYPE*)((__ARGV)->argv += ((sizeof(__TYPE) | 3) & ~3)))[-1]
#define setret(__TYPE, __ARGV, __VAL) (*((__TYPE*)(__ARGV->retv)) = (__TYPE)(__VAL))

static inline void reti32(state s, int32t val) { setret(int32t, s, val); }
static inline void reti64(state s, int64t val) { setret(int64t, s, val); }
static inline void retf32(state s, flt32t val) { setret(flt32t, s, val); }
static inline void retf64(state s, flt64t val) { setret(flt64t, s, val); }
static inline flt32t popf32(state s) { return poparg(s, flt32t); }
static inline int32t popi32(state s) { return poparg(s, int32t); }
static inline flt64t popf64(state s) { return poparg(s, flt64t); }
static inline int64t popi64(state s) { return poparg(s, int64t); }

#endif

/* example ccTags
 * int ccTags(state s, char *srcfile) {
 * 	if (!ccOpen(s, srcFile, srcfile))	// will cal ccInit
 * 		return ccDone(s);
 * 	if (compile(s, srcScript) != 0)
 * 		return ccDone(s);
 * 	ccDone(s);		// here returns 0
 * 	dump(s, "tags.txt", dump_new | dump_sym, NULL);
 * 	return 0;
 * }
 */
