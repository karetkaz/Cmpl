/* #define OPCODE_DEF(Name, Code, Size, In, Out, Text) Name = Code
 * Name: enumeration name
 * Code: enumeration value
 * Size: instruction length
 * In:   requires n elements on stack
 * Out:  produces n elements on stack
 * Text: instruction name
 */
//~ sys ========================================================================
OPCODE_DEF(opc_nop,  0x00, 1, 0, 0, "nop")          // no operation;
OPCODE_DEF(opc_nfc,  0x01, 4, 0, 0, "nfc")          // native function call
OPCODE_DEF(opc_call, 0x02, 1, 1, 1, "call")         // ip = pop(); push(IP); IP = ip;
OPCODE_DEF(opc_jmpi, 0x03, 1, 1, 0, "ret")          // IP = popref();
OPCODE_DEF(opc_jmp,  0x04, 4, 0, 0, "jmp")          // IP += arg.rel;
OPCODE_DEF(opc_jnz,  0x05, 4, 1, 0, "jnz")          // if (popi32() != 0) {IP += arg.rel;}
OPCODE_DEF(opc_jz,   0x06, 4, 1, 0, "jz")           // if (popi32() == 0) {IP += arg.rel;}
OPCODE_DEF(opc_x07,  0x07, 0, 0, 0, NULL)           // task
OPCODE_DEF(opc_x08,  0x08, 0, 0, 0, NULL)           // sync
OPCODE_DEF(opc_not,  0x09, 1, 1, 1, "not")          // sp(0).u32 = !sp(0).u32;
OPCODE_DEF(opc_inc,  0x0a, 4, 1, 1, "inc.i32")      // push32(popi32(n) + arg.rel);
OPCODE_DEF(opc_mad,  0x0b, 4, 2, 1, "mad.u32")      // sp(1).u32 += sp(0).u32 * arg.rel; pop1;
OPCODE_DEF(opc__0c,  0x0c, 0, 0, 0, NULL)           // -
OPCODE_DEF(opc__0d,  0x0d, 0, 0, 0, NULL)           // -
OPCODE_DEF(opc_spc,  0x0e, 4, 0, 0, "inc.sp")       // SP += arg.rel;
OPCODE_DEF(opc_move, 0x0f, 4, 2, 0, "copy.mem")     // todo: replace copy(sp(1), sp(0), ip.rel); pop2;

//~ stk ========================================================================
OPCODE_DEF(opc_lc32, 0x10, 5, 0, 1, "load.c32")     // push(imm32);
OPCODE_DEF(opc_lc64, 0x11, 9, 0, 2, "load.c64")     // pushx2(imm64);
OPCODE_DEF(opc_lzx1, 0x12, 1, 0, 1, "load.z32")     // push(0);
OPCODE_DEF(opc_lzx2, 0x13, 1, 0, 2, "load.z64")     // push(0, 0);
OPCODE_DEF(opc_lzx4, 0x14, 1, 0, 4, "load.z128")    // push(0, 0, 0, 0);
OPCODE_DEF(opc_dup1, 0x15, 2, 0, 1, "dup.x32")      // push(sp(arg.1: 3));
OPCODE_DEF(opc_dup2, 0x16, 2, 0, 2, "dup.x64")      // pushx2(sp(arg.1: 3));
OPCODE_DEF(opc_dup4, 0x17, 2, 0, 4, "dup.x128")     // pushx4(sp(arg.1: 3));
OPCODE_DEF(opc_set1, 0x18, 2, 1, 0, "set.x32")      // sp(arg.1: 3) = pop();
OPCODE_DEF(opc_set2, 0x19, 2, 2, 0, "set.x64")      // sp(arg.1: 3) = popx2();
OPCODE_DEF(opc_set4, 0x1a, 2, 4, 0, "set.x128")     // sp(arg.1: 3) = popx4();
OPCODE_DEF(opc_mov1, 0x1b, 3, 0, 0, "mov.x32")      // mov
OPCODE_DEF(opc_mov2, 0x1c, 3, 0, 0, "mov.x64")      // mov
OPCODE_DEF(opc_mov4, 0x1d, 3, 0, 0, "mov.x128")     // mov
OPCODE_DEF(opc_ldsp, 0x1e, 4, 0, 1, "load.sp")      // push(SP + arg.rel);
OPCODE_DEF(opc_lref, 0x1f, 5, 0, 1, "load.ref")     // todo: instruction replaceable with load.c32 or load.c64

