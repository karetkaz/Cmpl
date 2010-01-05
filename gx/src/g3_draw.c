#include <math.h>
#include <time.h>
#include <stdio.h>
//~ #include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"

typedef float scalar;
#include "g3_math.c"

#define toRad(__A) ((__A) * (3.14159265358979323846 / 180))

extern double N;

enum {
	//~ draw_next = 0x0001,
	draw_mode = 0x0003,		// mask
	//~ draw_text = 0x0003,		//
	draw_none = 0x0000,		//
	draw_plot = 0x0001,		//
	draw_wire = 0x0002,		//
	draw_fill = 0x0003,		//

	draw_tex = 0x0004,		// use texture
	draw_lit = 0x0008,		// use lights

	cull_mode = 0x0030,		// enable culling
	cull_none = 0x0000,		// no cull
	cull_front= 0x0010,		// back face
	cull_back = 0x0020,		// front face
	cull_all  = 0x0030,		// back & front face


	//~ pcol_mode = 0x00300,
	//~ pcol_nrmc = 0x00100,
	//~ pcol_texc = 0x00200,
	//~ pcol_texl = 0x00300,
	//~ pcol_litc = 0x00300,

	disp_lght = 0x00010000,		// lights
	disp_norm = 0x00020000,		// normals
	disp_bbox = 0x00040000,		// Bounding Box
	disp_zbuf = 0x00080000,
	disp_info = 0x00100000,
	disp_mesh = 0x00200000,
};

//~ int draw = draw_fill|litMode|disp_info;
int draw = disp_mesh | draw_fill | cull_front | disp_info;

typedef union texcol {
	struct {
		unsigned short s;
		unsigned short t;
	};
	struct {
		unsigned short s;
		unsigned short t;
	} tex;
	struct {
		unsigned char b;
		unsigned char g;
		unsigned char r;
		unsigned char a;
	};
	unsigned long val;
	argb rgb;
} *texcol;

typedef struct {		// material
	union vector ambi;		// Ambient
	union vector diff;		// Diffuse
	union vector spec;		// Specular
	scalar spow;		// Shin...
	union vector emis;		// Emissive
	struct gx_Surf tex;	// texture map
} material, *mtlptr;

#define lz 2
#define lp 2
#define lA 10

static struct lght {		// lights
	enum {						// Type
		L_off  = 0x0000,		// light is off
		L_on   = 0x0001,		// light is on
		//~ L_attn = 0x0002,		// attenuate
		//~ L_dir  = 0x0004,		// is directional
		//~ L_spot = 0x0008,		// is directional
	} attr;
	union vector	ambi;		// Ambient
	union vector	diff;		// Diffuse
	union vector	spec;		// Specular
	union vector	attn;		// Attenuation
	union vector	pos;		// position
	union vector	dir;		// direction
	scalar	sCos;
	scalar	sExp;
	struct lght*	next;
}
Lights[] = {
	{	// light0(Gray)
		L_on,
		{{.6, .6, .6, 1}},	// ambi
		{{.9, .9, .9, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., 0., 0., 0}},	// attn
		//~ {{-2, +15, -15, 1}},	// pos
		{{+lp, +lp, +lz, 1}},	// pos
		{{0., 0., 0., 0}},	// dir
		0, 32}, // */
	{	// light1(RGB/R)
		L_off,
		{{.4, .0, .0, 1}},	// ambi
		{{.8, .0, .0, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., 0., 0., 0}},	// attn
		{{+lp, -lp, +lz, 1}},	// pos
		{{-0., +0., -0., 0}},	// dir // is normalized in main()
		lA, 32}, // */
	{	// light2(RGB/G)
		L_off,
		{{0., .4, .0, 1}},	// ambi
		{{.0, .8, .0, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., .0, 0., 0}},	// attn
		{{-lp, -lp, +lz, 1}},	// pos
		{{-0., +0., -0., 0}},	// dir // is normalized in main()
		lA, 32}, // */
	{	// light3(RGB/B)
		L_off,
		{{.0, .0, .4, 1}},	// ambi
		{{.0, .0, .8, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., .0, 0., 0}},	// attn
		{{-lp, +lp, +lz, 1}},	// pos
		{{-0., +0., -0., 0}},	// dir // is normalized in main()
		lA, 32},
};

material defmtl[] = {
	{//  0 Brass
		{0.32941, 0.223529, 0.027451, 1.0},
		{0.780392,  0.568627, 0.113725, 1.0},
		{0.992157, 0.941176, 0.807843, 1.0},
		27.8974
	},
	{//  1 Bronze
		{0.2125, 0.1275, 0.054, 1.0},
		{0.714, 0.4284, 0.18144, 1.0},
		{0.393548, 0.271906, 0.166721},
		25.6
	},
	{//  2 Polished Bronze
		{0.25, 0.148, 0.06475, 1.0},
		{0.4, 0.2368, 0.1036, 1.0},
		{0.774597, 0.458561, 0.200621, 1.0},
		76.8
	},
	{//  3 Chrome
		{0.25, 0.25, 0.25, 1.0},
		{0.4, 0.4, 0.4, 1.0},
		{0.774597, 0.774597, 0.774597, 1.0},
		76.8
	},
	{//  4 Copper
		{0.19125, 0.0735, 0.0225, 1.0},
		{0.7038, 0.27048, 0.0828, 1.0},
		{0.256777, 0.137622, 0.086014, 1.0},
		12.8
	},
	{//  5 Polished Copper
		{0.2295, 0.08825, 0.0275, 1.0},
		{0.5508, 0.2118, 0.066, 1.0},
		{0.580594, 0.223257, 0.0695701, 1.0},
		51.2
	},
	{//  6 Gold
		{0.24725, 0.1995, 0.0745, 1.0},
		{0.75164, 0.60648, 0.22648, 1.0},
		{0.628281, 0.555802, 0.366065, 1.0},
		51.2
	},
	{//  7 Polished Gold
		{0.24725, 0.2245, 0.0645, 1.0},
		{0.34615, 0.3143,0.0903, 1.0},
		{0.797357, 0.723991, 0.208006, 1.0},
		83.2
	},
	{//  8 Pewter
		{0.105882, 0.058824, 0.113725,1.0},
		{0.427451, 0.470588, 0.541176,1.0},
		{0.333333, 0.333333, 0.521569, 1.0},
		9.84615
	},
	{//  9 Silver
		{0.19225, 0.19225, 0.19225, 1.0},
		{0.50754, 0.50754, 0.50754, 1.0},
		{0.508273, 0.508273, 0.508273, 1.0},
		51.2
	},
	{// 10 Polished Silver
		{0.23125, 0.23125, 0.23125, 1.0},
		{0.2775, 0.2775, 0.2775, 1.0},
		{0.773911, 0.773911, 0.773911, 1.0},
		89.6
	},
	{// 11 Emerald
		{0.0215, 0.1745, 0.0215, 0.55},
		{0.07568, 0.61424, 0.07568, 0.55},
		{0.633, 0.727811, 0.633, 0.55},
		76.8
	},
	{// 12 Jade
		{0.135,0.2225,0.1575,0.95},
		{0.54, 0.89, 0.63, 0.95},
		{0.316228, 0.316228, 0.316228, 0.95},
		12.8
	},
	{// 13 Obsidian
		{0.05375,0.05 ,0.06625,0.82},
		{0.18275,0.17, 0.22525, 0.82},
		{0.332741,0.328634,0.346435,0.82},
		38.4
	},
	{// 14 Pearl
		{0.25,0.20725,0.20725,0.922 },
		{1.0, 0.829, 0.829,0.922},
		{0.296648,0.296648,0.296648, 0.922},
		11.264
	},
	{// 15 Ruby
		{0.1745,0.01175 ,0.01175,0.55 },
		{0.61424,0.04136, 0.04136,0.55 },
		{0.727811, 0.626959, 0.626959,0.55},
		76.8
	},
	{// 16 Turquoise
		{0.1, 0.18725, 0.1745, 0.8},
		{0.396,0.74151,0.69102, 0.8},
		{0.297254, 0.30829,  0.306678, 0.8},
		12.8
	},
	{// 17 Black Plastic
		{0.0, 0.0, 0.0, 1.0},
		{0.01, 0.01, 0.01, 1.0},
		{0.5, 0.5, 0.5, 1.0},
		32
	},
	{// 18 Black Rubber
		{0.02, 0.02, 0.02, 1.0},
		{0.01, 0.01, 0.01, 1.0},
		{0.4, 0.4, 0.4, 1.0},
		10
	}
};

