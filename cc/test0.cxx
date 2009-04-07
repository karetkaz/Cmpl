//~ main -c -wx text0.cxx
//~ main -c -t text0.cxx
//~ main -c -x -we test0.cxx
//~ ./main -x "3 + 3 -4 + -2;"

/* type test
/+	alias
define type int08;
//~ define type uns08;
int min = type.min;
int max = type.max;
//~ +/
/+	struct
struct vec4d:4 {
	struct Rec1 {
		struct Rec11 {
			int alma;
			float[10][10] a;
			//~ define type = __DEFN__;
		};
		struct Rec12 {
			//~ float[10][10] a;
			define name = __DEFN__;
			define file = __FILE__;
			define line = __LINE__;
		};
	};
	//~ float[10][10] a;
	//~ long c;
	float x;
	float y;
	float z;
	float w;
	//~ Swizzle enu;
};

//~ define dd vec4d.Rec1.Rec11;
// +/
/+	typdef
//~ define tomb int[10];
//~ define alma tomb(int, int, double);
//~ define xxx int(int(int, int), int(double), double[](double x, double));
//~ define cmp_fn int(int x, int y);
//~ define sort1 int(int[], int len, cmp_fn cmp);
define xxx int[66][77](int[], int, int (int x, int y));

//~ alma z[100] = 0;
//~ alma[100] w = 0;
//~ xxx[100] q = 0;
//~ int(int, int, double) x[100] = alma(2);
//~ int(int, int, double) y[100] = int(0), s = int(1);
int[100](int, int, double) a = xxx(0);
// +/
/+	function
int[] fn(int a, int b, double d) {
	int xx = a;
	for (int i = 0; i < a; i += 1)
		d += i;
	int fn2() {
		for (int i = 0; i < a; i += 1)
			d += i;
	};
};
// +/

//~ // */

//~ /* functios

//~ double e = math.e;
//~ double pi = math.pi;
//~ double pi2 = math.pi/2;
//~ double sin = math.sin((math.pi/2));
//~ double dd = math.atan2(2, 3);
//~ double lg2 = math.log2(8);
//~ double xp2 = math.exp2(3);
//~ double lxp = math.exp2(math.log2(4));
//~ flt32 x = 1, y = 2, z = 3, w = 4; x = 2; y = w+3;
//~ float a[1][2][3]; a[2][3][4] = 0;
//~ int32 a1 = math.shl(1, 10);
//~ int64 b1 = math.shl(long(1), 10);

//~ flt64 aT2 = math.pi;
//~ flt64 at2 = libc.atan2(flt64(2), flt64(1));
//~ double sin = libc.sin(double(math.pi/2));

//~ define bit = 128;
//~ define cnt = 16;

//~ float bitsize = bit;
//~ float elemcnt = cnt;
//~ float elembit = bit / cnt;
//~ float argbits = cnt * math.bsr(cnt);
//~ int32 b = 1;
//~ int64 a = libc.shl(int64(b), 50);

// */

/* constants
//~ flt64 a = -0.;
//~ flt64 b = +0.;
//~ flt64 c = a == b;
//~ int a = 3 & -3;
//~ define con math.con;
//~ define v = 1 - 2 * (math.nan != math.nan);
//~ float v = 1 - 2 * math.pi;
//~ flt32 s = v;
//~ flt64 d=v;
//~ {d = v;}

//~ int64 val = int64.min;

//~ flt32 seps = flt32.eps;
//~ flt32 smin = flt32.min;
//~ flt32 smax = flt32.max;
*/

/* test bits
int32 a := 0x80000100;
int32 res := -1;
//~ /+	// public static int bsr(int i)
int32 lib := math.bsr(a);
if (a) {
	uns32 x = a, ans = 0;
	if (x & 0xffff0000) { ans += 16; x >>= 16; }
	if (x & 0x0000ff00) { ans +=  8; x >>=  8; }
	if (x & 0x000000f0) { ans +=  4; x >>=  4; }
	if (x & 0x0000000c) { ans +=  2; x >>=  2; }
	if (x & 0x00000002) { ans +=  1; }
	res = ans;
}// +/

/+	// public static int hi1(int i)
int32 lib = math.bhi(a);
{
	uns32 u = a;
	u |= u >> 1;
	u |= u >> 2;
	u |= u >> 4;
	u |= u >> 8;
	u |= u >> 16;
	res = u - (u >> 1);
}// +/

/+	// public static int lo1(int i)
	res = a & -a;
// +/

/+	// public static int bitswap(uns i)
int32 lib = math.bsw(a);
if (a) {
	unsigned x = a;
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	res = (x >> 16) | (x << 16);
}
// +/

// */

