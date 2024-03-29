/**
 * Cmpl - A very "simple" C like language.
 * Extension libraries should use only this header file.
 */

#ifndef CMPL_API_H
#define CMPL_API_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined(__WATCOMC__) && defined(_WIN32)
typedef signed ssize_t;
#endif

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define inline __inline
#define PATH_MAX 1024

// disable warning messages
#pragma warning(disable: 4996) // warning C4996: 'read': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _read. See online help for details.
#pragma warning(disable: 4146) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable: 4244) // warning C4244: 'initializing': conversion from 'int64_t' to 'size_t', possible loss of data
#pragma warning(disable: 4018) // warning C4018: '>': signed/unsigned mismatch

#define exported extern __declspec(dllexport)
#else
#define exported extern
#endif

typedef float float32_t;
typedef double float64_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t vmOffs;                        // offset TODO: 32/64 bit offsets support
typedef struct symNode *symn;                   // symbol node
typedef struct astNode *astn;                   // syntax tree
typedef struct dbgNode *dbgn;                   // debug node
typedef struct rtContextRec *rtContext;         // runtimeContext
typedef struct ccContextRec *ccContext;         // compilerContext
typedef struct dbgContextRec *dbgContext;       // debuggerContext
typedef struct nfcContextRec *nfcContext;       // nativeCallContext
typedef struct userContextRec *userContext;     // customUserContext
typedef struct nfcArgs *nfcArgs;

/**
 * specify the extension file to be compiled with this plugin.
 *
 * @see cmplFile module:
 * const char cmplUnit[] = "/cmplStd/lib/system/File.ci";
 */
exported const char cmplUnit[];

/**
 * main entry point of the module plugin.
 *
 * @param rt Runtime context.
 * @return error code in case of failure, 0 on success
 *
 * @see cmplFile module
 */
exported int cmplInit(rtContext rt);

/**
 * method called when the the plugin is disposed (on app exits).
 *
 * @param rt Runtime context.
 */
exported void cmplClose(rtContext rt);

typedef enum {
	raiseFatal = -2,
	raiseError = -1,
	raisePrint = 0,
	raiseWarn = 1,          // treat these warnings as errors

	raise_warn_lex2 = 2,    // WARN_<VALUE>_OVERFLOW
	raise_warn_lex3 = 3,    // WARN_MULTI_CHAR_CONSTANT
	raise_warn_lex9 = 9,    // WARN_NO_NEW_LINE_AT_END / WARN_COMMENT_MULTI_LINE / WARN_COMMENT_NESTED

	raise_warn_par8 = 8,    // empty statement / use block statement
	raise_warn_pad6 = 6,    // WARN_PADDING_ALIGNMENT
	raise_warn_gen8 = 8,    // WARN_NO_CODE_GENERATED
	raise_warn_var8 = 8,    // UNINITIALIZED_VARIABLE

	raise_warn_typ2 = 2,    // WARN_STATIC_FIELD_ACCESS
	raise_warn_gen2 = 2,    // WARN_VARIABLE_TYPE_INCOMPLETE
	raise_warn_typ3 = 3,    // WARN_USING_BEST_OVERLOAD / PASS_ARG_BY_REFERENCE
	raise_warn_typ4 = 4,    // WARN_USING_SIGNED_CAST / UNINITIALIZED_ARRAY
	raise_warn_typ6 = 6,    // WARN_ADDING_IMPLICIT_CAST / WARN_USING_DEF_TYPE_INITIALIZER
	raise_warn_redef = 6,   // WARN_DECLARATION_REDEFINED
	raiseInfo = 13,
	raiseDebug = 14,
	raiseVerbose = 15,
} raiseLevel;

/**
 * Runtime error codes.
 *
 * one of these errors is returned by the bytecode execution.
 */
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

/// Symbol node: types and variables
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
	CAST_ref   = 0x0009,		// reference, pointer, array(c-like)
	CAST_val   = 0x000a,		// value, record, array(fixed)
	CAST_arr   = 0x000b,		// slice: pair of {size, data}
	CAST_var   = 0x000c,		// variant: pair of {type, data}
	//CAST_d   = 0x000d,		// dictionary
	CAST_enm   = 0x000e,		// enumeration
	//CAST_f   = 0x000f,		// unused(function)

	KIND_def   = 0x0000,		// alias (/ error at runtime)
	KIND_typ   = 0x0010,		// typename: struct metadata info.
	KIND_fun   = 0x0020,		// function: executable byte-code.
	KIND_var   = 0x0030,		// variable: function and typename are also variables

	ATTR_stat  = 0x0100,        // static attribute (not dynamic, usually computed at compile time)
	ATTR_cnst  = 0x0200,        // constant attribute (not mutable), (value, not a variable) can't be changed after initialization

	ARGS_inln  = 0x1000,        // inline argument
	ARGS_varg  = 0x2000,        // variable argument
	ARGS_this  = 0x4000,        // first argument is this (used at lookup)

	MASK_cast  = 0x000f,
	MASK_kind  = 0x0030,
	MASK_attr  = 0x0f00,
} ccKind;