#define lights (sizeof(Lights) / sizeof(*Lights))

/* frustum
void frustum_get(union vector planes[6], matrix mat) {
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

int ftest_sphere(union vector planes[6], vector p) {			// clip sphere
	scalar r = -p->w;
	if (vecdph(&planes[0], p) <= r) return 0;
	if (vecdph(&planes[1], p) <= r) return 0;
	if (vecdph(&planes[2], p) <= r) return 0;
	if (vecdph(&planes[3], p) <= r) return 0;
	if (vecdph(&planes[4], p) <= r) return 0;
	if (vecdph(&planes[5], p) <= r) return 0;
	return 1;	// inside
}

int ftest_point(union vector planes[6], vector p) {			// clip point
	const scalar r = 0;
	if (vecdph(p, &planes[0]) <= r) return 0;
	if (vecdph(p, &planes[1]) <= r) return 0;
	if (vecdph(p, &planes[2]) <= r) return 0;
	if (vecdph(p, &planes[3]) <= r) return 0;
	if (vecdph(p, &planes[4]) <= r) return 0;
	if (vecdph(p, &planes[5]) <= r) return 0;
	return 1;	// inside
}

vector bsphere(vector s, vector p1, vector p2, vector p3) {
	scalar	len, ln1, ln2;
	union vector	tmp;
	//~ union vector	min, max;
	//~ vecmax(&max, vecmax(&tmp, p1, p2), p3);					// minimum
	//~ vecmin(&min, vecmin(&tmp, p1, p2), p3);					// maximum
	//~ vecsca(s, vecadd(&tmp, &min, &max), sconst(1./2));		// Box center
	vecadd(&tmp, vecadd(&tmp, p1, p2), p3);
	vecsca(s, &tmp, 1. / 3);									// Baricentrum;

	len = vecdp3(&tmp, vecsub(&tmp, s, p1));
	ln1 = vecdp3(&tmp, vecsub(&tmp, s, p2));
	ln2 = vecdp3(&tmp, vecsub(&tmp, s, p3));
	if (len < ln1) len = ln1;
	if (len < ln2) len = ln2;
	s->w = sqrt(len);
	return s;
}

vector centerpt(vector P, vector p1, vector p2, vector p3) {
	vecadd(P, vecadd(P, p1, p2), p3);
	vecsca(P, P, 1./3);
	return P;
}
// */

scalar backface(vector N, vector p1, vector p2, vector p3) {
	union vector e1, e2, tmp;
	if (!N) N = &tmp;
	vecsub(&e1, p2, p1);
	vecsub(&e2, p3, p1);
	vecnrm(N, veccrs(N, &e1, &e2));
	N->w = 0;
	return vecdp3(N, p1);
}// */

vector mappos(vector dst, matrix mat, vector src) {
	union vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);

	dst->w = vecdph(&mat->w, src);
	if (dst->w != 0) dst->w = 1.0 / dst->w;
	dst->x = vecdph(&mat->x, src) * dst->w;
	dst->y = vecdph(&mat->y, src) * dst->w;
	dst->z = vecdph(&mat->z, src) * dst->w;
	return dst;
}

vector mapnrm(vector dst, matrix mat, vector src) {
	union vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);
	//~ dst->w = vecdph(&mat->w, src);
	//~ if (dst->w != 0) dst->w = 1.0 / dst->w;
	dst->x = vecdp3(&mat->x, src);// * dst->w;
	dst->y = vecdp3(&mat->y, src);// * dst->w;
	dst->z = vecdp3(&mat->z, src);// * dst->w;
	return dst;
}

//~ ----------------------------------------------------------------------------
static inline float absf(float a) {return a < 0 ? -a : a;}
static inline float satf(float a) {return a < 0 ? 0 : a > 1 ? 1 : a;}

inline argb fltrgb(scalar src) {
	argb res;

	//~ scalar r = src->r < -1 ? 0 : src->r > 1 ? 1 : ((src->r + 1) / 2);
	//~ scalar g = src->g < -1 ? 0 : src->g > 1 ? 1 : ((src->g + 1) / 2);
	//~ scalar b = src->b < -1 ? 0 : src->b > 1 ? 1 : ((src->b + 1) / 2);
	//~ scalar r = satf(src->r);
	//~ scalar g = satf(src->g);
	//~ scalar b = satf(src->b);
	//~ scalar r = satf(absf(src->r));
	//~ scalar g = satf(absf(src->g));
	//~ scalar b = satf(absf(src->b));
	//~ scalar r = satf((src->r + 1) / 2);
	//~ scalar g = satf((src->g + 1) / 2);
	//~ scalar b = satf((src->b + 1) / 2);
	//~ scalar r = src->r < 0 ? 0 : src->r > 1 ? 1 : src->r;
	//~ scalar g = src->g < 0 ? 0 : src->g > 1 ? 1 : src->g;
	//~ scalar b = src->b < 0 ? 0 : src->b > 1 ? 1 : src->b;
	res.r = satf(absf(src)) * 255;
	res.g = satf(absf(src)) * 255;
	res.b = satf(absf(src)) * 255;
	return res;
}

