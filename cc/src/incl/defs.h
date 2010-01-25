#ifdef TOKDEF
//~ #define TOKDEF(NAME, TYPE, SIZE, KEYW, STR) {KIND, TYPE, SIZE, STR},
//~ TOKDEF(TOKN_err, 0xff, 0, NULL, ".err")
TOKDEF(TYPE_any, 0x00, 0,     NULL, ".err")	// error
TOKDEF(TYPE_vid, 0x01, 0,     NULL, ".vid")	// void
TOKDEF(TYPE_bit, 0x02, 0,     NULL, ".bit")	// bool, uns32, uns16, uns8
TOKDEF(TYPE_int, 0x03, 0,     NULL, ".con")	// int64, int32, int16, int8
TOKDEF(TYPE_flt, 0x04, 0,     NULL, ".con")	// flt64, flt32
TOKDEF(TYPE_p4x, 0x05, 0,     NULL, ".vec")	// (p2f64, p4f32), (p4f64, p8f32) (p2i64, p4i32, p8i16, p16i8), (p2u64, p4u32, p8u16, p16u8), ...
TOKDEF(TYPE_ar3, 0x06, 0,     NULL, ".arr")	// pointer, string, array, ..., ???
TOKDEF(TYPE_def, 0x07, 0, "define", ".def")	// struct, union, class?
TOKDEF(TYPE_enu, TYPE_def, 0, "enum", ".enu")
TOKDEF(TYPE_rec, TYPE_def, 0, "struct", ".rec") // unions := struct:0
//~ TOKDEF(TYPE_cls, TYPE_def, 0, "class", ".cls")

TOKDEF(CNST_str, 0x00, 0, NULL, ".str")	// TODO: replace with TYPE_str
TOKDEF(TYPE_ref, 0x00, 0, NULL, ".ref")		// variable/function

TOKDEF(QUAL_sta, 0x01, 0, "static", "static")
TOKDEF(QUAL_par, 0x02, 0, "parralel", "parralel")
//? TOKDEF(QUAL_con, 0x10,  0, "const", ".con")	// constant
//? TOKDEF(QUAL_syn, 0x04, 0, "synchronized", "synchronized")

//~ vmInterface ================================================================
TOKDEF(EMIT_opc, 0x00, 0, "emit", ".emit")		// opcodes in emit(opcode, args...)
TOKDEF(TYPE_u32, TYPE_bit, 32, NULL, ".u32")
//~ TOKDEF(TYPE_u64, TYPE_bit, 64, NULL, ".u64")		// no vm support
TOKDEF(TYPE_i32, TYPE_int, 32, NULL, ".i32")
TOKDEF(TYPE_i64, TYPE_int, 64, NULL, ".i64")
TOKDEF(TYPE_f32, TYPE_flt, 32, NULL, ".f32")
TOKDEF(TYPE_f64, TYPE_flt, 64, NULL, ".f64")
//~ TOKDEF(TYPE_p4f, TYPE_p4x, 128,NULL, ".v4f")
//~ TOKDEF(TYPE_p2d, TYPE_p4x, 128,NULL, ".v2d")

//~ Statements =================================================================
TOKDEF(OPER_nop, 0x00, 0, NULL, ".cmd")		// stmt: stmt
TOKDEF(STMT_beg, 0x00, 0, NULL, ".beg")		// stmt: list {...}
TOKDEF(STMT_for, 0x00, 0, "for", ".for")	// stmt: for, while, repeat
TOKDEF(STMT_if,  0x00, 0, "if", ".if")		// stmt: if then else | break | continue | goto
TOKDEF(STMT_els, 0x00, 0, "else", ".cmd")	// ????
//~ TOKDEF(STMT_ret, 0x00, 0, "return", ".cmd")	// stmt
TOKDEF(STMT_end, 0x00, 0, NULL, "}")		// destruct calls ?

//~ Operators ==================================================================
TOKDEF(OPER_idx, 0x0f, 2, NULL, ".idx")		// a[i]		index
TOKDEF(OPER_fnc, 0x0f, 2, NULL, ".fnc")		// a(x)		function call, cast, ctor, emit, ...
TOKDEF(OPER_dot, 0x0f, 2, NULL, ".dot")		// a.b		member

