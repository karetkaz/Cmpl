#include "printer.h"
#include "util.h"
#include "code.h"

/// Retrieves the instruction pointer (IP) of the last emitted opcode
static vmInstruction lastIp(rtContext ctx) {
	return (vmInstruction) (ctx->_mem + ctx->vm.pc);
}

/// Determines whether a given offset con be converted to a shorter form, like: `dup`, `set`, `mov`
static int fitStackReg(size_t offs) {
	return ((offs & (vm_stk_align - 1)) == 0) && ((offs / vm_stk_align) <= vm_regs);
}

size_t emitOpc(rtContext ctx, vmOpcode opc, vmValue arg) {
	libc *nativeCalls = ctx->vm.nfc;
	vmInstruction ip = (vmInstruction)ctx->_beg;

	dieif((unsigned char*)ip + 16 >= ctx->_end, ERR_MEMORY_OVERRUN);

	if (opc == markIP) {
		return ctx->_beg - ctx->_mem;
	}

	// load / store
	else if (opc == opc_ldi) {
		dieif (arg.i32 != arg.i64, ERR_INTERNAL_ERROR);
		switch (arg.i32) {
			case 1:
				opc = opc_ldis1;
				break;

			case 2:
				opc = opc_ldis2;
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
				if (!emitInt(ctx, opc_ldsp, 4 - arg.i32)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return 0;
				}
				// copy
				if (!emitInt(ctx, opc_move, -arg.i32)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return 0;
				}
				// create n bytes on stack
				opc = opc_spc;
				break;
		}
	}
	else if (opc == opc_sti) {
		dieif (arg.i32 != arg.i64, ERR_INTERNAL_ERROR);
		switch (arg.i32) {
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
				if (!emitInt(ctx, opc_ldsp, vm_ref_size)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return 0;
				}
				// copy
				if (!emitInt(ctx, opc_move, arg.i32)) {
					dbgTrace(ERR_INTERNAL_ERROR);
					return 0;
				}
				ip = lastIp(ctx);
				if (ip->opc == opc_ldsp) {
					//~ if we copy from the stack to the stack
					//~ do not remove more elements than we copy
					if (arg.i64 > ip->rel) {
						arg.i64 = ip->rel;
					}
				}
				// remove n bytes from stack
				arg.i64 = -arg.i64;
				opc = opc_spc;
				break;
		}
	}
	else if (opc == opc_drop) {
		opc = opc_spc;
		arg.i64 = -arg.i64;
	}

	if (opc >= opc_last) {
		dbgTrace("invalid opc_x%x", opc);
		return 0;
	}

	// Optimize
	switch (opc) {
		default:
			break;

		case opc_lc32:
		case opc_lf32:
			if (!ctx->foldInstr) {
				break;
			}
			if (arg.i64 == 0) {
				opc = opc_lzx1;
				ip = lastIp(ctx);
				if (ip->opc == opc_lzx1 && rollbackPc(ctx)) {
					opc = opc_lzx2;
				}
			}
			break;

		case opc_lc64:
		case opc_lf64:
			if (!ctx->foldInstr) {
				break;
			}
			if (arg.i64 == 0) {
				opc = opc_lzx2;
				ip = lastIp(ctx);
				if (ip->opc == opc_lzx2 && rollbackPc(ctx)) {
					opc = opc_lzx4;
				}
			}
			break;

		case opc_ldi4:
			if (!ctx->foldMemory) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_ldsp) {
				if (!fitStackReg(ip->rel)) {
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = ip->rel / vm_stk_align;
				opc = opc_dup1;
			}
			else if (ip->opc == opc_lref) {
				arg.i64 = ip->arg.u32;
				if (arg.i64 == arg.i24 && rollbackPc(ctx)) {
					opc = opc_ld32;
				}
			}
			break;

		case opc_sti4:
			if (!ctx->foldMemory) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_ldsp) {
				if (!fitStackReg(ip->rel)) {
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = ip->rel / vm_stk_align;
				opc = opc_set1;
			}
			else if (ip->opc == opc_lref) {
				arg.i64 = ip->arg.u32;
				if (arg.i64 == arg.i24 && rollbackPc(ctx)) {
					opc = opc_st32;
				}
			}
			break;

		case opc_ldi8:
			if (!ctx->foldMemory) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_ldsp) {
				if (!fitStackReg(ip->rel)) {
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = ip->rel / vm_stk_align;
				opc = opc_dup2;
			}
			else if (ip->opc == opc_lref) {
				arg.i64 = ip->arg.u32;
				if (arg.i64 == arg.i24 && rollbackPc(ctx)) {
					opc = opc_ld64;
				}
			}
			break;

		case opc_sti8:
			if (!ctx->foldMemory) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_ldsp) {
				if (!fitStackReg(ip->rel)) {
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = ip->rel / vm_stk_align;
				opc = opc_set2;
			}
			else if (ip->opc == opc_lref) {
				arg.i64 = ip->arg.u32;
				if (arg.i64 == arg.i24 && rollbackPc(ctx)) {
					opc = opc_st64;
				}
			}
			break;

		case opc_ldiq:
			if (!ctx->foldMemory) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_ldsp) {
				if (!fitStackReg(ip->rel)) {
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = ip->rel / vm_stk_align;
				opc = opc_dup4;
			}
			else if (ip->opc == opc_lref) {
				arg.i64 = ip->arg.u32;
				if (arg.i64 == arg.i24 && rollbackPc(ctx)) {
					opc = opc_ld128;
				}
			}
			break;

		case opc_stiq:
			if (!ctx->foldMemory) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_ldsp) {
				if (!fitStackReg(ip->rel)) {
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = ip->rel / vm_stk_align;
				opc = opc_set4;
			}
			else if (ip->opc == opc_lref) {
				arg.i64 = ip->arg.u32;
				if (arg.i64 == arg.i24 && rollbackPc(ctx)) {
					opc = opc_st128;
				}
			}
			break;

		// shl, shr, sar, and with immediate value.
		case u32_shl:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_lc32 && rollbackPc(ctx)) {
				arg.i32 = u32_bit_shl | (ip->arg.i32 & 0x3f);
				opc = u32_bit;
			}
			break;

		case u32_shr:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_lc32 && rollbackPc(ctx)) {
				arg.i32 = u32_bit_shr | (ip->arg.i32 & 0x3f);
				opc = u32_bit;
			}
			break;

		case u32_sar:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_lc32 && rollbackPc(ctx)) {
				arg.i32 = u32_bit_sar | (ip->arg.i32 & 0x3f);
				opc = u32_bit;
			}
			break;

		case u32_and:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_lc32 && (ip->arg.i32 & (ip->arg.i32 + 1)) == 0) {
				if (ip->arg.i32 == -1) {
					// todo: a & -1 is always a
					break;
				}
				if (!rollbackPc(ctx)) {
					break;
				}
				arg.i32 = u32_bit_and | (bitsf(ip->arg.u32 + 1) & 0x3f);
				opc = u32_bit;
			}
			break;

		//* TODO: eval Arithmetic using vm.
		case opc_not:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_not:
					ctx->_beg = (void *)ip;
					ip->opc = opc_nop;
					return ctx->vm.pc;

				case opc_lc32:
					ip->arg.i32 = ip->arg.i32 == 0;
					return ctx->vm.pc;

				case opc_lzx1:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = 1;
					opc = opc_lc32;
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = 1;
					opc = opc_lc32;
					break;

				case opc_lc64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.i64 == 0;
					opc = opc_lc32;
					break;

				case opc_lf32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.f32 == 0;
					opc = opc_lc32;
					break;

				case opc_lf64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.f64 == 0;
					opc = opc_lc32;
					break;
			}
			break;

		case i32_neg:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == i32_neg) {
				ctx->_beg = (void *)ip;
				ip->opc = opc_nop;
				return ctx->vm.pc;
			}
			if (ip->opc == opc_lc32 && ctx->foldConst) {
				ip->arg.i32 = -ip->arg.i32;
				return ctx->vm.pc;
			}
			break;

		case i64_neg:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == i64_neg) {
				ctx->_beg = (void *)ip;
				ip->opc = opc_nop;
				return ctx->vm.pc;
			}
			if (ip->opc == opc_lc64 && ctx->foldConst) {
				ip->arg.i64 = -ip->arg.i64;
				return ctx->vm.pc;
			}
			break;

		case f32_neg:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == f32_neg) {
				ctx->_beg = (void *)ip;
				ip->opc = opc_nop;
				return ctx->vm.pc;
			}
			if (ip->opc == opc_lf32 && ctx->foldConst) {
				ip->arg.f32 = -ip->arg.f32;
				return ctx->vm.pc;
			}
			break;

		case f64_neg:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == f64_neg) {
				ctx->_beg = (void *)ip;
				ip->opc = opc_nop;
				return ctx->vm.pc;
			}
			if (ip->opc == opc_lf64 && ctx->foldConst) {
				ip->arg.f64 = -ip->arg.f64;
				return ctx->vm.pc;
			}
			break;

		case i32_add:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_lc32) {
				arg.i64 = ip->arg.i32;
				if (arg.i24 == arg.i64 && rollbackPc(ctx)) {
					opc = opc_inc;
				}
			}
			break;

		case i32_sub:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_lc32) {
				arg.i64 = -ip->arg.i32;
				if (arg.i24 == arg.i64 && rollbackPc(ctx)) {
					opc = opc_inc;
				}
			}
			break;

		case opc_inc:
			if (arg.i64 == 0 && ctx->foldConst) {
				return ctx->vm.pc;
			}
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_inc) {
				vmValue tmp = {0};
				tmp.i64 = arg.i64 + ip->rel;
				if (tmp.i24 == tmp.i64) {
					ip->rel = tmp.i24;
					return ctx->vm.pc;
				}
			}
			else if (ip->opc == opc_ldsp) {
				vmValue tmp = {0};
				if (arg.i64 < 0) {
					// local variable bound error.
					dbgTrace(ERR_INTERNAL_ERROR);
					return 0;
				}
				tmp.i64 = arg.i64 + ip->rel;
				if (tmp.i24 == tmp.i64) {
					ip->rel = tmp.i24;
					return ctx->vm.pc;
				}
			}
			else if (ip->opc == opc_lref) {
				vmValue tmp = {0};
				if (arg.i64 < 0) {
					// global variable bound error.
					dbgTrace(ERR_INTERNAL_ERROR);
					return 0;
				}
				tmp.i64 = arg.i64 + ip->rel;
				if (tmp.i24 == tmp.i64) {
					ip->rel = tmp.i24;
					return ctx->vm.pc;
				}
			}
			break;

		case opc_mad:
			if (!ctx->foldInstr) {
				break;
			}
			if (arg.i64 == 0 && rollbackPc(ctx)) {
				opc = opc_inc;
			}
			else if (arg.i64 == 1) {
				ip = lastIp(ctx);
				if (ip->opc == opc_lc32) {
					arg.i64 = ip->arg.i32;
					if (arg.i24 == arg.i64 && rollbackPc(ctx)) {
						opc = opc_inc;
					}
				} else {
					opc = i32_add;
				}
			}
			break;

		//* TODO: eval Conversions using vm.
		case u32_i64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx2;
					break;

				case opc_lc32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.u32;
					opc = opc_lc64;
					break;
			}
			break;

		case i32_bol:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
				case opc_lzx2:
				case opc_lzx4:
					// zero is false
					return ctx->vm.pc;

				case opc_lc32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.i32 != 0;
					opc = opc_lc32;
					break;
			}
			break;

		case i32_f32:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
				case opc_lzx2:
				case opc_lzx4:
					// zero is zero
					return ctx->vm.pc;

				case opc_lc32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.f32 = (float32_t) ip->arg.i32;
					logif(ip->arg.i32 != arg.f32, "inexact cast: %d => %f", ip->arg.i32, arg.f32);
					opc = opc_lf32;
					break;
			}
			break;

		case i32_i64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx2;
					break;

				case opc_lc32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.i32;
					opc = opc_lc64;
					break;
			}
			break;

		case i32_f64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx2;
					break;

				case opc_lc32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.f64 = ip->arg.i32;
					logif(ip->arg.i32 != arg.f64, "inexact cast: %d => %F", ip->arg.i32, arg.f64);
					opc = opc_lf64;
					break;
			}
			break;

		case i64_i32:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx1;
					break;

				case  opc_lc64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.i64;
					logif(ip->arg.i64 != arg.i32, "inexact cast: %D => %d", ip->arg.i64, arg.i32);
					logif(ip->arg.i64 != arg.u32, "inexact cast: %D => %u", ip->arg.i64, arg.u32);
					opc = opc_lc32;
					break;
			}
			break;

		case i64_f32:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx1;
					break;

				case opc_lc64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.f32 = (float32_t) ip->arg.i64;
					logif(ip->arg.i64 != arg.f32, "inexact cast: %D => %f", ip->arg.i64, arg.f32);
					opc = opc_lf32;
					break;
			}
			break;

		case i64_bol:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx1;
					break;

				case opc_lc64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.i64 != 0;
					opc = opc_lc32;
					break;
			}
			break;

		case i64_f64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
				case opc_lzx4:
					// zero is zero
					return ctx->vm.pc;

				case opc_lc64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.f64 = (float64_t) ip->arg.i64;
					logif(ip->arg.i64 != arg.f64, "inexact cast: %D => %F", ip->arg.i64, arg.f64);
					opc = opc_lf64;
					break;
			}
			break;

		case f32_i32:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
				case opc_lzx2:
				case opc_lzx4:
					// zero is zero
					return ctx->vm.pc;

				case opc_lf32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = (int64_t) ip->arg.f32;
					logif(ip->arg.f32 != arg.i32, "inexact cast: %f => %d", ip->arg.f32, arg.i32);
					opc = opc_lc32;
					break;
			}
			break;

		case f32_bol:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
				case opc_lzx2:
				case opc_lzx4:
					// zero is false
					return ctx->vm.pc;

				case opc_lf32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.f32 != 0;
					opc = opc_lc32;
					break;
			}
			break;

		case f32_i64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx2;
					break;

				case opc_lf32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = (int64_t) ip->arg.f32;
					logif(ip->arg.f32 != arg.i64, "inexact cast: %f => %D", ip->arg.f32, arg.i64);
					opc = opc_lc64;
					break;
			}
			break;

		case f32_f64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx1:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx2;
					break;

				case opc_lf32:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.f64 = ip->arg.f32;
					logif(ip->arg.f32 != arg.f64, "inexact cast: %f => %F", ip->arg.f32, arg.f64);
					opc = opc_lf64;
					break;
			}
			break;

		case f64_i32:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx1;
					break;

				case opc_lf64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = (int64_t) ip->arg.f64;
					logif(ip->arg.f64 != arg.i32, "inexact cast: %F => %d", ip->arg.f64, arg.i32);
					opc = opc_lc32;
					break;
			}
			break;

		case f64_f32:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx1;
					break;

				case opc_lf64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.f32 = (float32_t) ip->arg.f64;
					logif(ip->arg.f64 != arg.f32, "inexact cast: %F => %f", ip->arg.f64, arg.f32);
					opc = opc_lf32;
					break;
			}
			break;

		case f64_i64:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
				case opc_lzx4:
					// zero is zero
					return ctx->vm.pc;

				case opc_lf64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = (int64_t) ip->arg.f64;
					logif(ip->arg.f64 != arg.i64, "inexact cast: %F => %D", ip->arg.f64, arg.i64);
					opc = opc_lc64;
					break;
			}
			break;

		case f64_bol:
			if (!ctx->foldCasts) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lzx2:
					if (!rollbackPc(ctx)) {
						break;
					}
					opc = opc_lzx1;
					break;

				case opc_lf64:
					if (!rollbackPc(ctx)) {
						break;
					}
					arg.i64 = ip->arg.f64 != 0;
					opc = opc_lc32;
					break;
			}
			break;

		// mul, div, mod
		case i32_mul:
		case u32_mul:
		case u32_div:
			if (!ctx->foldInstr2) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lc32:
					if (ip->arg.i32 <= 0) {
						// cannot replace: x % 0
						break;
					}
					if (ip->arg.u32 != (ip->arg.u32 & -ip->arg.u32)) {
						// cannot replace: more than one bit is set
						break;
					}

					if (!rollbackPc(ctx)) {
						break;
					}
					if (opc == u32_div) {
						arg.i32 = u32_bit_shr | bitLen32(ip->arg.u32 - 1);
					} else {
						arg.i32 = u32_bit_shl | bitLen32(ip->arg.u32 - 1);
					}
					opc = u32_bit;
					break;
			}
			break;

		case u32_rem:
			if (!ctx->foldInstr2) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lc32:
					if (ip->arg.i32 <= 0) {
						// cannot replace: x % 0
						break;
					}
					if (ip->arg.u32 != (ip->arg.u32 & -ip->arg.u32)) {
						// cannot replace: more than one bit is set
						break;
					}

					if (!rollbackPc(ctx)) {
						break;
					}

					arg.i32 = u32_bit_and | bitLen32(ip->arg.u32 - 1);
					opc = u32_bit;
					break;
			}
			break;

		case i64_mul:
		case u64_mul:
		case u64_div:
			if (!ctx->foldInstr2) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lc64:
					if (ip->arg.i64 <= 0) {
						// cannot replace: x % 0
						break;
					}
					if (ip->arg.u64 != (ip->arg.u64 & -ip->arg.u64)) {
						// cannot replace: more than one bit is set
						break;
					}
					if (!rollbackPc(ctx)) {
						break;
					}

					if (!emitInt(ctx, opc_lc32, bitLen64(ip->arg.u64 - 1))) {
						// error
						break;
					}

					if (opc == u64_div) {
						opc = u64_shr;
					} else {
						opc = u64_shl;
					}
					break;
			}
			break;

		case u64_rem:
			if (!ctx->foldInstr2) {
				break;
			}
			ip = lastIp(ctx);
			switch (ip->opc) {
				default:
					break;

				case opc_lc64:
					if (ip->arg.i64 <= 0) {
						// cannot replace: x % 0
						break;
					}
					if (ip->arg.u64 != (ip->arg.u64 & -ip->arg.u64)) {
						// cannot replace: more than one bit is set
						break;
					}

					ip->arg.u64 -= 1;
					opc = u64_and;
					break;
			}
			break;

		// conditional jumps
		case opc_jnz:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_not) {
				ctx->_beg = (void *)ip;
				opc = opc_jz;
			}
			// i32 to boolean can be skipped
			else if (ip->opc == i32_bol) {
				ctx->_beg = (void *)ip;
			}
			break;

		case opc_jz:
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_not) {
				ctx->_beg = (void *)ip;
				opc = opc_jnz;
			}
			// i32 to boolean can be skipped
			else if (ip->opc == i32_bol) {
				ctx->_beg = (void *)ip;
			}
			break;

		case opc_spc:
			if (arg.i64 == 0) {
				return ctx->_beg - ctx->_mem;
			}
			if (!ctx->foldInstr) {
				break;
			}
			ip = lastIp(ctx);
			if (ip->opc == opc_mov1 && ip->mov.src == 0 && arg.i64 == -4) {
				ctx->_beg = (void *)ip;
				arg.i32 = ip->mov.dst;
				opc = opc_set1;
			}
			if (ip->opc == opc_mov2 && ip->mov.src == 0 && arg.i64 == -8) {
				ctx->_beg = (void *)ip;
				arg.i32 = ip->mov.dst;
				opc = opc_set2;
			}
			if (ip->opc == opc_mov4 && ip->mov.src == 0 && arg.i64 == -16) {
				ctx->_beg = (void *)ip;
				arg.i32 = ip->mov.dst;
				opc = opc_set4;
			}

			/* TODO: merge stack allocations
			ip = lastIp(rt);
			if (ip->opc == opc_spc) {
				stkval tmp;
				tmp.i8 = arg.i64 + ip->rel;
				if (tmp.rel == tmp.i8) {
					rt->_beg = (memptr)ip;
					rt->vm.ss -= ip->rel / vm_stk_align;
					arg.i64 += ip->rel;
				}
			} */
			break;
	}

	// save previous program counter
	ctx->vm.pc = ctx->_beg - ctx->_mem;

	ip = (vmInstruction) ctx->_beg;
	ip->opc = opc;
	ip->arg = arg;

	switch (opc) {
		default:
			break;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
			if (arg.i64 == 0) {
				// offset is not known when emitting the instruction
				// update it later using fixJump
				break;
			}
			if (ctx->vm.px < arg.u64) {
				ctx->vm.px = arg.u64;
			}
			ip->rel -= (int) ctx->vm.pc;
			dieif((ip->rel + ctx->vm.pc) != arg.u64, ERR_INTERNAL_ERROR);
			break;

		case opc_spc:
			if (arg.i64 > 0x7fffff) {
				dbgTrace("local variable is too large: %D", arg.i64);
				return 0;
			}
			dieif(ip->rel != arg.i64, ERR_INTERNAL_ERROR);
			break;

		case opc_ld32:
		case opc_st32:
			dieif((ip->rel & 3) != 0, ERR_INTERNAL_ERROR);
			dieif(ip->rel != arg.i64, ERR_INTERNAL_ERROR);
			break;

		case opc_ld64:
		case opc_st64:
		case opc_ld128:
		case opc_st128:
			dieif((ip->rel & (vm_mem_align - 1)) != 0, ERR_INTERNAL_ERROR);
			dieif(ip->rel != arg.i64, ERR_INTERNAL_ERROR);
			break;

		case opc_ldsp:
		case opc_move:
		case opc_inc:
		case opc_mad:
			dieif(ip->rel != arg.i64, ERR_INTERNAL_ERROR);
			break;
	}

	dbgEmit("pc[%d]: sp(%d): %A", ctx->vm.pc, ctx->vm.ss, ip);
	switch (opc) {
		error_opc:
			// invalid instruction
			return 0;

		error_stc:
			// stack underflow
			return 0;

		#define STOP(__ERR, __CHK) if (__CHK) goto __ERR
		#define NEXT(__IP, __SP, __CHK)\
			STOP(error_stc, (ssize_t)ctx->vm.ss < (ssize_t)(__CHK));\
			ctx->vm.ss += (__SP);\
			ctx->_beg += (__IP);
		#include "exec.i"
	}

	if (opc == opc_call) {
		// each call ends with a return,
		// which drops ip from the top of the stack.
		ctx->vm.ss -= 1;
	}

	return ctx->vm.pc;
}

