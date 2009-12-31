#ifdef TOKDEF
TOKDEF(TYPE_any, 0x00, 0, TY, "TYPE_any")	// error
TOKDEF(TYPE_vid, 0x01, 0, TY, "TYPE_vid")	// void
TOKDEF(TYPE_bit, 0x02, 0, TY, "TYPE_bit")	// bool, uns32, uns16, uns8
TOKDEF(TYPE_int, 0x03, 0, TY, "TYPE_int")	// int64, int32, int16, int8
TOKDEF(TYPE_flt, 0x04, 0, TY, "TYPE_flt")	// flt64, flt32
TOKDEF(TYPE_p4x, 0x05, 0, TY, "TYPE_vec")	// (p2f64, p4f32), (p4f64, p8f32) (p2i64, p4i32, p8i16, p16i8), (p2u64, p4u32, p8u16, p16u8), ...
TOKDEF(TYPE_ar3, 0x06, 0, TY, "TYPE_arr")	// pointer, string, array, ..., ???
TOKDEF(TYPE_def, 0x07, 0, ID, "define")		// struct, union, class?

TOKDEF(TYPE_u32, TYPE_bit, 32, TY, "u32")
//~ TOKDEF(TYPE_u64, TYPE_bit, 64, TY, "u64")		// no vm support
TOKDEF(TYPE_i32, TYPE_int, 32, TY, "i32")
TOKDEF(TYPE_i64, TYPE_int, 64, TY, "i64")
TOKDEF(TYPE_f32, TYPE_flt, 32, TY, "f32")
TOKDEF(TYPE_f64, TYPE_flt, 64, TY, "f64")
//~ TOKDEF(TYPE_p4f, TYPE_p4x, 128, TY, "v4f")
//~ TOKDEF(TYPE_p2d, TYPE_p4x, 128, TY, "v2d")

TOKDEF(TYPE_enu, TYPE_def, 0, ID, "enum")
TOKDEF(TYPE_rec, TYPE_def, 0, ID, "struct")
//~ TOKDEF(TYPE_uni, TYPE_def, 0, ID, "union")	// use: 'class pack 0; size 32'
//~ TOKDEF(TYPE_cls, TYPE_def, 0, ID, "class")

//~ TOKDEF(CNST_chr, 0x02, 0, TY, "CNST_uns")	// 'a'
//~ TOKDEF(CNST_uns, 0x02, 0, TY, "CNST_uns")	// uns32
TOKDEF(CNST_int, 0x00, 0, TY, "CNST_int")	// int??
TOKDEF(CNST_flt, 0x00, 0, TY, "CNST_flt")	// float
TOKDEF(CNST_str, 0x00, 0, TY, "CNST_str")	// string
TOKDEF(TYPE_ref, 0x00, 0, TY, "TYPE_ref")	// function/variable

TOKDEF(EMIT_opc, 0x00, 0, ID, "emit")		// opcodes in emit(opcode, args...)
TOKDEF(QUAL_sta, 0x01, 0, ID, "static")
TOKDEF(QUAL_par, 0x02, 0, ID, "parralel")
//~ TOKDEF(QUAL_syn, 0x04, 0, ID, "synchronized")

//~ Statements =================================================================
TOKDEF(OPER_nop, 0x00, 0, XX, ";")		// stmt: expr
TOKDEF(STMT_beg, 0x00, 0, XX, "{")		// stmt: list {...}
TOKDEF(STMT_for, 0x00, 0, ID, "for")	// stmt: for, while, repeat
TOKDEF(STMT_if,  0x00, 0, ID, "if")		// stmt: if then else | break | continue | goto
TOKDEF(STMT_els, 0x00, 0, ID, "else")	// ????
TOKDEF(STMT_ret, 0x00, 0, ID, "return")	// stmt
TOKDEF(STMT_end, 0x00, 0, XX, "}")		// destruction calls

//~ Operators ==================================================================
TOKDEF(OPER_com, 0x01, 2, OP, ",")		// a, b

TOKDEF(ASGN_set, 0x12, 2, OP, ".set")		// a := b
TOKDEF(ASGN_mul, 0x12, 2, OP, "*=")		// a *= b
TOKDEF(ASGN_div, 0x12, 2, OP, "/=")		// a /= b
TOKDEF(ASGN_mod, 0x12, 2, OP, "%=")		// a %= b
TOKDEF(ASGN_add, 0x12, 2, OP, "+=")		// a += b
TOKDEF(ASGN_sub, 0x12, 2, OP, "-=")		// a -= b
TOKDEF(ASGN_shl, 0x12, 2, OP, "<<=")	// a <<= b
TOKDEF(ASGN_shr, 0x12, 2, OP, ">>=")	// a >>= b
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
TOKDEF(TOKN_err, 0xff, 0, XX, "?err?")

/*

//~ TOKDEF(QUAL_con, 0x10,  0, ID, "const")	// constant
//~ final = constant + volatile,		// final

//~ Operators ==================================================================
TOKDEF(OPER_min, 0x09, 2, OP, "<?")		// a <? b
TOKDEF(OPER_max, 0x09, 2, OP, ">?")		// a >? b
TOKDEF(OPER_pow, 0x0a, 2, OP, ">")		// a ** b
TOKDEF(ASGN_min, 0x09, 2, OP, "<?")		// a <?= b
TOKDEF(ASGN_max, 0x09, 2, OP, ">?")		// a >?= b
TOKDEF(ASGN_pow, 0x0a, 2, OP, ">")		// a **= b

//{ ============================================================================
//~ TOKDEF(OPER_new, 0x00, 2, 2, "new")
//~ TOKDEF(OPER_del, 0x00, 0, 2, "delete")

//? TOKDEF(OPER_try, 0x00, 0, 2, "try")		//
//? TOKDEF(OPER_thr, 0x00, 0, 2, "throw")		//
//? TOKDEF(OPER_cth, 0x00, 0, 2, "catch")		//
//? TOKDEF(OPER_fin, 0x00, 0, 2, "finally")		//

//~ TOKDEF(OPER_go2, 0x00, 0, 2, "goto")
//~ TOKDEF(OPER_brk, 0x00, 0, 2, "break")
//~ TOKDEF(OPER_con, 0x00, 0, 2, "continue")

//~ TOKDEF(OPER_swi, 0x00, 0, 2, "switch")
//~ TOKDEF(OPER_cas, 0x00, 0, 2, "case")
//? TOKDEF(OPER_def, 0x00, 0, 2, "default/else")

//~ TOKDEF(OPER_wht, 0x00, 0, 2, "while")		// while '(' <expr>')' <stmt>
//~ TOKDEF(OPER_whf, 0x00, 0, 2, "until")		// until '(' <expr>')' <stmt>
//~ TOKDEF(OPER_rep, 0x00, 0, 2, "repeat")		// repeat <stmt> ((until '(' <expr> ')' ) | (while '(' <expr> ')' ))?

//} */

#undef TOKDEF
#endif

