//~ #define NEXT(__IP, __CHK, __SP) {checkstack(__CHK); ip += _IP; sp += _SP;}

#ifdef NEXT

#define SP(__POS, __TYP) (((stkval*)((stkptr)sp + (__POS)))->__TYP)
#define MP(__POS, __TYP) (((stkval*)((memptr)mp + (__POS)))->__TYP)

//{ switch (ip->opc) { ---------------------------------------------------------
//{ 0x0?: SYS		// System
case opc_nop:  NEXT(1, 0, 0) {
} break;
case opc_loc:  NEXT(2, 0, +ip->idx) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
#endif
} break;
case opc_drop: NEXT(2, ip->idx, -ip->idx) {
} break;
case opc_spc:  NEXT(4, 0, 0) {
	int sm = (ip->rel + 3) / 4;
	if (sm > 0) {
		NEXT(0, 0, sm);
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
#endif
	}
	else {
		NEXT(0, -sm, sm);
	}
} break;
case opc_jmp:  NEXT(4, 0, -0) {
#ifdef EXEC
	pu->ip += ip->rel - 4;
#endif
} break;
case opc_jnz:  NEXT(4, 1, -1) {
#ifdef EXEC
	if (SP(0, i4) != 0)
		pu->ip += ip->rel - 4;
#endif
} break;
case opc_jz:   NEXT(4, 1, -1) {
#ifdef EXEC
	if (SP(0, i4) == 0)
		pu->ip += ip->rel - 4;
#endif
} break;
case opc_not:  NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, u4) = !SP(0, u4);
#endif
} break;
case opc_ldsp: NEXT(4, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = (memptr)sp - mp + ip->rel;
#endif
} break;
case opc_call: NEXT(1, 1, -0) {
#ifdef EXEC
	int retip = pu->ip - mp;
	pu->ip = mp + SP(0, u4);
	SP(0, u4) = retip;
#endif
} break;
case opc_jmpi: NEXT(1, 1, -1) {
#ifdef EXEC
	pu->ip = mp + SP(0, u4);
#endif
} break;
case opc_task: NEXT(4, 0, -0) {
#ifdef EXEC
	if (mtt(vm, doTask, pu, 0))
		pu->ip += ip->rel - 4;
	STOP(error_opc, 1);
#endif
} break;
case opc_libc: NEXT(2, libcvec[ip->idx].chk, -libcvec[ip->idx].pop) {
#ifdef EXEC
	vm->s->argv = (char *)sp;
	vm->s->retv = (char*)((stkptr)sp + libcvec[ip->idx].pop);
	vm->s->func = libcvec[ip->idx].sym->offs;
	vm->s->libc = libcvec[ip->idx].sym;
	if (ip->idx == 0) {
		struct symn module = {0};
		module.args = vm->s->defs;
		vm->s->retv = (char *)st;
		vm->s->libc = &module;
	}
	libcvec[ip->idx].call(vm->s);
	STOP(stop_vm, ip->idx == 0);		// Halt();
#endif
} break;
/*case opc_jsub: NEXT(4, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = (memptr)sp - mp + ip->rel;
	pu->ip += ip->rel - 4;
#endif
} break;*/
//}
//{ 0x1?: STK		// Stack
/*case opc_ldc1: NEXT(2, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = ip->arg.i1;
#endif
} break;
case opc_ldc2: NEXT(3, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = ip->arg.i2;
#endif
} break;// */
case opc_ldcf: // temporary opc
case opc_ldcr:
case opc_ldc4: NEXT(5, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = ip->arg.i4;
#endif
} break;
case opc_ldcF: // temporary opc
case opc_ldc8: NEXT(9, 0, +2) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-2, i8) = ip->arg.i8;
#endif
} break;
case opc_dup1: NEXT(2, ip->idx, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = SP(ip->idx, i4);
#endif
} break;
case opc_dup2: NEXT(2, ip->idx, +2) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-2, i8) = SP(ip->idx, i8);
#endif
} break;
case opc_dup4: NEXT(2, ip->idx, +4) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-4, x16) = SP(ip->idx, x16);
	//~ SP(-4, u4) = SP(ip->idx + 0, u4);
	//~ SP(-3, u4) = SP(ip->idx + 1, u4);
	//~ SP(-2, u4) = SP(ip->idx + 2, u4);
	//~ SP(-1, u4) = SP(ip->idx + 3, u4);