/* test libc
//~ flt64 one = math.log(math.exp(8.));
//~ if (0) {
flt64 one;// = math.log(flt64(3));
one = math.log(flt64(3));
//~ }

//~ */

/* factorial
define  n = 5;
double res = 1;
//~ int i;
for (int32 i = 1; i <= n; i += 1)
	res *= i;

// */

/* calc e
define eps = 1e-30;
flt32 tmp = 1, res = 1;
// i;
for (int i = 1; tmp > eps; i += 1) {
	res += tmp /= i;
	//~ res = res + (tmp = tmp / i);
	//~ tmp = tmp / i; res = res + tmp;
}
// */

/* count a lot
int32 a = 0x04f;
int32 b = 0x04f;
int32 c = 0x04f;
define n = 10000000;
for (int i = 1; i <= n; i += 1) {
	a += float(i);
	b += float(i);
	c += float(i);
}
// */

/* test if
define signed int32;
define float flt32;
signed i = 1 + 4;
float a = 4 * 2;

if (i) {
	if (i == 1) a = 1.1; else a = 2.1;
	if (i == 1) a = 1.2; else ;
	if (i == 1); else a = 2.2;
	if (i == 1) a = 1.3;

	else if (i == 2) a = 1.42;
	else if (i == 3) a = 1.43;
	else if (i == 4) a = 1.44;
	else if (i == 5) a = 1.45;
	else if (i == 6) a = 1.46;
}
else {
	if (a <= 5) {
		if (a < 2.5) i = 1;
		else i = 2;
	}
	else {
		if (a < 7.5) i = 3;
		else i = 4;
	}
}

//~ if (1)
	//~ {{{{{;;;};}};;;}{;;;}}
//~ else
	//~ {{;};;{;}}

// */

/* test if static
//~ define i = 0;
float a = math.nan;
int min = 0; //uns32.min;
int max = 1; //int32.max;
//~ float e = float(4);
float c = 1 + math.pi; //segmentation fault
if (a==a) {
	c = min;
}
else {
	c = max;
}
// */

/* RayTrace
define rgb int32;
define qual = 0;
define width = 320 << qual;
define height = 240 << qual;

rgb cBuff[width][height];
int zBuff[width][height];

//~ bmp.save("raytr1.bmp", cBuff, width, height);

//~ */

