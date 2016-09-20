/*******************************************************************************
 *   File: code.c
 *   Date: 2011/06/23
 *   Description: bytecode related stuff
 *******************************************************************************
code emmiting, executing and formatting
*******************************************************************************/
#include <string.h>
#include <math.h>
#include <time.h>
#include <stddef.h>
#include "internal.h"

#ifdef __WATCOMC__
#pragma disable_message(124);
// needed for opcode f32_mod
static inline double fmodf(float x, float y) {
	return fmod(x, y);
}
#endif

const struct opc_inf opc_tbl[255] = {
	#define OPCDEF(NAME, CODE, SIZE, CHCK, DIFF, MNEM) {CODE, SIZE, CHCK, DIFF, MNEM},
	#include "defs.inl"
};


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

typedef uint8_t *memptr;
typedef uint32_t *stkptr;

/// bytecode instruction
typedef struct bcde *bcde;

/// bytecode processor
typedef struct cell *cell;
typedef struct traceRec *tracePtr;

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
	};
};
#pragma pack(pop)

struct cell {
	memptr ip;		// Instruction pointer

	// Stack
	memptr bp;		// Stack base
	memptr tp;		// Trace pointer +(bp)
	memptr sp;		// Stack pointer -(bp + ss)

	// multiproc
	size_t			ss;			// stack size
	unsigned int	cp;			// child procs (join == 0)
	unsigned int	pp;			// parent proc (main == 0)
};

struct traceRec {
	memptr caller;      // Instruction pointer of caller
	memptr callee;      // Instruction pointer of callee
	clock_t func;       // time when the function was invoked
	clock_t stmt;       // time when the statement execution started
	stkptr sp;          // Stack pointer
};

/// Check if the pointer is inside the vm.
static inline int isValidOffset(rtContext rt, void* ptr) {
	if ((unsigned char*)ptr > rt->_mem + rt->_size) {
		return 0;
	}
	if (ptr && (unsigned char*)ptr < rt->_mem) {
		return 0;
	}
	return 1;
}

static inline void rollbackPc(rtContext rt) {
	bcde ip = getip(rt, rt->vm.pc);
	if ((void*)ip != rt->_beg) {
		rt->_beg = (memptr)ip;
		rt->vm.ss -= opc_tbl[ip->opc].diff;
	}
}

