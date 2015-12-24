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
typedef struct dbgContextRec *dbgContext;		// debugContext

typedef struct libcContextRec *libcContext;		// libcallContext -> runtimeContext
typedef struct customContextRec *customContext;	// customContext -> debugContext

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
	int32_t  errCount;		// error count
	uint32_t logLevel:3;	// runtime logging level (0-7)
	uint32_t logClose:1;	// close log file
	// disable code generation optimization
	uint32_t genCFold:1;	// do not fold const expressions
	uint32_t genBasic:1;	// do not optimize instructions
	uint32_t genForInc:1;	// do not optimize for expression shortener
	uint32_t genLocal:1;	// generate globals on stack
	uint32_t padFlags:24;
	FILE *logFile;		// log file

	symn  vars;		// global variables and functions
	symn  main;		// the main initializer function

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

	// TODO: remove variant and string typename symbols
	symn type_var;	// TODO: to be removed, used only for printing.
	symn type_str;	// TODO: to be removed, used only for printing.

	/** TODO: extract to a different struct
	 * @brief External library support.
	 * @note If a function returns error, the error was reported.
	 */
	struct {
		/**
		 * @brief Begin a namespace (static struct).
		 * @param Runtime context.
		 * @param name Name of the namespace.
		 * @return Defined symbol, null on error.
		 */
		symn (*const ccBegin)(rtContext, char* name);

		/**
		 * @brief Define a(n) integer, floating point or string constant.
		 * @param Runtime context.
		 * @param name Name of the constant.
		 * @param value Value of the constant.
		 * @return Defined symbol, null on error.
		 */
		symn (*const ccDefInt)(rtContext, char* name, int64_t value);
		symn (*const ccDefFlt)(rtContext, char* name, float64_t value);
		symn (*const ccDefStr)(rtContext, char* name, char* value);

		/**
		 * @brief Add a type to the runtime.
		 * @param Runtime context.
		 * @param name Name of the type.
		 * @param size Size of the type.
		 * @param refType Value indicating ByRef or ByValue.
		 * @return Defined symbol, null on error.
		 * @see: lstd.File;
		 */
		symn (*const ccAddType)(rtContext, const char* name, unsigned size, int refType);

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
			if (!rt->api.ccAddCall(rt, f64sin, NULL, "float64 sin(float64 x);")) {
				// error reported, terminate here the execution.
				return 0;
			}
		 */
		symn (*const ccAddCall)(rtContext, vmError libc(libcContext), void* extra, const char* proto);

		/**
		 * @brief Compile the given file or text block.
		 * @param Runtime context.
		 * @param warn Warning level.
		 * @param file File name of input.
		 * @param line First line of input.
		 * @param text If not null, this will be compiled instead of the file.
		 * @return Boolean value of success.
		 */
		int (*const ccAddCode)(rtContext, int warn, char *file, int line, char *code);

		/**
		 * @brief Close the namespace.
		 * @param Runtime context.
		 * @param cls Namespace to be closed. (The returned by ccBegin.)
		 * @note Makes all declared variables static.
		*/
		void (*const ccEnd)(rtContext, symn cls);

		/* Find a symbol by name.
		 * 
		symn (*const ccSymFind)(ccContext cc, symn in, char *name);
		 */

		/* offset of a pointer in the vm
		 * 
		int (*const vmOffset)(rtContext, void *ptr);
		 */

		/**
		 * @brief Find a static symbol by offset.
		 * @param Runtime context.
		 * @param ptr Pointer to the variable.
		 * @note Usefull for callbacks.
		 * @usage see also: test.gl/gl.c

			static symn onMouse = NULL;

			static int setMouseCb(libcContext rt) {
				void* fun = argref(rt, 0);

				// unregister event callback
				if (fun == NULL) {
					onMouse = NULL;
					return 0;
				}

				// register event callback using the symbol of the function.
				onMouse = rt->api.getsym(rt, fun);

				// runtime error if symbol is not valid.
				return onMouse != NULL;
			}

			static int mouseCb(int btn, int x, int y) {
				if (onMouse != NULL && rt != NULL) {
					// invoke the callback with arguments.
					struct {int32_t btn, x, y;} args = {btn, x, y};
					rt->api.invoke(rt, onMouse, NULL, &args, NULL);
				}
			}

			// expose the callback register function to the compiler
			if (!rt->api.ccAddCall(rt, setMouseCb, NULL, "void setMouseCallback(void Callback(int32 b, int32 x, int32 y);")) {
				error...
			}
		 * TODO: symn (*const vmSymbol)(rtContext, int offset);
		 */
		symn (*const getsym)(rtContext, void *ptr);

		/**
		 * @brief Invoke a function inside the vm.
		 * @param Runtime context.
		 * @param fun Symbol of the function.
		 * @param res Result value of the invoked function. (May be null.)
		 * @param args Arguments for the fuction. (May be null.)
		 * @param extra Extra data for each libcall executed from here.
		 * @return Error code of execution. (0 means success.)
		 * @usage see @getsym example.
		 * @note Invocation to execute must preceed this call.
		 */
		vmError (*const invoke)(rtContext, symn fun, void* res, void* args, void* extra);

		/**
		 * @brief Allocate or free memory inside the vm.
		 * @param Runtime context.
		 * @param ptr Allocated memory address in the vm, or null.
		 * @param size New size to reallocate or 0 to free memory.
		 * @return Pointer to the allocated memory.
		 * cases:
		 * 		size == 0 && ptr == null: nothing
		 * 		size == 0 && ptr != null: free
		 * 		size >  0 && ptr == null: alloc
		 * 		size >  0 && ptr != null: realloc
		 * @note Invocation to execute must preceed this call.
		 */
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
	rtContext rt;		// runtime context

	symn  fun;		// invoked function
	void* data;		// static data for function (passed to install)
	void* extra;	// extra data for function (passed to execute or invoke)

	void* retv;		// result of function
	char* argv;		// arguments for function
};

