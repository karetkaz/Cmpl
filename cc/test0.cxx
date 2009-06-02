//~ xterm -e gdb --quiet main
//~ main -c -wx text0.cxx
//~ main -c -t text0.cxx
//~ main -c -x -we test0.cxx
//~ ./main -x "3 + 3 -4 + -2;"

//~ define pi2 = math.pi;

//~ int a, b, c;
//~ 2 + math.pi;
//~ -math.pi;

//~ -math.sin(2);
/* Emit Test
//~ int32 operator add(int32 a, int32 b) {return emit(i32.add, i32(b), i32(a));}
//~ int32 operator add(int32 ref a, int32 b) {return a = emit(i32.add, i32(b), i32(a));}

//~ flt32 res = emit(f32.div, f32(2), f32(math.pi));	// ok
//~ flt32 res = emit(f32.neg, f32(math.pi));		// ok
//~ flt32 res = emit(void, f32(3.14));				// ok
flt64 res = sin(math.pi/4);				// ok

//~ emit(v4f.dp4, f32(3), f32(2), f32(1), f32(0), f32(3), f32(2), f32(1), f32(0));
//~ emit(v4f.add, f32(3), f32(2), f32(1), f32(0), f32(3), f32(2), f32(1), f32(0));
/+	Complex mul
define a_re = 3.;
define a_im = 4.;
define b_re = 2.;
define b_im = 5.;

//~ flt64 re2 = a_re * b_re - a_im * b_im;
//~ flt64 im2 = a_re * b_im + a_im * b_re;

emit(v2d.mul, f64(a_re), f64(a_im), f64(b_re), f64(b_im));		// a.re * b.re, a.im * b.im
flt64 re = emit(f64.sub);										//>re = a.re * b.re - a.im * b.im
emit(v2d.mul, f64(a_re), f64(a_im), f64(b_im), f64(b_re));		// a.re * b.im, a.im * b.re 
flt64 im = emit(f64.add);										//>im = a.re * b.im + a.im * b.re
//~ +/

//~ emit(void, f32(1), f32(2), f32(3), f32(4));
//~ emit(swz.zyxx);
//~ math.sin(math.pi);
//~ emit(f32.div);
//~ */
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
/* functions
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
// */

//~ /* test bits
int32 a = 0x80001482;
int32 res = -1;
//~ /+	// public static int bsr(int i)
//~ int32 lib = bsr(a);
if (a) {
	uns32 x = a;
	uns32 ans = 0;
	if (x & 0xffff0000) { ans += 16; x >>= 16; }
	if (x & 0x0000ff00) { ans +=  8; x >>=  8; }
	if (x & 0x000000f0) { ans +=  4; x >>=  4; }
	if (x & 0x0000000c) { ans +=  2; x >>=  2; }
	if (x & 0x00000002) { ans +=  1; }
	res = ans;
}// +/

