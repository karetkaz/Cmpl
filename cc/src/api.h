#ifndef CC_API_H
#define CC_API_H 2
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stddef.h>

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
#endif

typedef float float32_t;
typedef double float64_t;

typedef struct symNode *symn;			// symbol
typedef struct stateRec *state;			// runtime
typedef struct ccStateRec *ccState; 	// compiler
typedef struct dbgStateRec *dbgState;	// debugger

/** Native function invocation arguments
 * NOTE: the return value and the arguments begin at the same memory location.
 * setting the return value may invalidate the arguments.
 * 
 */
typedef struct libcArgsRec {
	state rt;		// runtime context

	symn  fun;		// invoked function
	void* retv;		// result of function
	char* argv;		// arguments for function

	void* data;		// function data (ccAddCall)
	void* extra;	// extra data (ccCall)
} *libcArgs;

/** runtime context
 * 
 */
struct stateRec {
	int   errc;		// error count
	int   closelog;	// close log file
	FILE* logf;		// log file

	symn  defs;		// global variables and functions
	symn  gdef;		// all static variables and functions

	// virtual machine state
	struct {
		void* cell;					// execution unit(s)
		void* libv;					// libcall vector

		unsigned int	pc;			// exec: entry point / cgen: prev program counter
		unsigned int	px;			// exec: exit point / cgen: program counter

		unsigned int	ro;			// exec: read only memory / cgen: function parameters
		signed int		ss;			// exec: stack size / cgen: current stack size

		signed int		sm;			// exec: - / cgen: stack minimum size
		signed int		su;			// exec: - / cgen: stack access (parallel processing)

		unsigned int	pos;		// exec: trace pos / cgen: current positin in buffer
		signed int		opti;		// exec: - / cgen: optimization levevel

		struct {
			unsigned int	meta;
			unsigned int	code;
			//~ unsigned int	data;
		} size;

		void* _heap;
	} vm;

	/* compiler context
	 * after code was generated this becomes null.
	 */
	ccState cc;

	/* debug context
	 * this holds:
	 *  * debugger function
	 *  * code2line mapping
	 *  ? stack traces
	 *  ? break points
	 */
	dbgState dbg;

	// external library support
	struct {
		/** Begin a namespace. Returns NULL if error occurs.
		 * 
		 */
		symn (*const ccBegin)(state, char* cls);

		/** Define a(n) integer, floating point or string constant.
		 * @param the runtime context.
		 * @param name the name of the constant.
		 * @param value the value of the constant.
		 * @return the symbol to the definition, null on error.
		 */
		symn (*const ccDefInt)(state, char* name, int64_t value);
		symn (*const ccDefFlt)(state, char* name, float64_t value);
		symn (*const ccDefStr)(state, char* name, char* value);

		/** Add a type to the runtime.
		 * @param state the runtime context.
		 * @param name the name of the type.
		 * @param size the size of the type.
		 * @param refType non zero if is a reference type (class).
		 * @return the symbol to the definition, null on error.
		 */
		symn (*const ccAddType)(state, const char* name, unsigned size, int refType);

		/** Add a libcall (native function) to the runtime.
		 * @param the runtime context.
		 * @param libc the c function.
		 * @param data user data for this function.
		 * @param proto prototype of the function, do not forget the ending ';'
		 * @return the symbol to the definition, null on error.
		 * @usage see also: test.gl/gl.c
			static int f64sin(state rt) {
				float64_t x = argf64(rt, 0);	// get first argument
				retf64(rt, sin(x));				// set the return value
				return 0;						// no error in this call
			}
			if (!rt->api.ccAddCall(rt, f64sin, NULL, "float64 sin(float64 x);")) {
				error...
			}
		 */
		symn (*const ccAddCall)(state, int libc(libcArgs), void* extra, const char* proto);

		/** Compile the given file or text block.
		 * @param the runtime context.
		 * @param warn the warning level to be used.
		 * @param file filename of the compiled unit; this file will be opened if code is not null.
		 * @param line linenumber where compilation begins; it will be reset if code is not null.
		 * @param code if not null, this text will be compiled instead of the file.
		 * @return 0 on fail.
		 */
		int (*const ccAddCode)(state, int warn, char *file, int line, char *code);

		/** End the namespace, makes all declared variables static.
		 * @param the runtime context.
		 * @param cls the namespace, returned by ccBegin.
		*/
		void (*const ccEnd)(state, symn cls);