//~ mem (indirect memory access) ===============================================
OPCODE_DEF(opc_ldis1,0x20, 1, 1, 1, "load.is8")     // copy(sp, sp(0), 1);
OPCODE_DEF(opc_ldiu1,0x21, 1, 1, 1, "load.iu8")     // copy(sp, sp(0), 1);
OPCODE_DEF(opc_ldis2,0x22, 1, 1, 1, "load.is16")    // copy(sp, sp(0), 2);
OPCODE_DEF(opc_ldiu2,0x23, 1, 1, 1, "load.iu16")    // copy(sp, sp(0), 2);
OPCODE_DEF(opc_ldi4, 0x24, 1, 1, 1, "load.i32")     // copy(sp, sp(0), 4);
OPCODE_DEF(opc_ldi8, 0x25, 1, 1, 2, "load.i64")     // copy(sp, sp(0), 8);
OPCODE_DEF(opc_ldiq, 0x26, 1, 1, 4, "load.i128")    // copy(sp, sp(0), 16);
OPCODE_DEF(opc_sti1, 0x27, 1, 2, 0, "store.i8")     // copy(sp(1), sp(0), 1); pop2;
OPCODE_DEF(opc_sti2, 0x28, 1, 2, 0, "store.i16")    // copy(sp(1), sp(0), 2); pop2;
OPCODE_DEF(opc_sti4, 0x29, 1, 2, 0, "store.i32")    // copy(sp(1), sp(0), 4); pop2;
OPCODE_DEF(opc_sti8, 0x2a, 1, 3, 0, "store.i64")    // copy(sp(1), sp(0), 8); pop3;
OPCODE_DEF(opc_stiq, 0x2b, 1, 5, 0, "store.i128")   // copy(sp(1), sp(0), 16); pop5;
OPCODE_DEF(opc_ld32, 0x2c, 4, 0, 1, "load.m32")     // copy(sp, ip.rel, 4);
OPCODE_DEF(opc_ld64, 0x2d, 4, 0, 2, "load.m64")     // copy(sp, ip.rel, 8);
OPCODE_DEF(opc_st32, 0x2e, 4, 1, 0, "store.m32")    // copy(ip.rel, sp(0), 4); pop1;
OPCODE_DEF(opc_st64, 0x2f, 4, 2, 0, "store.m64")    // copy(ip.rel, sp(0), 8); pop2;

