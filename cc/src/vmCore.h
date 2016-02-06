/**
 * virtual machine core functions.
 */
#ifndef VM_CORE_H
#define VM_CORE_H

#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opcodes - VM
typedef enum {
#define OPCDEF(Name, Code, Size, Args, Push, Mnem) Name = Code,
#include "defs.inl"
	opc_last,

	opc_neg,		// argument is the type
	opc_add,
	opc_sub,
	opc_mul,
	opc_div,
	opc_mod,

	opc_cmt,
	opc_shl,
	opc_shr,
	opc_and,
	opc_ior,
	opc_xor,

	opc_ceq,
	opc_cne,
	opc_clt,
	opc_cle,
	opc_cgt,
	opc_cge,

	opc_ldi,		// argument is the size
	opc_sti,		// argument is the size
	opc_drop,		// convert this to opcode.spc

	markIP,

	b32_bit_and = 0 << 6,
	b32_bit_shl = 1 << 6,
	b32_bit_shr = 2 << 6,
	b32_bit_sar = 3 << 6,

	vm_size = 4,//sizeof(int),	// size of data element on stack
			px_size = 5,// size in bytes of the exit instruction exit(0)
			rt_size = 8,//sizeof(void*),
			vm_regs = 255	// maximum registers for dup, set, pop, ...
} vmOpcode;
struct opc_inf {
	signed int const code;		// opcode value (0..255)
	unsigned int const size;	// length of opcode with args
	signed int const chck;		// minimum elements on stack before execution
	signed int const diff;		// stack size difference after execution
	char *const name;	// mnemonic for the opcode
};
extern const struct opc_inf opc_tbl[255];

typedef union {		// on stack value type
	int8_t		i1;
	int16_t		i2;
	int32_t		i4;
	int64_t		i8;
	uint8_t		u1;
	uint16_t	u2;
	uint32_t	u4;
	//uint64_t	u8;
	float32_t	f4;
	float64_t	f8;
	int32_t		rel:24;
	struct {int32_t data; int32_t length;} arr;	// slice
	struct {int32_t value; int32_t type;} var;	// variant
	struct {int32_t func; int32_t vars;} del;	// delegate
} stkval;

/// Abstract Syntax Tree Node
typedef struct astNode *astn;
/// Debug Node
typedef struct dbgNode *dbgn;

/**
 * @brief Initialize runtime context.
 * @param mem optionally use pre allocated memory.
 * @param size the size of memory in bytes to be used.
 * @return runtime context.
 */
rtContext rtInit(void* mem, size_t size);

/**
 * @brief Close runtime context.
 * @param Runtime context.
 * release resources.
 */
void rtClose(rtContext);

// open log file for both compiler and runtime
void logFILE(rtContext, FILE *file);
int logfile(rtContext, char *file, int append);

/** Generate executable bytecode.
 * @param Runtime context.
 * @param mode see@(cgen_opti, cgen_info, cgen_glob)
 * @return boolean value of success.
 */
int gencode(rtContext, int mode);

/**
 * @brief Execute the compiled script.
 * @param Runtime context.
 * @param ss Stack size in bytes.
 * @param extra Extra data for libcalls.
 * @return Error code of execution.
 * @todo units number of execution units to run on.
 * @todo argc(int) argument count from main.
 * @todo argv(char*[]) arguments from main.
 */
vmError execute(rtContext, size_t ss, void *extra);

/// Invoke a function; @see rtContext.api.invoke
vmError invoke(rtContext, symn fun, void *ret, void* args, void *extra);

/// Lookup symbol by offset; @see rtContext.api.rtFindSym
symn rtFindSym(rtContext, size_t offs, int callsOnly);

/// Alloc, resize or free memory; @see rtContext.api.rtAlloc
void* rtAlloc(rtContext, void* ptr, size_t size, void dbg(rtContext rt, void* mem, size_t size, int used));

