#include "mcga.h"
#include <math.h>
#include <time.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//~ #include "libs/fixed/fixed.h"

#ifdef FXPOINT

typedef long scalar;

#define sconst(__a) (long)(((__a) * (float)(1 << FXPOINT)))
#define scaldf(__a) (long)(((__a) * (float)(1 << FXPOINT)))
#define scatof(__a) ((__a) / (float)(1 << FXPOINT))
#define scatoi(__a) ((long)(__a) >> FXPOINT)

#define scamul(__a, __b) fxpmul(__a, __b)
#define scadiv(__a, __b) fxpdiv(__a, __b)
#define scainv(__a) fxpinv(__a)

#define scasin(__a) fxpsin(__a)
#define scacos(__a) fxpcos(__a)
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
#define scasqrt(__a) sqrt(__a)
#define scarsq(__a) (1. / sqrt(__a))
#define scapow(__a, __b) pow(__a, __b)
#define stof16(__a) ((long)((__a) * (1<<16)))
#define stof24(__a) ((long)((__a) * (1<<24)))

#endif

#include "vecmat.c"

#define SCRW 320
#define SCRH 200

unsigned char cBuff[SCRW*SCRH];
unsigned long zBuff[SCRW*SCRH];
rect rec = {0,0,SCRW,SCRH};

matrix projM;		// projection
matrix viewM;		// camera * world
//~ matrix worldM;		// world
//~ matrix provM;		// proj * view
enum {
	wire = 0x0001,		// 
	fill = 0x0002,		//
	flat = 0x0004,		// goroaud
	back = 0x0008,		// back face
	dspL = 0x0010,		// lights
	dspN = 0x0020,		// normals
}; int draw = fill+wire+back;

struct vtx {
	vector pos;			// (x, y, z, 1) positions
	vector nrm;			// (x, y, z, 0) normals
	short s, t;
	//~ union {
		//~ long col;
		//~ struct {
			//~ short s, t;
		//~ } tex;
	//~ };
	//~ vector tex;			// (u, v, 0, 0) textute
	//~ vector col;			// (r, g, b, 1) color
};

struct tri {
	unsigned i1;
	unsigned i2;
	unsigned i3;
};

struct mip {		// texture
	int tmp;
};

static struct {			// mesh
	unsigned maxvtx, vtxcnt;
	unsigned maxtri, tricnt;
	//~ struct mat back, fore;
	//~ struct mip back, fore;
	struct vtx *vtxptr;
	struct tri *triptr;
} mesh;

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

int frustum_csp(vector planes[6], vecptr p) {			// clip sphere
	scalar r = -p->w;
	if (vecdph(&planes[0], p) <= r) return 0;
	if (vecdph(&planes[1], p) <= r) return 0;
	if (vecdph(&planes[2], p) <= r) return 0;
	if (vecdph(&planes[3], p) <= r) return 0;
	if (vecdph(&planes[4], p) <= r) return 0;
	if (vecdph(&planes[5], p) <= r) return 0;
	return 1;	// inside
}

int frustum_cpt(vector planes[6], vecptr p) {			// clip sphere
	if (vecdph(&planes[0], p) <= 0) return 0;
	if (vecdph(&planes[1], p) <= 0) return 0;
	if (vecdph(&planes[2], p) <= 0) return 0;
	if (vecdph(&planes[3], p) <= 0) return 0;
	if (vecdph(&planes[4], p) <= 0) return 0;
	if (vecdph(&planes[5], p) <= 0) return 0;
	return 1;	// inside
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

scalar backface(vecptr N, vecptr p1, vecptr p2, vecptr p3 /*, vector *nP*/) {
	vector e1, e2, tmp;
	if (!N) N = &tmp;
	vecsub(&e1, p3, p1);
	vecsub(&e2, p3, p2);
	vecnrm(N, veccp3(N, &e1, &e2));
	N->w = 0;
	return vecdp3(p1, N);
}

vecptr mapvec(vecptr dst, matptr mat, vecptr src) {
	scalar wdx;
	vector tmp;
	if (src == dst)
		src = veccpy(&tmp, src);
	dst->x = vecdp4(&mat->x, src);
	dst->y = vecdp4(&mat->y, src);
	dst->z = vecdp4(&mat->z, src);
	if ((wdx = vecdp4(&mat->w, src))) {
		vecsca(dst, dst, scainv(wdx));
		dst->w = sconst(1);
	} else dst->w = sconst(0);
	return dst;
}

/*scalar mapvecd(vecptr dst, matptr mat, vecptr src) {
	vector tmp = *src;
	scalar wdx;
	if ((wdx = vecdph(&mat->w, &tmp))) {
		dst->x = scadiv(vecdph(&mat->x, &tmp), wdx);
		dst->y = scadiv(vecdph(&mat->y, &tmp), wdx);
		dst->z = scadiv(vecdph(&mat->z, &tmp), wdx);
		//~ dst->w = sconst(1);
	}
	return wdx;
}

scalar mapvecm(vecptr dst, matptr mat, vecptr src) {
	vector tmp = *src;
	scalar wdx;
	if ((wdx = vecdph(&mat->w, &tmp))) {
		dst->x = vecdph(&mat->x, &tmp);
		dst->y = vecdph(&mat->y, &tmp);
		dst->z = vecdph(&mat->z, &tmp);
		vecsca(dst, dst, scainv(wdx));
		//~ dst->w = sconst(1);
	}// * /
	return wdx;
}

//~ scalar (*mapvec)(vecptr dst, matptr mat, vecptr src) = mapvecd;

scalar mappos(vecptr dst, matptr mat, vecptr src) {
	vector tmp = *src;
	scalar wdx;
	if ((wdx = vecdph(&mat->w, &tmp))) {
		dst->x = vecdph(&mat->x, &tmp);
		dst->y = vecdph(&mat->y, &tmp);
		dst->z = vecdph(&mat->z, &tmp);
		vecsca(dst, dst, scainv(wdx));
		//~ dst->w = sconst(1);
	}// * /
	return wdx;
}

scalar mapnrm(vecptr dst, matptr mat, vecptr src) {
	vector tmp = *src;
	scalar wdx;
	/ *if ((wdx = vecdp4(&mat->w, &tmp))) {
		dst->x = vecdp4(&mat->x, &tmp);
		dst->y = vecdp4(&mat->y, &tmp);
		dst->z = vecdp4(&mat->z, &tmp);
		vecsca(dst, dst, scainv(wdx));
		//~ dst->w = sconst(1);
	}// * /
	if ((wdx = mat->wt)) {
		dst->x = vecdp3(&mat->x, &tmp);
		dst->y = vecdp3(&mat->y, &tmp);
		dst->z = vecdp3(&mat->z, &tmp);
		vecsca(dst, dst, scainv(wdx));
		//~ dst->w = sconst(1);
	}// * /
	/ *if ((wdx = mat->wt)) {
		dst->x = scadiv(vecdp3(&mat->x, &tmp), wdx);
		dst->y = scadiv(vecdp3(&mat->y, &tmp), wdx);
		dst->z = scadiv(vecdp3(&mat->z, &tmp), wdx);
		//~ dst->w = sconst(0);
	}// * /
	return wdx;
}
*/

scalar mapnrm(vecptr dst, matptr mat, vecptr src) {
	vector tmp = *src;
	scalar wdx;
	if ((wdx = mat->wt)) {
		dst->x = vecdp3(&mat->x, &tmp);
		dst->y = vecdp3(&mat->y, &tmp);
		dst->z = vecdp3(&mat->z, &tmp);
		vecsca(dst, dst, scainv(wdx));
		//~ dst->w = sconst(1);
	}
	return wdx;
}// */

inline long vecrgb(vecptr src) {
	float r = scatof(src->r);
	float g = scatof(src->g);
	float b = scatof(src->b);
	if (r > 1) r = 1; else if (r < 0) r = 0;
	if (g > 1) g = 1; else if (g < 0) g = 0;
	if (b > 1) b = 1; else if (b < 0) b = 0;
	return (long)((0.299 * r + 0.587 * g + 0.114 * b) * 255);
}

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

int addvtxdv(double pos[3], double nrm[3]) {
	if (mesh.vtxcnt >= mesh.maxvtx) {
		mesh.vtxptr = (struct vtx*)realloc(mesh.vtxptr, sizeof(struct vtx) * (mesh.maxvtx <<= 1));
		if (!mesh.vtxptr) {
			k3d_exit();
			return -1;
		}
	}
	vecldf(&mesh.vtxptr[mesh.vtxcnt].pos, pos[0], pos[1], pos[2], 1);
	if (nrm) vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, nrm[0], nrm[1], nrm[2], 0);
	else vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, 0, 0, 0, 0);
	return mesh.vtxcnt++;
}

int addvtxfv(float pos[3], float nrm[3]) {
	if (mesh.vtxcnt >= mesh.maxvtx) {
		mesh.vtxptr = (struct vtx*)realloc(mesh.vtxptr, sizeof(struct vtx) * (mesh.maxvtx <<= 1));
		if (!mesh.vtxptr) {
			k3d_exit();
			return -1;
		}
	}
	vecldf(&mesh.vtxptr[mesh.vtxcnt].pos, pos[0], pos[1], pos[2], 1);
	if (nrm) vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, nrm[0], nrm[1], nrm[2], 0);
	else vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, 0, 0, 0, 0);
	return mesh.vtxcnt++;
}

