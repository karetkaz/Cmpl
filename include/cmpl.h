/**
 * Cmpl - A very "simple" C like language.
 *
 * Extension libraries should use only this header file as the main interface
 * for integrating with the Cmpl language into a dynamically linked library:
 *    - Windows: .dll   (example: cmplFile.dll)
 *    - Linux:   .so    (example: cmplFile.so)
 *    - macOS:   .dylib (example: cmplFile.dylib)
 * exported functions and symbols by extensions can be:
 *    - cmplInit
 *    - cmplUnit
 *    - cmplClose
 *
 * @see cmplFile module
 *
 */
#ifndef CMPL_API_H
#define CMPL_API_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__WATCOMC__) && defined(_WIN32)
typedef signed ssize_t;
#endif

typedef float float32_t;
typedef double float64_t;

typedef uint32_t vmOffs;                        // offset TODO: 32 / 64 bit offsets support
typedef struct symNode *symn;                   // symbol node
typedef struct astNode *astn;                   // syntax tree
typedef struct nfcArgs *nfcArgs;                // argument list iterator
typedef struct rtContextRec *rtContext;         // Runtime context
typedef struct ccContextRec *ccContext;         // Compiler context
typedef struct dbgContextRec *dbgContext;       // Debugger context
typedef struct nfcContextRec *nfcContext;       // Native call context
typedef struct userContextRec *userContext;     // Custom user context

/**
 * specify the extension file to be compiled with this plugin.
 *
 * @see cmplFile module
 */
extern const char cmplUnit[];

/**
 * The main entry point of the module plugin.
 *
 * @param ctx runtime context.
 * @param cc compiler context.
 * @param cc The compiler context to which libraries and extensions are added.
 * @return An integer indicating success (0) or failure (non-zero) to initialize the contexts.
 */
extern int cmplInit(rtContext ctx, ccContext cc);

/**
 * method called when the plugin is disposed (on app exit).
 *
 * @param ctx runtime context.
 */
extern void cmplClose(rtContext ctx);

typedef enum {
	raiseFatal = -2,
	raiseError = -1,
	raisePrint = 0,
	raiseWarn = 1,
	raiseInfo = 13,
	raiseDebug = 14,
	raiseVerbose = 15,
} raiseLevel;

/// Execution and native function error codes.
typedef enum {
	noError,
	illegalState,
	stackOverflow,
	divisionByZero,
	illegalInstruction,
	illegalMemoryAccess,
	//+ illegalArrayAccess, => TODO: array index out of bounds
	nativeCallError,
} vmError;

/// Symbol node kind and modifiers
typedef enum {
	CAST_any   = 0x0000,        // error
	CAST_vid   = 0x0001,        // void
	CAST_bit   = 0x0002,        // bool
	CAST_i32   = 0x0003,        // int32, int16, int8
	CAST_u32   = 0x0004,        // uint32, uint16, uint8
	CAST_i64   = 0x0005,        // int64
	CAST_u64   = 0x0006,        // uint64
	CAST_f32   = 0x0007,        // float32
	CAST_f64   = 0x0008,        // float64
	CAST_val   = 0x0009,        // value, record, array(fixed)
	CAST_ptr   = 0x000a,        // pointer, reference, array(c-like)
	CAST_obj   = 0x000b,        // object, reference counted, garbage collected
	CAST_var   = 0x000c,        // variant: a pair of {type, data}
	CAST_arr   = 0x000d,        // array: a pair of {size, data}
	CAST_map   = 0x000e,        // dictionary
	CAST_enm   = 0x000f,        // enumeration

	KIND_def   = 0x0000,        // alias (/ error at runtime)
	KIND_typ   = 0x0010,        // typename: struct metadata info.
	KIND_fun   = 0x0020,        // function: executable byte-code.
	KIND_var   = 0x0030,        // variable: function and typename are also variables

	// todo: variance: https://flow.org/en/docs/lang/variance
	//ATTR_inv = 0x0000,        // Invariant - cannot assign super or subtypes
	//ATTR_cov = 0x0040,        // Covariant - can assign to subtypes: object o = (string) s;
	//ATTR_ctv = 0x0080,        // Contravariant - can assign to supertypes: string s = (object) o;
	//ATTR_biv = 0x00c0,        // Bivariant - can assign to anything in the hierarchy tree

	ATTR_stat  = 0x0100,        // static attribute (not dynamic, usually computed at compile time)

	// variables should be pure by default (value, constant, immutable) should not change after initialization, no side effect for function
	ATTR_mut   = 0x0200,        // `mutable` (variable): int var = 0; var += 2;
	ATTR_opt   = 0x0400,        // `optional` (pass by reference): int nullable? = null;

	MASK_cast  = 0x000f,
	MASK_kind  = 0x0030,
	MASK_attr  = 0x0f00,

	// todo: remove
	ARGS_inln  = 0x1000,            // inline argument
	ARGS_varg  = 0x2000,            // variable argument
	ARGS_this  = 0x4000,            // first argument is this (used at lookup)
} ccKind;

