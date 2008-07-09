#include <math.h>
#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "mcga.h"
#include "gx_vbe.h"
#include "gx_surf.h"
#include "gx_color.h"

//~ #include "libs/fixed/fixed.h"

//~ /*
#ifdef FXPOINT

typedef long scalar;

#define sconst(__a) (TOFIXED(__a))
#define scaldf(__a) (fxpldf(__a))
//~ #define scatof(__a) ((__a) / (float)(1 << FXPOINT))
#define scatoi(__a) (fxptoi(__a))

#define scamul(__a, __b) fxpmul(__a, __b)
#define scadiv(__a, __b) fxpdiv(__a, __b)
#define scainv(__a) fxpinv(__a)

#define scasin(__a) fxpsin(__a)
#define scacos(__a) fxpcos(__a)
#define scatan(__a) fxptan(__a)
#define scasqrt(__a) fxpsqrt(__a)
#define scarsq(__a) fxprsq(__a)
#define scapow(__a, __b) fxppow(__a, __b)
#define stof16(__a) (__a)
#define stof24(__a) ((__a) << 8)

#else

typedef float scalar;
//~ typedef double scalar;
#define sconst(__a) ((float)(__a))
#define scaldf(__a) ((float)(__a))
#define scatof(__a) ((float)(__a))
#define scatoi(__a) ((long)(__a))

#define scamul(__a, __b) ((__a)*(__b))
#define scadiv(__a, __b) ((__a)/(__b))
#define scainv(__a) (1. / (__a))

#define scasin(__a) sin(__a)
#define scacos(__a) cos(__a)
#define scatan(__a) tan(__a)
#define scasqrt(__a) sqrt(__a)
#define scarsq(__a) (1. / sqrt(__a))
#define scapow(__a, __b) pow(__a, __b)
#define stof16(__a) ((long)((__a) * (1<<16)))
#define stof24(__a) ((long)((__a) * (1<<24)))

#endif
//~ */

#include "vecmat.c"

#define MCGA 0

#define SCRW (1280)
#define SCRH (1024)

//~ #define SCRW (640)
//~ #define SCRH (480)

//~ #define SCRW (320)
//~ #define SCRH (240)

#define SCRD (32)

#if (MCGA == 1)
#undef SCRW
#undef SCRH
#undef SCRD
#define SCRW (320)
#define SCRH (200)
#endif

//~ | MD_LINEAR

//~ unsigned char cBuff[SCRW*SCRH];
unsigned long zBuff[SCRW*SCRH];
rect rec = {0,0,SCRW,SCRH};

matrix projM;		// projection
matrix viewM;		// camera * world
//~ matrix worldM;		// world
//~ matrix provM;		// proj * view
struct gx_Surf_t img;

enum {
	cull = 0x0020,		// enable culling
	//~ face = 0x0010,		// back/front face

	smt_ = 0x0008,		// enable smooth
	lit_ = 0x0004,		// enable lights

	fill = 0x0003,		// mask
	text = 0x0003,		// texture
	flat = 0x0002,		// goroaud
	wire = 0x0001,		// 
	plot = 0x0000,		// 

	//~ cull = 0x0080,		// enable culling
	//~ face = 0x0040,		// back/front face

	dspL = 0x0100,		// lights
	dspN = 0x0200,		// normals
	//~ refl = 0x0400,		// reflect
	dspZ = 0x0800,		// zbuffer
};

int draw = flat;

typedef struct {
	unsigned short s, t;
	//~ unsigned long rgb;
} texpos;

struct vtx {
	vector	pos;			// (x, y, z, 1) position
	vector	nrm;			// (x, y, z, 0) normal
	texpos	tex;			// (s, t, 0, 0) coord
};

struct tri {
	unsigned i1;
	unsigned i2;
	unsigned i3;
};

static struct {			// mesh
	//~ unsigned maxtex, texcnt;	// textures
	unsigned maxvtx, vtxcnt;	// vertices
	unsigned maxtri, tricnt;	// triangles
	//~ struct mat back, fore;
	//~ vecptr *posptr;
	//~ vecptr *nrmptr;
	//~ texpos *texptr;
	struct vtx *vtxptr;
	struct tri *triptr;
} mesh;

enum {
	L_on   = 0x0001,		// light is on
	//~ L_attn = 0x0002,		// attenuate
	//~ L_dir  = 0x0004,		// is directional
};

static struct {			// lights
	//~ float m_fConstant;
	//~ float m_fLinear;
	//~ float m_fQuadratic;
	//~ float m_fIntensity;
	short	attr;
	vector	lpos;		// light position
	//~ vector	ldir;		// light position
	vector	ambi;		// Ambient
	vector	diff;		// Diffuse
	vector	spec;		// Specular
	//~ vector	attn;		// attenuation = 1 / (K1 + K2 � dist + K3 � dist ** 2)
	//~ scalar	spot;		// == 1
}

