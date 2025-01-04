/* #define TOKEN_DEF(Name, Type, Args, Text) ...
	NAME: enumeration name
	TYPE: operator priority (if != 0 then operator)
	ARGS: number of operands (if != 0 then operator)
	TEXT: text representation of the token
*/
TOKEN_DEF(TOKEN_any, 0x00, 0, NULL, ".error")
TOKEN_DEF(TOKEN_val, 0x00, 0, NULL, ".literal")
TOKEN_DEF(TOKEN_var, 0x00, 0, NULL, ".variable")
//~ Operators ==================================================================
TOKEN_DEF(OPER_fnc, 0x0f, 2, NULL, "()")                  // a(x): function call, cast, emit, ctor = <typename>(args...), dtor ?= void(<typename> &&variable), ...
TOKEN_DEF(OPER_idx, 0x0f, 2, NULL, "[]")                  // a[i]: index
TOKEN_DEF(OPER_dot, 0x0f, 2, NULL, ".")                   // a.b:  member

TOKEN_DEF(PNCT_dot3, 0x1e, 1, NULL, "...")                // ...a, triple dot
TOKEN_DEF(OPER_adr, 0x1e, 1, NULL, "&")                   // & a:  address of
TOKEN_DEF(OPER_pls, 0x1e, 1, "operator+", "+")            // + a:  unary plus
TOKEN_DEF(OPER_mns, 0x1e, 1, "operator-", "-")            // - a:  unary minus
TOKEN_DEF(OPER_cmt, 0x1e, 1, "operator~", "~")            // ~ a:  complement / ?reciprocal
TOKEN_DEF(OPER_not, 0x1e, 1, "operator!", "!")            // ! a:  logical not

TOKEN_DEF(OPER_mul, 0x0d, 2, "operator*", "*")            // a * b
TOKEN_DEF(OPER_div, 0x0d, 2, "operator/", "/")            // a / b
TOKEN_DEF(OPER_mod, 0x0d, 2, "operator%", "%")            // a % b

TOKEN_DEF(OPER_add, 0x0c, 2, "operator+", "+")            // a + b
TOKEN_DEF(OPER_sub, 0x0c, 2, "operator-", "-")            // a - b

TOKEN_DEF(OPER_shr, 0x0b, 2, "operator>>", ">>")           // a >> b
TOKEN_DEF(OPER_shl, 0x0b, 2, "operator<<", "<<")           // a << b

TOKEN_DEF(OPER_cgt, 0x0a, 2, "operator>", ">")            // a > b
TOKEN_DEF(OPER_cge, 0x0a, 2, "operator>=", ">=")          // a >= b
TOKEN_DEF(OPER_clt, 0x0a, 2, "operator<", "<")            // a < b
TOKEN_DEF(OPER_cle, 0x0a, 2, "operator<=", "<=")          // a <= b

TOKEN_DEF(OPER_ceq, 0x09, 2, "operator==", "==")           // a == b
TOKEN_DEF(OPER_cne, 0x09, 2, "operator!=", "!=")           // a != b

TOKEN_DEF(OPER_and, 0x08, 2, "operator&", "&")            // a & b
TOKEN_DEF(OPER_xor, 0x07, 2, "operator^", "^")            // a ^ b
TOKEN_DEF(OPER_ior, 0x06, 2, "operator|", "|")            // a | b

TOKEN_DEF(OPER_all, 0x05, 2, NULL, "&&")                  // a && b
TOKEN_DEF(OPER_any, 0x04, 2, NULL, "||")                  // a || b
TOKEN_DEF(OPER_sel, 0x13, 2, NULL, "?")                   // question mark: a ? b : c
TOKEN_DEF(OPER_cln, 0x13, 2, NULL, ":")                   // colon

TOKEN_DEF(INIT_set, 0x12, 2, NULL, ":=")                  // a := b
TOKEN_DEF(ASGN_set, 0x12, 2, NULL, ":=")                  // a := b
TOKEN_DEF(ASGN_mul, 0x12, 2, NULL, "*=")                  // a *= b
TOKEN_DEF(ASGN_div, 0x12, 2, NULL, "/=")                  // a /= b
TOKEN_DEF(ASGN_mod, 0x12, 2, NULL, "%=")                  // a %= b
TOKEN_DEF(ASGN_add, 0x12, 2, NULL, "+=")                  // a += b
TOKEN_DEF(ASGN_sub, 0x12, 2, NULL, "-=")                  // a -= b
TOKEN_DEF(ASGN_shl, 0x12, 2, NULL, "<<=")                 // a <<= b
TOKEN_DEF(ASGN_shr, 0x12, 2, NULL, ">>=")                 // a >>= b
TOKEN_DEF(ASGN_and, 0x12, 2, NULL, "&=")                  // a &= b
TOKEN_DEF(ASGN_xor, 0x12, 2, NULL, "^=")                  // a ^= b
TOKEN_DEF(ASGN_ior, 0x12, 2, NULL, "|=")                  // a |= b

TOKEN_DEF(OPER_com, 0x01, 2, NULL, ",")                   // a, b

//~ Statements =================================================================
//! keep STMT_beg the first and STMT_end the last statement token
TOKEN_DEF(STMT_beg, 0x00, 0, NULL, "{}")                  // block statement {...}
TOKEN_DEF(STMT_if,  0x00, 0, NULL, "if")                  // if then else
TOKEN_DEF(STMT_sif, 0x00, 0, NULL, "static if")           // compile time if
TOKEN_DEF(STMT_for, 0x00, 0, NULL, "for")                 // for, while, repeat
 TOKEN_DEF(STMT_sfor,0x00, 0, NULL, "static for")          // unroll loop compile time
TOKEN_DEF(STMT_con, 0x00, 0, NULL, "continue")            // continue statement
TOKEN_DEF(STMT_brk, 0x00, 0, NULL, "break")               // break statement
TOKEN_DEF(STMT_ret, 0x00, 0, NULL, "return")              // return statement
TOKEN_DEF(STMT_end, 0x00, 0, NULL, ";")                   // expression statement

//~ Keywords ===================================================================
TOKEN_DEF(ELSE_kwd, 0x00, 0, NULL, "else")
TOKEN_DEF(ENUM_kwd, 0x00, 0, NULL, "enum")
TOKEN_DEF(INLINE_kwd, 0x00, 0, NULL, "inline")
TOKEN_DEF(STATIC_kwd, 0x00, 0, NULL, "static")
TOKEN_DEF(RECORD_kwd, 0x00, 0, NULL, "struct")

//~ Other tokens ===============================================================
TOKEN_DEF(LEFT_par,  0x00, 0, NULL, "(")                  // Left parenthesis
TOKEN_DEF(RIGHT_par, 0x00, 0, NULL, ")")                  // Right parenthesis
TOKEN_DEF(LEFT_sqr,  0x00, 0, NULL, "[")                  // Left square bracket
TOKEN_DEF(RIGHT_sqr, 0x00, 0, NULL, "]")                  // Right square bracket
TOKEN_DEF(RIGHT_crl, 0x00, 0, NULL, "}")                  // Right curly bracket

TOKEN_DEF(TOKEN_doc, 0x00, 0, NULL, ".doc")               // doc comment
TOKEN_DEF(TOKEN_opc, 0x00, 0, NULL, ".opc")               // opcode

#undef TOKEN_DEF
