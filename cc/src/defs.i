#ifdef TOKDEF
/* #define TOKDEF(NAME, TYPE, opArgs, STR) {KIND, TYPE, SIZE, STR},
	NAME: enum name

	TYPE:	oprator priority
			typename size, if == ff then keyword

	opArgs: if != 0 then operator
// */
TOKDEF(TYPE_any, 0x00, 0, ".err")	// error
TOKDEF(TYPE_vid, 0x00, 0, ".vid")	// void
TOKDEF(TYPE_bit, 0x04, 0, ".bit")	// bool
TOKDEF(TYPE_i32, 0x04, 0, ".i32")	// int8, int16, int32
TOKDEF(TYPE_i64, 0x08, 0, ".i64")	// int64
TOKDEF(TYPE_u32, 0x04, 0, ".u32")	// uint8, uint16, uint32
//~ TOKDEF(TYPE_u64, 0x08, 0, ".u64")		// uint64: no vm support
TOKDEF(TYPE_f32, 0x04, 0, ".f32")	// float32
TOKDEF(TYPE_f64, 0x08, 0, ".f64")	// float64
TOKDEF(TYPE_ptr, 0x04, 0, ".ptr")	// pointer


TOKDEF(TYPE_def, 0x00, 0, "define")		// typedef or inline expression
TOKDEF(TYPE_rec, 0x00, 0, "struct")		// union := struct:0
TOKDEF(TYPE_ref, 0x00, 0, ".ref")		// variable / function
TOKDEF(TYPE_arr, 0x00, 0, ".arr")		// pointer, string, array, ..., ???

//~ TOKDEF(TYPE_p4x, 0x00, 128, ".p4x")

//~ TOKDEF(WORD_nsp, 0x00, 0, "namespace")

TOKDEF(QUAL_con, 0x00, 0, "const")	// constant
TOKDEF(QUAL_sta, 0x00, 0, "static")
TOKDEF(QUAL_par, 0x00, 0, "parallel")
//~ TOKDEF(QUAL_syn, 0x00, 0, "synchronized")

//~ vmInterface ================================================================
TOKDEF(EMIT_opc, 0x00,   0, "emit")		// opcodes in emit(opcode, args...)
// typeof and sizeof  : `var`.class / `var`.class.size

//~ Statements =================================================================
//! keep beg the first and end the last statement token
TOKDEF(STMT_beg, 0x00, 0, ".beg")		// stmt: list {...}
TOKDEF(STMT_do,  0x00, 0, ".do")		// stmt: decl / expr
TOKDEF(STMT_for, 0x00, 0, "for")		// stmt: for, while, repeat
TOKDEF(STMT_if,  0x00, 0, "if")			// stmt: if then else
TOKDEF(STMT_els, 0x00, 0, "else")		// ????
TOKDEF(STMT_brk, 0x00, 0, "break")		// stmt: break
TOKDEF(STMT_con, 0x00, 0, "continue")	// stmt: continue
TOKDEF(STMT_ret, 0x00, 0, "return")	// stmt: return
TOKDEF(STMT_end, 0x00, 0, ".end")		// destruct calls ?

//~ Operators ==================================================================
TOKDEF(OPER_idx, 0x0f, 2, ".idx")		// a[i]		index
TOKDEF(OPER_fnc, 0x0f, 2, ".fnc")		// a(x)		function call, cast, ctor, dtor = cast(void, var &), emit, ...
TOKDEF(OPER_dot, 0x0f, 2, ".dot")		// a.b		member

TOKDEF(OPER_adr, 0x1e, 1, ".adr")		// & a		address of
TOKDEF(OPER_pls, 0x1e, 1, ".pls")		// + a		unary plus
TOKDEF(OPER_mns, 0x1e, 1, ".neg")		// - a		unary minus
TOKDEF(OPER_cmt, 0x1e, 1, ".cmt")		// ~ a		complement / ?reciprocal
TOKDEF(OPER_not, 0x1e, 1, ".not")		// ! a		logical not

TOKDEF(OPER_mul, 0x0d, 2, ".mul")		// a * b
TOKDEF(OPER_div, 0x0d, 2, ".div")		// a / b
TOKDEF(OPER_mod, 0x0d, 2, ".mod")		// a % b

TOKDEF(OPER_add, 0x0c, 2, ".add")		// a + b
TOKDEF(OPER_sub, 0x0c, 2, ".sub")		// a - b

TOKDEF(OPER_shr, 0x0b, 2, ".shr")		// a >> b
TOKDEF(OPER_shl, 0x0b, 2, ".shl")		// a << b

