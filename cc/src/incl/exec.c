#define SP(__POS, __TYP) (((stkval*)(((char*)st)+(4*(__POS))))->__TYP)
//~ #define SPRES(__TYP) (((stkval*)((char*)pu->sp))->__TYP)
// { switch (ip->opc)------------------------------------------------------------
//{ 0x0?: SYS		// System
case opc_nop  : NEXT(1, 0, 0) {
} break;
case opc_loc  : NEXT(2, 0, +ip->idx) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
#endif
} break;
case opc_pop  : NEXT(2, ip->idx, -ip->idx) {
} break;
case opc_jmp  : NEXT(4, 0, -0) {
#ifdef EXEC
	pu->ip += ip->jmp - 4;
#endif
} break;
case opc_jnz  : NEXT(4, 1, -1) {
#ifdef EXEC
	if (SP(0, i4) != 0)
		pu->ip += ip->jmp - 4;
#endif
} break;
case opc_jz   : NEXT(4, 1, -1) {
#ifdef EXEC
	if (SP(0, i4) == 0)
		pu->ip += ip->jmp - 4;
#endif
} break;
case opc_jmpi : NEXT(1, 1, -1) {
#ifdef EXEC
	pu->ip = (void*)SP(0, u4);
#endif
} break;
case opc_task : NEXT(4, 0, -0) {
#ifdef EXEC
	//~ if (task(pu, ip->cl, ip->dl, ss))
		//~ pu->ip += ip->jmp - 4;
	STOP(error_opc, 1);
#endif
} break;
case opc_sysc : NEXT(2, 0, -0) {
#ifdef EXEC
	switch (ip->arg.u1) {
		//~ case: exit, halt, wait, join, sync, trap), (alloc, free), (copy, init)
		default: STOP(error_opc, 1);
		case 0x00: {		// exit
			pu->ip = 0;
			//~ return 0;
		} break;
		case 0x01: {		// halt
			pu->ip = 0;
			//~ return 0;
		} break;
	}
#endif
} break;
case opc_libc : NEXT(2, libcfnc[ip->idx].chk, -libcfnc[ip->idx].pop) {
#ifdef EXEC
	struct libcarg args;
	args.data = ud;
	args.argv = (char *)st;
	args.retv = st + libcfnc[ip->idx].pop * 4;
	libcfnc[ip->idx].call(&args);
#endif
} break;
case opc_call : NEXT(4, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	pu->ip += ip->jmp - 4;
#endif
} break;
//}
//{ 0x1?: STK		// Stack
case opc_ldc1: NEXT(2, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = ip->arg.i1;
#endif
} break;
case opc_ldc2: NEXT(3, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = ip->arg.i2;
#endif
} break;
case opc_ldcf: // temporary opc : loc
case opc_ldcr: // temporary opc : pop
case opc_ldc4: NEXT(5, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = ip->arg.i4;
#endif
} break;
case opc_ldcF: // temporary opc
case opc_ldc8: NEXT(9, 0, +2) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-2, i8) = ip->arg.i8;
#endif
} break;
case opc_dup1: NEXT(2, ip->idx, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = SP(ip->idx, i4);
#endif
} break;
case opc_dup2: NEXT(2, ip->idx, +2) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-2, u8) = SP(ip->idx, u8);
#endif
} break;
case opc_dup4: NEXT(2, ip->idx, +4) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	//~ SP(-1, u4) = SP(ip->idx + 3, u4);
	//~ SP(-2, u4) = SP(ip->idx + 2, u4);
	//~ SP(-3, u4) = SP(ip->idx + 1, u4);
	//~ SP(-4, u4) = SP(ip->idx + 0, u4);
	SP(-4, x4) = SP(ip->idx + 0, x4);
#endif
} break;
case opc_set1: NEXT(2, ip->idx, -1) {
#ifdef EXEC
	SP(ip->idx, u4) = SP(0, u4);
#endif
} break;
case opc_set2: NEXT(2, ip->idx, -2) {
#ifdef EXEC
	SP(ip->idx, u8) = SP(0, u8);
#endif
} break;
case opc_set4: NEXT(2, ip->idx, -4) {
#ifdef EXEC
	SP(ip->idx, x4) = SP(0, x4);
#endif
} break;
case opc_ldz1: NEXT(1, 0, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = 0;
#endif
} break;
case opc_ldz2: NEXT(1, 0, +2) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = 0;
	SP(-2, i4) = 0;
