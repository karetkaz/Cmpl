#ifdef TOKDEF
TOKDEF(TYPE_any, 0x00, 0, TY, "TYPE_any")	// first enum == 0/boolean
TOKDEF(TYPE_vid, 0x01, 0, TY, "TYPE_vid")		// void
TOKDEF(TYPE_bit, 0x02, 0, TY, "TYPE_bit")		//  bool, uns32, uns16, uns8
TOKDEF(TYPE_int, 0x03, 0, TY, "TYPE_int")		// int64, int32, int16, int8
TOKDEF(TYPE_flt, 0x04, 0, TY, "TYPE_flt")		// flt64, flt32
TOKDEF(TYPE_p4x, 0x05, 0, TY, "TYPE_vec")	// (p2f64, p4f32), (p2i64, p4i32, p8i16, p16i8), (p2u64, p4u32, p8u16, p16u8), ...
TOKDEF(TYPE_arr, 0x06, 0, TY, "TYPE_arr")	// string, array, variable|function
TOKDEF(TYPE_def, 0x07, 0, ID, "define")		// struct, union, constant|typename

TOKDEF(TYPE_u32, 0x02, 0, TY, "TYPE_u32")
TOKDEF(TYPE_i32, 0x03, 0, TY, "TYPE_i32")
TOKDEF(TYPE_i64, 0x03, 0, TY, "TYPE_i64")
TOKDEF(TYPE_f32, 0x04, 0, TY, "TYPE_f32")
TOKDEF(TYPE_f64, 0x04, 0, TY, "TYPE_f64")
//~ TOKDEF(TYPE_p4x, 0x05, 0, TY, "TYPE_p4x")
//~ TOKDEF(TYPE_p4x, 0x05, 0, TY, "TYPE_p4x")

//~ TOKDEF(TYPE_v4f, 0x05, 0, TY, "vec4f")
//~ TOKDEF(TYPE_v2d, 0x05, 0, TY, "vec2d")
//~ TOKDEF(TYPE_rgb, 0x02, 0, ID, "rgb32")

//~ TOKDEF(TYPE_arr, 0x06, 0, TY, "TYPE_arr(TYPE_ptr)")

TOKDEF(TYPE_enu, 0x07, 0, ID, "enum")
TOKDEF(TYPE_rec, 0x07, 0, ID, "struct")
//~ TOKDEF(TYPE_uni, 0x07, 0, ID, "union")
//~ TOKDEF(TYPE_cls, 0x07, 0, ID, "class")

TOKDEF(EMIT_opc, 0x00, 0, ID, "emit")

//~ TOKDEF(CNST_chr, 0x02, 0, TY, "CNST_uns")	// 'a'
//~ TOKDEF(CNST_uns, 0x02, 0, TY, "CNST_uns")	// uns32
TOKDEF(CNST_int, 0x03, 0, TY, "CNST_int")	// int32
TOKDEF(CNST_flt, 0x04, 0, TY, "CNST_flt")	// flt64
TOKDEF(CNST_str, 0x05, 0, TY, "CNST_str")	// string

TOKDEF(TYPE_ref, 0x05, 0, TY, "TYPE_ref")	// identifyer/variable/typename/constant
//~ TOKDEF(TYPE_new, 0x05, 0, TY, "TYPE_new")	// int a;

TOKDEF(OPER_nop, 0x00, 0, XX, ";")		// stmt: expr
TOKDEF(STMT_beg, 0x00, 0, XX, "{")		// stmt: list {...}
TOKDEF(STMT_for, 0x00, 0, ID, "for")		// stmt: for, while, repeat
TOKDEF(STMT_if,  0x00, 0, ID, "if")		// stmt: if then else | break | continue | goto
TOKDEF(STMT_els, 0x00, 0, ID, "else")		// ????
TOKDEF(STMT_end, 0x00, 0, XX, "}")		// ????

//~ Operators ==================================================================
TOKDEF(OPER_com, 0x01, 2, OP, ",")		// a, b

TOKDEF(ASGN_set, 0x12, 2, OP, ".set")		// a := b
TOKDEF(ASGN_mul, 0x12, 2, OP, "*=")		// a *= b
TOKDEF(ASGN_div, 0x12, 2, OP, "/=")		// a /= b
TOKDEF(ASGN_mod, 0x12, 2, OP, "%=")		// a %= b
TOKDEF(ASGN_add, 0x12, 2, OP, "+=")		// a += b
TOKDEF(ASGN_sub, 0x12, 2, OP, "-=")		// a -= b
TOKDEF(ASGN_shl, 0x12, 2, OP, "<<=")		// a <<= b
TOKDEF(ASGN_shr, 0x12, 2, OP, ">>=")		// a >>= b
TOKDEF(ASGN_and, 0x12, 2, OP, "&=")		// a &= b
TOKDEF(ASGN_xor, 0x12, 2, OP, "^=")		// a ^= b
TOKDEF(ASGN_ior, 0x12, 2, OP, "|=")		// a |= b

TOKDEF(OPER_sel, 0x13, 3, OP, "?:")		// a ? b : c
TOKDEF(OPER_lor, 0x04, 2, OP, "||")		// a || b
TOKDEF(OPER_lnd, 0x05, 2, OP, "&&")		// a && b

TOKDEF(OPER_ior, 0x06, 2, OP, ".ior")		// a | b
TOKDEF(OPER_xor, 0x07, 2, OP, ".xor")		// a ^ b
TOKDEF(OPER_and, 0x08, 2, OP, ".and")		// a & b