Lights[] = {
	{	// light0
		L_on,
		{{15, 15, 15, 1}},	// lpos
		//~ {{0., 0., 0., 1}},	// ldir
		{{.8, .0, .0, 1}},	// ambi
		{{1., 0., 1., 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		}, // * /
	{	// light1
		L_on,
		{{15, 0, -15, 1}},	// lpos
		//~ {{0., 0., 0., 1}},	// ldir
		{{.2, .2, .2, 1}},	// ambi
		{{1., 1., 1., 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		}, // * /
};

struct mat {			// material
	vector emis;		// Emissive
	vector ambi;		// Ambient
	vector diff;		// Diffuse
	vector spec;		// Specular
	scalar spow;
}

material = {
	{{.0, .0, .0, 1}},		// emissive color
	{{.8, .8, .8, 1}},		// ambient color
	{{.8, .8, .8, 1}},		// diffuse color
	{{1., 1., 1., 1}},		// specular color
	7,				// specular power
};

// */

//~ /*	frustum
void frustum_get(vector planes[6], matptr mat) {
	//~ near and far	: -1 < z < 1
	vecnrm(&planes[0], vecadd(&planes[0], &mat->w, &mat->z));
	vecnrm(&planes[1], vecsub(&planes[1], &mat->w, &mat->z));
	//~ left and rigth	: -1 < x < 1
	vecnrm(&planes[2], vecadd(&planes[2], &mat->w, &mat->x));
	vecnrm(&planes[3], vecsub(&planes[3], &mat->w, &mat->x));
	//~ top and bottom	: -1 < y < 1
	vecnrm(&planes[4], vecadd(&planes[4], &mat->w, &mat->y));
	vecnrm(&planes[5], vecsub(&planes[5], &mat->w, &mat->y));
}

int ftest_sphere(vector planes[6], vecptr p) {			// clip sphere
	scalar r = -p->w;
	if (vecdph(&planes[0], p) <= r) return 0;
	if (vecdph(&planes[1], p) <= r) return 0;
	if (vecdph(&planes[2], p) <= r) return 0;
	if (vecdph(&planes[3], p) <= r) return 0;
	if (vecdph(&planes[4], p) <= r) return 0;
	if (vecdph(&planes[5], p) <= r) return 0;
	return 1;	// inside
}

int ftest_point(vector planes[6], vecptr p) {			// clip point
	if (vecdph(&planes[0], p) <= 0) return 0;
	if (vecdph(&planes[1], p) <= 0) return 0;
	if (vecdph(&planes[2], p) <= 0) return 0;
	if (vecdph(&planes[3], p) <= 0) return 0;
	if (vecdph(&planes[4], p) <= 0) return 0;
	if (vecdph(&planes[5], p) <= 0) return 0;
	return 1;	// inside
}

int ftest_aabox(vector planes[6], vecptr bmin, vecptr bmax) {			// clip aabb
	return 0;	// implement
}

vecptr bsphere(vecptr s, vecptr p1, vecptr p2, vecptr p3) {
	scalar	len, ln1, ln2;
	vector	tmp;
	//~ vector	min, max;
	//~ vecmax(&max, vecmax(&tmp, p1, p2), p3);					// minimum
	//~ vecmin(&min, vecmin(&tmp, p1, p2), p3);					// maximum
	//~ vecsca(s, vecadd(&tmp, &min, &max), sconst(1./2));		// Box center
	vecadd(&tmp, vecadd(&tmp, p1, p2), p3);
	vecsca(s, &tmp, sconst(1./3));							// Baricentrum;

	len = vecdp3(&tmp, vecsub(&tmp, s, p1));
	ln1 = vecdp3(&tmp, vecsub(&tmp, s, p2));
	ln2 = vecdp3(&tmp, vecsub(&tmp, s, p3));
	if (len < ln1) len = ln1;
	if (len < ln2) len = ln2;
	s->w = scasqrt(len);
	return s;
}//*/

scalar backface(vecptr N, vecptr p1, vecptr p2, vecptr p3) {
	vector e1, e2, tmp;
	if (!N) N = &tmp;
	vecsub(&e1, p1, p2);
	vecsub(&e2, p1, p3);
	vecnrm(N, veccrs(N, &e1, &e2));
	N->w = 0;
	return vecdp3(p1, N);
}

vecptr centerpt(vecptr P, vecptr p1, vecptr p2, vecptr p3) {
	vecadd(P, p1, p2);
	vecadd(P, P, p3);
	vecsca(P, P, 1./3);
	return P;
}

vecptr mapvec(vecptr dst, matptr mat, vecptr src) {
	vector tmp;
	if (src == dst) src = veccpy(&tmp, src);
	dst->x = vecdp4(&mat->x, src);
	dst->y = vecdp4(&mat->y, src);
	dst->z = vecdp4(&mat->z, src);
	dst->w = vecdp4(&mat->w, src);
	if (dst->w) {
		dst->x = scadiv(dst->x, dst->w);
		dst->y = scadiv(dst->y, dst->w);
		dst->z = scadiv(dst->z, dst->w);
		//~ vecsca(dst, dst, scainv(dst->w));
		dst->w = sconst(1);
	}
	return dst;
}

inline argb vecrgb(vecptr src) {
	argb res;
	float r = scatof(src->r);
	float g = scatof(src->g);
	float b = scatof(src->b);
	if (r > 1) r = 1; else if (r < 0) r = 0;
	if (g > 1) g = 1; else if (g < 0) g = 0;
	if (b > 1) b = 1; else if (b < 0) b = 0;
	res.r = (unsigned char)(r * 255);
	res.g = (unsigned char)(g * 255);
	res.b = (unsigned char)(b * 255);
	return res;
	//~ return (long)((0.299 * r + 0.587 * g + 0.114 * b) * 255);
}

inline vecptr reflect(vecptr dst, vecptr dir, vecptr nrm) {
	//~ vector tmp;
	//~ return vecsub(dst, dir, vecsca(&tmp, nrm, 2 * vecdp3(nrm, dir)));
	return vecsub(dst, dir, vecsca(dst, nrm, 2 * vecdp3(nrm, dir)));
}

inline argb lit(vecptr pos, vecptr nrm, vecptr eye, matptr view) {
	vector Ptmp, Ntmp, Etmp, col;
	unsigned i, n = sizeof(Lights) / sizeof(*Lights);
	eye = mapvec(&Etmp, view, eye);
	pos = mapvec(&Ptmp, view, pos);
	nrm = mapvec(&Ntmp, view, nrm);
	veccpy(&col, &material.emis);
	for (i = 0; i < n; i += 1) if (Lights[i].attr & L_on) {
		scalar dotp;
		vector tmp, lp, k, L;//, R;
		mapvec(&lp, view, &Lights[i].lpos);

		// Ambient
		vecmul(&k, &material.ambi, &Lights[i].ambi);
		vecadd(&col, &col, &k);
		//~ */

		// Diffuse
		vecnrm(&L, vecsub(&tmp, &lp, pos));		// direction of light to vertex
		if ((dotp = vecdp3(nrm, &L)) > 0) {
			vecmul(&k, &material.diff, &Lights[i].diff);
			vecadd(&col, &col, vecsca(&tmp, &k, dotp));
		} // */

		// Specular
		/*/~ vecnrm(&R, reflect(&tmp, eye, pos));
		vecnrm(&R, vecsub(&tmp, &L, pos));
		if ((dotp = vecdp3(nrm, &R)) > 0) {
			vecmul(&k, &material.spec, &Lights[i].spec);
			vecadd(&col, &col, vecsca(&tmp, &k, pow(dotp, material.spow)));
		}// */

		// Attenuate
		//~ TODO
		// Spot effect
		//~ TODO
	}
	return vecrgb(&col);
	
}// */

//~ ----------------------------------------------------------------------------

int k3d_exit() {
	mesh.maxtri = mesh.tricnt = 0;
	mesh.maxvtx = mesh.vtxcnt = 0;
	free(mesh.vtxptr);
	mesh.vtxptr = 0;
	free(mesh.triptr);
	mesh.triptr = 0;
	return 0;
}

int k3d_open() {
	mesh.maxtri = 128; mesh.tricnt = 0;
	mesh.maxvtx = 128; mesh.vtxcnt = 0;
	mesh.triptr = (struct tri*)malloc(sizeof(struct tri) * mesh.maxtri);
	mesh.vtxptr = (struct vtx*)malloc(sizeof(struct vtx) * mesh.maxvtx);
	if (!mesh.triptr || !mesh.vtxptr) {
		k3d_exit();
		return -1;
	}
	return 0;
}

/*void prepos(vecptr dst, matptr mat, void* src, int n, unsigned stride) {
	while (n-- > 0) {
		vecptr vec = (vecptr)src;
		dst->x = vecdph(&mat->x, vec);
		dst->y = vecdph(&mat->y, vec);
		dst->z = vecdph(&mat->z, vec);
		dst->w = vecdph(&mat->w, vec);
		if (dst->w) {
			scalar w = 1. / dst->w;
			dst->x *= w;
			dst->y *= w;
			dst->z *= w;
			dst->w = 1;
		}
		src = (void*)((char*)src + stride);
		dst += 1;
	}
}

void prelit(argb c[], vecptr eye, matptr view) {
	vector col, nrm, pos, dir;
	unsigned i, j, n = sizeof(Lights) / sizeof(*Lights);
	if (eye) mapvec(&dir, view, eye);
	for (j = 0; j < mesh.vtxcnt; j += 1) {
		mapvec(&nrm, view, &mesh.vtxptr[j].nrm);
		mapvec(&pos, view, &mesh.vtxptr[j].pos);
		veccpy(&col, &material.emis);
		for (i = 0; i < n; i += 1) if (Lights[i].attr & L_on) {
			//{ VertexColor(r, g, b, a)  =
			//~ M.emis + 
			//~ W.ambi * M.ambi +
			//~ sum(i : 1 to nlights) (
				//~ L[i].ambi * M.ambi +
				//~ L[i].diff * M.diff * max(dot(L, n), 0) +
				//~ L[i].spec * M.spec * pow(max(dot(s, n), 0)), M.spow)
			//~ ) * L[i].attn * L[i].spot
				//~ dist = pos_L - pos_V;
				//~ attn = 1 / (K1 + K2 � dist + K3 � dist ** 2);
				//~ L = unit vector from vertex to light position
				//~ n = unit normal (at a vertex)
				//~ s = (x  + (0, 0, 1) ) / | x + (0, 0, 1) | / halfvector ???
				//~ x = vector indicating direction from light source (or direction of directional light)
			//}
			scalar diff, spec;
			vector lp, kA, kD, kS, L, H, tmp;
			mapvec(&lp, view, &Lights[i].lpos);
			vecmul(&kA, &material.ambi, &Lights[i].ambi);
			vecmul(&kD, &material.diff, &Lights[i].diff);
			vecmul(&kS, &material.spec, &Lights[i].spec);
			//~ mapvec(&lp, view, vecldf(&tmp, Lights[i].lpos[0], Lights[i].lpos[1], Lights[i].lpos[2], 1));
			//~ vecldf(&kA, material.ambi[0] * Lights[i].ambi[0], material.ambi[1] * Lights[i].ambi[1], material.ambi[2] * Lights[i].ambi[2], 1);
			//~ vecldf(&kD, material.diff[0] * Lights[i].diff[0], material.diff[1] * Lights[i].diff[1], material.diff[2] * Lights[i].diff[2], 1);
			//~ vecldf(&kS, material.spec[0] * Lights[i].spec[0], material.spec[1] * Lights[i].spec[1], material.spec[2] * Lights[i].spec[2], 1);

			// Ambient
			vecadd(&col, &col, &kA);

			// Diffuse
			vecnrm(&L, vecsub(&tmp, &lp, &pos));		// direction of light to vertex
			if ((diff = vecdp3(&nrm, &L)) > 0) vecadd(&col, &col, vecsca(&tmp, &kD, diff));

			// Specular
			vecnrm(&H, vecsub(&tmp, &L, &dir));			// halfvector betveen eye & light
			if ((spec = vecdp3(&nrm, &H)) > 0) vecadd(&col, &col, vecsca(&tmp, &kS, pow(spec, material.spow)));
		}
		c[j] = vecrgb(&col);
	}
}// */

void g3_setpixel(gx_Surf s, int x, int y, unsigned z, long c) {
	if ((unsigned)x >= (unsigned)SCRW) return;
	if ((unsigned)y >= (unsigned)SCRH) return;
	if (zBuff[y * SCRW + x] >= z) {
		zBuff[y * SCRW + x] = z;
		gx_setpixel(s, x, y, c);
		//~ cBuff[SCRW * y + x] = c;
	}
}

void g3_putpixel(gx_Surf s, vecptr p1, int c) {
	long x = scatoi((SCRW-1) * (sconst(1) + p1->x) / 2);
	long y = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	g3_setpixel(s, x, y, stof24((sconst(1) - p1->z) / 2), c);
}

void g3_drawline(gx_Surf s, vecptr p1, vecptr p2, long c) {
	long x1, x2, sx, dx = 0;
	long y1, y2, sy, dy = 0;
	long z1, z2, zs = 0, e;
	rect *r = &rec;

	x1 = scatoi((SCRW-1) * (sconst(1) + p1->x) / 2);
	y1 = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	z1 = stof24((sconst(1) - p1->z) / 2);

	x2 = scatoi((SCRW-1) * (sconst(1) + p2->x) / 2);
	y2 = scatoi((SCRH-1) * (sconst(1) - p2->y) / 2);
	z2 = stof24((sconst(1) - p2->z) / 2);

	dx |= (y1 >= r->ymax) << 0;
	dx |= (y1 <  r->ymin) << 1;
	dx |= (x1 >= r->xmax) << 2;
	dx |= (x1 <  r->xmin) << 3;
	dy |= (y2 >= r->ymax) << 0;
	dy |= (y2 <  r->ymin) << 1;
	dy |= (x2 >= r->xmax) << 2;
	dy |= (x2 <  r->xmin) << 3;

	while (dx | dy) {
		return;
	}
	if ((dy = y2 - y1) < 0) {
		dy = -dy;
		sy = -1;
	} else sy = 1;
	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		sx = -1;
	} else sx = 1;
	if (dx > dy) {
		e = dx >> 1;
		zs = (z2 - z1) / dx;
		while (x1 != x2) {
			g3_setpixel(s,x1,y1,z1,c);
			if ((e += dy) > dx) {
				y1 += sy;
				e -= dx;
			}
			x1 += sx;
			z1 += zs;
		}
	}
	else if (dy) {
		e = dy >> 1;
		zs = (z2 - z1) / dy;
		while (y1 != y2) {
			g3_setpixel(s,x1,y1,z1,c);
			if ((e += dx) > dy) {
				x1 += sx;
				e -= dy;
			}
			y1 += sy;
			z1 += zs;
		}
	}
	//g3_setpixel(s,x2,y2,z2,c);
}

void g3_drawoval(gx_Surf s, vecptr p1, long rx, long ry, long c) {
	long r;
	long x0, x1, sx, dx = 0;
	long y0, y1, sy, dy = 0;
	long x = scatoi((SCRW-1) * (sconst(1) + p1->x) / 2);
	long y = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	long z = stof24((sconst(1) - p1->z) / 2);

	x0 = x - rx; y0 = y - ry;
	x1 = x + rx; y1 = y + ry;

	if(x0 > x1) {x0 ^= x1; x1 ^= x0; x0 ^= x1;}
	if(y0 > y1) {y0 ^= y1; y1 ^= y0; y0 ^= y1;}

	dx = x1 - x0; dy = y1 - y0;
	x1 = x0 += dx >> 1; x0 +=dx & 1;
	dx += dx & 1; dy += dy & 1;
	sx = dx * dx; sy = dy * dy;
	r = sx * dy >> 2;
	dx = 0; dy = r << 1;

	g3_setpixel(s,x0,y0,z,c); g3_setpixel(s,x0,y1,z,c);
	g3_setpixel(s,x1,y0,z,c); g3_setpixel(s,x1,y1,z,c);

	while (y0 < y1) {
		if (r >= 0) {
			x0--; x1++;
			r -= dx += sy;
		}
		if (r < 0) {
			y0++; y1--;
			r += dy -= sx;
		}
		g3_setpixel(s,x0,y0,z,c); g3_setpixel(s,x0,y1,z,c);
		g3_setpixel(s,x1,y0,z,c); g3_setpixel(s,x1,y1,z,c);
	}
}

void g3_filloval(gx_Surf s, vecptr p1, long rx, long ry, long c) {
	long x0, x1, sx, dx = 0;
	long y0, y1, sy, dy = 0;
	long x, y, z, r;

	x = scatoi((SCRW-1) * (sconst(1) + p1->x) / 2);
	y = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	z = stof24((sconst(1) - p1->z) / 2);

	if (x < 0 || x > SCRW) return;
	if (y < 0 || y > SCRH) return;

	x0 = x - rx; y0 = y - ry;
	x1 = x + rx; y1 = y + ry;

	if(x0 > x1) {x0 ^= x1; x1 ^= x0; x0 ^= x1;}
	if(y0 > y1) {y0 ^= y1; y1 ^= y0; y0 ^= y1;}

	dx = x1 - x0; dy = y1 - y0;
	x1 = x0 += dx >> 1; x0 +=dx & 1;
	dx += dx & 1; dy += dy & 1;
	sx = dx * dx; sy = dy * dy;
	r = sx * dy >> 2;
	dx = 0; dy = r << 1;
	for (x = x0; x < x1; x += 1) g3_setpixel(s,x,y0,z,c);
	for (x = x0; x < x1; x += 1) g3_setpixel(s,x,y1,z,c);
	while (y0 < y1) {
		if (r >= 0) {
			x0--; x1++;
			r -= dx += sy;
		}
		if (r < 0) {
			y0++; y1--;
			r += dy -= sx;
		}
		if (y0 < 0 || y1 >= SCRH) continue;
		if (x0 < 0) x0 = 0;
		if (x1 > SCRW) x1 = SCRW;
		for (x = x0; x < x1; x += 1) g3_setpixel(s,x,y0,z,c);
		for (x = x0; x < x1; x += 1) g3_setpixel(s,x,y1,z,c);
	}
}

void g3_fill3gon(gx_Surf s, vector *p, texpos *t, argb *c, int i1, int i2, int i3) {
	argb (*tex2D)(gx_Surf, long, long) = 0;
	gx_Clip roi = gx_getclip(s);
	const float dbias = 0;
	register long tmp, y;
	long y1, y2, y3;
	long X1, X2, X3;
	long Z1, Z2, Z3;
	long s1, s2, s3;
	long t1, t2, t3;
	argb c1, c2, c3;
	struct {
		long x, dx, z, dz;
		long s, ds, t, dt;
		long r, dr, g, dg, b, db;
		#define incr(__v, __i) {\
			__v.x += __i * __v.dx;\
			__v.z += __i * __v.dz;\
			__v.s += __i * __v.ds;\
			__v.t += __i * __v.dt;\
			__v.r += __i * __v.dr;\
			__v.g += __i * __v.dg;\
			__v.b += __i * __v.db;}
	} v1, v2, *l, *r;

	if (s->depth != 32) return;

	if (p[i1].y < p[i3].y) {i1 ^= i3; i3 ^= i1; i1 ^= i3;}
	if (p[i1].y < p[i2].y) {i1 ^= i2; i2 ^= i1; i1 ^= i2;}
	if (p[i2].y < p[i3].y) {i2 ^= i3; i3 ^= i2; i2 ^= i3;}

	y1 = scatoi((SCRH-1) * (sconst(1) - p[i1].y) / 2);
	y2 = scatoi((SCRH-1) * (sconst(1) - p[i2].y) / 2);
	y3 = scatoi((SCRH-1) * (sconst(1) - p[i3].y) / 2);

	//~ if (p[i1].z >= 0 && p[i2].z >= 0 && p[i3].z >= 0) return;
	//~ if (p[i1].z > 0 || p[i2].z > 0 || p[i3].z > 0) return;

	if ((tmp = y3 - y1)>0) {				// draw nothing / clip (y)
		if (y3 < roi->ymin) return;
		if ((y = y1) > roi->ymax) return;
		X1 = stof16((SCRW-1) * (sconst(1) + p[i1].x) / 2);
		X2 = stof16((SCRW-1) * (sconst(1) + p[i2].x) / 2);
		X3 = stof16((SCRW-1) * (sconst(1) + p[i3].x) / 2);
		Z1 = stof24((sconst(1) - (p[i1].z + dbias)) / 2);
		Z2 = stof24((sconst(1) - (p[i2].z + dbias)) / 2);
		Z3 = stof24((sconst(1) - (p[i3].z + dbias)) / 2);
		if (t) {
			//~ tex2D = (argb (*)(gx_Surf, long, long)) gx_getpnear;
			tex2D = (argb (*)(gx_Surf, long, long)) gx_getpnlin;
			s1 = t[i1].s * img.width; t1 = t[i1].t * img.height;
			s2 = t[i2].s * img.width; t2 = t[i2].t * img.height;
			s3 = t[i3].s * img.width; t3 = t[i3].t * img.height;
		}
		if (c) c1 = c[i1], c2 = c[i2], c3 = c[i3];
		else c1.col = c2.col = c3.col = 0x0ffffff;

		v1.dx = (X3 - (v1.x = X1)) / tmp;
		v1.dz = (Z3 - (v1.z = Z1)) / tmp;
		v1.ds = (s3 - (v1.s = s1)) / tmp;
		v1.dt = (t3 - (v1.t = t1)) / tmp;
		v1.dr = ((c3.r-c1.r) <<16) / tmp;
		v1.dg = ((c3.g-c1.g) <<16) / tmp;
		v1.db = ((c3.b-c1.b) <<16) / tmp;
		v1.r = c1.r << 16;
		v1.g = c1.g << 16;
		v1.b = c1.b << 16;

		if ((tmp = roi->ymin - y) > 0) {
			incr(v1, tmp);
			y = roi->ymin;
		}
	} else return;
	if ((tmp = y2 - y1)>0) {				// y1 < y < y2
		int yy = y2 < roi->ymax ? y2 : roi->ymax;
		v2.dx = (X2 - (v2.x = X1)) / tmp;
		v2.dz = (Z2 - (v2.z = Z1)) / tmp;
		v2.ds = (s2 - (v2.s = s1)) / tmp;
		v2.dt = (t2 - (v2.t = t1)) / tmp;
		v2.dr = ((c2.r-c1.r) <<16) / tmp;
		v2.dg = ((c2.g-c1.g) <<16) / tmp;
		v2.db = ((c2.b-c1.b) <<16) / tmp;
		v2.r = c1.r << 16;
		v2.g = c1.g << 16;
		v2.b = c1.b << 16;

		if (v2.dx > v1.dx) {
			l = &v1;
			r = &v2;
		} else {
			l = &v2;
			r = &v1;
		}

		if ((tmp = y - y1) > 0) incr(v2, tmp);

		for ( ; y < yy; y++) {
			int x1 = l->x >> 16;
			int x2 = r->x >> 16;
			if ((tmp = x2 - x1) > 0) {
				long z0 = l->z, zs = (r->z - z0) / tmp;
				long s0 = l->s, ss = (r->s - s0) / tmp;
				long t0 = l->t, ts = (r->t - t0) / tmp;
				long r0 = l->r, rs = (r->r - r0) / tmp;
				long g0 = l->g, gs = (r->g - g0) / tmp;
				long b0 = l->b, bs = (r->b - b0) / tmp;
				unsigned long *zb;
				argb *cb;

				if (x2 > roi->xmax) x2 = roi->xmax;
				if (x1 < roi->xmin) {
					tmp = roi->xmin - x1;
					z0 += zs * tmp;
					s0 += ss * tmp;
					t0 += ts * tmp;
					r0 += rs * tmp;
					g0 += gs * tmp;
					b0 += bs * tmp;
					x1 = roi->xmin;
				}
				zb = &zBuff[SCRW*y + x1];
				//~ cb = &cBuff[SCRW*y + x1];
				cb = (argb*)gx_getpaddr(s, x1, y);
				while (x1 < x2) {
					if ((unsigned)z0 < *zb) {
						*zb = z0;
						if (tex2D) {
							argb tex = tex2D(&img, s0, t0);
							cb->b = b0 * tex.b >> 24;
							cb->g = g0 * tex.g >> 24;
							cb->r = r0 * tex.r >> 24;
						}
						else {
							cb->b = b0 >> 16;
							cb->g = g0 >> 16;
							cb->r = r0 >> 16;
						}
						//~ cb->b = tex.b;
						//~ cb->g = tex.g;
						//~ cb->r = tex.r;
					}// * /
					r0 += rs; g0 += gs; b0 += bs;
					s0 += ss; t0 += ts;
					z0 += zs; x1++;
					zb++; cb++;
				}
			}
			incr(v1, 1);
			incr(v2, 1);
		}
	}
	if ((tmp = y3 - y2)>0) {				// y2 < y < y3
		int yy = y3 < roi->ymax ? y3 : roi->ymax;
		v2.dx = (X3 - (v2.x = X2)) / tmp;
		v2.dz = (Z3 - (v2.z = Z2)) / tmp;
		v2.ds = (s3 - (v2.s = s2)) / tmp;
		v2.dt = (t3 - (v2.t = t2)) / tmp;
		v2.dr = ((c3.r-c2.r) <<16) / tmp;
		v2.dg = ((c3.g-c2.g) <<16) / tmp;
		v2.db = ((c3.b-c2.b) <<16) / tmp;
		v2.r = c2.r << 16;
		v2.g = c2.g << 16;
		v2.b = c2.b << 16;

		if ((tmp = y - y2) > 0) incr(v2, tmp);
		if (v2.x > v1.x) {
			l = &v1;
			r = &v2;
		} else {
			l = &v2;
			r = &v1;
		}
		for ( ; y < yy; y++) {
			int x1 = l->x >> 16;
			int x2 = r->x >> 16;
			if ((tmp = x2 - x1) > 0) {
				long z0 = l->z, zs = (r->z - z0) / tmp;
				long s0 = l->s, ss = (r->s - s0) / tmp;
				long t0 = l->t, ts = (r->t - t0) / tmp;
				long r0 = l->r, rs = (r->r - r0) / tmp;
				long g0 = l->g, gs = (r->g - g0) / tmp;
				long b0 = l->b, bs = (r->b - b0) / tmp;
				unsigned long *zb;
				argb *cb;

				if (x2 > roi->xmax) x2 = roi->xmax;
				if (x1 < roi->xmin) {
					tmp = roi->xmin - x1;
					z0 += zs * tmp;
					s0 += ss * tmp;
					t0 += ts * tmp;
					r0 += rs * tmp;
					g0 += gs * tmp;
					b0 += bs * tmp;
					x1 = roi->xmin;
				}
				zb = &zBuff[SCRW*y + x1];
				//~ cb = &cBuff[SCRW*y + x1];
				cb = (argb*)gx_getpaddr(s, x1, y);
				while (x1 < x2) {
					if ((unsigned)z0 < *zb) {
						*zb = z0;
						if (tex2D) {
							argb tex = tex2D(&img, s0, t0);
							cb->b = b0 * tex.b >> 24;
							cb->g = g0 * tex.g >> 24;
							cb->r = r0 * tex.r >> 24;
						}
						else {
							cb->b = b0 >> 16;
							cb->g = g0 >> 16;
							cb->r = r0 >> 16;
						}
					}// * /
					r0 += rs; g0 += gs; b0 += bs;
					s0 += ss; t0 += ts;
					z0 += zs; x1++;
					zb++; cb++;
				}
			}
			incr(v1, 1);
			incr(v2, 1);
		}
	}// * /
	#undef incr
}//*/

// plotmesh/drawmesh/fillmesh

void g3_drawmesh(gx_Surf s, camera_p cam) {		// this should be wireframe
	const long wire_col = 0xffffff;
	const long norm_col = 0xff0000;
	matrix proj, view;

	unsigned i;

	//~ scalar culllface;
	vector v[6], N, P;
	argb   c[3], *col = ((draw & fill) == flat) ? c : 0;
	texpos t[3], *tex = ((draw & fill) == text) ? t : 0;
	if (draw & lit_) col = c;
	c[0].col = 0xffffff;
	c[1].col = 0xffffff;
	c[2].col = 0xffffff;

	cammat(&view, cam);
	matmul(&view, &view, &viewM);
	matmul(&proj, &projM, &view);
	frustum_get(v, &proj);
	gx_setblock(s, 0, 0, 0x7fffffff, 0x7fffffff, 0);
	//~ for(i = 0; i < SCRW*SCRH; i+=1) cBuff[i] = 0x00000000;
	for(i = 0; i < SCRW*SCRH; i+=1) zBuff[i] = 0x00ffffff;
	for (i = 0; i < mesh.tricnt; i += 1) {
		long i1 = mesh.triptr[i].i1;
		long i2 = mesh.triptr[i].i2;
		long i3 = mesh.triptr[i].i3;
		mapvec(&v[0], &proj, &mesh.vtxptr[i1].pos);
		mapvec(&v[1], &proj, &mesh.vtxptr[i2].pos);
		mapvec(&v[2], &proj, &mesh.vtxptr[i3].pos);
		//~ culllface = backface(&N, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i3].pos);
		//~ culllface = backface(&N, v+0, v+1, v+2);
		if (draw & cull) {				// culling faces
			if (backface(0, v+0, v+1, v+2) <= 0) continue;
		}
		if (draw & lit_) {					// dinamic light
			if ((draw & smt_)) {
				c[0] = lit(&mesh.vtxptr[i1].pos, &mesh.vtxptr[i1].nrm, &cam->pos, &view);
				c[1] = lit(&mesh.vtxptr[i2].pos, &mesh.vtxptr[i2].nrm, &cam->pos, &view);
				c[2] = lit(&mesh.vtxptr[i3].pos, &mesh.vtxptr[i3].nrm, &cam->pos, &view);
			} else {
				centerpt(&P, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i3].pos);
				backface(&N, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i3].pos);
				c[0] = c[1] = c[2] = lit(&P, &N, &cam->pos, &view);
			}
		}
		switch (draw & fill) {
			case text : {
				t[0] = mesh.vtxptr[i1].tex;
				t[1] = mesh.vtxptr[i2].tex;
				t[2] = mesh.vtxptr[i3].tex;
				g3_fill3gon(s, v, tex, col, 0, 1, 2);
			} break;
			case flat : {
				if (!(draw & lit_)) {
					c[0] = vecrgb(&mesh.vtxptr[i1].nrm);
					c[1] = vecrgb(&mesh.vtxptr[i2].nrm);
					c[2] = vecrgb(&mesh.vtxptr[i3].nrm);
				}
				g3_fill3gon(s, v, 0, col, 0, 1, 2);
			} break;
			case wire : {
				g3_drawline(s, v+0, v+1, wire_col);
				g3_drawline(s, v+0, v+2, wire_col);
				g3_drawline(s, v+1, v+2, wire_col);
			} break;
			case plot : {
				g3_putpixel(s, v+0, c[0].col);
				g3_putpixel(s, v+1, c[1].col);
				g3_putpixel(s, v+2, c[2].col);
			} break;
		}
		if (draw & dspN) {					// normals
			//~ mapvec(v+3, &proj, vecadd(&N, &mesh.vtxptr[i1].pos, vecsca(&N, &mesh.vtxptr[i1].nrm, norm_sca)));
			//~ mapvec(v+4, &proj, vecadd(&N, &mesh.vtxptr[i2].pos, vecsca(&N, &mesh.vtxptr[i2].nrm, norm_sca)));
			//~ mapvec(v+5, &proj, vecadd(&N, &mesh.vtxptr[i3].pos, vecsca(&N, &mesh.vtxptr[i3].nrm, norm_sca)));
			mapvec(v+3, &proj, vecadd(&N, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i1].nrm));
			mapvec(v+4, &proj, vecadd(&N, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i2].nrm));
			mapvec(v+5, &proj, vecadd(&N, &mesh.vtxptr[i3].pos, &mesh.vtxptr[i3].nrm));
			g3_drawline(s, v+0, v+3, norm_col);
			g3_drawline(s, v+1, v+4, norm_col);
			g3_drawline(s, v+2, v+5, norm_col);
		}
	}
	if (draw & dspL) for (i = 0; i < sizeof(Lights) / sizeof(*Lights); i += 1) {
		vector lp;
		long siz, col = vecrgb(&Lights[i].ambi).col;
		mapvec(&lp, &proj, &Lights[i].lpos);
		//~ siz = (int)(SCRH * 1 * (1. / sqrt(lp.x*lp.x + lp.y*lp.y + lp.z*lp.z)));
		siz = (int)((1+lp.z) * SCRW);
		if (Lights[i].attr & L_on)
			g3_filloval(s, &lp, siz, siz, col);
		else
			g3_drawoval(s, &lp, siz, siz, col);
	}
	{// if (draw & dspB) // bounds
		vector c0, c1, c2, c3, c4, c5, c6, c7;
		void bboxMesh(vecptr, vecptr);
		bboxMesh(&c0, &c6);		// aabb

		c4.x = c7.x = c3.x = c0.x;
		c1.x = c2.x = c5.x = c6.x;

		c1.y = c5.y = c4.y = c0.y;
		c3.y = c2.y = c7.y = c6.y;

		c1.z = c2.z = c3.z = c0.z;
		c4.z = c5.z = c7.z = c6.z;

		mapvec(&c0, &proj, &c0);
		mapvec(&c1, &proj, &c1);
		mapvec(&c2, &proj, &c2);
		mapvec(&c3, &proj, &c3);
		mapvec(&c4, &proj, &c4);
		mapvec(&c5, &proj, &c5);
		mapvec(&c6, &proj, &c6);
		mapvec(&c7, &proj, &c7);

		// front
		g3_drawline(s, &c0, &c1, norm_col);
		g3_drawline(s, &c1, &c2, norm_col);
		g3_drawline(s, &c2, &c3, norm_col);
		g3_drawline(s, &c3, &c0, norm_col);

		// back
		g3_drawline(s, &c4, &c5, norm_col);
		g3_drawline(s, &c5, &c6, norm_col);
		g3_drawline(s, &c6, &c7, norm_col);
		g3_drawline(s, &c7, &c4, norm_col);

		// join
		g3_drawline(s, &c0, &c4, norm_col);
		g3_drawline(s, &c1, &c5, norm_col);
		g3_drawline(s, &c2, &c6, norm_col);
		g3_drawline(s, &c3, &c7, norm_col);

	}
}

