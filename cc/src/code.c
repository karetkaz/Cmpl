/*******************************************************************************
 *   File: code.c
 *   Date: 2011/06/23
 *   Description: bytecode related stuff
 *******************************************************************************
code emmiting, executing and formatting
*******************************************************************************/
#include <string.h>
#include <math.h>
#include "core.h"

#ifdef __WATCOMC__
#pragma disable_message(124);
// needed for opcode f32_mod
static inline double fmodf(float x, float y) {
	return fmod(x, y);
}
#endif

/// Bit scan forward. (get the index of the highest bit seted)
static inline int32_t bitsf(uint32_t x) {
	int result = 0;
	if ((x & 0x0000ffff) == 0) {
		result += 16;
		x >>= 16;
	}
	if ((x & 0x000000ff) == 0) {
		result += 8;
		x >>= 8;
	}
	if ((x & 0x0000000f) == 0) {
		result += 4;
		x >>= 4;
	}
	if ((x & 0x00000003) == 0) {
		result += 2;
		x >>= 2;
	}
	if ((x & 0x00000001) == 0) {
		result += 1;
	}
	return x ? result : -1;
}

typedef uint8_t* memptr;
typedef uint32_t* stkptr;

/// bytecode instruction
typedef struct bcde *bcde;

/// bytecode processor
typedef struct cell *cell;
typedef struct trace *trace;

#pragma pack(push, 1)
struct bcde {
	uint8_t opc;
	union {
		stkval arg;			// argument (load const)
		uint8_t idx;		// usualy index for stack
		int32_t rel:24;		// relative offset (ip, sp) +- 7MB
		struct {
			// used when starting a new paralel task
			uint8_t  dl;	// data to to be copied to stack
			uint16_t cl;	// code to to be executed paralel: 0 means fork
		};
		/* TODO: extended opcodes
		struct {				// extended: 4 bytes `res := lhs OP rhs`
			uint32_t mem:2;		// mem access
			uint32_t res:8;		// res
			uint32_t lhs:7;		// lhs
			uint32_t rhs:7;		// rhs

			/+ --8<-------------------------------------------
			void* res = sp + ip->ext.res;
			void* lhs = sp + ip->ext.lhs;
			void* rhs = sp + ip->ext.rhs;

			// check stack
			CHKSTK(ip->ext.res);
			CHKSTK(ip->ext.lhs);
			CHKSTK(ip->ext.rhs);

			// memory indirection
			switch (ip->ext.mem) {
				case 0:
					break;
				case 1:
					CHKMEM_W(*(int**)res);
					res = *(void**)res;
					break;
				case 2:
					CHKMEM_R(*(int**)lhs);
					lhs = *(void**)lhs;
					break;
				case 3:
					CHKMEM_R(*(int**)rhs);
					rhs = *(void**)rhs;
					break;
			}

			switch (ip->opc) {
				case ext_neg: *(type*)res = -(*(type*)rhs); break;
				case ext_add: *(type*)res = (*(type*)lhs) + (*(type*)rhs); break;
				case ext_sub: *(type*)res = (*(type*)lhs) - (*(type*)rhs); break;
				case ext_mul: *(type*)res = (*(type*)lhs) * (*(type*)rhs); break;
				case ext_div: *(type*)res = (*(type*)lhs) / (*(type*)rhs); break;
				case ext_mod: *(type*)res = (*(type*)lhs) % (*(type*)rhs); break;
				case ext_..1: res = lhs ... rhs; break;
				case ext_..2: res = lhs ... rhs; break;
			}
			// +/ --8<----------------------------------------
		} ext;// */
	};
};
#pragma pack(pop)

struct cell {
	unsigned char	*ip;		// Instruction pointer

	// Stack
	unsigned char	*bp;		// Stack base
	unsigned char	*tp;		// Trace pointer +(bp)
	unsigned char	*sp;		// Stack pointer -(bp + ss)

	// multiproc
	size_t			ss;			// stack size
	unsigned int	cp;			// child procs (join == 0)
	unsigned int	pp;			// parent proc (main == 0)
};

struct trace {
	unsigned char	*ip;		// Instruction pointer
	signed char	*sp;			// Stack pointer
};

/// Check if the pointer is inside the vm.
static inline int isValidOffset(state rt, void* ptr) {
	if ((unsigned char*)ptr > rt->_mem + rt->_size) {
		return 0;
	}
	if (ptr && (unsigned char*)ptr < rt->_mem) {
		return 0;
	}
	return 1;
}

static inline void rollbackPc(state rt) {
	bcde ip = getip(rt, rt->vm.pc);
	if ((void*)ip != rt->_beg) {
		rt->_beg = (memptr)ip;
		rt->vm.ss -= opc_tbl[ip->opc].diff;
	}
}

size_t vmOffset(state rt, void* ptr) {
	dieif(!isValidOffset(rt, ptr), "invalid reference(%06x)", ((unsigned char*)ptr - rt->_mem));
	return ptr ? (unsigned char*)ptr - rt->_mem : 0;
}

/**
 * @brief Check for an instruction at the given offset.
 * @param offs Offset of the opcode.
 * @param opc Opcode to check.
 * @param arg Copy the argument of the opcode.
 * @return non zero if at the given location the opc was found.
 * @note Aborts if opc is not valid.
 */
static int checkOcp(state rt, size_t offs, vmOpcode opc, stkval *arg) {
	bcde ip = getip(rt, offs);
	if (opc >= opc_last) {
		fatal("invalid opc(0x%x)", opc);
		return 0;
	}
	if (ip->opc == opc) {
		if (arg != NULL) {
			*arg = ip->arg;
		}
		return 1;
	}
	return 0;
}

static int removeOpc(state rt, size_t offs) {
	if (offs >= rt->vm.ro && offs <= rt->vm.pc) {
		bcde ip = getip(rt, offs);
		unsigned char *dst = (unsigned char *)ip;
		size_t size = opc_tbl[ip->opc].size;
		memcpy(dst, dst + size, rt->_beg - dst);
		rt->_beg -= size;
		rt->vm.pc = rt->_beg - rt->_mem;
		return 1;
	}
	return 0;
}

static size_t nextIp(state rt, size_t offs) {
	register bcde ip = getip(rt, offs);
	switch (ip->opc) {
		__ERROR:
			fatal("%.*A", offs, ip);
			return 0;

		#define NEXT(__IP, __SP, __CHK) offs += __IP;
		#define STOP(__ERR, __CHK, __ERC) do {if (__CHK) {goto __ERROR;}} while(0)
		#include "code.inl"
	}
	return offs;
}

static int decrementStackAccess(state rt, size_t offsBegin, size_t offsEnd, int count) {
	size_t offs;
	for (offs = offsBegin; offs < offsEnd; offs = nextIp(rt, offs)) {
		register bcde ip = getip(rt, offs);
		switch (ip->opc) {
			case opc_set1:
			case opc_set2:
			case opc_set4:
			case opc_dup1:
			case opc_dup2:
			case opc_dup4:
				ip->idx -= count;
				break;
			case opc_ldsp:
				ip->rel -= count * vm_size;
				break;
		}
	}
	return 1;
}