/**
 * Runtime context.
 */
struct rtContextRec {
	unsigned foldConst: 1;  // fold constant expressions (3 + 4 => 7)
	unsigned foldCasts: 1;  // fold cast expressions (float(int(3.2)) => 3.f)
	unsigned foldInstr: 1;  // replace instructions with a faster or shorter version (`load 1`, `add.i32` with `inc 1`)
	unsigned foldMemory: 1; // replace `load address`, `load indirect` with `dup`, `set`, `load` or `store` instruction
	unsigned foldAssign: 1; // replace `dup x`, `set y` instructions with `move x, y` instruction
	unsigned logLevel: 4;   // runtime logging level (0-15)
	unsigned traceLevel: 8; // runtime backtrace level (0-255)
	unsigned closeLog: 1;   // close log file
	unsigned freeMem: 1;    // release memory

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

		size_t pc;         // exec: entry point / cgen: program counter
		size_t px;         // exec: exit point / cgen: max jump offset

		size_t ro;         // size of read only memory
		size_t cs;         // size of code (functions)
		size_t ds;         // size of data (static var)
		size_t ss;         // size of stack
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
		astn (*const ccAddUnit)(ccContext cc, char *file, int line, const char *text);

		/// Lookup function by name; @see ccLookup
		symn (*const ccLookup)(rtContext ctx, symn scope, const char *name);

		void (*const raise)(rtContext ctx, raiseLevel level, const char *msg, ...);

		/// Lookup function by offset; @see rtLookup
		symn (*const rtLookup)(rtContext ctx, size_t offset, ccKind filter);

		/// Invoke a function; @see invoke
		vmError (*const invoke)(rtContext ctx, symn fun, void *res, void *args, const void *extra);

		/// Alloc, resize or free memory; @see rtAlloc
		void *(*const rtAlloc)(rtContext ctx, void *ptr, size_t size);

		/// Reset to the first argument inside a native function
		struct nfcArgs (*const nfcArgs)(nfcContext ctx);

		/// Advance to the next argument inside a native function
		nfcArgs (*const nextArg)(nfcArgs ctx);
	} api;

	/**
	 * Main initializer function.
	 *
	 * This function initializes global variables.
	 */
	symn main;

	// memory management for the compiler and code generator
	unsigned char *_beg;        // permanent memory (increments)
	unsigned char *_end;        // temporary memory (decrements)

	// memory available for the compiler and runtime
	const size_t _size;         // size of available memory
	unsigned char _mem[];       // this is where the memory begins.
};

struct symNode {
	const char *name;   // symbol name
	const char *unit;   // declared in unit
	const char *file;   // declared in file
	const char *doc;    // document comment
	const char *fmt;    // print format
	int32_t line;       // declared on line
	int32_t nest;       // declared on scope level
	size_t size;        // variable or function size
	size_t offs;        // address of variable or function

	symn next;          // next symbol: field / param / ... / (in scope table)
	symn type;          // base type of array / typename (void, int, float, struct, function, ...)
	symn scope;         // global variables and functions / while_compiling variables of the block in reverse order

	symn owner;         // the type that declares the current symbol (DeclaringType)
	symn fields;        // all fields: static + non-static
	symn params;        // function parameters, return value is the first parameter.

	astn init;          // VAR init / FUN body, this should be null after code generation?

	ccKind kind;        // KIND_def / KIND_typ / KIND_fun / KIND_var + ATTR_xxx + CAST_xxx

	// TODO: merge scope and global attributes
	symn override;      // function overrides this other function
	symn global;        // all static variables and functions
	astn use;           // TEMP: usages
	astn tag;           // TEMP: declaration reference
};

static inline int isConst(symn sym) {
	return (sym->kind & ATTR_cnst) != 0;
}
static inline int isStatic(symn sym) {
	return (sym->kind & ATTR_stat) != 0;
}
static inline int isInline(symn sym) {
	return (sym->kind & MASK_kind) == KIND_def;
}
static inline int isFunction(symn sym) {
	return (sym->kind & MASK_kind) == KIND_fun;
}
static inline int isVariable(symn sym) {
	return (sym->kind & MASK_kind) == KIND_var;
}
static inline int isTypename(symn sym) {
	return (sym->kind & MASK_kind) == KIND_typ;
}
static inline int isEnumType(symn sym) {
	return (sym->kind & (MASK_kind | MASK_cast)) == (KIND_typ | CAST_enm);
}
static inline int isArrayType(symn sym) {
	return (sym->kind & (MASK_kind | MASK_cast)) == (KIND_typ | CAST_arr);
}
static inline int isFixedArray(symn sym) {
	if (!isArrayType(sym->type)) {
		return 0;
	}
	return (sym->kind & (MASK_kind | MASK_cast)) == (KIND_var | CAST_val);
}
static inline int byReference(symn sym) {
	return (sym->kind & MASK_cast) == CAST_ref;
}
static inline int isInvokable(symn sym) {
	return sym->params != NULL;
}
static inline int isMember(symn sym) {
	if (sym->owner == NULL) {
		// global variable
		return 0;
	}
	return isTypename(sym->owner);
}
static inline ccKind castOf(symn sym) {
	return sym->kind & MASK_cast;
}