inline argb nrmrgb(vector src) {
	argb res;
	//~ scalar r = src->r < -1 ? 0 : src->r > 1 ? 1 : ((src->r + 1) / 2);
	//~ scalar g = src->g < -1 ? 0 : src->g > 1 ? 1 : ((src->g + 1) / 2);
	//~ scalar b = src->b < -1 ? 0 : src->b > 1 ? 1 : ((src->b + 1) / 2);

	//~ scalar r = src->r;
	//~ scalar g = src->g;
	//~ scalar b = src->b;

	//~ scalar r = absf(src->r);
	//~ scalar g = absf(src->g);
	//~ scalar b = absf(src->b);

	scalar r = (src->r + 1.) * .5;
	scalar g = (src->g + 1.) * .5;
	scalar b = (src->b + 1.) * .5;

	res.r = satf(r) * 255;
	res.g = satf(g) * 255;
	res.b = satf(b) * 255;
	return res;
}

inline argb torgb(long val) {
	argb res;
	res.val = val;
	return res;
}

#define scatoi(__VAL, __FIX) ((long)((__VAL) * (1 << (__FIX))))
static inline long projx(gx_Surf dst, vector p, int sca) {return scatoi((dst->width - 1) * (1 + p->x) / 2, sca);}
static inline long projy(gx_Surf dst, vector p) {return scatoi((dst->height - 1) * (1 - p->y) / 2, 0);}
static inline long projz(gx_Surf dst, vector p) {return scatoi((1 - p->z) / 2, 24);}

//~ ----------------------------------------------------------------------------

int g3_init(gx_Surf offs, int w, int h) {
	static struct gx_Clip rec;
	int memsize = 0;
	void *cBuff, *zBuff;
	rec.xmin = memsize;
	rec.ymin = memsize;
	rec.xmax = w - memsize;
	rec.ymax = h - memsize;
	memsize = w * h * sizeof(long);

	cBuff = malloc(2 * memsize);
	if (cBuff == NULL) return -1;
	zBuff = (unsigned long *) cBuff + w * h;

	offs->clipPtr = &rec;
	offs->width = w;
	offs->height = h;
	offs->flags = 0;
	offs->depth = 32;
	offs->pixeLen = 4;
	offs->scanLen = w*4;
	offs->basePtr = cBuff;
	offs->tempPtr = zBuff;
	//~ surf = offs;
	return 0;
}

static inline int testZ(gx_Surf dst, unsigned long z, int offs) {
	unsigned long *zBuff = dst->tempPtr;
	if (zBuff[offs] > z) {
		zBuff[offs] = z;
		return 1;
	}
	return 0;
}

void g3_setpixel(gx_Surf dst, int x, int y, unsigned z, long c) {
	long *cBuff = dst->basePtr;
	int offs = y * dst->width + x;
	const gx_Clip roi = gx_getclip(dst);
	if (y < roi->ymin || y >= roi->ymax) return;
	if (x < roi->xmin || x >= roi->xmax) return;
	if (testZ(dst, z, offs)) {
		cBuff[offs] = c;
	}
}

void mixpixel(gx_Surf dst, int x, int y, unsigned z, argb c, int a) {
	argb *cBuff = dst->basePtr;
	int offs = y * dst->width + x;
	//~ argb *col = (argb*)&cBuff[offs];
	//~ const gx_Clip roi = gx_getclip(dst);
	//~ if (y < roi->ymin || y >= roi->ymax) return;
	//~ if (x < roi->xmin || x >= roi->xmax) return;
	a &= 0xff;
	if (testZ(dst, z, offs)) {
		cBuff[offs].r += ((c.r - cBuff[offs].r) * a) >> 8;
		cBuff[offs].g += ((c.g - cBuff[offs].g) * a) >> 8;
		cBuff[offs].b += ((c.b - cBuff[offs].b) * a) >> 8;
	}
	else {
		//~ dst->val = -1;
		//~ a ^= 0xff;
		//~ dst->r += ((dst->r - c.r) * a) >> 8;
		//~ dst->g += ((dst->g - c.g) * a) >> 8;
		//~ dst->b += ((dst->b - c.b) * a) >> 8;
	}
}// */

int g2_clipcalc(gx_Clip roi, int x, int y) {
	int c = (y >= roi->ymax) << 0;
	c |= (y <  roi->ymin) << 1;
	c |= (x >= roi->xmax) << 2;
	c |= (x <  roi->xmin) << 3;
	return c;
}

int g2_clipline(gx_Clip roi, int *x1, int *y1, int *x2, int *y2) {
	//~ gx_Clip *roi = &rec;//(gx_Clip *)gx_getclip(surf);
	int c1 = g2_clipcalc(roi, *x1, *y1);
	int c2 = g2_clipcalc(roi, *x2, *y2);
	int x, dx, y, dy, e;

	while (c1 | c2) {
		if (c1 & c2) return 0;
		e = c1 ? c1 : c2;
		dx = *x2 - *x1;
		dy = *y2 - *y1;
		switch(e & -e) {		// get lowest bit set
			case 1 : {
				y = roi->ymax - 1;
				if (dy) dy = ((y - *y1) << 16) / dy;
				x = *x1 + ((dx * dy) >> 16);
			} break;
			case 2 : {
				y = roi->ymin;
				if (dy) dy = ((y - *y1) << 16) / dy;
				x = *x1 + ((dx * dy) >> 16);
			} break;
			case 4 : {
				x = roi->xmax - 1;
				if (dx) dx = ((x - *x1) << 16) / dx;
				y = *y1 + ((dx * dy) >> 16);
			} break;
			case 8 : {
				x = roi->xmin;
				if (dx) dx = ((x - *x1) << 16) / dx;
				y = *y1 + ((dx * dy) >> 16);
			} break;
		}
		if (c1) {
			*x1 = x; *y1 = y;
			c1 = g2_clipcalc(roi, x, y);
		}
		else {
			*x2 = x; *y2 = y;
			c2 = g2_clipcalc(roi, x, y);
		}
	}
	return 1;
}