// zoomsurf?
void blursurf(gx_Surf dst, int cnt) {
	int x, y;
	//~ if (dst->width != src->width) return;
	//~ if (dst->height != src->height) return;
	//~ if (dst->depth != src->depth || src->depth != 32) return;
	for (y = 1; y < dst->height - 1; y += 1) {
		long *zptr = (long*)&zBuff[y*SCRW];
		argb *dptr = (argb*)gx_getpaddr(dst, 1, y);
		argb *sptr = (argb*)gx_getpaddr(dst, 1, y);
		argb *sptm = (argb*)((char*)sptr - dst->scanLen);
		argb *sptp = (argb*)((char*)sptr + dst->scanLen);
		for (x = 1; x < dst->width - 1; x += 1) {
			//~ int r = sptm[x].r + sptr[x-1].r + sptr[x+1].r + sptp[x].r;
			//~ int g = sptm[x].g + sptr[x-1].g + sptr[x+1].g + sptp[x].g;
			//~ int b = sptm[x].b + sptr[x-1].b + sptr[x+1].b + sptp[x].b;
			unsigned r = (sptm[x-1].r + sptm[x].r + sptm[x+1].r + sptr[x-1].r + sptr[x+1].r + sptp[x-1].r + sptp[x].r + sptp[x+1].r) >> 3;
			unsigned g = (sptm[x-1].g + sptm[x].g + sptm[x+1].g + sptr[x-1].g + sptr[x+1].g + sptp[x-1].g + sptp[x].g + sptp[x+1].g) >> 3;
			unsigned b = (sptm[x-1].b + sptm[x].b + sptm[x+1].b + sptr[x-1].b + sptr[x+1].b + sptp[x-1].b + sptp[x].b + sptp[x+1].b) >> 3;
			//~ if ((r -= cnt) < 0) dptr[x].r = 0; else dptr[x].r = r >> 3;
			//~ if ((g -= cnt) < 0) dptr[x].g = 0; else dptr[x].g = g >> 3;
			//~ if ((b -= cnt) < 0) dptr[x].b = 0; else dptr[x].b = b >> 3;
			cnt = (zptr[x] >> 10) & 0xff;
			dptr[x].r = sptr[x].r + (cnt * (r - sptr[x].r) >> 8);
			dptr[x].g = sptr[x].g + (cnt * (g - sptr[x].g) >> 8);
			dptr[x].b = sptr[x].b + (cnt * (b - sptr[x].b) >> 8);
		}
	}
}

