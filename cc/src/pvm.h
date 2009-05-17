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
typedef struct astn_t *node;

/// vm

//~ int emitidx(state s, int opc, int pos);
//~ int emiti32(state s, int opc, int32t arg);
//~ int emiti64(state s, int opc, int64t arg);
//~ int emitf32(state s, int opc, flt32t arg);
//~ int emitf64(state s, int opc, flt64t arg);

//~ int cgen(state, int opt);

//~ int emit(state, int opc, ... );

//~ int dump(state, int what, char* file);

//~ state create();
//~ int destroy(state);

//? text = prep(input)
//? toks = clex(text) + lex.errors
//~ tree = scan(text) + stx.errors
//~ root = type(tree) + sem.errors
//~ code = cgen(root) + fatal.errors

int srctext(state, char *file, int line, char *text);	// {source(s,0,text);setsrc(file, line);
int srcfile(state, char *file);				// source(s,1,text);
int logfile(state, char *file);				// file for errors

int compile(state, int mode);				// expr, script, source, decl, makefile

//~ int gencode(state);
//~ int execute(state, int cc, int ss, int dbgfn(state, cell, int puid, bcde ip, int ss));
//~ int source(state, int mode, char* text);		// mode: file/text



//~ node unit(state, int mode);				// source / script
//~ node decl(state, int mode);				// enable statements
//~ node stmt(state, int mode);				// enable decls
//~ node expr(state, int mode);				// enable comma

//~ int cgen(state, int dbgl);			// 

//~ int emit(state, int opc, ...);		// ret: error code
//~ int exec(state, int cc, int ss);	// ret: error code

#endif
