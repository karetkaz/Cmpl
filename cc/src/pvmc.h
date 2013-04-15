#ifndef __PVMC_API
#define __PVMC_API

#include "api.h"

enum {
	creg_base = 0x0000,			// type system only

	creg_tptr = 0x0001,			//~ pointers with memory manager
	creg_tvar = 0x0002,			//~ variants with reflection

	creg_emit = 0x0010,				// emit(...)
	creg_eopc = 0x0020 | creg_emit,	// emit opcodes : emit.i32.add
	creg_swiz = 0x0040 | creg_eopc,	// swizzle constants : emit.swz.(xxxx, ... xyzw, ... wwww)

	// register defaults if ccInit not invoked explicitly.
	creg_def  = creg_tptr + creg_tvar + creg_eopc,

	// dump(dump_xxx | (level & 0xff))
	dump_sym = 0x0000100,
	dump_ast = 0x0000200,
	dump_asm = 0x0000400,
	dump_bin = 0x0000800,
};

/** Initialize the global state of execution and or compilation.
 * @param mem if not null allocate memory else use it for the state
 * @param size the size of memory in bytes to be used.
 * 
 */
state rtInit(void* mem, unsigned size);

// logging
int logFILE(state, FILE *file);				// set logger
int logfile(state, char *file);				// set logger

// compile
/** Add a libcall (native function) to the runtime.
 * initializes the compiler state.
 * @param runtime state.
 * @param libc the c function.
 * @param proto prototype of the function, do not forget the ending ';'
 * @return the symbol to the definition, null on error.
 * @usage:
	static int f64sin(state rt) {
		float64_t x = popf64(rt);		// pop one argument
		setret(rt, float64_t, sin(x));	// set the return value
		return 0;						// no error in this call
	}
	if (!ccAddCall(api->rt, f64sin, "float64 sin(float64 x);")) {
		error...
	}
 */
symn ccAddCall(state, int libc(state, void* data), void* data, const char* proto);

/// install a type symbol
symn ccAddType(state, const char* name, unsigned size, int refType);

/** compile the given file or text block.
 * initializes the compiler state.
 * @param state
 * @param warn the warning level to be used.
 * @param file filename of the compiled unit; this file will be opened if code is not null.
 * @param line linenumber where compilation begins; it will be reset if code is not null.
 * @param code if not null, this text will be compiled instead of the file.
 * @return the number of errors.
 */
int ccAddCode(state, int warn, char *file, int line, char *code);

// declaring namespaces
symn ccBegin(state, char *cls);

//~ declaring constants
symn ccDefInt(state, char *name, int64_t value);
symn ccDefFlt(state, char *name, double value);
symn ccDefStr(state, char *name, char* value);

void ccEnd(state, symn cls);

ccState ccInit(state, int mode, int libcHalt(state, void*));
ccState ccOpen(state, char* file, int line, char* source);
int ccDone(state);

// searching for symbols ...
symn symfind(state, void *ptr);

symn ccFindSym(ccState, symn in, char *name);
int ccSymValInt(symn sym, int* res);
int ccSymValFlt(symn sym, double* res);

/** instal standard functions.
 * io, mem, math and parse optionaly the given file
 * @param state
 * @param file if not null parse this file @see stdlib.cvx
 * @param level warning level for parsing.
 */
int install_stdc(state, char* file, int level);

/** Memory manager for the virtual machine
 * this function should be called after vmExec.
 * aborts if ptr is not null and not inside the context.
 * @param the runtime state.
 * @param ptr: an allocated memory address in the vm or null.
 * @param size: the new size to reallocate or 0 to free memory.
 * @return non zero on error.
	ptr == null && size == 0: nothing
	ptr == null && size >  0: alloc
	ptr != null && size == 0: free
	ptr != null && size >  0: TODO: realloc
*/
void* rtAlloc(state rt, void* ptr, unsigned size);	// realloc/alloc/free

int gencode(state, int opti);
//~ int execute(state, int cc, int ss, dbgf dbg);

// executing code ...
typedef int (*dbgf)(state, int pu, void *ip, long* sptr, int scnt);

int vmExec(state, dbgf dbg, int ss);
int vmCall(state, symn fun, void* ret, void* args);

// output
void fputfmt(FILE *fout, const char *msg, ...);
void dump(state, int dumpWhat, symn, char* msg, ...);

typedef struct astn *astn;		// Abstract Syntax Tree Node

/* symbols
	there are 4 kind of symbols:[TODO]
		alias is a shortcut to another symbol or an expression
		typename
		variable
		function
*/
struct symn {				// type node (data)
	char*	name;		// symbol name
	char*	file;		// declared in file
	int		line;		// declared on line

	int32_t	size;		// sizeof(TYPE_xxx)
	int32_t	offs;		// addrof(TYPE_xxx)

	symn	type;		// base type of TYPE_ref/TYPE_arr/function (void, int, float, struct, ...)

	//~ TODO: temporarly array variable base type
	symn	args;		// struct members / function paramseters
	symn	sdef;		// static members (is the tail of args) / function return value(out value)

	symn	decl;		// declared in namespace/struct/class, function, ...
	symn	next;		// symbols on table / next param / next field / next symbol

	#if DEBUGGING
	ccToken	kind;		// TYPE_def / TYPE_rec / TYPE_ref / TYPE_arr
	ccToken	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	uint16_t __castkindpadd;
	#else
	uint8_t	kind;		// TYPE_ref / TYPE_def / TYPE_rec / TYPE_arr
	uint8_t	cast;		// casts to type(TYPE_(bit, vid, ref, u32, i32, i64, f32, f64, p4x)).
	#endif

	union {				// Attributes
	struct {
	// TODO: remove call
	uint16_t	call:1;		// callable(function/definition) <=> (kind == TYPE_ref && args)
	uint16_t	cnst:1;		// constant
	uint16_t	priv:1;		// private
	uint16_t	stat:1;		// static ?
	uint16_t	glob:1;		// global

	//~ uint8_t	load:1;		// indirect reference: cast == TYPE_ref
	//~ uint8_t	used:1;		// 
	uint16_t _padd:11;		// attributes (const static /+ private, ... +/).
	};
	uint16_t	Attr;
	};

	int		nest;		// declaration level

	symn	defs;		// global variables and functions / while_compiling variables of the block in reverse order
	symn	gdef;		// static variables and functions / while_compiling ?

	astn	init;		// VAR init / FUN body, this shuld be null after codegen
	astn	used;		// how many times was referenced by lookup
	char*	pfmt;		// TEMP: print format
};

static inline int padded(int offs, int align) {
	//~ assert(align == (align & -align));
	return (offs + (align - 1)) & ~(align - 1);
}

#endif