TOKDEF(OPER_gte, 0x0a, 2, ".cgt")		// a > b
TOKDEF(OPER_geq, 0x0a, 2, ".cge")		// a >= b
TOKDEF(OPER_lte, 0x0a, 2, ".clt")		// a < b
TOKDEF(OPER_leq, 0x0a, 2, ".cle")		// a <= b
TOKDEF(OPER_equ, 0x09, 2, ".ceq")		// a == b
TOKDEF(OPER_neq, 0x09, 2, ".cne")		// a != b

TOKDEF(OPER_and, 0x08, 2, ".and")		// a & b
TOKDEF(OPER_xor, 0x07, 2, ".xor")		// a ^ b
TOKDEF(OPER_ior, 0x06, 2, ".ior")		// a | b

TOKDEF(OPER_lnd, 0x05, 2, "&&")			// a && b
TOKDEF(OPER_lor, 0x04, 2, "||")			// a || b
TOKDEF(OPER_sel, 0x13, 3, "?:")			// a ? b : c

TOKDEF(ASGN_set, 0x12, 2, ":=")			// a := b
TOKDEF(ASGN_mul, 0x12, 2, "*=")			// a *= b
TOKDEF(ASGN_div, 0x12, 2, "/=")			// a /= b
TOKDEF(ASGN_mod, 0x12, 2, "%=")			// a %= b
TOKDEF(ASGN_add, 0x12, 2, "+=")			// a += b
TOKDEF(ASGN_sub, 0x12, 2, "-=")			// a -= b
TOKDEF(ASGN_shl, 0x12, 2, "<<=")		// a <<= b
TOKDEF(ASGN_shr, 0x12, 2, ">>=")		// a >>= b
TOKDEF(ASGN_and, 0x12, 2, "&=")			// a &= b
TOKDEF(ASGN_xor, 0x12, 2, "^=")			// a ^= b
TOKDEF(ASGN_ior, 0x12, 2, "|=")			// a |= b

TOKDEF(OPER_com, 0x01, 2, ",")			// a, b

//~ temp =======================================================================

TOKDEF(PNCT_lc , 0x00, 0, "[par")		// curlies
TOKDEF(PNCT_rc , 0x00, 0, "]par")
TOKDEF(PNCT_lp , 0x00, 0, "(par")		// parentheses
TOKDEF(PNCT_rp , 0x00, 0, ")par")
TOKDEF(PNCT_qst, 0x00, 0, "?")			// question mark
TOKDEF(PNCT_cln, 0x00, 0, ":")			// colon

//~ TOKDEF(UNIT_def, 0x00, 0, "module")
//~ TOKDEF(OPER_kwd, 0x00, 0, "operator")
//~ TOKDEF(ENUM_kwd, 0x00, 0, "enum")		// keyword onlyd

/*
//~ Operators ==================================================================
TOKDEF(OPER_min, 0x09, 2, OP, "<?")		// a <? b
TOKDEF(OPER_max, 0x09, 2, OP, ">?")		// a >? b
TOKDEF(OPER_pow, 0x0a, 2, OP, "**")		// a ** b
TOKDEF(ASGN_min, 0x09, 2, OP, "<?=")	// a <?= b
TOKDEF(ASGN_max, 0x09, 2, OP, ">?=")	// a >?= b
TOKDEF(ASGN_pow, 0x0a, 2, OP, "**=")		// a **= b

//{ ============================================================================
//~ TOKDEF(OPER_new, 0x00, 2, 2, "new")
//~ TOKDEF(OPER_del, 0x00, 0, 2, "delete")

//? TOKDEF(STMT_try, 0x00, 0, 2, "try")			//
//? TOKDEF(STMT_thr, 0x00, 0, 2, "throw")		//
//? TOKDEF(STMT_cth, 0x00, 0, 2, "catch")		//
//? TOKDEF(STMT_fin, 0x00, 0, 2, "finally")		//

//~ TOKDEF(OPER_go2, 0x00, 0, 2, "goto")

//~ TOKDEF(OPER_swi, 0x00, 0, 2, "switch")
//~ TOKDEF(OPER_cas, 0x00, 0, 2, "case")
//? TOKDEF(OPER_def, 0x00, 0, 2, "default")		// or use else

//~ TOKDEF(OPER_wht, 0x00, 0, 2, "while")		// while '(' <expr>')' <stmt> => for(;<expr>;) <stmt>
//~ TOKDEF(OPER_whf, 0x00, 0, 2, "until")		// until '(' <expr>')' <stmt> => for(;!<expr>;) <stmt>
//~ TOKDEF(OPER_rep, 0x00, 0, 2, "repeat")		// repeat <stmt> ((until '(' <expr> ')' ) | (while '(' <expr> ')' ))?

//} */

