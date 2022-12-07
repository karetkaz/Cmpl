/**
 * Compiler core functions.
 */

#ifndef CMPL_CC_H
#define CMPL_CC_H

#include "cmpl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	#define TOKEN_DEF(NAME, TYPE, SIZE, STR) NAME,
	#include "defs.inl"
	tok_last,

	TOKEN_opc = EMIT_kwd,
	OPER_beg = OPER_fnc,
	OPER_end = OPER_com,
} ccToken;

struct tokenRec {
	const char *name;
	const int type;
	const int args;
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

enum Settings {
	// maximum token count in expressions
	maxTokenCount = 1024,

	// size of the hash table for the symbol names
	hashTableSize = 512,

	// limit the count of printed elements(stacktrace, array elements)
	maxLogItems = 25,

	// pre allocate space for argument on the stack
	// faster execution if each argument is pushed when calculated
	preAllocateArgs = 0
};

// linked list
typedef struct list {
	struct list *next;
	const char *data;
} *list;

/// Compiler context
struct ccContextRec {
	rtContext rt;
	astn root;                      // statements
	symn owner;                     // scope variables and functions
	symn scope;                     // scope variables and functions (next is symn->scope)
	symn global;                    // global variables and functions (next is symn->global)
	list native;                    // list of native functions
	astn jumps;                     // list of break and continue statements to fix

	int32_t nest;                   // nest level: modified by (enter/leave)
	unsigned inStaticIfFalse: 1;    // inside a static if false
	unsigned genDocumentation: 1;   // generate documentation
	unsigned genStaticGlobals: 1;   // generate global variables as static variables
	unsigned errPrivateAccess: 1;   // raise error accessing private data
	unsigned errUninitialized: 1;   // raise error for uninitialized variables
	astn scopeStack[maxTokenCount]; // scope stack used by enter leave

	// Lexer
	list stringTable[hashTableSize];// string table (hash)
	astn tokPool;                   // list of recycled tokens
	astn tokNext;                   // next token: look-ahead

	// Parser
	symn symbolStack[hashTableSize];// symbol stack (hash)
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
	symn type_str;        // string

	symn type_int;        // integer: 32-bit/64-bit signed
	symn type_idx;        // length / index: 32-bit/64-bit unsigned

	symn null_ref;        // variable null
	symn length_ref;      // slice length attribute

	symn emit_opc;        // emit intrinsic function, or whatever it is.
	astn void_tag;        // used to lookup function call with 0 argument

