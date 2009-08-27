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

//~ int cgen(state, int opt);

//~ int emit(state, int opc, ... );

//~ state create();
//~ int destroy(state);

typedef enum {
	source_none = 0,
	source_file,
	source_text,
	binary_file,
} vmLoadType;

int vmLoad(state, vmLoadType, char*);

int source(state, int mode, char* text);		// mode: file/text
int logger(state, char *file);				// file for errors
int srctext(state, char *file, int line, char *text);
int srcfile(state, char *file);				// source(s,1,text);
int logfile(state, char *file);				// file for errors

int compile(state, int mode);				// script, source, decl, makefile
int gencode(state, int mode);				// debug level / optimizations
int execute(state, int cc, int ss);

// compile
node scan(state, int mode);				// source / script
node decl(state, int mode);				// enable statements
node stmt(state, int mode);				// enable decls
node expr(state, int mode);				// enable comma
//~ defn spec(state, int qual);
//~ defn type(state, int qual);

// gencode
int emitidx(state, int opc, int stpos);
int emiti32(state, int opc, int32t arg);
int emiti64(state, int opc, int64t arg);
int emitf32(state, int opc, flt32t arg);
int emitf64(state, int opc, flt64t arg);
int emit(state, int opc, ...);			// ret: program counter

int cgen(state, node ast, int get);		// ret: typeId(ast)

// execute
//~ int exec(state, int cc, int ss);	// ret: error code

#endif