#undef TOKDEF
#endif
#ifdef OPCDEF
/* #define OPCDEF(Name, Code, Size, Chk, Diff, Time, Mnem) Name = Code
 * Name: enum name;
 * Code: enum value;
 * Size: opcode length; (0: UNIMPLEMENTED OR VARIABLE);
 * Diff: pops n stack elements; (9: VARIABLE);
 * Chk:  requires n stack elements; (9: VARIABLE);
 * Tick: ticks for executing instruction;
 */
//~ sys ========================================================================
OPCDEF(opc_nop,  0x00, 1, 0, +0, 1, "nop")		// ;					[…, a, b, c => […, a, b, c;
OPCDEF(opc_loc,  0x01, 2, 0, +9, 1, "loc")		// SP += 4*arg.u1;		[…, a, b, c => […, a, b, c, …;
OPCDEF(opc_drop, 0x02, 2, 9, +9, 1, "drop")		// SP -= 4*arg.u1;		[…, a, b, c => […, a, b;
OPCDEF(opc_inc,  0x03, 4, 1,  0, 1, "inc")		// push32(popi32(n) + arg.rel);
OPCDEF(opc_spc,  0x04, 4, 0,  0, 1, "spc")		// SP += arg.rel;
OPCDEF(opc_ldsp, 0x05, 4, 0, +1, 1, "ldsp")		// push(SP + arg.rel);		[…, a, b, c  => […, a, b, c, SP;
OPCDEF(opc__x06, 0x06, 0, 0,  0, 1, NULL)		// push(IP); ip += arg.rel;	[…, a, b, c  => […, a, b, c, IP;
OPCDEF(opc_call, 0x07, 1, 1,  0, 1, "call")		// swap(IP, *SP);			[…, a, b, IP => […, a, b, IP;
OPCDEF(opc_jmpi, 0x08, 1, 1, -1, 1, "jmpi")		// IP = popref();			[…, a, b, c  => […, a, b;
OPCDEF(opc_jmp,  0x09, 4, 0,  0, 1, "jmp")		// IP += arg.rel;			[…, a, b, c  => […, a, b, c;
OPCDEF(opc_jnz,  0x0a, 4, 1, -1, 1, "jnz")		// if popi32()!=0 IP += arg.rel;	[…, a, b, c => […, a, b;
OPCDEF(opc_jz,   0x0b, 4, 1, -1, 1, "jz")		// if popi32()==0 IP += arg.rel;	[…, a, b, c => […, a, b;
OPCDEF(opc_not,  0x0c, 1, 1,  0, 1, "not")		// sp(0) = !sp(0);
OPCDEF(opc_task, 0x0d, 4, 0,  0, 1, "task")		// arg.3: [code:16][data:8] task, [?fork if (arg.code == 0)]
OPCDEF(opc__x0f, 0x0e, 0, 0,  0, 1, "sync")		// wait, join, sync
OPCDEF(opc_libc, 0x0f, 2, 0,  9, 1, "libc")		// lib call
//~ stk ========================================================================
OPCDEF(opc__x10, 0x10, 0, 0,  0, 1, NULL)
OPCDEF(opc__x11, 0x11, 0, 0,  0, 1, NULL)
OPCDEF(opc_ldc4, 0x12, 5, 0, +1, 1, "ldc.b32")	// push(arg.4);			:2[…, a, b, c => […, a, b, c, 2;
OPCDEF(opc_ldc8, 0x13, 9, 0, +2, 1, "ldc.b64")	// push(arg.8);			:2[…, a, b, c => […, a, b, c, 2;
OPCDEF(opc_dup1, 0x14, 2, 9, +1, 1, "dup.x1")	// push(sp(n));			:2[…, a, b, c => […, a, b, c, a;
OPCDEF(opc_dup2, 0x15, 2, 9, +2, 1, "dup.x2")	// push(sp(n)) x 2;		:2[…, a, b, c => […, a, b, c, a, b;
OPCDEF(opc_dup4, 0x16, 2, 9, +4, 1, "dup.x4")	// push(sp(n)) x 4;		:3[…, a, b, c, d => […, a, b, c, d, a, b, c, d;
OPCDEF(opc_set1, 0x17, 2, 9,  9, 1, "set.x1")	// 
OPCDEF(opc_set2, 0x18, 2, 9,  9, 1, "set.x2")	// 
OPCDEF(opc_set4, 0x19, 2, 9,  9, 1, "set.x4")	// 
OPCDEF(opc_ldz1, 0x1a, 1, 0, +1, 1, "ldz.x1")	// push(0);				[…, a, b, c => […, a, b, c, 0;
OPCDEF(opc_ldz2, 0x1b, 1, 0, +2, 1, "ldz.x2")	// push(0, 0);			[…, a, b, c => […, a, b, c, 0, 0;
OPCDEF(opc_ldz4, 0x1c, 1, 0, +4, 1, "ldz.x4")	// push(0, 0, 0, 0);	[…, a, b, c => […, a, b, c, 0, 0, 0, 0;
OPCDEF(opc_ldcf, 0x1d, 5, 0, +1, 1, "ldc.f32")	// TODO: set.4.msk
OPCDEF(opc_ldcF, 0x1e, 9, 0, +2, 1, "ldc.f64")	// TODO: dup.4.swz
OPCDEF(opc_ldcr, 0x1f, 5, 0, +1, 1, "ldc.ref")	// 
//~ mem ========================================================================
OPCDEF(opc_ldi1, 0x20, 1, 1, +0, 1, "ldi.8")	// copy(sp, sp(0) 1);			[…, a, b, c => […, a, b, *c;
OPCDEF(opc_ldi2, 0x21, 1, 1, +0, 1, "ldi.16")	// copy(sp, sp(0) 2);			[…, a, b, c => […, a, b, *c;
OPCDEF(opc_ldi4, 0x22, 1, 1, +0, 1, "ldi.32")	// copy(sp, sp(0) 4);			[…, a, b, c => […, a, b, *c;
OPCDEF(opc_ldi8, 0x23, 1, 1, +1, 1, "ldi.64")	// copy(sp, sp(0) 8);			[…, a, b, c => […, a, b, *c12;
OPCDEF(opc_ldiq, 0x24, 0, 1, +3, 1, "ldi.128")	// copy(sp, sp(0) 16);			[…, a, b, c => […, a, b, *c1234;
OPCDEF(opc_sti1, 0x25, 1, 2, -2, 1, "sti.8")	// copy(sp(1) sp(0) 1);pop2;	[…, a, b, c => […, a
OPCDEF(opc_sti2, 0x26, 1, 2, -2, 1, "sti.16")	// copy(sp(1) sp(0) 2);pop2;	[…, a, b, c => […, a
OPCDEF(opc_sti4, 0x27, 1, 2, -2, 1, "sti.32")	// copy(sp(1) sp(0) 4);pop2;	[…, a, b, c => […, a
OPCDEF(opc_sti8, 0x28, 1, 3, -3, 1, "sti.64")	// copy(sp(1) sp(0) 8);pop3;	[…, a, b, c => […, a
OPCDEF(opc_stiq, 0x29, 0, 5, -5, 1, "sti.128")	// copy(sp(1) sp(0) 16);pop5;	[…, a, b, c => […, a
OPCDEF(opc_ld32, 0x2a, 4, 0, +1, 1, "ldm.b32")	// (32)
OPCDEF(opc_ld64, 0x2b, 4, 0, +2, 1, "ldm.b64")	// (64)
OPCDEF(opc___2c, 0x2c, 0, 0,  0, 1, NULL)		//
OPCDEF(opc_st32, 0x2d, 4, 1, -1, 1, "stm.b32")	//
OPCDEF(opc_st64, 0x2e, 4, 2, -2, 1, "stm.b64")	//
OPCDEF(opc_move, 0x2f, 4, 2, -2, 1, "mem.cpy")	//
//~ bit[ 32] ===================================================================
OPCDEF(b32_cmt,  0x30, 1, 1,  0, 1, "b32.cmt")	// sp(0) = ~sp(0);
OPCDEF(b32_and,  0x31, 1, 2, -1, 1, "b32.and")	// sp(1).u32 &= sp(0).u32; pop;
OPCDEF(b32_ior,  0x32, 1, 2, -1, 1, "b32.ior")	// sp(1).u32 |= sp(0).u32; pop;
OPCDEF(b32_xor,  0x33, 1, 2, -1, 1, "b32.xor")	// sp(1).u32 ^= sp(0).u32; pop;
OPCDEF(b32_shl,  0x34, 1, 2, -1, 1, "b32.shl")	// sp(1).u32 <<= sp(0).u32; pop;
OPCDEF(b32_shr,  0x35, 1, 2, -1, 1, "b32.shr")	// sp(1).u32 >>= sp(0).u32; pop;
OPCDEF(b32_sar,  0x36, 1, 2, -1, 1, "b32.sar")	// sp(1).i32 >>= sp(0).i32; pop;
OPCDEF(b32_bit,  0x37, 2, 1,  0, 1, "b32.")		// and, shl, shr, sar
OPCDEF(u32_clt,  0x38, 1, 2, -1, 1, "u32.clt")	// sp(1).b32 = sp(1).u32 < sp(0).u32; pop;
OPCDEF(u32_cgt,  0x39, 1, 2, -1, 1, "u32.cgt")	// sp(1).b32 = sp(1).u32 > sp(0).u32; pop;
OPCDEF(u32_mul,  0x3a, 1, 2, -1, 1, "u32.mul")	// sp(1) *= sp(0); pop;
OPCDEF(u32_div,  0x3b, 1, 2, -1, 1, "u32.div")	// sp(1) /= sp(0); pop;
OPCDEF(u32_mod,  0x3c, 1, 2, -1, 1, "u32.mod")	// sp(1) %= sp(0); pop;
OPCDEF(u32_mad,  0x3d, 1, 3, -2, 1, "u32.mad")	// sp(2) += sp(1)*sp(0); pop2;	[…, a, b, c => […, a + b * c;
OPCDEF(u32_i64,  0x3e, 1, 1, +1, 1, "u32.cvt2i64")
OPCDEF(u32___3f, 0x3f, 0, 0,  0, 1, "u32.ext")	// 

