#ifdef TOKDEF
TOKDEF(TYPE_any, 0x00, 0, TY, "TYPE_any")	// first enum == 0/boolean
TOKDEF(TYPE_vid, 0x01, 0, TY, "TYPE_vid")	// void
//~ TOKDEF(TYPE_bit, 0x02, 0, TY, "TYPE_bit")	// boolean, bitfield, ?
//~ TOKDEF(TYPE_uns, 0x03, 0, TY, "TYPE_uns")	// uns08, uns16, uns32, uns64
//~ TOKDEF(TYPE_int, 0x04, 0, TY, "TYPE_int")	// int08, int16, int32, int64
//~ TOKDEF(TYPE_flt, 0x05, 0, TY, "TYPE_flt")	// flt32, flt64
//~ TOKDEF(TYPE_ptr, 0x06, 0, TY, "TYPE_ptr")	// ptr?, label, string, array, variable|function
//~ TOKDEF(TYPE_def, 0x07, 0, ID, "define")	// enum, union, struct, class, constant|typename

TOKDEF(TYPE_bol, 0x02, 0, TY, "TYPE_bol")	// boolean

TOKDEF(TYPE_i08, 0x03, 0, TY, "TYPE_i08")	// char
TOKDEF(TYPE_u08, 0x03, 0, TY, "TYPE_u08")	// byte
TOKDEF(TYPE_i16, 0x04, 0, TY, "TYPE_i16")	// short
TOKDEF(TYPE_u16, 0x03, 0, TY, "TYPE_u16")	// u16
TOKDEF(TYPE_i32, 0x04, 0, TY, "TYPE_i32")	// signed
TOKDEF(TYPE_u32, 0x03, 0, TY, "TYPE_u32")	// unsigned
TOKDEF(TYPE_i64, 0x04, 0, TY, "TYPE_i64")	// long
TOKDEF(TYPE_u64, 0x04, 0, TY, "TYPE_u64")	// long
TOKDEF(TYPE_ptr, 0x05, 0, TY, "TYPE_ptr")	// array, string, label, variable|function
TOKDEF(TYPE_f32, 0x06, 0, TY, "TYPE_f32")	// float
TOKDEF(TYPE_f64, 0x06, 0, TY, "TYPE_f64")	// double
//~ TOKDEF(TYPE_ptr, 0x06, 0, TY, "TYPE_ptr")	// array, string, label, variable|function
TOKDEF(TYPE_arr, 0x06, 0, TY, "TYPE_arr")
TOKDEF(TYPE_fnc, 0x06, 0, XX, "TYPE_fnc")	// function

TOKDEF(TYPE_def, 0x07, 0, ID, "define")	// class, struct, union, constant|typename
TOKDEF(TYPE_enu, 0x07, 0, ID, "enum")		// enum
TOKDEF(TYPE_rec, 0x07, 0, ID, "struct")
TOKDEF(TYPE_cls, 0x07, 0, TY, "class")		// class

//~ TOKDEF(CNST_uns, 0x02, 0, TY, "CNST_uns")	// uns64|uns32 (impl)
TOKDEF(CNST_int, 0x03, 0, TY, "CNST_int")	// int64|int32 (impl)
TOKDEF(CNST_flt, 0x04, 0, TY, "CNST_flt")	// flt64
TOKDEF(CNST_str, 0x05, 0, TY, "CNST_str")	// string

TOKDEF(TYPE_ref, 0x05, 0, TY, "TYPE_ref")	// identifyer/variable/typename/constant

TOKDEF(EMIT_opc, 0x00, 0, ID, "emit")

TOKDEF(OPER_jmp, 0x00, 0, ID, "if")		// stmt: if then else | break | continue | goto
TOKDEF(OPER_els, 0x00, 0, ID, "else")		// ????
TOKDEF(OPER_for, 0x00, 0, ID, "for")		// stmt: for, while, repeat
TOKDEF(OPER_nop, 0x00, 0, XX, ";")		// stmt: expr
TOKDEF(OPER_beg, 0x00, 0, XX, "{")		// stmt: stmt {...}
TOKDEF(OPER_end, 0x00, 0, XX, "}")		// stmt: stmt {...}

TOKDEF(OPER_com, 0x01, 2, OP, ",")		// a, b

TOKDEF(ASGN_set, 0x12, 2, OP, "=")		// a := b
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

TOKDEF(OPER_ior, 0x06, 2, OP, "|")		// a | b
TOKDEF(OPER_xor, 0x07, 2, OP, "^")		// a ^ b
TOKDEF(OPER_and, 0x08, 2, OP, "&")		// a & b

TOKDEF(OPER_neq, 0x09, 2, OP, "!=")		// a != b
TOKDEF(OPER_equ, 0x09, 2, OP, "==")		// a == b
TOKDEF(OPER_gte, 0x0a, 2, OP, ">")		// a > b
TOKDEF(OPER_geq, 0x0a, 2, OP, ">=")		// a >= b
TOKDEF(OPER_lte, 0x0a, 2, OP, "<")		// a < b
TOKDEF(OPER_leq, 0x0a, 2, OP, "<=")		// a <= b

TOKDEF(OPER_shr, 0x0b, 2, OP, ">>")		// a >> b
TOKDEF(OPER_shl, 0x0b, 2, OP, "<<")		// a << b

TOKDEF(OPER_add, 0x0c, 2, OP, "+")		// a+b
TOKDEF(OPER_sub, 0x0c, 2, OP, "-")		// a-b
TOKDEF(OPER_mul, 0x0d, 2, OP, "*")		// a*b
TOKDEF(OPER_div, 0x0d, 2, OP, "/")		// a/b
TOKDEF(OPER_mod, 0x0d, 2, OP, "%")		// a%b

TOKDEF(OPER_pls, 0x1e, 1, OP, "+")		// +a		unary plus
TOKDEF(OPER_mns, 0x1e, 1, OP, "-")		// -a		unary minus
TOKDEF(OPER_cmt, 0x1e, 1, OP, "~")		// ~a		complement
TOKDEF(OPER_not, 0x1e, 1, OP, "!")		// !a		logical not

