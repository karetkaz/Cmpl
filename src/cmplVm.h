/**
 * Virtual machine core functions.
 */

#ifndef CMPL_VM_H
#define CMPL_VM_H

#include "cmpl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opcodes - VM
typedef enum {
	#define OPCODE_DEF(Name, Code, Size, In, Out, Text) Name = (Code),
	#include "defs.inl"
	opc_last,		// first invalid opcode

	opc_ldi,		// argument is the size
	opc_sti,		// argument is the size
	opc_drop,		// convert this to opcode.spc

	markIP,

	b32_bit_and = 0 << 6,
	b32_bit_shl = 1 << 6,
	b32_bit_shr = 2 << 6,
	b32_bit_sar = 3 << 6,

	vm_stk_align = sizeof(int32_t),	// stack alignment: size of one element on stack; must be 4: 32bits
	vm_mem_align = sizeof(void*),	// memory alignment: sizeof(void*), // value used to pad pointers
	vm_ref_size = sizeof(vmOffs),	// size of reference: TODO: allow 32 or 64 bit
	px_size = 4,	// size in bytes of the exit instruction halt()
	vm_regs = 255	// maximum registers for dup, set, pop, ...
} vmOpcode;

struct opcodeRec {
	signed int const code;			// opcode value (0..255)
	unsigned int const size;		// length of opcode with args
	unsigned int const stack_in;	// operation requires n elements on stack
	unsigned int const stack_out;	// operation produces n elements on stack
	const char *const name;			// mnemonic for the opcode
};
extern const struct opcodeRec opcode_tbl[256];

/// Value inside the virtual machine
typedef union {
	int8_t i08;
	int16_t i16;
	int32_t i32;
	int64_t i64;
	uint8_t u08;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	float32_t f32;
	float64_t f64;
	int32_t i24:24;
	struct {
		vmOffs ref;
		union {
			vmOffs type;
			vmOffs length;
		};
	};
} vmValue;

/**
 * Initialize runtime context.
 * 
 * @param mem (optional) use pre allocated memory.
 * @param size the size of memory in bytes to be used.
 * @return runtime context.
 */
rtContext rtInit(void *mem, size_t size);

/**
 * Close runtime context releasing resources.
 * 
 * @param rt Runtime context.
 * @return error count, 0 on success.
 */
int rtClose(rtContext rt);

/**
 * Initialize virtual machine to be able to emit instructions.
 * 
 * @param ctx Runtime context.
 * @param onHalt function to be executed when execution and external function invocation terminates.
 * @return offset of the first opcode that can be emitted.
 */
size_t vmInit(rtContext rt, int debug, vmError onHalt(nfcContext));

/**
 * Execute the generated bytecode.
 * 
 * @param rt Runtime context.
 * @param argc number of arguments.
 * @param argv the program arguments.
 * @param extra Extra data for native calls.
 * @return Error code of execution or noError on success.
 */
vmError execute(rtContext rt, int argc, char *argv[], void *extra);

/**
 * Invoke a function inside the vm.
 * 
 * @param rt Runtime context.
 * @param fun Symbol of the function.
 * @param ret (optional) Result value of the invoked function.
 * @param args (optional) Arguments for the function.
 * @param extra (optional) Extra data for each native call executed from here.
 * @return Error code of execution or noError on success.
 * @usage see @rtFindSym example.
 * @note Invocation to `execute` must precede this call.
 */
vmError invoke(rtContext rt, symn fun, void *ret, void *args, const void *extra);

/**
 * Lookup a static symbol by offset.
 * 
 * @param rt Runtime context.
 * @param ptr Pointer to the variable.
 * @param callsOnly lookup only functions.
 * @note Useful to invoke callbacks from native calls.
 * @usage:

	static symn onMouse = NULL;

	static int setMouseCb(nfcContext nfc) {
		size_t fun = argref(nfc, 0);

		// un-register event callback
		if (fun == 0) {
			onMouse = NULL;
			return noError;
		}

		// register event callback using the symbol of the function.
		onMouse = rt->api.rtLookup(rt, fun);

		// runtime error if symbol is not valid.
		return onMouse != NULL ? noError : nativeCallError;
	}

	static int mouseCb(int btn, int x, int y) {
		if (onMouse != NULL && rt != NULL) {
			// invoke the callback with arguments.
			struct {int32_t btn, x, y;} args = {btn, x, y};
			rt->api.invoke(rt, onMouse, NULL, &args, NULL);
		}
	}

	// expose the callback register function to the compiler
	if (!rt->api.ccDefCall(rt->cc, setMouseCb, "void setMouseCallback(void Callback(int32 btn, int32 x, int32 y)")) {
		error(...)
	}
 */