void zsurf(gx_Surf dst) {
	int x, y, z;
	//~ for(i = 0; i < SCRW*SCRH; i+=1) s->cBuff[i] = zBuff[i];
	//~ if (dst->width != src->width) return;
	//~ if (dst->height != src->height) return;
	//~ if (dst->depth != src->depth || src->depth != 32) return;
	for (y = 0; y < dst->height; y += 1) {
		argb *dptr = (argb*)gx_getpaddr(dst, 1, y);
		argb *sptr = (argb*)&zBuff[y*SCRW];
		for (x = 0; x < dst->width; x += 1) {
			z = sptr[x].col >> 13;
			//~ if (z < 0x3f00) z = 255;
			//~ if ((z = (sptr[x].col >> 10)) > 255);// z = 255;
			dptr[x].r = dptr[x].g = dptr[x].b = ~z;
			//~ dptr[x].r = (sptr[x].col >> 16) & 0xff;
			//~ dptr[x].g = (sptr[x].col >> 16) & 0xff;
			//~ dptr[x].b = (sptr[x].col >> 16) & 0xff;
		}
	}
}

//{-8<------------  MAIN  ------------------------------------------------------
#include "3d_surf.c"
#include "3d_mesh.c"

enum {
	rat_mov = 1 << 0,
	rat_lbp = 1 << 1,
	rat_lbr = 1 << 2,
	rat_rbp = 1 << 3,
	rat_rbr = 1 << 4,
	rat_rbc = rat_rbp | rat_rbr,
	rat_mbp = 1 << 5,
	rat_mbr = 1 << 6,
};

