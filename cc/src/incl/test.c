typedef enum {		// swizzle
	xxxx = 0x00, yxxx = 0x01, zxxx = 0x02, wxxx = 0x03, xyxx = 0x04, yyxx = 0x05, zyxx = 0x06, wyxx = 0x07, 
	xzxx = 0x08, yzxx = 0x09, zzxx = 0x0a, wzxx = 0x0b, xwxx = 0x0c, ywxx = 0x0d, zwxx = 0x0e, wwxx = 0x0f, 
	xxyx = 0x10, yxyx = 0x11, zxyx = 0x12, wxyx = 0x13, xyyx = 0x14, yyyx = 0x15, zyyx = 0x16, wyyx = 0x17, 
	xzyx = 0x18, yzyx = 0x19, zzyx = 0x1a, wzyx = 0x1b, xwyx = 0x1c, ywyx = 0x1d, zwyx = 0x1e, wwyx = 0x1f, 
	xxzx = 0x20, yxzx = 0x21, zxzx = 0x22, wxzx = 0x23, xyzx = 0x24, yyzx = 0x25, zyzx = 0x26, wyzx = 0x27, 
	xzzx = 0x28, yzzx = 0x29, zzzx = 0x2a, wzzx = 0x2b, xwzx = 0x2c, ywzx = 0x2d, zwzx = 0x2e, wwzx = 0x2f, 
	xxwx = 0x30, yxwx = 0x31, zxwx = 0x32, wxwx = 0x33, xywx = 0x34, yywx = 0x35, zywx = 0x36, wywx = 0x37, 
	xzwx = 0x38, yzwx = 0x39, zzwx = 0x3a, wzwx = 0x3b, xwwx = 0x3c, ywwx = 0x3d, zwwx = 0x3e, wwwx = 0x3f, 
	xxxy = 0x40, yxxy = 0x41, zxxy = 0x42, wxxy = 0x43, xyxy = 0x44, yyxy = 0x45, zyxy = 0x46, wyxy = 0x47, 
	xzxy = 0x48, yzxy = 0x49, zzxy = 0x4a, wzxy = 0x4b, xwxy = 0x4c, ywxy = 0x4d, zwxy = 0x4e, wwxy = 0x4f, 
	xxyy = 0x50, yxyy = 0x51, zxyy = 0x52, wxyy = 0x53, xyyy = 0x54, yyyy = 0x55, zyyy = 0x56, wyyy = 0x57, 
	xzyy = 0x58, yzyy = 0x59, zzyy = 0x5a, wzyy = 0x5b, xwyy = 0x5c, ywyy = 0x5d, zwyy = 0x5e, wwyy = 0x5f, 
	xxzy = 0x60, yxzy = 0x61, zxzy = 0x62, wxzy = 0x63, xyzy = 0x64, yyzy = 0x65, zyzy = 0x66, wyzy = 0x67, 
	xzzy = 0x68, yzzy = 0x69, zzzy = 0x6a, wzzy = 0x6b, xwzy = 0x6c, ywzy = 0x6d, zwzy = 0x6e, wwzy = 0x6f, 
	xxwy = 0x70, yxwy = 0x71, zxwy = 0x72, wxwy = 0x73, xywy = 0x74, yywy = 0x75, zywy = 0x76, wywy = 0x77, 
	xzwy = 0x78, yzwy = 0x79, zzwy = 0x7a, wzwy = 0x7b, xwwy = 0x7c, ywwy = 0x7d, zwwy = 0x7e, wwwy = 0x7f, 
	xxxz = 0x80, yxxz = 0x81, zxxz = 0x82, wxxz = 0x83, xyxz = 0x84, yyxz = 0x85, zyxz = 0x86, wyxz = 0x87, 
	xzxz = 0x88, yzxz = 0x89, zzxz = 0x8a, wzxz = 0x8b, xwxz = 0x8c, ywxz = 0x8d, zwxz = 0x8e, wwxz = 0x8f, 
	xxyz = 0x90, yxyz = 0x91, zxyz = 0x92, wxyz = 0x93, xyyz = 0x94, yyyz = 0x95, zyyz = 0x96, wyyz = 0x97, 
	xzyz = 0x98, yzyz = 0x99, zzyz = 0x9a, wzyz = 0x9b, xwyz = 0x9c, ywyz = 0x9d, zwyz = 0x9e, wwyz = 0x9f, 
	xxzz = 0xa0, yxzz = 0xa1, zxzz = 0xa2, wxzz = 0xa3, xyzz = 0xa4, yyzz = 0xa5, zyzz = 0xa6, wyzz = 0xa7, 
	xzzz = 0xa8, yzzz = 0xa9, zzzz = 0xaa, wzzz = 0xab, xwzz = 0xac, ywzz = 0xad, zwzz = 0xae, wwzz = 0xaf, 
	xxwz = 0xb0, yxwz = 0xb1, zxwz = 0xb2, wxwz = 0xb3, xywz = 0xb4, yywz = 0xb5, zywz = 0xb6, wywz = 0xb7, 
	xzwz = 0xb8, yzwz = 0xb9, zzwz = 0xba, wzwz = 0xbb, xwwz = 0xbc, ywwz = 0xbd, zwwz = 0xbe, wwwz = 0xbf, 
	xxxw = 0xc0, yxxw = 0xc1, zxxw = 0xc2, wxxw = 0xc3, xyxw = 0xc4, yyxw = 0xc5, zyxw = 0xc6, wyxw = 0xc7, 
	xzxw = 0xc8, yzxw = 0xc9, zzxw = 0xca, wzxw = 0xcb, xwxw = 0xcc, ywxw = 0xcd, zwxw = 0xce, wwxw = 0xcf, 
	xxyw = 0xd0, yxyw = 0xd1, zxyw = 0xd2, wxyw = 0xd3, xyyw = 0xd4, yyyw = 0xd5, zyyw = 0xd6, wyyw = 0xd7, 
	xzyw = 0xd8, yzyw = 0xd9, zzyw = 0xda, wzyw = 0xdb, xwyw = 0xdc, ywyw = 0xdd, zwyw = 0xde, wwyw = 0xdf, 
	xxzw = 0xe0, yxzw = 0xe1, zxzw = 0xe2, wxzw = 0xe3, xyzw = 0xe4, yyzw = 0xe5, zyzw = 0xe6, wyzw = 0xe7, 
	xzzw = 0xe8, yzzw = 0xe9, zzzw = 0xea, wzzw = 0xeb, xwzw = 0xec, ywzw = 0xed, zwzw = 0xee, wwzw = 0xef, 
	xxww = 0xf0, yxww = 0xf1, zxww = 0xf2, wxww = 0xf3, xyww = 0xf4, yyww = 0xf5, zyww = 0xf6, wyww = 0xf7, 
	xzww = 0xf8, yzww = 0xf9, zzww = 0xfa, wzww = 0xfb, xwww = 0xfc, ywww = 0xfd, zwww = 0xfe, wwww = 0xff
} vec_swz;