/**
 * Native function invocation context.
 */
struct nfcContextRec {
	const rtContext rt;         // runtime context
	const symn sym;             // invoked function (returned by ccAddCall)
	const char *proto;          // static data (passed to ccAddCall)
	const void *extra;          // extra data (passed to execute or invoke)
	const void *argv;           // arguments
	const size_t argc;          // argument count in bytes
};

/**
 * Get the pointer to an internal offset inside the vm.
 *
 * @param rt Runtime context.
 * @param offset global offset inside the vm.
 * @return pointer to the memory.
 */
static inline void *vmPointer(rtContext rt, size_t offset) {
	if (offset == 0 || offset >= rt->_size) {
		return NULL;
	}
	return rt->_mem + offset;
}

/**
 * Get the internal offset inside the vm of a reference.
 *
 * @param rt Runtime context.
 * @param ptr Memory location.
 * @return The internal offset.
 * @note Aborts if ptr is not null and outside the vm memory.
 */
static inline size_t vmOffset(rtContext rt, const void *ptr) {
	if (ptr == NULL) {
		return 0;
	}
	size_t offset = (unsigned char*)ptr - rt->_mem;
	if (offset >= rt->_size) {
		abort();
	}
	return offset;
}

/**
 * Get the value of a native function argument.
 *
 * @param args Native function call arguments context.
 * @param offset Relative offset of argument.
 * @return Pointer where the result is located.
 * @note Getting an argument must be used with padded offset.
 */
static inline void *argget(nfcContext args, size_t offset) {
	return (char *) args->argv + offset;
}

// speed up of getting arguments of known types
static inline int32_t argi32(nfcContext ctx, size_t offs) { return *(int32_t *) argget(ctx, offs); }
static inline int64_t argi64(nfcContext ctx, size_t offs) { return *(int64_t *) argget(ctx, offs); }
static inline uint32_t argu32(nfcContext ctx, size_t offs) { return *(uint32_t *) argget(ctx, offs); }
static inline uint64_t argu64(nfcContext ctx, size_t offs) { return *(uint64_t *) argget(ctx, offs); }
static inline float32_t argf32(nfcContext ctx, size_t offs) { return *(float32_t *) argget(ctx, offs); }
static inline float64_t argf64(nfcContext ctx, size_t offs) { return *(float64_t *) argget(ctx, offs); }
static inline void *arghnd(nfcContext ctx, size_t offs) { return *(void **) argget(ctx, offs); }
static inline size_t argref(nfcContext ctx, size_t offs) { return *(vmOffs *) argget(ctx, offs); }

/**
 * Set the return value of a wrapped native call.
 *
 * @param args arguments context.
 * @param result Pointer containing the result value.
 * @param size Size of the argument to copy to result.
 * @return Pointer where the result is located or copied.
 * @note Setting the return value may overwrite some arguments.
 */
static inline void *retset(nfcContext args, void *result, size_t size) {
	char *ptr = (char *) args->argv + args->argc - size;
	if (result != NULL && size > 0) {
		memcpy(ptr, result, size);
		return result;
	}
	return ptr;
}

// speed up of setting result of known types.
#define retset(__ARGV, __TYPE, __VAL) ((__TYPE*)((char *)(__ARGV)->argv + (__ARGV)->argc))[-1] = (__TYPE)(__VAL)
static inline void reti32(nfcContext ctx, int32_t val) { retset(ctx, int32_t, val); }
static inline void reti64(nfcContext ctx, int64_t val) { retset(ctx, int64_t, val); }
static inline void retu32(nfcContext ctx, uint32_t val) { retset(ctx, uint32_t, val); }
static inline void retu64(nfcContext ctx, uint64_t val) { retset(ctx, uint64_t, val); }
static inline void retf32(nfcContext ctx, float32_t val) { retset(ctx, float32_t, val); }
static inline void retf64(nfcContext ctx, float64_t val) { retset(ctx, float64_t, val); }
static inline void retref(nfcContext ctx, size_t val) { retset(ctx, vmOffs, val); }
static inline void rethnd(nfcContext ctx, void *val) { retset(ctx, char *, val); }
#undef retset

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

#ifdef __cplusplus
}
#endif
#endif
