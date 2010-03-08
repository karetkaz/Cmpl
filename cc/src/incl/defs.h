#ifdef TOKDEF
//~ #define TOKDEF(NAME, TYPE, SIZE, KEYW, STR) {KIND, TYPE, SIZE, STR},
TOKDEF(TYPE_any, 0x00, 0, ".err")	// error
TOKDEF(TYPE_vid, 0x00, 0, ".vid")	// void
TOKDEF(TYPE_bit, 0x00, 0, ".bit")	// bool, uns32, uns16, uns8
TOKDEF(TYPE_int, 0x00, 0, ".int")	// int64, int32, int16, int8
TOKDEF(TYPE_flt, 0x00, 0, ".flt")	// flt64, flt32
TOKDEF(TYPE_arr, 0x00, 0, ".arr")	// pointer, string, array, ..., ???
TOKDEF(TYPE_def, 0xff, 0, "define")	// struct, union, class?
TOKDEF(TYPE_enu, 0xff, 0, "enum")
TOKDEF(TYPE_rec, 0xff, 0, "struct") // union := struct:0
//~ TOKDEF(TYPE_cls, 0xff, 0, "class")

TOKDEF(CNST_str, 0x00, 0, ".str")	// TODO: replace with TYPE_str
TOKDEF(TYPE_ref, 0x00, 0, ".ref")		// variable/function

TOKDEF(QUAL_sta, 0xff, 0, "static")
TOKDEF(QUAL_par, 0xff, 0, "parralel")
//~ TOKDEF(QUAL_con, 0xff, 0, "const")	// constant
//~ TOKDEF(QUAL_syn, 0xff, 0, "synchronized")

//~ vmInterface ================================================================
TOKDEF(EMIT_opc, 0xff,   0, "emit")		// opcodes in emit(opcode, args...)
TOKDEF(TYPE_u32, 0x00,  32, ".u32")
//~ TOKDEF(TYPE_u64, 0x00,  64, ".u64")		// no vm support
TOKDEF(TYPE_i32, 0x00,  32, ".i32")
TOKDEF(TYPE_i64, 0x00,  64, ".i64")
TOKDEF(TYPE_f32, 0x00,  32, ".f32")
TOKDEF(TYPE_f64, 0x00,  64, ".f64")
//~ TOKDEF(TYPE_p4x, 0x00, 128, ".p4x")

//~ Statements =================================================================
TOKDEF(STMT_for, 0xff, 0, "for")	// stmt: for, while, repeat
TOKDEF(STMT_if,  0xff, 0, "if")		// stmt: if then else | break | continue | goto
TOKDEF(STMT_els, 0xff, 0, "else")	// ????
//~ TOKDEF(STMT_ret, 0xff, 0, "return")	// stmt
TOKDEF(STMT_end, 0x00, 0, ".end")		// destruct calls ?
TOKDEF(OPER_nop, 0x00, 0, ".cmd")		// stmt: stmt
TOKDEF(STMT_beg, 0x00, 0, ".beg")		// stmt: list {...}

//~ Operators ==================================================================
TOKDEF(OPER_idx, 0x0f, 2, ".idx")		// a[i]		index
TOKDEF(OPER_fnc, 0x0f, 2, ".fnc")		// a(x)		function call, cast, ctor, emit, ...
TOKDEF(OPER_dot, 0x0f, 2, ".dot")		// a.b		member

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

TOKDEF(OPER_lnd, 0x05, 2, "&&")		// a && b
TOKDEF(OPER_lor, 0x04, 2, "||")		// a || b
TOKDEF(OPER_sel, 0x13, 3, "?:")		// a ? b : c

TOKDEF(ASGN_set, 0x12, 2, ":=")		// a := b
TOKDEF(ASGN_mul, 0x12, 2, "*=")		// a *= b
TOKDEF(ASGN_div, 0x12, 2, "/=")		// a /= b
TOKDEF(ASGN_mod, 0x12, 2, "%=")		// a %= b
TOKDEF(ASGN_add, 0x12, 2, "+=")		// a += b
TOKDEF(ASGN_sub, 0x12, 2, "-=")		// a -= b
TOKDEF(ASGN_shl, 0x12, 2, "<<=")	// a <<= b
TOKDEF(ASGN_shr, 0x12, 2, ">>=")	// a >>= b
TOKDEF(ASGN_and, 0x12, 2, "&=")		// a &= b
TOKDEF(ASGN_xor, 0x12, 2, "^=")		// a ^= b
TOKDEF(ASGN_ior, 0x12, 2, "|=")		// a |= b

TOKDEF(OPER_com, 0x01, 2, ",")		// a, b

//~ temp =======================================================================

TOKDEF(PNCT_lc , 0x00, 0, "[par")		// curlies
TOKDEF(PNCT_rc , 0x00, 0, "]par")
TOKDEF(PNCT_lp , 0x00, 0, "(par")	// parentheses
TOKDEF(PNCT_rp , 0x00, 0, ")par")
TOKDEF(PNCT_qst, 0x00, 0, "?")		// question mark
TOKDEF(PNCT_cln, 0x00, 0, ":")		// colon

/*
//~ Operators ==================================================================
TOKDEF(OPER_min, 0x09, 2, OP, "<?")		// a <? b
TOKDEF(OPER_max, 0x09, 2, OP, ">?")		// a >? b
TOKDEF(OPER_pow, 0x0a, 2, OP, ">")		// a ** b
TOKDEF(ASGN_min, 0x09, 2, OP, "<?")		// a <?= b
TOKDEF(ASGN_max, 0x09, 2, OP, ">?")		// a >?= b
TOKDEF(ASGN_pow, 0x0a, 2, OP, ">")		// a **= b

//{ ============================================================================
//~ TOKDEF(OPER_new, 0xff, 2, 2, "new")
//~ TOKDEF(OPER_del, 0xff, 0, 2, "delete")

// these are statements
//? TOKDEF(OPER_try, 0xff, 0, 2, "try")		//
//? TOKDEF(OPER_thr, 0xff, 0, 2, "throw")		//
//? TOKDEF(OPER_cth, 0xff, 0, 2, "catch")		//
//? TOKDEF(OPER_fin, 0xff, 0, 2, "finally")		//

//~ TOKDEF(OPER_go2, 0xff, 0, 2, "goto")
//~ TOKDEF(OPER_brk, 0xff, 0, 2, "break")
//~ TOKDEF(OPER_con, 0xff, 0, 2, "continue")

//~ TOKDEF(OPER_swi, 0xff, 0, 2, "switch")
//~ TOKDEF(OPER_cas, 0xff, 0, 2, "case")
//? TOKDEF(OPER_def, 0xff, 0, 2, "default")		// use else

//~ TOKDEF(OPER_wht, 0xff, 0, 2, "while")		// while '(' <expr>')' <stmt>
//~ TOKDEF(OPER_whf, 0xff, 0, 2, "until")		// until '(' <expr>')' <stmt>
//~ TOKDEF(OPER_rep, 0xff, 0, 2, "repeat")		// repeat <stmt> ((until '(' <expr> ')' ) | (while '(' <expr> ')' ))?

//} */

#undef TOKDEF
#endif