#endif
} break;
case opc_set1: NEXT(2, ip->idx, ip->idx <= 1 ? -ip->idx : -1) {
#ifdef EXEC
	SP(ip->idx, u4) = SP(0, u4);
#endif
} break;
case opc_set2: NEXT(2, ip->idx, ip->idx <= 2 ? -ip->idx : -2) {
#ifdef EXEC
	SP(ip->idx, i8) = SP(0, i8);
#endif
} break;
case opc_set4: NEXT(2, ip->idx, ip->idx <= 4 ? -ip->idx : -4) {
#ifdef EXEC
	SP(ip->idx, x16) = SP(0, x16);
	//~ SP(ip->idx - 1, u4) = SP(0, u4);
	//~ SP(ip->idx - 2, u4) = SP(1, u4);
	//~ SP(ip->idx - 3, u4) = SP(2, u4);
	//~ SP(ip->idx - 4, u4) = SP(3, u4);

#endif
} break;
case opc_ldz1: NEXT(1, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = 0;
#endif
} break;
case opc_ldz2: NEXT(1, 0, +2) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = 0;
	SP(-2, i4) = 0;
#endif
} break;
case opc_ldz4: NEXT(1, 0, +4) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	SP(-1, i4) = 0;
	SP(-2, i4) = 0;
	SP(-3, i4) = 0;
	SP(-4, i4) = 0;
#endif
} break;
//}
//{ 0x2?: MEM		// Memory
case opc_ldi1: NEXT(1, 1, -0) {
#ifdef EXEC
	//~ STOP(error_seg, SP(0, i4) < mp);
	//~ STOP(error_seg, SP(0, i4) > mp + ms);
	SP(0, i4) = MP(SP(0, i4), i1);
#endif
} break;
case opc_ldi2: NEXT(1, 1, -0) {
#ifdef EXEC
	//~ STOP(error_seg, SP(0, i4) < mp);
	//~ STOP(error_seg, SP(0, i4) > mp + ms);
	SP(0, i4) = MP(SP(0, i4), i2);
#endif
} break;
case opc_ldi4: NEXT(1, 1, -0) {
#ifdef EXEC
	STOP(error_mem, SP(0, i4) <= 0);
	STOP(error_mem, SP(0, i4) > ms);
	SP(0, i4) = MP(SP(0, i4), i4);
#endif
} break;
case opc_ldi8: NEXT(1, 1, +1) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	STOP(error_mem, SP(0, i4) <= 0);
	STOP(error_mem, SP(0, i4) > ms);
	SP(-1, i8) = MP(SP(0, i4), i8);
#endif
} break;
case opc_ldiq: NEXT(1, 1, +3) {
#ifdef EXEC
	STOP(error_ovf, ovf(pu));
	STOP(error_mem, SP(0, i4) <= 0);
	STOP(error_mem, SP(0, i4) > ms);
	SP(-1, x16) = MP(SP(0, i4), x16);
#endif
} break;