int optimizeAssign(state rt, size_t offsBegin, size_t offsEnd) {
	stkval arg;
	//~ dieif(rt->vm.pc != offsEnd, "Error %d != %d", rt->vm.px, offsEnd);
	if (checkOcp(rt, offsBegin, opc_dup1, &arg)) {
		//~ logif(1, "speed up for (int32 i = 0; i < 10, i += 1) ...");
		//~ dumpTree(rt, NULL, offsBegin, offsEnd);

		// dupplicate top of stack then set top of stack
		if (arg.u1 != 0) {
			return 0;
		}
		if (!checkOcp(rt, offsEnd, opc_set1, &arg)) {
			return 0;
		}
		if (arg.u1 != 1) {
			return 0;
		}

		if (!removeOpc(rt, offsEnd)) {
			fatal("%.*A", offsEnd, getip(rt, offsEnd));
			return 0;
		}
		if (!removeOpc(rt, offsBegin)) {
			fatal("%.*A", offsEnd, getip(rt, offsEnd));
			return 0;
		}
		if (!decrementStackAccess(rt, offsBegin, offsEnd, 1)) {
			fatal("%.*A", offsBegin, getip(rt, offsBegin));
			return 0;
		}
		return 1;
	}
	if (checkOcp(rt, offsBegin, opc_dup2, &arg)) {
		//~ logif(1, "speed up for (int32 i = 0; i < 10, i += 1) ...");
		//~ dumpTree(rt, NULL, offsBegin, offsEnd);

		// dupplicate top of stack then set top of stack
		if (arg.u1 != 0) {
			return 0;
		}
		if (!checkOcp(rt, offsEnd, opc_set2, &arg)) {
			return 0;
		}
		if (arg.u1 != 2) {
			return 0;
		}
		if (!removeOpc(rt, offsEnd)) {
			fatal("%.*A", offsEnd, getip(rt, offsEnd));
			return 0;
		}
		if (!removeOpc(rt, offsBegin)) {
			fatal("%.*A", offsEnd, getip(rt, offsEnd));
			return 0;
		}
		if (!decrementStackAccess(rt, offsBegin, offsEnd, 2)) {
			fatal("%.*A", offsBegin, getip(rt, offsBegin));
			return 0;
		}
		return 1;
	}
	return 0;
}