int cpldblmul(state s) {
	int c0, c1, c2;
	cc_init(s);
	cc_buff(s, 0, __FILE__, __LINE__,
		//~ "flt64 r, i;"		// set c0 here
		"flt64 re = 3, im = 4;"		// -14, 23
		"flt64 r2 = 2, i2 = 5;"
#ifdef USE_COMPILED_CODE
		"flt64 r1 = re, i1 = im;"
		"re = r1 * r2 - i1 * i2;"
		"im = i1 * r2 + r1 * i2;"
#endif
	);
	vm_cgen(s);
	c0 = (c1 = (c2 = s->code.rets) - 4) - 4*0;
#ifndef USE_COMPILED_CODE
	emit(s, opc_dup4, argidx(s, c1));
	emit(s, opc_dup4, argidx(s, c2));
	emit(s, p4d_mul, noarg);
	emit(s, f64_sub, noarg);

	// we have one double on stack
	emit(s, opc_dup4, argidx(s, c1));
	emit(s, opc_dup4, argidx(s, c2));
	emit(s, p4d_swz, argi(zwxy));
	emit(s, p4d_mul, noarg);
	emit(s, f64_add, noarg);

	// we have two double on stack
	emit(s, opc_set4, argidx(s, c0));
	//~ emit(s, opc_pop, argidx(s, c0));		// pop until c0
#endif

	printf("Stack max = %d / %d slots\n", s->code.mins * 4, s->code.mins);
	printf("Code size = %d / %d instructions\n", s->code.cs, s->code.ic);
	vm_exec(s, cc, ss, dl);
	vm_dasm(s, NULL);
	//~ cc_tags(s, NULL);
	vm_tags(s);
	return cc_done(s);
}

int cplfltmul(state s) {
	int c0, c1, c2;
	cc_init(s);
	cc_buff(s, 0, __FILE__, __LINE__,
		"flt32 im, re;\n"						// -14 + 23i
		"flt32 i1 = 4, r1 = 3;"
		"flt32 i2 = 5, r2 = 2;"
#ifdef USE_COMPILED_CODE
		"re = r1 * r2 - i1 * i2;"	//~ Stack max = 36 / 9 slots
		"im = i1 * r2 + r1 * i2;"	//~ Code size = 48 / 21 instructions
#endif
	);
	vm_cgen(s);
	c0 = (c1 = (c2 = s->code.rets) - 2) - 2;
#ifndef USE_COMPILED_CODE
	//~ Complex.mul
	emit(s, opc_dup4, argidx(s, c2));
	emit(s, p4d_swz, argi(yyxx));
	emit(s, opc_dup4, argidx(s, c2));
	emit(s, p4d_swz, argi(zwwz));

	emit(s, p4f_mul, noarg);
	emit(s, f32_neg, noarg);
	emit(s, opc_dup4, argi(0));
	emit(s, p4d_swz, argi(zwxy));
	emit(s, p4f_add, noarg);

	emit(s, opc_set2, argidx(s, c0));
	emit(s, opc_pop, argi(2));
#endif

	printf("Stack max = %d / %d slots\n", s->code.mins * 4, s->code.mins);
	printf("Code size = %d / %d instructions\n", s->code.cs, s->code.ic);
	vm_exec(s, cc, ss, dl);
	vm_dasm(s, NULL);
	vm_tags(s);
	return cc_done(s);
}

int swz_test(state s) {
	cc_init(s);
	cc_buff(s, 0, __FILE__, __LINE__ + 1,
		"flt32 w = 4, z = 3, y = 2, x = 1;\n"
		"z = emit(add.i32, int32(w), int32(x));\n"
	);
	vm_cgen(s);
	//~ emit(s, opc_dup1, argi(0));
	//~ emit(s, opc_dup1, argi(0));
	//~ emit(s, opc_dup1, argi(0));
	//~ emit(s, p4d_swz, argi(zwxy));
	//~ emit(s, p4f_swz, argi(zwxy));
	//~ * /

	printf("Stack max = %d / %d slots\n", s->code.mins * 4, s->code.mins);
	printf("Code size = %d / %d instructions\n", s->code.cs, s->code.ic);
	vm_exec(s, cc, ss, dl);
	vm_dasm(s, NULL);
	//~ cc_tags(s, NULL);
	vm_tags(s);
	return cc_done(s);
}

//} */