case opc_sti1: NEXT(1, 2, -2) {
#ifdef EXEC
	//~ STOP(error_mem, mp + SP(0, i4) > (memptr)st);
	STOP(error_mem, SP(0, i4) <= vm->cs);
	STOP(error_mem, SP(0, i4) > ms);
	MP(SP(0, i4), i1) = SP(1, i4);
#endif
} break;
case opc_sti2: NEXT(1, 2, -2) {
#ifdef EXEC
	STOP(error_mem, SP(0, i4) <= vm->cs);
	STOP(error_mem, SP(0, i4) > ms);
	MP(SP(0, i4), i2) = SP(1, i4);
#endif
} break;
case opc_sti4: NEXT(1, 2, -2) {
#ifdef EXEC
	STOP(error_mem, SP(0, i4) <= vm->cs);
	STOP(error_mem, SP(0, i4) > ms);
	MP(SP(0, i4), i4) = SP(1, i4);
#endif
} break;
case opc_sti8: NEXT(1, 3, -3) {
#ifdef EXEC
	STOP(error_mem, SP(0, i4) <= vm->cs);
	STOP(error_mem, SP(0, i4) > ms);
	MP(SP(0, i4), i8) = SP(1, i8);
#endif
} break;
case opc_stiq: NEXT(1, 5, -5) {
#ifdef EXEC
	STOP(error_mem, SP(0, i4) <= vm->cs);
	STOP(error_mem, SP(0, i4) > ms);
	MP(SP(0, i4), x16) = SP(1, x16);
#endif
} break;
//}*/
//{ 0x3?: B32		// Unsigned
case b32_cmt: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, u4) = ~SP(0, u4);
#endif
} break;
case b32_and: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) &= SP(0, u4);
#endif
} break;
case b32_ior: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) |= SP(0, u4);
#endif
} break;
case b32_xor: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) ^= SP(0, u4);
#endif
} break;
case b32_shl: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) <<= SP(0, u4);
#endif
} break;
case b32_shr: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) >>= SP(0, u4);
#endif
} break;
case b32_sar: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) >>= SP(0, i4);
#endif
} break;

case u32_i64: NEXT(1, 1, +1) {
#if defined(EXEC) || defined(EVAL)
	SP(-1, i8) = SP(0, u4);
#endif
} break;
case u32_clt: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) = SP(1, u4)  < SP(0, u4);
#endif
} break;
case u32_cgt: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) = SP(1, u4)  > SP(0, u4);
#endif
} break;

/*case b32_zxt: NEXT(2, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, u4) = zxt(SP(0, u4), ip->arg.u1 >> 5, ip->arg.u1 & 31);
#endif
} break;*/
/*case b32_sxt: NEXT(2, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i4) = sxt(SP(0, i4), ip->arg.u1 >> 5, ip->arg.u1 & 31);
#endif
} break;*/
case u32_mul: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, u4) *= SP(0, u4);
#endif
} break;
case u32_div: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, u4) == 0);
	SP(1, u4) /= SP(0, u4);
#endif
} break;
/*case u32_mod: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, u4) == 0);
	SP(1, u4) %= SP(0, u4);
#endif
} break;// */
case u32_mad: NEXT(1, 3, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, u4) += SP(1, u4) * SP(0, u4);
#endif
} break;
//}
//{ 0x4?: I32		// Integer
case i32_neg: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i4) = -SP(0, i4);
#ifdef SETF
	pu->zf = SP(0, i4) == 0;
	pu->sf = SP(0, i4)  < 0;
	pu->cf = 0;
	pu->of = 0;
#endif
#endif
} break;
case i32_add: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) += SP(0, i4);
#ifdef SETF
	pu->zf = SP(1, i4) == 0;
	pu->sf = SP(1, i4)  < 0;
	pu->cf = SP(1, u4) < SP(0, u4);
	pu->of = 0;
#endif
#endif
} break;
case i32_sub: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) -= SP(0, i4);
#endif
} break;
case i32_mul: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) *= SP(0, i4);
#endif
} break;
case i32_div: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, i4) == 0);
	SP(1, i4) /= SP(0, i4);
#endif
} break;
case i32_mod: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, i4) == 0);
	SP(1, i4) %= SP(0, i4);
#endif
} break;

case i32_ceq: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(1, i4) == SP(0, i4);
#endif
} break;
case i32_clt: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(1, i4)  < SP(0, i4);
#endif
} break;
case i32_cgt: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(1, i4)  > SP(0, i4);
#endif
} break;