int addvtxf(float px, float py, float pz, float nx, float ny, float nz) {
	if (mesh.vtxcnt >= mesh.maxvtx) {
		mesh.vtxptr = (struct vtx*)realloc(mesh.vtxptr, sizeof(struct vtx) * (mesh.maxvtx <<= 1));
		if (!mesh.vtxptr) {
			k3d_exit();
			return -1;
		}
	}
	vecldf(&mesh.vtxptr[mesh.vtxcnt].pos, px, py, pz, 1);
	vecldf(&mesh.vtxptr[mesh.vtxcnt].nrm, nx, ny, nz, 0);
	return mesh.vtxcnt++;
}

int addtri(unsigned p1, unsigned p2, unsigned p3) {
	if (p1 >= mesh.vtxcnt || p2 >= mesh.vtxcnt || p3 >= mesh.vtxcnt) return -1;
	if (mesh.tricnt >= mesh.maxtri) {
		mesh.triptr = (struct tri*)realloc(mesh.triptr, sizeof(struct tri) * (mesh.maxtri <<= 1));
		if (!mesh.triptr) {
			k3d_exit();
			return -1;
		}
	}
	mesh.triptr[mesh.tricnt].i1 = p1;
	mesh.triptr[mesh.tricnt].i2 = p2;
	mesh.triptr[mesh.tricnt].i3 = p3;
	return mesh.tricnt ++;
}

