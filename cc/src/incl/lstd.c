install(c, -1, "int32 .add(int32 a, uns32 b) {return emit(add.i32, i32(a), i32(b));}", 0);
install(c, -1, "int64 .add(int32 a, uns64 b) {return emit(add.i64, i64(a), i64(b));}", 0);
install(c, -1, "int32 .add(int32 a, int32 b) {return emit(add.i32, i32(a), i32(b));}", 0);
install(c, -1, "int64 .add(int32 a, int64 b) {return emit(add.i64, i64(a), i64(b));}", 0);
install(c, -1, "flt32 .add(int32 a, flt32 b) {return emit(add.f32, f32(a), f32(b));}", 0);
install(c, -1, "flt64 .add(int32 a, flt64 b) {return emit(add.f64, f64(a), f64(b));}", 0);
install(c, -1, "int32 .add(out int32 a, uns32 b) {return a = add(a, b)}", 0);
install(c, -1, "int64 .add(out int32 a, uns64 b) {return a = add(a, b)}", 0);
install(c, -1, "int32 .add(out int32 a, int32 b) {return a = add(a, b)}", 0);
install(c, -1, "int64 .add(out int32 a, int64 b) {return a = add(a, b)}", 0);
install(c, -1, "flt32 .add(out int32 a, flt32 b) {return a = add(a, b)}", 0);
install(c, -1, "flt64 .add(out int32 a, flt64 b) {return a = add(a, b)}", 0);

install(c, -1, "int32 add(int32 a, uns32 b) {return emit(i32.add, i32(a), i32(b));}", 0);	// +
install(c, -1, "int32 sub(int32 a, uns32 b) {return emit(i32.sub, i32(a), i32(b));}", 0);	// -
install(c, -1, "int32 mul(int32 a, uns32 b) {return emit(i32.mul, i32(a), i32(b));}", 0);	// *
install(c, -1, "int32 div(int32 a, uns32 b) {return emit(i32.div, i32(a), i32(b));}", 0);	// /
install(c, -1, "int32 mod(int32 a, uns32 b) {return emit(i32.mod, i32(a), i32(b));}", 0);	// %

install(c, -1, "int32 ceq(int32 a, uns32 b) {return emit(b32.ceq, i32(a), i32(b));}", 0);	// ==
install(c, -1, "int32 cne(int32 a, uns32 b) {return emit(b32.cne, i32(a), i32(b));}", 0);	// !=
install(c, -1, "int32 clt(int32 a, uns32 b) {return emit(i32.clt, i32(a), i32(b));}", 0);	// <
install(c, -1, "int32 cle(int32 a, uns32 b) {return emit(i32.cle, i32(a), i32(b));}", 0);	// <=
install(c, -1, "int32 cgt(int32 a, uns32 b) {return emit(i32.cgt, i32(a), i32(b));}", 0);	// >
install(c, -1, "int32 cge(int32 a, uns32 b) {return emit(i32.cge, i32(a), i32(b));}", 0);	// >=

install(c, -1, "int32 shr(int32 a, uns32 b) {return emit(b32.shl, i32(a), i32(b));}", 0);	// >>
install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.sar, i32(a), i32(b));}", 0);	// <<
install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.and, i32(a), i32(b));}", 0);	// &
install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32. or, i32(a), i32(b));}", 0);	// |
install(c, -1, "int32 shl(int32 a, uns32 b) {return emit(b32.xor, i32(a), i32(b));}", 0);	// ^

install(c, -1, "int32 pls(int32 a) asm {return emit(void, i32(a));}", 0);	// +
install(c, -1, "int32 neg(int32 a) asm {return emit(neg.i32, i32(a));}", 0);	// -
install(c, -1, "int32 cmt(int32 a) asm {return emit(cmt.b32, i32(a));}", 0);	// ~
install(c, -1, "int32 not(int32 a) asm {return emit(b32.not, i32(a));}", 0);	// !