// base function emiting an ocode, see header
size_t emitarg(state rt, vmOpcode opc, stkval arg) {
	libc libcvec = rt->vm.libv;
	bcde ip = (bcde)rt->_beg;

	dieif((unsigned char*)ip + 16 >= rt->_end, ERR_MEMORY_OVERRUN);

	/* TODO: set max stack used.
	switch (opc) {
		case opc_dup1:
		case opc_set1:
			logif(1, "dupset(%d, %d)", rt->vm.su, arg.i4);
			if (rt->vm.su < arg.i4 + 1)
				rt->vm.su = arg.i4 + 1;
			break;
		case opc_dup2:
		case opc_set2:
			logif(1, "dupset(%d, %d)", rt->vm.su, arg.i4);
			if (rt->vm.su < arg.i4 + 2)
				rt->vm.su = arg.i4 + 2;
			break;
		case opc_dup4:
		case opc_set4:
			logif(1, "dupset(%d, %d)", rt->vm.su, arg.i4);
			if (rt->vm.su < arg.i4 + 4)
				rt->vm.su = arg.i4 + 4;
			break;
		case opc_ldi:
		case opc_ldi1:
		case opc_ldi2:
		case opc_ldi4:
		case opc_ldi8:
		case opc_ldiq:
		case opc_sti:
		case opc_sti1:
		case opc_sti2:
		case opc_sti4:
		case opc_sti8:
		case opc_stiq: {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp) {
				logif(1, "ldsp(%d, %d)", rt->vm.su, arg.i4);
				if (rt->vm.su < arg.i4 / 4)
					rt->vm.su = arg.i4 / 4;
			}
		} break;
	}
	// */

	if (opc == markIP) {
		return rt->_beg - rt->_mem;
	}

	// load / store
	else if (opc == opc_ldi) {
		ip = getip(rt, rt->vm.pc);
		if (ip->opc == opc_ldsp) {
			size_t vardist = (ip->rel + arg.i4) / 4;
			if (rt->vm.su < vardist)
				rt->vm.su = vardist;
		}

		dieif (arg.i4 != arg.i8, "FixMe");
		switch (arg.i4) {
			case 1:
				opc = opc_ldi1;
				break;

			case 2:
				opc = opc_ldi2;
				break;

			case 4:
				opc = opc_ldi4;
				break;

			case 8:
				opc = opc_ldi8;
				break;

			case 16:
				opc = opc_ldiq;
				break;

			default: // we have src on the stack

				// push dst
				if (!emitint(rt, opc_ldsp, 4 - arg.i4)) {
					trace("FixMe");
					return 0;
				}

				// copy
				if (!emitint(rt, opc_move, -arg.i4)) {
					trace("FixMe");
					return 0;
				}

				// create n bytes on stack
				opc = opc_spc;
				break;
		}
	}
	else if (opc == opc_sti) {
		ip = getip(rt, rt->vm.pc);
		if (ip->opc == opc_ldsp) {
			size_t vardist = (ip->rel + arg.i4) / 4;
			if (rt->vm.su < vardist)
				rt->vm.su = vardist;
		}

		dieif (arg.i4 != arg.i8, "FixMe");
		switch (arg.i4) {
			case 1:
				opc = opc_sti1;
				break;

			case 2:
				opc = opc_sti2;
				break;

			case 4:
				opc = opc_sti4;
				break;

			case 8:
				opc = opc_sti8;
				break;

			case 16:
				opc = opc_stiq;
				break;

			default: // we have dst on the stack

				// push dst
				if (!emitint(rt, opc_ldsp, vm_size)) {
					trace("FixMe");
					return 0;
				}

				// copy
				if (!emitint(rt, opc_move, arg.i4)) {
					trace("FixMe");
					return 0;
				}

				if (ip->opc == opc_ldsp) {
					//~ if we copy from the stack to the stack
					//~ do not remove more elements than we copy
					if (arg.i8 > ip->rel) {
						arg.i8 = ip->rel;
					}
				}
				// remove n bytes from stack
				arg.i8 = -arg.i8;
				opc = opc_spc;
				break;
		}
	}
	else if (opc == opc_drop) {
		opc = opc_spc;
		arg.i8 = -arg.i8;
	}

	if (opc >= opc_last) {
		fatal("invalid opc(0x%x, 0x%X)", opc, arg.i8);
		return 0;
	}

	// Optimize
	if (rt->vm.opti > 0) {
		if (0) {
		}

		/* TODO: ldzs are not optimized, uncomment when vm does constant evaluations.
		else if (opc == opc_lc32) {
			if (arg.i8 == 0) {
				opc = opc_ldz1;
				ip = getip(rt, rt->vm.pc);
				if (ip->opc == opc_ldz1) {
					opc = opc_ldz2;
					rollbackPc(rt);
				}
			}
		}
		else if (opc == opc_lc64) {
			if (arg.i8 == 0) {
				opc = opc_ldz2;
				ip = getip(rt, rt->vm.pc);
				if (ip->opc == opc_ldz2) {
					opc = opc_ldz4;
					rollbackPc(rt);
				}
			}
		}

		/ *else if (opc == opc_ldi1) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup1;
				rt->_beg = (memptr)ip;
				rt->vm.ss -= 1;
			}
		}
		else if (opc == opc_ldi2) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup1;
				rt->_beg = (memptr)ip;
				rt->vm.ss -= 1;
			}
		}
		// */

		else if (opc == opc_ldi4) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup1;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_ld32;
				rollbackPc(rt);
			}
		}
		else if (opc == opc_sti4) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set1;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_st32;
				rollbackPc(rt);
			}
		}

		else if (opc == opc_ldi8) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup2;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_ld64;
				rollbackPc(rt);
			}
		}
		else if (opc == opc_sti8) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set2;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref && ip->arg.u4 < 0x00ffffff) {
				arg.i8 = ip->arg.u4;
				opc = opc_st64;
				rollbackPc(rt);
			}
		}

		else if (opc == opc_ldiq) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) <= vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup4;
				rollbackPc(rt);
			}
		}
		else if (opc == opc_stiq) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) <= vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set4;
				rollbackPc(rt);
			}
		}

		// shl, shr, sar, and with immediate value.
		else if (opc == b32_shl) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i4 = b32_bit_shl | (ip->arg.i4 & 0x3f);
				opc = b32_bit;
				rollbackPc(rt);
			}
		}
		else if (opc == b32_shr) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i4 = b32_bit_shr | (ip->arg.i4 & 0x3f);
				opc = b32_bit;
				rollbackPc(rt);
			}
		}
		else if (opc == b32_sar) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i4 = b32_bit_sar | (ip->arg.i4 & 0x3f);
				opc = b32_bit;
				rollbackPc(rt);
			}
		}
		else if (opc == b32_and) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				if ((ip->arg.i4 & (ip->arg.i4 + 1)) == 0) {
					arg.i4 = b32_bit_and | (bitsf(ip->arg.u4 + 1) & 0x3f);
					opc = b32_bit;
					rollbackPc(rt);
				}
			}
		}

		//* TODO: eval Arithmetic using vm.
		else if (opc == opc_not) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_not) {
				rt->_beg = (memptr)ip;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
		}
		else if (opc == i32_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == i32_neg) {
				rt->_beg = (memptr)ip;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_lc32) {
				ip->arg.i4 = -ip->arg.i4;
				return rt->vm.pc;
			}
		}
		else if (opc == i64_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == i64_neg) {
				rt->_beg = (memptr)ip;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_lc64) {
				ip->arg.i8 = -ip->arg.i8;
				return rt->vm.pc;
			}
		}
		else if (opc == f32_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == f32_neg) {
				rt->_beg = (memptr)ip;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_lc32) {
				ip->arg.f4 = -ip->arg.f4;
				return rt->vm.pc;
			}
		}
		else if (opc == f64_neg) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == f64_neg) {
				rt->_beg = (memptr)ip;
				ip->opc = opc_nop;
				return rt->vm.pc;
			}
			if (ip->opc == opc_lc64) {
				ip->arg.f8 = -ip->arg.f8;
				return rt->vm.pc;
			}
		}
		else if (opc == i32_add) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i8 = ip->arg.i4;
				if (arg.rel == arg.i8) {
					opc = opc_inc;
					rollbackPc(rt);
				}
			}
		}
		else if (opc == i32_sub) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i8 = -ip->arg.i4;
				if (arg.rel == arg.i8) {
					opc = opc_inc;
					rollbackPc(rt);
				}
			}
		}
		else if (opc == opc_inc) {
			if (arg.i8 == 0) {
				return rt->vm.pc;
			}

			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_inc) {
				stkval tmp;
				tmp.i8 = arg.i8 + ip->rel;
				if (tmp.rel == tmp.i8) {
					ip->rel = tmp.rel;
					return rt->vm.pc;
				}
			}
			else if (ip->opc == opc_ldsp) {
				stkval tmp;
				if (arg.i8 < 0) {
					// local variable bound error.
					trace("FixMe");
					return 0;
				}
				tmp.i8 = arg.i8 + ip->rel;
				if (tmp.rel == tmp.i8) {
					ip->rel = tmp.rel;
					return rt->vm.pc;
				}
			}
			else if (ip->opc == opc_lref) {
				stkval tmp;
				if (arg.i8 < 0) {
					// global variable bound error.
					trace("FixMe");
					return 0;
				}
				tmp.i8 = arg.i8 + ip->rel;
				if (tmp.rel == tmp.i8) {
					ip->rel = tmp.rel;
					return rt->vm.pc;
				}
			}
		}

		//* TODO: eval Conversions using vm.
		else if (opc == u32_i64) {
			ip = getip(rt, rt->vm.pc);
			//~ if (ip->opc == opc_ldz2) { ... }
			if (ip->opc == opc_lc32) {
				arg.i8 = ip->arg.u4;
				opc = opc_lc64;
				rollbackPc(rt);
			}
		}
		else if (opc == i32_bol) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i8 = ip->arg.i4 != 0;
				opc = opc_lc32;
				rollbackPc(rt);
			}
		}
		else if (opc == i32_f32) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.f4 = (float32_t) ip->arg.i4;
				opc = opc_lf32;
				rollbackPc(rt);
				logif(ip->arg.i4 != arg.f4, "inexact cast: %d => %f", ip->arg.i4, arg.f4);
			}
		}
		else if (opc == i32_i64) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.i8 = ip->arg.i4;
				opc = opc_lc64;
				rollbackPc(rt);
			}
		}
		else if (opc == i32_f64) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc32) {
				arg.f8 = ip->arg.i4;
				opc = opc_lf64;
				rollbackPc(rt);
				logif(ip->arg.i4 != arg.f8, "inexact cast: %d => %F", ip->arg.i4, arg.f8);
			}
		}
		else if (opc == i64_i32) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc64) {
				arg.i8 = ip->arg.i8;
				opc = opc_lc32;
				rollbackPc(rt);
				//TODO: logif(ip->arg.i8 != arg.i4, "inexact cast: %D => %d", ip->arg.i8, arg.i4);
				logif(ip->arg.i8 != arg.i4 && ip->arg.i8 != arg.u4, "inexact cast: %D => %d", ip->arg.i8, arg.i4);
			}
		}
		else if (opc == i64_f32) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc64) {
				arg.f4 = (float32_t) ip->arg.i8;
				opc = opc_lf32;
				rollbackPc(rt);
				logif(ip->arg.i8 != arg.f4, "inexact cast: %D => %f", ip->arg.i8, arg.f4);
			}
		}
		else if (opc == i64_bol) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc64) {
				arg.i8 = ip->arg.i8 != 0;
				opc = opc_lc32;
				rollbackPc(rt);
			}
		}
		else if (opc == i64_f64) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lc64) {
				arg.f8 = (float64_t) ip->arg.i8;
				opc = opc_lf64;
				rollbackPc(rt);
				logif(ip->arg.i8 != arg.f8, "inexact cast: %D => %F", ip->arg.i8, arg.f8);
			}
		}
		else if (opc == f32_i32) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf32) {
				arg.i8 = (int64_t) ip->arg.f4;
				opc = opc_lc32;
				rollbackPc(rt);
				logif(ip->arg.f4 != arg.i4, "inexact cast: %f => %d", ip->arg.f4, arg.i4);
			}
		}
		else if (opc == f32_bol) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf32) {
				arg.i8 = ip->arg.f4 != 0;
				opc = opc_lc32;
				rollbackPc(rt);
			}
		}
		else if (opc == f32_i64) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf32) {
				arg.i8 = (int64_t) ip->arg.f4;
				opc = opc_lc64;
				rollbackPc(rt);
				logif(ip->arg.f4 != arg.i8, "inexact cast: %f => %D", ip->arg.f4, arg.i8);
			}
		}
		else if (opc == f32_f64) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf32) {
				arg.f8 = ip->arg.f4;
				opc = opc_lf64;
				rollbackPc(rt);
				logif(ip->arg.f4 != arg.f8, "inexact cast: %f => %F", ip->arg.f4, arg.f8);
			}
		}
		else if (opc == f64_i32) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf64) {
				arg.i8 = (int64_t) ip->arg.f8;
				opc = opc_lc32;
				rollbackPc(rt);
				logif(ip->arg.f8 != arg.i4, "inexact cast: %F => %d", ip->arg.f8, arg.i4);
			}
		}
		else if (opc == f64_f32) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf64) {
				arg.f4 = (float32_t) ip->arg.f8;
				opc = opc_lf32;
				rollbackPc(rt);
				logif(ip->arg.f8 != arg.f4, "inexact cast: %F => %f", ip->arg.f8, arg.f4);
			}
		}
		else if (opc == f64_i64) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf64) {
				arg.i8 = (int64_t) ip->arg.f8;
				opc = opc_lc64;
				rollbackPc(rt);
				logif(ip->arg.f8 != arg.i8, "inexact cast: %F => %D", ip->arg.f8, arg.i8);
			}
		}
		else if (opc == f64_bol) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_lf64) {
				arg.i8 = ip->arg.f8 != 0;
				opc = opc_lc32;
				rollbackPc(rt);
			}
		}
		// */

		/* mul, div, mod
		else if (opc == u32_mod) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_lc32) {
				int x = ip->arg.i4;
				// if constant contains only one bit
				if (x > 0 && lobit(x) == x) {
					ip->arg.i4 -= 1;
					opc = b32_and;
				}
			}
		}
		else if (opc == u32_div) {
			ip = getip(s, s->vm.pc);
			if (ip->opc == opc_lc32) {
				int x = ip->arg.i4;
				// if constant contains only one bit
				if (x > 0 && lobit(x) == x) {
					ip->arg.i4 ?= 1;
					opc = b32_shr;
				}
			}
		}*/

		// conditional jumps
		else if (opc == opc_jnz) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_not) {
				rt->_beg = (memptr)ip;
				opc = opc_jz;
			}
			// i32 to boolean can be skiped
			// f32 to boolean can not be skiped (-0f != 0)
			else if (ip->opc == i32_bol) {
				rt->_beg = (memptr)ip;
			}
		}
		else if (opc == opc_jz) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_not) {
				rt->_beg = (memptr)ip;
				opc = opc_jnz;
			}
			// i32 to boolean can be skiped
			// f32 to boolean can not be skiped (-0f != 0)
			else if (ip->opc == i32_bol) {
				rt->_beg = (memptr)ip;
			}
		}

		/* TODO: merge stack allocations
		else if (opc == opc_spc) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_spc) {
				stkval tmp;
				tmp.i8 = arg.i8 + ip->rel;
				if (tmp.rel == tmp.i8) {
					rt->_beg = (memptr)ip;
					rt->vm.ss -= ip->rel / vm_size;
					arg.i8 += ip->rel;
				}
			}
		}// */

		/* TODO: only if we have no jump here
		else if (opc == opc_jmpi) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_jmpi) {
				//~ rt->_beg = (memptr)ip;
				return rt->vm.pc;
			}
		}*/

	}

	ip = (bcde)rt->_beg;

	ip->opc = opc;
	ip->arg = arg;
	rt->vm.pc = rt->_beg - rt->_mem;	// previous program pointer is here

	// post checks
	if ((opc == opc_jmp || opc == opc_jnz || opc == opc_jz) && arg.i8 != 0) {
		ip->rel -= rt->vm.pc;
		logif((ip->rel + rt->vm.pc) != arg.i8, "FixMe");
	}
	else switch (opc) {
		case opc_spc:
			if (arg.i8 > 0x7fffff) {
				trace("max 2 megs can be allocated on stack, not(%D)", arg.i8);
				return 0;
			}
			dieif(ip->rel != arg.i8, ERR_INTERNAL_ERROR);
			break;

		case opc_st64:
		case opc_ld64:
			/*TODO: if ((ip->rel & 7) != 0) {
				trace("Error: %d", ip->rel);
				return 0;
			}*/
			//~ dieif((ip->rel & 7) != 0, "Error @ip @%06x", vmOffset(rt, ip));
			dieif(ip->rel != arg.i8, ERR_INTERNAL_ERROR);
			break;

		case opc_st32:
		case opc_ld32:
			dieif((ip->rel & 3) != 0, ERR_INTERNAL_ERROR);
			dieif(ip->rel != arg.i8, ERR_INTERNAL_ERROR);
			break;

		case opc_ldsp:
		case opc_move:
		case opc_inc:
		case opc_mad:
			dieif(ip->rel != arg.i8, ERR_INTERNAL_ERROR);
			break;

		default:
			break;
	}

	switch (opc) {
		error_opc:
			error(rt, NULL, 0, "invalid opcode: %.*A", rt->vm.pc, ip);
			return 0;

		error_stc:
			error(rt, NULL, 0, "stack underflow(%d): %.*A", rt->vm.ss, rt->vm.pc, ip);
			return 0;

		#define STOP(__ERR, __CHK, __ERC) if (__CHK) goto __ERR
		#define NEXT(__IP, __SP, __CHK)\
			STOP(error_stc, (ptrdiff_t)rt->vm.ss < (ptrdiff_t)(__CHK), -1);\
			rt->vm.ss += (__SP);\
			rt->_beg += (__IP);
		#include "code.inl"
	}

	if (opc == opc_call) {
		//~ call ends with a return,
		//~ wich drops ip from the stack.
		rt->vm.ss -= 1;
	}

	// set max stack used.
	if (rt->vm.sm < rt->vm.ss) {
		rt->vm.sm = rt->vm.ss;
	}

	return rt->vm.pc;
}
int fixjump(state rt, int src, int dst, int stc) {
	dieif(stc > 0 && stc & 3, "FixMe");
	if (src >= 0) {
		bcde ip = getip(rt, src);
		if (src) switch (ip->opc) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return 0;

			case opc_task:
				ip->dl = (uint8_t) (stc / 4);
				ip->cl = (uint16_t) (dst - src);
				dieif(ip->dl != stc / 4, "FixMe");
				dieif(ip->cl != dst - src, "FixMe");
				//TODO: dieif(ip->dl > vm_regs, "FixMe");
				return 1;

			//~ case opc_ldsp:
			//~ case opc_call:
			case opc_jmp:
			case opc_jnz:
			case opc_jz:
				ip->rel = dst - src;
				dieif(ip->rel != dst - src, "FixMe");
				break;
		}
		if (stc >= 0) {
			rt->vm.ss = stc / vm_size;
		}
		return 1;
	}

	fatal(ERR_INTERNAL_ERROR);
	return 0;
}