int g3_clipline(gx_Clip roi, int *x1, int *y1, int *z1, int *x2, int *y2, int *z2) {
	int c1 = g2_clipcalc(roi, *x1, *y1);
	int c2 = g2_clipcalc(roi, *x2, *y2);
	int x, dx, y, dy, z, e;

	while (c1 | c2) {
		if (c1 & c2) return 0;
		e = c1 ? c1 : c2;
		dx = *x2 - *x1;
		dy = *y2 - *y1;
		switch(e & -e) {		// get lowest bit set
			case 1 : {
				y = roi->ymax - 1;
				if (dy) dy = ((y - *y1) << 16) / dy;
				x = *x1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
			} break;
			case 2 : {
				y = roi->ymin;
				if (dy) dy = ((y - *y1) << 16) / dy;
				x = *x1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
			} break;
			case 4 : {
				x = roi->xmax - 1;
				if (dx) dx = ((x - *x1) << 16) / dx;
				y = *y1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
			} break;
			case 8 : {
				x = roi->xmin;
				if (dx) dx = ((x - *x1) << 16) / dx;
				y = *y1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
			} break;
		}
		if (c1) {
			*x1 = x; *y1 = y, *z1 = z;
			c1 = g2_clipcalc(roi, x, y);
		}
		else {
			*x2 = x; *y2 = y, *z2 = z;
			c2 = g2_clipcalc(roi, x, y);
		}
	}
	return 1;
}

//~ ----------------------------------------------------------------------------
void g3_putpixel(gx_Surf dst, vector p, int c) {
	long x = projx(dst, p, 0);
	long y = projy(dst, p);
	long z = projz(dst, p);
	g3_setpixel(dst, x, y, z, c);
}

void g3_drawlineA(gx_Surf dst, vector p1, vector p2, long c0) {		//TODO
	struct gx_Clip roi = *gx_getclip(dst);
	argb c;
	int x, y, z = 0, zs=0;
	int x1, y1, z1, dx;
	int x2, y2, z2, dy;

	if (p1->y < p2->y) {vector t = p1; p1 = p2; p2 = t;}	// : y1 < y2
	c.val = c0;

	x1 = projx(dst, p1, 0);
	y1 = projy(dst, p1);
	z1 = projz(dst, p1);

	x2 = projx(dst, p2, 0);
	y2 = projy(dst, p2);
	z2 = projz(dst, p2);

	dx = x2 - x1; dy = y2 - y1;
	roi.xmax -= 1;
	roi.ymax -= 1;

	if (!g3_clipline(&roi, &x1, &y1, &z1, &x2, &y2, &z2)) return;

	if (y1 == y2 && x1 == x2) {		// pixel
		//~ g3_setpixel(x1, y1, z, c.val);
	}
	else if (y1 == y2) {			// vline
		if (x1 > x2) {x = x1; x1 = x2; x2 = x;}
		zs = (z2 - (z = z1)) / dx;
		for (x = x1; x < x2; x++, z += zs)
			g3_setpixel(dst, x, y1, z, c.val);
	}
	else if (x1 == x2) {			// hline
		zs = (z2 - (z = z1)) / dy;
		for (y = y1; y < y2; y++, z += zs)
			g3_setpixel(dst, x1, y, z, c.val);
	}
	else {
		long xs = (((x2 - x1) << 16) / (y2 - y1));
		if ((xs >> 16) == (xs >> 31)) {							// fixme
			zs = (z2 - (z = z1)) / dy;
			x = (x1 << 16) - (x1 > x2);
			for (y = y1; y <= y2; y += 1, x += xs, z += zs) {
				int X = x >> 16;
				mixpixel(dst, X + 0, y, z, c, (~x >> 8) & 0xff);
				mixpixel(dst, X + 1, y, z, c, (+x >> 8) & 0xff);
			}
		}
		else if (x1 < x2) {
			long ys = (((y2 - y1) << 16) / (x2 - x1));
			zs = (z2 - (z = z1)) / dx;
			y = (y1 << 16);
			for (x = x1; x <= x2; x++, y += ys, z += zs) {
				int Y = y >> 16;
				mixpixel(dst, x, Y + 0, z, c, (~y >> 8) & 0xff);
				mixpixel(dst, x, Y + 1, z, c, (+y >> 8) & 0xff);
			}
		}
		else {
			long ys = (((y2 - y1) << 16) / (x1 - x2));
			zs = (z2 - (z = z1)) / dx;
			y = (y1 << 16);
			for (x = x1; x >= x2; x--, y += ys, z += zs) {
				int Y = y >> 16;
				mixpixel(dst, x, Y + 0, z, c, (~y >> 8) & 0xff);
				mixpixel(dst, x, Y + 1, z, c, (+y >> 8) & 0xff);
			}
		}
	}
}

void g3_drawlineB(gx_Surf dst, vector p1, vector p2, long c) {		// BresenHam
	long sx = 1, dx;
	long sy = 1, dy;
	long zs = 0, e;

	int x1 = projx(dst, p1, 0);
	int y1 = projy(dst, p1);
	int z1 = projz(dst, p1);

	int x2 = projx(dst, p2, 0);
	int y2 = projy(dst, p2);
	int z2 = projz(dst, p2);

	if (!g3_clipline(gx_getclip(dst), &x1, &y1, &z1, &x2, &y2, &z2)) return;

	if ((dy = y2 - y1) < 0) {
		dy = -dy;
		sy = -1;
	}
	if ((dx = x2 - x1) < 0) {
		dx = -dx;
		sx = -1;
	}
	if (dx > dy) {
		e = dx >> 1;
		zs = (z2 - z1) / dx;
		while (x1 != x2) {
			g3_setpixel(dst, x1, y1, z1, c);
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
			g3_setpixel(dst, x1, y1, z1, c);
			if ((e += dx) > dy) {
				x1 += sx;
				e -= dy;
			}
			y1 += sy;
			z1 += zs;
		}
	}
}

void (*g3_drawline)(gx_Surf dst, vector p1, vector p2, long c) = g3_drawlineB;

typedef struct ssds {
	long x;
	long z;
	long s;
	long t;
	argb c;
} *ssds;// */
typedef struct edge {
	long x, dx, z, dz;		//
	long s, ds, t, dt;		// texture
	long r, dr, g, dg, b, db;	// RGB color
	//~ long sz, dsz, tz, dtz, rz, drz;
	//~ float A, dA, S0, S1, T0, T1, Z0, Z1;
	//~ union vector pos, dp;
	//~ union vector nrm, dn;
	//~ union vector col, dc;
	//~ argb rgb;
} *edge;