#endif
} break;
case opc_ldz4: NEXT(1, 0, +4) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i4) = 0;
	SP(-2, i4) = 0;
	SP(-3, i4) = 0;
	SP(-4, i4) = 0;
#endif
} break;
//}
/*{ 0x2?: MEM		// Memory
case opc_ldi1: NEXT(1, 0, +1);{
#ifdef EXEC
	MEMP((void*)(long)SP(0, i4));
	sp->d1 = mp->d1;
#endif
} break;
case opc_ldi2: NEXT(1, 0, +1);{
#ifdef EXEC
	MEMP((void*)(long)SP(0, i4));
	sp->d2 = mp->d2;
#endif
} break;
case opc_ldi4: NEXT(1, 0, +1);{
#ifdef EXEC
	MEMP((void*)(long)SP(0, i4));
	sp->i4 = mp->i4;
#endif
} break;
case opc_sti1: NEXT(1, 2, -2);{
#ifdef EXEC
	MEMP((void*)(long)SP(1, i4));
	mp->d1 = sp->i4;
#endif
} break;
case opc_sti2: NEXT(1, 2, -2);{
#ifdef EXEC
	//~ MEMP(sp[1].vp);
	MEMP((void*)(long)sp[1].i4);
	mp->d2 = sp->i4;
#endif
} break;
case opc_sti4: NEXT(1, 2, -2);{
#ifdef EXEC
	//~ MEMP(sp[1].vp);
	MEMP((void*)(long)sp[1].i4);
	mp->i4 = sp->i4;
#endif
} break;
//}*/
//{ 0x3?: B32		// Unsigned
case b32_cmt: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, u4) = ~SP(0, u4);
#endif
} break;
case b32_and: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) &= SP(0, u4);
#endif
} break;
case b32_ior: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) |= SP(0, u4);
#endif
} break;
case b32_xor: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) ^= SP(0, u4);
#endif
} break;
case b32_shl: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) <<= SP(0, u4);
#endif
} break;
case b32_shr: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) >>= SP(0, u4);
#endif
} break;
case b32_sar: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) >>= SP(0, i4);
#endif
} break;

case b32_not: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, u4) = !SP(0, u4);
#endif
} break;
case u32_clt: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) = SP(1, u4)  < SP(0, u4);
#endif
} break;
case u32_cgt: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) = SP(1, u4)  > SP(0, u4);
#endif
} break;