case i32_bol: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i4) = 0 != SP(0, i4);
#endif
} break;
case i32_f32: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, f4) = SP(0, i4);
#endif
} break;
case i32_i64: NEXT(1, 1, +1) {
#if defined(EXEC) || defined(EVAL)
	SP(-1, i8) = SP(0, i4);
#endif
} break;
case i32_f64: NEXT(1, 1, +1) {
#if defined(EXEC) || defined(EVAL)
	SP(-1, f8) = SP(0, i4);
#endif
} break;
//}
//{ 0x5?: F32		// Float
case f32_neg: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, f4) = -SP(0, f4);
#endif
} break;
case f32_add: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, f4) += SP(0, f4);
#endif
} break;
case f32_sub: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, f4) -= SP(0, f4);
#endif
} break;
case f32_mul: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, f4) *= SP(0, f4);
#endif
} break;
case f32_div: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, f4) /= SP(0, f4);
	STOP(error_div, SP(0, f4) == 0);
#endif
} break;
case f32_mod: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, f4) == 0);
	SP(1, f4) = fmod(SP(1, f4), SP(0, f4));
#endif
} break;

case f32_ceq: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(1, f4) == SP(0, f4);
#endif
} break;
case f32_clt: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(1, f4)  < SP(0, f4);
#endif
} break;
case f32_cgt: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(1, f4)  > SP(0, f4);
#endif
} break;

case f32_i32: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i4) = SP(0, f4);
#endif
} break;
case f32_bol: NEXT(1, 1, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i4) = 0 != SP(0, f4);
#endif
} break;
case f32_i64: NEXT(1, 1, +1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_ovf, ovf(pu));
	SP(-1, i8) = SP(0, f4);
#endif
} break;
case f32_f64: NEXT(1, 1, +1) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_ovf, ovf(pu));
	SP(-1, f8) = SP(0, f4);
#endif
} break;
//} */
//{ 0x6?: I64		// Long
case i64_neg: NEXT(1, 2, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i8) = -SP(0, i8);
#endif
} break;
case i64_add: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, i8) += SP(0, i8);
#endif
} break;
case i64_sub: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, i8) -= SP(0, i8);
#endif
} break;
case i64_mul: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, i8) *= SP(0, i8);
#endif
} break;
case i64_div: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, i8) == 0);
	SP(2, i8) /= SP(0, i8);
#endif
} break;
case i64_mod: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, i8) == 0);
	SP(2, i8) %= SP(0, i8);
#endif
} break;

case i64_ceq: NEXT(1, 4, -3) {
#if defined(EXEC) || defined(EVAL)
	SP(3, i4) = SP(2, i8) == SP(0, i8);
#endif
} break;
case i64_clt: NEXT(1, 4, -3) {
#if defined(EXEC) || defined(EVAL)
	SP(3, i4) = SP(2, i8)  < SP(0, i8);
#endif
} break;
case i64_cgt: NEXT(1, 4, -3) {
#if defined(EXEC) || defined(EVAL)
	SP(3, i4) = SP(2, i8)  > SP(0, i8);
#endif
} break;

case i64_i32: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(0, i8);
#endif
} break;
case i64_f32: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, f4) = SP(0, i8);
#endif
} break;
case i64_bol: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = 0 != SP(0, i8);
#endif
} break;
case i64_f64: NEXT(1, 2, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, f8) = SP(0, i8);
#endif
} break;
//}
//{ 0x7?: F64		// Double
case f64_neg: NEXT(1, 2, -0) {
#if defined(EXEC) || defined(EVAL)
	//~ EXEC(f64neg, sp, 0,sp)
	SP(0, f8) = -SP(0, f8);
#endif
} break;
case f64_add: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	//~ EXEC(f64neg, sp + 2, sp + 2, sp)
	SP(2, f8) += SP(0, f8);
#endif
} break;
case f64_sub: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, f8) -= SP(0, f8);
#endif
} break;
case f64_mul: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, f8) *= SP(0, f8);
#endif
} break;
case f64_div: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	SP(2, f8) /= SP(0, f8);
	STOP(error_div, SP(0, f8) == 0);