/*float lrpst(float u0, float u1, float z0, float z1, float a) {
	//~ return u0 + a * (u1 - u0);
	return (u0/z0 + a * (u1/z1 - u0/z0)) / (1./z0 + a * (1./z1 - 1./z0));
}

float lerpz(float u1, float u0, float a) {
	return u0 + a * (u1 - u0);
}

inline void edge_initXX(edge e, long x1, long x2, long z1, long z2, argb c1, argb c2, long s1, long s2, long t1, long t2, int len, int skip) {
	if (len <= 0) return;

	e->dx = (x2 - (e->x = x1)) / len;
	e->dz = (z2 - (e->z = z1)) / len;

	e->dr = ((c2.r - c1.r) << 16) / len;
	e->dg = ((c2.g - c1.g) << 16) / len;
	e->db = ((c2.b - c1.b) << 16) / len;
	e->r = c1.r << 16;
	e->g = c1.g << 16;
	e->b = c1.b << 16;

	e->ds = (s2 - (e->s = s1)) / len;
	e->dt = (t2 - (e->t = t1)) / len;

	//~ e->dA = (1 + (e->A = 0.)) / len;
	//~ e->Z0 = z1; e->Z1 = z2;
	//~ e->S0 = s1 / 65535.; e->S1 = s2 / 65535.;
	//~ e->T0 = t1 / 65535.; e->T1 = t2 / 65535.;

	if (skip > 0)
		edge_skip(e, skip);

}

*/

static inline void edge_next(edge e) {
	e->x += e->dx;
	e->z += e->dz;

	e->s += e->ds, e->t += e->dt;
	e->r += e->dr, e->g += e->dg, e->b += e->db;

	/*
	e->A += e->dA;
	if (draw & text_afine) {
		e->s = scatoi(lrpst(e->S0, e->S1, e->Z0, e->Z1, e->A), 16);
		e->t = scatoi(lrpst(e->T0, e->T1, e->Z0, e->Z1, e->A), 16);

		//~ or better this way ? float O = (1 - e->A) * (1. / e->Z0) + e->A * (1. / e->Z1);
		//~ e->s = scatoi(((1 - e->A) * (e->S0 / e->Z0) + e->A * (e->S1 / e->Z1)) / O, 16);
		//~ e->t = scatoi(((1 - e->A) * (e->T0 / e->Z0) + e->A * (e->T1 / e->Z1)) / O, 16);
	}

	vecadd(&e->pos, &e->pos, &e->dp);
	vecadd(&e->nrm, &e->nrm, &e->dn);
	*/

}
static inline void edge_skip(edge e, int skip) {
	if (skip) {
		e->x += skip * e->dx;
		e->z += skip * e->dz;

		e->r += skip * e->dr;
		e->g += skip * e->dg;
		e->b += skip * e->db;

		e->s += skip * e->ds;
		e->t += skip * e->dt;

		/*
		e->A += skip * e->dA;

		e->pos.x += skip * e->dp.x;
		e->pos.y += skip * e->dp.y;
		e->pos.z += skip * e->dp.z;
		e->nrm.x += skip * e->dn.x;
		e->nrm.y += skip * e->dn.y;
		e->nrm.z += skip * e->dn.z;
		*/
	}
}
static inline void edge_init_edge(edge e, edge l, edge r, int len, int skip) {
	e->dx = (r->x - (e->x = l->x)) / len;
	e->dz = (r->z - (e->z = l->z)) / len;

	e->ds = (r->s - (e->s = l->s)) / len;
	e->dt = (r->t - (e->t = l->t)) / len;

	e->dr = (r->r - (e->r = l->r)) / len;
	e->dg = (r->g - (e->g = l->g)) / len;
	e->db = (r->b - (e->b = l->b)) / len;

	/*
	e->dA = (1 + (e->A = 0.)) / len;

	e->Z0 = lerpz(l->Z0, l->Z1, l->A);
	e->S0 = lrpst(l->S0, l->S1, l->Z0, l->Z1, l->A);
	e->T0 = lrpst(l->T0, l->T1, l->Z0, l->Z1, l->A);

	e->Z1 = lerpz(r->Z0, r->Z1, r->A);
	e->S1 = lrpst(r->S0, r->S1, r->Z0, r->Z1, r->A);
	e->T1 = lrpst(r->T0, r->T1, r->Z0, r->Z1, r->A);

	e->dp.x = (r->pos.x - (e->pos.x = l->pos.x)) / len;
	e->dp.y = (r->pos.y - (e->pos.y = l->pos.y)) / len;
	e->dp.z = (r->pos.z - (e->pos.z = l->pos.z)) / len;

	e->dn.x = (r->nrm.x - (e->nrm.x = l->nrm.x)) / len;
	e->dn.y = (r->nrm.y - (e->nrm.y = l->nrm.y)) / len;
	e->dn.z = (r->nrm.z - (e->nrm.z = l->nrm.z)) / len;
	*/

	if (skip > 0)
		edge_skip(e, skip);

}
static inline void edge_init_ssds(edge e, ssds s1, ssds s2, int len, int skip) {
	if (len <= 0) return;

	e->dx = (s2->x - (e->x = s1->x)) / len;
	e->dz = (s2->z - (e->z = s1->z)) / len;

	e->dr = ((s2->c.r - s1->c.r) << 16) / len;
	e->dg = ((s2->c.g - s1->c.g) << 16) / len;
	e->db = ((s2->c.b - s1->c.b) << 16) / len;

	e->r = s1->c.r << 16;
	e->g = s1->c.g << 16;
	e->b = s1->c.b << 16;

	e->ds = (s2->s - (e->s = s1->s)) / len;
	e->dt = (s2->t - (e->t = s1->t)) / len;

	/*
	e->dA = (1 + (e->A = 0.)) / len;
	e->Z0 = z1; e->Z1 = z2;
	e->S0 = s1 / 65535.; e->S1 = s2 / 65535.;
	e->T0 = t1 / 65535.; e->T1 = t2 / 65535.;
	*/

	if (skip > 0)
		edge_skip(e, skip);
}// */