//~ u32 ========================================================================
OPCODE_DEF(u32_cmt,  0x30, 1, 1, 1, "cmt.u32")      // sp(0).u32 = ~sp(0).u32;
OPCODE_DEF(u32_and,  0x31, 1, 2, 1, "and.u32")      // sp(1).u32 &= sp(0).u32; pop;
OPCODE_DEF(u32_ior,  0x32, 1, 2, 1, "or.u32")       // sp(1).u32 |= sp(0).u32; pop;
OPCODE_DEF(u32_mul,  0x33, 1, 2, 1, "mul.u32")      // sp(1).u32 *= sp(0).u32; pop;
OPCODE_DEF(u32_div,  0x34, 1, 2, 1, "div.u32")      // sp(1).u32 /= sp(0).u32; pop;
OPCODE_DEF(u32_rem,  0x35, 1, 2, 1, "rem.u32")      // sp(1).u32 %= sp(0).u32; pop;
OPCODE_DEF(u32___6,  0x36, 0, 0, 0, NULL)           //
OPCODE_DEF(u32___7,  0x37, 0, 0, 0, NULL)           //
OPCODE_DEF(u32_clt,  0x38, 1, 2, 1, "clt.u32")      // sp(1).u32 = sp(1).u32 < sp(0).u32; pop;
OPCODE_DEF(u32_cgt,  0x39, 1, 2, 1, "cgt.u32")      // sp(1).u32 = sp(1).u32 > sp(0).u32; pop;
OPCODE_DEF(u32_shl,  0x3a, 1, 2, 1, "shl.u32")      // sp(1).u32 <<= sp(0).u32; pop;
OPCODE_DEF(u32_shr,  0x3b, 1, 2, 1, "shr.u32")      // sp(1).u32 >>>= sp(0).u32; pop;
OPCODE_DEF(u32_sar,  0x3c, 1, 2, 1, "sar.u32")      // sp(1).i32 >>= sp(0).i32; pop;
OPCODE_DEF(u32_xor,  0x3d, 1, 2, 1, "xor.u32")      // sp(1).u32 ^= sp(0).u32; pop;
OPCODE_DEF(u32_i64,  0x3e, 1, 1, 2, "u32.2i64")     // push(i64(pop.u32))
OPCODE_DEF(u32_bit,  0x3f, 2, 1, 1, "u32.")         // extended: and, shl, shr, sar

//~ u64 ========================================================================
OPCODE_DEF(u64_cmt,  0x40, 1, 2, 2, "cmt.b64")      // complement
OPCODE_DEF(u64_and,  0x41, 1, 4, 2, "and.b64")      // sp(2).u64 &= sp(0).u64; pop2;
OPCODE_DEF(u64_ior,  0x42, 1, 4, 2, "or.b64")       // sp(2).u64 |= sp(0).u64; pop2;
OPCODE_DEF(u64_mul,  0x43, 1, 4, 2, "mul.u64")      // sp(2).u64 *= sp(0).u64; pop2;
OPCODE_DEF(u64_div,  0x44, 1, 4, 2, "div.u64")      // sp(2).u64 /= sp(0).u64; pop2;
OPCODE_DEF(u64_rem,  0x45, 1, 4, 2, "rem.u64")      // sp(2).u64 %= sp(0).u64; pop2;
OPCODE_DEF(u64___6,  0x46, 0, 0, 0, NULL)           //
OPCODE_DEF(u64___7,  0x47, 0, 0, 0, NULL)           //
OPCODE_DEF(u64_clt,  0x48, 1, 4, 1, "clt.u64")      // sp(3).u32 = sp(2).u64 < sp(0).u64; pop3;
OPCODE_DEF(u64_cgt,  0x49, 1, 4, 1, "cgt.u64")      // sp(3).u32 = sp(2).u64 > sp(0).u64; pop3;
OPCODE_DEF(u64_shl,  0x4a, 1, 3, 2, "shl.b64")      // sp(1).u64 <<= sp(0).u32; pop1;
OPCODE_DEF(u64_shr,  0x4b, 1, 3, 2, "shr.b64")      // sp(1).u64 >>= sp(0).u32; pop1;
OPCODE_DEF(u64_sar,  0x4c, 1, 3, 2, "sar.b64")      // sp(1).u64 >>>= sp(0).u32; pop1;
OPCODE_DEF(u64_xor,  0x4d, 1, 4, 2, "xor.b64")      // sp(2).u64 ^= sp(0).u64; pop2;
OPCODE_DEF(u64___e,  0x4e, 0, 0, 0, NULL)           //
OPCODE_DEF(u64___f,  0x4f, 0, 0, 0, NULL)           // TODO: extended: and, shl, shr, sar