// TODO: to be removed.
static inline void logProc(cell pu, int cp, char* msg) {
	trace("%s: {pu:%d, ip:%x, bp:%x, sp:%x, stacksize:%d, parent:%d, childs:%d}", msg, cp, pu[cp].ip, pu[cp].bp, pu[cp].sp, pu[cp].ss, pu[cp].pp, pu[cp].cp);
	(void)pu;
	(void)cp;
	(void)msg;
}

/// Try to start a new child cell for task.
static inline int task(cell pu, int n, int master, int cl) {
	// find an empty cell
	int slave = master + 1;
	while (slave < n) {
		if (pu[slave].ip == NULL) {
			break;
		}
		slave += 1;
	}

	if (slave < n) {
		// set workers, parent, ip, sp and copy stack
		pu[master].cp++;
		pu[slave].cp = 0;
		pu[slave].pp = master;

		pu[slave].ip = pu[master].ip;
		pu[slave].sp = pu[slave].bp + pu[slave].ss - cl;
		memcpy(pu[slave].sp, pu[master].sp, (size_t) cl);

		logProc(pu, master, "master");
		logProc(pu, slave, "slave");

		return slave;
	}
	//~ else trace("master(%d): continue.", master);
	return 0;
}

/// Wait for child cells to halt.
static int sync(cell pu, int cp, int wait) {
	int pp = pu[cp].pp;
	logProc(pu, cp, "join");

	// slave proc
	if (pp != cp) {
		if (pu[cp].cp == 0) {
			//~ trace("stop slave(%d).", cp);
			pu[cp].ip = NULL;
			pu[pp].cp -= 1;
			return 1;
		}
		return 0;
	}

	if (pu[cp].cp > 0) {
		//~ trace("master(%d): continue.", cp);
		return !wait;
	}

	// cell can continue to work
	return 1;
}