/// returns a pointer to an offset inside the vm.
// TODO: to be removed.
static inline void* getip(rtContext rt, size_t offset) {
	if (offset == 0) {
		return NULL;
	}
	return (void*)(rt->_mem + offset);
}

/**
 * @brief Get the internal offset of a reference.
 * @param ptr Memory location.
 * @return The internal offset.
 * @note Aborts if ptr is not null and not inside the context.
 */
size_t vmOffset(rtContext, void *ptr);

/**
 * @brief Optimize an assigment by removing extra copy of the value if it is on the top of the stack.
 * @param Runtime context.
 * @param offsBegin Begin of the byte code.
 * @param offsEnd End of the byte code.
 * @return non zero if the code was optimized.
 */
// TODO: this function should be internal
int optimizeAssign(rtContext, size_t offsBegin, size_t offsEnd);

/**
 * @brief Emit an instruction.
 * @param Runtime context.
 * @param opc Opcode.
 * @param arg Argument.
 * @return Program counter.
 */
size_t emitarg(rtContext, vmOpcode opc, stkval arg);

/**
 * @brief Emit an instruction with int argument.
 * @param Runtime context.
 * @param opc Opcode.
 * @param arg Integer argument.
 * @return Program counter.
 */
size_t emitint(rtContext, vmOpcode opc, int64_t arg);

/**
 * @brief Fix a jump instruction.
 * @param Runtime context.
 * @param src Location of the instruction.
 * @param dst Absolute position to jump.
 * @param stc Fix also stack size.
 * @return
 */
int fixjump(rtContext, int src, int dst, int stc);

/**
 * print stack trace
 */
void logTrace(rtContext rt, FILE *out, int indent, int startLevel, int traceLevel);

/**
 * @brief Print formatted text to the output stream.
 * @param fout Output stream.
 * @param msg Format text.
 * @param ... Format variables.
 * @note %(\?)?[+-]?[0 ]?([1-9][0-9]*)?(.(\*)|([1-9][0-9]*))?[tTkKAIbBoOxXuUdDfFeEsScC]
 *    skip: (\?)? skip printing if variable is null or zero (prints pad character if specified.)
 *    sign: [+-]? sign flag / alignment.
 *    padd: [0 ]? padding character.
 *    len:  ([1-9][0-9]*)? length
 *    offs: (.(*)|([1-9][0-9]*))? percent or offset.
 *
 *    T: symbol
 *      +: expand function
 *      -: qualified name only
 *    k: ast
 *      +: expand statements
 *      -: ?
 *    K: kind
 *    A: instruction (asm)
 *    I: indent
 *    b: 32 bit bin value
 *    B: 64 bit bin value
 *    o: 32 bit oct value
 *    O: 64 bit oct value
 *    x: 32 bit hex value
 *    X: 64 bit hex value
 *    u: 32 bit unsigned value
 *    U: 64 bit unsigned value
 *    d: 32 bit signed value (decimal)
 *    D: 64 bit signed value (decimal)
 *    f: 32 bit floating point value
 *    F: 64 bit floating point value
 *    e: 32 bit floating point value (Scientific notation (mantissa/exponent))
 *    E: 64 bit floating point value (Scientific notation (mantissa/exponent))
 *    s: ansi string
 *    c: ansi character
 *    S: ?wide string
 *    C: ?wide character
 * flags
 *    +-
 *        oOuU: ignored
 *        bBxX: lowerCase / upperCase
 *        dDfFeE: forceSign /
 *
 */
void fputfmt(FILE *out, const char *fmt, ...);

// Debuging.
dbgn getDbgStatement(rtContext rt, char* file, int line);
dbgn mapDbgStatement(rtContext rt, size_t position);
dbgn addDbgStatement(rtContext rt, size_t start, size_t end, astn tag);
dbgn mapDbgFunction(rtContext rt, size_t position);
dbgn addDbgFunction(rtContext rt, symn fun);

#ifdef __cplusplus
}
#endif
#endif
