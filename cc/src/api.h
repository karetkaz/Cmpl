#ifndef CC_API_H
#define CC_API_H 2
#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>		//for logf in struct state
#include <string.h>		//for memcpy in poparg

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

typedef struct symRec *symn;			// symbol
typedef struct stateRec *state;		// runtime
typedef struct ccStateRec *ccState;	// compiler
typedef struct dbgStateRec *dbgState;	// debugger

struct stateRec {
	int   errc;		// error count
	int   closelog;	// close log file
	FILE* logf;		// log file

	symn  defs;		// global variables and functions
	symn  gdef;		// static variables and functions

	// TODO: should be inside vm.
	// function data
	struct libcstate {
		symn  libc;		// invoked function
		char* argv;		// first argument
		void* retv;		// return value
		void* data;		// user data
	} libc;

	// virtual machine
	struct {
		void* cell;					// execution unit(s)
		void* libv;					// libcall vector

		unsigned int	pc;			// entry point / prev program counter
		unsigned int	px;			// exit point / program counter

		unsigned int	ss;			// stack size / current stack size
		unsigned int	sm;			// stack minimum size
		unsigned int	su;			// stack access (parallel processing)

		unsigned int	ro;			// <= ro : read only region(meta data) / cgen:(function parameters)
		unsigned int	pos;		// trace pos / current positin in buffer
		signed int		opti;		// optimization levevel

		struct {
			unsigned int	meta;
			unsigned int	code;
			unsigned int	data;
		} size;

		void* _heap;
	} vm;

	/* compiler enviroment
	 * after code was generated this becomes null.
	 */
	ccState cc;

	/* debug informatios
	 * this struct should hold:
	 *  * debugger function
	 *  * line mappings
	 *  * stack traces
	 *  * break points
	 */
	dbgState dbg;

	// external library support
	struct {
		/** Begin a namespace. Returns NULL if error occurs.
		 * 
		 */
		symn (*ccBegin)(state, char* cls);

		/** Define a(n) integer, floating point or string constant.
		 * @param the runtime state.
		 * @param name the name of the constant.
		 * @param value the value of the constant.
		 * @return the symbol to the definition, null on error.
		 */
		symn (*ccDefInt)(state, char* name, int64_t value);
		symn (*ccDefFlt)(state, char* name, float64_t value);
		symn (*ccDefStr)(state, char* name, char* value);

		/** Add a type to the runtime.
		 * @param state the runtime state.
		 * @param name the name of the type.
		 * @param size the size of the type.
		 * @param refType non zero if is a reference type (class).
		 * @return the symbol to the definition, null on error.
		 */
		symn (*ccAddType)(state, const char* name, unsigned size, int refType);

		/** Add a libcall (native function) to the runtime.
		 * @param the runtime state.
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
		symn (*ccAddCall)(state, int libc(state, void* data), void* data, const char* proto);

		/** compile the given file or text block.
		 * @param state
		 * @param warn the warning level to be used.
		 * @param file filename of the compiled unit; this file will be opened if code is not null.
		 * @param line linenumber where compilation begins; it will be reset if code is not null.
		 * @param code if not null, this text will be compiled instead of the file.
		 * @return the number of errors.
		 */
		int (*ccAddCode)(state, int warn, char *file, int line, char *code);

		/** End the namespace, makes all declared variables static.
		 * @param state
		 * @param cls the namespace, returned by ccBegin.
		*/
		void (*ccEnd)(state, symn cls);

		/* Find a symbol by name.
		 * 
		symn (*ccSymFind)(ccState cc, symn in, char *name);
		 */

		/* offset of a pointer in the vm
		 * 
		int (*vmOffset)(state, void *ptr);
		 */

		/** Find a global symbol by offset.
		 * usefull for callbacks
		 * @usage see also: test.gl/gl.c

			static symn onMouse = NULL;

			static int setMouseCb(state rt) {
				void* fun = argref(rt, 0);

				// unregister event callback
				if (fun == NULL) {
					onMouse = NULL;
					return 1;
				}

				// register event callback
				onMouse = rt->api.symfind(rt, fun);		// get and find the functions symbol
				return onMouse != NULL;
			}

			static int mouseCb(int btn, int x, int y) {
				if (onMouse != NULL) {
					// invoke the callback with arguments.
					struct {int32_t btn, x, y;} args = {btn, x, y};
					rt->api.invoke(rt, onMouse, NULL, &args);
				}
			}

			// expose the callback register function to the compiler
			if (!rt->api.ccAddCall(rt, setMouseCb, NULL, "void setMouseCallback(void Callback(int b, int x, int y);")) {
				error...
			}
		 */
		symn (*symfind)(state, void *ptr);

		/** Invoke a callback function inside the vm.
		 * @param state
		 * @param fun the symbol of the function.
		 * @return non zero on error.
		 * @usage see @symfind example.
		*/
		int (*invoke)(state, symn fun, void* result, void *args);

		/** memory manager of the vm.
		 * @param the runtime state.
		 * @param ptr an allocated memory address in the vm or null.
		 * @param size the new size to reallocate or 0.
		 * @return pointer to the allocated memory.
		 * cases:
			ptr == null && size == 0: nothing
			ptr == null && size >  0: alloc
			ptr != null && size == 0: free
			ptr != null && size >  0: realloc
		*/
		void* (*rtAlloc)(state, void* ptr, unsigned size);
	} api;

	// memory related
	long _size;				// size of total memory
	unsigned char* _beg;	// cc: used memory; vm: heap begin
	unsigned char* _end;
	unsigned char _mem[];
};

static inline void* argval(state rt, int offset, void *result, int size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, rt->libc.argv + offset, size);
	}
	else {
		result = rt->libc.argv + offset;
	}
	return result;
}
#define argval(__ARGV, __OFFS, __TYPE) (*(__TYPE*)((__ARGV)->libc.argv + __OFFS))
static inline int32_t argi32(state rt, int offs) { return argval(rt, offs, int32_t); }
static inline int64_t argi64(state rt, int offs) { return argval(rt, offs, int64_t); }
static inline float32_t argf32(state rt, int offs) { return argval(rt, offs, float32_t); }
static inline float64_t argf64(state rt, int offs) { return argval(rt, offs, float64_t); }
static inline void* arghnd(state rt, int offs) { return argval(rt, offs, void*); }
static inline void* argref(state rt, int offs) { int32_t p = argval(rt, offs, int32_t); return p ? rt->_mem + p : NULL; }
static inline char* argstr(state rt, int offs) { return (char*)argref(rt, offs); }
#undef argval

static inline void* setret(state rt, void *result, int size) {
	if (result != NULL) {
		memcpy(rt->libc.retv, result, size);
	}
	return rt->libc.retv;
}
#define getret(__ARGV, __TYPE) (*((__TYPE*)(__ARGV->libc.retv)))
#define setret(__ARGV, __TYPE, __VAL) (getret(__ARGV, __TYPE) = (__TYPE)(__VAL))
static inline void reti32(state rt, int32_t val) { setret(rt, int32_t, val); }
static inline void reti64(state rt, int64_t val) { setret(rt, int64_t, val); }
static inline void retf32(state rt, float32_t val) { setret(rt, float32_t, val); }
static inline void retf64(state rt, float64_t val) { setret(rt, float64_t, val); }
static inline void rethnd(state rt, void* val) { setret(rt, void*, val); }
//~ static inline void retref(state rt, void* val) { setret(rt, void*, vmOffset(rt, val)); }
#undef setret

#ifdef __cplusplus
}
#endif
#endif
