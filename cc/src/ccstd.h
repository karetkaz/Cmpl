#ifndef CC_BASE_H
#define CC_BASE_H 2

#include "ccapi.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	// ccInit
	creg_base = 0x0000,					// type system only

	creg_tptr = 0x0001 | creg_base,		// pointers and memory manager
	creg_tvar = 0x0002 | creg_base,		// variants and reflection
	creg_tobj = 0x0004 | creg_base,		// classes and inheritance

	creg_emit = 0x0100 | creg_base,		// emit thingie: emit(...)
	creg_eopc = 0x0200 | creg_emit,		// emit opcodes: emit.i32.add
	creg_eswz = 0x0400 | creg_eopc,		// swizzle constants: emit.swz.(xxxx, ... xyzw, ... wwww)

	// register defaults if ccInit not invoked explicitly.
	creg_def  = creg_tptr + creg_tvar + creg_tobj + creg_eswz,
	creg_min  = creg_tptr + creg_tvar + creg_tobj + creg_emit,
};

/**
 * @brief Initialize runtime context.
 * @param mem optionally use this memory.
 * @param size the size of memory in bytes to be used.
 * @return runtime context.
 */
rtContext rtInit(void* mem, size_t size);

// logging
void logFILE(rtContext, FILE *file);			// set logger
int logfile(rtContext, char *file, int append);	// set logger

// compile
/**
 * @brief Initialze compiler context.
 * @param runtime context.
 * @param mode see the enum abowe
 * @param onHalt function to be executed when execution terminates.
 * @return compiler context.
 * @note installs: base types, emit, builtin functions
 */
ccContext ccInit(rtContext, int mode, int onHalt(libcContext));

/// Begin a namespace; @see rtContext.api.ccBegin
symn ccBegin(rtContext, const char *cls);

/// Declare int constant; @see rtContext.api.ccDefInt
symn ccDefInt(rtContext, char *name, int64_t value);
/// Declare float constant; @see rtContext.api.ccDefFlt
symn ccDefFlt(rtContext, char *name, double value);
/// Declare string constant; @see rtContext.api.ccDefStr
symn ccDefStr(rtContext, char *name, char* value);

/// Close a namespace extended.
void ccExtEnd(rtContext, symn sym, int mode);
/// Close a namespace; @see rtContext.api.ccEnd
void ccEnd(rtContext, symn sym);

/// Install a native function; @see rtContext.api.ccAddCall
symn ccAddCall(rtContext, int call(libcContext), void* data, const char* proto);

/// Install a type; @see rtContext.api.ccAddType
symn ccAddType(rtContext, const char* name, unsigned size, int refType);

/// Compile file or text; @see rtContext.api.ccAddCode
int ccAddCode(rtContext, int warn, char *file, int line, char *text);

/**
 * @brief Execute the unit function then optionally compile the file.
 * @param runtime context.
 * @param warn warning level.
 * @param unit function installing types and native functions.
 * @param file additional file to be compiled with this unit.
 * @return boolean value of success.
 */
int ccAddUnit(rtContext, int unit(rtContext), int warn, char *file);

/**
 * @brief Find symbol by name.
 * @param compiler context.
 * @param in search for symbol as member of this.
 * @param name name of the symbol to be found.
 * @return the first found symbol.
 */
symn ccFindSym(ccContext, symn in, char *name);

/** TODO: to be removed
 * @brief get the int value of constant symbol.
 * @param sym constant symbol.
 * @param res copy the value of constant here.
 * @return TYPE_int or 0 on fail.
 */
int ccSymValInt(symn sym, int* res);
/** TODO: to be removed
 * @brief get the float value of constant symbol.
 * @param sym constant symbol.
 * @param res copy the value of constant here.
 * @return TYPE_flt or 0 on fail.
 */
int ccSymValFlt(symn sym, double* res);

/** Instal standard functions.
 * io, mem, math, ...
 * @param Runtime context.
 */
int install_stdc(rtContext);

/** Instal file functions.
 * open, read, delete, ...
 * @param Runtime context.
 */
int install_file(rtContext);

/** Generate executable bytecode.
 * @param Runtime context.
 * @param mode see@(cgen_opti, cgen_info, cgen_glob)
 * @return boolean value of success.
 */
int gencode(rtContext, int mode);

/**
 * @brief Execute the compiled script.
 * @param Runtime context.
 * @param ss Stack size in bytes.
 * @param extra Extra data for libcalls.
 * @return Error code of execution, 0 on success.
 * @todo units number of execution units to run on.
 * @todo argc(int) argument count from main.
 * @todo argv(char*[]) arguments from main.
 */
vmError execute(rtContext, size_t ss, void *extra);

/// Invoke a function; @see rtContext.api.invoke
vmError invoke(rtContext, symn fun, void *ret, void* args, void *extra);

/// Lookup symbol by offset; @see rtContext.api.mapsym
symn mapsym(rtContext, size_t offs, int callsOnly);
symn getsym(rtContext, void* offs);

/// Check if at the given offset a resource string is located.
char* getResStr(rtContext rt, size_t offs);

/// Alloc, resize or free memory; @see rtContext.api.rtAlloc
void* rtAlloc(rtContext, void* ptr, size_t size, void dbg(rtContext rt, void* mem, size_t size, int used));

static inline void* paddptr(void *offs, unsigned align) {
	return (void*)(((ptrdiff_t)offs + (align - 1)) & ~(ptrdiff_t)(align - 1));
}
static inline size_t padded(size_t offs, unsigned align) {
	return (offs + (align - 1)) & ~(align - 1);
}

#ifdef __cplusplus
}
#endif
#endif