//~ i32 ========================================================================
OPCODE_DEF(i32_neg,  0x50, 1, 1, 1, "neg.i32")      // sp(0).i32 = -sp(0).i32;
OPCODE_DEF(i32_add,  0x51, 1, 2, 1, "add.i32")      // sp(1).i32 += sp(0).i32; pop;
OPCODE_DEF(i32_sub,  0x52, 1, 2, 1, "sub.i32")      // sp(1).i32 -= sp(0).i32; pop;
OPCODE_DEF(i32_mul,  0x53, 1, 2, 1, "mul.i32")      // sp(1).i32 *= sp(0).i32; pop;
OPCODE_DEF(i32_div,  0x54, 1, 2, 1, "div.i32")      // sp(1).i32 /= sp(0).i32; pop;
OPCODE_DEF(i32_rem,  0x55, 1, 2, 1, "rem.i32")      // sp(1).i32 %= sp(0).i32; pop;
OPCODE_DEF(i32___6,  0x56, 0, 0, 0, NULL)           //
OPCODE_DEF(i32_ceq,  0x57, 1, 2, 1, "ceq.i32")      // sp(1).u32 = sp(1).i32 == sp(0).i32; pop;
OPCODE_DEF(i32_clt,  0x58, 1, 2, 1, "clt.i32")      // sp(1).u32 = sp(1).i32  < sp(0).i32; pop;
OPCODE_DEF(i32_cgt,  0x59, 1, 2, 1, "cgt.i32")      // sp(1).u32 = sp(1).i32  > sp(0).i32; pop;
OPCODE_DEF(i32_bol,  0x5a, 1, 1, 1, "i32.2bool")    // push(pop.i32 != 0)
OPCODE_DEF(i32_i64,  0x5b, 1, 1, 2, "i32.2i64")     // push(i64(pop.i32))
OPCODE_DEF(i32_f32,  0x5c, 1, 1, 1, "i32.2f32")     // push(f32(pop.i32))
OPCODE_DEF(i32_f64,  0x5d, 1, 1, 2, "i32.2f64")     // push(f64(pop.i32))
OPCODE_DEF(i32___e,  0x5e, 0, 0, 0, NULL)           //
OPCODE_DEF(i32___f,  0x5f, 0, 0, 0, NULL)           //

//~ i64 ========================================================================
OPCODE_DEF(i64_neg,  0x60, 1, 2, 2, "neg.i64")      // sp(0) = -sp(0);
OPCODE_DEF(i64_add,  0x61, 1, 4, 2, "add.i64")      // sp(2) += sp(0); pop2;
OPCODE_DEF(i64_sub,  0x62, 1, 4, 2, "sub.i64")      // sp(2) -= sp(0); pop2;
OPCODE_DEF(i64_mul,  0x63, 1, 4, 2, "mul.i64")      // sp(2) *= sp(0); pop2;
OPCODE_DEF(i64_div,  0x64, 1, 4, 2, "div.i64")      // sp(2) /= sp(0); pop2;
OPCODE_DEF(i64_rem,  0x65, 1, 4, 2, "rem.i64")      // sp(2) %= sp(0); pop2;
OPCODE_DEF(i64___6,  0x66, 0, 0, 0, NULL)           //
OPCODE_DEF(i64_ceq,  0x67, 1, 4, 1, "ceq.i64")      // sp(2).u32 = sp(2).i64 == sp(0).i64; pop3;
OPCODE_DEF(i64_clt,  0x68, 1, 4, 1, "clt.i64")      // sp(2).u32 = sp(2).i64  < sp(0).i64; pop3;
OPCODE_DEF(i64_cgt,  0x69, 1, 4, 1, "cgt.i64")      // sp(2).u32 = sp(2).i64  > sp(0).i64; pop3;
OPCODE_DEF(i64_i32,  0x6a, 1, 2, 1, "i64.2i32")     // push(i32(pop.i64))
OPCODE_DEF(i64_bol,  0x6b, 1, 2, 1, "i64.2bool")    // push(pop.i64 != 0)
OPCODE_DEF(i64_f32,  0x6c, 1, 2, 1, "i64.2f32")     // push(f32(pop.i64))
OPCODE_DEF(i64_f64,  0x6d, 1, 2, 2, "i64.2f64")     // push(f64(pop.i64))
OPCODE_DEF(i64___e,  0x6e, 0, 0, 0, NULL)           //
OPCODE_DEF(i64___f,  0x6f, 0, 0, 0, NULL)           //

