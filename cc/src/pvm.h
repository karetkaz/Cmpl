#ifndef __CCVM_API
#define __CCVM_API

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

typedef struct state_t *state;
//~ typedef struct ccEnv *ccEnv;
typedef struct vmEnv *vmEnv;
typedef struct astn *astn;
typedef struct symn *symn;		// symbols

typedef enum {
	source_none = 0,	// close or something like that
	source_file = 1,
	source_buff = 2,
} LoadType;

//~ int logger(state, char *file);					// file for errors

state ccInit(int size);
int ccDone(state env);

int source(state, LoadType mode, char* text);		// mode: file/text

int srctext(state, char *file, int line, char *text);	// source(s, source_text, text); setfpos(s, file, line);
int srcfile(state, char *file);							// source(s, source_file, text);
int logfile(state, char *file);							// file for errors

int compile(state, int mode);				// script, source, decl, makefile
int gencode(state/* , int mode */);				// debug level / optimizations
int execute(state, int cc, int ss);

// compile
astn scan(state, int mode);				// source / script
astn decl(state, int mode);				// enable statements
astn stmt(state, int mode);				// enable decls
astn expr(state, int mode);				// enable comma
//~ symn spec(state, int qual);
//~ symn type(state, int qual);
//~ eval, cgen

// gencode
vmEnv vmInit(void *dst, int size);
int vmLoad(vmEnv, LoadType, char* source);

typedef struct libcarg *libcarg;
typedef void (*libcall)(libcarg);
void installlibc(state, void call(libcarg), const char* proto);

void* usrval(libcarg);
flt32t popf32(libcarg);
int32t popi32(libcarg);
flt64t popf64(libcarg);
int64t popi64(libcarg);
void reti32(libcarg, int32t val);
void reti64(libcarg, int64t val);
void retf32(libcarg, flt32t val);
void retf64(libcarg, flt64t val);

int emitidx(vmEnv, int opc, int stpos);
int emitint(vmEnv, int opc, int64t arg);
int emiti32(vmEnv, int32t arg);
int emiti64(vmEnv, int64t arg);
int emitf32(vmEnv, flt32t arg);
int emitf64(vmEnv, flt64t arg);
int emit(vmEnv, int opc, ...);			// ret: program counter

//~ int cgen(state, astn ast, int get);		// ret: typeId(ast)

// execute
//typedef int (*dbgf)(vmEnv env, void* pu, int n, void *ip, unsigned ss);
typedef int (*dbgf)(vmEnv vm, int pu, void *ip, long* sptr, int sc);

int exec(vmEnv, unsigned cc, unsigned ss, dbgf dbg, void* ud);
//~ int exec(vmEnv, dbgf dbg, unsigned ss, unsigned su, void *sp);

#endif