camera cam;
vector eye, tgt, up;
scalar S = sconst(20);
int act = 0;

void kbdHND(int lkey, int key) {
	const scalar d = sconst(1.);
	const scalar D = sconst(.2);
	switch (key) {
		case  27 : act = -1; break;
		case  13 : camset(&cam, &eye, &tgt, &up); break;
		case 'w' : cammov(&cam, &cam.dirF, +d); break;
		case 'W' : cammov(&cam, &cam.dirF, +D); break;
		case 's' : cammov(&cam, &cam.dirF, -d); break;
		case 'S' : cammov(&cam, &cam.dirF, -D); break;
		case 'd' : cammov(&cam, &cam.dirR, +d); break;
		case 'D' : cammov(&cam, &cam.dirR, +D); break;
		case 'a' : cammov(&cam, &cam.dirR, -d); break;
		case 'A' : cammov(&cam, &cam.dirR, -D); break;
		case ' ' : cammov(&cam, &cam.dirU, +D); break;
		case 'c' : cammov(&cam, &cam.dirU, -D); break;

		case '`' : {
			//~ FILE* pgm = fopen("screen.pgm", "wb");
			//~ fprintf(pgm, "P5\n# Mandelbrot set\n");
			//~ fprintf(pgm, "%3d %3d\n%3d\n", SCRW, SCRH, 255);
			//~ fwrite(cBuff, 1, SCRW*SCRH, pgm);
			//~ fclose(pgm);
		} break;
		case '~' : {
			//~ FILE* f = fopen("pos.txt", "w");
			//~ fprintf(f, "x : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.xx), scatof(worldM.xy), scatof(worldM.xz), scatof(worldM.xt));
			//~ fprintf(f, "y : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.yx), scatof(worldM.yy), scatof(worldM.yz), scatof(worldM.yt));
			//~ fprintf(f, "z : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.zx), scatof(worldM.zy), scatof(worldM.zz), scatof(worldM.zt));
			//~ fprintf(f, "w : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.wx), scatof(worldM.wy), scatof(worldM.wz), scatof(worldM.wt));
			//~ fprintf(f, "e : %+.3f, %+.3f, %+.3f, %+f\n", scatof(eye.x), scatof(eye.y), scatof(eye.z), scatof(eye.w));
			//~ fclose(f);
		} break;
		case '\\': optimesh(); break;
		case 'X' : matidn(&viewM); break;//camset(&cam, &eye, &tgt, &up); projv_mat(&projM, 90, (SCRW+.0)/SCRH, 10, 1000, 0); break;
		//~ case 'x' : vecldf(&eye, S, 0, 0, 1); vecldf(&up, 0, 1, 0, 1); camset(&cam, &eye, &tgt, &up); break;
		//~ case 'y' : vecldf(&eye, 0, S, 0, 1); vecldf(&up, 0, 0, 1, 1); camset(&cam, &eye, &tgt, &up); break;
		//~ case 'z' : vecldf(&eye, 0, 0, S, 1); vecldf(&up, 0, 1, 0, 1); camset(&cam, &eye, &tgt, &up); break;

		case '\t': draw = (draw & ~fill) | ((draw + 1) & fill); break;
		//~ case 't' : draw ^= tex_; break;
		case 'l' : draw ^= lit_; break;
		case 'g' : draw ^= smt_; break;
		//~ case 'r' : draw ^= refl; break;
		case '/' : draw ^= cull; break;

		case 'N' : draw ^= dspN; break;
		case 'L' : draw ^= dspL; break;

		case '1' : Lights[0].attr ^= L_on; break;
		case '2' : Lights[1].attr ^= L_on; break;

		case '?' : draw ^= dspZ; break;
		//~ case 'm': if (mapvec == mapvecd) mapvec = mapvecm; else mapvec = mapvecd; break;
	}
}