symn rtLookup(rtContext rt, size_t offs, ccKind filter);

/**
 * Allocate free and resize memory inside the vm heap.
 * 
 * @param rt Runtime context.
 * @param ptr resize this memory chunk, or allocate new if null.
 * @param size new size of the allocation or free by resizing the chunk to 0 bytes.
 * @param dbg debug memory callback to be invoked for each memory block.
 * @return Pointer to the allocated memory inside the vm heap.
 * cases:
 * 		size >  0 && ptr != null: re-alloc
 * 		size >  0 && ptr == null: alloc
 * 		size == 0 && ptr != null: free
 * 		size == 0 && ptr == null: nop
 * @note Invocation to `execute` must precede this call.
 */
void *rtAlloc(rtContext rt, void *ptr, size_t size, void dbg(dbgContext rt, void *mem, size_t size, char *kind));

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Native calls
/**
 * Reset argument iterator to the first argument.
 * 
 * @param nfc the native call context.
 * @return relative offset of the first argument.
 */
size_t nfcFirstArg(nfcContext nfc);

/**
 * Advance argument iterator to the next argument.
 * 
 * @param nfc the native call context.
 * @return relative offset of the current argument.
 */
size_t nfcNextArg(nfcContext nfc);

/**
 * Read the value at the given offset converting offsets to pointers.
 * 
 * @param nfc the native call context.
 * @param offs relative offset of the argument.
 * @return value.
 * @note offsets inside structs will be not converted to pointers.
 * @note undefined behaviour if the argument is a struct larger than rtValue.
 */
rtValue nfcReadArg(nfcContext nfc, size_t offs);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Instructions
/**
 * Emit an instruction.
 * 
 * @param rt Runtime context.
 * @param opc Opcode.
 * @param arg Argument.
 * @return Program counter.
 */
size_t emitOpc(rtContext rt, vmOpcode opc, vmValue arg);

/**
 * Test for an instruction at the given offset.
 * 
 * @param rt Runtime context.
 * @param offs Offset of the opcode.
 * @param opc Operation code to check.
 * @param arg (optional) Copy the argument of the opcode.
 * @return non zero if at the given location the opc was found.
 */
int testOcp(rtContext rt, size_t offs, vmOpcode opc, vmValue *arg);

/**
 * Fix a jump instruction, and the stack size.
 * 
 * @param rt Runtime context.
 * @param src Location of the instruction.
 * @param dst Absolute position to jump.
 * @param stc Fix also stack size.
 * @return
 */
int fixJump(rtContext rt, size_t src, size_t dst, ssize_t stc);

/// Emit an instruction with zero argument(s).
static inline size_t emit(rtContext rt, vmOpcode opc) {
	vmValue arg;
	arg.i64 = 0;
	return emitOpc(rt, opc, arg);
}

/// Emit an instruction with an integer argument.
static inline size_t emitInt(rtContext rt, vmOpcode opc, int64_t value) {
	vmValue arg;
	arg.i64 = value;
	return emitOpc(rt, opc, arg);
}

/// Emit load int64 constant value instruction.
static inline size_t emitI64(rtContext rt, int64_t value) {
	vmValue arg;
	arg.i64 = value;
	return emitOpc(rt, opc_lc64, arg);
}

/// Emit load float64 constant value instruction.
static inline size_t emitF64(rtContext rt, float64_t value) {
	vmValue arg;
	arg.f64 = value;
	return emitOpc(rt, opc_lf64, arg);
}

/// Emit load offset value instruction.
static inline size_t emitRef(rtContext rt, size_t value) {
	vmValue arg;
	arg.i64 = value;
	return emitOpc(rt, opc_lref, arg);
}

#ifdef __cplusplus
}
#endif
#endif