size_t vmOffset(rtContext rt, void* ptr) {
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
static int checkOcp(rtContext rt, size_t offs, vmOpcode opc, stkval *arg) {
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

static int removeOpc(rtContext rt, size_t offs) {
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

static size_t nextIp(rtContext rt, size_t offs) {
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

static int decrementStackAccess(rtContext rt, size_t offsBegin, size_t offsEnd, int count) {
	size_t offs;
	for (offs = offsBegin; offs < offsEnd; offs = nextIp(rt, offs)) {
		register bcde ip = getip(rt, offs);
		switch (ip->opc) {
			default:
				break;
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

int optimizeAssign(rtContext rt, size_t offsBegin, size_t offsEnd) {
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
size_t emitarg(rtContext rt, vmOpcode opc, stkval arg) {
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
	if (rt->fastInstr) {
		if (0) {}

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
			else if (ip->opc == opc_lref) {
				arg.i8 = ip->arg.u4;
				if (arg.i8 == arg.rel) {
					opc = opc_ld32;
					rollbackPc(rt);
				}
			}
		}
		else if (opc == opc_sti4) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set1;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref) {
				arg.i8 = ip->arg.u4;
				if (arg.i8 == arg.rel) {
					opc = opc_st32;
					rollbackPc(rt);
				}
			}
		}

		else if (opc == opc_ldi8) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_dup2;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref) {
				arg.i8 = ip->arg.u4;
				if (arg.i8 == arg.rel) {
					opc = opc_ld64;
					rollbackPc(rt);
				}
			}
		}
		else if (opc == opc_sti8) {
			ip = getip(rt, rt->vm.pc);
			if (ip->opc == opc_ldsp && ((ip->rel & (vm_size-1)) == 0) && ((ip->rel / vm_size) < vm_regs)) {
				arg.i4 = ip->rel / vm_size;
				opc = opc_set2;
				rollbackPc(rt);
			}
			else if (ip->opc == opc_lref) {
				arg.i8 = ip->arg.u4;
				if (arg.i8 == arg.rel) {
					opc = opc_st64;
					rollbackPc(rt);
				}
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
		logif((ip->rel + rt->vm.pc) != arg.sz, "FixMe");
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
			error(rt, NULL, 0, "invalid opcode: %.A @%06x", ip, rt->vm.pc);
			return 0;

		error_stc:
			error(rt, NULL, 0, "stack underflow(%d): %.A @%06x", rt->vm.ss, ip, rt->vm.pc);
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
int fixjump(rtContext rt, int src, int dst, int stc) {
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
static inline int sync(cell pu, int cp, int wait) {
	int pp = pu[cp].pp;
	logProc(pu, cp, "join");

	// slave proc
	if (pp != cp) {
		if (pu[cp].cp == 0) {
			//~ trace("slave(%d): stop.", cp);
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

/** manage function call stack traces
 * @param rt runtime context
 * @param sp tack pointer (for tracing arguments
 * @param caller the address of the caller, NULL on start and end.
 * @param callee the called function adress, -1 on leave.
 * @param extra TO BE REMOVED
 */
static inline int traceCall(rtContext rt, void* sp, void* caller, void* callee) {
	clock_t now = clock();
	cell pu = rt->vm.cell;
	if (rt->dbg == NULL) {
		return 0;
	}
	// trace execute statement
	if (callee == NULL) {
		fatal(ERR_INTERNAL_ERROR);
	}
	// trace leave function
	else if ((ptrdiff_t)callee < 0) {
		tracePtr tp;
		int recursive = 0;
		if (pu->tp - pu->bp < (ptrdiff_t)sizeof(struct traceRec)) {
			debug("tp: %d - sp: %d", pu->tp - pu->bp, pu->sp - pu->bp);
			return 0;
		}
		pu->tp -= sizeof(struct traceRec);

		for (tp = (tracePtr)pu->bp; tp < (tracePtr)pu->tp; tp++) {
			if (tp->callee == ((tracePtr)pu->tp)->callee) {
				recursive = 1;
				break;
			}
		}
		tp = (tracePtr)pu->tp;

		clock_t ticks = now - tp->func;
		size_t callerOffs = vmOffset(rt, tp->caller);
		dbgn calleeFunc = mapDbgFunction(rt, vmOffset(rt, tp->callee));
		dbgn callerFunc = mapDbgFunction(rt, callerOffs);
		//dbgn callerStmt = mapDbgStatement(rt, callerOffs);
		if (calleeFunc != NULL) {
			calleeFunc->exec += 1;
			if (!recursive) {
				calleeFunc->total += ticks;
			}
			calleeFunc->self += ticks;
		}
		if (callerFunc != NULL) {
			callerFunc->self -= ticks;
		}
		/* TODO: update caller statement times
		if (callerStmt != NULL) {
			if (!recursive) {
				callerStmt->self -= ticks;
			}
			else {
				callerStmt->total -= ticks;
			}
		}*/
		if (rt->dbg->profile != NULL) {
			rt->dbg->profile(rt->dbg, (pu->tp - pu->bp) / sizeof(struct traceRec), caller, callee, now);
		}
	}
	// trace enter function
	else {
		tracePtr tp = (tracePtr)pu->tp;
		if (ovf(pu)) {
			debug("tp: %d - sp: %d", pu->tp - pu->bp, pu->sp - pu->bp);
			return 0;
		}
		tp->caller = caller;
		tp->callee = callee;
		tp->func = now;
		tp->sp = sp;
		dbgn calleeFunc = mapDbgFunction(rt, vmOffset(rt, tp->callee));
		if (calleeFunc != NULL) {
			calleeFunc->hits += 1;
		}

		if (rt->dbg->profile != NULL) {
			rt->dbg->profile(rt->dbg, (pu->tp - pu->bp) / sizeof(struct traceRec), caller, callee, now);
		}
		pu->tp += sizeof(struct traceRec);
	}
	return 1;
}

/// Private dummy debug function.
static int dbgDummy(dbgContext ctx, vmError err, size_t ss, void* sp, void* ip) {
	if (err != noError) {
		char *errorStr = NULL;
		switch (err) {
			case noError:
				break;

			case invalidIP:
				errorStr = "Invalid instruction pointer";
				break;

			case invalidSP:
				errorStr = "Invalid stack pointer";
				break;

			case illegalInstruction:
				errorStr = "Illegal instruction";
				break;

			case stackOverflow:
				errorStr = "Stack overflow";
				break;

			case divisionByZero:
				errorStr = "Division by zero";
				break;

			case libCallAbort:
			case executionAborted:
				errorStr = "Native call aborted";
				break;

			case memReadError:
			case memWriteError:
				errorStr = "Invalid memory acces";
				break;
		}
		error(ctx->rt, NULL, 0, "%s executing instruction: %.A @%06x", errorStr, ip, vmOffset(ctx->rt, ip));
		return executionAborted;
	}
	(void)sp;
	(void)ss;
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
static vmError exec(rtContext rt, cell pu, symn fun, void *extra) {
	const int cc = 1;
	const libc libcvec = rt->vm.libv;

	vmError execError = noError;
	const size_t ms = rt->_size;			// memory size
	const size_t ro = rt->vm.ro;			// read only region
	const memptr mp = (void*)rt->_mem;
	const stkptr st = (void*)(pu->bp + pu->ss);

	// run in debug of profile mode
	if (rt->dbg != NULL) {
		const void *oldTP = pu->tp;
		const stkptr spMin = (stkptr)(pu->bp);
		const stkptr spMax = (stkptr)(pu->bp + pu->ss);
		const bcde ipMin = (bcde)(rt->_mem + rt->vm.ro);
		const bcde ipMax = (bcde)(rt->_mem + rt->vm.px + px_size);

		int (*dbg)(dbgContext, vmError, size_t, void*, void*) = rt->dbg->debug;

		if (dbg == NULL) {
			dbg = dbgDummy;
		}
		// invoked function(from external code) will return with a ret instruction, removing trace info
		traceCall(rt, pu->sp, NULL, getip(rt, fun->offs));

		for ( ; ; ) {
			register const bcde ip = (bcde)pu->ip;
			register const stkptr sp = (stkptr)pu->sp;

			const tracePtr tp = (tracePtr)pu->tp - 1;
			const size_t pc = vmOffset(rt, ip);
			const dbgn stmt = NULL;//mapDbgStatement(rt, pc);

			if (ip >= ipMax || ip < ipMin) {
				dbg(rt->dbg, invalidIP, st - sp, sp, ip);
				return invalidIP;
			}
			if (sp > spMax || sp < spMin) {
				dbg(rt->dbg, invalidSP, st - sp, sp, ip);
				return invalidSP;
			}
			if (dbg(rt->dbg, noError, st - sp, sp, ip) != 0) {
				// abort execution from debuging
				return executionAborted;
			}
			if (stmt != NULL) {
				if (pc == stmt->start) {
					clock_t now = clock();
					stmt->hits += 1;
					tp->stmt = now;
					/* TODO: remove: print start executing statement
					symn sym = NULL;
					int symOffs = 0;
					if (stmt != NULL) {
						if (sym == NULL) {
							sym = rtFindSym(rt, stmt->start, 1);
						}
						if (sym != NULL) {
							symOffs = stmt->start - sym->offs;
						}
					}
					//~ fputfmt(stdout, "%s:%?u:enter[%06x, %06x): %06x <%?T+%d>: %A, hits(%D), time: %D\n", stmt->file, stmt->line, stmt->start, stmt->end, vmOffset(rt, ip), sym, symOffs, ip, stmt->hits, (int64_t)now);
					fputfmt(stdout, "%s:%?u:enter@%D <%?T+%d> %.A\n", stmt->file, stmt->line, (int64_t)now, sym, symOffs, ip);
					//~ */
				}
			}
			switch (ip->opc) {
				dbg_stop_vm:	// halt virtual machine
					dbg(rt->dbg, execError, st - sp, sp, ip);
					while (pu->tp != oldTP) {
						traceCall(rt, NULL, NULL, (void*)-2);
						//pu->tp = oldTP;	// during exec we may return from code.
					}
					return execError;

				dbg_error_opc:
					execError = illegalInstruction;
					goto dbg_stop_vm;

				dbg_error_ovf:
				dbg_error_trace_ovf:
					execError = stackOverflow;
					goto dbg_stop_vm;

				dbg_error_mem_read:
					execError = memReadError;
					goto dbg_stop_vm;

				dbg_error_mem_write:
					execError = memWriteError;
					goto dbg_stop_vm;

				dbg_error_div_flt:
					dbg(rt->dbg, divisionByZero, st - sp, sp, ip);
					// continue execution on floating point division by zero.
					break;

				dbg_error_div:
					execError = divisionByZero;
					goto dbg_stop_vm;

				dbg_error_libc:
					execError = libCallAbort;
					goto dbg_stop_vm;

				#define NEXT(__IP, __SP, __CHK) pu->sp -= vm_size * (__SP); pu->ip += (__IP);
				#define STOP(__ERR, __CHK, __ERC) do {if (__CHK) {goto dbg_##__ERR;}} while(0)
				#define EXEC
				#define TRACE(__SP, __CALLER, __CALLEE) do { if (!traceCall(rt, __SP, __CALLER, __CALLEE)) goto dbg_error_trace_ovf; } while(0)
				#include "code.inl"
			}
			if (stmt != NULL) {
				const tracePtr tp2 = (tracePtr)pu->tp - 1;
				if ((memptr)ip != tp2->caller) {
					size_t pc = vmOffset(rt, pu->ip);
					if (pc < stmt->start || pc >= stmt->end) {
						clock_t now = clock();
						stmt->exec += 1;
						stmt->total += now - tp->stmt;
						/* TODO: remove: print end executing statement
						symn sym = NULL;
						int symOffs = 0;
						if (sym == NULL) {
							sym = rtFindSym(rt, stmt->start, 1);
						}
						if (sym != NULL) {
							symOffs = stmt->start - sym->offs;
						}
						//~ fputfmt(stdout, "%s:%?u:leave[%06x, %06x): %06x <%?T+%d>: %A, hits(%D), time: %D - %D = %D\n", stmt->file, stmt->line, stmt->start, stmt->end, vmOffset(rt, ip), sym, symOffs, ip, stmt->hits, (int64_t)now, (int64_t)tp->stmt, (int64_t)now - (int64_t)tp->stmt);
						fputfmt(stdout, "%s:%?u:leave@%D: %D <%?T+%d> %.A\n", stmt->file, stmt->line, (int64_t)now, (int64_t)(now - tp->stmt), sym, symOffs, ip);
						sym = NULL;
						// */
					}
				}
			}
		}
		return execError;
	}

	// code for maximum execution speed
	for ( ; ; ) {
		register const bcde ip = (bcde)pu->ip;
		register const stkptr sp = (stkptr)pu->sp;
		switch (ip->opc) {
			stop_vm:	// halt virtual machine
				if (execError != noError) {
					struct dbgContextRec dbg = { .rt = rt };
					dbgDummy(&dbg, execError, st - sp, sp, ip);
				}
				return execError;

			error_opc:
				execError = illegalInstruction;
				goto stop_vm;

			error_ovf:
				execError = stackOverflow;
				goto stop_vm;

			error_mem_read:
				execError = memReadError;
				goto stop_vm;

			error_mem_write:
				execError = memWriteError;
				goto stop_vm;

			error_div_flt: {
					struct dbgContextRec dbg = { .rt = rt };
					dbgDummy(&dbg, divisionByZero, st - sp, sp, ip);
				}
				// continue execution on floating point division by zero.
				break;

			error_div:
				execError = divisionByZero;
				goto stop_vm;

			error_libc:
				execError = libCallAbort;
				goto stop_vm;

			#define NEXT(__IP, __SP, __CHK) {pu->sp -= vm_size * (__SP); pu->ip += (__IP);}
			#define STOP(__ERR, __CHK, __ERC) do {if (__CHK) {goto __ERR;}} while(0)
			#define TRACE(__SP, __CALLER, __CALLEE)
			#define EXEC
			#include "code.inl"
		}
	}
	return execError;
}


vmError invoke(rtContext rt, symn fun, void* res, void* args, void* extra) {
	cell pu = rt->vm.cell;
	unsigned char *ip = pu->ip;
	unsigned char *sp = pu->sp;
	unsigned char *tp = pu->tp;

	// result is the first param
	// TODO: ressize = fun->prms->size;
	size_t ressize = sizeOf(fun->type, 1);
	void* resp = NULL;
	vmError result;

	// invoked symbol must be a function reference
	dieif(fun->kind != CAST_ref || !fun->call, "FixMe");

	// result is the last argument.
	resp = pu->sp - ressize;

	// make space for result and arguments// result is the first param
	pu->sp -= fun->prms->offs;

	if (args != NULL) {
		memcpy(pu->sp, args, fun->prms->offs - ressize);
	}

	// return here: vm->px: program exit
	*(stkptr)(pu->sp -= vm_size) = rt->vm.px;

	pu->ip = getip(rt, fun->offs);

	result = exec(rt, pu, fun, extra);
	if (result == noError && res != NULL) {
		memcpy(res, resp, (size_t) ressize);
	}

	pu->ip = ip;
	pu->sp = sp;	//
	if (pu->tp != tp) {
		fatal(ERR_INTERNAL_ERROR);
		pu->tp = tp;	// during exec we may return from code.
	}

	return result;
}
vmError execute(rtContext rt, size_t ss, void *extra) {
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

	rt->vm.ss = ss;
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
		return stackOverflow;
	}

	return exec(rt, pu, rt->main, extra);
}

void fputAsm(FILE *out, const char *esc[], rtContext rt, void *ptr, int mode) {
	bcde ip = (bcde)ptr;
	size_t i, len = (size_t) mode & prAsmCode;
	size_t offs = (size_t)ptr;
	char *fmt_addr = " .%06x"; // "0x%08x";
	char *fmt_offs = " <%.T+%d>";
	symn sym = NULL;

	if (rt != NULL) {
		offs = vmOffset(rt, ptr);
		if (mode & prAsmAddr && mode & prAsmName) {
			sym = rtFindSym(rt, offs, 0);
		}
	}

	if (mode & prAsmAddr) {
		if (sym != NULL) {
			size_t symOffs = offs - sym->offs;
			fputFmt(out, esc, fmt_offs + 1, sym, symOffs);

			int paddLen = 3 + (symOffs != 0);
			while (symOffs > 0) {
				symOffs /= 10;
				paddLen -= 1;
			}
			if (paddLen < 0) {
				paddLen = 0;
			}
			fputFmt(out, esc, "% I", paddLen);
		}
		else {
			fputFmt(out, esc, fmt_addr + 1, offs);
		}
		fputFmt(out, esc, ": ");
	}

	//~ write data as bytes
	//~ TODO: simplify this
	if (len > 1 && len < opc_tbl[ip->opc].size) {
		for (i = 0; i < len - 2; i++) {
			if (i < opc_tbl[ip->opc].size) {
				fputFmt(out, esc, "%02x ", ((unsigned char*)ptr)[i]);
			}
			else {
				fputFmt(out, esc, "   ");
			}
		}
		if (i < opc_tbl[ip->opc].size) {
			fputFmt(out, esc, "%02x... ", ((unsigned char*)ptr)[i]);
		}
	}
	else {
		for (i = 0; i < len; i++) {
			if (i < opc_tbl[ip->opc].size) {
				fputFmt(out, esc, "%02x ", ((unsigned char*)ptr)[i]);
			}
			else {
				fputFmt(out, esc, "   ");
			}
		}
	}

	if (opc_tbl[ip->opc].name != NULL) {
		fputFmt(out, esc, "%s", opc_tbl[ip->opc].name);
	}
	else {
		fputFmt(out, esc, "opc.x%02x", ip->opc);
	}

	switch (ip->opc) {
		default:
			break;

		case opc_inc:
		case opc_spc:
		case opc_ldsp:
		case opc_move:
		case opc_mad:
			fputFmt(out, esc, " %d", ip->rel);
			break;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
		case opc_task: {
			if (ip->opc == opc_task) {
				fputFmt(out, esc, " %d,", ip->dl);
				i = ip->cl;
			}
			else {
				i = (size_t) ip->rel;
			}
			if (sym != NULL && mode & prAsmAddr) {
				fputFmt(out, esc, fmt_offs, sym, offs + i - sym->offs);
			}
			else if (mode & prAsmAddr) {
				fputFmt(out, esc, fmt_addr, offs + i);
			}
			else {
				fputFmt(out, esc, " %+d", i);
			}
			break;
		}

		case opc_sync:
			fputFmt(out, esc, " %d", ip->idx);
			break;

		case b32_bit: switch (ip->idx & 0xc0) {
			default:
				fatal(ERR_INTERNAL_ERROR);
				return;

			case b32_bit_and:
				fputFmt(out, esc, "%s 0x%03x", "and", (1 << (ip->idx & 0x3f)) - 1);
				break;

			case b32_bit_shl:
				fputFmt(out, esc, "%s 0x%03x", "shl", ip->idx & 0x3f);
				break;

			case b32_bit_shr:
				fputFmt(out, esc, "%s 0x%03x", "shr", ip->idx & 0x3f);
				break;

			case b32_bit_sar:
				fputFmt(out, esc, "%s 0x%03x", "sar", ip->idx & 0x3f);
				break;
		} break;

		case opc_dup1:
		case opc_dup2:
		case opc_dup4:
		case opc_set1:
		case opc_set2:
		case opc_set4:
			fputFmt(out, esc, " sp(%d)", ip->idx);
			break;

		case opc_lc32:
			fputFmt(out, esc, " %d", ip->arg.i4);
			break;

		case opc_lc64:
			fputFmt(out, esc, " %D", ip->arg.i8);
			break;

		case opc_lf32:
			fputFmt(out, esc, " %f", ip->arg.f4);
			break;

		case opc_lf64:
			fputFmt(out, esc, " %F", ip->arg.f8);
			break;

		case opc_ld32:
		case opc_st32:
		case opc_ld64:
		case opc_st64:
		case opc_lref:
			if (ip->opc == opc_lref) {
				offs = ip->arg.u4;
			}
			else {
				offs = (size_t) ip->rel;
			}

			fputFmt(out, esc, fmt_addr, offs);
			if (rt != NULL) {
				symn sym = rtFindSym(rt, offs, 0);
				if (sym != NULL) {
					fputFmt(out, esc, " ;%T%?+d", sym, offs - sym->offs);
				}
				else if (rt->cc != NULL) {
					char *str = getip(rt, offs);
					for (i = 0; i < TBLS; i += 1) {
						list lst;
						for (lst = rt->cc->strt[i]; lst; lst = lst->next) {
							if (str == (char*)lst->data) {
								fputFmt(out, esc, " ;\"%s\"", str);
								i = TBLS;
								break;
							}
						}
					}
				}
			}
			break;

		case opc_libc:
			offs = (size_t) ip->rel;
			fputFmt(out, esc, "(%d)", offs);

			if (rt != NULL) {
				libc lc = NULL;
				if (rt->cc != NULL) {
					for (lc = rt->cc->libc; lc; lc = lc->next) {
						if (lc->pos == offs) {
							break;
						}
					}
				}
				else if (rt->vm.libv) {
					lc = &((libc)rt->vm.libv)[offs];
				}
				if (lc && lc->sym) {
					fputFmt(out, esc, " ;%T", lc->sym);
				}
			}
			break;

		case p4x_swz: {
			char c1 = "xyzw"[ip->idx >> 0 & 3];
			char c2 = "xyzw"[ip->idx >> 2 & 3];
			char c3 = "xyzw"[ip->idx >> 4 & 3];
			char c4 = "xyzw"[ip->idx >> 6 & 3];
			fputFmt(out, esc, " %c%c%c%c(%02x)", c1, c2, c3, c4, ip->idx);
		} break;
	}
}

void fputVal(FILE *out, const char *esc[], rtContext rt, symn var, stkval *ref, int mode, int indent) {
	symn typ = var;
	const char *fmt = var->pfmt;

	static int initStatic = 1;
	static struct symNode func;	// for printig only
	static struct symNode type;	// for printig only

	if (initStatic) {
		initStatic = 0;

		type = *rt->main->flds;
		type.name = "<typename>";

		func = *rt->main->flds;
		func.name = "<function>";
	}

	if (indent > 0) {
		fputFmt(out, esc, "%I", indent);
	}
	else {
		indent = -indent;
	}

	if (var->kind == CAST_ref) {
		typ = var->call ? &func : var->type;
		fmt = var->pfmt ? var->pfmt : typ->pfmt;

		//~ fputfmt(out, "@%06x", vmOffset(rt, ref));
		if (var != typ && var->cast == CAST_ref && ref != NULL) {	// indirect reference
			ref = getip(rt, ref->u4);
			//~ fputfmt(out, "->@%06x", vmOffset(rt, ref));
		}
		//~ fputfmt(out, ": ");
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
		if (var->cast == CAST_ref) {
			if (typ->cast != CAST_ref) {
				byref = '&';
			}
		}
		fputFmt(out, esc, "%.*T%?c: ", mode & ~prSymType, var, byref);
	}

	if (mode & prSymType) {
		fputFmt(out, esc, "%T(", typ);
	}

	// invalid offset.
	if (!isValidOffset(rt, ref)) {
		fputFmt(out, esc, "BadRef@%06x", var->offs);
	}
	// null reference.
	else if (ref == NULL) {
		fputFmt(out, esc, "null");
	}
	// builtin type
	else if (fmt != NULL) {
		if (fmt == type_fmt_variant) {
			typ = getip(rt, (size_t) ref->var.type);
			ref = getip(rt, (size_t) ref->var.value);
			fputFmt(out, esc, type_fmt_variant, typ);
			fputVal(out, esc, rt, typ, ref, mode & ~prSymQual, indent);
		}
		else if (fmt == type_fmt_string) {
			fputFmt(out, esc, fmt, ref);
		}
		else switch (typ->size) {
			default:
				// there should be no formated(<=> builtin) type matching none of this size.
				fputFmt(out, esc, "!-!@%?c%06x, size: %d", var->stat ? 0 : '+', var->offs, var->size);
				break;

			case 1:
				if (typ->cast == CAST_u32) {
					fputFmt(out, esc, fmt, ref->u1);
				}
				else {
					fputFmt(out, esc, fmt, ref->i1);
				}
				break;

			case 2:
				if (typ->cast == CAST_u32) {
					fputFmt(out, esc, fmt, ref->u2);
				}
				else {
					fputFmt(out, esc, fmt, ref->i2);
				}
				break;

			case 4:
				if (typ->cast == CAST_f32) {
					fputFmt(out, esc, fmt, ref->f4);
				}
				else if (typ->cast == CAST_u32) {
					// force zero extend (may sign extend to int64 ?).
					fputFmt(out, esc, fmt, ref->u4);
				}
				else {
					fputFmt(out, esc, fmt, ref->i4);
				}
				break;

			case 8:
				if (typ->cast == CAST_f64) {
					fputFmt(out, esc, fmt, ref->f8);
				}
				else {
					fputFmt(out, esc, fmt, ref->i8);
				}
				break;
		}
	}
	// disable deep printing
	else if (indent > LOG_MAX_ITEMS) {
		fputFmt(out, esc, "???");
	}
	else switch (typ->kind) {
		case TYPE_rec: {
			if (typ->prms != NULL) {
				int n = 0;
				symn fld = typ->prms;
				fputFmt(out, esc, "{");
				for (; fld; fld = fld->next) {

					if (fld->stat || fld->kind != CAST_ref)
						continue;

					if (fld->pfmt && !*fld->pfmt)
						continue;

					if (n > 0) {
						fputFmt(out, esc, ",");
					}

					fputFmt(out, esc, "\n");
					fputVal(out, esc, rt, fld, (void *) ((char *) ref + fld->offs), mode & ~prSymQual, indent + 1);
					n += 1;
				}
				if (n > 0) {
					fputFmt(out, esc, "\n%I}", indent);
				}
				else {
					fputFmt(out, esc, "}");
				}
			}
			else {
				// empty struct, typename, function, pointer
				size_t offs = vmOffset(rt, ref);
				symn sym = rtFindSym(rt, offs, 0);
				if (sym != NULL) {
					offs = offs - sym->offs;
				}
				fputFmt(out, esc, "<%?.T%?+d@%06x>", sym, (int32_t)offs, vmOffset(rt, ref));
			}
		} break;
		case CAST_arr: {
			// ArraySize
			int i, n = typ->size;
			symn base = typ->type;
			int elementsOnNewLine = 0;
			int arrayHasMoreElements = 0;

			//~ fputFmt(out, esc, "@%06x", vmOffset(rt, ref));
			if (typ->cast != CAST_arr) { // TODO: OR USE: if (typ->stat) {
				// fixed size array
				dieif(n % base->size != 0, "FixMe");
				if (base->pfmt == type_fmt_character) {
					fputFmt(out, esc, type_fmt_string, ref);
					break;
				}
				fputFmt(out, esc, "[");
				n /= base->size;
			}
			else {
				n = ref->arr.length;
				ref = (stkval*)(rt->_mem + ref->u4);
				fputFmt(out, esc, "<%d>[", n);
			}

			#ifdef LOG_MAX_ITEMS
			if (n > LOG_MAX_ITEMS) {
				n = LOG_MAX_ITEMS;
				arrayHasMoreElements = 1;
			}
			#endif

			if (base->kind == CAST_arr) {
				elementsOnNewLine = 1;
			}

			for (i = 0; i < n; ++i) {
				if (i > 0) {
					fputFmt(out, esc, ",");
				}
				if (elementsOnNewLine) {
					fputFmt(out, esc, "\n");
				}
				else if (i > 0) {
					fputFmt(out, esc, " ");
				}

				fputVal(out, esc, rt, base, (stkval *) ((char *) ref + i * sizeOf(base, 0)), 0, elementsOnNewLine ? indent + 1 : -indent);
			}

			if (arrayHasMoreElements) {
				if (elementsOnNewLine) {
					fputFmt(out, esc, ",\n%I...", indent + 1);
				}
				else {
					fputFmt(out, esc, ", ...");
				}
			}

			if (elementsOnNewLine) {
				fputFmt(out, esc, "\n%I", indent);
			}
			fputFmt(out, esc, "]");
			break;
		}
		case TYPE_def:
			fputFmt(out, esc, "%T: %+T", typ, typ->type);
			break;
		default:
			fputFmt(out, esc, "%+T[ERROR(%K)]", typ, typ->kind);
			break;
	}
	if (mode & prSymType) {
		fputFmt(out, esc, ")");
	}
}

static void traceArgs(rtContext rt, FILE *out, symn fun, char *file, int line, void* sp, int ident) {
	symn sym;
	int printFileLine = 0;

	if (out == NULL) {
		out = rt->logFile;
	}
	if (file == NULL) {
		file = "native.code";
	}
	fputfmt(out, "%I%s:%u: %?T", ident, file, line, fun);
	if (ident < 0) {
		printFileLine = 1;
		ident = -ident;
	}

	dieif(sp == NULL, ERR_INTERNAL_ERROR);
	dieif(fun == NULL, ERR_INTERNAL_ERROR);
	if (fun->prms != NULL/* && fun->prms != rt->vars*/) {
		int firstArg = 1;
		if (ident > 0) {
			fputfmt(out, "(");
		}
		else {
			fputfmt(out, "\n");
		}
		for (sym = fun->prms; sym; sym = sym->next) {
			char *offs = (char*)sp;

			if (firstArg == 0) {
				fputfmt(out, ", ");
			}
			else {
				firstArg = 0;
			}

			if (printFileLine) {
				if (sym->file != NULL && sym->line != 0) {
					fputfmt(out, "%I%s:%u: ", ident, sym->file, sym->line);
				}
				else {
					fputfmt(out, "%I", ident);
				}
			}
			dieif(sym->stat, ERR_INTERNAL_ERROR);

			if (fun->kind == CAST_ref) {
				// vm_size holds the return value of the function.
				offs += vm_size + fun->prms->offs - sym->offs;
				fputVal(out, NULL, rt, sym, (void *) offs, 0, -ident);
			}
			else {
				// TODO: find the offsets of arguments for libcalls.
				//offs += sym->offs;
				//fputVal(rt, out, sym, (void*)offs, -ident, 0);
				fputfmt(out, "%.T: %.T", sym, sym->type);
			}
		}
		if (ident > 0) {
			fputfmt(out, ")");
		}
		else {
			fputfmt(out, "\n");
		}
	}
}

void logTrace(dbgContext dbg, FILE *out, int indent, int startLevel, int traceLevel) {
	int i, isOutput = 0;
	size_t pos;
	if (dbg == NULL) {
		return;
	}

	rtContext rt = dbg->rt;
	cell pu = rt->vm.cell;
	tracePtr tr = (tracePtr)pu->bp;

	if (out == NULL) {
		out = rt->logFile;
	}

	pos = ((tracePtr)pu->tp) - tr - 1;
	if (pos < 1) {
		// TODO: check why we have 0
		pos = 1;
	}
	if (traceLevel > (int) pos) {
		traceLevel = (int) pos;
	}
	for (i = startLevel; i < traceLevel; ++i) {
		dbgn trInfo = mapDbgStatement(rt, vmOffset(rt, tr[pos - i].caller));
		symn fun = rtFindSym(rt, vmOffset(rt, tr[pos - i - 1].callee), 1);
		stkptr sp = tr[pos - i - 1].sp;
		char *file = NULL;
		int line = 0;

		dieif(fun == NULL, ERR_INTERNAL_ERROR);

		if (trInfo != NULL) {
			file = trInfo->file;
			line = trInfo->line;
		}

		if (isOutput > 0) {
			fputc('\n', out);
		}
		traceArgs(rt, out, fun, file, line, sp, indent);
		isOutput += 1;
	}
	if (i < (int) pos) {
		if (isOutput > 0) {
			fputc('\n', out);
		}
		fputfmt(out, "%I... %d more", indent, pos - i);
		isOutput += 1;
	}

	if (isOutput) {
		fputc('\n', out);
	}
}

int vmSelfTest() {
	int i, err = 0;
	int test = 0, skip = 0;
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
					skip += 1;
					// skip unimplemented instruction.
					continue;
				}
				fprintf(out, "Invalid instruction opc_x%02x: %s\n", i, opc_tbl[i].name);
				err += 1;
				break;

			error_len:
				fputfmt(out, "invalid opcode size 0x%02x: '%.A': expected: %d, found: %d\n", i, ip, opc_tbl[i].size, IS);
				err += 1;
				break;

			error_chk:
				fputfmt(out, "stack check 0x%02x: '%.A': expected: %d, found: %d\n", i, ip, opc_tbl[i].chck, CHK);
				err += 1;
				break;

			error_dif:
				fputfmt(out, "stack difference 0x%02x: '%.A': expected: %d, found: %d\n", i, ip, opc_tbl[i].diff, DIFF);
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
				if (opc_tbl[i].chck != 9 && opc_tbl[i].chck != (int)(__CHK)) {\
					goto error_chk;\
				}\
				if (opc_tbl[i].diff != 9 && opc_tbl[i].diff != (__DIFF)) {\
					goto error_dif;\
				}\
			}
			#define STOP(__ERR, __CHK, __ERR1) if (__CHK) goto __ERR
			#include "code.inl"
		}

		test += 1;
		if (NEXT > 1 && i != opc_spc) {
			fputfmt(out, "More than one NEXT: opcode 0x%02x: '%.A'\n", i, ip);
		}
	}
	fputfmt(out, "vmSelfTest finished with %d errors from %d tested and %d skiped instructions\n", err, test, skip);
	return err;
}
