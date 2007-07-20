
tok_msk = 0x0000f000,		// token type mask

tok_und = 0x00000000,		// undefined
tok_err = 0x0000e000,		// error on token
tok_sym = 0x00004000,		// symbols
tok_val = 0x00003000,		// Constant (int, float, cstr)
tok_opr = 0x00002000,		// Operator		// +, -, ...
tok_idf = 0x00001000,		// identifier (type, name, keyw)


//~ ============================================================================
//~ sym_msk = 0x000000ff | tok_sym,		// 
sym_spc = 0x00000000 | tok_sym,		// white space: '\n', '\t', ' '
sym_lbs = 0x00000001 | tok_sym,		// '('
sym_rbs = 0x00000002 | tok_sym,		// ')'
sym_lbi = 0x00000003 | tok_sym,		// '['
sym_rbi = 0x00000004 | tok_sym,		// ']'
sym_lbb = 0x00000005 | tok_sym,		// '{'
sym_rbb = 0x00000006 | tok_sym,		// '}'
sym_dot = 0x00000007 | tok_sym,		// '.'
sym_qtm = 0x00000008 | tok_sym,		// '?'	question mark
sym_col = 0x00000009 | tok_sym,		// ':'	colon
sym_sem = 0x0000000a | tok_sym,		// ';'	semicolon
sym_com = 0x0000000b | tok_sym,		// ','	comma
sym_und = 0x0000000f | tok_sym,		// '`', '@', '#', '$', '\', ...

//~ ============================================================================

idf_msk = 0x00000f00 | tok_idf,			// identifyer mask -> idf_kwd, idf_dcl, idf_ref(var)
//~ idf_def = 0x00000000 | tok_idf,			// definition
idf_kwd = 0x00000100 | tok_idf,			// keyword (if, for, break, ...)
idf_dcl = 0x00000200 | tok_idf,			// type (int, float, ...)
idf_ref = 0x00000400 | tok_idf,			// name
//~ idf_fnc = 0x00000500 | tok_idf,			// function

idm_msk = 0x000000ff | tok_idf,			// id modifyer mask

idr_mem = 0x00000006,			// reference in memory

idm_con = 0x00000001 | tok_idf,			// const
idm_sta = 0x00000002 | tok_idf,			// static
idm_vol = 0x00000004 | tok_idf,			// volatile
//~ idm_use = 0x00000008 | tok_idf,			// always add to data
//~ idm_vrd = 0x00000010 | tok_idf,			// was readed
//~ idm_vrd = 0x00000020 | tok_idf,			// volatile

//~ ============================================================================

opm_pri = 0x000000f0,				// operator priority mask
opf_rla = 0x00000001,				// rigth to left associativity flag
otf_unf = 0x00000002,				// unary operator flag
//~ ============================================================================
opr_nop = 0x00000000 | tok_opr,		// []	index
opr_jmp = 0x00000001 | tok_opr,		// goto

opr_idx = 0x000004f0 | tok_opr,		// []	index
opr_fnc = 0x000003f2 | tok_opr,		// ()	function call, typecast, sizeof, ctor.
opr_tpc = 0x000002f2 | tok_opr,		// ()	typecast
opr_dot = 0x000001f0 | tok_opr,		// .	member

opr_unm = 0x000004e3 | tok_opr,		// -						 : <=
opr_unp = 0x000003e3 | tok_opr,		// +
opr_not = 0x000002e3 | tok_opr,		// ~
opr_neg = 0x00000fe3 | tok_opr,		// !

opr_mul = 0x000003d0 | tok_opr,		// *
opr_div = 0x000002d0 | tok_opr,		// /
opr_mod = 0x000001d0 | tok_opr,		// %

opr_add = 0x000002c0 | tok_opr,		// +
opr_sub = 0x000001c0 | tok_opr,		// -

opr_shr = 0x000002b0 | tok_opr,		// >>
opr_shl = 0x000001b0 | tok_opr,		// <<

opr_gte = 0x000004a0 | tok_opr,		// >
opr_geq = 0x000003a0 | tok_opr,		// >=
opr_lte = 0x000002a0 | tok_opr,		// <
opr_leq = 0x000001a0 | tok_opr,		// <=

opr_neq = 0x00000290 | tok_opr,		// !=
opr_equ = 0x00000190 | tok_opr,		// ==

opr_and = 0x00000080 | tok_opr,		// &
opr_xor = 0x00000070 | tok_opr,		// ^
opr_or  = 0x00000060 | tok_opr,		// |

opr_lnd = 0x00000050 | tok_opr,		// &&
opr_lor = 0x00000040 | tok_opr,		// ||

opr_ite = 0x00000231 | tok_opr,		// ? If ? Then : Else		 : <=
opr_els = 0x00000131 | tok_opr,		// : If ? Then : Else

opr_ass = 0x00000021 | tok_opr,		// "=" assigment			 : <=
ass_equ = 0x00000b00 | opr_ass,		// =
ass_mul = 0x00000a00 | opr_ass,		// *=
ass_div = 0x00000900 | opr_ass,		// /=
ass_mod = 0x00000800 | opr_ass,		// %=
ass_add = 0x00000700 | opr_ass,		// +=
ass_sub = 0x00000600 | opr_ass,		// -=
ass_shr = 0x00000500 | opr_ass,		// >>=
ass_shl = 0x00000400 | opr_ass,		// <<=
ass_and = 0x00000300 | opr_ass,		// &=
ass_xor = 0x00000200 | opr_ass,		// ^=
ass_or  = 0x00000100 | opr_ass,		// |=

opr_com = 0x00000010 | tok_opr,		// "," comma

//~ ============================================================================

//~ ============================================================================
//~ opr_inc = 0x00000002 | tok_opr,		// ++
//~ opr_dec = 0x00000003 | tok_opr,		// --
//~ opr_poi = 0x000000f5 | tok_opr,		// ++	post increment
//~ opr_pod = 0x000000f4 | tok_opr,		// ++	post decrement
//~ opr_pri = 0x000001e5 | tok_opr,		// ++	pre increment
//~ opr_prd = 0x000001e4 | tok_opr,		// ++	pre decrement
//~ opr_sof = 0x000000e0 | tok_opr,		// sizeof(a)
//~ opr_sof = 0x000000e0 | tok_opr,		// typeof(a)			???
//~ opr_adr = 0x000000e3 | tok_opr,		// addrof(a) X&a		???
//~ opr_tpc = 0x000000e1 | tok_opr,		// typecast : type(a)
//~ opr_ptr = 0x000000ea | tok_opr,		// *a

//~ ============================================================================
//~ idk_msk = 0x0000000f | tok_idf,			// id keyword mask
//~ kwd_if  = 0x00000005 | idf_kwd,			// if
//~ kwd_for = 0x00000004 | idf_kwd,			// for
//~ kwd_ret = 0x00000003 | idf_kwd,			// ret
//~ kwd_brk = 0x00000002 | idf_kwd,			// break
//~ kwd_cnt = 0x00000001 | idf_kwd,			// continue
//~ kwd_rep = 0x00000001 | idf_kwd,			// repeat
//~ kwd_whi = 0x00000001 | idf_kwd,			// while
//~ kwd_unt = 0x00000001 | idf_kwd,			// until

//~ def_inl
