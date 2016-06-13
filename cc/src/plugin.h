/**
 * Plugin sources should include only this header file.
 */

#ifndef CC_API_H
#define CC_API_H 2

#include <stdio.h> // FILE
#include <string.h> // memcpy

#ifdef _MSC_VER
typedef signed char			int8_t;
typedef signed short		int16_t;
typedef signed long			int32_t;
typedef signed long long	int64_t;
typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;
#define inline __inline
#else
#include <stdint.h>
#include <stddef.h>

#endif
typedef float float32_t;
typedef double float64_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct symNode *symn;					// symbol
typedef struct rtContextRec *rtContext;			// runtimeContext
typedef struct ccContextRec *ccContext; 		// compilerContext
typedef struct dbgContextRec *dbgContext;		// debuggerContext
typedef struct libcContextRec *libcContext;		// nativeCallContext
typedef struct userContextRec *userContext;		// customUserContext

/**
 * @brief Runtime error codes
 */
typedef enum {
	noError,
	invalidIP,
	invalidSP,
	stackOverflow,
	memReadError,
	memWriteError,
	divisionByZero,
	illegalInstruction,
	libCallAbort,
	executionAborted		// execution aborted by debuger
	//+ ArrayBoundsExceeded
} vmError;


/**
 * @brief Runtime context
 */
struct rtContextRec {
	int32_t errCount;		// error count
	int32_t padFlags: 23;
	// disable code generation optimization
	int32_t foldConst: 1;	// fold constant expressions (3 + 4 => 7)
	int32_t fastInstr: 1;	// replace some instructions with a faster or shorter version (load 1, add => inc 1)
	int32_t fastAssign: 1;	// remove dup and set instructions when modifying the last declared variable.
	int32_t genGlobals: 1;	// generate global variables as static variables

	uint32_t logLevel: 3;	// runtime logging level (0-7)
	uint32_t logClose: 1;	// close log file
	uint32_t freeMem: 1;	// release memory
	FILE *logFile;			// log file

	symn main;		// the main initializer function

	/**
	 * @brief Compiler context.
	 * @note After code generation is seted to null.
	 */
	ccContext cc;

	/**
	 * @brief Debug context.
	 * this holds:
	 *  * debugger function
	 *  * code line mapping
	 *  * break point lists
	 */
	dbgContext dbg;

	// virtual machine state
	struct {
		void* libv;		// libcall vector
		void* cell;		// execution unit(s)
		void* heap;		// heap memory

		size_t pc;			// exec: entry point / cgen: prev program counter
		size_t px;			// exec: exit point / cgen: program counter

		size_t ro;			// exec: read only memory / cgen: function parameters
		size_t ss;			// exec: stack size / cgen: stack size

		size_t sm;			// exec: - / cgen: minimum stack size
		size_t su;			// exec: - / cgen: stack access (parallel processing)
	} vm;

	/** TODO: extract to a different struct
	 * @brief External library API support.
	 */
	struct {
		/// Begin a namespace; @see Core#ccBegin
		symn (*const ccBegin)(rtContext, const char* name);
		/// Close a namespace; @see Core#ccEnd
		void (*const ccEnd)(rtContext, symn cls);

		/// Declare int constant; @see Core#ccDefInt
		symn (*const ccDefInt)(rtContext, const char* name, int64_t value);
		/// Declare float constant; @see Core#ccDefFlt
		symn (*const ccDefFlt)(rtContext, const char* name, float64_t value);
		/// Declare string constant; @see Core#ccDefStr
		symn (*const ccDefStr)(rtContext, const char* name, char* value);

		/// Declare a typename; @see Core#ccDefType
		symn (*const ccDefType)(rtContext, const char* name, unsigned size, int refType);
		/// Declare a native function; @see Core#ccDefCall
		symn (*const ccDefCall)(rtContext, vmError libc(libcContext), void* extra, const char* proto);

		/// Compile file or text; @see Core#ccAddUnit
		int (*const ccAddUnit)(rtContext, int warn, char *file, int line, char *code);

		/// Lookup symbol by offset; @see Core#rtFindSym
		symn (*const rtFindSym)(rtContext, void *ptr);

		/// Invoke a function; @see Core#invoke
		vmError (*const invoke)(rtContext, symn fun, void* res, void* args, void* extra);

		/// Alloc, resize or free memory; @see Core#rtAlloc
		void* (*const rtAlloc)(rtContext, void* ptr, size_t size);
	} api;

	// memory related
	const size_t _size;			// size of available memory
	unsigned char *_beg;		// permanent memory (increments)
	unsigned char *_end;		// temporary memory (decrements)
	unsigned char _mem[];		// this is where the memory begins.
};

/**
 * @brief Native function invocation arguments.
 * @note Setting the return value may invalidate some of the arguments.
 * (The return value and the arguments begin at the same memory location.)
 */
struct libcContextRec {
	const rtContext rt;     // runtime context
	void* const extra;      // extra data for function (passed to execute or invoke)
	void* const data;       // static data for function (passed to install)
	char* const argv;       // arguments for function
	void* const retv;       // result of function
};

/**
 * @brief Get the value of a libcall argument.
 * @param args Libcall arguments context.
 * @param result Optionally copy here the result.
 * @param offset Relative offset of argument.
 * @param size Size of the argument to copy to result.
 * @return Pointer where the result is located or copied.
 */
static inline void* argget(libcContext args, void *result, size_t offset, size_t size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, args->argv + offset, size);
	}
	else {
		result = (void*)(args->argv + offset);
	}
	return result;
}

// speed up of getting arguments of known types.
#define argget(__ARGV, __OFFS, __TYPE) (*(__TYPE*)((__ARGV)->argv + (__OFFS)))
static inline int32_t argi32(libcContext args, int offs) { return argget(args, offs, int32_t); }
static inline int64_t argi64(libcContext args, int offs) { return argget(args, offs, int64_t); }
static inline float32_t argf32(libcContext args, int offs) { return argget(args, offs, float32_t); }
static inline float64_t argf64(libcContext args, int offs) { return argget(args, offs, float64_t); }
static inline void* arghnd(libcContext args, int offs) { return argget(args, offs, void*); }
static inline void* argref(libcContext args, int offs) { int32_t p = argget(args, offs, int32_t); return p ? args->rt->_mem + p : NULL; }
// TODO: remove argsym
static inline void* argsym(libcContext args, int offs) { return args->rt->api.rtFindSym(args->rt, argref(args, offs)); }
#undef argget

/**
 * @brief Set the return value of a wrapped native call.
 * @param args arguments context.
 * @param result Pointer containing the result value.
 * @param size Size of the argument to copy to result.
 * @return Pointer where the result is located or copied.
 * @note May invalidate some of the arguments.
 */
static inline void* retset(libcContext args, void *result, size_t size) {
	if (result != NULL) {
		memcpy(args->retv, result, size);
	}
	return args->retv;
}

// speed up of setting result of known types.
#define retset(__ARGV, __TYPE, __VAL) (*(__TYPE*)(__ARGV)->retv = (__TYPE)(__VAL))
static inline void reti32(libcContext args, int32_t val) { retset(args, int32_t, val); }
static inline void reti64(libcContext args, int64_t val) { retset(args, int64_t, val); }
static inline void retf32(libcContext args, float32_t val) { retset(args, float32_t, val); }
static inline void retf64(libcContext args, float64_t val) { retset(args, float64_t, val); }
static inline void rethnd(libcContext args, void* val) { retset(args, void*, val); }
#undef retset

#ifdef __cplusplus
}
#endif
#endif
