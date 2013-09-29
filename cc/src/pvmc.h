#include "api.h"

#ifndef CC_BASE_H
#define CC_BASE_H 2
#ifdef __cplusplus
extern "C" {
#endif

enum {
	creg_base = 0x0000,			// type system only

	creg_tptr = 0x0001,			// pointers and memory manager
	creg_tvar = 0x0002,			// variants and reflection

	creg_emit = 0x0010,				// emit thingie: emit(...)
	creg_eopc = 0x0020 | creg_emit,	// emit opcodes: emit.i32.add
	creg_eswz = 0x0040 | creg_eopc,	// swizzle constants: emit.swz.(xxxx, ... xyzw, ... wwww)

	// register defaults if ccInit not invoked explicitly.
	creg_def  = creg_tptr + creg_tvar + creg_eopc,

	// dump(dump_xxx | (level & 0xff))
	//~ dump_msg = 0x0000000,
	dump_sym = 0x0000100,
	dump_ast = 0x0000200,
	dump_asm = 0x0000400,
	dump_bin = 0x0000800
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
void dump(state, int dumpWhat, symn, const char* msg, ...);

// compile
/** Initialze compiler state
 * installs base types, emit, standard lib, ...
 */
ccState ccInit(state, int mode, int onHalt(state, void*));

// declare and start a namespace (static struct)
symn ccBegin(state, char *cls);

//~ declare constants
symn ccDefInt(state, char *name, int64_t value);
symn ccDefFlt(state, char *name, double value);
symn ccDefStr(state, char *name, char* value);

// end the current namespace (static struct)
void ccExtEnd(state, symn sym, int mode);
void ccEnd(state, symn sym);

/** Add a libcall (native function) to the runtime.
 * initializes the compiler state.
 * @param runtime state.
 * @param libc the c function.
 * @param data extra data for the function.
 * @param proto prototype of the function, do not forget the ending ';'
 * @return the symbol to the definition, null on error.
 * @usage:
	static int f64sin(state rt, void* _) {
		float64_t x = popf64(rt);		// pop one argument
		setret(rt, float64_t, sin(x));	// set the return value
		return 0;						// no error in this call
	}
	if (!ccAddCall(api->rt, f64sin, NULL, "float64 sin(float64 x);")) {
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
 * @return boolean value of success.
 */
int ccAddCode(state, int warn, char *file, int line, char *code);

/** execute the unit function then compile the file
 * @param context
 * @param warn the warning level to be used.
 * @param unit this function should install types and functions of the unit
 * @param file additional file to be compiled with this unit.
 */
int ccAddUnit(state, int unit(state), int warn, char *file);

symn ccFindSym(ccState, symn in, char *name);
int ccSymValInt(symn sym, int* res);
int ccSymValFlt(symn sym, double* res);

/** instal type system.
 * io, mem, math, ...
 * @param state runtime state.
 + @param level warning level.
 + @param file additional file extending module
 */
int install_base(state, int mode, int onHalt(state, void*));

/** instal standard functions.
 * io, mem, math, ...
 * @param state context
 */
int install_stdc(state);

/** instal file functions.
 * @param state
 */
int install_file(state);

/** generate executable bytecode.
 * @param context
 * @param optimizeLevel level of optimization
 * @todo param onHalt the libcall which is executed before vmExec or vmCall returns.
 * @param dbg debuger function, this is executed for each opcode.
 * @return 0 on success.
 */
int gencode(state, int optimizeLevel, int dbg(state, int pu, void *ip, long* sp, int ss));

/** execute initializer.
 * @param context
 * @todo param cc number of execution units to run on.
 * @param ss stack size to use for one execution unit.
 * @todo param argc(int) argument count from main.
 * @todo param args(char*[]) arguments from main.
 * @return 0 on success.
 */
int vmExec(state, int ss);

/** execute a function inside the context.
 * @param context
 * @param fun symbol of the function to be executed.
 * @param ret result of the function.
 * @param args arguments for the function.
 * @return 0 on success.
 */
int vmCall(state, symn fun, void* ret, void* args);

int libCallHaltDebug(state, void*);

// searching for symbols ...
symn mapsym(state, void *ptr);

/** memory manager for the virtual machine
 * this function should be called after vmExec.
 * aborts if ptr is not null and not inside the context.
 * @param context.
 * @param ptr an allocated memory address in the vm or null.
 * @param size the new size to reallocate or 0 to free memory.
 * @return allocated memory in the vm context.
 *     ptr == null && size == 0: nothing
 *     ptr == null && size >  0: alloc
 *     ptr != null && size == 0: free
 *     ptr != null && size >  0: TODO: realloc
*/
void* rtAlloc(state, void* ptr, unsigned size);

/* TODO: symbols
	there are 4 kind of symbols:[TODO]
		0: alias is a shortcut to another symbol or an expression
		1: typename
		2: variable
		3: function
*/
static inline void* paddptr(void *offs, unsigned align) {
	//~ assert(align == (align & -align));
	return (void*)(((ptrdiff_t)offs + (align - 1)) & ~(ptrdiff_t)(align - 1));
}
static inline int padded(int offs, unsigned align) {
	return (int)paddptr((void*)offs, align);
}

#ifdef __cplusplus
}
#endif
#endif