/// Check for stack overflow.
static inline int ovf(cell pu) {
	//~ logif((pu->sp - pu->tp) < 0, "{tp: %d, sp: %d}", pu->tp - pu->bp, pu->sp - pu->bp);
	return (pu->sp - pu->tp) < 0;
}

// TODO: to be renamed; manages function call stack traces
static inline int dotrace(state rt, void* ip, void* sp) {
	cell pu = rt->vm.cell;
	if (rt->dbg == NULL) {
		return 0;
	}
	if (sp == NULL) {
		if (pu->tp - pu->bp < (ptrdiff_t)sizeof(struct trace)) {
			debug("tp: %d - sp: %d", pu->tp - pu->bp, pu->sp - pu->bp);
			return 0;
		}
		pu->tp -= sizeof(struct trace);
	}
	else {
		trace tp = (trace)pu->tp;
		if (ovf(pu)) {
			debug("tp: %d - sp: %d", pu->tp - pu->bp, pu->sp - pu->bp);
			return 0;
		}
		tp->ip = ip;
		tp->sp = sp;
		pu->tp += sizeof(struct trace);
	}
	return 1;
}

/// Private dummy debug function.
static int dbgDummy(state rt, int pu, void *ip, void* sp, size_t ss, vmError err, size_t fp) {
	if (err != noError) {
		// TODO: get file and line from debug information ?
		error(rt, NULL, 0, "exec: %d(%?d):[sp%02d]@%.*A rw@%06x", err, pu, ss, vmOffset(rt, ip), ip);
		logTrace(rt, NULL, 1, -1, 100);
		return -1;
	}
	(void)sp;
	(void)fp;
	return 0;
}

/**
 * @brief Execute bytecode.
 * @param rt Runtime context.
 * @param pu Cell to execute on.
 * @param fun Executing function.
 * @param extra Extra data for libcalls.
 * @param dbg function which is executed after each instruction or on error.
 * @return Error code of execution, 0 on success.
 */
static int exec(state rt, cell pu, symn fun, void* extra, int dbg(state, int, void*, void*, size_t, vmError, size_t)) {
	size_t err_code = 0;

	const int cc = 1;
	const libc libcvec = rt->vm.libv;

	const size_t ms = rt->_size;			// memory size
	const size_t ro = rt->vm.ro;			// read only region
	const memptr mp = (void*)rt->_mem;
	const stkptr st = (void*)(pu->bp + pu->ss);

	// run in debug mode if the specified function is not null or
	if (rt->dbg != NULL || dbg != NULL) {
		const stkptr spMin = (stkptr)(pu->bp);
		const stkptr spMax = (stkptr)(pu->bp + pu->ss);
		const bcde ipMin = (bcde)(rt->_mem + rt->vm.ro);
		const bcde ipMax = (bcde)(rt->_mem + rt->vm.px + 2);

		if (dbg == NULL) {
			dbg = rt->dbg->dbug;
			if (dbg == NULL) {
				dbg = dbgDummy;
			}
		}
		// invoked function will return with a ret instruction
		dotrace(rt, NULL, pu->sp);
		for ( ; ; ) {
			register const bcde ip = (bcde)pu->ip;
			register const stkptr sp = (stkptr)pu->sp;

			if (ip >= ipMax || ip < ipMin) {
				dbg(rt, 0, ip, sp, pu->ss, invalidIP, vmOffset(rt, ip));
				return invalidIP;
			}
			if (sp > spMax || sp < spMin) {
				dbg(rt, 0, ip, sp, pu->ss, invalidSP, vmOffset(rt, sp));
				return invalidSP;
			}
			if ((err_code = dbg(rt, 0, ip, sp, st - sp, noError, 0)) != 0) {
				// abort execution from debuging
				return err_code;
			}
			switch (ip->opc) {
				dbg_stop_vm:	// halt virtual machine
					return 0;

				dbg_error_opc:
					dbg(rt, err_code, ip, sp, pu->ss, invalidOpcode, err_code);
					return invalidOpcode;

				dbg_error_ovf:
					dbg(rt, err_code, ip, sp, pu->ss, stackOverflow, err_code);
					return stackOverflow;

				dbg_error_trace_ovf:
					dbg(rt, err_code, ip, sp, pu->ss, traceOverflow, err_code);
					return traceOverflow;

				dbg_error_mem:
					dbg(rt, err_code, ip, sp, pu->ss, segmentationFault, err_code);
					return segmentationFault;

				dbg_error_div_flt:
					dbg(rt, err_code, ip, sp, pu->ss, divisionByZero, err_code);
					// continue execution on floating point division by zero.
					break;

				dbg_error_div:
					dbg(rt, err_code, ip, sp, pu->ss, divisionByZero, err_code);
					return divisionByZero;

				dbg_error_libc:
					dbg(rt, err_code, ip, sp, pu->ss, libCallError, libcvec[ip->rel].sym->offs);
					return libCallError;

				#define NEXT(__IP, __SP, __CHK) pu->sp -= vm_size * (__SP); pu->ip += (__IP);
				#define STOP(__ERR, __CHK, __ERC) do {if (__CHK) {err_code = __ERC; goto dbg_##__ERR;}} while(0)
				#define EXEC
				#define TRACE(__IP, __SP) do { if (!dotrace(rt, __IP, __SP)) goto dbg_error_trace_ovf; } while(0)
				#include "code.inl"
			}
		}
		return 0;
	}

	// code for maximum execution speed
	for ( ; ; ) {
		register const bcde ip = (bcde)pu->ip;
		register const stkptr sp = (stkptr)pu->sp;
		switch (ip->opc) {
			stop_vm:	// halt virtual machine
				return 0;

			error_opc:
				dbgDummy(rt, err_code, ip, sp, pu->ss, invalidOpcode, err_code);
				return invalidOpcode;

			error_ovf:
				dbgDummy(rt, err_code, ip, sp, pu->ss, stackOverflow, err_code);
				return stackOverflow;

			error_mem:
				dbgDummy(rt, err_code, ip, sp, pu->ss, segmentationFault, err_code);
				return segmentationFault;

			error_div_flt:
				dbgDummy(rt, err_code, ip, sp, pu->ss, divisionByZero, err_code);
				// continue execution on floating point division by zero.
				break;

			error_div:
				dbgDummy(rt, err_code, ip, sp, pu->ss, divisionByZero, err_code);
				return divisionByZero;

			error_libc:
				dbgDummy(rt, err_code, ip, sp, pu->ss, libCallError, libcvec[ip->rel].sym->offs);
				return libCallError;

			#define NEXT(__IP, __SP, __CHK) {pu->sp -= vm_size * (__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK, __ERC) if (__CHK) {err_code = __ERC; goto __ERR;}
			#define EXEC
			#include "code.inl"
		}
	}
	return 0;
}