TOKDEF(OPER_equ, 0x09, 2, OP, ".ceq")		// a == b
TOKDEF(OPER_neq, 0x09, 2, OP, ".cne")		// a != b
TOKDEF(OPER_gte, 0x0a, 2, OP, ".cgt")		// a > b
TOKDEF(OPER_geq, 0x0a, 2, OP, ".cge")		// a >= b
TOKDEF(OPER_lte, 0x0a, 2, OP, ".clt")		// a < b
TOKDEF(OPER_leq, 0x0a, 2, OP, ".cle")		// a <= b

TOKDEF(OPER_shr, 0x0b, 2, OP, ".shr")		// a >> b
TOKDEF(OPER_shl, 0x0b, 2, OP, ".shl")		// a << b

TOKDEF(OPER_add, 0x0c, 2, OP, ".add")		// a+b
TOKDEF(OPER_sub, 0x0c, 2, OP, ".sub")		// a-b

TOKDEF(OPER_mul, 0x0d, 2, OP, ".mul")		// a*b
TOKDEF(OPER_div, 0x0d, 2, OP, ".div")		// a/b
TOKDEF(OPER_mod, 0x0d, 2, OP, ".mod")		// a%b

TOKDEF(OPER_pls, 0x1e, 1, OP, ".pls")		// +a		unary plus
TOKDEF(OPER_mns, 0x1e, 1, OP, ".neg")		// -a		unary minus
TOKDEF(OPER_cmt, 0x1e, 1, OP, ".cmt")		// ~a		complement
TOKDEF(OPER_not, 0x1e, 1, OP, ".not")		// !a		logical not

TOKDEF(OPER_idx, 0x0f, 2, OP, ".idx")		// a[n]		index / pointer to [a] <=> *a
TOKDEF(OPER_fnc, 0x0f, 2, OP, ".fnc")		// a(x)		function call, sizeof, cast/ctor.
TOKDEF(OPER_dot, 0x0f, 2, OP, ".dot")		// a.b		member

//~ temp =======================================================================
TOKDEF(PNCT_lc , 0x00, 0, XX, "[")		// curlies
TOKDEF(PNCT_rc , 0x00, 0, XX, "]")
TOKDEF(PNCT_lp , 0x00, 0, XX, "PNCT_lp")	// parentheses
TOKDEF(PNCT_rp , 0x00, 0, XX, "PNCT_rp")
TOKDEF(PNCT_qst, 0x00, 0, XX, "?")		// question mark
TOKDEF(PNCT_cln, 0x00, 0, XX, ":")		// colon

TOKDEF(TOKN_err, 0xff, 0, XX, "TOKN_err")

/*/  #define TOKDEF(NAME, PREC, ARGC, KIND, STR) ================================

//~ TOKDEF(QUAL_con, 0x10,  0, ID, "const")	// constant
//~ TOKDEF(QUAL_sta, 0x20,  0, ID, "static")	// static
//~ TOKDEF(QUAL_sta, 0x40,  0, ID, "parallel")	// parallel
//~ TOKDEF(QUAL_vol, 0x80,  0, ID, "volatile")	// volatile
//~ final = constant + volatile,		// final

//~ Operators ==================================================================
TOKDEF(OPER_min, 0x09, 2, OP, "<?")		// a <? b
TOKDEF(OPER_max, 0x09, 2, OP, ">?")		// a >? b
TOKDEF(OPER_pow, 0x0a, 2, OP, ">")		// a ** b
TOKDEF(ASGN_min, 0x09, 2, OP, "<?")		// a <?= b
TOKDEF(ASGN_max, 0x09, 2, OP, ">?")		// a >?= b
TOKDEF(ASGN_pow, 0x0a, 2, OP, ">")		// a **= b

//{ ============================================================================
//~ TOKDEF(OPER_sta, 0x20,  0, 3, "static")	// static if, ...
//~ TOKDEF(OPER_syn, 0x01,  0, 3, "sync")	// `sync`(ref) {...} | `sync` ( `task` ) {...} | `sync` {...}
//~ TOKDEF(OPER_tsk, 0x02,  0, 3, "task")	// `task`(id) {...} | `task` {...}
//~ TOKDEF(OPER_frk, 0x03,  0, 3, "fork")	// `task`(0)

//~ TOKDEF(OPER_new, 0x00, 2, 2, "new")		// stmt: declare
//~ TOKDEF(OPER_del, 0x00, 0, 2, "delete")		//
//~ TOKDEF(OPER_ret, 0x00, 0, 2, "return")
//? TOKDEF(OPER_try, 0x00, 0, 2, "try")		//
//? TOKDEF(OPER_thr, 0x00, 0, 2, "throw")		//
//? TOKDEF(OPER_cth, 0x00, 0, 2, "catch")		//
//? TOKDEF(OPER_fin, 0x00, 0, 2, "finally")		//
//~ TOKDEF(OPER_cas, 0x00, 0, 2, "case")
//~ TOKDEF(OPER_go2, 0x00, 0, 2, "goto")
//~ TOKDEF(OPER_brk, 0x00, 0, 2, "break")
//~ TOKDEF(OPER_swi, 0x00, 0, 2, "switch")
//? TOKDEF(OPER_def, 0x00, 0, 2, "default")
//~ TOKDEF(OPER_con, 0x00, 0, 2, "continue")
//~ TOKDEF(OPER_wht, 0x00, 0, 2, "while")		// while(...) {...}
//~ TOKDEF(OPER_whf, 0x00, 0, 2, "until")		// until(...) {...}
//~ TOKDEF(OPER_rep, 0x00, 0, 2, "repeat")		// repeat {...} ((until(...))|(while(...)))?
//~ TOKDEF(OPER_wth, 0x00, 0, 2, "with")

//} */

