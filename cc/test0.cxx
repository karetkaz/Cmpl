//~ ./main -x "3 + 3 -4 + -2;"
//~ define pi2 = math.pi;

/* Emit Test
//~ int32 operator add(int32 a, int32 b) {return emit(i32.add, i32(b), i32(a));}
//~ int32 operator add(int32 ref a, int32 b) {return a = emit(i32.add, i32(b), i32(a));}

//~ flt32 res1 = emit(f32.div, f32(2), f32(math.pi));	// ok
//~ flt64 res2 = emit(f32.neg, f32(math.pi));		// ok
//~ flt64 res3 = emit(void, f32(3.14));				// ok

//~ flt32 res = emit(v4f.dp4, f32(3), f32(2), f32(1), f32(0), f32(3), f32(2), f32(1), f32(0));
//f32x4 res = 
//~ emit(v4f.add, f32(3), f32(2), f32(1), f32(0), f32(3), f32(2), f32(1), f32(0));
/+	Complex mul
define a_re = 3.;
define a_im = 4.;
define b_re = 2.;
define b_im = 5.;

//~ flt64 re = a_re * b_re - a_im * b_im;
//~ flt64 im = a_re * b_im + a_im * b_re;

emit(v2d.mul, f64(b_im), f64(b_re), f64(a_im), f64(a_re));		// a.re * b.re, a.im * b.im
flt64 re = emit(f64.sub);										//>re = a.re * b.re - a.im * b.im
emit(v2d.mul, f64(b_re), f64(b_im), f64(a_im), f64(a_re));		// a.re * b.im, a.im * b.re
flt64 im = emit(f64.add);										//>im = a.re * b.im + a.im * b.re
//~ +/

//~ */
//~ /* Libc Test
int32 var = 0x00001081;
int32 res = -1;
//~ /+	// public static int bsr(int i)
int32 lib = bsr(var);
if (var) {
	uns32 x = var;
	uns32 ans = 0;
	if (x & 0xffff0000) { ans += 16; x >>= 16; }
	if (x & 0x0000ff00) { ans +=  8; x >>=  8; }
	if (x & 0x000000f0) { ans +=  4; x >>=  4; }
	if (x & 0x0000000c) { ans +=  2; x >>=  2; }
	if (x & 0x00000002) { ans +=  1; }
	res = ans;
}
//~+/
/+	// public static int hi1(int i)
int32 lib = bhi(var);
if (var) {
	uns32 u = var;
	u |= u >> 1;
	u |= u >> 2;
	u |= u >> 4;
	u |= u >> 8;
	u |= u >> 16;
	res = u - (u >> 1);
}
//~+/
/+	// public static int lo1(int i)
	res = var & -var;
//~+/
/+	// public static int bitswap(uns i)
int32 lib = swp(var);
if (var) {
	uns32 x = var;
	x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
	x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
	x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
	x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
	res = (x >> 16) | (x << 16);
}
//~+/
//~*/
/* Type Test
/+	alias
define myint int08;
int64 min = myint.min;
int64 max = myint.max;
int64 msk = myint.mask;
int64 bts = myint.bits;
int64 size = myint.size;
//~ +/
/+	struct
struct vec4d {
	struct vec3d {
		float x;
		float y;
		float z;
	};
	float x;
	float y;
	float z;
	float w;
}
define dd vec4d.vec3d;

// +/
/+X	typdef
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
//~ int[100](int, int, double) a = xxx(0);
// +/
/+X	function
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
//~*/

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

// */
/* constants
define ftype flt64;
ftype a = -0.;
ftype b = +0.;
ftype c = math.nan;
ftype w = a == b;
ftype x = c == c;
//~ float v = 1 - 2 * math.pi;

//~ int32 xxx = float(int(3));
//~ flt64 f = 1e310;
//~ flt32 f32 = math.inf;
//~ flt64 f64 = math.inf;
//~ flt64 f642 = f32;
//~ flt32 f322 = f64;
// */

/* factorial
define n = 5;
flt64 res = 1;
//~ int i;
for (int32 i = 1; i <= n; i += 1)
	res *= i;

// */
/* calc e
define eps = 1e-15;
flt64 res = 1;
flt64 tmp = 1;
for (flt64 i = 1; tmp > eps; i += 1) {
	res += tmp /= i;
	//~ res = res + tmp = tmp / i;
	//~ tmp = tmp / i; res = res + tmp;
}
// */
/* count a lot
int32 a = 0x04f;
int32 b = 0x04f;
int32 c = 0x04f;
define n = 10000000;
for (int32 i = 1; i < n; i += 1) {
	a += float(i);
	b += double(i);
	c += flt32(i);
	a += float(n);
	b += double(n);
	c += int64(n);
}
// */
/* test if
int i = 3;
float a = 4;
if (i) {
	a = 0;
	if (i == 1) a = 1.1; else a = 2.1;
	if (i == 1) a = 1.2; else ;
	if (i == 1) ;else a = 2.2;
	if (i == 1) a = 3;

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

// */
/* test if 2
//~ if (1)
	//~ {{{{{;;;};}};;;}{;;;}}
//~ else
	//~ {{;};;{;}}
//*/
/* test if 3 (nan != nan)
float a = math.nan;
float c = 0;
if (a == a) {
	c = 1;
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