int invoke(state rt, symn fun, void* res, void* args, void* extra) {
	cell pu = rt->vm.cell;
	unsigned char *ip = pu->ip;
	unsigned char *sp = pu->sp;
	unsigned char *tp = pu->tp;

	// result is the first param
	// TODO: ressize = fun->prms->size;
	size_t ressize = sizeOf(fun->type, 1);
	void* resp = NULL;
	int result;

	dieif(fun->kind != TYPE_ref || !fun->call, "FixMe");

	// result is the last argument.
	resp = pu->sp - ressize;

	// make space for result and arguments// result is the first param
	pu->sp -= fun->prms->offs;

	if (args != NULL) {
		memcpy(pu->sp, args, fun->prms->offs - ressize);
	}

	// return here: vm->px: program exit
	*(size_t *)(pu->sp -= vm_size) = rt->vm.px;

	pu->ip = getip(rt, fun->offs);

	result = exec(rt, pu, fun, extra, NULL);
	if (result == noError && res != NULL) {
		memcpy(res, resp, (size_t) ressize);
	}

	pu->ip = ip;
	pu->sp = sp;	//
	pu->tp = tp;	// during exec we may return from code.

	return result;
}
int execute(state rt, void* extra, size_t ss) {
	// TODO: cells should be in runtime read only memory?
	cell pu;

	// invalidate compiler
	rt->cc = NULL;

	// invalidate memory manager
	rt->vm.heap = NULL;

	// allocate cell(s)
	rt->_end = rt->_mem + rt->_size;
	rt->_end -= sizeof(struct cell);
	rt->vm.cell = pu = (void*)rt->_end;

	rt->_end -= ss;

	pu->cp = 0;
	pu->pp = 0;
	pu->ss = ss;
	pu->bp = rt->_end;
	pu->tp = rt->_end;
	pu->sp = rt->_end + ss;
	pu->ip = rt->_mem + rt->vm.pc;

	dieif((ptrdiff_t)pu->sp & (vm_size - 1), "invalid statck alignment");

	if (pu->bp > pu->sp) {
		error(rt, NULL, 0, "invalid statck size");
		return -99;
	}

	// TODO argc, argv
	return exec(rt, pu, rt->init, extra, NULL);
}


void fputopc(FILE* fout, unsigned char* ptr, size_t len, size_t offs, state rt) {
	size_t i;
	bcde ip = (bcde)ptr;

	//~ write data as bytes
	//~ TODO: symplify this
	if (len > 1 && len < opc_tbl[ip->opc].size) {
		for (i = 0; i < len - 2; i++) {
			if (i < opc_tbl[ip->opc].size) {
				fputfmt(fout, "%02x ", ptr[i]);
			}
			else {
				fputfmt(fout, "   ");
			}
		}
		if (i < opc_tbl[ip->opc].size) {
			fputfmt(fout, "%02x... ", ptr[i]);
		}
	}
	else {
		for (i = 0; i < len; i++) {
			if (i < opc_tbl[ip->opc].size) {
				fputfmt(fout, "%02x ", ptr[i]);
			}
			else {
				fputfmt(fout, "   ");
			}
		}
	}

	if (opc_tbl[ip->opc].name != NULL) {
		fputs(opc_tbl[ip->opc].name, fout);
	}
	else {
		fputfmt(fout, "opc.x%02x", ip->opc);
	}

	switch (ip->opc) {
		default:
			break;

		case opc_inc:
		case opc_spc:
		case opc_ldsp:
		case opc_move:
			fputfmt(fout, " %+d", ip->rel);
			break;

		case opc_mad:
			fputfmt(fout, " %d", ip->rel);
			break;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
			if (offs == (size_t)-1) {
				fputfmt(fout, " %+d", ip->rel);
			}
			else {
				fputfmt(fout, " @%06x", offs + ip->rel);
			}
			break;

		case opc_task:
			if (offs == (size_t)-1) {
				fputfmt(fout, " %d, %d", ip->dl, ip->cl);
			}
			else {
				fputfmt(fout, " %d, .%06x", ip->dl, offs + ip->cl);
			}
			break;

		case opc_sync:
			fputfmt(fout, " %d", ip->idx);
			break;

		case b32_bit: switch (ip->idx & 0xc0) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return;

			case b32_bit_and:
				fputfmt(fout, "and 0x%03x", (1 << (ip->idx & 0x3f)) - 1);
				break;

			case b32_bit_shl:
				fputfmt(fout, "shl 0x%03x", ip->idx & 0x3f);
				break;

			case b32_bit_shr:
				fputfmt(fout, "shr 0x%03x", ip->idx & 0x3f);
				break;

			case b32_bit_sar:
				fputfmt(fout, "sar 0x%03x", ip->idx & 0x3f);
				break;
		} break;

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			fputfmt(fout, " sp(%d)", ip->idx);
			break;

		case opc_lc32:
			fputfmt(fout, " %d", ip->arg.i4);
			break;

		case opc_lc64:
			fputfmt(fout, " %D", ip->arg.i8);
			break;

		case opc_lf32:
			fputfmt(fout, " %f", ip->arg.f4);
			break;

		case opc_lf64:
			fputfmt(fout, " %F", ip->arg.f8);
			break;

		case opc_lref: {
			fputfmt(fout, " %x", ip->arg.u4);
			if (rt != NULL) {
				symn sym = mapsym(rt, ip->arg.u4, 0);
				if (sym != NULL) {
					fputfmt(fout, ": %+T", sym);
					if (ip->arg.u4 > sym->offs) {
						fputfmt(fout, "<+%06x>", ip->arg.u4 - sym->offs);
					}
					fputfmt(fout, ": %T", sym->type);
				}
				else {
					char *str = getResStr(rt, ip->arg.u4);
					if (str != NULL) {
						fputfmt(fout, ": \"%s\"", str);
					}
				}
			}
			break;
		}

		case opc_ld32:
		case opc_st32:
		case opc_ld64:
		case opc_st64:
			fputfmt(fout, " %x", ip->rel);
			if (rt != NULL) {
				symn sym = mapsym(rt, ip->rel, 0);
				if (sym != NULL) {
					fputfmt(fout, ": %+T", sym);
					if (ip->rel > sym->offs) {
						fputfmt(fout, "<+%06x>", ip->rel - sym->offs);
					}
					fputfmt(fout, ": %T", sym->type);
				}
				else {
					char *str = getResStr(rt, ip->rel);
					if (str != NULL) {
						fputfmt(fout, ": \"%s\"", str);
					}
				}
			}
			break;

		case opc_libc:
			if (rt != NULL) {
				libc lc = NULL;
				if (rt->cc && rt->cc->libc) {
					lc = rt->cc->libc;
					while (lc) {
						if (lc->pos == ip->idx) {
							break;
						}
						lc = lc->next;
					}
				}
				else if (rt->vm.libv) {
					lc = &((libc)rt->vm.libv)[ip->idx];
				}
				if (lc && lc->sym) {
					fputfmt(fout, "(%d): %+T: %T", ip->idx, lc->sym, lc->sym->type);
				}
				else {
					fputfmt(fout, "(%d)", ip->idx);
				}
			}
			else {
				fputfmt(fout, "(%d)", ip->idx);
			}
			break;

		case p4x_swz: {
			char c1 = "xyzw"[ip->idx >> 0 & 3];
			char c2 = "xyzw"[ip->idx >> 2 & 3];
			char c3 = "xyzw"[ip->idx >> 4 & 3];
			char c4 = "xyzw"[ip->idx >> 6 & 3];
			fputfmt(fout, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->idx);
		} break;
	}
}
void fputasm(state rt, FILE* fout, size_t beg, size_t end, int mode) {
	size_t i, is;

	if (end == (size_t)-1) {
		end = rt->_beg - rt->_mem;
	}

	for (i = beg; i < end; i += is) {
		bcde ip = getip(rt, i);
		dbgInfo dbg = getCodeMapping(rt, i);

		switch (ip->opc) {
			error_opc:
				error(rt, NULL, 0, "invalid opcode: %02x `%A`", ip->opc, ip);
				return;

			#define NEXT(__IP, __SP, __CHK) {if (__IP) is = (__IP);}
			#define STOP(__ERR, __CHK, __ERC) if (__CHK) goto __ERR
			#include "code.inl"
		}

		if (dbg != NULL) {
			if (dbg->start == i && dbg->stmt) {
				if ((mode & 0x30) != 0) {
					fputfmt(fout, "%s:%d: [%06x-%06x): %+k\n", dbg->file, dbg->line, dbg->start - 0, dbg->end - 0, dbg->stmt);
				}
				else {
					fputfmt(fout, "%s:%d: %+k\n", dbg->file, dbg->line, dbg->stmt);
				}
			}
		}

		if (mode & 0xf00) {
			fputfmt(fout, "%I", (mode & 0xf00) >> 8);
		}

		switch (mode & 0x30) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return;

			case 0x00: // relative offsets
				fputopc(fout, (void*)ip, mode & 0xf, -1, rt);
				break;

			case 0x10: // local offsets
				fputfmt(fout, "+%06x: ", i - beg);
				fputopc(fout, (void*)ip, mode & 0xf, i - beg, rt);
				break;

			case 0x20: // global offsets
				fputfmt(fout, ".%06x: ", i);
				fputopc(fout, (void*)ip, mode & 0xf, i, rt);
				break;

			case 0x30: // global + local offsets
				fputfmt(fout, ".%06x <+%06x>: ", i, i - beg);
				fputopc(fout, (void*)ip, mode & 0xf, i - beg, rt);
				break;
		}

		fputc('\n', fout);
	}
}