#undef TOKDEF
#endif

#ifdef OPCDEF
// #define 
/* #define OPCDEF(Name, Code, Size, Diff, Chck, Time, Mnem) Name = Code
 * Name: enum name;
 * Code: value of enum;
 * Size: opcode length; (-1 variable) (0 ???);
 * Diff: pops n stack elements;

 * Schk: requires n stack elements;
 * Tick: ticks for executing instruction;
 */
//~ sys ========================================================================
OPCDEF(opc_nop,  0x00, 1, 0, +0, 1,	"nop")		// ;					[�, a, b, c => [�, a, b, c;
OPCDEF(opc_loc,  0x01, 2, 0, +9, 1,	"loc")		// sp += 4*arg.1;		[�, a, b, c => [�, a, b, c, �;
OPCDEF(opc_pop,  0x02, 2, 9, +9, 1,	"pop")		// sp -= 4*arg.1;		[�, a, b, c => [�, a, b;
OPCDEF(opc___3,  0x03, 0, 0, +0, 1,	NULL)		// 						[�, a, b, c => [�, a, b, c, a;
OPCDEF(opc___4,  0x04, 0, 0, +0, 1,	NULL)		// 						[�, a, b, c => [�, c, b;
OPCDEF(opc___5,  0x05, 0, 0, +0, 1,	"ldsp")		// push(SP + arg.3); SP += 4;	[�, a, b, c => [�, a, b, c, SP;
OPCDEF(opc_call, 0x06, 4, 0, +1, 1,	"call")		// push(IP); ip += arg.3;	[�, a, b, c => [�, a, b, c, IP;
OPCDEF(opc_jmpi, 0x07, 1, 1, -1, 1,	"jmpi")		// IP = pop();				[�, a, b, c => [�, a, b;
OPCDEF(opc_jmp,  0x08, 4, 0, +0, 1,	"jmp")		// IP += arg.3;			[�, a, b, c => [�, a, b, c;
OPCDEF(opc_jnz,  0x09, 4, 1, -1, 1,	"jnz")		// if pop(0)!=0 IP += arg.3;	[�, a, b, c => [�, a, b;
OPCDEF(opc_jz,   0x0a, 4, 1, -1, 1,	"jz")		// if pop(0)==0 IP += arg.3;	[�, a, b, c => [�, a, b;
OPCDEF(opc___0b, 0x0b, 0, 0,  0, 1,	NULL)		// if pop(0) <0 IP += arg.3;	[�, a, b, c => [�, a, b;
OPCDEF(opc___0c, 0x0c, 0, 0, +0, 1,	NULL)		// 
OPCDEF(opc_libc, 0x0d, 2, 0, +0, 1,	"libc")		//-lib call
OPCDEF(opc_task, 0x0e, 4, 0, +0, 1,	"task")		// arg.3: [code:16][data:8] task, [?fork if (arg.code == 0)]
OPCDEF(opc_sysc, 0x0f, 2, 0, +0, 1,	"sysc")		// arg.1: [type:8](exit, halt, wait?join?sync, trap) (alloc, free, copy, init) ...
//~ stk ========================================================================
OPCDEF(opc_ldc1, 0x10, 2, 0, +1, 1,	"ldc.b08")	// push(arg.1);			:2[�, a, b, c => [�, a, b, c, 2;
OPCDEF(opc_ldc2, 0x11, 3, 0, +1, 1,	"ldc.b16")	// push(arg.2);			:2[�, a, b, c => [�, a, b, c, 2;
OPCDEF(opc_ldc4, 0x12, 5, 0, +1, 1,	"ldc.b32")	// push(arg.4);			:2[�, a, b, c => [�, a, b, c, 2;
OPCDEF(opc_ldc8, 0x13, 9, 0, +2, 1,	"ldc.b64")	// push(arg.8);			:2[�, a, b, c => [�, a, b, c, 2;
OPCDEF(opc_dup1, 0x14, 2, 9, +1, 1,	"dup.x1")	// push(sp(n));			:2[�, a, b, c => [�, a, b, c, a;
OPCDEF(opc_dup2, 0x15, 2, 9, +2, 1,	"dup.x2")	// push(sp(n))x2;		:2[�, a, b, c => [�, a, b, c, a, b;
OPCDEF(opc_dup4, 0x16, 2, 9, +4, 1,	"dup.x4")	// push(sp(n))x4;		:3[�, a, b, c, d => [�, a, b, c, d, a, b, c, d;
OPCDEF(opc_set1, 0x17, 2, 9, -1, 1,	"set.x1")	// 
OPCDEF(opc_set2, 0x18, 2, 9, -2, 1,	"set.x2")	// 
OPCDEF(opc_set4, 0x19, 2, 9, -4, 1,	"set.x4")	// 
OPCDEF(opc_ldz1, 0x1a, 1, 0, +1, 1,	"ldz.x1")	// push(0);			[�, a, b, c => [�, a, b, c, 0;
OPCDEF(opc_ldz2, 0x1b, 1, 0, +2, 1,	"ldz.x2")	// push(0, 0);			[�, a, b, c => [�, a, b, c, 0, 0;
OPCDEF(opc_ldz4, 0x1c, 1, 0, +4, 1,	"ldz.x4")	// push(0, 0, 0, 0);		[�, a, b, c => [�, a, b, c, 0, 0, 0, 0;
OPCDEF(opc_ldcr, 0x1d, 5, 0, +1, 1,	"ldc.ref")	// TEMP/dup.4.swz
OPCDEF(opc_ldcf, 0x1e, 5, 0, +1, 1,	"ldc.f32")	// TEMP/set.4.msk
OPCDEF(opc_ldcF, 0x1f, 9, 0, +2, 1,	"ldc.f64")	// TEMP/?
//~ mem ========================================================================
OPCDEF(opc_ldi1, 0x20, 1, 0,  0, 1,	"ldi.b")	// copy(sp, sp(0) 1);		[�, a, b, c => [�, a, b, *c;
OPCDEF(opc_ldi2, 0x21, 1, 0,  0, 1,	"ldi.h")	// copy(sp, sp(0) 2);		[�, a, b, c => [�, a, b, *c;
OPCDEF(opc_ldi4, 0x22, 1, 0,  0, 1,	"ldi.w")	// copy(sp, sp(0) 4);		[�, a, b, c => [�, a, b, *c;
OPCDEF(opc_ldi8, 0x23, 1, 0,  0, 1,	"ldi.d")	// copy(sp, sp(0) 8);		[�, a, b, c => [�, a, b, *c:1, *c:2;
OPCDEF(opc_ldiq, 0x24, 1, 0,  0, 1,	"ldi.q")	// copy(sp, sp(0) 16);		[�, a, b, c => [�, a, b, *c;
OPCDEF(opc_sti1, 0x25, 1, 0, +0, 1,	"sti.b")	// copy(sp(1) sp(0) 1);pop2;	[�, a, b, c => [�, a
OPCDEF(opc_sti2, 0x26, 1, 0, +0, 1,	"sti.h")	// copy(sp(1) sp(0) 2);pop2;	[�, a, b, c => [�, a
OPCDEF(opc_sti4, 0x27, 1, 0, +0, 1,	"sti.w")	// copy(sp(1) sp(0) 4);pop2;	[�, a, b, c => [�, a
OPCDEF(opc_sti8, 0x28, 1, 0, +0, 1,	"sti.d")	// copy(sp(1) sp(0) 8);pop3;	[�, a, b, c => [�, a
OPCDEF(opc_stiq, 0x29, 1, 0, +0, 1,	"sti.q")	// copy(sp(1) sp(0) 16);pop5;	[�, a, b, c => [�, a
OPCDEF(opc___a,  0x2a, 5, 0,  0, 1,	"ldm.w")	// (32)
OPCDEF(opc___b,  0x2b, 5, 0,  0, 1,	"ldm.d")	// (64)
OPCDEF(opc___c,  0x2c, 5, 0,  0, 1,	"ldm.q")	// (128)
OPCDEF(opc___d,  0x2d, 5, 0,  0, 1,	"stm.w")	// 
OPCDEF(opc___e,  0x2e, 5, 0,  0, 1,	"stm.d")	// 
OPCDEF(opc___f,  0x2f, 5, 0,  0, 1,	"stm.q")	// 
//~ bit[ 32] ===================================================================
OPCDEF(b32_cmt,  0x30, 1, 1, -0, 1,	"b32.cmt")	// sp(0) = ~sp(0);
OPCDEF(b32___1,  0x31, 0, 2, -1, 1,	"b32.adc")	// sp(1) += sp(0); pop;
OPCDEF(b32___2,  0x32, 0, 2, -1, 1,	"b32.sbb")	// sp(1) -= sp(0); pop;
OPCDEF(u32_mul,  0x33, 1, 2, -1, 1,	"u32.mul")	// sp(1) *= sp(0); pop;
OPCDEF(u32_div,  0x34, 1, 2, -1, 1,	"u32.div")	// sp(1) /= sp(0); pop;
OPCDEF(u32_mad,  0x35, 1, 3, -2, 1,	"u32.mad")		// mul; add;
OPCDEF(b32___6,  0x36, 2, 1, -0, 1,	"bit.")		// sp(0) = {any, all, ...}[opc.arg.i1](sp(0));
OPCDEF(b32_not,  0x37, 1, 1, -0, 1,	"b32.not")	// sp(0) = !sp(0);
OPCDEF(u32_clt,  0x38, 1, 2, -1, 1,	"u32.clt")	// sp(1).b32 = sp(1).u32 < sp(0).u32; pop;
OPCDEF(u32_cgt,  0x39, 1, 2, -1, 1,	"u32.cgt")	// sp(1).b32 = sp(1).u32 > sp(0).u32; pop;
OPCDEF(b32_and,  0x3a, 1, 2, -1, 1,	"b32.and")	// sp(1).u32 &= sp(0).u32; pop;
OPCDEF(b32_ior,  0x3b, 1, 2, -1, 1,	"b32.ior")	// sp(1).u32 |= sp(0).u32; pop;
OPCDEF(b32_xor,  0x3c, 1, 2, -1, 1,	"b32.xor")	// sp(1).u32 ^= sp(0).u32; pop;
OPCDEF(b32_shl,  0x3d, 1, 2, -1, 1,	"b32.shl")	// sp(1).u32 <<= sp(0).u32; pop;
OPCDEF(b32_shr,  0x3e, 1, 2, -1, 1,	"b32.shr")	// sp(1).u32 >>= sp(0).u32; pop;
OPCDEF(b32_sar,  0x3f, 1, 2, -1, 1,	"b32.sar")	// sp(1).i32 >>= sp(0).i32; pop;
//~ OPCDEF(opc_rot,  0x3x, 1, 2, -1, 1,	"rot")	 	//?rotate
//~ OPCDEF(opc_zxt,  0x3x, 1, 2, -0, 1,	"zxt")		//!zero.extend arg:[offs:3][size:5]
//~ OPCDEF(opc_sxt,  0x3x, 1, 2, -0, 1,	"sxt")		//!sign.extend arg:[offs:3][size:5]
//~ OPCDEF(u32_mad,  0x3x, 1, 3, -2, 1,	"u32.mad")	// sp(2) += sp(1)*sp(0); pop2;	[�, a, b, c => [�, a + b * c;
//~ i32[ 32] ===================================================================
OPCDEF(i32_neg,  0x40, 1, 1, -0, 1,	"i32.neg")	// sp(0).i32 = -sp(0).i32;
OPCDEF(i32_add,  0x41, 1, 2, -1, 1,	"i32.add")	// sp(1).i32 += sp(0).i32; pop;
OPCDEF(i32_sub,  0x42, 1, 2, -1, 1,	"i32.sub")	// sp(1).i32 -= sp(0).i32; pop;
OPCDEF(i32_mul,  0x43, 1, 2, -1, 1,	"i32.mul")	// sp(1).i32 *= sp(0).i32; pop;
OPCDEF(i32_div,  0x44, 1, 2, -1, 1,	"i32.div")	// sp(1).i32 /= sp(0).i32; pop;
OPCDEF(i32_mod,  0x45, 1, 2, -1, 1,	"i32.mod")	// sp(1).i32 %= sp(0).i32; pop;
OPCDEF(i32___6,  0x46, 0, 0, -0, 1,	NULL)		// sp(0).i32 = {...} (sp(0).i32);
OPCDEF(i32_ceq,  0x47, 1, 2, -1, 1,	"i32.ceq")	// sp(1).b32 = sp(1).i32 == sp(0).i32; pop;
OPCDEF(i32_clt,  0x48, 1, 2, -1, 1,	"i32.clt")	// sp(1).b32 = sp(1).i32  < sp(0).i32; pop;
OPCDEF(i32_cgt,  0x49, 1, 2, -1, 1,	"i32.cgt")	// sp(1).b32 = sp(1).i32  > sp(0).i32; pop;
OPCDEF(i32___a,  0x4a, 0, 0, -0, 1,	NULL)	// sar
OPCDEF(i32_bol,  0x4b, 1, 1, -0, 1,	"i32.cvt2_bool")	// push(pop.i32 != 0)
OPCDEF(i32_f32,  0x4c, 1, 1, -0, 1,	"i32.cvt2_f32")	// push(f32(pop.i32))
OPCDEF(i32_i64,  0x4d, 1, 1, +1, 1,	"i32.cvt2_i64")	// push(i64(pop.i32))
OPCDEF(i32_f64,  0x4e, 1, 1, +1, 1,	"i32.cvt2_f64")	// push(f64(pop.i32))
OPCDEF(i32___f,  0x4f, 0, 0, -0, 1,	NULL)		//?Extended ops: idx,rev,imm,mem
//~ f32[ 32] ===================================================================
OPCDEF(f32_neg,  0x50, 1, 1, -0, 1,	"f32.neg")	// sp(0) = -sp(0);
OPCDEF(f32_add,  0x51, 1, 2, -1, 1,	"f32.add")	// sp(1) += sp(0); pop;
OPCDEF(f32_sub,  0x52, 1, 2, -1, 1,	"f32.sub")	// sp(1) -= sp(0); pop;
OPCDEF(f32_mul,  0x53, 1, 2, -1, 1,	"f32.mul")	// sp(1) *= sp(0); pop;
OPCDEF(f32_div,  0x54, 1, 2, -1, 1,	"f32.div")	// sp(1) /= sp(0); pop;
OPCDEF(f32_mod,  0x55, 1, 2, -1, 1,	"f32.mod")	// sp(1) %= sp(0); pop;
OPCDEF(f32___6,  0x56, 0, 0, -0, 1,	NULL)	// sp(0) = {...} (sp(0));
OPCDEF(f32_ceq,  0x57, 1, 2, -1, 1,	"f32.ceq")	// sp(1).b32 = sp(1).f32 == sp(0).f32; pop;
OPCDEF(f32_clt,  0x58, 1, 2, -1, 1,	"f32.clt")	// sp(1).b32 = sp(1).f32  < sp(0).f32; pop;
OPCDEF(f32_cgt,  0x59, 1, 2, -1, 1,	"f32.cgt")	// sp(1).b32 = sp(1).f32  > sp(0).f32; pop;
OPCDEF(f32___a,  0x5a, 0, 0, -0, 1,	NULL)	// abs
OPCDEF(f32_i32,  0x5b, 1, 1, -0, 1,	"f32.cvt2_i32")	// push(i32(pop.f32))
OPCDEF(f32_bol,  0x5c, 1, 1, -0, 1,	"f32.cvt2_bool")	// push(pop.f32 != 0)
OPCDEF(f32_i64,  0x5d, 1, 1, +1, 1,	"f32.cvt2_i64")	// push(i64(pop.f32))
OPCDEF(f32_f64,  0x5e, 1, 1, +1, 1,	"f32.cvt2_f64")	// push(f64(pop.f32))
OPCDEF(f32___f,  0x5f, 0, 0, -0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ i64[ 64] ===================================================================
OPCDEF(i64_neg,  0x60, 1, 2, -0, 1,	"i64.neg")	// sp(0) = -sp(0);
OPCDEF(i64_add,  0x61, 1, 4, -2, 1,	"i64.add")	// sp(2) += sp(0); pop2;
OPCDEF(i64_sub,  0x62, 1, 4, -2, 1,	"i64.sub")	// sp(2) -= sp(0); pop2;
OPCDEF(i64_mul,  0x63, 1, 4, -2, 1,	"i64.mul")	// sp(2) *= sp(0); pop2;
OPCDEF(i64_div,  0x64, 1, 4, -2, 1,	"i64.div")	// sp(2) /= sp(0); pop2;
OPCDEF(i64_mod,  0x65, 1, 4, -2, 1,	"i64.mod")	// sp(2) %= sp(0); pop2;
OPCDEF(i64___6,  0x66, 0, 0, -0, 1,	NULL)	// sp(0) = {...} (sp(0));
OPCDEF(i64_ceq,  0x67, 1, 4, -3, 1,	"i64.ceq")	// sp(2).b32 = sp(2).i64 == sp(0).i64; pop2;
OPCDEF(i64_clt,  0x68, 1, 4, -3, 1,	"i64.clt")	// sp(2).b32 = sp(2).i64  < sp(0).i64; pop2;
OPCDEF(i64_cgt,  0x69, 1, 4, -3, 1,	"i64.cgt")	// sp(2).b32 = sp(2).i64  > sp(0).i64; pop2;
OPCDEF(i64___a,  0x6a, 0, 0, -0, 1,	NULL)	// abs
OPCDEF(i64_i32,  0x6b, 1, 2, -1, 1,	"i64.cvt2_i32")	// push(i32(pop.i64))
OPCDEF(i64_f32,  0x6c, 1, 2, -1, 1,	"i64.cvt2_f32")	// push(f32(pop.i64))
OPCDEF(i64_bol,  0x6d, 1, 2, -1, 1,	"i64.cvt2_bool")	// push(pop.i64 != 0)
OPCDEF(i64_f64,  0x6e, 1, 2, -0, 1,	"i64.cvt2_f64")	// push(f64(pop.i64))
OPCDEF(i64___f,  0x6f, 0, 0, -0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ f64[ 64] ===================================================================
OPCDEF(f64_neg,  0x70, 1, 2, -0, 1,	"f64.neg")	// sp(0) = -sp(0);
OPCDEF(f64_add,  0x71, 1, 4, -2, 1,	"f64.add")	// sp(2) += sp(0); pop2;
OPCDEF(f64_sub,  0x72, 1, 4, -2, 1,	"f64.sub")	// sp(2) -= sp(0); pop2;
OPCDEF(f64_mul,  0x73, 1, 4, -2, 1,	"f64.mul")	// sp(2) *= sp(0); pop2;
OPCDEF(f64_div,  0x74, 1, 4, -2, 1,	"f64.div")	// sp(2) /= sp(0); pop2;
OPCDEF(f64_mod,  0x75, 1, 4, -2, 1,	"f64.mod")	// sp(2) %= sp(0); pop2;
OPCDEF(f64___6,  0x76, 0, 0, -0, 1,	NULL)	// sp(0) = {min, max, ...} (sp(0));
OPCDEF(f64_ceq,  0x77, 1, 4, -3, 1,	"f64.ceq")	// sp(2).b32 = sp(2).f64 == sp(0).f64; pop2;
OPCDEF(f64_clt,  0x78, 1, 4, -3, 1,	"f64.clt")	// sp(2).b32 = sp(2).f64  < sp(0).f64; pop2;
OPCDEF(f64_cgt,  0x79, 1, 4, -3, 1,	"f64.cgt")	// sp(2).b32 = sp(2).f64  > sp(0).f64; pop2;
OPCDEF(f64___a,  0x7a, 0, 0, -0, 1,	NULL)	//
OPCDEF(f64_i32,  0x7b, 1, 2, -1, 1,	"f64.cvt2_i32")	// push(i32(pop.f64))
OPCDEF(f64_f32,  0x7c, 1, 2, -1, 1,	"f64.cvt2_f32")	// push(f32(pop.f64))
OPCDEF(f64_i64,  0x7d, 1, 2, -0, 1,	"f64.cvt2_i64")	// push(i64(pop.f64))
OPCDEF(f64_bol,  0x7e, 1, 2, -1, 1,	"f64.cvt2_bool")	// push(pop.f64 != 0)
OPCDEF(f64___f,  0x7f, 0, 0, -0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ v4f[128] ===================================================================8
OPCDEF(v4f_neg,  0x80, 1, 4, -0, 1,	"v4f.neg")	// sp(0) = -sp(0);
OPCDEF(v4f_add,  0x81, 1, 8, -4, 1,	"v4f.add")	// sp(4) += sp(0); pop4;
OPCDEF(v4f_sub,  0x82, 1, 8, -4, 1,	"v4f.sub")	// sp(4) -= sp(0); pop4;
OPCDEF(v4f_mul,  0x83, 1, 8, -4, 1,	"v4f.mul")	// sp(4) *= sp(0); pop4;
OPCDEF(v4f_div,  0x84, 1, 8, -4, 1,	"v4f.div")	// sp(4) /= sp(0); pop4;
OPCDEF(v4f_crs,  0x85, 1, 8, -4, 1,	"v4f.crs")	// cross
OPCDEF(v4f___6,  0x86, 0, 0, -0, 1,	NULL)	//
OPCDEF(v4f_cmp,  0x87, 1, 8, -7, 1,	"v4f.ceq")	// sp(4).b32 = sp(4) == sp(0) pop7;
OPCDEF(v4f_min,  0x88, 1, 8, -4, 1,	"v4f.min")	// sp(4).b32 = sp(4) <? sp(0) pop4;
OPCDEF(v4f_max,  0x89, 1, 8, -4, 1,	"v4f.max")	// sp(4).b32 = sp(4) >? sp(0) pop4;
OPCDEF(v4f___a,  0x8a, 0, 0, -0, 1,	NULL)		// pov, lrp, ..., abs, log, cos, ...
OPCDEF(v4f___b,  0x8b, 0, 0, -0, 1,	NULL)	//
OPCDEF(v4f_dp3,  0x8c, 1, 8, -7, 1,	"v4f.dp3")	//
OPCDEF(v4f_dp4,  0x8d, 1, 8, -7, 1,	"v4f.dp4")	//
OPCDEF(v4f_dph,  0x8e, 1, 8, -7, 1,	"v4f.dph")	//
OPCDEF(v4f___f,  0x8f, 0, 0, -0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ p4d[128] ===================================================================9
OPCDEF(v2d_neg,  0x90, 1, 4, -0, 1,	"pf2.neg")	// sp(0) = -sp(0);
OPCDEF(v2d_add,  0x91, 1, 8, -4, 1,	"pf2.add")	// sp(4) += sp(0); pop4;
OPCDEF(v2d_sub,  0x92, 1, 8, -4, 1,	"pf2.sub")	// sp(4) -= sp(0); pop4;
OPCDEF(v2d_mul,  0x93, 1, 8, -4, 1,	"pf2.mul")	// sp(4) *= sp(0); pop4;
OPCDEF(v2d_div,  0x94, 1, 8, -4, 1,	"pf2.div")	// sp(4) /= sp(0); pop4;
OPCDEF(p4d___5,  0x95, 0, 0, -0, 1,	NULL)		// mod
OPCDEF(p4d_swz,  0x96, 2, 4, -0, 1,	"px4.swz")	// swizzle
OPCDEF(v2d_ceq,  0x97, 1, 8, -7, 1,	"pf2.ceq")	// sp(1) = sp(1) == sp(0) pop7;
OPCDEF(v2d_min,  0x98, 1, 8, -4, 1,	"pf2.min")	// sp(1) = sp(1) <? sp(0) pop4;
OPCDEF(v2d_max,  0x99, 1, 8, -4, 1,	"pf2.max")	// sp(1) = sp(1) >? sp(0) pop4;
OPCDEF(p4d___a,  0x9a, 0, 0, -0, 1,	NULL)	//
OPCDEF(p4d___b,  0x9b, 0, 0, -0, 1,	"p2i.alu")	//
OPCDEF(p4d___c,  0x9c, 0, 0, -0, 1,	"p4i.alu")	//
OPCDEF(p4d___d,  0x9d, 0, 0, -0, 1,	"p8i.alu")	//
OPCDEF(p4d___e,  0x9e, 0, 0, -0, 1,	"p16.alu")	//
OPCDEF(p4d___f,  0x9f, 0, 0, -0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ rgb[ 32] ===================================================================a
//~ OPCDEF(rgb_neg,  0xa0, 0, 	"rgb.neg")
//~ OPCDEF(rgb_add,  0xa1, 0, 	"rgb.add")
//~ OPCDEF(rgb_sub,  0xa2, 0, 	"rgb.sub")
//~ OPCDEF(rgb_mul,  0xa3, 0, 	"rgb.mul")
//~ OPCDEF(rgb_div,  0xa4, 0, 	"rgb.div")
//~ OPCDEF(rgb___5,  0xa5, 0, 	NULL)		// ???
//~ OPCDEF(rgb___6,  0xa6, 0, 	NULL)		// ???
//~ OPCDEF(rgb___7,  0xa7, 0, 	"")
//~ OPCDEF(rgb_min,  0xa8, 0, 	"rgb.min")
//~ OPCDEF(rgb_max,  0xa9, 0, 	"rgb.max")
//~ OPCDEF(rgb_lum,  0xaa,-1, 	NULL)		//? r*RF + g*GF + b*BF
//~ OPCDEF(rgb_avg,  0xab,-1, 	NULL)		//? (r + g + b)/3
//~ OPCDEF(rgb_mix,  0xac, 0, 	NULL)
//~ OPCDEF(rgb_fv4,  0xad,-1, 	NULL)		// from v4
//~ OPCDEF(rgb_tv4,  0xae, 0, 	NULL)
//~ OPCDEF(rgb_mad,  0xaf,-1, 	NULL) 		// a += b * c

//~ pi2[ 64] ===================================================================b
//~ pi4[ 64] ===================================================================c
//~ ??? ========================================================================d
//~ ??? ========================================================================e
//~ ??? ========================================================================f

//~ abs
//~ sat
//~ nrm

//~ sin
//? cos
//~ exp
//~ log

//~ lrp
//~ pow


//~ bit, sxt, zxt, 
//~ adc, sbb, mul, div, mod, rot, 

/* opcode table
	bin: b32
	num: i32/i64/f32/f64
	vec: .../p4i/p2l/p4f/p2d
	opc = code		// bin		num			vec

	neg = 0x?0,		// cmt		neg			neg
	add = 0x?1,		// and		add			add
	sub = 0x?2,		// ior 		sub			sub
	mul = 0x?3,		// xor 		mul			mul
	div = 0x?4,		// shl 		div			div
	mod = 0x?5,		// shr		rem			crs
	fun = 0x?6,		// sar		abs, sin, pow, ...

	ceq = 0x?7,		// not		ceq			ceq
	clt = 0x?8,		// ult		clt			min
	cge = 0x?9,		// ugt		cgt			max

	??? = 0x?a,		// mad		 ? 			 ? 
	cvt = 0x?b,		// ?		2i32|bool	dp2
	cvt = 0x?c,		// ?		2f32|bool	dp3
	cvt = 0x?d,		// ?		2i64|bool	dph
	cvt = 0x?e,		// ?		2f64|bool	dp4
	cvt = 0x?f,		// ?		ext			ext
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
	// chck = max(res, lhs, rhs);

	ext_set = 0,		// sp(res) = OP(sp(res), sp(0)); pop?
	ext_dup = 1,		// push(opr(sp(0), sp(res))); set?
	ext_sto = 2,		// sp[res] = OP(sp[res], sp(0)); pop?
	ext_lod = 3,		// push(opr(sp(0), sp[res])); set?

	ext_set = 0,		// 3 / sp(n) = opr(sp(n), sp(0)); pop(size & 1)				// a += b
	ext_dup = 1,		// 3 / sp(0) = opr(sp(0), sp(n)); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c
	ext_sto = 2,		// 6 / mp[n] = opr(mp[n], sp(0)); pop(size & 1)				// a += b
	ext_lod = 3,		// 6 / sp(0) = opr(sp(0), mp[n]); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c

//~ */
/* opc_bit argc = 1: [extop:3][cnt:5]		// bit ops
	//~ opx == 000: test/scan 		??
		-- as flags after cmp --------------
		//~ cnt== 00: nop
		//~ cnt== 01: seq		// zero 		: ==
		//~ cnt== 02: sne		// !zero		: !=
		//~ cnt== 03: ???		// sign 		: <
		//~ cnt== 04: ???		// (sign|zero)	: <=
		//~ cnt== 05: ???		// !sign 		: >
		//~ cnt== 06: ???		// !(sign|zero)	: >=
		//~ cnt== 07: ???		// ovf  		: 
		//~ cnt== 08: ???		// !ovf 		: 
		//~ cnt== 09: ???		// 
		//~ cnt== 0a: ssat		// saturate (signed)
		//~ cnt== 0b: usat		// saturate (unsigned)
		-- as flags after cmp --------------
		//~ cnt== 0c: any		// any bit set ( != 0) ? 0 : 1
		//~ cnt== 0d: all		// all bits set ( = -1)? 0 : 1
		//~ cnt== 0e: sgn		// sign bit set ( < 0) ? 0 : 1
		//~ cnt== 0f: par		// parity bit set ? 0 : 1
		------------------------------------
		//~ cnt== 10: shl		// 1
		//~ cnt== 11: shr		// 1
		//~ cnt== 12: sar		// 1
		//~ cnt== 13: rol		// 1
		//~ cnt== 14: ror		// 1
		//~ cnt== 15: cnt		// count bits
		//~ cnt== 16: hi1		// keep highest bit / 1 << bsf
		//~ cnt== 17: bsf		// scan forward / lg2(hi1)
		//~ cnt== 18: lo1		// keep lowest bit / 1 << bsr
		//~ cnt== 19: bsr		// scan reverse / lg2(lo1)
		//~ cnt== 1a: swp		// swap bits
		//~ xxx== 20: 

	//~ opx == 001: shl		// shift left
	//~ opx == 010: shr		// shift right
	//~ opx == 011: sar		// shift arithm right

	//~ opx == 100: get		// get bit
	//~ opx == 110: set		// set bit
	//~ opx == 101: clr		// clear
	//~ opx == 111: cmt		// complement
//~ */
/* opc_ixt argc = 1: [offs:3][size:5]		// extend (zero/sign)
opc_zxt argc = 1: [offs:3][size:5]
		((unsigned)val << (32 - (offs + size))) >> (32 - size);

opc_sxt argc = 1: [offs:3][size:5]
		((signed)val << (32 - (offs + size))) >> (32 - size);
//~ */
/* opc_cmp argc = 1: [sat:2][cmp:4][typ:2]	// compare
	bit:[0-1, 2: compare <ctz, ceq, clt, cle>, (not result)]
		?0	nz	a = 0
		1	eq	a = b
		2	lt	a < b
		3	gt	a > b

	bit:[3-4, 5: type: <i32, f32, i64, f64>, (unordered / unsigned)]

	arg:[6-7: action: <nop, set, sel, jmp>]
		0	nop: leave the flags
		1	set: true or false
		2	sel: min/max
		3	jmp: goto
*/
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