static void draw_tri_part(gx_Surf dst, edge l, edge r, int swap, int y1, int y2, gx_Surf img, argb tex2D(gx_Surf, long, long)) {
	struct edge v;
	register int x, y;
	gx_Clip roi = gx_getclip(dst);
	if (swap) { edge x = l; l = r; r = x;}
	for (y = y1; y < y2; y++) {
		int offs;
		int lx = l->x >> 16;
		int rx = r->x >> 16;
		int xlr = rx - lx, sx = 0;
		if (xlr > 0) {
			if (rx > roi->xmax) rx = roi->xmax;
			if (lx < (x = roi->xmin)) {
				sx = x - lx;
				lx = x;
			}

			offs = dst->width * y + lx;
			edge_init_edge(&v, l, r, xlr, sx);

			while (lx < rx) {
				if (testZ(dst, v.z, offs)) {
					register argb *cBuff = dst->basePtr;
					if (tex2D) {
						argb tex = tex2D(img, v.s, v.t);
						cBuff[offs].b = (v.b * tex.b) >> 24;
						cBuff[offs].g = (v.g * tex.g) >> 24;
						cBuff[offs].r = (v.r * tex.r) >> 24;
					}
					else {
						cBuff[offs].b = v.b >> 16;
						cBuff[offs].g = v.g >> 16;
						cBuff[offs].r = v.r >> 16;
					}
				}
				edge_next(&v);
				lx++; offs++;
			}
		}
		edge_next(l);
		edge_next(r);
	}// */
}
static void draw_triangle(gx_Surf dst, vector p, texcol tc, texcol lc, int i1, int i2, int i3, gx_Surf img) {
	argb (*tex2D)(gx_Surf, long, long) = 0;
	gx_Clip roi = gx_getclip(dst);

	long y1, y2, y3;
	long dy1 = 0, dy2 = 0;
	long ly1 = 0, ly2 = 0;
	register int y = roi->ymin;

	struct edge v1, v2;
	struct ssds s1, s2, s3;

	// sort by y
	if (p[i1].y < p[i3].y) {i1 ^= i3; i3 ^= i1; i1 ^= i3;}
	if (p[i1].y < p[i2].y) {i1 ^= i2; i2 ^= i1; i1 ^= i2;}
	if (p[i2].y < p[i3].y) {i2 ^= i3; i3 ^= i2; i2 ^= i3;}

	y1 = projy(dst, p + i1);
	y2 = projy(dst, p + i2);
	y3 = projy(dst, p + i3);

	if (y1 < y3) {				// clip (y)

		s1.x = projx(dst, p + i1, 16);
		s2.x = projx(dst, p + i2, 16);
		s3.x = projx(dst, p + i3, 16);

		s1.z = projz(dst, p + i1);
		s2.z = projz(dst, p + i2);
		s3.z = projz(dst, p + i3);

		s1.c.col = s2.c.col = s3.c.col = 0x00ffffff;

		if (img && tc) {
			int w = img->width-1;
			int h = img->height-1;
			s1.s = tc[i1].tex.s * w;
			s1.t = tc[i1].tex.t * h;
			s2.s = tc[i2].tex.s * w;
			s2.t = tc[i2].tex.t * h;
			s3.s = tc[i3].tex.s * w;
			s3.t = tc[i3].tex.t * h;

			//~ tex2D = (argb (*)(gx_Surf*, long, long)) gx_getpnear;
			tex2D = (argb (*)(gx_Surf, long, long)) gx_getpblin;

			//TODO: mip-map selection
			//~ if (y1 == y2) w = X2 - X1;
			//~ else if (y2 == y3) w = X3 - X2;
			//~ else w = X1 < X3 ? X1 - X2 : X3 - X1;
			//~ printf ("tri H:(%+014d), W:(%+014d)\r", y3 - y1, ((w < 0 ? -w : w) >> 16));
			//~ if ((w >>= 16) < 0)w = -w;
			//~ if (tmp > SCRH || ((w < 0 ? -w : w) >> 16) > SCRW) return;
		}
		else if (tc) {
			s1.c = tc[i1].rgb;
			s2.c = tc[i2].rgb;
			s3.c = tc[i3].rgb;
		}
		//~ else {
			//~ s1.c.col = 0x0ffffff;
			//~ s2.c.col = 0x0ffffff;
			//~ s3.c.col = 0x0ffffff;
		//~ }

		if (lc) {
			#define litmix(__r, __a, __b) {\
				(__r).r = ((__a).r + (__b).r) / 2;\
				(__r).g = ((__a).g + (__b).g) / 2;\
				(__r).b = ((__a).b + (__b).b) / 2;}
			litmix(s1.c, s1.c, lc[i1]);
			litmix(s2.c, s2.c, lc[i2]);
			litmix(s3.c, s3.c, lc[i3]);
			#undef litmix
		}

		// calc slope y3 - y1 (the longest)
		//~ edge_init(&v1, x1, x3, z1, z3, c1, c3, s1, s3, t1, t3, y3 - y1, y - y1);
		edge_init_ssds(&v1, &s1, &s3, y3 - y1, y - y1);
		ly1 = y2 - y1; dy1 = y - y1;
		ly2 = y3 - y2; dy2 = y - y2;
		// clip y1, y2, y3
		if (y1 < y) {
			if (y2 < y) {
				if (y3 < y) {
					return;
				}
				y2 = y;
			}
			y1 = y;
		}
		if (y3 > (y = roi->ymax)) {
			if (y2 > y) {
				if (y1 > y) {
					return;
				}
				y2 = y;
			}
			y3 = y;
		}

		//~ if (s1.z < 0) return;
		//~ if (s2.z < 0) return;
		//~ if (s3.z < 0) return;
	} else return;
	if (y1 < y2) {				// y1 < y < y2
		edge_init_ssds(&v2, &s1, &s2, ly1, dy1);
		draw_tri_part(dst, &v1, &v2, v2.dx < v1.dx, y1, y2, img, tex2D);
	}
	if (y2 < y3) {				// y2 < y < y3
		edge_init_ssds(&v2, &s2, &s3, ly2, dy2);
		draw_tri_part(dst, &v1, &v2, v2.x < v1.x, y2, y3, img, tex2D);
	}
}