/// Symbol node: types, variables and functions
struct symNode {
	const char *name;   // symbol name
	const char *unit;   // declared in unit
	const char *file;   // declared in file
	const char *doc;    // document comment
	const char *fmt;    // print format
	int32_t line;       // declaration line
	int32_t nest;       // declared on scope level
	size_t size;        // variable or function size
	size_t offs;        // address of variable or function

	symn next;          // next symbol: field / param / ... / (in scope table)
	symn type;          // base type of array / typename (void, int, float, struct, function, ...)
	symn scope;         // global variables and functions / while_compiling variables of the block in reverse order

	symn owner;         // the type that declares the current symbol (DeclaringType)
	symn fields;        // all fields: static and non-static
	symn params;        // function parameters, return value is the first parameter.

	astn init;          // function body, variable initializer, this should be null after code was generated

	ccKind kind;        // KIND_def / KIND_typ / KIND_fun / KIND_var + ATTR_xxx + CAST_xxx

	// TODO: merge scope and global attributes
	symn override;      // function overrides this other function
	symn global;        // all static variables and functions
	astn use;           // TEMP: usages
	astn tag;           // TEMP: declaration reference
};

/// Runtime context.
struct rtContextRec {
	unsigned freeMem: 1;    // release memory
	unsigned closeLog: 1;   // close the log file
	unsigned logLevel: 4;   // runtime logging level (0-15)

	unsigned foldConst: 1;  // fold constant expressions (3 + 4 => 7)
	unsigned foldCasts: 1;  // fold cast expressions (float(int(3.2)) => 3.f)
	unsigned foldInstr: 1;  // replace instructions with a faster or shorter version (`load 1`, `add.i32` with `inc 1`)
	unsigned foldInstr2: 1;  // fold mul, div, mod expressions to bitwise when possible: x * 4 => x << 2
	unsigned foldMemory: 1; // replace `load address`, `load indirect` with `dup`, `set`, `load` or `store` instruction
	unsigned foldAssign: 1; // replace `dup x`, `set y` instructions with `move x, y` instruction

	int32_t errors;         // error count
	FILE *logFile;          // log file

	/**
	 * Compiler context.
	 *
	 * @note After code generation it is set to null.
	 */
	ccContext cc;

	/**
	 * Debugger context.
	 *
	 * contains:
	 *  * profiler function
	 *  * debugger function
	 *  * code line mapping
	 *  * break point lists
	 */
	dbgContext dbg;

	/**
	 * User context.
	 *
	 * contains custom context set by library
	 */
	userContext usr;

	// virtual machine state
	struct {
		void *nfc;         // native call vector
		void *cell;        // execution units
		void *heap;        // heap memory

		size_t pc;         // exec: entry point / gen: program counter
		size_t px;         // exec: exit point / gen: max jump offset

		size_t ro;         // size of read-only memory (<= pc)
		size_t cs;         // size of code (functions)
		size_t ds;         // size of data (static var)
		size_t ss;         // exec: stack size / gen: max fun locals
	} vm;

	/**
	 * Extension support api.
	 *
	 * These functions can be used in extension libraries (dll or so)
	 */
	struct {
		/// Begin a namespace; @see ccBegin
		symn (*const ccBegin)(ccContext ctx, const char *name);

		/// Extend a namespace; @see ccExtend
		symn (*const ccExtend)(ccContext ctx, symn sym);

		/// Close a namespace; @see ccEnd
		symn (*const ccEnd)(ccContext ctx, symn sym);

		/// Declare int constant; @see ccDefInt
		symn (*const ccDefInt)(ccContext ctx, const char *name, int64_t value);

		/// Declare float constant; @see ccDefFlt
		symn (*const ccDefFlt)(ccContext ctx, const char *name, float64_t value);

		/// Declare string constant; @see ccDefStr
		symn (*const ccDefStr)(ccContext ctx, const char *name, char *value);

		/// Declare variable; @see ccDefVar
		symn (*const ccDefVar)(ccContext ctx, const char *name, symn type);

		/// Declare a typename; @see ccAddType
		symn (*const ccAddType)(ccContext ctx, const char *name, unsigned size, int refType);

		/// Declare a native function; @see ccAddCall
		symn (*const ccAddCall)(ccContext ctx, vmError libc(nfcContext), const char *proto);

