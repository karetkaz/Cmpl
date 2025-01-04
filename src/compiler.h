/**
 * Compiler core functions.
 */
#ifndef CMPL_COMPILER_H
#define CMPL_COMPILER_H

#include <cmpl.h>
#include "util.h"

extern const char * const type_doc_builtin;
extern const char * const type_fmt_signed32;
extern const char * const type_fmt_signed64;
extern const char * const type_fmt_unsigned32;
extern const char * const type_fmt_unsigned64;
extern const char * const type_fmt_float32;
extern const char * const type_fmt_float64;
extern const char * const type_fmt_character;
extern const char * const type_fmt_string;
extern const char * const type_fmt_typename;
extern const char type_fmt_string_chr;
extern const char type_fmt_character_chr;

extern const char *const sys_try_exec;

typedef enum ccConfig {
	// maximum token count in expressions
	maxTokenCount = 1024,

	// pre allocate space for argument on the stack
	// faster execution if each argument is pushed when calculated
	preAllocateArgs = 0,

	// what to install
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
} ccConfig;

/// Compiler context
struct ccContextRec {
	rtContext rt;
	astn root;                      // statements
	symn owner;                     // scope variables and functions
	symn scope;                     // scope variables and functions (next is symn->scope)
	symn global;                    // global variables and functions (next is symn->global)
	list native;                    // list of native functions
	struct arrBuffer jumps;         // list of break and continue statements to fix

	int32_t nest;                   // nest level: modified by (enter/leave)
	unsigned inStaticIfFalse: 1;    // inside a static if false
	unsigned genDocumentation: 1;   // generate documentation
	unsigned genStaticGlobals: 1;   // generate global variables as static variables
	unsigned errPrivateAccess: 1;   // raise error accessing private data
	unsigned errUninitialized: 1;   // raise error for uninitialized variables
	astn scopeStack[maxTokenCount]; // scope stack used by enter leave

	// Lexer
	list stringTable[512];          // string table (hash)
	astn tokPool;                   // list of recycled tokens
	astn tokNext;                   // next token: look-ahead

	// Parser
	symn symbolStack[64];           // symbol stack (hash)
	const char *home;               // home folder
	const char *unit;               // unit file name
	const char *file;               // current file name
	int line;                       // current line number

	// Type cache
	symn type_vid;        // void
	symn type_bol;        // boolean
	symn type_chr;        // character
	symn type_i08;        //  8bit signed integer
	symn type_i16;        // 16bit signed integer
	symn type_i32;        // 32bit signed integer
	symn type_i64;        // 64bit signed integer
	symn type_u08;        //  8bit unsigned integer
	symn type_u16;        // 16bit unsigned integer
	symn type_u32;        // 32bit unsigned integer
	symn type_u64;        // 64bit unsigned integer
	symn type_f32;        // 32bit floating point
	symn type_f64;        // 64bit floating point
	symn type_ptr;        // pointer
	symn type_var;        // variant
	symn type_rec;        // typename
	symn type_fun;        // function
	symn type_obj;        // object
	symn type_str;        // c-string

	symn type_int;        // integer: 32-bit/64-bit signed

	symn null_ref;        // variable null
	symn length_ref;      // slice length attribute

	symn emit_opc;        // emit intrinsic function, or whatever it is.
	astn void_tag;        // used to lookup function call with 0 argument
	astn init_und;        // used for undefined initialization

	symn libc_dbg;        // raise(char file[*], int line, int level, int trace, char message[*], variant details...);
	symn libc_try;        // tryExec(pointer args, void action(pointer args));
	symn libc_mal;        // pointer.alloc(pointer old, int size);
	symn libc_new;        // object.create(typename type);
};

/**
 * Initialize compiler context.
 *
 * @param ctx Runtime context.
 * @param config specify what to install.
 * @param onHalt function to be executed when execution and external function invocation terminates.
 * @return compiler context.
 * @note installs: builtin types, builtin functions, emit intrinsic, ...
 */
ccContext ccInit(rtContext ctx, ccConfig config, vmError onHalt(nfcContext));

/**
 * Open a stream (file or text) for compilation.
 *
 * @param ctx Compiler context.
 * @param file file name of input.
 * @param line first line of input.
 * @param content if not null, this will be compiled instead of the file.
 * @return error code, 0 on success.
 */
int ccParse(ccContext ctx, const char *file, int line, char *content);


/** Add string to string table.
 * @brief add string to string table.
 * @param ctx compiler context.
 * @param str the string to be mapped.
 * @param size the length of the string, -1 recalculates using strlen.
 * @param hash pre-calculated hashcode, -1 recalculates.
 * @return the mapped string in the string table.
 */
const char *ccUniqueStr(ccContext ctx, const char *str, size_t size/* = -1*/, unsigned hash/* = -1*/);

#endif