int g3_drawenvc(gx_Surf dst, struct gx_Surf img[6], vector view, matrix proj, double size) {
	//~ #define CLPNRM 1
	//~ const scalar e = 0;
	union vector v[8];//, nrm[1];
	union texcol t[8];

	v[4].x = v[7].x = v[3].x = v[0].x = -size;
	v[1].x = v[2].x = v[5].x = v[6].x = +size;
	v[1].y = v[5].y = v[4].y = v[0].y = -size;
	v[3].y = v[2].y = v[7].y = v[6].y = +size;
	v[1].z = v[2].z = v[3].z = v[0].z = -size;
	v[4].z = v[5].z = v[7].z = v[6].z = +size;

	mappos(v + 0, proj, v + 0);
	mappos(v + 1, proj, v + 1);
	mappos(v + 2, proj, v + 2);
	mappos(v + 3, proj, v + 3);
	mappos(v + 4, proj, v + 4);
	mappos(v + 5, proj, v + 5);
	mappos(v + 6, proj, v + 6);
	mappos(v + 7, proj, v + 7);

	//~ if (vecdp3(view, vecldf(nrm, -1, 0, 0, 0)) < e || CLPNRM)
	{
		t[1].s = 0x0000; t[1].t = 0xffff;
		t[2].s = 0x0000; t[2].t = 0x0000;
		t[6].s = 0xffff; t[6].t = 0x0000;
		t[5].s = 0xffff; t[5].t = 0xffff;
		draw_triangle(dst, v, t, 0, 1, 2, 6, img + 2);
		draw_triangle(dst, v, t, 0, 1, 6, 5, img + 2);
	}
	//~ if (vecdp3(view, vecldf(nrm, +1, 0, 0, 0)) < e || CLPNRM)
	{
		t[0].s = 0xffff; t[0].t = 0xffff;
		t[3].s = 0xffff; t[3].t = 0x0000;
		t[7].s = 0x0000; t[7].t = 0x0000;
		t[4].s = 0x0000; t[4].t = 0xffff;
		draw_triangle(dst, v, t, 0, 0,3,7, img + 3);
		draw_triangle(dst, v, t, 0, 0,7,4, img + 3);
	}
	//~ if (vecdp3(view, vecldf(nrm, 0, +1, 0, 0)) < e || CLPNRM)
	{
		t[0].s = 0x0000; t[0].t = 0x0000;
		t[1].s = 0xffff; t[1].t = 0x0000;
		t[5].s = 0xffff; t[5].t = 0xffff;
		t[4].s = 0x0000; t[4].t = 0xffff;
		draw_triangle(dst, v, t, 0, 0,1,5, img + 5);
		draw_triangle(dst, v, t, 0, 0,5,4, img + 5);
	}
	//~ if (vecdp3(view, vecldf(nrm, 0, -1, 0, 0)) < e || CLPNRM)
	{
		t[2].s = 0xffff; t[2].t = 0xffff;
		t[3].s = 0x0000; t[3].t = 0xffff;
		t[7].s = 0x0000; t[7].t = 0x0000;
		t[6].s = 0xffff; t[6].t = 0x0000;
		draw_triangle(dst, v, t, 0, 2,3,7, img + 4);
		draw_triangle(dst, v, t, 0, 2,7,6, img + 4);
	}
	//~ if (vecdp3(view, vecldf(nrm, 0, 0, +1, 0)) < e || CLPNRM)
	{
		t[0].s = 0x0000; t[0].t = 0xffff;
		t[1].s = 0xffff; t[1].t = 0xffff;
		t[2].s = 0xffff; t[2].t = 0x0000;
		t[3].s = 0x0000; t[3].t = 0x0000;
		draw_triangle(dst, v, t, 0, 0,1,2, img + 0);
		draw_triangle(dst, v, t, 0, 0,2,3, img + 0);
	}
	//~ if (vecdp3(view, vecldf(nrm, 0, 0, -1, 0)) < e || CLPNRM)
	{
		t[4].s = 0xffff; t[4].t = 0xffff;
		t[5].s = 0x0000; t[5].t = 0xffff;
		t[6].s = 0x0000; t[6].t = 0x0000;
		t[7].s = 0xffff; t[7].t = 0x0000;
		draw_triangle(dst, v, t, 0, 4,5,6, img + 1);
		draw_triangle(dst, v, t, 0, 4,6,7, img + 1);
	}

	return 0;
}// */

void g3_drawOXYZ(gx_Surf dst, camera cam, double n) {
	union matrix tmp[3];
	matrix proj, view;
	union vector v[4];

	view = cammat(tmp, cam);
	proj = matmul(tmp + 2, &cam->proj, view);
	vecldf(v + 0, 0, 0, 0, 0);
	vecldf(v + 1, n, 0, 0, 0);
	vecldf(v + 2, 0, n, 0, 0);
	vecldf(v + 3, 0, 0, n, 0);

	mappos(v + 0, proj, v + 0);
	mappos(v + 1, proj, v + 1);
	mappos(v + 2, proj, v + 2);
	mappos(v + 3, proj, v + 3);

	g3_drawline(dst, v, v + 1, 0xff0000);
	g3_drawline(dst, v, v + 2, 0x00ff00);
	g3_drawline(dst, v, v + 3, 0x0000ff);

}// */