#endif
} break;
case f64_mod: NEXT(1, 4, -2) {
#if defined(EXEC) || defined(EVAL)
	STOP(error_div, SP(0, f8) == 0);
	SP(2, f8) = fmod(SP(2, f8), SP(0, f8));
#endif
} break;

case f64_ceq: NEXT(1, 4, -3) {
#if defined(EXEC) || defined(EVAL)
	SP(3, i4) = SP(2, f8) == SP(0, f8);
#endif
} break;
case f64_clt: NEXT(1, 4, -3) {
#if defined(EXEC) || defined(EVAL)
	SP(3, i4) = SP(2, f8)  < SP(0, f8);
#endif
} break;
case f64_cgt: NEXT(1, 4, -3) {
#if defined(EXEC) || defined(EVAL)
	SP(3, i4) = SP(2, f8)  > SP(0, f8);
#endif
} break;

case f64_i32: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = SP(0, f8);
#endif
} break;
case f64_f32: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, f4) = SP(0, f8);
#endif
} break;
case f64_i64: NEXT(1, 2, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, i8) = SP(0, f8);
#endif
} break;
case f64_bol: NEXT(1, 2, -1) {
#if defined(EXEC) || defined(EVAL)
	SP(1, i4) = 0 != SP(0, f8);
#endif
} break;
//}
//{ 0x8?: PF4		// Vector
case v4f_neg: NEXT(1, 4, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, f4) = -SP(0, f4);
	SP(1, f4) = -SP(1, f4);
	SP(2, f4) = -SP(2, f4);
	SP(3, f4) = -SP(3, f4);
#endif
} break;
case v4f_add: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	SP(4, f4) += SP(0, f4);
	SP(5, f4) += SP(1, f4);
	SP(6, f4) += SP(2, f4);
	SP(7, f4) += SP(3, f4);
#endif
} break;
case v4f_sub: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	SP(4, f4) -= SP(0, f4);
	SP(5, f4) -= SP(1, f4);
	SP(6, f4) -= SP(2, f4);
	SP(7, f4) -= SP(3, f4);
#endif
} break;
case v4f_mul: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	SP(4, f4) *= SP(0, f4);
	SP(5, f4) *= SP(1, f4);
	SP(6, f4) *= SP(2, f4);
	SP(7, f4) *= SP(3, f4);
#endif
} break;
case v4f_div: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	//~ STOP(error_div, !SP(0, f4) || !SP(1, f4) || !SP(2, f4) || !SP(3, f4));
	SP(4, f4) /= SP(0, f4);
	SP(5, f4) /= SP(1, f4);
	SP(6, f4) /= SP(2, f4);
	SP(7, f4) /= SP(3, f4);
#endif
} break;
case v4f_ceq: NEXT(1, 8, -7) {
#if defined(EXEC)
	SP(7, i4) = (SP(4, f4) == SP(0, f4))
			 && (SP(5, f4) == SP(1, f4))
			 && (SP(6, f4) == SP(2, f4))
			 && (SP(7, f4) == SP(3, f4));
#endif
}
case v4f_min: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	if (SP(4, f4) > SP(0, f4))
		SP(4, f4) = SP(0, f4);
	if (SP(5, f4) > SP(1, f4))
		SP(5, f4) = SP(1, f4);
	if (SP(6, f4) > SP(2, f4))
		SP(6, f4) = SP(2, f4);
	if (SP(7, f4) > SP(3, f4))
		SP(7, f4) = SP(3, f4);
#endif
} break;
case v4f_max: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	if (SP(4, f4) < SP(0, f4))
		SP(4, f4) = SP(0, f4);
	if (SP(5, f4) < SP(1, f4))
		SP(5, f4) = SP(1, f4);
	if (SP(6, f4) < SP(2, f4))
		SP(6, f4) = SP(2, f4);
	if (SP(7, f4) < SP(3, f4))
		SP(7, f4) = SP(3, f4);