		/// Compile code snippet; @see ccAddUnit
		astn (*const ccAddUnit)(ccContext ctx, char *file, int line, const char *text);

		/// Lookup function by name; @see ccLookup
		symn (*const ccLookup)(ccContext ctx, symn scope, const char *name);

		/// Alloc, resize or free memory; @see rtAlloc
		void *(*const alloc)(nfcContext ctx, void *ptr, size_t size);

		/// Raise a runtime error, warning or info
		void (*const raise)(nfcContext ctx, raiseLevel level, const char *msg, ...);

		/// Lookup function by offset; @see rtLookup
		symn (*const lookup)(rtContext ctx, size_t offset, ccKind filter);

		/// Invoke a function; @see invoke
		vmError (*const invoke)(nfcContext ctx, symn fun, void *res, void *args, const void *extra);

		/// Advance to the next argument of a native function
		nfcArgs (*const nextArg)(nfcArgs ctx);

		/// Get the iterator to a native function's arguments
		struct nfcArgs (*const nfcArgs)(nfcContext ctx);
	} api;

	/**
	 * Main initializer function.
	 *
	 * This function initializes global variables.
	 */
	symn main;

	// memory available for the compiler and runtime
	const size_t _size;         // size of available memory
	unsigned char *_beg;        // permanent memory (increments)
	unsigned char *_end;        // temporary memory (decrements)
	unsigned char _mem[];       // this is where the memory begins.
};

/// Native function invocation context.
struct nfcContextRec {
	const rtContext rt;         // runtime context
	const symn sym;             // invoked function (returned by ccAddCall)
	const char *proto;          // static data (passed to ccAddCall)
	const void *extra;          // extra data (passed to execute or invoke)
	const void *argv;           // arguments
	const size_t argc;          // argument count in bytes
};

/// Native function argument iterator.
struct nfcArgs {
	nfcContext ctx;
	vmOffs offset;          // the current argument offset
	symn param;             // the current argument
	union {
		int32_t i32;
		int64_t i64;
		uint32_t u32;
		uint64_t u64;
		float32_t f32;
		float64_t f64;
		void *ref;
		struct nfcArgArr {
			void *ref;
			size_t length;
		} arr;
		struct nfcArgVar {
			void *ref;
			symn type;
		} var;
	};
};

static inline ccKind castOf(symn sym) {
	return sym->kind & MASK_cast;
}

static inline int isInline(symn sym) {
	return (sym->kind & MASK_kind) == KIND_def;
}
static inline int isTypename(symn sym) {
	return (sym->kind & MASK_kind) == KIND_typ;
}
static inline int isFunction(symn sym) {
	return (sym->kind & MASK_kind) == KIND_fun;
}
static inline int isVariable(symn sym) {
	return (sym->kind & MASK_kind) == KIND_var;
}
static inline int isInvokable(symn sym) {
	return sym->params != NULL;
}

static inline int isStatic(symn sym) {
	return (sym->kind & ATTR_stat) != 0;
}
static inline int isMutable(symn sym) {
	return (sym->kind & ATTR_mut) != 0;
}
static inline int isOptional(symn sym) {
	return (sym->kind & ATTR_opt) != 0;
}
static inline int isIndirect(symn sym) {
	switch (castOf(sym)) {
		case CAST_ptr:
			return CAST_ptr;

		case CAST_obj:
			return CAST_obj;

		default:
			return 0;
	}
}

static inline int isEnumType(symn sym) {
	/// Check if the given symbol is an enumeration type
	return (sym->kind & (MASK_kind | MASK_cast)) == (KIND_typ | CAST_enm);
}
static inline int isObjectType(symn sym) {
	/// Check if the given symbol is extending the object type
	return (sym->kind & (MASK_kind | MASK_cast)) == (KIND_typ | CAST_obj);
}
static inline int isArrayType(symn sym) {
	return (sym->kind & (MASK_kind | MASK_cast)) == (KIND_typ | CAST_arr);
}
static inline int isUnknownSizeArray(symn sym) {
	if (!isArrayType(sym)) {
		return 0;
	}
	return sym->fields == NULL;
}
static inline int isDynamicSizeArray(symn sym) {
	if (!isArrayType(sym)) {
		return 0;
	}
	return sym->fields != NULL && !isStatic(sym->fields);
}
static inline int isFixedSizeArray(symn sym) {
	if (!isArrayType(sym)) {
		return 0;
	}
	return sym->fields != NULL && isStatic(sym->fields);
}

/**
 * Get the pointer to an internal offset inside the vm.
 *
 * @param ctx runtime context.
 * @param offset global offset inside the vm.
 * @return pointer to the memory.
 */
static inline void *vmPointer(rtContext ctx, size_t offset) {
	if (offset == 0 || offset >= ctx->_size) {
		return NULL;
	}
	return ctx->_mem + offset;
}