/** 3d math

struct flt32 {
	final flt32 add(flt32 lhs, flt32 rhs) {emit(vm.add.f32, flt32(lhs), flt32(rhs));}
	final flt32 operator +=(out flt32 lhs, flt32 rhs) = (lhs = add(lhs, rhs));
	final flt32 operator + (flt32 lhs, flt32 rhs) = (add(lhs, rhs));
}

//~ int08
//~ int16
//~ int32
//~ int64

//~ uns08
//~ uns16
//~ uns32
//~ uns64

//? flt16
//~ flt32
//~ flt64

//p2x32
//+ f2x32

//p4x32
//? f8x16
//+ f4x32
//+ f2x64

struct vec4f : p4x32 {
	//~ flt32 x;
	//~ flt32 y;
	//~ flt32 z;
	//~ flt32 w;
	//~ static this() {}
	this() {emit(vm.ldz.x4);}
	this(flt32 a) {emit(vm.dup.x1, 0, a); emit(vm.dup.x2, 0);}
	this(flt32 x, flt32 y, flt32 z) {this.x = x; this.y = y; this.z = z; this.w = 0;}
	this(flt32 x, flt32 y, flt32 z, flt32 w) {this.x = x; this.y = y; this.z = z; this.w = w;}

	vec4f final operator +(vec4f lhs, vec4f rhs) {return add(lhs, rhs);}
	vec4f final operator +(vec4f lhs, flt32 rhs) {return add(lhs, vec4f(rhs));}
	vec4f final operator +(flt32 lhs, vec4f rhs) {return add(vec4f(lhs), rhs);}
	vec4f final operator +=(out vec4f lhs, vec4f rhs) {return lhs = add(lhs, rhs);}
	vec4f final operator +=(out vec4f lhs, flt32 rhs) {return lhs = add(lhs, vec4f(rhs));}

	static vec4f neg(vec4f rhs) {emit(vm.neg.pf4, p4x32(rhs));}
	static vec4f add(vec4f lhs, vec4f rhs) {return emit(vm.add.pf4, vec4f(lhs), vec4f(rhs));}
	static vec4f sub(vec4f lhs, vec4f rhs) {return emit(vm.sub.pf4, vec4f(lhs), vec4f(rhs));}
	static vec4f mul(vec4f lhs, vec4f rhs) {return emit(vm.mul.pf4, vec4f(lhs), vec4f(rhs));}
	static vec4f div(vec4f lhs, vec4f rhs) {return emit(vm.div.pf4, vec4f(lhs), vec4f(rhs));}

	static flt32 dp3(vec4f lhs, vec4f rhs) {return flt32(vm.emit(vm.dp3.pf4, vec4f(lhs), vec4f(rhs)));}
	static flt32 dp4(vec4f lhs, vec4f rhs) {return flt32(vm.emit(vm.dp4.pf4, vec4f(lhs), vec4f(rhs)));}
	static flt32 dph(vec4f lhs, vec4f rhs) {return flt32(vm.emit(vm.dph.pf4, vec4f(lhs), vec4f(rhs)));}
	static flt32 len(vec4f lhs) {return sqrt(flt32(vm.emit(vm.dph.pf4, vec4f(lhs), vec4f(lhs))));}

	static flt32 eval(float val) {return (((w * val + z) * val + y) * val) + x;}

	flt32 dp3(vec4f rhs) {return dp3(this, rhs);}
	flt32 dph(vec4f rhs) {return dph(this, rhs);}
	flt32 dp4(vec4f rhs) {return dp4(this, rhs);}
	flt32 len() {return len(this);}

	static vec4f mul(out vec4f res, ref vec4f lhs, ref vec4f rhs) {
		res.x = lhs.x * rhs.x;
		res.y = lhs.y * rhs.y;
		res.z = lhs.z * rhs.z;
		res.w = lhs.w * rhs.w;
		return res;
	}

	operator vec *= (vec rhs) {return mul(this, this, rhs);}
	static operator vec * (vec lhs, vec rhs) {return mul(this, lhs, rhs);}

	flt32 operator () (flt32 val) {return this.eval(val);}
};

struct mat4f {
	vec4f x;
	vec4f y;
	vec4f z;
	vec4f w;
	//~ static this() {}
	this() {}
	//~ this(flt32 a) {emit(vm.dup.x1, 0, a); emit(vm.dup.x2, 0);}
	//~ this(flt32 x, flt32 y, flt32 z) {this.x = x; this.y = y; this.z = z; this.w = 0;}
	//~ this(flt32 x, flt32 y, flt32 z, flt32 w) {this.x = x; this.y = y; this.z = z; this.w = w;}

	mat4f operator +(ref mat4f lhs, ref mat4f rhs) {return add(new, lhs, rhs);}
	mat4f operator +=(out mat4f lhs, ref mat4f rhs) {return add(lhs, lhs, rhs);}

	static vec4f dp3(ref mat4f mat, vec4f vec) {
		return vec4f(
			vec4f.dp3(mat.x, vec),
			vec4f.dp3(mat.y, vec),
			vec4f.dp3(mat.z, vec),
			1);
	}
	static vec4f dp4(ref mat4f mat, vec4f vec) {
		return vec4f(
			vec4f.dp4(mat.x, src),
			vec4f.dp4(mat.y, src),
			vec4f.dp4(mat.z, src),
			vec4f.dp4(mat.w, src));
	}
	static vec4f dph(ref mat4f mat, vec4f vec) {
		return vec4f(
			vec4f.dph(mat.x, src),
			vec4f.dph(mat.y, src),
			vec4f.dph(mat.z, src),
			vec4f.dph(mat.w, src));
	}

	vec4f dp3(vec4f rhs) {return dp3(this, rhs);}
	vec4f dph(vec4f rhs) {return dph(this, rhs);}
	vec4f dp4(vec4f rhs) {return dp4(this, rhs);}

	operator vec4f mul(ref mat4f mat, vec4f vec) {return dp4(mat, rhs);}

	static mat4f add(out mat4f res, ref mat4f lhs, ref mat4f rhs) {
		res.x = lhs.x + rhs.x;
		res.y = lhs.y + rhs.y;
		res.z = lhs.z + rhs.z;
		res.w = lhs.w + rhs.w;
		return dst;
	}
}

//~ */

