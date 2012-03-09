#ifndef __CCVM_API
#define __CCVM_API

#include "api.h"

//~ typedef struct astn *astn;		// tree
//~ typedef struct ccState *ccState;

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
	creg_def = creg_eopc,// | creg_swiz,
};

typedef enum {
	srcText = 0x00,		// file / buffer
	srcFile = 0x10,		// file / buffer

	//~ srcUnit = 0x01,
	//~ srcDecl = 0x02,
	//~ srcExpr = 0x03,

	// unit / script (ask the file what is it ? (: first tokens : 'package' 'name'))

	//~ dump_bin = 0x0000100,
	dump_sym = 0x0000100,
	dump_asm = 0x0000200,
	dump_ast = 0x0000400,
	dump_txt = 0x0000800,
	dumpMask = 0x0000f00,
} srcType, dumpMode;

// Runtime / global state
state rtInit(void* mem, unsigned size);
void* rtAlloc(state rt, void* ptr, unsigned size);	// realloc/alloc/free

/** return the internal offset of a reference
 *  aborts if ptr not inside the context.
 */
int vmOffset(state, void *ptr);

// output
void fputfmt(FILE *fout, const char *msg, ...);
void dump(state, dumpMode, symn, char* text, ...);

// logging
int logFILE(state, FILE *file);				// set logger
int logfile(state, char *file);				// set logger

// compile
//~ int srcfile(state, char *file);				// source
//~ int srctext(state, char *file, int line, char *src);				// source
//~ int compile(state, int level);				// warning level
int gencode(state, int level);				// optimize level
//~ int execute(state, int cc, int ss, dbgf dbg);
int parse(ccState, srcType, int wl);

symn libcall(state, int libc(state), const char* proto);
symn installtyp(state, const char* name, unsigned size, int refType);
//~ symn installvar(state s, const char* name, symn type, int refType);	// install a static variable.

// instal io, mem, math and parse optionaly the given file
int install_stdc(state rt, char* file, int level);

// Level 1 Functions: use less these
ccState ccInit(state, int mode, int libcHalt(state));
ccState ccOpen(state, srcType, char* source);
int ccDone(state);

//~ state vmInit(state);
//~ int vmOpen(state, char* binary);
//~ int vmDone(state s);

void ccSource(ccState, char *file, int line);		// set source position

// declaring namespaces
symn ccBegin(state, char *cls);

//~ declaring constants
symn ccDefInt(state, char *name, int64_t value);
symn ccDefFlt(state, char *name, double value);
symn ccDefStr(state, char *name, char* value);

//~ declaring types and variables
//~ symn ccDefVar(state, char *name, char* value);
//~ symn ccDefType(state, char *name, char* value);

void ccEnd(state, symn cls);

// searching for symbols ...
symn findref(state, void *ptr);
symn findsym(ccState, symn in, char *name);

int symvalint(symn sym, int* res);
int symvalflt(symn sym, double* res);

// executing code ...
typedef int (*dbgf)(state, int pu, void *ip, long* sptr, int scnt);

int vmExec(state, dbgf dbg);
int vmCall(state, symn fun, ...);

#endif