/*case b32_bit: NEXT(2, 1, -0) {
#ifdef EXEC
	switch (ip->arg.u1 >> 5) {
		default : STOP(error_opc, true);
		case 1 : SP(0, u4) <<= ip->arg.u1 & 0x1F; break;				// shl
		case 2 : SP(0, u4) >>= ip->arg.u1 & 0x1F; break;				// shr
		case 3 : SP(0, i4) >>= ip->arg.u1 & 0x1F; break;				// sar
		case 4 : SP(0, u4) = rot(SP(0, u4), ip->arg.u1 & 0x1F); break;			// rot
		//~ case 5 : SP(0, u4) = (SP(0, u4) >> (ip->arg.u1 & 0x1F)) & 1; break;		// get
		//~ case 6 : SP(0, u4) |= 1 << (ip->arg.u1 & 0x1F); break;			// set
		//~ case 7 : SP(0, u4) ^= 1 << (ip->arg.u1 & 0x1F); break;			// cmt
		case 0 : switch (ip->arg.u1 & 0x1F) {	// 32  posibilities
			default : STOP(error_opc, true);
			case 1 : SP(0, u4) = !SP(0, u4); break;			// any		// any bit set ? 0 : 1
			case 2 : SP(0, u4) = SP(0, u4) == 0xffffffff; break;	// all		// all bits set ? 0 : 1
			case 3 : SP(0, u4) = btc(SP(0, u4)); break;		// cnt		// count bits
			case 4 : SP(0, u4) = bsf(SP(0, u4)); break;		// bsf		// scan forward
			case 5 : SP(0, u4) = bsr(SP(0, u4)); break;		// bsr		// scan reverse
			case 6 : SP(0, u4) >>= 31; break;			// sgn		// sign bit set ? 0 : 1
			case 7 : SP(0, u4) &= 1; break;				// par		// parity bit set ? 0 : 1
			// swp, neg, cmt,
		} break;
	}
#endif
} break;*/
/*case b32_rot: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) = rot(SP(1, u4), SP(0, u4));
#endif
} break;*/
/*case b32_zxt: NEXT(2, 1, -0) {
#ifdef EXEC
	SP(0, u4) = zxt(SP(0, u4), ip->arg.u1 >> 5, ip->arg.u1 & 31);
#endif
} break;*/
/*case b32_sxt: NEXT(2, 1, -0) {
#ifdef EXEC
	SP(0, i4) = sxt(SP(0, i4), ip->arg.u1 >> 5, ip->arg.u1 & 31);
#endif
} break;*/
/*case u32_mul: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, u4) *= SP(0, u4);
#endif
} break;*/
/*case u32_div: NEXT(1, 2, -1) {
#ifdef EXEC
	STOP(error_div, SP(0, u4) == 0);
	SP(1, u4) /= SP(0, u4);
#endif
} break;*/
/*case u32_mod: NEXT(1, 2, -1) {
#ifdef EXEC
	STOP(error_div, SP(0, u4) == 0);
	SP(1, u4) %= SP(0, u4);
#endif
} break;*/
case u32_mad: NEXT(1, 3, -2) {
#ifdef EXEC
	SP(2, u4) += SP(1, u4) * SP(0, u4);
#endif
} break;
//}
//{ 0x4?: I32		// Integer
case i32_neg: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, i4) = -SP(0, i4);
#ifdef SETF
	pu->zf = SP(0, i4) == 0;
	pu->sf = SP(0, i4)  < 0;
	pu->cf = 0;
	pu->of = 0;
#ifdef CDBG
	STOP(debug_ovf, pu->cf || pu->of);
#endif
#endif
#endif
} break;
case i32_add: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) += SP(0, i4);
#ifdef SETF
	pu->zf = SP(1, i4) == 0;
	pu->sf = SP(1, i4)  < 0;
	pu->cf = SP(1, u4) < SP(0, u4);
	pu->of = 0;
#ifdef CDBG
	STOP(debug_ovf, pu->cf || pu->of);
#endif
#endif
#endif
} break;
case i32_sub: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) -= SP(0, i4);
#endif
} break;
case i32_mul: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) *= SP(0, i4);
#endif
} break;
case i32_div: NEXT(1, 2, -1) {
#ifdef EXEC
	STOP(error_div, SP(0, i4) == 0);
	SP(1, i4) /= SP(0, i4);
#endif
} break;
case i32_mod: NEXT(1, 2, -1) {
#ifdef EXEC
	STOP(error_div, SP(0, i4) == 0);
	SP(1, i4) %= SP(0, i4);
#endif
} break;

case i32_ceq: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(1, i4) == SP(0, i4);
#endif
} break;
case i32_clt: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(1, i4)  < SP(0, i4);
#endif
} break;
case i32_cgt: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(1, i4)  > SP(0, i4);
#endif
} break;