		/* Find a symbol by name.
		 * 
		symn (*const ccSymFind)(ccState cc, symn in, char *name);
		 */

		/* offset of a pointer in the vm
		 * 
		int (*const vmOffset)(state, void *ptr);
		 */

		/** Find a global symbol by offset.
		 * usefull for callbacks
		 * @usage see also: test.gl/gl.c

			static symn onMouse = NULL;

			static int setMouseCb(libcArgs rt) {
				void* fun = argref(rt, 0);

				// unregister event callback
				if (fun == NULL) {
					onMouse = NULL;
					return 0;
				}

				// register event callback using the symbol of the function.
				onMouse = rt->api.mapsym(rt, fun);

				// runtime error if symbol was not found.
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
		 *
		 * TODO: symn (*const vmSymbol)(state, void *ptr);
		 */
		symn (*const mapsym)(state, void *ptr);

		/** Invoke a function inside the vm.
		 * @param the runtime context.
		 * @param fun the symbol of the function.
		 * @param res if not null copy here the result of the function.
		 * @param args the arguments of the fuction are located here.
		 * @param extra extra parameter passed to each libcall executed from here.
		 * @return non zero on error.
		 * @usage see @mapsym example.
		*/
		int (*const invoke)(state, symn fun, void* res, void* args, void* extra);

		/** Allocate free  memory inside the vm.
		 * @param the runtime context.
		 * @param ptr an allocated memory address in the vm or null.
		 * @param size the new size to reallocate or 0 to free.
		 * @return pointer to the allocated memory.
		 * cases:
			size == 0 && ptr == null: nothing
			size == 0 && ptr != null: free
			size >  0 && ptr == null: alloc
			size >  0 && ptr != null: realloc
		*/
		void* (*const rtAlloc)(state, void* ptr, unsigned size);
	} api;

	// memory related
	unsigned char *_beg;		// cc: used memory; vm: >? heap begin
	unsigned char *_end;

	const long _size;			// size of total memory
	unsigned char _mem[];		// this is whwewe the memory begins.
};

static inline void* argval(libcArgs args, int offset, void *result, int size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, args->argv + offset, size);
	}
	else {
		result = args->argv + offset;
	}
	return result;
}
#define argval(__ARGV, __OFFS, __TYPE) (*(__TYPE*)((__ARGV)->argv + __OFFS))
static inline int32_t argi32(libcArgs args, int offs) { return argval(args, offs, int32_t); }
static inline int64_t argi64(libcArgs args, int offs) { return argval(args, offs, int64_t); }
static inline float32_t argf32(libcArgs args, int offs) { return argval(args, offs, float32_t); }
static inline float64_t argf64(libcArgs args, int offs) { return argval(args, offs, float64_t); }
static inline void* arghnd(libcArgs args, int offs) { return argval(args, offs, void*); }
static inline void* argref(libcArgs args, int offs) { int32_t p = argval(args, offs, int32_t); return p ? args->rt->_mem + p : NULL; }
static inline void* argsym(libcArgs args, int offs) { return args->rt->api.mapsym(args->rt, argref(args, offs)); }
//static inline char* argstr(libcArgs args, int offs) { return (char*)argref(args, offs); }
#undef argval

static inline void* setret(libcArgs args, void *result, int size) {
	if (result != NULL) {
		memcpy(args->retv, result, size);
	}
	return args->retv;
}
#define getret(__ARGV, __TYPE) (*((__TYPE*)((__ARGV)->retv)))
#define setret(__ARGV, __TYPE, __VAL) (getret(__ARGV, __TYPE) = (__TYPE)(__VAL))
static inline void reti32(libcArgs args, int32_t val) { setret(args, int32_t, val); }
static inline void reti64(libcArgs args, int64_t val) { setret(args, int64_t, val); }
static inline void retf32(libcArgs args, float32_t val) { setret(args, float32_t, val); }
static inline void retf64(libcArgs args, float64_t val) { setret(args, float64_t, val); }
static inline void rethnd(libcArgs args, void* val) { setret(args, void*, val); }
//~ static inline void retref(libcArgs args, void* val) { setret(args, void*, vmOffset(args->rt, val)); }
#undef setret

#ifdef __cplusplus
}
#endif
#endif