/** tlib
	int32 __add(int32 a, uns32 b) asm {emit(add.i32, int32(a), int32(b));}
	int64 __add(int32 a, uns64 b) asm {emit(add.i64, int64(a), int64(b));}
	int32 __add(int32 a, int32 b) asm {emit(add.i32, int32(a), int32(b));}
	int64 __add(int32 a, int64 b) asm {emit(add.i64, int64(a), int64(b));}
	flt32 __add(int32 a, flt32 b) asm {emit(add.f32, flt32(a), flt32(b));}
	flt64 __add(int32 a, flt64 b) asm {emit(add.f64, flt64(a), flt64(b));}

	int64 __add(int64 a, uns32 b) asm {emit(add.i64, int64(a), int64(b));}
	int64 __add(int64 a, uns64 b) asm {emit(add.i64, int64(a), int64(b));}
	int64 __add(int64 a, int32 b) asm {emit(add.i64, int64(a), int64(b));}
	int64 __add(int64 a, int64 b) asm {emit(add.i64, int64(a), int64(b));}
	flt32 __add(int64 a, flt32 b) asm {emit(add.f32, flt32(a), flt32(b));}
	flt64 __add(int64 a, flt64 b) asm {emit(add.f64, flt64(a), flt64(b));}

	class cpl64 {
		flt64 re;
		flt64 im;
		this(flt64 re) {
			this.re = re;
			this.im = 0;
		}
		this(flt64 re, flt64 im) {
			this.re = re;
			this.im = im;
		}
		static cpl mul2(cpl64 a, cpl64 b) asm {
			emit(mul.pf2, a, b);		// a.re * b.re, a.im * b.im
			emit(sub.f64);			//>re = a.re * b.re - a.im * b.im
			emit(swz.zwxy, a, b);		// a.re, b.re, b.im, a.im
			emit(mul.pf2);			// a.re * b.im, a.im * b.re
			emit(add.f64);			//>im = a.re * b.im + a.im * b.re
		}
		static cpl mul(cpl64 a, cpl64 b) asm {
			emit(mul.pf2, a.re, a.im, b.re, b.im);		// a.re * b.re, a.im * b.im
			emit(sub.f64);					//>re = a.re * b.re - a.im * b.im
			emit(mul.pf2, a.re, a.im, b.im, b.re);		// a.re * b.im, a.im * b.re
			emit(add.f64);					//>im = a.re * b.im + a.im * b.re
		}
		static cpl mul(out cpl64 res, cpl64 lhs, cpl64 rhs) asm {
			emit(mul.pf2, a.re, a.im, b.re, b.im);		// a.re * b.re, a.im * b.im
			emit(sub.f64);					//>re = a.re * b.re - a.im * b.im
			emit(mul.pf2, a.re, a.im, b.im, b.re);		// a.re * b.im, a.im * b.re
			emit(add.f64);					//>im = a.re * b.im + a.im * b.re
		}
		static cpl mul(cpl64 a, flt64 b) asm {
			emit(vm.mul.pf2, a, b, b);		// a.re * b, a.im * b
		}
		static cpl add(cpl64 a, cpl64 b) asm {
			emit(vm.add.pf2, a, b);
		}
		static cpl sub(cpl64 a, cpl64 b) asm {
			emit(vm.sub.pf2, a, b);
		}
		static cpl neg(cpl64 a) asm {
			emit(vm.neg.pf2, a);
		}
		static cpl rcp(cpl64 a) asm {			// ? reciprocal
			//~ return new Complex(
				//~ +a.re / (a.re * a.re + a.im * a.im),
				//~ -a.im / (a.re * a.re + a.im * a.im));
			emit(neg.f64, a);	// a.re, -a.im
			emit(mul.pd2, a, a);	// a.re ** 2, a.im ** 2
			emit(dupp, 0);		// a.re ** 2, a.im ** 2
			emit(add.pd2);
			emit(div.pd2);
		}

		friend operator cpl + (cpl64 a, flt64 b) {
			return add(a, b);
		}
		friend operator cpl += (out cpl64 a, flt64 b) {
			return a = add(a, b);
		}
		operator cpl + (flt64 b) {
			return add(this, b);
		}
		operator cpl += (flt64 b) {
			return this = add(this, b);
		}
	}

	int32 add_int32(out int32 a, int32 b) x86.eax {
		emit("add [eax], edx", eax : offs(a), edx : int32(b));
		emit("mov eax, [eax]");
	}
	void* mul_f4x32(void* res, void* lhs, void* rhs) x86.eax {
		emit("movps xmm0, [%0]", lhs);
		emit("addps xmm0, [%1]", rhs);
		emit("movps [eax], xmm0", eax : res);
	}

void* vecmul(void* dst, void* lhs, void* rhs);
#pragma aux (parm [edi] [eax] [edx] value [edi] modify exact []) vecmul =\
	".686"\
	"movps	xmm0, [eax]"\
	"mulps	xmm0, [edx]"\
	"movps	[edi], xmm0"//
// */