case i32_bol: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, i4) = 0 != SP(0, i4);
#endif
} break;
case i32_f32: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, f4) = SP(0, i4);
#endif
} break;
case i32_i64: NEXT(1, 1, +1) {
#ifdef EXEC
	SP(-1, i8) = SP(0, i4);
#endif
} break;
case i32_f64: NEXT(1, 1, +1) {
#ifdef EXEC
	SP(-1, f8) = SP(0, i4);
#endif
} break;
//}
//{ 0x5?: F32		// Float
case f32_neg: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, f4) = -SP(0, f4);
#endif
} break;
case f32_add: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, f4) += SP(0, f4);
#endif
} break;
case f32_sub: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, f4) -= SP(0, f4);
#endif
} break;
case f32_mul: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, f4) *= SP(0, f4);
#endif
} break;
case f32_div: NEXT(1, 2, -1) {
#ifdef EXEC
	STOP(error_div, SP(0, f4) == 0);
	SP(1, f4) /= SP(0, f4);
#endif
} break;
case f32_mod: NEXT(1, 2, -1) {
#ifdef EXEC
	STOP(error_div, SP(0, f4) == 0);
	SP(1, f4) = fmod(SP(1, f4), SP(0, f4));
#endif
} break;
case f32_ceq: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(1, f4) == SP(0, f4);
#endif
} break;
case f32_clt: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(1, f4)  < SP(0, f4);
#endif
} break;
case f32_cgt: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(1, f4)  > SP(0, f4);
#endif
} break;
case f32_i32: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, i4) = SP(0, f4);
#endif
} break;
case f32_bol: NEXT(1, 1, -0) {
#ifdef EXEC
	SP(0, i4) = 0 != SP(0, f4);
#endif
} break;
case f32_i64: NEXT(1, 1, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, i8) = SP(0, f4);
#endif
} break;
case f32_f64: NEXT(1, 1, +1) {
#ifdef EXEC
	STOP(error_ovf, pu->sp < pu->bp);
	SP(-1, f8) = SP(0, f4);
#endif
} break;
//} */
//{ 0x6?: I64		// Long
case i64_neg: NEXT(1, 2, -0) {
#ifdef EXEC
	SP(0, i8) = -SP(0, i8);
#endif
} break;
case i64_add: NEXT(1, 4, -2) {
#ifdef EXEC
	SP(2, i8) += SP(0, i8);
#endif
} break;
case i64_sub: NEXT(1, 4, -2) {
#ifdef EXEC
	SP(2, i8) -= SP(0, i8);
#endif
} break;
case i64_mul: NEXT(1, 4, -2) {
#ifdef EXEC
	SP(2, i8) *= SP(0, i8);
#endif
} break;
case i64_div: NEXT(1, 4, -2) {
#ifdef EXEC
	STOP(error_div, SP(0, i8) == 0);
	SP(2, i8) /= SP(0, i8);
#endif
} break;
case i64_mod: NEXT(1, 4, -2) {
#ifdef EXEC
	STOP(error_div, SP(0, i8) == 0);
	SP(2, i8) %= SP(0, i8);
#endif
} break;

case i64_ceq: NEXT(1, 4, -3) {
#ifdef EXEC
	SP(3, i4) = SP(2, i8) == SP(0, i8);
#endif
} break;
case i64_clt: NEXT(1, 4, -3) {
#ifdef EXEC
	SP(3, i4) = SP(2, i8)  < SP(0, i8);
#endif
} break;
case i64_cgt: NEXT(1, 4, -3) {
#ifdef EXEC
	SP(3, i4) = SP(2, i8)  > SP(0, i8);
#endif
} break;

case i64_i32: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(0, i8);
#endif
} break;
case i64_f32: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, f4) = SP(0, i8);
#endif
} break;
case i64_bol: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = 0 != SP(0, i8);
#endif
} break;
case i64_f64: NEXT(1, 2, -0) {
#ifdef EXEC
	SP(0, f8) = SP(0, i8);
#endif
} break;
//}
//{ 0x7?: F64		// Double
case f64_neg: NEXT(1, 2, -0) {
#ifdef EXEC
	SP(0, f8) = -SP(0, f8);
#endif
} break;
case f64_add: NEXT(1, 4, -2) {
#ifdef EXEC
	SP(2, f8) += SP(0, f8);
#endif
} break;
case f64_sub: NEXT(1, 4, -2) {
#ifdef EXEC
	SP(2, f8) -= SP(0, f8);
#endif
} break;
case f64_mul: NEXT(1, 4, -2) {
#ifdef EXEC
	SP(2, f8) *= SP(0, f8);
#endif
} break;
case f64_div: NEXT(1, 4, -2) {
#ifdef EXEC
	STOP(error_div, SP(0, f8) == 0);
	SP(2, f8) /= SP(0, f8);
#endif
} break;
case f64_mod: NEXT(1, 4, -2) {
#ifdef EXEC
	STOP(error_div, SP(0, f8) == 0);
	SP(2, f8) = fmod(SP(2, f8), SP(0, f8));
#endif
} break;

case f64_ceq: NEXT(1, 4, -3) {
#ifdef EXEC
	SP(3, i4) = SP(2, f8) == SP(0, f8);
#endif
} break;
case f64_clt: NEXT(1, 4, -3) {
#ifdef EXEC
	SP(3, i4) = SP(2, f8)  < SP(0, f8);
#endif
} break;
case f64_cgt: NEXT(1, 4, -3) {
#ifdef EXEC
	SP(3, i4) = SP(2, f8)  > SP(0, f8);
#endif
} break;