//~ f32 ========================================================================
OPCODE_DEF(f32_neg,  0x70, 1, 1, 1, "neg.f32")      // sp(0) = -sp(0);
OPCODE_DEF(f32_add,  0x71, 1, 2, 1, "add.f32")      // sp(1) += sp(0); pop;
OPCODE_DEF(f32_sub,  0x72, 1, 2, 1, "sub.f32")      // sp(1) -= sp(0); pop;
OPCODE_DEF(f32_mul,  0x73, 1, 2, 1, "mul.f32")      // sp(1) *= sp(0); pop;
OPCODE_DEF(f32_div,  0x74, 1, 2, 1, "div.f32")      // sp(1) /= sp(0); pop;
OPCODE_DEF(f32_rem,  0x75, 1, 2, 1, "rem.f32")      // sp(1) %= sp(0); pop;
OPCODE_DEF(f32___6,  0x76, 0, 0, 0, NULL)           //
OPCODE_DEF(f32_ceq,  0x77, 1, 2, 1, "ceq.f32")      // sp(1).u32 = sp(1).f32 == sp(0).f32; pop;
OPCODE_DEF(f32_clt,  0x78, 1, 2, 1, "clt.f32")      // sp(1).u32 = sp(1).f32  < sp(0).f32; pop;
OPCODE_DEF(f32_cgt,  0x79, 1, 2, 1, "cgt.f32")      // sp(1).u32 = sp(1).f32  > sp(0).f32; pop;
OPCODE_DEF(f32_i32,  0x7a, 1, 1, 1, "f32.2i32")     // push(i32(pop.f32))
OPCODE_DEF(f32_i64,  0x7b, 1, 1, 2, "f32.2i64")     // push(i64(pop.f32))
OPCODE_DEF(f32_bol,  0x7c, 1, 1, 1, "f32.2bool")    // push(pop.f32 != 0)
OPCODE_DEF(f32_f64,  0x7d, 1, 1, 2, "f32.2f64")     // push(f64(pop.f32))
OPCODE_DEF(f32___e,  0x7e, 0, 0, 0, NULL)           //
OPCODE_DEF(opc_lf32, 0x7f, 5, 0, 1, "load.f32")     // todo: instruction replaceable with load.c32

//~ f64 ========================================================================
OPCODE_DEF(f64_neg,  0x80, 1, 2, 2, "neg.f64")      // sp(0) = -sp(0);
OPCODE_DEF(f64_add,  0x81, 1, 4, 2, "add.f64")      // sp(2) += sp(0); pop2;
OPCODE_DEF(f64_sub,  0x82, 1, 4, 2, "sub.f64")      // sp(2) -= sp(0); pop2;
OPCODE_DEF(f64_mul,  0x83, 1, 4, 2, "mul.f64")      // sp(2) *= sp(0); pop2;
OPCODE_DEF(f64_div,  0x84, 1, 4, 2, "div.f64")      // sp(2) /= sp(0); pop2;
OPCODE_DEF(f64_rem,  0x85, 1, 4, 2, "rem.f64")      // sp(2) %= sp(0); pop2;
OPCODE_DEF(f64___6,  0x86, 0, 0, 0, NULL)           //
OPCODE_DEF(f64_ceq,  0x87, 1, 4, 1, "ceq.f64")      // sp(2).u32 = sp(2).f64 == sp(0).f64; pop2;
OPCODE_DEF(f64_clt,  0x88, 1, 4, 1, "clt.f64")      // sp(2).u32 = sp(2).f64  < sp(0).f64; pop2;
OPCODE_DEF(f64_cgt,  0x89, 1, 4, 1, "cgt.f64")      // sp(2).u32 = sp(2).f64  > sp(0).f64; pop2;
OPCODE_DEF(f64_i32,  0x8a, 1, 2, 1, "f64.2i32")     // push(i32(pop.f64))
OPCODE_DEF(f64_i64,  0x8b, 1, 2, 2, "f64.2i64")     // push(i64(pop.f64))
OPCODE_DEF(f64_f32,  0x8c, 1, 2, 1, "f64.2f32")     // push(f32(pop.f64))
OPCODE_DEF(f64_bol,  0x8d, 1, 2, 1, "f64.2bool")    // push(pop.f64 != 0)
OPCODE_DEF(f64___e,  0x8e, 0, 0, 0, NULL)           //
OPCODE_DEF(opc_lf64, 0x8f, 9, 0, 2, "load.f64")     // todo: instruction replaceable with load.c64