//~ OPCDEF(b32___1,  0x31, 0, 2, -1, 1, "b32.adc")	// sp(1) += sp(0); pop;
//~ OPCDEF(b32___2,  0x32, 0, 2, -1, 1, "b32.sbb")	// sp(1) -= sp(0); pop;
//~ i32[ 32] ===================================================================
OPCDEF(i32_neg,  0x40, 1, 1, -0, 1, "i32.neg")	// sp(0).i32 = -sp(0).i32;
OPCDEF(i32_add,  0x41, 1, 2, -1, 1, "i32.add")	// sp(1).i32 += sp(0).i32; pop;
OPCDEF(i32_sub,  0x42, 1, 2, -1, 1, "i32.sub")	// sp(1).i32 -= sp(0).i32; pop;
OPCDEF(i32_mul,  0x43, 1, 2, -1, 1, "i32.mul")	// sp(1).i32 *= sp(0).i32; pop;
OPCDEF(i32_div,  0x44, 1, 2, -1, 1, "i32.div")	// sp(1).i32 /= sp(0).i32; pop;
OPCDEF(i32_mod,  0x45, 1, 2, -1, 1, "i32.mod")	// sp(1).i32 %= sp(0).i32; pop;
OPCDEF(i32___6,  0x46, 0, 0, -0, 1, NULL)		// sp(0).i32 = {...} (sp(0).i32);
OPCDEF(i32_ceq,  0x47, 1, 2, -1, 1, "i32.ceq")	// sp(1).b32 = sp(1).i32 == sp(0).i32; pop;
OPCDEF(i32_clt,  0x48, 1, 2, -1, 1, "i32.clt")	// sp(1).b32 = sp(1).i32  < sp(0).i32; pop;
OPCDEF(i32_cgt,  0x49, 1, 2, -1, 1, "i32.cgt")	// sp(1).b32 = sp(1).i32  > sp(0).i32; pop;
OPCDEF(i32___a,  0x4a, 0, 0, -0, 1, NULL)	// 
OPCDEF(i32_bol,  0x4b, 1, 1, -0, 1, "i32.cvt2bool")	// push(pop.i32 != 0)
OPCDEF(i32_f32,  0x4c, 1, 1, -0, 1, "i32.cvt2f32")	// push(f32(pop.i32))
OPCDEF(i32_i64,  0x4d, 1, 1, +1, 1, "i32.cvt2i64")	// push(i64(pop.i32))
OPCDEF(i32_f64,  0x4e, 1, 1, +1, 1, "i32.cvt2f64")	// push(f64(pop.i32))
OPCDEF(i32___f,  0x4f, 0, 0, -0, 1, NULL)		//-Extended ops
//~ f32[ 32] ===================================================================
OPCDEF(f32_neg,  0x50, 1, 1, -0, 1, "f32.neg")	// sp(0) = -sp(0);
OPCDEF(f32_add,  0x51, 1, 2, -1, 1, "f32.add")	// sp(1) += sp(0); pop;
OPCDEF(f32_sub,  0x52, 1, 2, -1, 1, "f32.sub")	// sp(1) -= sp(0); pop;
OPCDEF(f32_mul,  0x53, 1, 2, -1, 1, "f32.mul")	// sp(1) *= sp(0); pop;
OPCDEF(f32_div,  0x54, 1, 2, -1, 1, "f32.div")	// sp(1) /= sp(0); pop;
OPCDEF(f32_mod,  0x55, 1, 2, -1, 1, "f32.mod")	// sp(1) %= sp(0); pop;
OPCDEF(f32___6,  0x56, 0, 0, -0, 1, NULL)	// sp(0) = {...} (sp(0));
OPCDEF(f32_ceq,  0x57, 1, 2, -1, 1, "f32.ceq")	// sp(1).b32 = sp(1).f32 == sp(0).f32; pop;
OPCDEF(f32_clt,  0x58, 1, 2, -1, 1, "f32.clt")	// sp(1).b32 = sp(1).f32  < sp(0).f32; pop;
OPCDEF(f32_cgt,  0x59, 1, 2, -1, 1, "f32.cgt")	// sp(1).b32 = sp(1).f32  > sp(0).f32; pop;
OPCDEF(f32___a,  0x5a, 0, 0, -0, 1, NULL)	// abs
OPCDEF(f32_i32,  0x5b, 1, 1, -0, 1, "f32.cvt2i32")	// push(i32(pop.f32))
OPCDEF(f32_bol,  0x5c, 1, 1, -0, 1, "f32.cvt2bool")	// push(pop.f32 != 0)
OPCDEF(f32_i64,  0x5d, 1, 1, +1, 1, "f32.cvt2i64")	// push(i64(pop.f32))
OPCDEF(f32_f64,  0x5e, 1, 1, +1, 1, "f32.cvt2f64")	// push(f64(pop.f32))
OPCDEF(f32___f,  0x5f, 0, 0, -0, 1, NULL)		//-Extended ops
//~ i64[ 64] ===================================================================
OPCDEF(i64_neg,  0x60, 1, 2, -0, 1, "i64.neg")	// sp(0) = -sp(0);
OPCDEF(i64_add,  0x61, 1, 4, -2, 1, "i64.add")	// sp(2) += sp(0); pop2;
OPCDEF(i64_sub,  0x62, 1, 4, -2, 1, "i64.sub")	// sp(2) -= sp(0); pop2;
OPCDEF(i64_mul,  0x63, 1, 4, -2, 1, "i64.mul")	// sp(2) *= sp(0); pop2;
OPCDEF(i64_div,  0x64, 1, 4, -2, 1, "i64.div")	// sp(2) /= sp(0); pop2;
OPCDEF(i64_mod,  0x65, 1, 4, -2, 1, "i64.mod")	// sp(2) %= sp(0); pop2;
OPCDEF(i64___6,  0x66, 0, 0, -0, 1, NULL)	// sp(0) = {...} (sp(0));
OPCDEF(i64_ceq,  0x67, 1, 4, -3, 1, "i64.ceq")	// sp(2).b32 = sp(2).i64 == sp(0).i64; pop2;
OPCDEF(i64_clt,  0x68, 1, 4, -3, 1, "i64.clt")	// sp(2).b32 = sp(2).i64  < sp(0).i64; pop2;
OPCDEF(i64_cgt,  0x69, 1, 4, -3, 1, "i64.cgt")	// sp(2).b32 = sp(2).i64  > sp(0).i64; pop2;
OPCDEF(i64___a,  0x6a, 0, 0, -0, 1, NULL)	// abs
OPCDEF(i64_i32,  0x6b, 1, 2, -1, 1, "i64.cvt2i32")	// push(i32(pop.i64))
OPCDEF(i64_f32,  0x6c, 1, 2, -1, 1, "i64.cvt2f32")	// push(f32(pop.i64))
OPCDEF(i64_bol,  0x6d, 1, 2, -1, 1, "i64.cvt2bool")	// push(pop.i64 != 0)
OPCDEF(i64_f64,  0x6e, 1, 2, -0, 1, "i64.cvt2f64")	// push(f64(pop.i64))
OPCDEF(i64___f,  0x6f, 0, 0, -0, 1, NULL)		//-Extended ops
//~ f64[ 64] ===================================================================
OPCDEF(f64_neg,  0x70, 1, 2, -0, 1, "f64.neg")	// sp(0) = -sp(0);
OPCDEF(f64_add,  0x71, 1, 4, -2, 1, "f64.add")	// sp(2) += sp(0); pop2;
OPCDEF(f64_sub,  0x72, 1, 4, -2, 1, "f64.sub")	// sp(2) -= sp(0); pop2;
OPCDEF(f64_mul,  0x73, 1, 4, -2, 1, "f64.mul")	// sp(2) *= sp(0); pop2;
OPCDEF(f64_div,  0x74, 1, 4, -2, 1, "f64.div")	// sp(2) /= sp(0); pop2;
OPCDEF(f64_mod,  0x75, 1, 4, -2, 1, "f64.mod")	// sp(2) %= sp(0); pop2;
OPCDEF(f64___6,  0x76, 0, 0, -0, 1, NULL)	// sp(0) = {min, max, ...} (sp(0));
OPCDEF(f64_ceq,  0x77, 1, 4, -3, 1, "f64.ceq")	// sp(2).b32 = sp(2).f64 == sp(0).f64; pop2;
OPCDEF(f64_clt,  0x78, 1, 4, -3, 1, "f64.clt")	// sp(2).b32 = sp(2).f64  < sp(0).f64; pop2;
OPCDEF(f64_cgt,  0x79, 1, 4, -3, 1, "f64.cgt")	// sp(2).b32 = sp(2).f64  > sp(0).f64; pop2;
OPCDEF(f64___a,  0x7a, 0, 0, -0, 1, NULL)	//
OPCDEF(f64_i32,  0x7b, 1, 2, -1, 1, "f64.cvt2i32")	// push(i32(pop.f64))
OPCDEF(f64_f32,  0x7c, 1, 2, -1, 1, "f64.cvt2f32")	// push(f32(pop.f64))
OPCDEF(f64_i64,  0x7d, 1, 2, -0, 1, "f64.cvt2i64")	// push(i64(pop.f64))
OPCDEF(f64_bol,  0x7e, 1, 2, -1, 1, "f64.cvt2bool")	// push(pop.f64 != 0)
OPCDEF(f64___f,  0x7f, 0, 0, -0, 1, NULL)		//-Extended ops
//~ v4f[128] ===================================================================8
OPCDEF(v4f_neg,  0x80, 1, 4, -0, 1, "v4f.neg")	// sp(0) = -sp(0);
OPCDEF(v4f_add,  0x81, 1, 8, -4, 1, "v4f.add")	// sp(4) += sp(0); pop4;
OPCDEF(v4f_sub,  0x82, 1, 8, -4, 1, "v4f.sub")	// sp(4) -= sp(0); pop4;
OPCDEF(v4f_mul,  0x83, 1, 8, -4, 1, "v4f.mul")	// sp(4) *= sp(0); pop4;
OPCDEF(v4f_div,  0x84, 1, 8, -4, 1, "v4f.div")	// sp(4) /= sp(0); pop4;
OPCDEF(v4f_crs,  0x85, 0, 8, -4, 1, "v4f.crs")	// cross
OPCDEF(v4f___6,  0x86, 0, 0, -0, 1, NULL)	// 
OPCDEF(v4f_ceq,  0x87, 0, 8, -7, 1, "v4f.ceq")	// sp(4).b32 = sp(4) == sp(0) pop7;
OPCDEF(v4f_min,  0x88, 1, 8, -4, 1, "v4f.min")	// sp(4).b32 = sp(4) <? sp(0) pop4;
OPCDEF(v4f_max,  0x89, 1, 8, -4, 1, "v4f.max")	// sp(4).b32 = sp(4) >? sp(0) pop4;
OPCDEF(v4f___a,  0x8a, 0, 0, -0, 1, NULL)		//
OPCDEF(v4f___b,  0x8b, 0, 0, -0, 1, NULL)		//
OPCDEF(v4f_dp3,  0x8c, 1, 8, -7, 1, "v4f.dp3")	//
OPCDEF(v4f_dp4,  0x8d, 1, 8, -7, 1, "v4f.dp4")	//
OPCDEF(v4f_dph,  0x8e, 1, 8, -7, 1, "v4f.dph")	//
OPCDEF(v4f___f,  0x8f, 0, 0, -0, 1, NULL)		//-Extended ops
//~ p4d[128] ===================================================================9
OPCDEF(v2d_neg,  0x90, 1, 4, -0, 1, "v2d.neg")	// sp(0) = -sp(0);
OPCDEF(v2d_add,  0x91, 1, 8, -4, 1, "v2d.add")	// sp(4) += sp(0); pop4;
OPCDEF(v2d_sub,  0x92, 1, 8, -4, 1, "v2d.sub")	// sp(4) -= sp(0); pop4;
OPCDEF(v2d_mul,  0x93, 1, 8, -4, 1, "v2d.mul")	// sp(4) *= sp(0); pop4;
OPCDEF(v2d_div,  0x94, 1, 8, -4, 1, "v2d.div")	// sp(4) /= sp(0); pop4;
OPCDEF(p2d___5,  0x95, 0, 0, -0, 1, NULL)		// mod
OPCDEF(p4x_swz,  0x96, 2, 4, -0, 1, "p4x.swz")	// swizzle
OPCDEF(v2d_ceq,  0x97, 0, 8, -7, 1, "v2d.ceq")	// sp(1) = sp(1) == sp(0) pop7;
OPCDEF(v2d_min,  0x98, 1, 8, -4, 1, "v2d.min")	// sp(1) = sp(1) <? sp(0) pop4;
OPCDEF(v2d_max,  0x99, 1, 8, -4, 1, "v2d.max")	// sp(1) = sp(1) >? sp(0) pop4;
OPCDEF(p4d___a,  0x9a, 0, 0, -0, 1, NULL)		//
OPCDEF(p4d___b,  0x9b, 0, 0, -0, 1, "p2i.alu")	//
OPCDEF(p4d___c,  0x9c, 0, 0, -0, 1, "p4i.alu")	//
OPCDEF(p4d___d,  0x9d, 0, 0, -0, 1, "p8i.alu")	//
OPCDEF(p4d___e,  0x9e, 0, 0, -0, 1, "p16i.alu")	//
OPCDEF(p4d___f,  0x9f, 0, 0, -0, 1, NULL)		//-Extended ops: idx, rev, imm, mem
//~ rgb[ 32] ===================================================================a
//~ pi2[ 64] ===================================================================b
//~ pi4[ 64] ===================================================================c
//~ ??? ========================================================================d
//~ ??? ========================================================================e
//~ ext ========================================================================f