/**
 * @brief Get the value of a libcall argument.
 * @param args Libcall arguments context.
 * @param offset Relative offset of argument.
 * @param result Optionally copy here the result.
 * @param size Size of the argument to copy to result.
 * @return Pointer where the result is located or copied.
 */
static inline void* argval(libcContext args, size_t offset, void *result, size_t size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, args->argv + offset, size);
	}
	else {
		result = args->argv + offset;
	}
	return result;
}

// speed up of getting arguments of known types.
#define argval(__ARGV, __OFFS, __TYPE) (*(__TYPE*)((__ARGV)->argv + (__OFFS)))
static inline int32_t argi32(libcContext args, int offs) { return argval(args, offs, int32_t); }
static inline int64_t argi64(libcContext args, int offs) { return argval(args, offs, int64_t); }
static inline float32_t argf32(libcContext args, int offs) { return argval(args, offs, float32_t); }
static inline float64_t argf64(libcContext args, int offs) { return argval(args, offs, float64_t); }
static inline void* arghnd(libcContext args, int offs) { return argval(args, offs, void*); }
static inline void* argref(libcContext args, int offs) { int32_t p = argval(args, offs, int32_t); return p ? args->rt->_mem + p : NULL; }
static inline void* argsym(libcContext args, int offs) { return args->rt->api.getsym(args->rt, argref(args, offs)); }
#undef argval

/**
 * @brief Set the return value of a libcall.
 * @param args Libcall arguments context.
 * @param result Pointer containing the result value.
 * @param size Size of the argument to copy to result.
 * @return Pointer where the result is located or copied.
 * @note May invalidate some of the arguments.
 */
static inline void* setret(libcContext args, void *result, size_t size) {
	if (result != NULL) {
		memcpy(args->retv, result, size);
	}
	return args->retv;
}

// speed up of setting result of known types.
#define setret(__ARGV, __TYPE, __VAL) (*(__TYPE*)(__ARGV)->retv = (__TYPE)(__VAL))
static inline void reti32(libcContext args, int32_t val) { setret(args, int32_t, val); }
static inline void reti64(libcContext args, int64_t val) { setret(args, int64_t, val); }
static inline void retf32(libcContext args, float32_t val) { setret(args, float32_t, val); }
static inline void retf64(libcContext args, float64_t val) { setret(args, float64_t, val); }
static inline void rethnd(libcContext args, void* val) { setret(args, void*, val); }
//~ static inline void retref(libcContext args, void* val) { setret(args, void*, vmOffset(args->rt, val)); }
#undef setret

#ifdef __cplusplus
}
#endif
#endif
