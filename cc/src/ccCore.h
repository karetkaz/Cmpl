/**
 * compiler core functions.
 */
#ifndef CC_CORE_H
#define CC_CORE_H

#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tokens - CC
typedef enum {
#define TOKDEF(NAME, TYPE, SIZE, STR) NAME,
#include "defs.inl"
	tok_last,

	OPER_beg = OPER_fnc,
	OPER_end = OPER_com,

	TOKN_err = TYPE_any,
	TYPE_int = TYPE_i64,
	TYPE_flt = TYPE_f64,
	TYPE_str = TYPE_ptr,

	KIND_mask   = 0x0ff,		// mask
	ATTR_mask   = 0xf00,		// mask
	ATTR_stat   = 0x400,		// static
	ATTR_const  = 0x800,		// constant
	ATTR_paral  = 0x100,		// parallel

	stmt_NoDefs = 0x100,		// disable typedefs in stmt.
	stmt_NoRefs = 0x200,		// disable variables in stmt.

	decl_NoDefs = 0x100,		// disable typedefs in decl.
	decl_NoInit = 0x200,		// disable initialization.
	decl_ItDecl = 0x400,		// enable ':' after declaration: for(int a : range(0, 12))

	/*? enum Kind {
		CAST_any    = 0x000000;		// invalid, error, ...
		CAST_vid    = 0x000001;		// void;
		CAST_bit    = 0x000002;		// bool;
		CAST_i32    = 0x000003;		// int32, int16, int8

		// ...
		// alias    = 0x000000;		// invalid at runtime.

		KIND_typ
		typename    = 0x000010;		// struct metadata info.

		KIND_fun
		function    = 0x000020;		// function

		KIND_var
		variable    = 0x000030;		// functions and types are also variables

		ATTR_const  = 0x000040;
		ATTR_static = 0x000080;
	}*/
} ccToken;
struct tok_inf {
	int const	type;
	int const	argc;
	char *const	name;
};
extern const struct tok_inf tok_tbl[255];

enum {
	// ccInit
	installBase = 0x0000,                       // basic type system only

	install_ptr = 0x0001,                       // pointers and malloc(memory manager)
	install_var = 0x0002,                       // variants and reflection(runtime type system)
	install_obj = 0x0004,                       // classes and inheritance(counted references)

	installEmit = 0x0010 | installBase,         // emit intrinsic: emit(...)
	installEopc = 0x0020 | installEmit,         // emit opcodes: emit.i32.add
	installEswz = 0x0040 | installEopc,         // swizzle constants: emit.swz.(xxxx, ... xyzw, ... wwww)

	// register defaults if ccInit not invoked explicitly.
	install_def = install_ptr + install_var + install_obj + installEopc,
	install_all = install_ptr + install_var + install_obj + installEswz,
};

/**
 * @brief Initialize compiler context.
 * @param runtime context.
 * @param mode see the enum above
 * @param onHalt function to be executed when execution terminates.
 * @return compiler context.
 * @note installs: base types, emit, builtin functions
 */
ccContext ccInit(rtContext, int mode, vmError onHalt(libcContext));

/**
 * @brief Begin a namespace (static struct).
 * @param Runtime context.
 * @param name Name of the namespace.
 * @return Defined symbol, null on error.
 */
symn ccBegin(rtContext, const char *name);
/**
 * @brief Close the namespace.
 * @param Runtime context.
 * @param cls Namespace to be closed. (The returned by ccBegin.)
 * @note Makes all declared variables static.
*/
symn ccEnd(rtContext, symn sym, int mode);

/**
 * @brief Define an integer constant.
 * @param Runtime context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefInt(rtContext, const char *name, int64_t value);
/**
 * @brief Define a floating point constant.
 * @param Runtime context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefFlt(rtContext, const char *name, double value);
/**
 * @brief Define a string constant.
 * @param Runtime context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefStr(rtContext, const char *name, char* value);

/**
 * @brief Add a type to the runtime.
 * @param Runtime context.
 * @param name Name of the type.
 * @param size Size of the type.
 * @param refType Value indicating ByRef or ByValue.
 * @return Defined symbol, null on error.
 * @see: lstd.File;
 */
symn ccDefType(rtContext, const char* name, unsigned size, int refType);
/**
 * @brief Add a native function (libcall) to the runtime.
 * @param Runtime context.
 * @param libc The c function.
 * @param data Extra user data for the function.
 * @param proto Prototype of the function. (Must end with ';')
 * @return Defined symbol, null on error.
 * @usage see also: test.gl/gl.c
	static int f64sin(libcContext rt) {
		float64_t x = argf64(rt, 0);	// get first argument
		retf64(rt, sin(x));				// set the return value
		return 0;						// no runtime error in call
	}
	...
	if (!rt->api.ccDefCall(rt, f64sin, NULL, "float64 sin(float64 x);")) {
		// error reported, terminate here the execution.
		return 0;
	}
 */
symn ccDefCall(rtContext, vmError call(libcContext), void *data, const char *proto);

/**
 * @brief Compile the given file or text block.
 * @param Runtime context.
 * @param warn Warning level.
 * @param file File name of input.
 * @param line First line of input.
 * @param text If not null, this will be compiled instead of the file.
 * @return the root node of the compilation unit.
 */
astn ccAddUnit(rtContext, int warn, char *file, int line, char *text);

/** Add a module / library
 * @brief Execute the libInit function then optionally compile the unit.
 * @param runtime context.
 * @param warn warning level.
 * @param unit function installing types and native functions.
 * @param unit additional file to be compiled with this library.
 * @return boolean value of success.
 */
int ccAddLib(rtContext, int libInit(rtContext), int warn, char *unit);

/**
 * @brief Find symbol by name.
 * @param compiler context.
 * @param in search for symbol as member of this.
 * @param name name of the symbol to be found.
 * @return the first found symbol.
 */
symn ccLookupSym(ccContext, symn in, char *name);

/// standard functions
int ccLibStdc(rtContext);
/// file access
int ccLibFile(rtContext);

#ifdef __cplusplus
}
#endif
#endif