TOKDEF(OPER_idx, 0x0f, 2, OP, "[]")		// a[n]		index / pointer to [a] <=> *a
TOKDEF(OPER_fnc, 0x0f, 2, OP, "()")		// a(x)		function call, sizeof, cast/ctor.
TOKDEF(OPER_dot, 0x0f, 2, OP, ".")		// a.b		member

//~ temp =======================================================================
//~ TOKDEF(PNCT_lb , 0x00, 0, XX, "{")		// brackets
//~ TOKDEF(PNCT_rb , 0x00, 0, XX, "}")
TOKDEF(PNCT_lc , 0x00, 0, XX, "[")		// curlies
TOKDEF(PNCT_rc , 0x00, 0, XX, "]")
TOKDEF(PNCT_lp , 0x00, 0, XX, "PNCT_lp")		// parentheses
TOKDEF(PNCT_rp , 0x00, 0, XX, "PNCT_rp")
TOKDEF(PNCT_qst, 0x00, 0, XX, "?")		// question mark
TOKDEF(PNCT_cln, 0x00, 0, XX, ":")		// colon

TOKDEF(TOKN_err, 0xff, 0, XX, "TOKN_err")
TOKDEF(CODE_inf, 0x00, 0, XX, "CODE_inf")

/*/  #define TOKDEF(NAME, PREC, ARGC, KIND, STR) ================================
//~ Types ======================================================================
TOKDEF(TYPE_any, 0x00, 0, TY, "TYPE_err")	// first enum == 0
TOKDEF(TYPE_vid, 0x01, 0, TY, "TYPE_vid")	// void

TOKDEF(TYPE_bol, 0x02, 0, TY, "bool")		// boolean
TOKDEF(TYPE_u08, 0x02, 0, TY, "uns08")		// byte
TOKDEF(TYPE_u16, 0x02, 0, TY, "uns16")		// u16
TOKDEF(TYPE_u32, 0x02, 0, TY, "uns32")		// unsigned

TOKDEF(TYPE_i08, 0x03, 0, TY, "int08")	// char
TOKDEF(TYPE_i16, 0x03, 0, TY, "int16")	// short
TOKDEF(TYPE_i32, 0x03, 0, TY, "int32")	// signed
TOKDEF(TYPE_i64, 0x03, 0, TY, "int64")	// long

TOKDEF(TYPE_f32, 0x04, 0, TY, "flt32")	// float
TOKDEF(TYPE_f64, 0x04, 0, TY, "flt64")	// double

TOKDEF(TYPE_ptr, 0x05, 0, TY, "TYPE_ptr")	// array, string, label, variable|function
TOKDEF(TYPE_arr, 0x05, 0, TY, "TYPE_arr")
//~ TOKDEF(TYPE_str, 0x05, 0, ID, "string")

//~ TOKDEF(TYPE_rgb, 0x02,  32, ID, "color")		// packed u08[4]

//~ TOKDEF(TYPE_p4c, 0x03,  64, ID, "p4i08")		// packed i08[16]
//~ TOKDEF(TYPE_p4s, 0x03,  64, ID, "p4i16")		// packed i16[8]
//~ TOKDEF(TYPE_p4i, 0x03,  64, ID, "p4i32")		// packed i32[4]
//~ TOKDEF(TYPE_p4l, 0x03,  64, ID, "p4i64")		// packed i64[2]

//~ TOKDEF(TYPE_p4b, 0x03,  64, ID, "p4u08")		// packed i08[16]
//~ TOKDEF(TYPE_p4w, 0x03,  64, ID, "p4u16")		// packed i16[8]
//~ TOKDEF(TYPE_p4d, 0x03,  64, ID, "p4u32")		// packed i32[4]
//~ TOKDEF(TYPE_p4q, 0x03,  64, ID, "p4u64")		// packed i64[2]

TOKDEF(TYPE_pf4, 0x04, 128, XX, "p4f32")		// packed f32[4]
TOKDEF(TYPE_pf2, 0x04, 128, XX, "p4f64")		// packed f64[2]

//~ TOKDEF(TYPE_pck, 0x06, 0, ID, "packed")	// rgb, vec, rat, ???
TOKDEF(TYPE_def, 0x07, 0, ID, "define")	// class, struct, union, constant|typename
TOKDEF(TYPE_enu, 0x07, 0, ID, "enum")		// enum

TOKDEF(TYPE_rec, 0x07, 0, ID, "struct")
//~ TOKDEF(TYPE_uni, 0x07, 0, ID, "union")
//~ TOKDEF(TYPE_cls, 0x07, 0, ID, "class")

//~ TOKDEF(TYPE_opr, 0x08, 0, XX, "TYPE_opr")		// operator
TOKDEF(TYPE_fnc, 0x08, 0, XX, "TYPE_fnc")		// function
TOKDEF(TYPE_ref, 0x08, 0, XX, "TYPE_ref")		// variable

//~ TOKDEF(CNST_chr, 0x03, 0, XX, NULL)		// 'a'
//~ TOKDEF(CNST_uns, 0x03, 0, XX, NULL)		// 12U
TOKDEF(CNST_int, 0x14, 0, XX, "CNST_int")		// constant
TOKDEF(CNST_flt, 0x14, 0, XX, "CNST_flt")		// constant
TOKDEF(CNST_str, 0x15, 0, XX, "CNST_str")		// constant

TOKDEF(TYPE_idf, 0x00, 0, XX, "TYPE_idf")	// identifyer (untyped)
TOKDEF(TYPE_lnk, 0x00, 0, XX, "TYPE_lnk")	// identifyer (untyped)

//~ TOKDEF(QUAL_sta, 0x80,  0, ID, "static")	// static
//~ TOKDEF(QUAL_sta, 0x80,  0, ID, "parallel")	// static
//~ TOKDEF(QUAL_loc, 0x20,  0, ID, "local")	// !static:packed
//~ TOKDEF(QUAL_con, 0x10,  0, ID, "const")	// constant:readonly
//~ TOKDEF(QUAL_vol, 0x20,  0, ID, "volat")	// volatile:memory
//~ final = constant + volatile,		// final

//~ Statements =================================================================
TOKDEF(OPER_jmp, 0x00, 0, ID, "if")		// stmt: if then else | break | continue | goto
TOKDEF(OPER_els, 0x00, 0, ID, "else")		// ????
TOKDEF(OPER_for, 0x00, 0, ID, "for")		// stmt: for, while, repeat
TOKDEF(OPER_nop, 0x00, 0, XX, ";")		// stmt: expr
TOKDEF(OPER_beg, 0x00, 0, XX, "{}")		// stmt: stmt {...}

//~ ============================================================================

//~ Operators ==================================================================
TOKDEF(OPER_com, 0x01, 2, OP, ",")		// a, b
TOKDEF(ASGN_set, 0x12, 2, OP, "=")		// a := b
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
//~ TOKDEF(OPER_lor, 0x04, 2, OP, "||")		// a || b
//~ TOKDEF(OPER_lnd, 0x05, 2, OP, "&&")		// a && b
TOKDEF(OPER_ior, 0x06, 2, OP, "|")		// a | b
TOKDEF(OPER_xor, 0x07, 2, OP, "^")		// a ^ b
TOKDEF(OPER_and, 0x08, 2, OP, "&")		// a & b
TOKDEF(OPER_neq, 0x09, 2, OP, "!=")		// a != b
TOKDEF(OPER_equ, 0x09, 2, OP, "==")		// a == b
//~ TOKDEF(OPER_min, 0x09, 2, OP, "<?")		// a <? b
//~ TOKDEF(OPER_max, 0x09, 2, OP, ">?")		// a >? b
TOKDEF(OPER_gte, 0x0a, 2, OP, ">")		// a > b
TOKDEF(OPER_geq, 0x0a, 2, OP, ">=")		// a >= b
TOKDEF(OPER_lte, 0x0a, 2, OP, "<")		// a < b
TOKDEF(OPER_leq, 0x0a, 2, OP, "<=")		// a <= b
TOKDEF(OPER_shr, 0x0b, 2, OP, ">>")		// a >> b
TOKDEF(OPER_shl, 0x0b, 2, OP, "<<")		// a << b
TOKDEF(OPER_add, 0x0c, 2, OP, "+")		// a+b
TOKDEF(OPER_sub, 0x0c, 2, OP, "-")		// a-b
TOKDEF(OPER_mul, 0x0d, 2, OP, "*")		// a*b
TOKDEF(OPER_div, 0x0d, 2, OP, "/")		// a/b
TOKDEF(OPER_mod, 0x0d, 2, OP, "%")		// a%b
TOKDEF(OPER_pls, 0x1e, 1, OP, "+")		// +a		unary plus
TOKDEF(OPER_mns, 0x1e, 1, OP, "-")		// -a		unary minus
TOKDEF(OPER_cmt, 0x1e, 1, OP, "~")		// ~a		complement
TOKDEF(OPER_not, 0x1e, 1, OP, "!")		// !a		logical not
TOKDEF(OPER_idx, 0x0f, 2, OP, "[]")		// a[n]		index / pointer to [a] <=> *a
TOKDEF(OPER_fnc, 0x0f, 2, OP, "()")		// a()		function call, sizeof, cast/ctor.
TOKDEF(OPER_dot, 0x0f, 2, OP, ".")		// a.b		member

//~ temp =======================================================================
TOKDEF(PNCT_lb , 0x00, 0, XX, "{")		// brackets
TOKDEF(PNCT_rb , 0x00, 0, XX, "}")
TOKDEF(PNCT_lc , 0x00, 0, XX, "[")		// curlies
TOKDEF(PNCT_rc , 0x00, 0, XX, "]")
TOKDEF(PNCT_lp , 0x00, 0, XX, "(")		// parentheses
TOKDEF(PNCT_rp , 0x00, 0, XX, ")")
TOKDEF(PNCT_qst, 0x00, 0, XX, "?")		// question mark
TOKDEF(PNCT_cln, 0x00, 0, XX, ":")		// colon

TOKDEF(TOKN_err, 0xff, 0, XX, "TOKN_err")
TOKDEF(CODE_inf, 0x00, 0, XX, "CODE_inf")

//{ ============================================================================
//~ TOKDEF(TYPE_vid, 0x01,   0, ID, "void")		// void
//~ TOKDEF(TYPE_rgb, 0x0d,  32, ID, "color")		// packed byte
//~ TOKDEF(TYPE_fv4, 0x0e, 128, ID, "vector")		// packed float
//~ TOKDEF(TYPE_fv2, 0x0f, 128, ID, "complex")		// packed double
//~ TOKDEF(TYPE_ptr, 0x10,   0, XX, "__ptr")		// pointer
//~ TOKDEF(TYPE_int, 0x10,   0, XX, "__ptr")		// pointer
//~ TYPE_int = TYPE_i32;
//~ TYPE_uns = TYPE_u32;

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
//~ TOKDEF(OPER_jmp, 0x00, 0, 2, "if")			// stmt: if then else | break | continue | goto
//~ TOKDEF(OPER_els, 0x00, 0, 2, "else")
//~ TOKDEF(OPER_cas, 0x00, 0, 2, "case")
//~ TOKDEF(OPER_go2, 0x00, 0, 2, "goto")
//~ TOKDEF(OPER_brk, 0x00, 0, 2, "break")
//~ TOKDEF(OPER_swi, 0x00, 0, 2, "switch")
//? TOKDEF(OPER_def, 0x00, 0, 2, "default")
//~ TOKDEF(OPER_con, 0x00, 0, 2, "continue")
//~ TOKDEF(OPER_for, 0x00, 0, 2, "for")		// stmt: for, while, repeat
//~ TOKDEF(OPER_wht, 0x00, 0, 2, "while")		// while(...) {...}
//~ TOKDEF(OPER_whf, 0x00, 0, 2, "until")		// until(...) {...}
//~ TOKDEF(OPER_rep, 0x00, 0, 2, "repeat")		// repeat {...} ((until(...))|(while(...)))?
//~ TOKDEF(OPER_wth, 0x00, 0, 2, "with")

//~ #define isAssign(__OPR) (opt[__OPR].type <= 1)
//~ #define isBinary(__OPR) (opt[__OPR].size == 2)
//} */