/+	// public static int hi1(int i)
int32 lib = bhi(a);
if (1) {
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
//~ int32 lib = bsw(a);
if (a) {
	uns32 x = a;
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	res = (x >> 16) | (x << 16);
	//~ res = x >> 16;
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
for (int i = 1; i <= n; i += 1)
	res *= i;

// */
/* calc e
define eps = 1e-30;
flt32 tmp, res = 1;
for (int32 i = 1; tmp > eps; i += 1) {
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
//~ define signed int32;
//~ signed i = 1 + 4;
//~ float a = 4 * 2;

//~ define i = 2;
if (i) {
	if (i == 1) a = 1.1; else a = 2.1;
	if (i == 1) a = 1.2; else ;
	if (i == 1) ;else a = 2.2;
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
define qual = 2;
define width = 320 << qual;
define height = 240 << qual;

rgb cBuff[width][height];
//~ int zBuff[width][height];

class Light {
	vector	ambi;		// Ambient
	vector	diff;		// Diffuse
	vector	spec;		// Specular
	vector	attn;		// Attenuation
	vector	pos;		// position
	vector	dir;		// direction
	scalar	sCos;
	scalar	sExp;
	short	attr;		// typeof(on/off)
	vec4f lit(out ambi, out diff, out spec, vec4f pos, vec4f nrm, vec4f eye, Material mtl) {
		vec4f tmp, color = mtl.emis;

		if (attr & L_on) {
			vecptr D = pos - this.pos;	// distance
			vecptr R, L = dir;			// (L) lightDir
			scalar dotp, attn = 1, spot = 1;

			if (vec4f.dp3(dir, dir)) {			// directional or spot light
				if (sCos) {						// Spot Light
					scalar spot = L.dp3(D.nrm());
					attn = 0;
					if (spot > cos(toRad(sCos))) {
						spot **= sExp;
					}
				}
			}
			else {
				L = vec4f.nrm(D);
			}// * /

			attn = spot / vecpev(this.attn, D.len());

			color += ambi = this.ambi * mtl.ambi * attn;

			if ((dotp = -vecdp3(N, L)) > 0) {
				color += diff = this.diff * mtl.diff * (attn * dotp);

				R = vecnrm(vecrfl(V - eye, N));
				if ((dotp = -vecdp3(R, L)) > 0) {
					color += spec = this.spec * mtl.spec * (attn * (dotp ** spow));
				}
			}
		}
		return color;
		col[i].rgb = vecrgb(&color);
	}
}

class Material {	// material
	vec4f emis(0, 0, 0, 1);		// Emissive
	vec4f ambi(0, 0, 0, 1);		// Ambient
	vec4f diff(0, 0, 0, 1);		// Diffuse
	vec4f spec(0, 0, 0, 1);		// Specular
	flt32 spow(1);				// Shin...
	static Material Brass = Material {
		ambi(0.329410, 0.223529, 0.027451, 1),
		diff(0.780392, 0.568627, 0.113725, 1),
		spec(0.992157, 0.941176, 0.807843, 1),
		spow(27.8974)
	};
	static Material Bronze = Material {
		ambi(0.2125, 0.1275, 0.054, 1.0),
		diff(0.714, 0.4284, 0.18144, 1.0), 
		spec(0.393548, 0.271906, 0.166721),
		spow(25.6)
	};
	static Material Chrome = Material {
		ambi(0.25, 0.25, 0.25, 1.0),
		diff(0.4, 0.4, 0.4, 1.0),
		spec(0.774597, 0.774597, 0.774597, 1.0),
		spow(76.8)
	};
	static Material Silver = Material {
		ambi(0.19225, 0.19225, 0.19225, 1.0),
		diff(0.50754, 0.50754, 0.50754, 1.0),
		spec(0.508273, 0.508273, 0.508273, 1.0),
		spow(51.2)
	};
	static Material Gold = Material {
		ambi(0.24725, 0.1995, 0.0745, 1.0),
		diff(0.75164, 0.60648, 0.22648, 1.0),
		spec(0.628281, 0.555802, 0.366065, 1.0),
		spow(51.2)
	};
	static Material Jade = Material {
		ambi(0.135,0.2225,0.1575,0.95),
		diff(0.54, 0.89, 0.63, 0.95),
		spec(0.316228, 0.316228, 0.316228, 0.95),
		spow(12.8)
	};
	static Material Ruby = Material {
		ambi(0.1745,0.01175 ,0.01175,0.55 ),
		spec(0.61424,0.04136, 0.04136,0.55),
		diff(0.727811, 0.626959, 0.626959,0.55),
		spow(76.8)
	};
	lit(Light[] l) {}
};

lights[] = {
	Light(...),
	new lghtsrc(...),
};

scene[] = {
	new prim(new sphere(...), mtl.gold),
	new prim(new tours(...), mtl.gold),
	new mesh(mtl.gold),
};

struct hitinfo {
	vec4f pos;
	vec4f nrm;
	flt32 len;
}

vec4f traceRay(const vec4f dir, int depth) {
	vec4f pos, N, col;
	flt32 d = math.inf;
	prim p = null;
	hitinfo hit;

	for (prim ps : scene) {
		if (ps.hit(hit, dir)) {
			if (hit.len > d) {
				pos = hit.pos;
				N = hit.nrm;
				d = hit.len;
				p = ps;
			}
		}
	}
	if (p != null) {
		for (Light l : lights) {
			bool inShadow = false;
			vec4f p2l = nrm(p.pos - l.pos);		// direction to light
			for (prim ps : scene) {
				if (ps.hit(hit, dir)) {
					inShadow = true;
				}
			}
			if (!inShadow) {
				col = p.mtl.lit(l, N, pos, vec4f(0, 0, 1));
			}
		}
		if (depth > 0 && p.mtl.reflect) {
			col += traceRay(rfl(nrm, pos), depth - 1);
		}
		if (depth > 0 && p.mtl.refract) {
			col += traceRay(rfr(nrm, pos), depth - 1);
		}
	}
	return col;
}

main() {
	for (int y = 0; y < height; y += 1) {
		for (int x = 0; x < width; x += 1) {
			flt32 s = x - width / 2.;
			flt32 t = y - height / 2.;
			vec4f dir = vec4f(s, t, 100, 0);
			cBuff[x][y] = traceRay(dir.nrm());
		}
	}
	bmp.save("raytr1.bmp", cBuff, width, height, 24);
}

class lights {
	Light[] l;
	uns32 it;

	Light first() {return l[it = 0];}
	Light next() {return l[it = 0];}
	bool has() {return it < l.length;}
}

for (Light l : lights) => for (Light l = lights.first(); lights.has(); l = lights.next())
//~ for (Light l : lights) => for (Light l = lights.first(); lights.first(); l = lights.next())


//~ */