struct mat {			// material
	vector emis;		// Emissive
	vector ambi;		// Ambient
	vector diff;		// Diffuse
	vector spec;		// Specular
	scalar spow;
} material = {
	{{.0, .0, .0, 1}},		// emissive color
	{{.1, .8, .5}},		// ambient color
	{{.1, .8, .5}},		// diffuse color
	{{1., 1., 1.}},		// specular color
	256,				// specular power
};

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
	vector	ldir;		// light position
	vector	ambi;		// Ambient
	vector	diff;		// Diffuse
	vector	spec;		// Specular
	//~ vector	attn;		// attenuation = 1 / (K1 + K2 � dist + K3 � dist ** 2)
	//~ scalar	spot;		// == 1
} Lights[] = {
	{	// light0
		L_on,
		{{150, 150, 150, 1}},	// lpos
		{{0., 0., 0., 1}},	// ldir
		{{.2, .2, .2, 1}},	// ambi
		{{1., 1., 1., 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		}, // */
	{	// light1 - off
		L_on,
		{{150, 0, -150, 1}},	// lpos
		{{0., 0., 0., 1}},	// ldir
		{{.2, .2, .2, 1}},	// ambi
		{{1., 1., 1., 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		}, // */
};

long calcLight(vecptr pos, vecptr nrm, vecptr eye, matptr view) {
	vector Ptmp, Ntmp, kA, kD, kS, col, dir;
	unsigned i, n = sizeof(Lights) / sizeof(*Lights);
	if (eye) mapvec(&dir, view, eye);
	pos = mapvec(&Ptmp, view, pos);
	nrm = mapvec(&Ntmp, view, nrm);
	veccpy(&col, &material.emis);
	for (i = 0; i < n; i += 1) if (Lights[i].attr & L_on) {
		//{ VertexColor(r, g, b, a) =
		//~ M.emis + 
		//~ W.ambi * M.ambi +
		//~ sum(i : 1 to nlights) (
			//~ L[i].ambi * M.ambi +
			//~ L[i].diff * M.diff * max(dot(L, n), 0) +
			//~ L[i].spec * M.spec * pow(max(dot(H || R, n), 0)), M.spow)
		//~ ) * L[i].attn * L[i].spot
			//~ dist = pos_L - pos_V;
			//~ attn = 1 / (K1 + K2 � dist + K3 � dist ** 2);
			//~ L = unit vector from vertex to light position
			//~ n = unit normal (at a vertex)
			//~ s = (x  + (0, 0, 1) ) / | x + (0, 0, 1) | / halfvector ???
			//~ x = vector indicating direction from light source (or direction of directional light)
		//}
		scalar diff, spec;
		vector tmp, lp, L, H;
		mapvec(&lp, view, &Lights[i].lpos);
		vecmul(&kA, &material.ambi, &Lights[i].ambi);
		vecmul(&kD, &material.diff, &Lights[i].diff);
		vecmul(&kS, &material.spec, &Lights[i].spec);
		//~ vecldf(&kD, material.diff[0] * Lights[i].diff[0], material.diff[1] * Lights[i].diff[1], material.diff[2] * Lights[i].diff[2], 1);
		//~ vecldf(&kS, material.spec[0] * Lights[i].spec[0], material.spec[1] * Lights[i].spec[1], material.spec[2] * Lights[i].spec[2], 1);

		// Ambient
		vecadd(&col, &col, &kA);
		// Diffuse
		vecnrm(&L, vecsub(&tmp, &lp, pos));		// direction of light to vertex
		if ((diff = vecdp3(nrm, &L)) > 0) vecadd(&col, &col, vecsca(&tmp, &kD, diff));
		// Specular
		//~ vecnrm(&H, vecreflect(&H, eye, nrm));
		vecnrm(&H, vecsub(&tmp, &L, &dir));		// halfvector betveen eye & light
		if ((spec = vecdp3(nrm, &H)) > 0) vecadd(&col, &col, vecsca(&tmp, &kS, pow(spec, material.spow)));
		
		// Attenuate
		//~ TODO
		// Spot effect
		//~ TODO
	}
	return vecrgb(&col);
}// */

void precalcL(long c[], vecptr eye, matptr view) {
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

void g3_setpixel(void* s, int x, int y, unsigned z, int c) {
	if (x < 0 || x >= SCRW) return;
	if (y < 0 || y >= SCRH) return;
	if (zBuff[SCRW * y + x] > z) {
		zBuff[SCRW * y + x] = z;
		cBuff[SCRW * y + x] = c;
	}
}

void g3_putpixel(void* s, vecptr p1, int c) {
	long x = scatoi((SCRW-1) * (sconst(1) + p1->x) / 2);
	long y = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	long z = stof24((sconst(1) - p1->z) / 2);
	g3_setpixel(s, x, y, z, c);
}

void g3_drawline(void* s, vecptr p1, vecptr p2, long c) {
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
	}// */
	g3_setpixel(s,x2,y2,z2,c);
}

void g3_drawoval(void* s, vecptr p1, long rx, long ry, long c) {
	long x0, x1, sx, dx = 0;
	long y0, y1, sy, dy = 0;
	long x, y, z, r;

	x = scatoi((SCRW-1) * (sconst(1) + p1->x) / 2);
	y = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	z = stof24((sconst(1) - p1->z) / 2);
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

void g3_filloval(void* s, vecptr p1, long rx, long ry, long c) {
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
		for (x = x0; x < x1; x += 1) g3_setpixel(s,x,y0,z,c);
		for (x = x0; x < x1; x += 1) g3_setpixel(s,x,y1,z,c);
	}
}

void g3_fillpoly(void* s, vecptr p1, vecptr p2, vecptr p3, long c1, long c2, long c3) {
	rect *roi = &rec;
	register long tmp, y;
	long y1, y2, y3;
	long X1, X2, X3;
	long Z1, Z2, Z3;
	struct {
		long c;
		long x, z;
		//~ scalar r, g, b;
	} v1, v2, s1, s2, *l, *r;
	if (p1->y < p3->y) {
		void* tmp = p1; p1 = p3; p3 = (vecptr)tmp;
		c1 ^= c3; c3 ^= c1; c1 ^= c3;
	}
	if (p1->y < p2->y) {
		void* tmp = p1; p1 = p2; p2 = (vecptr)tmp;
		c1 ^= c2; c2 ^= c1; c1 ^= c2;
	}
	if (p2->y < p3->y) {
		void* tmp = p2; p2 = p3; p3 = (vecptr)tmp;
		c2 ^= c3; c3 ^= c2; c2 ^= c3;
	}

	X1 = stof16((SCRW-1) * (sconst(1) + p1->x) / 2);
	y1 = scatoi((SCRH-1) * (sconst(1) - p1->y) / 2);
	Z1 = stof24((sconst(1) - p1->z) / 2);

	X2 = stof16((SCRW-1) * (sconst(1) + p2->x) / 2);
	y2 = scatoi((SCRH-1) * (sconst(1) - p2->y) / 2);
	Z2 = stof24((sconst(1) - p2->z) / 2);

	X3 = stof16((SCRW-1) * (sconst(1) + p3->x) / 2);
	y3 = scatoi((SCRH-1) * (sconst(1) - p3->y) / 2);
	Z3 = stof24((sconst(1) - p3->z) / 2);

	if ((tmp = y3 - y1)) {				// draw nothing / clip (y)
		if (y3 < roi->ymin) return;
		if ((y = y1) > roi->ymax) return;

		v1.c = c1 << 16;
		s1.x = (X3 - (v1.x = X1)) / tmp;
		s1.z = (Z3 - (v1.z = Z1)) / tmp;
		s1.c = ((c3 - c1)  << 16) / tmp;
		//~ s1.r = ((c3.r-c1.r) <<16) / tmp;
		//~ s1.g = ((c3.g-c1.g) <<16) / tmp;
		//~ s1.b = ((c3.b-c1.b) <<16) / tmp;
		if ((tmp = roi->ymin - y) > 0) {
			v1.x += s1.x * tmp;
			v1.z += s1.z * tmp;
			v1.c += s1.c * tmp;
			y = roi->ymin;
		}
	} else return;
	if ((tmp = y2 - y1)) {				// y1 < y < y2
		int yy = y2;
		v2.c = c1 << 16;
		s2.x = (X2 - (v2.x = X1)) / tmp;
		s2.z = (Z2 - (v2.z = Z1)) / tmp;
		s2.c = ((c2 - c1)  << 16) / tmp;
		//~ s2.r = ((c2.r-c1.r) <<16) / tmp;
		//~ s2.g = ((c2.g-c1.g) <<16) / tmp;
		//~ s2.b = ((c2.b-c1.b) <<16) / tmp;
		if (yy > roi->ymax) yy = roi->ymax;
		if ((tmp = y - y1) > 0) {
			v2.x += s2.x * tmp;
			v2.z += s2.z * tmp;
			v2.c += s2.c * tmp;
		}
		if (s2.x > s1.x) {
			l = &v1;
			r = &v2;
		} else {
			l = &v2;
			r = &v1;
		}
		while (y < yy) {
			unsigned char *cb;
			unsigned long *zb, z = l->z;
			int s, g, c = l->c;
			int x1 = l->x >> 16;
			int x2 = r->x >> 16;
			if ((g = x2 - x1)) {
				s = (r->z - l->z) / g;
				g = (r->c - l->c) / g;
			} else s = 0;
			if (x2 > roi->xmax) {
				x2 = roi->xmax;
			}
			if (x1 < roi->xmin) {
				z += s * (roi->xmin - x1);
				c += g * (roi->xmin - x1);
				x1 = roi->xmin;
			}
			zb = &zBuff[SCRW*y + x1];
			cb = &cBuff[SCRW*y + x1];
			while (x1 < x2) {
				g3_setpixel(NULL, x1, y, z, c>>16);
				/*if (z < *zb) {
					*zb = z;
					cb[0] = c >> 16;
				}// */
				x1++; zb++;
				cb += 1;
				z += s;
				c += g;
			}
			v1.c += s1.c; v2.c += s2.c;
			v1.x += s1.x; v2.x += s2.x;
			v1.z += s1.z; v2.z += s2.z;
			y++;
		}
	}
	if ((tmp = y3 - y2)) {				// y2 < y < y3
		long yy = y3;
		v2.c = c2 << 16;
		s2.x = (X3 - (v2.x = X2)) / tmp;
		s2.z = (Z3 - (v2.z = Z2)) / tmp;
		s2.c = ((c3 - c2)  << 16) / tmp;
		//~ s2.r = ((c3.r-c2.r) <<16) / tmp;
		//~ s2.g = ((c3.g-c2.g) <<16) / tmp;
		//~ s2.b = ((c3.b-c2.b) <<16) / tmp;
		//~ v2.r = c2.r << 16;
		//~ v2.g = c2.g << 16;
		//~ v2.b = c2.b << 16;
		if (yy > roi->ymax) yy = roi->ymax;
		if ((tmp = y - y2) > 0) {
			v2.x += s2.x * tmp;
			v2.z += s2.z * tmp;
			v2.c += s2.c * tmp;
		}
		if (v2.x > v1.x) {
			l = &v1;
			r = &v2;
		} else {
			l = &v2;
			r = &v1;
		}
		while (y < yy) {
			unsigned char *cb;
			unsigned long *zb, z = l->z;
			int s, g, c = l->c;
			long x1 = l->x >> 16;
			long x2 = r->x >> 16;
			if ((g = x2 - x1)) {
				s = (r->z - l->z) / g;
				g = (r->c - l->c) / g;
			} else s = 0;
			if (x2 > roi->xmax) {
				x2 = roi->xmax;
			}
			if (x1 < roi->xmin) {
				z += s * (roi->xmin - x1);
				c += g * (roi->xmin - x1);
				x1 = roi->xmin;
			}
			zb = &zBuff[SCRW*y + x1];
			cb = &cBuff[SCRW*y + x1];
			while (x1 < x2) {
				g3_setpixel(NULL, x1, y, z, c>>16);
				/*if (z < *zb) {
					*zb = z;
					cb[0] = c >> 16;
				}// */
				x1++;
				zb++;
				cb += 1;
				z += s;
				c += g;
			}
			v1.c += s1.c; v2.c += s2.c;
			v1.x += s1.x; v2.x += s2.x;
			v1.z += s1.z; v2.z += s2.z;
			y++;
		}
	}// */
}

void mappos(vecptr dst, matptr mat, void* src, int n, unsigned stride) {
	while (n-- > 0) {
		//~ /*
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

void g3_drawmeshf(camera_p cam) {
	unsigned i;
	long col[65536];
	vector pos[65536], frustum[6], bs;
	matrix proj, view, tmp;
	if (mesh.vtxcnt > 65536) {
		return;
	}
	matmul(&view, cammat(&tmp, cam), &viewM);
	matmul(&proj, &projM, &view);
	frustum_get(frustum, &proj);
	for(i = 0; i < SCRW*SCRH; i+=1) cBuff[i] = 0;
	for(i = 0; i < SCRW*SCRH; i+=1) zBuff[i] = 0x00ffffff;
	precalcL(col, &cam->pos, &view);
	mappos(pos, &proj, mesh.vtxptr, mesh.vtxcnt, sizeof(struct vtx));
	for (i = 0; i < mesh.tricnt; i += 1) {
		long i1 = mesh.triptr[i].i1;
		long i2 = mesh.triptr[i].i2;
		long i3 = mesh.triptr[i].i3;
		if (!frustum_csp(frustum, bsphere(&bs, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i3].pos))) continue;
		if (draw & fill) {
			g3_fillpoly(cBuff, &pos[i1], &pos[i2], &pos[i3], col[i1], col[i2], col[i3]);
		} else if (draw & wire) {
			g3_drawline(cBuff, &pos[i1], &pos[i2], 255);
			g3_drawline(cBuff, &pos[i1], &pos[i3], 255);
			g3_drawline(cBuff, &pos[i2], &pos[i3], 255);
		}
	}
}

void g3_drawmesh(camera_p cam) {
	vector frustum[6], bs;
	matrix proj, view, tmp;
	unsigned i, out = 0;
	//~ cammat(&view, cam);
	//~ matcpy(&view, &viewM);
	//~ matmul(&mat, &projM, &viewM);
	matmul(&view, cammat(&tmp, cam), &viewM);
	matmul(&proj, &projM, &view);
	frustum_get(frustum, &proj);
	for(i = 0; i < SCRW*SCRH; i+=1) cBuff[i] = 0;
	for(i = 0; i < SCRW*SCRH; i+=1) zBuff[i] = 0x00ffffff;
	//~ precalcL(&cam->pos, &view);
	for (i = 0; i < mesh.tricnt; i += 1) {
		long i1 = mesh.triptr[i].i1;
		long i2 = mesh.triptr[i].i2;
		long i3 = mesh.triptr[i].i3;
		vector N, p1, p2, p3, n1, n2, n3;
		if (!frustum_cpt(frustum, bsphere(&bs, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i3].pos))) continue;
		//~ if (!frustum_cpt(frustum, &mesh.vtxptr[i1].pos)) continue;
		//~ if (!frustum_cpt(frustum, &mesh.vtxptr[i1].pos)) continue;
		//~ if (!frustum_cpt(frustum, &mesh.vtxptr[i1].pos)) continue;
		mapvec(&p1, &proj, &mesh.vtxptr[i1].pos);
		mapvec(&p2, &proj, &mesh.vtxptr[i2].pos);
		mapvec(&p3, &proj, &mesh.vtxptr[i3].pos);
		//~ if (!(draw & 4) || backface(0, &p1, &p2, &p3) < sconst(0)) continue;
		if (!(draw & back) || backface(0, &p1, &p2, &p3) > sconst(0)) {
			if (draw & fill) {
				long c1, c2, c3;
				// calc lights
				if (draw & flat) {
					backface(&N, &mesh.vtxptr[i1].pos, &mesh.vtxptr[i2].pos, &mesh.vtxptr[i3].pos);
					c1 = calcLight(&mesh.vtxptr[i1].pos, &N, &cam->pos, &view);
					c2 = calcLight(&mesh.vtxptr[i2].pos, &N, &cam->pos, &view);
					c3 = calcLight(&mesh.vtxptr[i3].pos, &N, &cam->pos, &view);
				}
				else {
					c1 = calcLight(&mesh.vtxptr[i1].pos, &mesh.vtxptr[i1].nrm, &cam->pos, &view);
					c2 = calcLight(&mesh.vtxptr[i2].pos, &mesh.vtxptr[i2].nrm, &cam->pos, &view);
					c3 = calcLight(&mesh.vtxptr[i3].pos, &mesh.vtxptr[i3].nrm, &cam->pos, &view);
				}
				g3_fillpoly(cBuff, &p1, &p2, &p3, c1, c2, c3);
			} else if (draw & wire) {				// draw backface in wire
				g3_drawline(cBuff, &p1, &p2, 255);
				g3_drawline(cBuff, &p1, &p3, 255);
				g3_drawline(cBuff, &p2, &p3, 255);
			}
		} else if (draw & wire) {
			g3_drawline(cBuff, &p1, &p2, 255);
			g3_drawline(cBuff, &p1, &p3, 255);
			g3_drawline(cBuff, &p2, &p3, 255);
		}
		if (draw & dspN) {				// draw normals
			scalar Sn = 8;			// scale the normal
			mapvec(&n1, &proj, vecadd(&n1, &mesh.vtxptr[i1].pos, vecsca(&n1, &mesh.vtxptr[i1].nrm, Sn)));
			mapvec(&n2, &proj, vecadd(&n2, &mesh.vtxptr[i2].pos, vecsca(&n2, &mesh.vtxptr[i2].nrm, Sn)));
			mapvec(&n3, &proj, vecadd(&n3, &mesh.vtxptr[i3].pos, vecsca(&n3, &mesh.vtxptr[i3].nrm, Sn)));
			g3_drawline(cBuff, &p1, &n1, 128);
			g3_drawline(cBuff, &p2, &n2, 128);
			g3_drawline(cBuff, &p3, &n3, 128);

			g3_putpixel(cBuff, &n1, 255);
			g3_putpixel(cBuff, &n2, 255);
			g3_putpixel(cBuff, &n3, 255);
		}
		out ++;
	} if (out == 0) {						// draw 2 points if totally clipped
		setpixel(cBuff, 0, 0, 0xff);
		setpixel(cBuff, 1, 0, 0xff);
		setpixel(cBuff, 2, 0, 0x00);
		setpixel(cBuff, 0, 1, 0x00);
		setpixel(cBuff, 1, 1, 0x00);
		setpixel(cBuff, 2, 1, 0x00);
	} else if (out < mesh.tricnt) {		// draw 1 point if partialy clipped
		setpixel(cBuff, 0, 0, 0xff);
		setpixel(cBuff, 1, 0, 0x00);
		setpixel(cBuff, 0, 1, 0x00);
		setpixel(cBuff, 1, 1, 0x00);
	}// */
	if (draw & dspL) for (i = 0; i < sizeof(Lights) / sizeof(*Lights); i += 1) {
		vector lp;
		long col = vecrgb(&Lights[i].diff);
		mapvec(&lp, &proj, &Lights[i].lpos);
		g3_drawoval(cBuff, &lp, (int)((1+lp.z)*500), (int)((1+lp.z)*500), col);
		if (Lights[i].attr & L_on) g3_filloval(cBuff, &lp, (int)((1+lp.z)*500), (int)((1+lp.z)*500), col);
	}// */
	bs.x = 0;
}

//{-8<------------  MAIN  ------------------------------------------------------
//{ param surf
//{ dv3opr

inline double* dv3set(double dst[3], double x, double y, double z) {
	dst[0] = x;
	dst[1] = y;
	dst[2] = z;
	return dst;
}

inline double* dv3cpy(double dst[3], double src[3]) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	return dst;
}

inline double* dv3add(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] + rhs[0];
	dst[1] = lhs[1] + rhs[1];
	dst[2] = lhs[2] + rhs[2];
	return dst;
}

inline double* dv3sub(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] - rhs[0];
	dst[1] = lhs[1] - rhs[1];
	dst[2] = lhs[2] - rhs[2];
	return dst;
}

inline double* dv3mul(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] * rhs[0];
	dst[1] = lhs[1] * rhs[1];
	dst[2] = lhs[2] * rhs[2];
	return dst;
}