	symn libc_dbg;        // raise(char file[*], int line, int level, int trace, char message[*], variant inspect);
	symn libc_try;        // tryExec(pointer args, void action(pointer args));
	symn libc_mal;        // pointer.alloc(pointer old, int size);
	symn libc_new;        // object.create(typename type);
};

/**
 * Initialize compiler context.
 *
 * @param rt Runtime context.
 * @param mode specify what to install.
 * @param onHalt function to be executed when execution and external function invocation terminates.
 * @return compiler context.
 * @note installs: builtin types, builtin functions, emit intrinsic, ...
 */
ccContext ccInit(rtContext rt, ccInstall mode, vmError onHalt(nfcContext));

/**
 * Open a stream (file or text) for compilation.
 *
 * @param cc Compiler context.
 * @param file file name of input.
 * @param line first line of input.
 * @param text if not null, this will be compiled instead of the file.
 * @return error code, 0 on success.
 */
int ccOpen(ccContext cc, const char *file, int line, char *text);

/**
 * Close stream, ensuring it ends correctly.
 *
 * @param cc Compiler context.
 * @return number of errors.
 */
int ccClose(ccContext cc);

/**
 * Begin a namespace (static struct) or scope.
 *
 * @param cc Compiler context.
 * @param name Name of the namespace.
 * @return Defined symbol, null on error (error is logged).
 */
symn ccBegin(ccContext cc, const char *name);
/**
 * Extend a namespace (static struct) with static members.
 *
 * @param cc Compiler context.
 * @param sym Namespace to be extended.
 * @return type if it can be extended.
 */
symn ccExtend(ccContext cc, symn sym);
/**
 * Close the namespace returned by ccBegin or ccExtend.
 *
 * @param cc Compiler context.
 * @param cls Namespace to be closed.
 * @note Makes all declared variables static.
*/
symn ccEnd(ccContext cc, symn sym);

/**
 * Define an integer constant.
 *
 * @param cc Compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefInt(ccContext cc, const char *name, int64_t value);
/**
 * Define a floating point constant.
 *
 * @param cc Compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefFlt(ccContext cc, const char *name, double value);
/**
 * Define a string constant.
 *
 * @param cc Compiler context.
 * @param name Name of the constant.
 * @param value Value of the constant.
 * @return Defined symbol, null on error.
 */
symn ccDefStr(ccContext cc, const char *name, const char *value);

/**
 * Define a variable or reference.
 *
 * @param cc Compiler context.
 * @param name Name of the variable.
 * @param type Type of the variable.
 * @return Defined symbol, null on error.
 */
symn ccDefVar(ccContext cc, const char *name, symn type);

/**
 * Define a new type.
 *
 * @param cc Compiler context.
 * @param name Name of the type.
 * @param size Size of the type.
 * @param refType Value or Reference type.
 * @return Defined symbol, null on error.
 * @see plugins/file.c
 */
symn ccAddType(ccContext cc, const char *name, unsigned size, int refType);

/**
 * Define a native function.
 *
 * @param cc Compiler context.
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
symn ccAddCall(ccContext cc, vmError call(nfcContext), const char *proto);

/**
 * Compile the given file or text as a unit.
 *
 * @param cc Compiler context.
 * @param init function installing types and native functions.
 * @param file File name of input.
 * @param line First line of input.
 * @param text If not null, this will be compiled instead of the file.
 * @return Root node of the compilation unit.
 */
astn ccAddUnit(ccContext cc, const char *file, int line, char *text);

/**
 * Generate bytecode from the compiled syntax tree.
 *
 * @param cc Compiler context.
 * @param debug generate debug info
 * @return error code/count, 0 on success.
 */
int ccGenCode(ccContext cc, int debug);

/**
 * Lookup a symbol by name.
 *
 * @param rt Runtime context.
 * @param scope search in the members of this symbol.
 * @param name name of the symbol to be found.
 * @return the first found symbol.
 */
symn ccLookup(rtContext rt, symn scope, char *name);

/// same as castOf forcing arrays to cast as reference
static inline ccKind refCast(symn sym) {
	ccKind got = castOf(sym);
	if (got == CAST_arr && isArrayType(sym)) {
		symn len = sym->fields;
		if (len == NULL || isStatic(len)) {
			// pointer or fixed size array
			got = CAST_ref;
		}
	}
	if (got == CAST_enm && isEnumType(sym)) {
		got = refCast(sym->type);
	}
	return got;
}

static inline size_t refSize(symn sym) {
	switch (refCast(sym)) {
		default:
			return sym->size;

		case CAST_ref:
			return sizeof(vmOffs);

		case CAST_arr:
		case CAST_var:
			return 2 * sizeof(vmOffs);
	}
}

/**
 * check if the given filter excludes the given symbol or not
 * isFiltered(staticVariable, KIND_var | ATTR_stat) == 0
 * isFiltered(localVariable, KIND_var | ATTR_stat) != 0
 * @param sym the symbol to be checked
 * @param filter the filter to be applied
 * @return 0 if is not excluded
 */
static inline int isFiltered(symn sym, ccKind filter) {
	ccKind filterStat = filter & ATTR_stat;
	if (filterStat != 0 && (sym->kind & ATTR_stat) != filterStat) {
		return 1;
	}
	ccKind filterCnst = filter & ATTR_cnst;
	if (filterCnst != 0 && (sym->kind & ATTR_cnst) != filterCnst) {
		return 1;
	}
	ccKind filterKind = filter & MASK_kind;
	if (filterKind != 0 && (sym->kind & MASK_kind) != filterKind) {
		return 1;
	}
	ccKind filterCast = filter & MASK_cast;
	if (filterCast != 0 && (sym->kind & MASK_cast) != filterCast) {
		return 1;
	}
	return 0;
}

/**
 * Check if the given symbol is extending the object type
 */
int isObjectType(symn sym);

#ifdef __cplusplus
}
#endif
#endif