#undef TOKDEF
#endif

#ifdef OPCDEF
/* #define OPCDEF(Name, Code, Size, Args, Push, Time, Mnem) Name = Code
 * Name : enum name;
 * Code : value of enum;
 * Size : opcode length (1 + argsize) (-1 variable) (0 ???);
 * Args : on stack elements check (pops);
 * Stch : ;
 * Tick : ticks for executing instruction;
 */
//~ sys ========================================================================
OPCDEF(opc_nop,  0x00, 1, 0, 0, 1,	"nop")		// ;				[…, a, b, c => […, a, b, c;
OPCDEF(opc_loc,  0x01, 2, 0, 0, 1,	"loc")		// sp += 4*arg.1;		[…, a, b, c => […, a, b, c, …;
OPCDEF(opc_pop,  0x02, 2, 0, 0, 1,	"pop")		// sp -= 4*arg.1;		[…, a, b, c => […, a, b;
OPCDEF(opc_x03,  0x03, 0, 0, 0, 1,	NULL)		// 				[…, a, b, c => […, a, b, c, a;
OPCDEF(opc_x04,  0x04, 0, 0, 0, 1,	NULL)		// 				[…, a, b, c => […, c, b;
OPCDEF(opc_x05,  0x05, 0, 0, 0, 1,	"ldsp")	// push(SP + arg.3); SP += 4;	[…, a, b, c => […, a, b, c, SP;
OPCDEF(opc_call, 0x06, 0, 0, 0, 1,	"call")	// push(IP); ip += arg.3;	[…, a, b, c => […, a, b, c, IP;
OPCDEF(opc_ji,   0x07, 1, 0, 0, 1,	"jmpi")	// IP = sp(0); pop;		[…, a, b, c => […, a, b;
OPCDEF(opc_jmp,  0x08, 4, 0, 0, 1,	"jmp")		// IP += arg.3;			[…, a, b, c => […, a, b, c;
OPCDEF(opc_jnz,  0x09, 4, 0, 0, 1,	"jnz")		// if sp(0)!=0 IP += arg.3;pop;	[…, a, b, c => […, a, b;
OPCDEF(opc_jz ,  0x0a, 4, 0, 0, 1,	"jz")		// if sp(0)==0 IP += arg.3;pop;	[…, a, b, c => […, a, b;
OPCDEF(opc_x0b,  0x0b, 0, 0, 0, 1,	"js")		// if sp(0) <0 IP += arg.3;pop;	[…, a, b, c => […, a, b;
OPCDEF(opc_cmp,  0x0c, 0, 0, 0, 1,	"cmp")		// TEMP: this is Ugly
OPCDEF(opc_libc, 0x0d, 2, 0, 0, 1,	"libc")	//-lib call
OPCDEF(opc_task, 0x0e, 4, 0, 0, 1,	"task")	// arg.3 : [code:16][data:8] task, [?fork if (arg.code == 0)]
OPCDEF(opc_sysc, 0x0f, 2, 0, 0, 1,	"sysc")	// arg.1 : [type:8](exit, halt, wait?join?sync, trap) (alloc, free, copy, init) ...
//~ stk ========================================================================
OPCDEF(opc_ldc1, 0x10, 2, 0, 0, 1,	"ldc.b08")	// push(arg.1);			:2[…, a, b, c => […, a, b, c, 2;
OPCDEF(opc_ldc2, 0x11, 3, 0, 0, 1,	"ldc.b16")	// push(arg.2);			:2[…, a, b, c => […, a, b, c, 2;
OPCDEF(opc_ldc4, 0x12, 5, 0, 0, 1,	"ldc.b32")	// push(arg.4);			:2[…, a, b, c => […, a, b, c, 2;
OPCDEF(opc_ldc8, 0x13, 9, 0, 0, 1,	"ldc.b64")	// push(arg.8);			:2[…, a, b, c => […, a, b, c, 2;
OPCDEF(opc_dup1, 0x14, 2, 0, 0, 1,	"dup.1")	// push(sp(n));			:2[…, a, b, c => […, a, b, c, a;
OPCDEF(opc_dup2, 0x15, 2, 0, 0, 1,	"dup.2")	// push(sp(n))x2;		:2[…, a, b, c => […, a, b, c, a, b;
OPCDEF(opc_dup4, 0x16, 2, 0, 0, 1,	"dup.4")	// push(sp(n))x4;		:3[…, a, b, c, d => […, a, b, c, d, a, b, c, d;
OPCDEF(opc_set1, 0x17, 2, 0, 0, 1,	"set.1")	// 
OPCDEF(opc_set2, 0x18, 2, 0, 0, 1,	"set.2")	// 
OPCDEF(opc_set4, 0x19, 2, 0, 0, 1,	"set.4")	// 
OPCDEF(opc_ldz1, 0x1a, 1, 0, 0, 1,	"ldz.1")	// push(0);			[…, a, b, c => […, a, b, c, 0;
OPCDEF(opc_ldz2, 0x1b, 1, 0, 0, 1,	"ldz.2")	// push(0, 0);			[…, a, b, c => […, a, b, c, 0, 0;
OPCDEF(opc_ldz4, 0x1c, 1, 0, 0, 1,	"ldz.4")	// push(0, 0, 0, 0);		[…, a, b, c => […, a, b, c, 0, 0, 0, 0;
OPCDEF(opc_ldcr, 0x1d, 5, 0, 0, 1,	"ldc.ref")	// TEMP
OPCDEF(opc_ldcf, 0x1e, 5, 0, 0, 1,	"ldc.f32")	// TEMP
OPCDEF(opc_ldcF, 0x1f, 9, 0, 0, 1,	"ldc.f64")	// TEMP
//~ mem ========================================================================
OPCDEF(opc_ldi1, 0x20, 0, 0, 0, 1,	"ldi.b")	// copy(sp, sp(0) 1);		[…, a, b, c => […, a, b, *c;
OPCDEF(opc_ldi2, 0x21, 0, 0, 0, 1,	"ldi.h")	// copy(sp, sp(0) 2);		[…, a, b, c => […, a, b, *c;
OPCDEF(opc_ldi4, 0x22, 0, 0, 0, 1,	"ldi.w")	// copy(sp, sp(0) 4);		[…, a, b, c => […, a, b, *c;
OPCDEF(opc_ldi8, 0x23, 0, 0, 0, 1,	"ldi.d")	// copy(sp, sp(0) 8);		[…, a, b, c => […, a, b, *c:1, *c:2;
OPCDEF(opc_ldiq, 0x24, 0, 0, 0, 1,	"ldi.q")	// copy(sp, sp(0) 16);		[…, a, b, c => […, a, b, *c;
OPCDEF(opc_sti1, 0x25, 0, 0, 0, 1,	"sti.b")	// copy(sp(1) sp(0) 1);pop2;	[…, a, b, c => […, a
OPCDEF(opc_sti2, 0x26, 0, 0, 0, 1,	"sti.h")	// copy(sp(1) sp(0) 2);pop2;	[…, a, b, c => […, a
OPCDEF(opc_sti4, 0x27, 0, 0, 0, 1,	"sti.w")	// copy(sp(1) sp(0) 4);pop2;	[…, a, b, c => […, a
OPCDEF(opc_sti8, 0x28, 0, 0, 0, 1,	"sti.d")	// copy(sp(1) sp(0) 8);pop3;	[…, a, b, c => […, a
OPCDEF(opc_stiq, 0x29, 0, 0, 0, 1,	"sti.q")	// copy(sp(1) sp(0) 16);pop5;	[…, a, b, c => […, a
OPCDEF(opc_x2a,  0x2a, 5, 0, 0, 1,	"ldm.w")	// (32)
OPCDEF(opc_x2b,  0x2b, 5, 0, 0, 1,	"ldm.d")	// (64)
OPCDEF(opc_x2c,  0x2c, 5, 0, 0, 1,	"ldm.q")	// (128)
OPCDEF(opc_x2d,  0x2d, 5, 0, 0, 1,	"stm.w")	// 
OPCDEF(opc_x2e,  0x2e, 5, 0, 0, 1,	"stm.d")	// 
OPCDEF(opc_x2f,  0x2f, 5, 0, 0, 1,	"stm.q")	// 
//~ bit[ 32] ===================================================================
OPCDEF(b32_cmt,  0x30, 1, 0, 0, 1,	"cmt.b32")	// sp(0) = ~sp(0);
OPCDEF(b32_adc,  0x31, 0, 0, 0, 1,	"add.Carry")	// sp(1) += sp(0); pop;
OPCDEF(b32_sbb,  0x32, 0, 0, 0, 1,	"sub.Borrow")	// sp(1) -= sp(0); pop;
OPCDEF(u32_mul,  0x33, 0, 0, 0, 1,	"mul.u32")	// sp(1) *= sp(0); pop;
OPCDEF(u32_div,  0x34, 0, 0, 0, 1,	"div.u32")	// sp(1) /= sp(0); pop;
OPCDEF(u32_mod,  0x35, 0, 0, 0, 1,	"mod.u32")	// sp(1) %= sp(0); pop;
OPCDEF(u32_x46,  0x36, 0, 0, 0, 1,	NULL)		// sp(0) = zxt(sp(0));
OPCDEF(b32_bit,  0x37, 2, 0, 0, 1,	"bit.???")	// any, all, ...
OPCDEF(u32_clt,  0x38, 0, 0, 0, 1,	"clt.u32")	//?
OPCDEF(u32_cle,  0x39, 0, 0, 0, 1,	"cle.u32")	//?
OPCDEF(b32_and,  0x3a, 1, 0, 0, 1,	"and.b32")	// and
OPCDEF(b32_ior,  0x3b, 1, 0, 0, 1,	"or.b32")	// or
OPCDEF(b32_xor,  0x3c, 1, 0, 0, 1,	"xor.b32")	// xor
OPCDEF(b32_shl,  0x3d, 1, 0, 0, 1,	"shl.b32")	// shl
OPCDEF(b32_shr,  0x3e, 1, 0, 0, 1,	"shr.b32")	// shr
OPCDEF(b32_sar,  0x3f, 1, 0, 0, 1,	"sar.b32")	//-Extended ops: idx, rev, imm, mem
//~ OPCDEF(0x37, 0, opc_rot,	"rot")		//?rotate
//~ OPCDEF(0x38, 1, opc_extu,	"zxt")		//!zero.extend arg:[offs:3][size:5]
//~ OPCDEF(0x39, 1, opc_exts,	"sxt")		//!sign.extend arg:[offs:3][size:5]
//~ OPCDEF(0x3a, 1, opc_bit,	"")		// bit.imm(rot, shl, sar, shr, set, get, ...)arg:[eop:3][cnt:5]
//~ OPCDEF(0x3b, 0, opc_x3b,	NULL)		// ?
//~ OPCDEF(0x3f, 0, opc_madu,	"mad.u32")	// sp(2) += sp(1)*sp(0); pop2;	[…, a, b, c => […, a + b * c;
//~ i32[ 32] ===================================================================
OPCDEF(i32_neg,  0x40, 1, 0, 0, 1,	"neg.i32")	// sp(0) = -sp(0);
OPCDEF(i32_add,  0x41, 1, 0, 0, 1,	"add.i32")	// sp(1) += sp(0); pop;
OPCDEF(i32_sub,  0x42, 1, 0, 0, 1,	"sub.i32")	// sp(1) -= sp(0); pop;
OPCDEF(i32_mul,  0x43, 1, 0, 0, 1,	"mul.i32")	// sp(1) *= sp(0); pop;
OPCDEF(i32_div,  0x44, 1, 0, 0, 1,	"div.i32")	// sp(1) /= sp(0); pop;
OPCDEF(i32_mod,  0x45, 1, 0, 0, 1,	"mod.i32")	// sp(1) %= sp(0); pop;
OPCDEF(i32_x46,  0x46, 2, 0, 0, 1,	"mul.u32")	//
OPCDEF(i32_x47,  0x47, 0, 0, 0, 1,	"div.u32")	//
OPCDEF(i32_x48,  0x48, 0, 0, 0, 1,	"mod.u32")	//
OPCDEF(i32_x49,  0x49, 0, 0, 0, 1,	"cmp.cvt")	//
OPCDEF(i32_x4a,  0x4a, 0, 0, 0, 1,	NULL)	//
OPCDEF(i32_bol,  0x4b, 0, 0, 0, 1,	"tob.i")	// i32 to bol
OPCDEF(i32_f32,  0x4c, 1, 0, 0, 1,	"tof.i")	// i32 to f32
OPCDEF(i32_i64,  0x4d, 1, 0, 0, 1,	"tol.i")	// i32 to i64
OPCDEF(i32_f64,  0x4e, 1, 0, 0, 1,	"tod.i")	// i32 to f64
OPCDEF(i32_x4f,  0x4f, 0, 0, 0, 1,	NULL)		//?Extended ops: idx,rev,imm,mem
//~ f32[ 32] ===================================================================
OPCDEF(f32_neg,  0x50, 1, 0, 0, 1,	"neg.f32")	// sp(0) = -sp(0);
OPCDEF(f32_add,  0x51, 1, 0, 0, 1,	"add.f32")	// sp(1) += sp(0); pop;
OPCDEF(f32_sub,  0x52, 1, 0, 0, 1,	"sub.f32")	// sp(1) -= sp(0); pop;
OPCDEF(f32_mul,  0x53, 1, 0, 0, 1,	"mul.f32")	// sp(1) *= sp(0); pop;
OPCDEF(f32_div,  0x54, 1, 0, 0, 1,	"div.f32")	// sp(1) /= sp(0); pop;
OPCDEF(f32_mod,  0x55, 1, 0, 0, 1,	"mod.f32")	// sp(1) %= sp(0); pop;
OPCDEF(f32_x56,  0x56, 2, 0, 0, 1,	NULL)	// 
OPCDEF(f32_x57,  0x57, 0, 0, 0, 1,	NULL)	// 
OPCDEF(f32_x58,  0x58, 0, 0, 0, 1,	NULL)	// 
OPCDEF(f32_x59,  0x59, 0, 0, 0, 1,	NULL)	// 
OPCDEF(f32_x5a,  0x5a, 0, 0, 0, 1,	NULL)	//
OPCDEF(f32_bol,  0x5b, 0, 0, 0, 1,	NULL)	// f32 to bool
OPCDEF(f32_i32,  0x5c, 1, 0, 0, 1,	"toi.f")	// f32 to i32
OPCDEF(f32_i64,  0x5d, 1, 0, 0, 1,	"toI.f")	// f32 to i64
OPCDEF(f32_f64,  0x5e, 1, 0, 0, 1,	"toF.f")	// f32 to f64
OPCDEF(f32_x5f,  0x5f, 0, 0, 0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ i64[ 64] ===================================================================
OPCDEF(i64_neg,  0x60, 1, 0, 0, 1,	"neg.i64")	// sp(0) = -sp(0);
OPCDEF(i64_add,  0x61, 1, 0, 0, 1,	"add.i64")	// sp(1) += sp(0); pop2;
OPCDEF(i64_sub,  0x62, 1, 0, 0, 1,	"sub.i64")	// sp(1) -= sp(0); pop2;
OPCDEF(i64_mul,  0x63, 1, 0, 0, 1,	"mul.i64")	// sp(1) *= sp(0); pop2;
OPCDEF(i64_div,  0x64, 1, 0, 0, 1,	"div.i64")	// sp(1) /= sp(0); pop2;
OPCDEF(i64_mod,  0x65, 1, 0, 0, 1,	"mod.i64")	// sp(1) %= sp(0); pop2;
OPCDEF(i64_x66,  0x66, 2, 0, 0, 1,	NULL)	// 
OPCDEF(i64_x67,  0x67, 0, 0, 0, 1,	NULL)	// 
OPCDEF(i64_x68,  0x68, 0, 0, 0, 1,	NULL)	// 
OPCDEF(i64_x69,  0x69, 0, 0, 0, 1,	NULL)	// 
OPCDEF(i64_x6a,  0x6a, 0, 0, 0, 1,	NULL)		//
OPCDEF(i64_bol,  0x6b, 0, 0, 0, 1,	NULL)		//
OPCDEF(i64_i32,  0x6c, 1, 0, 0, 1,	"toi.I")	// i64 to i32
OPCDEF(i64_i64,  0x6d, 1, 0, 0, 1,	"tof.I")	// i64 to f32
OPCDEF(i64_f64,  0x6e, 1, 0, 0, 1,	"toF.I")	// i64 to f64
OPCDEF(i64_x6f,  0x6f, 0, 0, 0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ f64[ 64] ===================================================================
OPCDEF(f64_neg,  0x70, 1, 0, 0, 1,	"neg.f64")	// sp(0) = -sp(0);
OPCDEF(f64_add,  0x71, 1, 0, 0, 1,	"add.f64")	// sp(1) += sp(0); pop2;
OPCDEF(f64_sub,  0x72, 1, 0, 0, 1,	"sub.f64")	// sp(1) -= sp(0); pop2;
OPCDEF(f64_mul,  0x73, 1, 0, 0, 1,	"mul.f64")	// sp(1) *= sp(0); pop2;
OPCDEF(f64_div,  0x74, 1, 0, 0, 1,	"div.f64")	// sp(1) /= sp(0); pop2;
OPCDEF(f64_mod,  0x75, 1, 0, 0, 1,	"mod.f64")	// sp(1) %= sp(0); pop2;
OPCDEF(f64_x76,  0x76, 1, 0, 0, 1,	NULL)	// 
OPCDEF(f64_x77,  0x77, 0, 0, 0, 1,	NULL)	// 
OPCDEF(f64_x78,  0x78, 0, 0, 0, 1,	NULL)	// 
OPCDEF(f64_x79,  0x79, 0, 0, 0, 1,	NULL)	// 
OPCDEF(f64_x7a,  0x7a, 0, 0, 0, 1,	NULL)		//
OPCDEF(f64_bol,  0x7b, 0, 0, 0, 1,	NULL)		//
OPCDEF(f64_i32,  0x7c, 1, 0, 0, 1,	"toi.F")	// f64 TO i32
OPCDEF(f64_i64,  0x7d, 1, 0, 0, 1,	"toI.F")	// f64 TO I64
OPCDEF(f64_f32,  0x7e, 1, 0, 0, 1,	"tof.F")	// f64 TO f32
OPCDEF(f64_x7f,  0x7f, 0, 0, 0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ p4f=========[128] ==========================================================8
OPCDEF(p4f_neg,  0x80, 1, 0, 0, 1,	"neg.fv4")	// sp(0) = -sp(0);
OPCDEF(p4f_add,  0x81, 1, 0, 0, 1,	"add.fv4")	// sp(4) += sp(0); pop4;
OPCDEF(p4f_sub,  0x82, 1, 0, 0, 1,	"sub.fv4")	// sp(4) -= sp(0); pop4;
OPCDEF(p4f_mul,  0x83, 1, 0, 0, 1,	"mul.fv4")	// sp(4) *= sp(0); pop4;
OPCDEF(p4f_div,  0x84, 1, 0, 0, 1,	"div.fv4")	// sp(4) /= sp(0); pop4;
OPCDEF(p4f_crs,  0x85, 1, 0, 0, 1,	"crs.fv3")	// cross
OPCDEF(p4f_x86,  0x86, 0, 0, 0, 1,	"")		// ???
OPCDEF(p4f_cmp,  0x87, 0, 0, 0, 1,	"ceq.fv4")	// sp(1) = sp(1) == sp(0) pop?;
OPCDEF(p4f_min,  0x88, 1, 0, 0, 1,	"min.fv4")	// sp(1) = sp(1) <? sp(0) pop4;
OPCDEF(p4f_max,  0x89, 1, 0, 0, 1,	"max.fv4")	// sp(1) = sp(1) >? sp(0) pop4;
OPCDEF(p4f_x8a,  0x8a, 0, 0, 0, 1,	"")		// pov, lrp
OPCDEF(p4f_x8b,  0x8b, 0, 0, 0, 1,	"?dp2")		// abs, log, exp, sin, cos, ...
OPCDEF(p4f_dp3,  0x8c, 1, 0, 0, 1,	"dot.fv3")	//
OPCDEF(p4f_dp4,  0x8d, 1, 0, 0, 1,	"dot.fv4")	//
OPCDEF(p4f_dph,  0x8e, 1, 0, 0, 1,	"dot.fvh")	//
OPCDEF(p4f_x8f,  0x8f, 0, 0, 0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ p4d[128] ===================================================================9
OPCDEF(p4d_neg,  0x90, 0, 0, 0, 1,	"neg.pf2")	// sp(0) = -sp(0);
OPCDEF(p4d_add,  0x91, 0, 0, 0, 1,	"add.pf2")	// sp(1) += sp(0); pop4;
OPCDEF(p4d_sub,  0x92, 0, 0, 0, 1,	"sub.pf2")	// sp(1) -= sp(0); pop4;
OPCDEF(p4d_mul,  0x93, 0, 0, 0, 1,	"mul.pf2")	// sp(1) *= sp(0); pop4;
OPCDEF(p4d_div,  0x94, 0, 0, 0, 1,	"div.pf2")	// sp(1) /= sp(0); pop4;
OPCDEF(p4d_x95,  0x95, 0, 0, 0, 1,	"")		// mod
OPCDEF(p4d_swz,  0x96, 1, 0, 0, 1,	"swz.pf4")	// swizzle
OPCDEF(p4d_ceq,  0x97, 0, 0, 0, 1,	"ceq.pf2")	// sp(1) = sp(1) == sp(0) pop?;
OPCDEF(p4d_min,  0x98, 0, 0, 0, 1,	"min.pf2")	// sp(1) = sp(1) <? sp(0) pop4;
OPCDEF(p4d_max,  0x99, 0, 0, 0, 1,	"max.pf2")	// sp(1) = sp(1) >? sp(0) pop4;
OPCDEF(p4d_x8a,  0x9a, 0, 0, 0, 1,	"")	//
OPCDEF(p4d_x8b,  0x9b, 0, 0, 0, 1,	"")	//
OPCDEF(p4d_x8c,  0x9c, 0, 0, 0, 1,	"")	//
OPCDEF(p4d_x8d,  0x9d, 0, 0, 0, 1,	"")	//
OPCDEF(p4d_x8e,  0x9e, 0, 0, 0, 1,	"")	//
OPCDEF(p4d_x8f,  0x9f,-1, 0, 0, 1,	NULL)		//-Extended ops: idx, rev, imm, mem
//~ arg:                 
	//~ 2,4,8,16

//~ rgb[ 32] ===================================================================a
//~ OPCDEF(rgb_neg,  0xa0, 0, 	"neg.i")
//~ OPCDEF(rgb_add,  0xa1, 0, 	"add.i")
//~ OPCDEF(rgb_sub,  0xa2, 0, 	"sub.i")
//~ OPCDEF(rgb_mul,  0xa3, 0, 	"mul.i")
//~ OPCDEF(rgb_div,  0xa4, 0, 	"div.i")
//~ OPCDEF(rgb_xa5,  0xa5, 0, 	NULL)		// ???
//~ OPCDEF(rgb_xa6,  0xa6, 0, 	NULL)		// ???
//~ OPCDEF(rgb_xa7,  0xa7, 0, 	"")
//~ OPCDEF(rgb_min,  0xa8, 0, 	"min")
//~ OPCDEF(rgb_max,  0xa9, 0, 	"max")
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

//? dp2
//~ abs
//~ sat
//~ nrm

//~ sin
//? cos
//~ exp
//~ log

//~ lrp
//~ pow

/* extended opcodes ???argc = 1: [size:2][type:2][code:4]
	type:
	ext_set = 0,		// 3 / sp(n) = opr(sp(n), sp(0)); pop(size & 1)				// a += b
	ext_dup = 1,		// 3 / sp(0) = opr(sp(0), sp(n)); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c
	ext_sto = 2,		// 6 / mp[n] = opr(mp[n], sp(0)); pop(size & 1)				// a += b
	ext_lod = 3,		// 6 / sp(0) = opr(sp(0), mp[n]); loc((1<<size)>>1[0, 1, 2, 4])		// (a+a) < c

	switch (type) {
		case ext_idx : ip += 3; {
			lhs = sp + u1[ip + 2];
			rhs = sp + 0;
			switch (size) {
				case 0 : dst = (sp -= 1);	// new result	push(opr(sp(n) sp(0)))
				case 1 : 					sp(0) = opr(sp(0) sp(n))
				case 2 : dst = (sp -= 1);	// get result	sp(n) = opr(sp(n) sp(0))
				case 3 : dst = (lhs);		// set result	sp(n) = opr(sp(n) sp(0)); pop
			}
		}
		dst = ip + 2
		lhs = sp(ip[2])
		rhs = ip(0)
	}

	code:			// bin(b32/b64)	num(int/flt)	vec(int/flt)64/128

	neg = 0x00,		// cmt		neg		neg
	add = 0x01,		//  ? 		add		add
	sub = 0x02,		//  ? 		sub		sub
	mul = 0x03,		// mul		mul		mul
	div = 0x04,		// div		div		 ? 
	mod = 0x05,		// rem		rem		crs

	ceq = 0x06,		//  ? 		ceq		ceq
	clt = 0x07,		// clt		clt		min
	cge = 0x08,		// cge		cge		max
	ext = 0x09,		// bit.		sin/bsf		abs, sin ...

	x0a,			// and		 ? 		 ? 
	x0b,			//  or		i32|not		 ? 
	x0c,			// xor		f32|not		dp3
	x0d,			// shl		i64|not		dph
	x0e,			// shr		f64|not		dp4
	x0f,			// sar		ext		ext

//~ */
/* opc_bit argc = 1: [extop:3][cnt:5]		// bit ops
	//~ opx == 000 : scan		??
		//~ cnt== 1 : any		// any bit set ( != 0) ? 0 : 1
		//~ cnt== 2 : all		// all bits set ( = -1)? 0 : 1
		//~ cnt== 3 : sgn		// sign bit set ( < 0) ? 0 : 1
		//~ cnt== 4 : par		// parity bit set ? 0 : 1
		//~ cnt== 5 : shl		// 1
		//~ cnt== 6 : shr		// 1
		//~ cnt== 7 : sar		// 1
		//~ cnt== 8 : rol		// 1
		//~ cnt== 9 : ror		// 1
		//~ cnt== a : cnt		// count bits
		//~ cnt== b : bsf		// scan forward
		//~ cnt== c : bsr		// scan reverse
	//~ opx == 001 : shl		// shift left
	//~ opx == 010 : shr		// shift right
	//~ opx == 011 : sar		// shift arithm right
	//~ opx == 100 : rot		// rotate
	//~ opx == 101 : get		// get bit
	//~ opx == 110 : set		// set bit
	//~ opx == 111 : cmt		// complement
//~ */
/* opc_ext argc = 1: [offs:3][size:5]		// extend
opc_zxt argc = 1: [offs:3][size:5]
		(unsigned)(val << (32 - (offs + size))) >> (32 - size);

opc_sxt argc = 1: [offs:3][size:5]
		(signed)(val << (32 - (offs + size))) >> (32 - size);
//~ */
/* opc_cmp argc = 1; [sat:2][cmp:4][typ:2]				// compare
	bit:[0-1: type : i32, f32, i64, f64]
	bit:[2-3: <ctz, ceq, clt, cle>]
		?0	nz	a == 0
		1	eq	a == b
		2	lt	a  < b
		3	le	a <= b

	bit:[  4: not (not result)]
	arg:[  5: .un: (unordered / unsigned)]

	arg:[6-7: act: <nop set sel jmp>]
		0	nop: leave the flags
		1	set: true or false
		2	sel: min/max
		3	jmp: goto
*/
/* opc_p4i argc = 1: [sat:1][uns:1][typ:2][opc:4]
	typ:
		00 :	u8[16]
		01 :	u16[8]
		10 :	u32[4]
		11 :	u64[2]
	uns:
		unsigned
	sat:
		saturate

	opc:
		neg = 0x00,	sat: cmt/neg
		add = 0x01,	sat: saturate
		sub = 0x02,	sat: saturate
		mul = 0x03,	sat: hi / lo
		div = 0x04,	sat: quo/rem
		mod = 0x05,	// avg
		not = 0x06,	???
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