inline double* dv3sca(double dst[3], double lhs[3], double rhs) {
	dst[0] = lhs[0] * rhs;
	dst[1] = lhs[1] * rhs;
	dst[2] = lhs[2] * rhs;
	return dst;
}

inline double  dv3dot(double lhs[3], double rhs[3]) {
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

inline double* dv3crs(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	return dst;
}

inline double* dv3mad(double dst[3]/* , double src[3] */, double mul[3], double add[3]) {
	double* src = dst;
	dst[0] = src[0] * mul[0] + add[0];
	dst[1] = src[1] * mul[1] + add[1];
	dst[2] = src[2] * mul[2] + add[2];
	return dst;
}

inline double* dv3nrm(double dst[3], double src[3]) {
	double len = sqrt(dv3dot(src, src));
	if (len > 0) {
		dst[0] = src[0] / len;
		dst[1] = src[1] / len;
		dst[2] = src[2] / len;
	}
	else {
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
	}
	return dst;
}

//}

static const double H = 1e-5;

double* diffS_gen(double dst[3], double s, double t, double* (*evalP)(double [3], double s, double t)) {
	double d[3], p[3];
	evalP(d, s + H, t);
	evalP(p, s, t);
	//~ dst[0] = (d[0] - p[0]) / H;
	//~ dst[1] = (d[1] - p[1]) / H;
	//~ dst[2] = (d[2] - p[2]) / H;
	dst[0] = (p[0] - d[0]) / H;
	dst[1] = (p[1] - d[1]) / H;
	dst[2] = (p[2] - d[2]) / H;
	return dst;
}

double* diffT_gen(double dst[3], double s, double t, double* (*evalP)(double [3], double s, double t)) {
	double d[3], p[3];
	evalP(d, s, t + H);
	evalP(p, s, t);
	//~ dst[0] = (d[0] - p[0]) / H;
	//~ dst[1] = (d[1] - p[1]) / H;
	//~ dst[2] = (d[2] - p[2]) / H;
	dst[0] = (p[0] - d[0]) / H;
	dst[1] = (p[1] - d[1]) / H;
	dst[2] = (p[2] - d[2]) / H;
	return dst;
}

double* evalN(double dst[3], double s, double t, double* (*evalP)(double [3], double s, double t)) {
	double pt[3], ds[3], dt[3], len;
	evalP(pt, s, t);
	evalP(ds, s + H, t);
	evalP(dt, s, t + H);
	ds[0] = (pt[0] - ds[0]) / H;
	ds[1] = (pt[1] - ds[1]) / H;
	ds[2] = (pt[2] - ds[2]) / H;
	dt[0] = (pt[0] - dt[0]) / H;
	dt[1] = (pt[1] - dt[1]) / H;
	dt[2] = (pt[2] - dt[2]) / H;

	dst[0] = ds[1] * dt[2] - ds[2] * dt[1];
	dst[1] = ds[2] * dt[0] - ds[0] * dt[2];
	dst[2] = ds[0] * dt[1] - ds[1] * dt[0];
	len = sqrt(dst[0]*dst[0] + dst[1]*dst[1] + dst[2]*dst[2]);
	if (len > 0) {
		dst[0] /= len;
		dst[1] /= len;
		dst[2] /= len;
	}

	//~ dv3nrm(dst, dv3crs(dst, ds, dt));
	return dst;
}

/*int mlcolor(double pos[3], double Normal[3]) {
	/ *	VertexColor(r, g, b, alpha)  =
			emis_M + 
			//~ ambi_LIGHTMODEL * M.ambi +
			attn_i * spotlighteffect_i *
			(
				L[i].ambi * M.ambi +
				L[i].diff * M.diff * max(dot(L, n), 0) +
				L[i].spec * M.spec * pow(max(dot(s, n), 0)), M.SHININESS)
			)
			Sum(i = 0; N - 1; i++) {
				//~ dist = pos_L - pos_V;
				//~ attn = 1 / (K1 + K2 � dist + K3 � dist ** 2);
				//~ L = unit vector from vertex to light position
				//~ n = unit normal (at a vertex)
				//~ s = (x  + (0, 0, 1) ) / | x + (0, 0, 1) |
				//~ x = vector indicating direction from light source (or direction of directional light)
				attn * (spotlighteffect)i
				* [ ambi_L * ambi_M  +
				max (dot(L, n), 0)  * diff_L * diff_M  +
				max (dot(s, n), 0) ) ** SHININESS     *  
				specular_L  *  specular_M] 
			} i
	* /
	double col[3];
	unsigned i, n = sizeof(Lights) / sizeof(*Lights);
	dv3cpy(col, material.emis);
	for (i = 0; i < n; i++) {
		double tmp[3];//, dir[3];
		//~ double dist = dv3dot(dv3sub(tmp, Lights[i].pos, pos), Normal);
		double diff = -dv3dot(dv3nrm(tmp, dv3sub(tmp, pos, Lights[i].pos)), Normal);
		//~ double spec;
		//~ dv3sub(tmp, pos, Lights[i].pos);
		//~ dv3nrm(dir, dv3sub(tmp, pos, Lights[i].pos));								// dir of light
		//~ spec = dv3dot(dv3nrm(tmp, dv3add(tmp, dv3set(tmp, 0,0,1), dir)), Normal);
			// l = pow(max(dot(s, n), 0)), SHININESS)
			// s = directional light ? light.dir : normalize (vector indicating direction from light source + (0, 0, 1))
		//~ double spec = dv3dot(Normal, s);
		//~ spec := dot(Normal, Lights[i].H);
		dv3add(col, col, dv3mul(tmp, material.ambi, Lights[i].ambi));
		if (diff > 0) dv3add(col, col, dv3sca(tmp, dv3mul(tmp, material.diff, Lights[i].diff), diff));
		//~ if (spec > 0) dv3add(col, col, dv3sca(tmp, dv3mul(tmp, material.spec, Lights[i].spec), pow(spec, material.spec[3])));
	}// * /
	if (col[0] > 1) col[0] = 1; else if (col[0] < 0) col[0] = 0;
	if (col[1] > 1) col[1] = 1; else if (col[1] < 0) col[1] = 0;
	if (col[2] > 1) col[2] = 1; else if (col[2] < 0) col[2] = 0;
	return (0.299 * col[0] + 0.587 * col[1] + 0.114 * col[2]) * 255;
}

//*/

double* evalP_sphere(double dst[3], double s, double t) {
	const double Tx = 0, Sx =  40.0;
	const double Ty = 0, Sy =  40.0;
	const double Tz = 0, Sz =  40.0;
	const double s_min =  0.0, s_max =  1*3.14159265358979323846;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = Tx + Sx * (sin(s) * cos(t));
	dst[1] = Ty + Sy * (sin(s) * sin(t));
	dst[2] = Tz + Sz * (cos(s));
	return dst;
}

double* evalP_apple(double dst[3], double s, double t) {
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min = -3.14159265358979323846, t_max =  3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = cos(s) * (4 + 3.8 * cos(t));
	dst[1] = sin(s) * (4 + 3.8 * cos(t));
	dst[2] = (cos(t) + sin(t) - 1) * (1 + sin(t)) * log(1 - 3.14159265358979323846 * t / 10) + 7.5 * sin(t);
	return dst;
}

double* evalP_tours(double dst[3], double s, double t) {
	const double R = 20;
	const double r = 10;
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = (R + r * sin(s)) * cos(t);
	dst[1] = (R + r * sin(s)) * sin(t);
	dst[2] = r * cos(s);
	return dst;
}

double* evalP_cone(double dst[3], double s, double t) {
	const double l = 2;
	const double s_min = -1.5, s_max =  1.5;
	//~ const double s_min =  0.0, s_max =  1.5;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = s * sin(t);
	dst[1] = s * cos(t);
	dst[2] = l * s;
	return dst;
}

double* evalP_001(double dst[3], double s, double t) {
	const double Sx =  20.0;
	const double Sy =  20.0;
	const double Sz =  20.0;
	double sst, H = 10 / 20.;
	const double s_min = -20.0, s_max = +20.0;
	const double t_min = -20.0, t_max = +20.0;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	if ((sst = sqrt(s*s+t*t)) == 0) sst = 1.;
	dst[0] = Sx * s / 20;
	dst[1] = Sy * t / 20;
	dst[2] = Sz * H * sin(sst) / sst;
	return dst;
}

double* evalP_002(double dst[3], double s, double t) {
	const double Rx =  1.0;
	const double Ry =  1.0;
	const double Rz =  1.0;
	const double s_min =  0.0, s_max =  2*3.14159265358979323846;
	const double t_min =  0.0, t_max =  2*3.14159265358979323846;
	s = s * (s_max - s_min) + s_min;
	t = t * (t_max - t_min) + t_min;
	dst[0] = Rx * cos(s) / (sqrt(2) + sin(t));
	dst[1] = Ry * sin(s) / (sqrt(2) + sin(t));
	dst[2] = -Rz / (sqrt(2) + cos(t));
	return dst;
}

void mesheval(double* evalP(double [3], double s, double t), unsigned sdiv, unsigned tdiv, unsigned closed) {
	unsigned si, ti, mp[256][256];
	double pos[3], s, ds = 1. / (sdiv - (closed & 1 ? 0 : 1));
	double nrm[3], t, dt = 1. / (tdiv - (closed & 2 ? 0 : 1));
	if (sdiv > 256 || tdiv > 256) return;
	for (s = si = 0; si < sdiv; si += 1, s += ds) {
		for (t = ti = 0; ti < tdiv; ti += 1, t += dt)
			mp[si][ti] = addvtxdv(evalP(pos, s, t), evalN(nrm, s, t, evalP));
	}
	for (ti = 0; ti < tdiv - 1; ti += 1) {
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si + 0][ti + 0];		// 		v0--v3
			int v3 = mp[si + 0][ti + 1];		// 		| \  |
			int v1 = mp[si + 1][ti + 0];		// 		|  \ |
			int v2 = mp[si + 1][ti + 1];		// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv - 1][ti + 0];		// 		v0--v3
			int v3 = mp[sdiv - 1][ti + 1];		// 		| \  |
			int v1 = mp[0][ti + 0];				// 		|  \ |
			int v2 = mp[0][ti + 1];				// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
	}
	if (closed & 2) {			// closed t
		for (si = 0; si < sdiv - 1; si += 1) {
			int v0 = mp[si + 0][tdiv - 1];		// 		v0--v3
			int v3 = mp[si + 0][0];				// 		| \  |
			int v1 = mp[si + 1][tdiv - 1];		// 		|  \ |
			int v2 = mp[si + 1][0];				// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
		if (closed & 1) {			// closed s
			int v0 = mp[sdiv - 1][tdiv - 1];		// 		v0--v3
			int v3 = mp[sdiv - 1][0];				// 		| \  |
			int v1 = mp[0][tdiv - 1];				// 		|  \ |
			int v2 = mp[0][0];						// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
	}// */
}

void mesheval2(double* evalP(double [3], double s, double t), unsigned sdiv, unsigned tdiv) {
	double s0, s1, ds = 1. / sdiv;
	double t0, t1, dt = 1. / tdiv;
	for (s1 = (s0 = 0) + ds; s0 < 1; s1 = (s0 = s1) + ds) {
		for (t1 = (t0 = 0) + dt; t0 < 1; t1 = (t0 = t1) + dt) {
			double vertex[4][3], normal[4][3];
			//~ evalP(vertex[0], s0, t0); evalN(normal[0], s0, t0, evalP);
			//~ evalP(vertex[1], s0, t1); evalN(normal[1], s0, t1, evalP);
			//~ evalP(vertex[2], s1, t0); evalN(normal[2], s1, t0, evalP);
			//~ evalP(vertex[3], s1, t1); evalN(normal[3], s1, t1, evalP);
			//~ int v0 = addvtxdv(evalP(vertex[0], s0, t0), evalN(normal[0], s0, t0, evalP));		// 		v0->v1
			//~ int v1 = addvtxdv(evalP(vertex[1], s0, t1), evalN(normal[1], s0, t1, evalP));		// 		| \  |
			//~ int v3 = addvtxdv(evalP(vertex[3], s1, t0), evalN(normal[3], s1, t0, evalP));		// 		|  \ |
			//~ int v2 = addvtxdv(evalP(vertex[2], s1, t1), evalN(normal[2], s1, t1, evalP));		// 		v3<-v2
			int v0 = addvtxdv(evalP(vertex[0], s0, t0), evalN(normal[0], s0, t0, evalP));		// 		v0--v3
			int v3 = addvtxdv(evalP(vertex[1], s0, t1), evalN(normal[1], s0, t1, evalP));		// 		| \  |
			int v1 = addvtxdv(evalP(vertex[3], s1, t0), evalN(normal[3], s1, t0, evalP));		// 		|  \ |
			int v2 = addvtxdv(evalP(vertex[2], s1, t1), evalN(normal[2], s1, t1, evalP));		// 		v1--v2
			addtri(v0, v1, v2);
			addtri(v0, v2, v3);
		}
	}
}

/*void mesh1(float sx, float sy, float sz, float tx, float ty, float tz, double* evalP(double [3], double s, double t), unsigned sdiv, unsigned tdiv) {
	vector S, T, tmp;
	double s0, s1, ds = 1. / sdiv;
	double t0, t1, dt = 1. / tdiv;
	vecldf(&S, sx, sy, sz, 1);
	vecldf(&T, tx, ty, tz, 1);
	for (s1 = (s0 = 0) + ds; s0 < 1; s1 = (s0 = s1) + ds) {
		for (t1 = (t0 = 0) + dt; t0 < 1; t1 = (t0 = t1) + dt) {
			double vertex[4][3], normal[4][3];
			//~ int v0 = addvtxdv(evalP(vertex[0], s0, t0), evalN(normal[0], s0, t0, evalP));		// 		v0--v1
			//~ int v1 = addvtxdv(evalP(vertex[1], s0, t1), evalN(normal[1], s0, t1, evalP));		// 		| \  |
			//~ int v3 = addvtxdv(evalP(vertex[3], s1, t0), evalN(normal[3], s1, t0, evalP));		// 		|  \ |
			//~ int v2 = addvtxdv(evalP(vertex[2], s1, t1), evalN(normal[2], s1, t1, evalP));		// 		v3--v2
			int v0 = addvtxdv(evalP(vertex[0], s0, t0), evalN(normal[0], s0, t0, evalP));		// 		v0--v3
			int v1 = addvtxdv(evalP(vertex[1], s1, t0), evalN(normal[1], s1, t0, evalP));		// 		| \  |
			int v2 = addvtxdv(evalP(vertex[3], s1, t1), evalN(normal[3], s1, t1, evalP));		// 		|  \ |
			int v3 = addvtxdv(evalP(vertex[2], s0, t1), evalN(normal[2], s0, t1, evalP));		// 		v1--v2
			//~ int n0 = mlcolor(vertex[0], normal[0]);
			//~ int n1 = mlcolor(vertex[1], normal[1]);
			//~ int n3 = mlcolor(vertex[3], normal[3]);
			//~ int n2 = mlcolor(vertex[2], normal[2]);
			int n0 = 32;
			int n1 = 64;
			int n3 = 128;
			int n2 = 64;
			addtri(v0, v1, v2, n0, n1, n2);
			addtri(v0, v2, v3, n0, n2, n3);
			vecadd(&vtxptr[v0].pos, vecmul(&vtx.pos[v0], &vtx.pos[v0], &S), &T); vtx.pos[v0].w = sconst(1);
			vecadd(&vtx.pos[v1], vecmul(&vtx.pos[v1], &vtx.pos[v1], &S), &T); vtx.pos[v1].w = sconst(1);
			vecadd(&vtx.pos[v2], vecmul(&vtx.pos[v2], &vtx.pos[v2], &S), &T); vtx.pos[v2].w = sconst(1);
			vecadd(&vtx.pos[v3], vecmul(&vtx.pos[v3], &vtx.pos[v3], &S), &T); vtx.pos[v3].w = sconst(1);
		}
	}
}*/
//}

//~ /*
int nmx=0, nmy=0, omx=0, omy=0, btn = 0, act = 0;

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

void centmesh() {
	unsigned i;
	vector min, max, center;
	for (i = 0; i < mesh.vtxcnt; i += 1) {
		if (i == 0) {
			veccpy(&min, veccpy(&max, &mesh.vtxptr[i].pos));
		}
		else {
			vecmax(&max, &max, &mesh.vtxptr[i].pos);
			vecmin(&min, &min, &mesh.vtxptr[i].pos);
		}
	}
	vecadd(&center, &max, &min);
	vecsca(&center, &center, 1./2);
	printf("min : %+f, %+f, %+f\n", min.x, min.y, min.z);
	printf("max : %+f, %+f, %+f\n", max.x, max.y, max.z);
	//~ printf("cen : %+f, %+f, %+f\n", center.x, center.y, center.z);
	for (i = 0; i < mesh.vtxcnt; i += 1) {
		vecsub(&mesh.vtxptr[i].pos, &mesh.vtxptr[i].pos, &center);
		mesh.vtxptr[i].pos.w = 1;
	}
}

/*void optimesh() {
	unsigned i, j, k, n;
	for (n = i = 1; i < mesh.vtxcnt; i += 1) {
		for (j = 0; j < n && !vecequ(&mesh.vtxptr[i].pos, &mesh.vtxptr[j].pos, 0); j += 1);
		if (j == n) mesh.vtxptr[n++] = mesh.vtxptr[i];
		else vecadd(&mesh.vtxptr[j].nrm, &mesh.vtxptr[j].nrm, &mesh.vtxptr[i].nrm);
		for (k = 0; k < mesh.tricnt; k += 1) {
			if (mesh.triptr[k].i1 == i) mesh.triptr[k].i1 = j;
			if (mesh.triptr[k].i2 == i) mesh.triptr[k].i2 = j;
			if (mesh.triptr[k].i3 == i) mesh.triptr[k].i3 = j;
		}
	}
	for (j = 0; j < n; j += 1) vecnrm(&mesh.vtxptr[j].nrm, &mesh.vtxptr[j].nrm);
	mesh.vtxcnt = n;
}*/

float S = 180.;
camera cam;
vector eye, tgt, up;
vector X = {{sconst(1), 0, 0, 0}};
vector Y = {{0, sconst(1), 0, 0}};
vector Z = {{0, 0, sconst(1), 0}};

int readf(char* name) {
	FILE *f = fopen(name, "r");
	int cnt, n;
	if (f == NULL) return -1;

	fscanf(f, "%d", &cnt);
	if (fgetc(f) != ';') return 1;
	n = cnt;
	for (n = 0; n < cnt; n += 1) {
		double pos[3];
		fscanf(f, "%lf", &pos[0]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &pos[1]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &pos[2]);
		if (fgetc(f) != ';') return 1;
		switch (fgetc(f)) {
			default  : return -4;
			case ';' : if (n != cnt-1) {printf("pos: n != cnt : %d != %d\n", n, cnt); return -2;}
			case ',' : break;
		}
		if (n != addvtxdv(pos, NULL)) return -99;
	}
	for (n = 0; n < cnt; n += 1) {
		double nrm[3];
		fscanf(f, "%lf", &nrm[0]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &nrm[1]);
		if (fgetc(f) != ';') return 1;
		fscanf(f, "%lf", &nrm[2]);
		if (fgetc(f) != ';') return 1;
		switch (fgetc(f)) {
			default  : return -4;
			case ';' : if (n != cnt-1) {printf("pos: n != cnt : %d != %d\n", n, cnt); return -2;}
			case ',' : break;
		}
		vecldf(&mesh.vtxptr[n].nrm, nrm[0], nrm[1], nrm[2], 0);
		//setvtx(n, NULL, nrm);
	}
	fscanf(f, "%d", &cnt);
	if (fgetc(f) != ';') return 1;
	while (cnt--) {
		unsigned n, tri[3];
		fscanf(f, "%d", &n);
		if (fgetc(f) != ';') return 1;
		if (n == 3) {
			fscanf(f, "%d", &tri[0]);
			if (fgetc(f) != ',') return 2;
			fscanf(f, "%d", &tri[1]);
			if (fgetc(f) != ',') return 2;
			fscanf(f, "%d", &tri[2]);
			if (fgetc(f) != ';') return 2;
			switch (fgetc(f)) {
				default  : return -5;
				case ',' : break;
				case ';' : if (cnt) return -3;
			}
			addtri(tri[0], tri[1], tri[2]);
		}
		else 
			fprintf(stdout, "tri : cnt:%d n:%d\n", cnt, n);
	}
	return 0;
}

int saveMesh (char* name) {
	unsigned n;
	FILE *f = fopen(name, "w");
	if (f == NULL) return -1;

	fprintf(f, "%d\n", mesh.vtxcnt);
	for (n = 0; n < mesh.vtxcnt; n += 1) {
		fprintf(f,   "%f %f %f", mesh.vtxptr[n].pos.x, mesh.vtxptr[n].pos.y, mesh.vtxptr[n].pos.z);
		fprintf(f, ", %f %f %f", mesh.vtxptr[n].nrm.x, mesh.vtxptr[n].nrm.y, mesh.vtxptr[n].nrm.z);
		fprintf(f,    ", %f %f", mesh.vtxptr[n].s / 65535., mesh.vtxptr[n].t / 65535.);
		fprintf(f, ";\n");
	}
	fprintf(f, "%d\n", mesh.tricnt);
	for (n = 0; n < mesh.tricnt; n += 1) {
		fprintf(f, "%d %d %d\n", mesh.triptr[n].i1, mesh.triptr[n].i2, mesh.triptr[n].i3);
	}
	fclose(f);
	return 0;
}

int readMesh (char* name) {
	int cnt, n;
	FILE *f = fopen(name, "r");
	if (f == NULL) return -1;
	fscanf(f, "%d", &cnt);
	for (n = 0; n < cnt; n += 1) {
		double pos[3], nrm[3], tex[2];
		fscanf(f, "%lf%lf%lf", &pos[0], &pos[1], &pos[2]);
		if (fgetc(f) != ',') return -9;
		fscanf(f, "%lf%lf%lf", &nrm[0], &nrm[1], &nrm[2]);
		if (fgetc(f) != ',') return -9;
		fscanf(f, "%lf%lf", &tex[0], &tex[1]);
		if (fgetc(f) != ';') return -9;
		if (feof(f)) break;
		addvtxdv(pos, nrm);
	}
	fscanf(f, "%d", &cnt);
	for (n = 0; n < cnt; n += 1) {
		unsigned tri[3];
		fscanf(f, "%u%u%u", &tri[0], &tri[1], &tri[2]);
		//~ if (fgetc(f) != ';') return -9;
		//~ if (feof(f)) break;
		addtri(tri[0], tri[1], tri[2]);
	}
	fclose(f);
	return 0;
}

int readMobj (char* name) {
	int chr, v[3], i=0;
	char buff[65536];
	FILE *f = fopen(name, "r");
	if (f == NULL) return -1;
	for (;;) {
		fscanf(f, "%s", buff);
		if (feof(f)) break;
		if (*buff == 'v') {
			if (buff[1] == 't') {
				while ((chr = fgetc(f)) != -1) {
					if (chr == '\n') break;
				}
			}
			else {
				double pos[3];
				if (i >= 3) {
					fclose(f);
					return -2;
				}
				fscanf(f, "%lf%lf%lf", &pos[0], &pos[1], &pos[2]);
				v[i++] = addvtxdv(pos, 0);
			}
		}
		else if (*buff == 'f') {
			vector N;
			if (i != 3) {
				fclose(f);
				return -3;
			}
			//~ mesh.vtxptr[v[0]].pos, v[1], v[2]
			backface(&N, &mesh.vtxptr[v[0]].pos, &mesh.vtxptr[v[1]].pos, &mesh.vtxptr[v[2]].pos);
			mesh.vtxptr[v[0]].nrm = N;
			mesh.vtxptr[v[1]].nrm = N;
			mesh.vtxptr[v[2]].nrm = N;
			addtri(v[0], v[1], v[2]);
			i = 0;
		}
		else /*if (*buff == '#') */ {
			while ((chr = fgetc(f)) != -1) {
				if (chr == '\n') break;
			}
		}
	}
	fclose(f);
	return 0;
}

void kbdHND(int lkey, int key) {
	const scalar d = sconst(1.);
	const scalar D = sconst(.2);
	scalar z = cam.zoom;
	//~ int shift = (GetKeyState(lkey) & 0x80000000) != 0;
	switch (key) {
		case  27 : act = -1; break;
		case  13 : camset(&cam, &eye, &tgt, &up); cam.zoom = z;break;
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
		case 'z' : cam.zoom = sconst(1); break;

		case '`' : {
			FILE* pgm = fopen("screen.pgm", "wb");
			fprintf(pgm, "P5\n# Mandelbrot set\n");
			fprintf(pgm, "%3d %3d\n%3d\n", SCRW, SCRH, 255);
			fwrite(cBuff, 1, SCRW*SCRH, pgm);
			fclose(pgm);
		} break;
		case '~' : {
			FILE* f = fopen("pos.txt", "w");
			//~ fprintf(f, "x : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.xx), scatof(worldM.xy), scatof(worldM.xz), scatof(worldM.xt));
			//~ fprintf(f, "y : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.yx), scatof(worldM.yy), scatof(worldM.yz), scatof(worldM.yt));
			//~ fprintf(f, "z : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.zx), scatof(worldM.zy), scatof(worldM.zz), scatof(worldM.zt));
			//~ fprintf(f, "w : %+.3f, %+.3f, %+.3f, %+f\n", scatof(worldM.wx), scatof(worldM.wy), scatof(worldM.wz), scatof(worldM.wt));
			fprintf(f, "e : %+.3f, %+.3f, %+.3f, %+f\n", scatof(eye.x), scatof(eye.y), scatof(eye.z), scatof(eye.w));
			fclose(f);
		} break;
		//~ case '\\': optimesh(); break;
		case 'x' : matidn(&viewM); break;
		case 'X' : vecldf(&eye, S, 0, 0, 1); camset(&cam, &eye, &tgt, &up); break;
		case 'Y' : vecldf(&eye, 0, S, 0, 1); camset(&cam, &eye, &tgt, &up);break;
		case 'Z' : vecldf(&eye, 0, 0, S, 1); camset(&cam, &eye, &tgt, &up);break;
		
		case '/' : draw ^= wire; break;
		case '\t': draw ^= fill; break;
		case '?' : draw ^= back; break;
		case 'n' : draw ^= dspN; break;
		case 'l' : draw ^= dspL; break;
		case 'g' : draw ^= flat; break;
		case '1' : Lights[0].attr ^= L_on; break;
		case '2' : Lights[1].attr ^= L_on; break;

		//~ case 'm': if (mapvec == mapvecd) mapvec = mapvecm; else mapvec = mapvecd; break;
	}
}

void mousehnd(int x, mouseinf* m) {
	nmx = m->relx;
	nmy = m->rely;
	btn = m->button;
	act = x;
}

/*void coor(float sx, float sy, float sz) {
	int oyz = 32;
	int oxz = 64;
	int oxy = 128;
	int xyz = 255;
	int o = add_pd(0., 0., 0.);		// O
	int x = add_pd(sx, 0., 0.);		// X
	int y = add_pd(0., sy, 0.);		// Y
	int z = add_pd(0., 0., sz);		// Z
	addtri(o, x, y, oxy, oxy, oxy);
	addtri(o, y, z, oyz, oyz, oyz);
	addtri(o, x, z, oxz, oxz, oxz);
	addtri(x, y, z, xyz, xyz, xyz);		// xyz
	xyz = 0;
}

void cube(float sx, float sy, float sz) {
	int v0, v1, v2, v3;
	int p000 = add_pd(-sx, -sy, -sz);
	int p001 = add_pd(-sx, -sy, +sz);
	int p010 = add_pd(-sx, +sy, -sz);
	int p011 = add_pd(-sx, +sy, +sz);
	int p100 = add_pd(+sx, -sy, -sz);
	int p101 = add_pd(+sx, -sy, +sz);
	int p110 = add_pd(+sx, +sy, -sz);
	//~ int p111 = add_pd(+sx, +sy, +sz);

	v0 = p010; v1 = p110; v2 = p100; v3 = p000;		// x = 0
	addtri(v0, v1, v2, 255, 255, 255);
	addtri(v0, v2, v3, 255, 255, 255);

	v0 = p100; v1 = p101; v2 = p001; v3 = p000;		// y = 0
	addtri(v0, v1, v2, 255, 255, 255);
	addtri(v0, v2, v3, 255, 255, 255);
	
	v0 = p011; v1 = p010; v2 = p000; v3 = p001;		// z = 0
	addtri(v0, v1, v2, 255, 255, 255);
	addtri(v0, v2, v3, 255, 255, 255);
	
	
}
*/
const int division = 32;

int main() {
	int i, j, frames = 0;//, zoom = 0;
	time_t nt, ot;
	clock_t cc;
	struct {
		unsigned char b,g,r,a;
	} pal[256];
	for (i = 0, j = 1; i < 256; ++i, j+=2) {
		pal[i].r = i;
		pal[i].g = i;
		pal[i].b = i;
	}
	k3d_open();
	//~ coor(1., 1., 1.);
	//~ cube(1., 1., 1.);
	//~ printf ("reading mesh : %d\n", readf("tp.txt"));
	//~ mesheval(evalP_sphere, division, division, 0);
	//~ readMesh("sphere.txt");
	//~ mesheval(evalP_apple, division, division, 1);
	//~ mesheval(evalP_tours, division, division, 1+2);
	//~ mesheval(evalP_cone, division, division, 2);
	//~ mesheval(evalP_001, division, division, 0);
	//~ mesheval(evalP_002, division, division, 0);
	//~ precalcL(0);
	//~ saveMesh("xxx.txt");
	printf("%d\n", readMobj("manta.obj"));
	//~ printf("%d\n", readMobj("scorpion.obj"));
	//~ printf("%d\n", readMobj("raptor.obj"));
	printf("vtx cnt : %d / %d\n", mesh.vtxcnt, mesh.maxvtx);
	printf("tri cnt : %d / %d\n", mesh.tricnt, mesh.maxtri);
	centmesh();
	//~ optimesh();

	vecldf(&up,   0,  1,  0, 1);
	vecldf(&tgt,  0,  0,  0, 1);
	vecldf(&eye,  S,  S,  S, 1);

	camset(&cam, &eye, &tgt, &up);
	matidn(&viewM);
	projv_mat(&projM, 45, (SCRW+.0)/SCRH, 1, 10000, 0);

	printf("vtx cnt : %d / %d\n", mesh.vtxcnt, mesh.maxvtx);
	printf("tri cnt : %d / %d\n", mesh.tricnt, mesh.maxtri);
	printf("press any key ...\n");
	if (getch()== 27) {
		k3d_exit();
		return 0;
	}// */

	#define MCGA 1
	#if MCGA
	MCGA_open();
	setpalette(pal);
	#endif
	nt = ot = time(NULL);
	initMouse(mousehnd, -1);
	bndMouse(&rec);
	omx = nmx; omy = nmy;
	for(cc = clock(); ; frames++) {
		if (kbhit()) kbdHND(0, getch());
		if (act == -1) break;
		//~ if (act & rat_rbr && btn == 2) zoom ^= 2;
		/*if (btn == 3) {
			//~ if (zoom & 2)
				//~ cam.zoom = sconst(1);
			//~ else 
			if (cam.zoom < sconst(24))
				cam.zoom = scamul(cam.zoom, sconst(1.05));
		}// */
		if (act & rat_mov) {
			const float MOUSEROT = 1. / 360;
			const float MOUSEMOV = 1. / 10;
			if (btn == 0) {
				matrix rot;
				//~ if (nmy != omy) matmul(&viewM, &viewM, matldR(&rot, &X, scaldf((nmy - omy) * (MOUSEROT))));
				//~ if (nmx != omx) matmul(&viewM, &viewM, matldR(&rot, &Y, scaldf((nmx - omx) * (MOUSEROT))));
				//~ if (nmy != omy) matmul(&viewM, &viewM, matldR(&rot, &cam.dirR, scaldf((nmy - omy) * (MOUSEROT))));
				//~ if (nmx != omx) matmul(&viewM, &viewM, matldR(&rot, &cam.dirU, scaldf((nmx - omx) * (MOUSEROT))));

				if (nmy != omy) matmul(&viewM, &viewM, matldR(&rot, &viewM.x, scaldf((nmy - omy) * (MOUSEROT))));
				if (nmx != omx) matmul(&viewM, &viewM, matldR(&rot, &viewM.y, scaldf((nmx - omx) * (MOUSEROT))));
				vecnrm(&viewM.x, &viewM.x);
				vecnrm(&viewM.y, &viewM.y);
				vecnrm(&viewM.z, &viewM.z);
				//~ vecnrm(&worldM.y, &worldM.y);
				//~ veccp3(&worldM.z, &worldM.x, &worldM.y);
				//~ if (nmy != omy) cammov(&cam, &cam.dirF, scaldf((nmy - omy) * MOUSEMOV));

			}
			//~ if (nmy != omy) camrot(&cam, &cam.dirR, scaldf((nmy - omy) * MOUSEROT));		// pitch : up/down
			//~ if (nmx != omx) camrot(&cam, &cam.dirU, scaldf((nmx - omx) * MOUSEROT));		// turn (walk): left/rigth
			//~ if (nmx != omx) camrot(&cam, &up, scaldf((nmx - omx) * MOUSEROT));				// turn (fly) : left/rigth
			//~ if (nmx != omx) camrot(&cam, &cam.dirF, scaldf((nmz - omz) * MOUSEROT));		// roll
			else if (btn == 1) {
				if (nmy != omy) cammov(&cam, &cam.dirF, scaldf((nmy - omy) * MOUSEMOV));		// move
				if (nmx != omx) camrot(&cam, &cam.dirU, scaldf((nmx - omx) * MOUSEROT));		// turn
			}
			else if (btn == 2) {
				if (nmy != omy) camrot(&cam, &cam.dirR, scaldf((nmy - omy) * MOUSEROT));		// pitch : up/down
				if (nmx != omx) camrot(&cam, &cam.dirU, scaldf((nmx - omx) * MOUSEROT));		// move
			}// */
			else if (btn == 3) {
				if (nmx != omx) cammov(&cam, &cam.dirR, scaldf((nmx - omx) * MOUSEMOV));
				if (nmy != omy) cammov(&cam, &cam.dirU, scaldf((nmy - omy) * MOUSEMOV));
			}// */
			omx = nmx; omy = nmy;
		}
		g3_drawmesh(&cam);
		frames ++;
		#if MCGA
		/*/{	Draw Cross
		cBuff[((SCRW-1)/2-2)+((SCRH-1)/2-2)*SCRW] = 0;
		cBuff[((SCRW-1)/2-2)+((SCRH-1)/2-1)*SCRW] = 0;
		cBuff[((SCRW-1)/2-2)+((SCRH-1)/2+0)*SCRW] = 0;
		cBuff[((SCRW-1)/2-2)+((SCRH-1)/2+1)*SCRW] = 0;
		cBuff[((SCRW-1)/2-2)+((SCRH-1)/2+2)*SCRW] = 0;

		cBuff[((SCRW-1)/2-1)+((SCRH-1)/2-2)*SCRW] = 0;
		cBuff[((SCRW-1)/2-1)+((SCRH-1)/2-1)*SCRW] = 0;
		cBuff[((SCRW-1)/2-1)+((SCRH-1)/2+0)*SCRW] = -1;
		cBuff[((SCRW-1)/2-1)+((SCRH-1)/2+1)*SCRW] = 0;
		cBuff[((SCRW-1)/2-1)+((SCRH-1)/2+2)*SCRW] = 0;

		cBuff[((SCRW-1)/2+0)+((SCRH-1)/2-2)*SCRW] = 0;
		cBuff[((SCRW-1)/2+0)+((SCRH-1)/2-1)*SCRW] = -1;
		cBuff[((SCRW-1)/2+0)+((SCRH-1)/2+0)*SCRW] = -1;
		cBuff[((SCRW-1)/2+0)+((SCRH-1)/2+1)*SCRW] = -1;
		cBuff[((SCRW-1)/2+0)+((SCRH-1)/2+2)*SCRW] = 0;

		cBuff[((SCRW-1)/2+1)+((SCRH-1)/2-2)*SCRW] = 0;
		cBuff[((SCRW-1)/2+1)+((SCRH-1)/2-1)*SCRW] = 0;
		cBuff[((SCRW-1)/2+1)+((SCRH-1)/2+0)*SCRW] = -1;
		cBuff[((SCRW-1)/2+1)+((SCRH-1)/2+1)*SCRW] = 0;
		cBuff[((SCRW-1)/2+1)+((SCRH-1)/2+2)*SCRW] = 0;

		cBuff[((SCRW-1)/2+2)+((SCRH-1)/2-2)*SCRW] = 0;
		cBuff[((SCRW-1)/2+2)+((SCRH-1)/2-1)*SCRW] = 0;
		cBuff[((SCRW-1)/2+2)+((SCRH-1)/2+0)*SCRW] = 0;
		cBuff[((SCRW-1)/2+2)+((SCRH-1)/2+1)*SCRW] = 0;
		cBuff[((SCRW-1)/2+2)+((SCRH-1)/2+2)*SCRW] = 0;
		//} */
		//~ waitretrace();
		memcpy((void*)0xA0000, cBuff, 64000);
		#else
		if ((nt = time(NULL)) != ot) {
			double secs = (double)(clock() - cc) / CLOCKS_PER_SEC;
			printf("\nfps : %4.2f", frames / secs);
			ot = nt;
			frames = 0;
			cc = clock();
		}
		#endif
	}
	k3d_exit();
	exitMouse();
	MCGA_exit();
	frames = 0;
	return 0;
}//*/

/*
long fxpmul1(long, long);
long fxpmul2(long a, long b){
	register long long c = (long long)a * b;
	return (c + (c >> (FXPOINT-1))) >> FXPOINT;
	//~ return (c + (FXP_ONE / 2)) >> FXPOINT;
}

#if ((defined __WATCOMC__) && (FXPOINT == 16))
#pragma aux fxpmul1 parm [eax] [edx] modify [edx eax] value [eax] = \
	"imul	edx"\
	"shrd	eax, edx, 16";

#pragma aux fxpmul2 parm [eax] [edx] modify [edx ecx eax] value [eax] = \
	"imul	edx"\
	"mov	ecx, eax"\
	"shrd	ecx, edx, 17"\
	"add	eax, ecx"\
	"adc	edx, 0"\
	"shrd	eax, edx, 16";
#endif

int main() {
	float a = 3.141566;
	float b = 3.141566;
	printf ("%f * %f = %f / %f / %f\n", a, b, a*b, fxptof(fxpmul1(fxpldf(a), fxpldf(b))), fxptof(fxpmul2(fxpldf(a), fxpldf(b))));
	getch();
	return 0;
}//*/

/*
#include "DRV_WIN.c"
camera cam;
vector eye, tgt, up;

void ratHND(int mod, int x, int y) {
	//~ MK_CONTROL
	//~ The CTRL key is down.
	//~ MK_LBUTTON
	//~ The left mouse button is down.
	//~ MK_MBUTTON
	//~ The middle mouse button is down.
	//~ MK_RBUTTON
	//~ The right mouse button is down.
	//~ MK_SHIFT
	//~ The SHIFT key is down.
	static int lx = -1, ly = -1;
	//~ static int zoom = 0;
	int mx, my;
	if (lx == -1) {
		lx = x;
		ly = y;
	}
	mx = x - lx; lx = x;
	my = y - ly; ly = y;
	//~ if (mod == MK_LBUTTON) {
		//~ cammov(&cam, &cam.dirF, sconst(+.2));
	//~ }
	//~ if (mod == MK_RBUTTON) {
		//~ if (touched == 0x08) zoom ^= 2;
		//~ if (zoom & 2) {
			//~ cam.zoom = sconst(1);
		//~ } else
		//~ if (cam.zoom < sconst(24)) {
			//~ cam.zoom = scamul(cam.zoom, sconst(1.05));
		//~ }
	//~ }
	if (mx || my) {
		#define MOUSEMUL (3. / 360)
		camrot(&cam, &up, scaldf(MOUSEMUL*mx));				// turn : left/rigth
		camrot(&cam, &cam.dirR, scaldf(MOUSEMUL*my));		// pitch : up/down
		//~ camrot(&cam, &cam.dirU, scaldf((j - Beta) * MOUSEMUL));			// turn : left/rigth
		//~ camrot(&cam, &cam.dirF, scaldf((i - Alfa) * MOUSEMUL));			// roll
		#undef MOUSEMUL
	}
}

int main() {
	struct gx_Surf_t img = {{0},SCRW,SCRH,0,8, 1,SCRW,0,cBuff,0,0};
	MSG msg;
	msg.wParam = 0;

	k3d_open();
	intitWIN(SCRW, SCRH, kbdHND, ratHND);
	gx_initdraw(&img, rop2_cpy);

	//~ makecube(10,10,10,20,20,20);

	//~ makecoor(10,10,10);
	//~ vecldf(&tgt,  0,  0,  0, 1);
	//~ vecldf(&eye,  0,  0, 20, 1);

	makesphere(0, 0, 0, SPr, SPr, SPr);
	vecldf(&tgt,  0,  0,  1, 1);
	//~ vecldf(&eye,  0,  0,  -(100+SPr), 1);

	vecldf(&eye,  0,  0,  0, 1);

	vecldf(&up,  0,  1,  0, 1);
	camset(&cam, &eye, &tgt, &up);
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		k3d_render(&cam);
		gx_wincopy(&img);
	}
	doneWin();
	k3d_exit();
	return msg.wParam;
}//*/
//}-8<--------------------------------------------------------------------------