void fputval(state rt, FILE* fout, symn var, stkval* ref, int level, int mode) {
	symn typ = var;
	char* fmt = var->pfmt;


	static int initStatic = 1;
	static struct symNode func;	// for printig only
	static struct symNode type;	// for printig only

	if (initStatic) {
		initStatic = 0;

		type = *rt->defs;
		type.name = "<typename>";

		func = *rt->defs;
		func.name = "<function>";
	}

	if (level > 0) {
		fputfmt(fout, "%I", level);
	}
	else {
		level = -level;
	}

	if (var->kind == TYPE_ref) {
		typ = var->call ? &func : var->type;
		fmt = var->pfmt ? var->pfmt : typ->pfmt;

		//~ fputfmt(fout, "@%06x", vmOffset(rt, ref));
		if (var != typ && var->cast == TYPE_ref && ref != NULL) {	// indirect reference
			//~ fputfmt(fout, "->@%06x", vmOffset(rt, ref));
			ref = getip(rt, ref->u4);
		}
		//~ fputfmt(fout, ": ");
	}
	else if (var->kind == TYPE_def) {
		fmt = NULL;
		mode = 0;
	}
	else if ((void*)var == (void*)ref) {
		typ = var->call ? &func : &type;
		fmt = NULL;
	}
	if (var != typ) {
		int byref = 0;
		if (var->cast == TYPE_ref) {
			if (typ->cast != TYPE_ref) {
				byref = '&';
			}
		}
		if (mode & prQual) {
			fputfmt(fout, "%+T%?c: ", var, byref);
		}
		else {
			fputfmt(fout, "%T%?c: ", var, byref);
		}
		if (var->call) {
			// print return type.
			fputfmt(fout, "%+T: ", var->type);
		}
	}

	if (mode & prType) {
		fputfmt(fout, "%-T(", typ);
	}

	if (ref == NULL) {					// null reference.
		fputfmt(fout, "null");
	}
	else if (!isValidOffset(rt, ref)) {	// invalid offset.
		fputfmt(fout, "BadRef@%06x", var->offs);
	}
	else if (typ == rt->type_str) {
		fputfmt(fout, fmt, ref);
	}
	else if (typ == rt->type_var) {		// TODO: temp only.
		typ = getip(rt, (size_t) ref->var.type);
		ref = getip(rt, (size_t) ref->var.value);
		fputfmt(fout, "%+T, ", typ);
		fputval(rt, fout, typ, ref, level, prType);
	}

	else switch (typ->kind) {
		case TYPE_rec: {
			int n = 0;
			if (fmt != NULL) {
				switch (typ->size) {
					default:
						// there should be no formated(<=> builtin) type matching none of this size.
						fputfmt(fout, "!-!@%?c%06x, size: %d", var->stat ? 0 : '+', var->offs, var->size);
						break;

					case 1:
						if (typ->cast == TYPE_u32) {
							fputfmt(fout, fmt, ref->u1);
						}
						else {
							fputfmt(fout, fmt, ref->i1);
						}
						break;

					case 2:
						if (typ->cast == TYPE_u32) {
							fputfmt(fout, fmt, ref->u2);
						}
						else {
							fputfmt(fout, fmt, ref->i2);
						}
						break;

					case 4:
						if (typ->cast == TYPE_f32) {
							fputfmt(fout, fmt, ref->f4);
						}
						else if (typ->cast == TYPE_u32) {
							// force zero extend (may sign extend to int64 ?).
							fputfmt(fout, fmt, ref->u4);
						}
						else {
							fputfmt(fout, fmt, ref->i4);
						}
						break;

					case 8:
						if (typ->cast == TYPE_f64) {
							fputfmt(fout, fmt, ref->f8);
						}
						else {
							fputfmt(fout, fmt, ref->i8);
						}
						break;
				}
			}
			else if (typ->prms != NULL) {
				symn tmp;
				fputfmt(fout, "{");
				for (tmp = typ->prms; tmp; tmp = tmp->next) {

					if (tmp->stat || tmp->kind != TYPE_ref)
						continue;

					if (tmp->pfmt && !*tmp->pfmt)
						continue;

					if (n > 0) {
						fputfmt(fout, ",");
					}

					fputfmt(fout, "\n");
					fputval(rt, fout, tmp, (void*)((char*)ref + tmp->offs), level + 1, prType);
					n += 1;
				}
				fputfmt(fout, "\n%I}", level);
			}
			else {
				// empty struct.
				fputfmt(fout, "@%?c%06x, size: %d", var->stat ? 0 : '+', var->offs, var->size);
			}
		} break;
		case TYPE_arr: {
			// ArraySize
			int i, n = typ->size;
			symn base = typ->type;
			int elementsOnNewLine = 0;
			int arrayHasMoreElements = 0;

			//~ fputfmt(fout, "@%06x", vmOffset(rt, ref));
			if (typ->cast != TYPE_arr) { // TODO: OR USE: if (typ->stat) {
				// static array
				dieif(n % base->size != 0, "FixMe");
				fputfmt(fout, "[");
				n /= base->size;
			}
			else {
				n = ref->arr.length;
				ref = (stkval*)(rt->_mem + ref->u4);
				fputfmt(fout, "<%d>[", n);
			}

			#ifdef MAX_ARR_PRINT
			if (n > MAX_ARR_PRINT) {
				n = MAX_ARR_PRINT;
				arrayHasMoreElements = 1;
			}
			#endif

			if (base->kind == TYPE_arr) {
				elementsOnNewLine = 1;
			}

			//~ if (base->kind == TYPE_rec && base->prms) {
				//~ elementsOnNewLine = 1;
			//~ }

			for (i = 0; i < n; ++i) {
				if (i > 0) {
					fputfmt(fout, ",");
				}
				if (elementsOnNewLine) {
					fputfmt(fout, "\n");
				}
				else if (i > 0) {
					fputfmt(fout, " ");
				}

				fputval(rt, fout, base, (stkval*)((char*)ref + i * sizeOf(base, 0)), elementsOnNewLine ? level + 1 : -level, 0);
			}

			if (arrayHasMoreElements) {
				if (elementsOnNewLine) {
					fputfmt(fout, ",\n%I...", level + 1);
				}
				else {
					fputfmt(fout, ", ...");
				}
			}

			if (elementsOnNewLine) {
				fputfmt(fout, "\n%I", level);
			}
			fputfmt(fout, "]");
			break;
		}
		case TYPE_def:
			fputfmt(fout, "%+T: %+T", typ, typ->type);
			break;
		default:
			fputfmt(fout, "%+T[ERROR(%t)]", typ, typ->kind);
			break;
	}
	if (mode & prType) {
		fputfmt(fout, ")");
	}
}


