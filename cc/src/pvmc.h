#ifndef __CCVM_API
#define __CCVM_API

#include <stdio.h>
#include <stdint.h>
typedef float flt32_t;
typedef double flt64_t;

#define useBuiltInFuncs

//~ typedef struct astn *astn;		// tree
//~ typedef struct symn *symn;		// symbols
typedef struct ccEnv *ccEnv;
typedef struct vmEnv *vmEnv;

typedef struct state {
	FILE* logf;			// log file
	int   errc;			// error count

	void* data;		// user data for execution
	char* argv;		// first arg
	void* retv;		// TODO: RemMe: retval

	ccEnv cc;		// compiler enviroment
	vmEnv vm;		// execution enviroment

	long _cnt;
	char *_ptr;		// write here symbols and string constants
	char *_end;		// space for ast and list nodes, temporary
	char _mem[];
} *state;
typedef enum {
	srcFile = 0x10,		// file / buffer

	//~ srcAuto = -1,		// first token ?
	//~ srcUnit = 0x01,		// unit / script
	// unit / script (ask the file what is it ? (: first tokens : 'package' 'name'))

	//~ srcDefs, srcMake;	???

	// dump()
	//~ dump_bin = 0x0000100,
	dump_sym = 0x0000200,
	dump_asm = 0x0000300,
	dump_ast = 0x0000400,
	dumpMask = 0x0000f00,

	//~ cc_Emit = 0x000001,

} srcType, dumpMode;

state gsInit(void* mem, unsigned size);

// compile
int logfile(state, char *file);				// logger
int srcfile(state, char *file);				// source
//~ int srctext(state, char *file, int line, char *src);				// source
int compile(state, int level);				// warning level
int gencode(state, int level);				// optimize level
//~ int execute(state, int cc, int ss, dbgf dbg);

int libcall(state, int call(state), const char* proto);

// execute
typedef int (*dbgf)(state s, int pu, void *ip, long* sptr, int scnt);
int dbgInfo(state, int, void*, long*, int);				// if compiled file will print results at the end
int dbgCon(state, int, void*, long*, int);				// 
int exec(vmEnv, unsigned ss, dbgf dbg);

// output
void dump(state, dumpMode, char* text);

// Level 1 Functions: use less these
ccEnv ccInit(state);
ccEnv ccOpen(state, srcType, char* source);
int ccDone(state);

vmEnv vmInit(state);
//~ int vmOpen(state, char* source);
void vmInfo(FILE*, vmEnv s);
//~ int vmDone(state s);		// what should do this

#define retptr(__TYPE, __ARGV) ((__TYPE*)(__ARGV->retv))
#define setret(__TYPE, __ARGV, __VAL) (*retptr(__TYPE, __ARGV) = (__TYPE)(__VAL))
#define poparg(__ARGV, __TYPE) ((__TYPE*)((__ARGV)->argv += ((sizeof(__TYPE) | 3) & ~3)))[-1]

static inline void reti32(state s, int32_t val) { setret(int32_t, s, val); }
static inline void reti64(state s, int64_t val) { setret(int64_t, s, val); }
static inline void retf32(state s, flt32_t val) { setret(flt32_t, s, val); }
static inline void retf64(state s, flt64_t val) { setret(flt64_t, s, val); }
static inline flt32_t popf32(state s) { return poparg(s, flt32_t); }
static inline int32_t popi32(state s) { return poparg(s, int32_t); }
static inline flt64_t popf64(state s) { return poparg(s, flt64_t); }
static inline int64_t popi64(state s) { return poparg(s, int64_t); }

#endif

/* example ccTags
 *int ccTags(state s, char *srcfile) {
 *	if (!ccOpen(s, srcFile, srcfile)) {	// will cal ccInit
 *		return ccDone(s);
 *	}
 *	if (logfile(s, "tags.out") != 0) {	// log in file instead of stderr
 *		return ccDone(s);
 *	}
 *	if (compile(s, warnLevel) != 0) {
 *		logfile(s, NULL);
 *		return ccDone(s);
 *	}
 *	ccDone(s);
 *	logdump(s, dump_sym, NULL);
 *	logfile(s, NULL);
 *	return 0;
 *}
**/

/* emit
 * seg.text: RO initialized data: enums, definitions, as text on new lines.
 * seg.data: RO initialized data: initializers, meta data?
 * seg.code: RO executable code: functions, main
 * 
**/
