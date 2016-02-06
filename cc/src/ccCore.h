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
		variable    = 0x000030;		// functions and typenames are also variables

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

	install_ptr = 0x0001,                       // pointers and memory manager
	install_var = 0x0002,                       // variants and reflection
	install_obj = 0x0004,                       // classes and inheritance

	installEmit = 0x0010 | installBase,         // emit thingie: emit(...)
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

/// Begin a namespace; @see rtContext.api.ccBegin
symn ccBegin(rtContext, const char *name);

/// Close a namespace; @see rtContext.api.ccEnd
symn ccEnd(rtContext, symn sym, int mode);

/// Declare int constant; @see rtContext.api.ccDefInt
symn ccDefInt(rtContext, const char *name, int64_t value);
/// Declare float constant; @see rtContext.api.ccDefFlt
symn ccDefFlt(rtContext, const char *name, double value);
/// Declare string constant; @see rtContext.api.ccDefStr
symn ccDefStr(rtContext, const char *name, char* value);

/// Declare a type; @see rtContext.api.ccDefType
symn ccDefType(rtContext, const char* name, unsigned size, int refType);

/// Declare a native function; @see rtContext.api.ccDefCall
symn ccDefCall(rtContext, vmError call(libcContext), void *data, const char *proto);

/// Compile file or text; @see rtContext.api.ccDefCode
int ccDefCode(rtContext, int warn, char *file, int line, char *text);

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
symn ccLookupSym(ccContext, symn in, char *name);

/// standard functions
int ccUnitStdc(rtContext);
/// file access
int ccUnitFile(rtContext);

#ifdef __cplusplus
}
#endif
#endif