//~ v4f[128] ===================================================================
OPCODE_DEF(v4f_neg,  0x90, 1, 4, 4, "neg.v4f")      // sp(0) = -sp(0);
OPCODE_DEF(v4f_add,  0x91, 1, 8, 4, "add.v4f")      // sp(4) += sp(0); pop4;
OPCODE_DEF(v4f_sub,  0x92, 1, 8, 4, "sub.v4f")      // sp(4) -= sp(0); pop4;
OPCODE_DEF(v4f_mul,  0x93, 1, 8, 4, "mul.v4f")      // sp(4) *= sp(0); pop4;
OPCODE_DEF(v4f_div,  0x94, 1, 8, 4, "div.v4f")      // sp(4) /= sp(0); pop4;
OPCODE_DEF(v4f___5,  0x95, 0, 0, 0, NULL)           //
OPCODE_DEF(v4f___6,  0x96, 0, 0, 0, NULL)           //
OPCODE_DEF(v4f_ceq,  0x97, 1, 8, 1, "ceq.v4f")      // sp(4).p4f = sp(4) == sp(0) pop7;
OPCODE_DEF(v4f_min,  0x98, 1, 8, 4, "min.v4f")      // sp(4).p4f = sp(4) <? sp(0) pop4;
OPCODE_DEF(v4f_max,  0x99, 1, 8, 4, "max.v4f")      // sp(4).p4f = sp(4) >? sp(0) pop4;
OPCODE_DEF(v4f_dp3,  0x9a, 1, 8, 1, "dp3.v4f")      // 3-component dot product
OPCODE_DEF(v4f_dp4,  0x9b, 1, 8, 1, "dp4.v4f")      // 4-component dot product
OPCODE_DEF(v4f_dph,  0x9c, 1, 8, 1, "dph.v4f")      // homogeneous dot product
OPCODE_DEF(p4x_swz,  0x9d, 2, 4, 4, "swz.p4x")      // […, a, b, c, d => […, b, c, a, d;
OPCODE_DEF(opc_ld128,0x9e, 4, 0, 4, "load.m128")    // temp: copy(sp, ip.rel, 16);
OPCODE_DEF(opc_st128,0x9f, 4, 4, 0, "store.m128")   // temp: copy(ip.rel, sp(0), 16); pop2;