int mx, my, mdx=0, mdy=0, btn = 0;

void mousehnd(int x, mouseinf* m) {
	static int ox, oy;
	mx = m->posx; my = m->posy;
	mdx = m->relx - ox; ox = m->relx;
	mdy = m->rely - oy; oy = m->rely;
	btn = m->button;
	act = x;
}

void coor(float sx, float sy, float sz) {
	int o = 0, x = 1, y = 2, z = 3;
	vecldf(&mesh.vtxptr[o].pos, 0., 0., 0., 1.);		// O
	vecldf(&mesh.vtxptr[x].pos, sx, 0., 0., 1.);		// X
	vecldf(&mesh.vtxptr[y].pos, 0., sy, 0., 1.);		// Y
	vecldf(&mesh.vtxptr[z].pos, 0., 0., sz, 1.);		// Z
	mesh.triptr[0].i1 = o; mesh.triptr[0].i2 = x; mesh.triptr[0].i3 = y;
	mesh.triptr[1].i1 = o; mesh.triptr[1].i2 = y; mesh.triptr[1].i3 = z;
	mesh.triptr[2].i1 = o; mesh.triptr[2].i2 = x; mesh.triptr[2].i3 = z;

	mesh.vtxcnt = 4;
	mesh.tricnt = 3;
}