/**
 * Get the internal offset inside the vm of a reference.
 *
 * @param ctx runtime context.
 * @param ptr Memory location.
 * @return The internal offset.
 * @note Aborts if ptr is not null and outside the vm memory.
 */
static inline size_t vmOffset(rtContext ctx, const void *ptr) {
	if (ptr == NULL) {
		return 0;
	}
	size_t offset = (unsigned char*)ptr - ctx->_mem;
	if (offset >= ctx->_size) {
		abort();
	}
	return offset;
}

/**
 * Get the value of a native function argument.
 *
 * @param ctx execution context.
 * @param offset Relative offset of argument.
 * @return Pointer where the result is located.
 * @note Getting an argument must be used with padded offset.
 */
static inline void *getArg(const nfcContext ctx, size_t offset) {
	return (char *) ctx->argv + offset;
}

// speed up of getting arguments of known types.
static inline int32_t argI32(nfcContext ctx, size_t offs) { return *(int32_t *) getArg(ctx, offs); }
static inline int64_t argI64(nfcContext ctx, size_t offs) { return *(int64_t *) getArg(ctx, offs); }
static inline uint32_t argU32(nfcContext ctx, size_t offs) { return *(uint32_t *) getArg(ctx, offs); }
static inline uint64_t argU64(nfcContext ctx, size_t offs) { return *(uint64_t *) getArg(ctx, offs); }
static inline float32_t argF32(nfcContext ctx, size_t offs) { return *(float32_t *) getArg(ctx, offs); }
static inline float64_t argF64(nfcContext ctx, size_t offs) { return *(float64_t *) getArg(ctx, offs); }
static inline size_t argRef(nfcContext ctx, size_t offs) { return *(vmOffs *) getArg(ctx, offs); }
static inline void *argHnd(nfcContext ctx, size_t offs) { return *(void **) getArg(ctx, offs); }

/**
 * Set the return value of a wrapped native call.
 *
 * @param ctx execution context.
 * @param result Pointer containing the result value.
 * @return Pointer where the result is located or copied.
 * @note Setting the return value may overwrite some arguments.
 */
static inline void *setRet(nfcContext ctx, const void *result) {
	size_t size = ctx->sym->params->size; // the first param is `result`
	char *ptr = getArg(ctx, ctx->argc - size);
	if (result != NULL && size > 0) {
		memcpy(ptr, result, size);
	}
	return ptr;
}

#define setRet(__ARGV, __TYPE, __VAL) ((__TYPE*)((char *)(__ARGV)->argv + (__ARGV)->argc))[-1] = (__TYPE)(__VAL)
static inline void retI32(nfcContext ctx, int32_t val) { setRet(ctx, int32_t, val); }
static inline void retI64(nfcContext ctx, int64_t val) { setRet(ctx, int64_t, val); }
static inline void retU32(nfcContext ctx, uint32_t val) { setRet(ctx, uint32_t, val); }
static inline void retU64(nfcContext ctx, uint64_t val) { setRet(ctx, uint64_t, val); }
static inline void retF32(nfcContext ctx, float32_t val) { setRet(ctx, float32_t, val); }
static inline void retF64(nfcContext ctx, float64_t val) { setRet(ctx, float64_t, val); }
static inline void retRef(nfcContext ctx, size_t val) { setRet(ctx, vmOffs, val); }
static inline void retHnd(nfcContext ctx, void *val) { setRet(ctx, char *, val); }
#undef setRet

static inline int cmplVersion() {
	int month = (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ? 1 :
				(__DATE__[0] == 'F' && __DATE__[1] == 'e' && __DATE__[2] == 'b') ? 2 :
				(__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ? 3 :
				(__DATE__[0] == 'A' && __DATE__[1] == 'p' && __DATE__[2] == 'r') ? 4 :
				(__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ? 5 :
				(__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ? 6 :
				(__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ? 7 :
				(__DATE__[0] == 'A' && __DATE__[1] == 'u' && __DATE__[2] == 'g') ? 8 :
				(__DATE__[0] == 'S' && __DATE__[1] == 'e' && __DATE__[2] == 'p') ? 9 :
				(__DATE__[0] == 'O' && __DATE__[1] == 'c' && __DATE__[2] == 't') ? 10 :
				(__DATE__[0] == 'N' && __DATE__[1] == 'o' && __DATE__[2] == 'v') ? 11 :
				(__DATE__[0] == 'D' && __DATE__[1] == 'e' && __DATE__[2] == 'c') ? 12 : 0;
	int day = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
	int year = (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
	return (year * 100 + month) * 100 + day;
}

#ifdef __cplusplus
}
#endif
#endif