//~ p128 =======================================================================
OPCODE_DEF(v2d_neg,  0xa0, 1, 4, 4, "neg.v2d")      // […, a, a, b, b => […, -(a, a), -(b, b);
OPCODE_DEF(v2d_add,  0xa1, 1, 8, 4, "add.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a + c, a + c), (b + d, b + d);
OPCODE_DEF(v2d_sub,  0xa2, 1, 8, 4, "sub.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a - c, a - c), (b - d, b - d);
OPCODE_DEF(v2d_mul,  0xa3, 1, 8, 4, "mul.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a * c, a * c), (b * d, b * d);
OPCODE_DEF(v2d_div,  0xa4, 1, 8, 4, "div.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a / c, a / c), (b / d, b / d);
OPCODE_DEF(p2d___5,  0xa5, 0, 0, 0, NULL)           //
OPCODE_DEF(p2d___6,  0xa6, 0, 0, 0, NULL)           //
OPCODE_DEF(v2d_ceq,  0xa7, 1, 8, 1, "ceq.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a == c, a == c), (b == d, b == d);
OPCODE_DEF(v2d_min,  0xa8, 1, 8, 4, "min.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a <? c, a <? c), (b <? d, b <? d);
OPCODE_DEF(v2d_max,  0xa9, 1, 8, 4, "max.v2d")      // […, (a, a), (b, b), (c, c), (d, d) => […, (a >? c, a >? c), (b >? d, b >? d);
OPCODE_DEF(p2d___a,  0xaa, 0, 0, 0, NULL)           //
OPCODE_DEF(p2i___b,  0xab, 0, 0, 0, NULL)           // TODO: p2i.alu
OPCODE_DEF(p4i___c,  0xac, 0, 0, 0, NULL)           // TODO: p4i.alu
OPCODE_DEF(p8i___d,  0xad, 0, 0, 0, NULL)           // TODO: p8i.alu
OPCODE_DEF(p16i__e,  0xae, 0, 0, 0, NULL)           // TODO: p16i.alu
OPCODE_DEF(p4d___f,  0xaf, 0, 0, 0, NULL)           //

#undef OPCODE_DEF

/* arithmetic operations table
	bit: u32 / u64
	num: i32 / i64 / f32 / f64
	vec: v4f / v2d / ...

	code        bit         num         vec

	0x?0        cmt         neg         neg
	0x?1        or          add         add
	0x?2        xor         sub         sub
	0x?3        umul        mul         mul
	0x?4        udiv        div         div
	0x?5        urem        rem         rem
	0x?6        ?           ?           ?
	0x?7        and         ceq         ceq
	0x?8        ult         clt         min
	0x?9        ugt         cgt         max
	0x?a        shl         2i32        dp3/<?>
	0x?b        shr         2f32        dph/<?>
	0x?c        sar         2i64        dp4/<?>
	0x?d        ext         2f64        crs/<?>

	0x?e        <?>         <?>         <?>
	0x?f           ?load | store | const?

	bit.ext32(byte arg): mask, shl, shr, sar with constant.
//~ */

/* extended fast opcodes
	opc_ext argc = 3(24bit): [mem:2][res:8][lhs:7][rhs:7]
	struct {                // extended: 4 bytes `res := lhs OP rhs`
		uint32_t mem:2;     // indirect memory access
		uint32_t res:8;     // res
		uint32_t lhs:7;     // lhs
		uint32_t rhs:7;     // rhs
	} ext;
	/+ --8<-------------------------------------------
	void *res = sp + ip->ext.res;
	void *lhs = sp + ip->ext.lhs;
	void *rhs = sp + ip->ext.rhs;

	// check stack
	CHKSTK(ip->ext.res);
	CHKSTK(ip->ext.lhs);
	CHKSTK(ip->ext.rhs);

	// memory indirection
	switch (ip->ext.mem) {
		case 0:
			// no indirection
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
		case ext_rem: *(type*)res = (*(type*)lhs) % (*(type*)rhs); break;
		case ext_..1: res = lhs ... rhs; break;
		case ext_..2: res = lhs ... rhs; break;
	}
	// +/ --8<----------------------------------------
//~ */

/* extended packed
	opc_p4i argc = 1(8bit): [sat:1][uns:1][typ:2][opc:4]
	bit:[0-1, 2: type, unsigned]
		00:	i8[16]
		01:	i16[8]
		10:	i32[4]
		11:	i64[2]

	bit:[3: saturate:]
		.sat

	opc:
		neg = 0x00,	sat: cmt/neg
		add = 0x01,	sat: saturate
		sub = 0x02,	sat: saturate
		mul = 0x03,	sat: hi / lo
		div = 0x04,	sat: quo/rem
		rem = 0x05,	// avg

		??? = 0x06,	???

		ceq = 0x07,	???
		min = 0x08,	sat: ??
		max = 0x09,	sat: ??

		shl = 0x0a,
		shr = 0x0b,
		and = 0x0c,
		ior = 0x0d,
		xor = 0x0e,

		??? = 0x0f,

//~ */