#endif
} break;
case v4f_dp3: NEXT(1, 8, -7) {
#if defined(EXEC) || defined(EVAL)
	//~ SP(7, f4) = 
	//~ SP(4, pf).x * SP(0, pf).x +
	//~ SP(4, pf).y * SP(0, pf).y +
	//~ SP(4, pf).z * SP(0, pf).z ;
	SP(7, f4) = 
	SP(4, f4) * SP(0, f4) +
	SP(5, f4) * SP(1, f4) +
	SP(6, f4) * SP(2, f4);
#endif
} break;
case v4f_dph: NEXT(1, 8, -7) {
#if defined(EXEC) || defined(EVAL)
	SP(7, f4) += 
	SP(4, f4) * SP(0, f4) +
	SP(5, f4) * SP(1, f4) +
	SP(6, f4) * SP(2, f4);
#endif
} break;
case v4f_dp4: NEXT(1, 8, -7) {
#if defined(EXEC) || defined(EVAL)
	SP(7, f4) = 
	SP(4, f4) * SP(0, f4) +
	SP(5, f4) * SP(1, f4) +
	SP(6, f4) * SP(2, f4) +
	SP(7, f4) * SP(3, f4);
#endif
} break;
//}*/
//{ 0x9?: PD2		// Complex
case v2d_neg: NEXT(1, 4, -0) {
#if defined(EXEC) || defined(EVAL)
	SP(0, f8) = -SP(0, f8);
	SP(2, f8) = -SP(2, f8);
#endif
} break;
case v2d_add: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	SP(4, f8) += SP(0, f8);
	SP(6, f8) += SP(2, f8);
#endif
} break;
case v2d_sub: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	SP(4, f8) -= SP(0, f8);
	SP(6, f8) -= SP(2, f8);
#endif
} break;
case v2d_mul: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	SP(4, f8) *= SP(0, f8);
	SP(6, f8) *= SP(2, f8);
#endif
} break;
case v2d_div: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	//~ STOP(error_div, SP(0, f8) || SP(2, f8));
	SP(4, f8) /= SP(0, f8);
	SP(6, f8) /= SP(2, f8);
#endif
} break;
case p4d_swz: NEXT(2, 4, -0) {
#if defined(EXEC) || defined(EVAL)
	uint32_t swz = ip->idx;
	uint32_t d0 = SP((swz >> 0) & 3, i4);
	uint32_t d1 = SP((swz >> 2) & 3, i4);
	uint32_t d2 = SP((swz >> 4) & 3, i4);
	uint32_t d3 = SP((swz >> 6) & 3, i4);
	SP(0, i4) = d0;
	SP(1, i4) = d1;
	SP(2, i4) = d2;
	SP(3, i4) = d3;
#endif
} break;
case v2d_ceq: NEXT(1, 8, -7) {
#if defined(EXEC)
	SP(7, i4) = (SP(4, f8) == SP(0, f8)) && (SP(6, f8) == SP(2, f8));
#endif
} break;
case v2d_min: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	if (SP(4, f8) > SP(0, f8))
		SP(4, f8) = SP(0, f8);
	if (SP(6, f8) > SP(2, f8))
		SP(6, f8) = SP(2, f8);
#endif
} break;
case v2d_max: NEXT(1, 8, -4) {
#if defined(EXEC) || defined(EVAL)
	if (SP(4, f8) < SP(0, f8))
		SP(4, f8) = SP(0, f8);
	if (SP(6, f8) < SP(2, f8))
		SP(6, f8) = SP(2, f8);
#endif
} break;
//}*/
//~ 0xa?: ???		//
//~ 0xb?: ???		//
//~ 0xc?: ???		//
//~ 0xd?: ???		//
//~ 0xe?: ???		//
//~ 0xf?: ???		//
default: STOP(error_opc, 1);
//}-----------------------------------------------------------------------------

#undef SP
#undef MP

#undef EVAL
#undef EXEC
#undef STOP
#undef NEXT
#endif