const int division = 32;
struct gx_Surf_t offs;		// offscreen
//~ struct gx_Surf_t blur;
//~ struct gx_Surf_t vga0;
struct gx_Surf_t cur1;		// cursor
struct gx_Surf_t fnt1;		// font
struct {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char a;
} pal[256];
extern void colcpy_lum32();
extern void colcpy_xtr32();

int main() {
	struct {
		time_t	sec;
		time_t	cnt;
		long	col;
		char	str[4];
		long	len;
	} fps;
	char str[256];
	//~ double vtx[3];
	//~ int x,y,z,o;
	int e;//, d;

	k3d_open();

	printf ("reading tetx : %d\n", gx_loadBMP(&img, "text/earth2.bmp", 32));
	//~ mesheval(evalP_sphere, division, division, 0);
	//~ mesheval(evalP_tours, division, division, 3);
	//~ mesheval(evalP_apple, division, division, 3);
	//~ mesheval(evalP_001, division, division, 0);
	/*
	vtx[0] = 0; vtx[1] = 0; vtx[2] = 0; 
	o = addvtxdv(vtx,0,0);
	vtx[0] = 1; vtx[1] = 0; vtx[2] = 0; 
	x = addvtxdv(vtx,0,0);
	vtx[0] = 0; vtx[1] = y; vtx[2] = 0; 
	y = addvtxdv(vtx,0,0);
	vtx[0] = 0; vtx[1] = 0; vtx[2] = 1; 
	z = addvtxdv(vtx,0,0);
	
	//~ addtri(o,x,y);
	//~ */

	printf ("reading mesh : %d\n", readf1("mesh/rabbit"));
	//~ printf ("reading mesh : %d\n", readf("mesh/tp.txt"));

	//~ missing
	//~ printf ("reading tetx : %d\n", gx_loadBMP(&img, "text/scorpion.bmp", 32));
	//~ printf("reading mesh : %d\n", readMobj("mesh/scorpion.obj"));

	//~ printf ("reading tetx : %d\n", gx_loadBMP(&img, "text/manta.bmp", 32));
	//~ printf("reading mesh : %d\n", readMobj("mesh/manta.obj"));

	//~ printf ("reading tetx : %d\n", gx_loadBMP(&img, "text/raptor.bmp", 32));
	//~ printf("reading mesh : %d\n", readMobj("mesh/raptor.obj"));

	//~ printf ("reading tetx : %d\n", gx_loadBMP(&img, "text/???.bmp", 32));
	//~ printf("reading mesh : %d\n", readMobj("mesh/???.obj"));

	//~ printf("vtx cnt : %d / %d\n", mesh.vtxcnt, mesh.maxvtx);
	//~ printf("tri cnt : %d / %d\n", mesh.tricnt, mesh.maxtri);
	centmesh();
	//~ tranMesh(matldT(&projM, vecldf(&tgt,  10,  10, 0, 0), 1));
	//~ tranMesh(matldS(&projM, vecldf(&tgt,  10,  10, 10, 0), 1));
	//~ printf("%d\n", saveMesh("scorpion.msh")); k3d_exit(); getch(); return 0;
	//~ optimesh();
	//~ coor(S,S,S);

	printf("vtx cnt : %d / %d\n", mesh.vtxcnt, mesh.maxvtx);
	printf("tri cnt : %d / %d\n", mesh.tricnt, mesh.maxtri);
	bboxMesh(&tgt, &eye);
	printf("box min : %f, %f, %f\n", tgt.x, tgt.y, tgt.z);
	printf("box max : %f, %f, %f\n", eye.x, eye.y, eye.z);
	printf("press any key ...\n");

	//~ vecsub(&tgt, &tgt, &eye);
	vecabs(&tgt, &tgt);
	S = tgt.x;
	if (S < tgt.y) S = tgt.y;
	if (S < tgt.z) S = tgt.z;

	tranMesh(matldS(&projM, vecldf(&up, 1, 1, 1, 0), 20./S));

	bboxMesh(&tgt, &eye);
	printf("box min : %f, %f, %f\n", tgt.x, tgt.y, tgt.z);
	printf("box max : %f, %f, %f\n", eye.x, eye.y, eye.z);

	S= 10.;
	matidn(&viewM);
	vecldf(&up,   0,  1,  0, 1);
	vecldf(&tgt,  0,  0,  0, 1);
	vecldf(&eye,  S,  S,  S, 1);
	camset(&cam, &eye, &tgt, &up);
	projv_mat(&projM, 45, (SCRW+.0)/SCRH, 1, 1000, 0);

	if (getch()== 27) {
		k3d_exit();
		return 0;
	}
#if MCGA
#if (SCRW != 320) || (SCRH != 200)
#error (SCRW != 320) || (SCRH != 200)
#endif
	for (e = 0; e < 256; e += 1) {
		pal[e].r = e;
		pal[e].g = e;
		pal[e].b = e;
	}
	MCGA_open();
	setpalette(pal);
#else
	if ((e = gx_vbeinit())) {
		printf("Cannot open vbe %x.%04x\n", e >> 16, e & 0XFFFF);
		getch();
		exit(1);
	}
	if ((e = gx_modeopen(&offs, SCRW, SCRH, SCRD, 0))) {
		gx_vbeexit();
		printf("Cannot open mode %x.%04x\n", e >> 16, e & 0XFFFF);
		getch();
		exit(1);
	}
#endif
	offs.width = SCRW;
	offs.height = SCRH;
	offs.depth = 32;
	offs.scanLen = SCRW*4;
	if ((e = gx_initSurf(&offs, 0, 0, 0))) {
		printf("Cannot attach surf : %d\n", e);
		printf("surf.clip = %p\n", offs.clipPtr);
		printf("surf.Xres = %d\n", offs.width);
		printf("surf.Yres = %d\n", offs.height);
		printf("surf.Cres = %x\n", offs.depth);
		printf("surf.flag = %x\n", offs.flags);
		printf("surf.Plen = %d\n", offs.pixeLen);
		printf("surf.Slen = %lu\n", offs.scanLen);
		printf("surf.clut = %p\n", offs.clutPtr);
		printf("surf.base = %p\n", offs.basePtr);
		printf("surf.offs = %p\n", offs.offsPtr);
		printf("surf.move = %p\n", offs.movePtr);
		gx_doneSurf(&offs);
		getch();
		exit(1);
	}
	if ((e = gx_loadFNT(&fnt1, "antique.fnt"))) {
		gx_doneSurf(&offs);
		gx_doneSurf(&fnt1);
		printf("Cannot open font %d : antique.fnt\n",e);
		getch();
		exit(1);
	}// */
	if ((e = gx_loadCUR(&cur1, "arrow.cur", 1))) {
		gx_doneSurf(&offs);
		gx_doneSurf(&fnt1);
		gx_doneSurf(&cur1);
		printf("Cannot open cursor %d : arrow.cur\n",e);
		getch();
		exit(1);
	}// */
	initMouse(mousehnd, -1);
	bndMouse(&rec);
	setMouse(SCRW / 2, SCRH / 2);

	for(fps.cnt = 0, fps.sec = time(NULL); ; fps.cnt++) {
		if (kbhit()) kbdHND(0, getch());
		if (act == -1) break;
		if (act & rat_mov) {
			const float MOUSEROT = 1. *(3.14 / 360);
			const float MOUSEMOV = 8. *(1.00 / 10);
			if (btn == 0) {
				matrix rot;
				if (mdy) matmul(&viewM, &viewM, matldR(&rot, &viewM.x, scaldf(mdy * (MOUSEROT))));
				if (mdx) matmul(&viewM, &viewM, matldR(&rot, &viewM.y, scaldf(mdx * (MOUSEROT))));
				vecnrm(&viewM.x, &viewM.x);
				vecnrm(&viewM.y, &viewM.y);
				vecnrm(&viewM.z, &viewM.z);
			}
			else if (btn == 1) {
				if (mdy) cammov(&cam, &cam.dirF, scaldf(mdy * MOUSEMOV));		// move
				if (mdx) camrot(&cam, &cam.dirU, scaldf(mdx * MOUSEROT));		// turn
			}
			else if (btn == 2) {
				if (mdy) camrot(&cam, &cam.dirR, scaldf(mdy * MOUSEROT));		// pitch : up/down
				if (mdx) camrot(&cam, &cam.dirU, scaldf(mdx * MOUSEROT));		// move
			}
			else if (btn == 3) {
				if (mdx) cammov(&cam, &cam.dirR, scaldf(mdx * MOUSEMOV));
				if (mdy) cammov(&cam, &cam.dirU, scaldf(mdy * MOUSEMOV));
			}
			//~ if (mdx) camrot(&cam, &up, scaldf(mdx * MOUSEROT));				// turn (fly) : left/rigth
			//~ if (mdx) camrot(&cam, &cam.dirU, scaldf(mdx * MOUSEROT));		// turn (walk): left/rigth
			//~ if (mdy) camrot(&cam, &cam.dirR, scaldf(mdy * MOUSEROT));		// pitch : up/down
			//~ if (mdz) camrot(&cam, &cam.dirF, scaldf(mdz * MOUSEROT));		// roll
			mdy = mdx = 0;

		}

		g3_drawmesh(&offs, &cam);
		//~ zBuff[SCRW/2 + SCRH*SCRH/2] = 0;
		//~ d = zBuff[SCRW/2+SCRW*SCRH/2];
		//~ zBuff[SCRW/2+SCRW*SCRH/2] = 0;
		//~ if (draw & dspZ) zsurf(&offs);
		//~ blursurf(&offs, 256);
		/*
		gx_copysurf(&offs, 0, 0, &img, 0);
		for (e = 0; e < mesh.tricnt; e += 1) {
			int i1 = mesh.triptr[e].i1;
			int i2 = mesh.triptr[e].i2;
			int i3 = mesh.triptr[e].i3;
			gx_drawline(&offs, mesh.vtxptr[i1].tex.s * img.width >> 16, mesh.vtxptr[i1].tex.t * img.height >> 16, mesh.vtxptr[i2].tex.s * img.width >> 16, mesh.vtxptr[i2].tex.t * img.height >> 16, 0xffffff);
			gx_drawline(&offs, mesh.vtxptr[i1].tex.s * img.width >> 16, mesh.vtxptr[i1].tex.t * img.height >> 16, mesh.vtxptr[i3].tex.s * img.width >> 16, mesh.vtxptr[i3].tex.t * img.height >> 16, 0xffffff);
			gx_drawline(&offs, mesh.vtxptr[i2].tex.s * img.width >> 16, mesh.vtxptr[i2].tex.t * img.height >> 16, mesh.vtxptr[i3].tex.s * img.width >> 16, mesh.vtxptr[i3].tex.t * img.height >> 16, 0xffffff);
		}
		// */
		if (fps.sec != time(NULL)) {
			fps.col = fps.cnt < 30 ? 0xff0000 : fps.cnt < 50 ? 0xffff00 : 0x00ff00;
			sprintf(fps.str, "%u", (unsigned)fps.cnt);
			fps.len = strlen(fps.str)+3;
			fps.sec = time(NULL);
			fps.cnt = 0;
		}
		//~ gx_setblock(&offs, 0, 0, 0x7fffffff, 0x7fffffff, 0);
		//~ gx_setpixel(&offs, SCRW/2, SCRH/2, 0);
		//~ d = zBuff[SCRW/2 + SCRH*SCRH/2];
		sprintf(str, "%08x", (int)zBuff[SCRW/2 + SCRH*SCRH/2]);
		gx_drawText(&offs, SCRW - fps.len*8, 0, &fnt1, fps.str, fps.col);
		gx_drawText(&offs, SCRW - 9*8, 16, &fnt1, str, 0xffffffff);

		//~ blursurf(&offs, 256);
		//~ gx_drawCurs(&offs, ox, oy, &cur1, CURS_PUT);				// show mask// noo need to hide
		//~ gx_drawCurs(&offs, ox = mx, oy = my, &cur1, CURS_GET+CURS_MIX);		// show curs
		#if MCGA
		//~ waitretrace();
		//~ gx_callcbltf(colcpy_lum32, (void*)0xA0000, offs.basePtr, 320*200, NULL);
		//~ gx_callcbltf(colcpy_xtr32, (void*)0xA0000, zBuff, 320*200, (void*)3);
		gx_callcbltf(colcpy_xtr32, (void*)0xA0000, offs.basePtr, 320*200, (void*)1);
		#else
		gx_vgacopy(&offs, 0);
		#endif
	}
	k3d_exit();
	exitMouse();
	gx_doneSurf(&cur1);
	gx_doneSurf(&fnt1);
	gx_doneSurf(&offs);
	gx_doneSurf(&img);
	#if MCGA
	MCGA_exit();
	#else
	gx_vbeexit();
	#endif
	return 0;
}

//}-8<--------------------------------------------------------------------------