case f64_i32: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = SP(0, f8);
#endif
} break;
case f64_f32: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, f4) = SP(0, f8);
#endif
} break;
case f64_i64: NEXT(1, 2, -0) {
#ifdef EXEC
	SP(1, i8) = SP(0, f8);
#endif
} break;
case f64_bol: NEXT(1, 2, -1) {
#ifdef EXEC
	SP(1, i4) = 0 != SP(0, f8);
#endif
} break;
//}
//{ 0x8?: PF4		// vector
case v4f_neg: NEXT(1, 4, -0) {
#ifdef EXEC
	SP(0, f4) = -SP(0, f4);
	SP(1, f4) = -SP(1, f4);
	SP(2, f4) = -SP(2, f4);
	SP(3, f4) = -SP(3, f4);
#endif
} break;
case v4f_add: NEXT(1, 8, -4) {
#ifdef EXEC
	SP(4, f4) += SP(0, f4);
	SP(5, f4) += SP(1, f4);
	SP(6, f4) += SP(2, f4);
	SP(7, f4) += SP(3, f4);
#endif
} break;
case v4f_sub: NEXT(1, 8, -4) {
#ifdef EXEC
	SP(4, f4) -= SP(0, f4);
	SP(5, f4) -= SP(1, f4);
	SP(6, f4) -= SP(2, f4);
	SP(7, f4) -= SP(3, f4);
#endif
} break;
case v4f_mul: NEXT(1, 8, -4) {
#ifdef EXEC
	SP(4, f4) *= SP(0, f4);
	SP(5, f4) *= SP(1, f4);
	SP(6, f4) *= SP(2, f4);
	SP(7, f4) *= SP(3, f4);
#endif
} break;
case v4f_div: NEXT(1, 8, -4) {
#ifdef EXEC
	//~ STOP(error_div, !SP(0, f4) || !SP(1, f4) || !SP(2, f4) || !SP(3, f4));
	SP(4, f4) /= SP(0, f4);
	SP(5, f4) /= SP(1, f4);
	SP(6, f4) /= SP(2, f4);
	SP(7, f4) /= SP(3, f4);
#endif
} break;
/*case v4f_crs: NEXT(1, 8, -4) {//???
#ifdef EXEC
	f32x4 lhs = (f32x4)SPTR(0);
	f32x4 rhs = (f32x4)SPTR(4);
	float x = lhs->y * rhs->z - lhs->z * rhs->y;
	float y = lhs->z * rhs->x - lhs->x * rhs->z;
	float z = lhs->x * rhs->y - lhs->y * rhs->x;
	//~ flt32t x = SPY(LHS) * SPZ(RHS) - SPZ(LHS) * SPY(RHS);
	//~ flt32t y = SPZ(LHS) * SPX(RHS) - SPX(LHS) * SPZ(RHS);
	//~ flt32t z = SPX(LHS) * SPY(RHS) - SPY(LHS) * SPX(RHS);
	SPX(0) = x;
	SPY(0) = y;
	SPZ(0) = z;
	goto error_opc;
#endif
} break;*/
/*case p4f_cmp: NEXT(1, 8, -7) STOP(error_opc, true);{
#ifdef EXEC
	//~ 00:equal
	//~ 01:less
	//~ 10:greater
	//~ 11:unordered
	int res = 0;
	float lhs, rhs;

	lhs = SP(0, f4), rhs = SP(4, f4);
	res |= (lhs == rhs ? 0 : lhs < rhs ? 1 : lhs > rhs ? 2 : 3) << 0;

	lhs = SP(1, f4), rhs = SP(5, f4);
	res |= (lhs == rhs ? 0 : lhs < rhs ? 1 : lhs > rhs ? 2 : 3) << 2;

	lhs = SP(2, f4), rhs = SP(6, f4);
	res |= (lhs == rhs ? 0 : lhs < rhs ? 1 : lhs > rhs ? 2 : 3) << 4;

	lhs = SP(3, f4), rhs = SP(7, f4);
	res |= (lhs == rhs ? 0 : lhs < rhs ? 1 : lhs > rhs ? 2 : 3) << 6;
	SP(7, i4) = res;
#endif
} break;*/
case v4f_min: NEXT(1, 8, -4) {
#ifdef EXEC
	//~ if (SP(4, pf).x > SP(0, pf).x)
		//~ SP(4, pf).x = SP(0, pf).x;
	//~ if (SP(4, pf).y > SP(0, pf).y)
		//~ SP(4, pf).y = SP(0, pf).y;
	//~ if (SP(4, pf).z > SP(0, pf).z)
		//~ SP(4, pf).z = SP(0, pf).z;
	//~ if (SP(4, pf).w > SP(0, pf).w)
		//~ SP(4, pf).w = SP(0, pf).w;
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
#ifdef EXEC
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
#ifdef EXEC
	SP(7, f4) = 
	SP(4, pf).x * SP(0, pf).x +
	SP(4, pf).y * SP(0, pf).y +
	SP(4, pf).z * SP(0, pf).z ;
#endif
} break;
case v4f_dph: NEXT(1, 8, -7) {
#ifdef EXEC
	SP(7, f4) = SP(4, pf).w +
	SP(4, pf).x * SP(0, pf).x +
	SP(4, pf).y * SP(0, pf).y +
	SP(4, pf).z * SP(0, pf).z ;
#endif
} break;
case v4f_dp4: NEXT(1, 8, -7) {
#ifdef EXEC
	SP(7, f4) = 
	SP(4, pf).w * SP(0, pf).w +
	SP(4, pf).x * SP(0, pf).x +
	SP(4, pf).y * SP(0, pf).y +
	SP(4, pf).z * SP(0, pf).z ;
#endif
} break;
//}*/
//{ 0x9?: PD2		// complex
case v2d_neg: NEXT(1, 4, -0) {
#ifdef EXEC
	SP(0, f8) = -SP(0, f8);
	SP(2, f8) = -SP(2, f8);
#endif
} break;
case v2d_add: NEXT(1, 8, -4) {
#ifdef EXEC
	SP(4, f8) += SP(0, f8);
	SP(6, f8) += SP(2, f8);
#endif
} break;
case v2d_sub: NEXT(1, 8, -4) {
#ifdef EXEC
	SP(4, f8) -= SP(0, f8);
	SP(6, f8) -= SP(2, f8);
#endif
} break;
case v2d_mul: NEXT(1, 8, -4) {
#ifdef EXEC
	SP(4, f8) *= SP(0, f8);
	SP(6, f8) *= SP(2, f8);
#endif
} break;
case v2d_div: NEXT(1, 8, -4) {
#ifdef EXEC
	//~ STOP(error_div, SP(0, f8) || SP(2, f8));
	SP(4, f8) /= SP(0, f8);
	SP(6, f8) /= SP(2, f8);
#endif
} break;
case p4d_swz: NEXT(2, 4, -0) {
#ifdef EXEC
	uns32t swz = ip->arg.u1;
	uns32t d0 = SP((swz >> 0) & 3, u4);
	uns32t d1 = SP((swz >> 2) & 3, u4);
	uns32t d2 = SP((swz >> 4) & 3, u4);
	uns32t d3 = SP((swz >> 6) & 3, u4);
	SP(0, u4) = d0;
	SP(1, u4) = d1;
	SP(2, u4) = d2;
	SP(3, u4) = d3;
#endif
} break;
case v2d_ceq: NEXT(1, 8, -7) {
#ifdef EXEC
	SP(7, i4) = 
		(SP(4, f8) != SP(0, f8)) &
		(SP(6, f8) != SP(2, f8)) ;
#endif
} break;
case v2d_min: NEXT(1, 8, -4) {
#ifdef EXEC
	if (SP(4, f8) > SP(0, f8))
		SP(4, f8) = SP(0, f8);
	if (SP(6, f8) > SP(2, f8))
		SP(6, f8) = SP(2, f8);
#endif
} break;
case v2d_max: NEXT(1, 8, -4) {
#ifdef EXEC
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
// }-----------------------------------------------------------------------------
#undef MEMP
#undef EXEC
//~ #undef SETF
//~ #undef CDBG
#undef NEXT
#undef STOP