#include "g3_mesh.c"
int g3_drawmesh(gx_Surf dst, mesh msh, matrix objm, camera cam) {
	gx_Surf img = NULL;
	const long line_col = 0xffffff;
	const long norm_col = 0xff0000;
	const long bbox_col = 0xff00ff;

	unsigned i, l, tricnt = 0;
	union vector eye, v[8];
	material lcol[lights];
	union matrix tmp[3];
	matrix proj, view;

	#define MAXVTX (65536*16)
	static union vector pos[MAXVTX];
	static union texcol col[MAXVTX];
	static union texcol littmparr[MAXVTX];
	texcol lit = 0;
	if (msh->vtxcnt > MAXVTX) return -1;

	//~ World*Wiew*Proj
	cammat(tmp, cam);
	view = matmul(tmp + 1, cammat(tmp, cam), objm);
	proj = matmul(tmp + 2, &cam->proj, view);

	//~ frustum_get(v, &proj);
	//~ matinv(&invv, mmat);
	//~ matvph(&eye, &invv, &cam->pos);

	for (l = 0; l < lights; l += 1) {
		vecmul(&lcol[l].ambi, &msh->mtl.ambi, &Lights[l].ambi);
		vecmul(&lcol[l].diff, &msh->mtl.diff, &Lights[l].diff);
		vecmul(&lcol[l].spec, &msh->mtl.spec, &Lights[l].spec);
		lcol[l].spow = msh->mtl.spow;
	}
	eye = cam->pos;

	if (draw & draw_lit)
		lit = littmparr;

	if (draw & draw_tex)// && msh->hasTex)
		img = &msh->mtl.tex;

	for (i = 0; i < msh->vtxcnt; i += 1) {		// calc

		mappos(pos + i, proj, msh->pos + i);

		if (lit) {
			union vector tmp, color = msh->mtl.emis;
			//~ const vector N = &msh->vtxptr[i].nrm;	// normalVec
			//~ const vector V = &msh->vtxptr[i].pos;	// vertexPos
			const vector N = matvp3(v+0, objm, msh->nrm + i);	// normalVec
			const vector V = matvph(v+1, objm, msh->pos + i);	// vertexPos
			for (l = 0; l < lights; l += 1) if (Lights[l].attr & L_on) {
				vector D = vecsub(v+2, V, &Lights[l].pos);
				vector R, L = &Lights[l].dir;
				scalar dotp, attn = 1, spot = 1;

				if (vecdp3(L, L)) {					// directional or spot light
					if (Lights[l].sCos) {			// Spot Light
						spot = vecdp3(L, vecnrm(&tmp, D));
						if (spot > cos(toRad(Lights[l].sCos))) {
							spot = pow(spot, Lights[l].sExp);
						}
						else spot = 0;
					}// */
				}
				else {
					L = vecnrm(v+3, D);
				}//*/

				attn = spot / vecpev(&Lights[l].attn, veclen(D));

				R = vecnrm(v+5, vecrfl(v+5, vecsub(v+4, V, &eye), N));

				// Ambient
				vecsca(&tmp, &lcol[l].ambi, attn);
				vecadd(&color, &color, &tmp);
				if ((dotp = -vecdp3(N, L)) > 0) {
					// Diffuse
					vecsca(&tmp, &lcol[l].diff, attn * dotp);
					vecadd(&color, &color, &tmp);
				}
				if ((dotp = -vecdp3(R, L)) > 0) {
					// Specular
					vecsca(&tmp, &lcol[l].spec, attn * pow(dotp, msh->mtl.spow));
					vecadd(&color, &color, &tmp);
				}
			}
			lit[i].rgb = vecrgb(&color);
		}

		if (img)
			col[i].val = msh->tex[i].val;
		else if (lit)
			col[i].rgb = torgb(0);
		else
			col[i].rgb = nrmrgb(msh->nrm + i);
	}

	/*if (img && !msh->hasTex) {
		for (i = 0; i < msh->vtxcnt; i += 1) {
			int s = col[i].s * (msh->mtl.tex.width - 1);
			int t = col[i].t * (msh->mtl.tex.height - 1);
			col[i].val = gx_getpblin(&msh->mtl.tex, s, t);
		}
		img = 0;
	}// */

	for (i = 0; i < msh->tricnt; i += 1) {		// draw
		long i1 = msh->triptr[i].i1;
		long i2 = msh->triptr[i].i2;
		long i3 = msh->triptr[i].i3;

		// drop degenerated triangles
		if (pos[i1].w <= 0 || pos[i2].w <= 0 || pos[i3].w <= 0) continue;

		switch (draw & cull_mode) {				// culling faces(FIXME)
			default: return 0;
			case cull_none: break;
			case cull_front:
			case cull_back: {
				vector f = pos;
				double fA = (double)(f[i2].x - f[i1].x) * (f[i3].y - f[i1].y);
				double fB = (double)(f[i2].y - f[i1].y) * (f[i3].x - f[i1].x);
				switch (draw & cull_mode) {
					case cull_back: if (fA <= fB) continue; break;
					case cull_front: if (fA >= fB) continue; break;
				}
			}
		}
		switch (draw & draw_mode) {
			default : return 0;
			case draw_plot: {
				g3_putpixel(dst, pos + i1, col[i1].val);
				g3_putpixel(dst, pos + i2, col[i2].val);
				g3_putpixel(dst, pos + i3, col[i3].val);
			} break;
			case draw_wire: {
				g3_drawline(dst, pos + i1, pos + i2, col[i1].val);
				g3_drawline(dst, pos + i2, pos + i3, col[i2].val);
				g3_drawline(dst, pos + i1, pos + i3, col[i3].val);
			} break;
			case draw_fill: {
				draw_triangle(dst, pos, col, lit, i1, i2, i3, img);
			} break;
		}
		tricnt += 1;
	}
	for (i = 0; i < msh->segcnt; i += 1) {		// draw segs
		long i1 = msh->segptr[i].p1;
		long i2 = msh->segptr[i].p2;
		g3_drawline(dst, pos + i1, pos + i2, line_col);
	}
	if (msh->hlplt > 0 && msh->hlplt <= msh->vtxcnt) {
		const long s2 = 3;
		vector p = pos + msh->hlplt - 1;
		long x = projx(dst, p, 0);//scatoi ((dst->width - 1) * (1 + p->x) / 2, 0);
		long y = projy(dst, p);//scatoi((dst->height - 1) * (1 - p->y) / 2, 0);
		//~ long z = scatoi((1 - p->z) / 2, 24);
		g2_drawoval(dst, x - s2, y - s2, x + s2, y + s2, bbox_col);
	}
	if (draw & disp_norm) {						// normals
		for (i = 0; i < msh->vtxcnt; i += 1) {
			vecadd(v, msh->pos + i, vecsca(v, msh->nrm + i, N));
			mappos(v, proj, v);
			g3_drawline(dst, v, pos + i, norm_col);
		}
	}
	if (draw & disp_bbox) {						// bbox
		bboxMesh(msh, v + 0, v + 6);
		v[4].x = v[7].x = v[3].x = v[0].x;
		v[1].x = v[2].x = v[5].x = v[6].x;
		v[1].y = v[5].y = v[4].y = v[0].y;
		v[3].y = v[2].y = v[7].y = v[6].y;
		v[1].z = v[2].z = v[3].z = v[0].z;
		v[4].z = v[5].z = v[7].z = v[6].z;

		mappos(v + 0, proj, v + 0);
		mappos(v + 1, proj, v + 1);
		mappos(v + 2, proj, v + 2);
		mappos(v + 3, proj, v + 3);
		mappos(v + 4, proj, v + 4);
		mappos(v + 5, proj, v + 5);
		mappos(v + 6, proj, v + 6);
		mappos(v + 7, proj, v + 7);

		// front
		g3_drawline(dst, v + 0, v + 1, bbox_col);
		g3_drawline(dst, v + 1, v + 2, bbox_col);
		g3_drawline(dst, v + 2, v + 3, bbox_col);
		g3_drawline(dst, v + 3, v + 0, bbox_col);

		// back
		g3_drawline(dst, v + 4, v + 5, bbox_col);
		g3_drawline(dst, v + 5, v + 6, bbox_col);
		g3_drawline(dst, v + 6, v + 7, bbox_col);
		g3_drawline(dst, v + 7, v + 4, bbox_col);

		// join
		g3_drawline(dst, v + 0, v + 4, bbox_col);
		g3_drawline(dst, v + 1, v + 5, bbox_col);
		g3_drawline(dst, v + 2, v + 6, bbox_col);
		g3_drawline(dst, v + 3, v + 7, bbox_col);
	}
	return tricnt;
}

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

typedef int (*flip_scr)(gx_Surf, int);
typedef int (*peek_msg)(int);

#if defined __WIN_DRV
#include "drv_win.c"

#elif defined __VBE_DRV
#include "drv_dos.c"

#elif defined __X11_DRV
#include "drv_x11.c"

#else

int init_WIN(gx_Surf *offs, flip_scr *flip, peek_msg *msgp, void (**done)(void), void ratHND(int btn, int mx, int my), int kbdHND(int lkey, int key)){
	return -1;
}

#endif