//~ abs, sat, nrm
//~ sin, cos, exp, log, lrp, pow

//~ bit, sxt, zxt, 
//~ adc, sbb, mul, div, mod, rot, 

/* opcodes
	bit: b32/?64
	num: i32/i64/f32/f64
	vec: v4f/v2d/...

	opc = code		// bit		num			vec

	neg = 0x?0,		// cmt		neg			neg
	add = 0x?1,		// and		add			add
	sub = 0x?2,		// or		sub			sub
	mul = 0x?3,		// xor		mul			mul
	div = 0x?4,		// shl		div			div
	mod = 0x?5,		// shr		rem			crs/<?>
	fun = 0x?6,		// sar		<*>			<*>

	ceq = 0x?7,		// <*>		ceq			ceq
	clt = 0x?8,		// ult		clt			min
	cge = 0x?9,		// ugt		cgt			max

	??? = 0x?a,		// umul		<?>			<?>
	cvt = 0x?b,		// udiv		2i32/bool	<?>
	cvt = 0x?c,		// umod		2f32/bool	dp3/<?>
	cvt = 0x?d,		// mad		2i64/bool	dph/<?>
	cvt = 0x?e,		// 2i64|b	2f64/bool	dp4/<?>
	cvt = 0x?f,		// <?>		ext			ext

	<*> means functions.
//~ */

