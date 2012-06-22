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

typedef struct symn *symn;		// symbol
typedef struct state *state;	// runtime
typedef struct ccState *ccState;// compiler

struct state {
	int   errc;		// error count
	int   closelog;	// close log file
	FILE* logf;		// log file

	symn  defs;		// global variables and functions
	symn  gdef;		// static variables and functions

	void* libv;		// libcall vector

	symn  libc;		// library call symbol
	void* retv;		// return value
	char* argv;		// first argument
	void* udata;	//?user data for execution passed to vmExec

	ccState cc;		// compiler enviroment
	int (*dbug)(state, int pu, void* ip, long* sptr, int scnt);

	struct vm {

		unsigned int	pc;			// entry point / prev program counter
		unsigned int	px;			// exit point / program counter

		unsigned int	ss;			// stack size / current stack size
		unsigned int	sm;			// stack minimum size
		unsigned int	su;			// stack access (parallel processing)

		unsigned int	ro;			// <= ro : read only region(meta data) / cgen:(function parameters)
		unsigned int	pos;		// current positin in buffer
		signed int		opti;		// optimization levevel
		struct size {
			unsigned int	meta;
			unsigned int	code;
			unsigned int	data;
		} size;
	} vm;

	long _size;		// size of total memory
	void* _free;	// list of free memory
	void* _used;	// list of used memory

	unsigned char* _beg;	// cc: used memory; vm: max stack when calling vmCall from a libcall
	unsigned char* _end;

	unsigned char _mem[];
};

typedef struct stateApi {
	/** dll should contain the folowing entry:
	 *     int apiMain(stateApi api) {}
	 * this struct should be copied?
	**/

	state rt;	// runtime

	void (*onClose)();	// callback: when the library is closed

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
	 * @usage
		static int f64sin(state rt) {
			float64_t x = popf64(rt);		// pop one argument
			setret(rt, float64_t, sin(x));	// set the return value
			return 0;						// no error in this call
		}
		if (!api->ccAddLibc(api->rt, f64sin, NULL, "float64 sin(float64 x);")) {
			error...
		}
	 */
	symn (*ccAddLibc)(state, int libc(state, void* data), void* data, const char* proto);

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

	/** Find a global symbol by offset.
	 * usefull for callbacks
	 * @usage
		static symn onMouse = null;
		static int mouseCb(int btn, int x, int y) {
			if (onMouse) {
				struct {int btn, x, y;} args = {btn, x, y};
				api->invoke(api->rt, onMouse, &args);		// invoke the callback
			}
		}
		static int setMouseCb(state rt) {
			onMouse = api->findref(api->rt, popref(rt));		// pop and find the functions symbol
			return onMouse != NULL;								// no error in this call
		}
		if (!api->ccAddLibc(api->rt, setMouseCb, NULL, "void regMouse(void CB(int b, int x, int y);")) {
			error...
		}
	 */
	symn (*findref)(state, void *ptr);

	/* Find a global symbol by name.
	 * 
	symn (*findsym)(ccState cc, symn in, char *name);
	 */
	/* offset of a pointer in the vm
	 * 
	int (*vmOffset)(state, void *ptr);
	 */

	/** Invoke a function inside the vm.
	 * @param state
	 * @param fun the symbol of the function.
	 * @return non zero on error.
	 * @usage
		static int setpixels(state rt) {
			symn pixelfun = api->findref(api->rt, popref(rt));		// pop and find the functions symbol
			for (int y = 0; y < sceen.height; ++y) {
				for (int x = 0; x < sceen.width; ++x) {
					struct {double x, y;} args = {(double)x / sceen.width, (double)y / sceen.height};
					if (api->invoke(api->rt, pixelfun, &args) == 0) {
						sceen.pixels[x + y * sceen.width] = getret(api->rt, int32_t);
					}
				}
			}
		}
		if (!api->ccAddLibc(api->rt, setpixels, NULL, "void setpixels(int CallBack(double x, double y);")) {
			error...
		}
	*/
	int (*invoke)(state, symn fun, void *args);

	/** memory manager of the vm.
	 * @param the runtime state.
	 * @param ptr an allocated memory address in the vm or null.
	 * @param size the new size to reallocate or 0.
	 * @return non zero on error.
		ptr == null && size == 0: nothing
		ptr == null && size >  0: alloc
		ptr != null && size == 0: free
		ptr != null && size >  0: realloc
	*/
	void* (*rtAlloc)(state, void* ptr, unsigned size);

}* stateApi;

static inline int padded(int offs, int align) {
	//~ assert(align == (align & -align));
	//~ return align + ((offs - 1) & ~(align - 1));
	return offs + (align - 1) & ~(align - 1);
}

static inline void* setret(state rt, void *result, int size) {
	if (result != NULL) {
		memcpy(rt->retv, result, size);
	}
	return rt->retv;
}
static inline void* poparg(state rt, void *result, int size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, rt->argv, size);
	}
	else {
		result = rt->argv;
	}
	rt->argv += padded(size, 4);
	return result;
}

//~ #define poparg(__ARGV, __SIZE) ((void*)(((__ARGV)->argv += (((__SIZE) + 3) & ~3)) - (((__SIZE) + 3) & ~3))))
//~ #define popaty(__ARGV, __TYPE) (*(__TYPE*)(poparg((__ARGV), sizeof(__TYPE))))

#define poparg(__ARGV, __TYPE) (((__TYPE*)((__ARGV)->argv += ((sizeof(__TYPE) + 3) & ~3)))[-1])
//~ static inline int32_t popi32(state rt) { return *(int32_t*)poparg(rt, NULL, sizeof(int32_t)); }
//~ static inline int64_t popi64(state rt) { return *(int64_t*)poparg(rt, NULL, sizeof(int64_t)); }
//~ static inline float32_t popf32(state rt) { return *(float32_t*)poparg(rt, NULL, sizeof(float32_t)); }
//~ static inline float64_t popf64(state rt) { return *(float64_t*)poparg(rt, NULL, sizeof(float64_t)); }
static inline int32_t popi32(state rt) { return poparg(rt, int32_t); }
static inline int64_t popi64(state rt) { return poparg(rt, int64_t); }
static inline float32_t popf32(state rt) { return poparg(rt, float32_t); }
static inline float64_t popf64(state rt) { return poparg(rt, float64_t); }
static inline void* popref(state rt) { int32_t p = popi32(rt); return p ? rt->_mem + p : NULL; }
static inline char* popstr(state rt) { return popref(rt); }
#undef poparg

#define getret(__ARGV, __TYPE) (*((__TYPE*)(__ARGV->retv)))
#define setret(__ARGV, __TYPE, __VAL) (getret(__ARGV, __TYPE) = (__TYPE)(__VAL))
static inline void reti32(state rt, int32_t val) { setret(rt, int32_t, val); }
static inline void reti64(state rt, int64_t val) { setret(rt, int64_t, val); }
static inline void retf32(state rt, float32_t val) { setret(rt, float32_t, val); }
static inline void retf64(state rt, float64_t val) { setret(rt, float64_t, val); }
//~ static inline void retref(state rt, void* val) { setret(rt, void*, vmOffset(rt, val)); }
#undef setret