TOKDEF(OPER_pls, 0x1e, 1, NULL, ".pls")		// + a		unary plus
TOKDEF(OPER_mns, 0x1e, 1, NULL, ".neg")		// - a		unary minus
TOKDEF(OPER_cmt, 0x1e, 1, NULL, ".cmt")		// ~ a		complement / ?reciprocal
TOKDEF(OPER_not, 0x1e, 1, NULL, ".not")		// ! a		logical not

TOKDEF(OPER_mul, 0x0d, 2, NULL, ".mul")		// a * b
TOKDEF(OPER_div, 0x0d, 2, NULL, ".div")		// a / b
TOKDEF(OPER_mod, 0x0d, 2, NULL, ".mod")		// a % b

TOKDEF(OPER_add, 0x0c, 2, NULL, ".add")		// a + b
TOKDEF(OPER_sub, 0x0c, 2, NULL, ".sub")		// a - b

TOKDEF(OPER_shr, 0x0b, 2, NULL, ".shr")		// a >> b
TOKDEF(OPER_shl, 0x0b, 2, NULL, ".shl")		// a << b

TOKDEF(OPER_gte, 0x0a, 2, NULL, ".cgt")		// a > b
TOKDEF(OPER_geq, 0x0a, 2, NULL, ".cge")		// a >= b
TOKDEF(OPER_lte, 0x0a, 2, NULL, ".clt")		// a < b
TOKDEF(OPER_leq, 0x0a, 2, NULL, ".cle")		// a <= b
TOKDEF(OPER_equ, 0x09, 2, NULL, ".ceq")		// a == b
TOKDEF(OPER_neq, 0x09, 2, NULL, ".cne")		// a != b

TOKDEF(OPER_and, 0x08, 2, NULL, ".and")		// a & b
TOKDEF(OPER_xor, 0x07, 2, NULL, ".xor")		// a ^ b
TOKDEF(OPER_ior, 0x06, 2, NULL, ".ior")		// a | b

TOKDEF(OPER_lnd, 0x05, 2, NULL, "&&")		// a && b
TOKDEF(OPER_lor, 0x04, 2, NULL, "||")		// a || b
TOKDEF(OPER_sel, 0x13, 3, NULL, "?:")		// a ? b : c

TOKDEF(ASGN_set, 0x12, 2, NULL, ".set")		// a := b
TOKDEF(ASGN_mul, 0x12, 2, NULL, "*=")		// a *= b
TOKDEF(ASGN_div, 0x12, 2, NULL, "/=")		// a /= b
TOKDEF(ASGN_mod, 0x12, 2, NULL, "%=")		// a %= b
TOKDEF(ASGN_add, 0x12, 2, NULL, "+=")		// a += b
TOKDEF(ASGN_sub, 0x12, 2, NULL, "-=")		// a -= b
TOKDEF(ASGN_shl, 0x12, 2, NULL, "<<=")		// a <<= b
TOKDEF(ASGN_shr, 0x12, 2, NULL, ">>=")		// a >>= b
TOKDEF(ASGN_and, 0x12, 2, NULL, "&=")		// a &= b
TOKDEF(ASGN_xor, 0x12, 2, NULL, "^=")		// a ^= b
TOKDEF(ASGN_ior, 0x12, 2, NULL, "|=")		// a |= b

TOKDEF(OPER_com, 0x01, 2, NULL, ",")		// a, b

//~ temp =======================================================================

TOKDEF(PNCT_lc , 0x00, 0, NULL, "[par")		// curlies
TOKDEF(PNCT_rc , 0x00, 0, NULL, "]par")
TOKDEF(PNCT_lp , 0x00, 0, NULL, "(par")	// parentheses
TOKDEF(PNCT_rp , 0x00, 0, NULL, ")par")
TOKDEF(PNCT_qst, 0x00, 0, NULL, "?")		// question mark
TOKDEF(PNCT_cln, 0x00, 0, NULL, ":")		// colon

/*
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

// these are statements
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