/* opc_ext argc = 3: [OP:4][ind:2][res:6][lhs:6][rhs:6]
	switch (ind) {
		case 0: sp[res] = sp[lhs] OP sp[rhs];
		case 1:
			if (res != lhs && res != rhs)
				*sp[res] = sp[lhs] OP sp[rhs];
			else if (res != lhs)
				*sp[res] = sp[lhs] OP *sp[res];
			else if (res != rhs)
				*sp[res] = *sp[res] OP sp[rhs];
			else if (lhs != rhs)
				*sp[res] = *sp[res] OP sp[rhs];
			else
				*sp[res] = null;
		case 2:
			if (res == lhs && res == rhs)
			else if (rhs == lhs) [res] *= [lhs];
			else if (res == rhs) a op= b pop?
			else if (res == lhs) 
			else
		case 3: push(sp[lhs] OP sp[rhs]);
	}
	// chk = max(res, lhs, rhs);

	ext_set = 0,		// sp(res) = OP(sp(res), sp(0)); pop?
	ext_dup = 1,		// push(opr(sp(0), sp(res))); set?
	ext_sto = 2,		// sp[res] = OP(sp[res], sp(0)); pop?
	ext_lod = 3,		// push(opr(sp(0), sp[res])); set?

	ext_set = 0,		// 3 / sp(n) = opr(sp(n), sp(0)); pop(size & 1)				// a += b
	ext_dup = 1,		// 3 / sp(0) = opr(sp(0), sp(n)); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c
	ext_sto = 2,		// 6 / mp[n] = opr(mp[n], sp(0)); pop(size & 1)				// a += b
	ext_lod = 3,		// 6 / sp(0) = opr(sp(0), mp[n]); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c
//~ */

/* opc_p4i argc = 1: [sat:1][uns:1][typ:2][opc:4]
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
		mod = 0x05,	// avg

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
#undef OPCDEF
#endif