void* nextOpc(rtContext ctx, size_t *pc, ssize_t *ss) {
	vmInstruction ip = vmPointer(ctx, *pc);
	if (ip == NULL) {
		*pc = SIZE_MAX;
		return NULL;
	}

	libc *nativeCalls = ctx->vm.nfc;
	switch (ip->opc) {
		#define STOP(__ERR, __CHK) if (__CHK) return NULL;
		#define NEXT(__IP, __SP, __CHK)\
		if (ss != NULL) { *ss += vm_stk_align * (__SP); }\
		*pc += (__IP);
		#include "exec.i"
	}
	return ip;
}
int testOcp(rtContext ctx, size_t offs, vmOpcode opc, vmValue *arg) {
	vmInstruction ip = vmPointer(ctx, offs);
	if (opc >= opc_last) {
		dbgTrace("invalid opc_x%x", opc);
		return 0;
	}
	if (ip != NULL && ip->opc == opc) {
		if (arg != NULL) {
			*arg = ip->arg;
		}
		return 1;
	}
	return 0;
}

void fixJump(rtContext ctx, size_t src, size_t dst, ssize_t stc) {
	dieif(stc > 0 && stc & 3, ERR_INTERNAL_ERROR);

	if (ctx->vm.pc < dst) {
		// protect jumps
		ctx->vm.pc = dst;
	}
	if (ctx->vm.px < dst) {
		// update max jump
		ctx->vm.px = dst;
	}
	if (stc != -1) {
		// update stack used
		ctx->vm.ss = stc / vm_stk_align;
	}

	if (src == 0) {
		// update only stack size and max jump
		return;
	}

	vmInstruction ip = vmPointer(ctx, src);
	switch (ip->opc) {
		default:
			fatal(ERR_INTERNAL_ERROR);
			return;

		case opc_jmp:
		case opc_jnz:
		case opc_jz:
			ip->rel = (int32_t) (dst - src);
			dieif(ip->rel != (int32_t) (dst - src), ERR_INTERNAL_ERROR);
			break;
	}
}

void *rollbackPc(rtContext ctx) {
	ssize_t ss = 0;
	size_t pc = ctx->vm.pc;
	vmInstruction ip = nextOpc(ctx, &pc, &ss);

	if (ip == NULL || (void *) ip == ctx->_beg) {
		return NULL;
	}

	if (ctx->vm.px > ctx->vm.pc) {
		return NULL;
	}

	if (ctx->_beg != vmPointer(ctx, pc)) {
		return NULL;
	}

	ctx->vm.ss -= ss / vm_stk_align;
	ctx->_beg = (void *) ip;
	return ip;
}