static void traceArgs(state rt, FILE *outf, symn fun, char *file, int line, void* sp, int ident) {
	symn sym;
	int printFileLine = 0;

	if (outf == NULL) {
		outf = rt->logf;
	}
	if (file == NULL) {
		file = "native.code";
	}
	fputfmt(outf, "%I%s:%u: %?T", ident, file, line, fun);
	if (ident < 0) {
		printFileLine = 1;
		ident = -ident;
	}

	dieif(sp == NULL, ERR_INTERNAL_ERROR);
	dieif(fun == NULL, ERR_INTERNAL_ERROR);
	if (fun->prms != NULL && fun->prms != rt->defs) {
		int firstArg = 1;
		if (ident > 0) {
			fputfmt(outf, "(");
		}
		else {
			fputfmt(outf, "\n");
		}
		for (sym = fun->prms; sym; sym = sym->next) {
			void *offs;

			/* fun->prms should contain only function parameters.
			if (sym->call)
			continue;

			if (sym->kind != TYPE_ref)
			continue;
			*/

			if (firstArg == 0) {
				fputfmt(outf, ", ");
			}
			else {
				firstArg = 0;
			}

			if (printFileLine) {
				if (sym->file != NULL && sym->line != 0) {
					fputfmt(outf, "%I%s:%u: ", ident, sym->file, sym->line);
				}
				else {
					fputfmt(outf, "%I", ident);
				}
			}
			dieif(sym->stat, ERR_INTERNAL_ERROR);

			// 1 * vm_size holds the return value of the function.
			offs = (char*)sp + fun->prms->offs + 1 * vm_size - sym->offs;

			fputval(rt, outf, sym, offs, -ident, 0);
		}
		if (ident > 0) {
			fputfmt(outf, ")");
		}
		else {
			fputfmt(outf, "\n");
		}
	}
}

int logTrace(state rt, FILE *outf, int ident, int startlevel, int tracelevel) {
	int pos;
	int i, isOutput = 0;
	cell pu = rt->vm.cell;
	trace tr = (trace)pu->bp;

	if (outf == NULL) {
		outf = rt->logf;
	}
	if (rt->dbg == NULL) {
		return 0;
	}
	pos = ((trace)pu->tp) - tr - 1;
	if (tracelevel > pos) {
		tracelevel = pos;
	}
	for (i = startlevel; i < tracelevel; ++i) {
		size_t pc = vmOffset(rt, tr[pos - i].ip);
		dbgInfo trInfo = getCodeMapping(rt, pc);
		signed char *sp = tr[pos - i - 1].sp;
		symn fun = mapsym(rt, pc, 1);
		char *file = NULL;
		int line = 0;

		if (fun == NULL) {
			continue;
		}

		if (trInfo != NULL) {
			file = trInfo->file;
			line = trInfo->line;
		}

		if (isOutput > 0) {
			fputc('\n', outf);
		}
		traceArgs(rt, outf, fun, file, line, sp, ident);
		isOutput += 1;
	}
	if (i < pos) {
		if (isOutput > 0) {
			fputc('\n', outf);
		}
		fputfmt(outf, "%I... %d more", ident, pos - i);
		isOutput += 1;
	}

	if (isOutput) {
		fputc('\n', outf);
	}
	return isOutput;
}

#if defined DEBUGGING
int vmTest() {
	int i, err = 0;
	FILE *out = stdout;
	struct bcde ip[1];
	struct libc libcvec[1];

	memset(ip, 0, sizeof(ip));
	memset(libcvec, 0, sizeof(libcvec));

	for (i = 0; i < opc_last; i++) {
		int IS, CHK, DIFF;
		int NEXT = 0;

		switch (ip->opc = (uint8_t) i) {
			error_opc:
				if (opc_tbl[i].name == NULL) {
					// skip unimplemented instruction.
					continue;
				}
				fprintf(out, "Invalid instruction opc_x%02x: %s\n", i, opc_tbl[i].name);
				err += 1;
				break;

			error_len:
				fputfmt(out, "invalid opcode size 0x%02x: '%A': expected: %d, found: %d\n", i, ip, opc_tbl[i].size, IS);
				err += 1;
				break;

			error_chk:
				fputfmt(out, "stack check 0x%02x: '%A': expected: %d, found: %d\n", i, ip, opc_tbl[i].chck, CHK);
				err += 1;
				break;

			error_dif:
				fputfmt(out, "stack difference 0x%02x: '%A': expected: %d, found: %d\n", i, ip, opc_tbl[i].diff, DIFF);
				err += 1;
				break;

			#define NEXT(__IP, __DIFF, __CHK) {\
				NEXT += 1;\
				IS = __IP;\
				CHK = __CHK;\
				DIFF = __DIFF;\
				if ((__IP) && opc_tbl[i].size != (__IP)) {\
					goto error_len;\
				}\
				if (opc_tbl[i].chck != 9 && opc_tbl[i].chck != (__CHK)) {\
					goto error_chk;\
				}\
				if (opc_tbl[i].diff != 9 && opc_tbl[i].diff != (__DIFF)) {\
					goto error_dif;\
				}\
			}
			#define STOP(__ERR, __CHK, __ERR1) if (__CHK) goto __ERR
			#include "code.inl"
		}

		if (NEXT > 1 && i != opc_spc) {
			fputfmt(out, "More than one NEXT: opcode 0x%02x: '%A'\n", i, ip);
		}
	}
	return err;
}
int vmHelp() {
	int i, err = 0;
	FILE* out = stdout;
	struct bcde ip[1];
	ip->arg.i8 = 0;

	fprintf(out, "\nVirtual machine opcode listing.\n");

	for (i = 0; i < opc_last; i++) {
		switch (i) {
			error_opc:
				if (opc_tbl[i].name == NULL) {
					// skip unimplemented instruction.
					continue;
				}
				fprintf(out, "Invalid instruction opc_x%02x: %s\n", i, opc_tbl[i].name);
				err += 1;
				continue;

			#define NEXT(__IP, __SP, __CHK) if ((__IP) && opc_tbl[i].size != (__IP)) goto error_opc;
			#define STOP(__ERR, __CHK, __ERR1) if (__CHK) goto error_opc
			#include "code.inl"
		}

		fprintf(out, "\nInstruction: %s: #", opc_tbl[i].name);
		fprintf(out, "\nOpcode		Size		Stack trasition");
		fprintf(out, "\n0x%02x		%d		[..., a, b, c, d => [..., a, b, c, d#", opc_tbl[i].code, opc_tbl[i].size);

		fprintf(out, "\n\nDescription");
		fprintf(out, "\n#");
		fprintf(out, "\n\nOperation");
		fprintf(out, "\n#");
		fprintf(out, "\n\nExceptions");
		fprintf(out, "\nNone#");

		fprintf(out, "\n\n");		// end of text
	}
	return err;
}
#endif
