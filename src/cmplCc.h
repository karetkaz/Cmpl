/**
 * Compiler core functions.
 */

#ifndef CMPL_COMPILER_H
#define CMPL_COMPILER_H

#include "cmpl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CAST_any   = 0x0000,		// error
	CAST_vid   = 0x0001,		// void
	CAST_bit   = 0x0002,		// bool
	CAST_i32   = 0x0003,		// int32, int16, int8
	CAST_u32   = 0x0004,		// uint32, uint16, uint8
	CAST_i64   = 0x0005,		// int64
	CAST_u64   = 0x0006,		// uint64
	CAST_f32   = 0x0007,		// float32
	CAST_f64   = 0x0008,		// float64
	CAST_val   = 0x0009,		// value, record
	CAST_ref   = 0x000a,		// reference, pointer, array
	CAST_var   = 0x000b,		// variant: pair of {type, data}
	CAST_arr   = 0x000c,		// slice: pair of {size, data}
	//CAST_d   = 0x000d,		// reserved(map)
	//CAST_e   = 0x000e,		// unused
	//CAST_f   = 0x000f,		// unused(function)

	KIND_def   = 0x0000,		// alias (/ error at runtime)
	KIND_typ   = 0x0010,		// typename: struct metadata info.
	KIND_fun   = 0x0020,		// function
	KIND_var   = 0x0030,		// variable: function and typename are also variables

	ATTR_stat  = 0x0040,		// static attribute
	ATTR_cnst  = 0x0080,		// constant attribute
	ATTR_paral = 0x0100,		// parallel

	MASK_cast  = 0x000f,
	MASK_kind  = 0x0030,
	MASK_attr  = 0x00c0,
} ccKind;
// Tokens - CC
typedef enum {
	#define TOKEN_DEF(NAME, TYPE, SIZE, STR) NAME,
	#include "defs.inl"
	tok_last,

	TOKEN_opc = EMIT_kwd,
	OPER_beg = OPER_fnc,
	OPER_end = OPER_com,
} ccToken;

struct tokenRec {
	int const	type;
	int const	argc;
	char *const	name;
};
extern const struct tokenRec token_tbl[256];

typedef enum {
	installBase = 0x0000,                       // base type system only

	install_ptr = 0x0001,                       // pointers with malloc(memory manager)
	install_var = 0x0002,                       // variants with reflection(runtime type system)
	install_obj = 0x0004,                       // objects with inheritance(counted references)

	installLibs = 0x0008,                       // install standard native functions and extensions

	installEmit = 0x0010 | installBase,         // emit intrinsic: emit(...)
	installEopc = 0x0020 | installEmit,         // emit opcodes: emit.i32.add
	installEswz = 0x0040 | installEopc,         // swizzle constants: emit.swz.(xxxx, ... xyzw, ... wwww)

	// register defaults if ccInit not invoked explicitly.
	install_min = install_ptr | install_var | install_obj | installLibs,
	install_def = install_min | installEopc,
} ccInstall;

/**
 * Initialize compiler context.
 * 
 * @param ctx Runtime context.
 * @param mode specify what to install.
 * @param onHalt function to be executed when execution and external function invocation terminates.
 * @return compiler context.
 * @note installs: builtin types, builtin functions, emit intrinsic, ...
 */
ccContext ccInit(rtContext ctx, ccInstall mode, vmError onHalt(nfcContext));

/**
 * Begin a namespace (static struct) or scope.
 * 
 * @param ctx Compiler context.
 * @param name Name of the namespace.
 * @return Defined symbol, null on error (error is logged).
 */
symn ccBegin(ccContext ctx, const char *name);
/**
 * Extend a namespace (static struct) with static members.
 * 
 * @param ctx Compiler context.
 * @param type Namespace to be extended.
 * @return type if it can be extended.
 */
symn ccExtend(ccContext ctx, symn type);
/**
 * Close the namespace returned by ccBegin or ccExtend.
 * 
 * @param ctx Compiler context.
 * @param cls Namespace to be closed.
 * @note Makes all declared variables static.
*/
symn ccEnd(ccContext ctx, symn cls);

/**
 * Define an integer constant.
 * 
 * @param ctx Compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefInt(ccContext ctx, const char *name, int64_t value);
/**
 * Define a floating point constant.
 * 
 * @param ctx Compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefFlt(ccContext ctx, const char *name, double value);
/**
 * Define a string constant.
 * 
 * @param ctx Compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefStr(ccContext ctx, const char *name, char *value);

/**
 * Define a new type.
 * 
 * @param ctx Compiler context.
 * @param name Name of the type.
 * @param size Size of the type.
 * @param refType Value or Reference type.
 * @return Defined symbol, null on error.
 * @see plugins/file.c
 */
symn ccDefType(ccContext ctx, const char *name, unsigned size, int refType);

/**
 * Define a native function.
 * 
 * @param ctx Compiler context.
 * @param call The native c function wrapper.
 * @param proto Prototype of the function.
 * @return Defined symbol, null on error.
 * @usage
	static vmError f64sin(nfcContext rt) {
		float64_t x = argf64(rt, 0);	// get first argument
		retf64(rt, sin(x));				// set the return value
		return noError;					// do not abort execution
	}
	...
	if (!rt->api.ccDefCall(rt, f64sin, "float64 sin(float64 x)")) {
		// error was reported, terminate by returning error count.
		return rt->errors;
	}
 */
symn ccDefCall(ccContext ctx, vmError call(nfcContext), const char *proto);

/**
 * Compile the given file or text as a unit.
 * 
 * @param ctx Compiler context.
 * @param file File name of input.
 * @param line First line of input.
 * @param text If not null, this will be compiled instead of the file.
 * @return Root node of the compilation unit.
 */
astn ccAddUnit(ccContext ctx, char *file, int line, char *text);

/**
 * Add a new module or library
 * 
 * @param ctx Compiler context.
 * @param init function installing types and native functions.
 * @param unit (optional) extension file to be compiled with this library.
 * @return boolean value of success.
 */
int ccAddLib(ccContext ctx, int init(ccContext), char *unit);

/**
 * Generate bytecode from the compiled syntax tree.
 * 
 * @param ctx Runtime context.
 * @param debug generate debug info
 * @return boolean value of success.
 */
int ccGenCode(rtContext ctx, int debug);

/**
 * Lookup a symbol by name.
 * 
 * @param ctx Compiler context.
 * @param in search in the members of this symbol.
 * @param name name of the symbol to be found.
 * @return the first found symbol.
 */
symn ccLookupSym(ccContext ctx, symn in, char *name);

/// standard modules for `ccAddLib`
int ccLibStd(ccContext ctx);

#ifdef __cplusplus
}
#endif
#endif