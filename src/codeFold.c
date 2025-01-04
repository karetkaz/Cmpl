// Code Optimization functions
#include "printer.h"
#include "code.h"

static int removeInstruction(rtContext ctx, size_t offs) {
	if (offs < ctx->vm.ro || offs > ctx->vm.pc) {
		return 0;
	}
	vmInstruction ip = vmPointer(ctx, offs);
	unsigned char *dst = (unsigned char *)ip;
	size_t size = opcode_tbl[ip->opc].size;
	memcpy(dst, dst + size, ctx->_beg - dst);
	ctx->_beg -= size;
	ctx->vm.pc = ctx->_beg - ctx->_mem;
	return 1;
}
static int decrementStackAccess(rtContext ctx, size_t offsBegin, size_t offsEnd, int count) {
	ssize_t ss = 0;
	for (size_t pc = offsBegin; pc < offsEnd; ) {
		size_t stack = ss;
		vmInstruction ip = nextOpc(ctx, &pc, &ss);
		if (ip == NULL) {
			fatal(ERR_INTERNAL_ERROR);
			return 0;
		}
		switch (ip->opc) {
			default:
				break;

			case opc_set1:
			case opc_set2:
			case opc_set4:
			case opc_dup1:
			case opc_dup2:
			case opc_dup4:
				if (ip->idx * vm_stk_align > stack) {
					ip->idx -= count;
				}
				break;

			case opc_mov1:
			case opc_mov2:
			case opc_mov4:
				if (ip->mov.dst * vm_stk_align > stack) {
					ip->mov.dst -= count;
				}
				if (ip->mov.src * vm_stk_align > stack) {
					ip->mov.src -= count;
				}
				break;

			case opc_ldsp:
				if ((size_t) ip->rel > stack) {
					ip->rel -= count * vm_stk_align;
				}
				break;
		}
	}
	return 1;
}
// to optimize assignment `a = a + 1`, `a` can be read only once in the expression
static int checkStackReadAccess(rtContext ctx, size_t offsBegin, size_t offsEnd, size_t size) {
	ssize_t ss = 0;
	int result = 0;
	for (size_t pc = offsBegin; pc < offsEnd; ) {
		vmInstruction ip = nextOpc(ctx, &pc, &ss);
		if (ip == NULL) {
			fatal(ERR_INTERNAL_ERROR);
			return 0;
		}
		switch (ip->opc) {
			default:
				break;

			case opc_dup1:
			case opc_dup2:
			case opc_dup4:
				result += ip->idx * vm_stk_align < size;
				break;

			case opc_set1:
			case opc_set2:
			case opc_set4:
				break;

			case opc_mov1:
			case opc_mov2:
			case opc_mov4:
				result += ip->mov.src * vm_stk_align < size;
				break;

			case opc_ldsp:
				result += (size_t) ip->rel < size;
				break;
		}
	}
	return result;
}

int foldAssignment(rtContext ctx, size_t start, size_t end) {
	vmValue arg = {0};
	vmValue argSet = {0};
	if (testOcp(ctx, start, opc_dup1, &arg)) {
		if (!testOcp(ctx, end, opc_set1, &argSet)) {
			return 0;
		}
		if (end - start == 2 && emit(ctx, markIP) - end == 2) {
			if (!removeInstruction(ctx, end)) {
				fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
				return 0;
			}
			if (!removeInstruction(ctx, start)) {
				fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
				return 0;
			}
			arg.p128.u08[1] = arg.u08;
			arg.p128.u08[0] = argSet.u08 - 1;
			return emitOpc(ctx, opc_mov1, arg) != 0;
		}

		if (arg.u08 != 0 || argSet.u08 != 1) {
			return 0;
		}
		if (checkStackReadAccess(ctx, start, end, 1) != 1) {
			return 0;
		}
		if (!removeInstruction(ctx, end)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
			return 0;
		}
		if (!removeInstruction(ctx, start)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
			return 0;
		}
		if (!decrementStackAccess(ctx, start, end, 1)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", start, vmPointer(ctx, start));
			return 0;
		}
		return 1;
	}
	if (testOcp(ctx, start, opc_dup2, &arg)) {
		if (!testOcp(ctx, end, opc_set2, &argSet)) {
			return 0;
		}
		if (end - start == 2 && emit(ctx, markIP) - end == 2) {
			if (!removeInstruction(ctx, end)) {
				fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
				return 0;
			}
			if (!removeInstruction(ctx, start)) {
				fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
				return 0;
			}
			arg.p128.u08[1] = arg.u08;
			arg.p128.u08[0] = argSet.u08 - 2;
			return emitOpc(ctx, opc_mov2, arg) != 0;
		}

		if (arg.u08 != 0 || argSet.u08 != 2) {
			return 0;
		}
		if (checkStackReadAccess(ctx, start, end, 2) != 1) {
			return 0;
		}
		if (!removeInstruction(ctx, end)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
			return 0;
		}
		if (!removeInstruction(ctx, start)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
			return 0;
		}
		if (!decrementStackAccess(ctx, start, end, 2)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", start, vmPointer(ctx, start));
			return 0;
		}
		return 1;
	}
	if (testOcp(ctx, start, opc_dup4, &arg)) {
		if (!testOcp(ctx, end, opc_set4, &argSet)) {
			return 0;
		}
		if (end - start == 2 && emit(ctx, markIP) - end == 2) {
			if (!removeInstruction(ctx, end)) {
				fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
				return 0;
			}
			if (!removeInstruction(ctx, start)) {
				fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
				return 0;
			}
			arg.p128.u08[1] = arg.u08;
			arg.p128.u08[0] = argSet.u08 - 4;
			return emitOpc(ctx, opc_mov4, arg) != 0;
		}

		if (arg.u08 != 0 || argSet.u08 != 4) {
			return 0;
		}
		if (checkStackReadAccess(ctx, start, end, 4) != 1) {
			return 0;
		}
		if (!removeInstruction(ctx, end)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
			return 0;
		}
		if (!removeInstruction(ctx, start)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", end, vmPointer(ctx, end));
			return 0;
		}
		if (!decrementStackAccess(ctx, start, end, 4)) {
			fatal(ERR_INTERNAL_ERROR": %.*A", start, vmPointer(ctx, start));
			return 0;
		}
		return 1;
	}
	return 0;
}
