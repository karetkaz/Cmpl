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
	FILE* logf;		// log file
	int   closelog;	// close log file

	symn  defs;		// definitions
	symn  gdef;		// definitions

	void* libv;		// libcall vector
	symn  libc;		// library call symbol

	void* retv;		// return value
	char* argv;		// first argument

	//~ TODO("fdata should be void*")
	int fdata;		// function data passed to libcall
	void* udata;		// user data for execution passed to vmExec

	ccState cc;		// compiler enviroment
	struct {
		int		opti;			// optimization levevel

		unsigned int	pc;			// entry point / prev program counter
		unsigned int	px;			// exit point / program counter

		unsigned int	ss;			// stack size / current stack size
		unsigned int	sm;			// stack minimum size

		unsigned int	ro;			// <= ro : read only region(meta data) / cgen:(function parameters)
		unsigned int	pos;		// current positin in buffer
	} vm;

	/*struct {

		// arguments
		int argc;
		char* argv[];

		// environment
		int envc;
		char* envv[];

		// system
		char* os;		// linux / windows
		char* user;
	}pe;*/

	long _size;		// size of total memory
	void *_free;	// list of free memory
	void *_used;	// list of used memory

	unsigned char *_ptr;	// cc: used memory; vm: max stack when calling vmCall from a libcall
	unsigned char _mem[];
};

typedef struct stateApi {
	state rt;	// runtime
	/* dll should contain the folowing entry:
		int apiMain(stateApi api) {}
	**/

	symn (*ccBegin)(state, char *cls);
	//symn (*ccCompile)(state, char *cls);

	int (*ccAddText)(state, int warn, char *__file, int __line, char *text);

	symn (*ccDefineInt)(state, char *name, int32_t value);
	//~ symn ccDefineFlt(state, char *name, double value);
	//~ symn ccDefineStr(state, char *name, char* value);

	symn (*libcall)(state, int libc(state), int pass, const char* proto);
	symn (*install)(state, const char* name, unsigned size);

	void (*ccEnd)(state, symn cls);

	symn (*findref)(state rt, void *ptr);
	symn (*findsym)(ccState cc, symn in, char *name);

	// offset of a pointer in the vm
	int (*vmOffset)(state rt, void *ptr);
	int (*invoke)(state, symn fun, ...);

	void* (*rtAlloc)(state rt, void* ptr, unsigned size);	// realloc/alloc/free


}* stateApi; // */

#ifdef _MSC_VER
#define STINLINE static __inline
#else
#define STINLINE static inline
#endif

//~ #define poparg(__ARGV, __SIZE) ((void*)(((__ARGV)->argv += (((__SIZE) + 3) & ~3)) - (((__SIZE) + 3) & ~3))))
//~ #define popaty(__ARGV, __TYPE) (*(__TYPE*)(poparg((__ARGV), sizeof(__TYPE))))
//~ #define poparg(__ARGV, __SIZE) ((void*)(((__ARGV)->argv += (__SIZE)) - (__SIZE)))

//~ #define setret(__ARGV, __TYPE, __VAL) (*((__TYPE*)(__ARGV->retv)) = (__TYPE)(__VAL))

#define retptr(__ARGV, __TYPE) ((__TYPE*)(__ARGV->retv))
#define setret(__ARGV, __TYPE, __VAL) (*retptr(__ARGV, __TYPE) = (__TYPE)(__VAL))
#define getret(__ARGV, __TYPE) (*retptr(__ARGV, __TYPE))

STINLINE void* poparg(state rt, void *res, int size) {
	// if result is not null copy
	if (res != NULL) {
		memcpy(res, rt->argv, size);
	}
	else {
		res = rt->argv;
	}
	rt->argv += size;
	return res;
}
STINLINE int32_t popi32(state rt) { return *(int32_t*)poparg(rt, NULL, sizeof(int32_t)); }
STINLINE int64_t popi64(state rt) { return *(int64_t*)poparg(rt, NULL, sizeof(int64_t)); }
STINLINE float32_t popf32(state rt) { return *(float32_t*)poparg(rt, NULL, sizeof(float32_t)); }
STINLINE float64_t popf64(state rt) { return *(float64_t*)poparg(rt, NULL, sizeof(float64_t)); }
STINLINE void* popref(state rt) { int32_t p = popi32(rt); return p ? rt->_mem + p : NULL; }
STINLINE char* popstr(state rt) { return popref(rt); }

//~ STINLINE void reti32(state rt, int32_t val) { setret(int32_t, rt, val); }
//~ STINLINE void reti64(state rt, int64_t val) { setret(int64_t, rt, val); }
//~ STINLINE void retf32(state rt, float32_t val) { setret(float32_t, rt, val); }
//~ STINLINE void retf64(state rt, float64_t val) { setret(float64_t, rt, val); }
//~ STINLINE void retref(state rt, float64_t val) { setret(float64_t, rt, vmOffset(s, val)); }
