#include <math.h>
#include "gx_surf.h"
#include "g3_draw.h"
#include "util.h"

void getFrustum(struct vector planes[6], matrix mat) {
	// near and far: -1 < z < 1
	vecnrm(&planes[0], vecadd(&planes[0], &mat->w, &mat->z));
	vecnrm(&planes[1], vecsub(&planes[1], &mat->w, &mat->z));
	// left and right: -1 < x < 1
	vecnrm(&planes[2], vecadd(&planes[2], &mat->w, &mat->x));
	vecnrm(&planes[3], vecsub(&planes[3], &mat->w, &mat->x));
	// top and bottom: -1 < y < 1
	vecnrm(&planes[4], vecadd(&planes[4], &mat->w, &mat->y));
	vecnrm(&planes[5], vecsub(&planes[5], &mat->w, &mat->y));
}

static vector boundSphere(vector s, vector p1, vector p2, vector p3) {
	vecadd(s, vecadd(s, p1, p2), p3);
	vecsca(s, s, (scalar) (1. / 3));                                    // set s to Baricentrum

	struct vector tmp[1];
	scalar len = vecdp3(tmp, vecsub(tmp, s, p1));
	scalar ln1 = vecdp3(tmp, vecsub(tmp, s, p2));
	scalar ln2 = vecdp3(tmp, vecsub(tmp, s, p3));
	if (len < ln1) {
		len = ln1;
	}
	if (len < ln2) {
		len = ln2;
	}
	s->w = sqrt(len);
	return s;
}

int testSphere(struct vector planes[6], vector p, scalar r) {
	if (vecdph(&planes[0], p) <= r) return 0;
	if (vecdph(&planes[1], p) <= r) return 0;
	if (vecdph(&planes[2], p) <= r) return 0;
	if (vecdph(&planes[3], p) <= r) return 0;
	if (vecdph(&planes[4], p) <= r) return 0;
	if (vecdph(&planes[5], p) <= r) return 0;
	return 1;    // inside
}

int testTriangle(struct vector planes[6], vector p1, vector p2, vector p3) {
	struct vector tmp[1];
	boundSphere(tmp, p1, p2, p3);
	return testSphere(planes, tmp, -tmp->w);
}

static inline int getOffs(GxImage dst, int x, int y) {
	return y * (dst->scanLen / 4) + x;
}

void setPixel3d(GxImage dst, int x, int y, unsigned z, argb c) {
	int offs = getOffs(dst, x, y);
	uint32_t *cBuff = (void *) dst->basePtr;
	uint32_t *zBuff = (void *) dst->tempPtr;

	if ((unsigned) y >= (unsigned) dst->height) {
		return;
	}
	if ((unsigned) x >= (unsigned) dst->width) {
		return;
	}

	if (zBuff[offs] > z) {
		zBuff[offs] = z;
		cBuff[offs] = c.val;
	}
}

typedef struct {
	struct vector emis;        // Emissive
	struct vector ambi;        // Ambient
	struct vector diff;        // Diffuse
	struct vector spec;        // Specular
	struct vector attn;        // Attenuation
	struct vector pos;         // light position
	struct vector dir;         // directional light
	scalar sCos;               // spotlight cutoff
	scalar sExp;               // spotlight
	scalar sPow;               // Shininess
} MtlLight;
typedef struct edge {
	int32_t x, dx, z, dz;           //
	int32_t s, ds, t, dt;           // texture
	int32_t r, dr, g, dg, b, db;    // RGB color
	//~ int32_t sz, dsz, tz, dtz, rz, drz;
	//~ float A, dA, S0, S1, T0, T1, Z0, Z1;
	//~ union vector pos, dp;
	//~ union vector nrm, dn;
	//~ union vector col, dc;
	//~ argb rgb;
} *edge;
// vertex projected to screen
typedef struct projVtx {
	int32_t x;
	int32_t y;
	int32_t z;	// 24-bit fixed point
	argb c;
	uint16_t s;
	uint16_t t;
} *projVtx;

static inline void drawPlot3d(GxImage dst, projVtx p) {
	setPixel3d(dst, p->x, p->y, p->z, p->c);
}

static inline int calcClip(GxRect roi, int x, int y) {
	return ((y >= roi->y1) << 0)
		   | ((y < roi->y0) << 1)
		   | ((x >= roi->x1) << 2)
		   | ((x < roi->x0) << 3);
}

static int clipLine3d(GxRect roi, int *x1, int *y1, int *z1, int *x2, int *y2, int *z2) {
	int c1 = calcClip(roi, *x1, *y1);
	int c2 = calcClip(roi, *x2, *y2);
	int x = 0, dx, y = 0, dy, z = 0, e;

	while (c1 | c2) {

		if (c1 & c2)
			return 0;

		e = c1 ? c1 : c2;
		dx = *x2 - *x1;
		dy = *y2 - *y1;

		switch (LOBIT(e)) {        // get the lowest bit set
			case 1:
				y = roi->y1 - 1;
				if (dy != 0) {
					dy = ((y - *y1) << 16) / dy;
				}
				x = *x1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
				break;

			case 2:
				y = roi->y0;
				if (dy != 0) {
					dy = ((y - *y1) << 16) / dy;
				}
				x = *x1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
				break;

			case 4:
				x = roi->x1 - 1;
				if (dx != 0) {
					dx = ((x - *x1) << 16) / dx;
				}
				y = *y1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
				break;

			case 8:
				x = roi->x0;
				if (dx != 0) {
					dx = ((x - *x1) << 16) / dx;
				}
				y = *y1 + ((dx * dy) >> 16);
				z = *z1 + ((dx * dy) >> 16);
				break;
		}
		if (c1 != 0) {
			*x1 = x;
			*y1 = y;
			*z1 = z;
			c1 = calcClip(roi, x, y);
		} else {
			*x2 = x;
			*y2 = y;
			*z2 = z;
			c2 = calcClip(roi, x, y);
		}
	}

	return 1;
}

static void drawLine3d(GxImage dst, projVtx p1, projVtx p2, argb c) {        // Bresenham
	int x1 = p1->x;
	int y1 = p1->y;
	int z1 = p1->z;
	int x2 = p2->x;
	int y2 = p2->y;
	int z2 = p2->z;

	int32_t sy = 1;
	int32_t dy = y2 - y1;
	if (dy < 0) {
		dy = -dy;
		sy = -1;
	}

	int32_t sx = 1;
	int32_t dx = x2 - x1;
	if (dx < 0) {
		dx = -dx;
		sx = -1;
	}

	struct GxRect rect = {
		.x0 = 0,
		.y0 = 0,
		.x1 = dst->width,
		.y1 = dst->height,
	};

	if (!clipLine3d(&rect, &x1, &y1, &z1, &x2, &y2, &z2)) {
		return;
	}

	if (dx > dy) {
		int32_t e = dx >> 1;
		int32_t zs = (z2 - z1) / dx;
		while (x1 != x2) {
			setPixel3d(dst, x1, y1, z1, c);
			if ((e += dy) > dx) {
				y1 += sy;
				e -= dx;
			}
			x1 += sx;
			z1 += zs;
		}
	}
	else if (dy) {
		int32_t e = dy >> 1;
		int32_t zs = (z2 - z1) / dy;
		while (y1 != y2) {
			setPixel3d(dst, x1, y1, z1, c);
			if ((e += dx) > dy) {
				x1 += sx;
				e -= dy;
			}
			y1 += sy;
			z1 += zs;
		}
	}
}

static inline void span_next(edge e) {
	e->z += e->dz;
	e->s += e->ds, e->t += e->dt;
	e->r += e->dr, e->g += e->dg, e->b += e->db;
}

static inline void edge_next(edge e) {
	e->x += e->dx;
	span_next(e);
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

static inline void span_init(edge e, edge l, edge r, int len, int skip) {
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

	if (skip > 0) {
		edge_skip(e, skip);
	}
}

static inline void edge_init(edge e, GxImage tex, projVtx vtx, int i1, int i2, int skip) {
	int32_t y1 = vtx[i1].y;
	int32_t y2 = vtx[i2].y;
	int32_t dy = y2 - y1;
	if (dy <= 0) {
		e->x = 0;
		return;
	}

	int32_t x1 = vtx[i1].x << 16;
	int32_t x2 = vtx[i2].x << 16;

	e->dx = (x2 - (e->x = x1)) / dy;
	e->dz = (vtx[i2].z - (e->z = vtx[i1].z)) / dy;

	e->dr = ((vtx[i2].c.r - vtx[i1].c.r) << 16) / dy;
	e->dg = ((vtx[i2].c.g - vtx[i1].c.g) << 16) / dy;
	e->db = ((vtx[i2].c.b - vtx[i1].c.b) << 16) / dy;

	e->r = vtx[i1].c.r << 16;
	e->g = vtx[i1].c.g << 16;
	e->b = vtx[i1].c.b << 16;

	if (tex != NULL) {
		int w = tex->width - 1;
		int h = tex->height - 1;
		e->ds = (vtx[i2].s * h - (e->s = vtx[i1].s * w)) / dy;
		e->dt = (vtx[i2].t * h - (e->t = vtx[i1].t * w)) / dy;

		// TODO: perspective correct texture mapping and mip-map selection
		//~ if (y1 == y2) w = X2 - X1;
		//~ else if (y2 == y3) w = X3 - X2;
		//~ else w = X1 < X3 ? X1 - X2 : X3 - X1;
		//~ if ((w >>= 16) < 0)w = -w;
		//~ if (tmp > SCRH || ((w < 0 ? -w : w) >> 16) > SCRW) return;
	} else {
		e->ds = e->dt = 0;
	}

	/*
	e->dA = (1 + (e->A = 0.)) / len;
	e->Z0 = z1; e->Z1 = z2;
	e->S0 = s1 / 65535.; e->S1 = s2 / 65535.;
	e->T0 = t1 / 65535.; e->T1 = t2 / 65535.;
	*/

	if (skip > 0) {
		edge_skip(e, skip);
	}
}

static void draw_tri_part(GxImage dst, GxRect roi, edge l, edge r, GxImage tex, int y1, int y2) {
	argb *cBuff = (void *) dst->basePtr;
	uint32_t *zBuff = (void *) dst->tempPtr;

	for (int y = y1; y < y2; y++) {
		int lx = l->x >> 16;
		int rx = r->x >> 16;
		int xlr = rx - lx;
		if (xlr > 0) {
			int sx = 0;
			int x = roi->x0;

			if (rx > roi->x1) {
				rx = roi->x1;
			}
			if (lx < x) {
				sx = x - lx;
				lx = x;
			}

			struct edge v = {0};
			span_init(&v, l, r, xlr, sx);
			int offs = getOffs(dst, lx, y);
			rx = offs + rx - lx;

			while (offs < rx) {
				if (zBuff[offs] > (uint32_t) v.z) {
					zBuff[offs] = v.z;
					if (tex) {
						argb col;
						col.val = getPixelLinear(tex, v.s, v.t);
						cBuff[offs].b = sat_s8((v.b >> 15) * col.b >> 8);
						cBuff[offs].g = sat_s8((v.g >> 15) * col.g >> 8);
						cBuff[offs].r = sat_s8((v.r >> 15) * col.r >> 8);
					} else {
						cBuff[offs].b = (uint8_t) (v.b >> 16);
						cBuff[offs].g = (uint8_t) (v.g >> 16);
						cBuff[offs].r = (uint8_t) (v.r >> 16);
					}
				}
				span_next(&v);
				offs++;
			}
		}
		edge_next(l);
		edge_next(r);
	}
}

static void drawTriangle3d(GxImage dst, GxImage tex, projVtx vtx, int i1, int i2, int i3) {
	// sort by y
	if (vtx[i1].y > vtx[i3].y) {
		i1 ^= i3;
		i3 ^= i1;
		i1 ^= i3;
	}
	if (vtx[i1].y > vtx[i2].y) {
		i1 ^= i2;
		i2 ^= i1;
		i1 ^= i2;
	}
	if (vtx[i2].y > vtx[i3].y) {
		i2 ^= i3;
		i3 ^= i2;
		i2 ^= i3;
	}

	int32_t y1 = vtx[i1].y;
	int32_t y2 = vtx[i2].y;
	int32_t y3 = vtx[i3].y;

	if (y1 == y3) {                // clip (y)
		return;
	}

	// calc slope y3 - y1 (the longest)
	struct GxRect roi = {
		.x0 = 0,
		.y0 = 0,
		.x1 = dst->width,
		.y1 = dst->height,
	};
	int y = roi.y0;
	int dy1 = y - y1;
	int dy2 = y - y2;

	struct edge v1 = {0};
	edge_init(&v1, tex, vtx, i1, i3, dy1);

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
	if (y3 > (y = roi.y1)) {
		if (y2 > y) {
			if (y1 > y) {
				return;
			}
			y2 = y;
		}
		y3 = y;
	}

	if (y1 < y2) {                // y1 < y < y2
		struct edge v2 = {0};
		edge_init(&v2, tex, vtx, i1, i2, dy1);
		if (v2.dx < v1.dx) {
			draw_tri_part(dst, &roi, &v2, &v1, tex, y1, y2);
		} else {
			draw_tri_part(dst, &roi, &v1, &v2, tex, y1, y2);
		}
	}
	if (y2 < y3) {                // y2 < y < y3
		struct edge v2 = {0};
		edge_init(&v2, tex, vtx, i2, i3, dy2);
		if (v2.x < v1.x) {
			draw_tri_part(dst, &roi, &v2, &v1, tex, y2, y3);
		} else {
			draw_tri_part(dst, &roi, &v1, &v2, tex, y2, y3);
		}
	}
}

static inline projVtx mapVertex(projVtx dst, matrix mat, vector src) {
	scalar div = vecdph(&mat->w, src);
	if (div != 0) {
		div = 1 / div;
	}
	dst->x = vecdph(&mat->x, src) * div;
	dst->y = vecdph(&mat->y, src) * div;
	dst->z = vecdph(&mat->z, src) * div;
	return dst;
}

static inline argb litVertex(vector color, vector V, vector N, vector E, MtlLight *light, size_t lightCnt) {
	struct vector tmp[8];
	for (size_t i = 0; i < lightCnt; ++i) {
		vector D = vecsub(tmp + 2, V, &light->pos);
		vector L = &light->dir;

		scalar spot = 1;
		if (vecdp3(L, L)) {               // directional or spotlight
			if (light->sCos) {            // spotlight
				spot = vecdp3(L, vecnrm(tmp, D));
				if (spot > cos(toRad(light->sCos))) {
					spot = pow(spot, light->sExp);
				} else {
					spot = 0;
				}
			}
		} else {
			L = vecnrm(tmp + 3, D);
		}

		scalar attn = spot / vecpev(&light->attn, veclen(D));

		// Ambient
		vecsca(tmp, &light->ambi, attn);
		vecadd(color, color, tmp);
		scalar dot = -vecdp3(N, L);
		if (dot > 0) {
			// Diffuse
			vecsca(tmp, &light->diff, attn * dot);
			vecadd(color, color, tmp);

			vector R = vecnrm(tmp + 5, vecrfl(tmp + 5, vecsub(tmp + 4, V, E), N));
			if ((dot = -vecdp3(R, L)) > 0) {
				// Specular
				vecsca(tmp, &light->spec, attn * pow(dot, light->sPow));
				vecadd(color, color, tmp);
			}
		}
		light++;
	}
	return vecrgb(color);
}

static int drawBbox(GxImage dst, GxMesh msh, matrix proj, argb bbox_col) {
	struct vector v[8] = {0};
	bboxMesh(msh, v + 0, v + 6);
	v[4].x = v[7].x = v[3].x = v[0].x;
	v[1].x = v[2].x = v[5].x = v[6].x;
	v[1].y = v[5].y = v[4].y = v[0].y;
	v[3].y = v[2].y = v[7].y = v[6].y;
	v[1].z = v[2].z = v[3].z = v[0].z;
	v[4].z = v[5].z = v[7].z = v[6].z;

	struct projVtx p[8] = {0};
	mapVertex(p + 0, proj, v + 0);
	mapVertex(p + 1, proj, v + 1);
	mapVertex(p + 2, proj, v + 2);
	mapVertex(p + 3, proj, v + 3);
	mapVertex(p + 4, proj, v + 4);
	mapVertex(p + 5, proj, v + 5);
	mapVertex(p + 6, proj, v + 6);
	mapVertex(p + 7, proj, v + 7);
	for (size_t i = 0; i < lengthOf(p); i++) {
		p[i].c = bbox_col;
		p[i].s = 0;
		p[i].t = 0;
	}

	// front
	drawLine3d(dst, p + 0, p + 1, bbox_col);
	drawLine3d(dst, p + 1, p + 2, bbox_col);
	drawLine3d(dst, p + 2, p + 3, bbox_col);
	drawLine3d(dst, p + 3, p + 0, bbox_col);

	// back
	drawLine3d(dst, p + 4, p + 5, bbox_col);
	drawLine3d(dst, p + 5, p + 6, bbox_col);
	drawLine3d(dst, p + 6, p + 7, bbox_col);
	drawLine3d(dst, p + 7, p + 4, bbox_col);

	// join
	drawLine3d(dst, p + 0, p + 4, bbox_col);
	drawLine3d(dst, p + 1, p + 5, bbox_col);
	drawLine3d(dst, p + 2, p + 6, bbox_col);
	drawLine3d(dst, p + 3, p + 7, bbox_col);
	return 0;
}

int drawMesh(GxImage dst, GxMesh msh, camera cam, GxLight lights, int mode) {
	static const argb line_col = {.val = 0xffffff};
	static const argb bbox_col = {.val = 0xff00ff};
	static const argb norm_col = {.val = 0xff0000};
	static struct projVtx pos[16 << 16]; // fixme: allocate in mesh

	if ((dst->flags & ImageType) != Image3d) {
		return -1;
	}
	if (msh->vtxcnt > lengthOf(pos)) {
		return -1;
	}

	if ((mode & keep_buff) == 0) {
		int *cBuff = (void *) dst->basePtr;
		for (int e = 0; e < dst->width * dst->height; e += 1) {
			cBuff[e] = 0;
		}
		int *zBuff = (void *) dst->tempPtr;
		for (int e = 0; e < dst->width * dst->height; e += 1) {
			zBuff[e] = 0x3fffffff;
		}
	}

	if ((mode & cull_mode) == cull_mode) {
		return 0;
	}
	if ((mode & draw_mode) == 0) {
		return 0;
	}

	GxImage texture = msh->mtl.texture;
	if (!msh->hasTex || (mode & draw_tex) == 0) {
		// texturing isn't needed
		texture = NULL;
	}
	if ((mode & draw_lit) == 0) {
		// lights aren't needed
		lights = NULL;
	}

	// prepare and pre-calculate lightning
	size_t lightCnt = 0;
	MtlLight light[32];
	for (GxLight l = lights; l != NULL; l = l->next) {
		if (lightCnt > lengthOf(light)) {
			gx_debug("too many lights. using only first 32");
			break;
		}

		if (!(l->attr & L_on)) {
			continue;
		}

		// pre multiply lights (fore back)
		MtlLight *mLight = &light[lightCnt];
		vecmul(&mLight->ambi, &msh->mtl.ambi, &l->ambi);
		vecmul(&mLight->diff, &msh->mtl.diff, &l->diff);
		vecmul(&mLight->spec, &msh->mtl.spec, &l->spec);
		mLight->emis = msh->mtl.emis;
		mLight->sPow = msh->mtl.spow;
		mLight->attn = l->attn;
		mLight->sExp = l->sExp;
		mLight->sCos = l->sCos;
		mLight->dir = l->dir;
		mLight->pos = l->pos;
		lightCnt += 1;
	}

	// prepare view and projection matrix
	struct matrix proj[2];
	cammat(proj, cam);
	// apply perspective matrix
	matmul(proj, &cam->proj, proj);
	// prepare screen viewport matrix
	matrix viewport = viewport_mat(proj + 1, dst->width, dst->height, 1 << 24);

	// extract frustum planes to clip invisible objects
	struct vector planes[6] = {0};
	getFrustum(planes, proj);
	// apply viewport after extracting the frustum
	matmul(proj, viewport, proj);

	// calculate positions, lightning, colors
	for (size_t i = 0; i < msh->vtxcnt; i += 1) {
		GxMeshVtx v = msh->vtxptr + i;
		mapVertex(pos + i, proj, &v->p);
		pos[i].s = texture ? v->s : 0;
		pos[i].t = texture ? v->t : 0;

		if (lights != NULL) {
			struct vector Col = msh->mtl.emis;
			pos[i].c = litVertex(&Col, &v->p, &v->n, &cam->pos, light, lightCnt);
		}
		else if (texture != NULL) {
			int s = v->s * (texture->width - 1);
			int t = v->t * (texture->height - 1);
			pos[i].c.val = getPixelLinear(texture, s, t);
		}
		else {
			pos[i].c = nrmrgb(&v->n);
		}
	}

	// draw triangles
	int triangles = 0;
	if (mode & draw_fill) {
		for (size_t i = 0; i < msh->tricnt; i += 1) {
			int32_t i1 = msh->triptr[i].i1;
			int32_t i2 = msh->triptr[i].i2;
			int32_t i3 = msh->triptr[i].i3;

			if (!testTriangle(planes, &msh->vtxptr[i1].p, &msh->vtxptr[i2].p, &msh->vtxptr[i3].p)) {
				continue;
			}

			// drop degenerated triangles
			float fA = (pos[i2].x - pos[i1].x) * (pos[i3].y - pos[i1].y);
			float fB = (pos[i2].y - pos[i1].y) * (pos[i3].x - pos[i1].x);
			switch (mode & cull_mode) {
				case cull_back:
					if (fA < fB) {
						continue;
					}
					break;

				case cull_front:
					if (fA > fB) {
						continue;
					}
					break;
			}

			drawTriangle3d(dst, texture, pos, i1, i2, i3);
			triangles += 1;
		}
	}
	if (mode & draw_wire) {
		for (size_t i = 0; i < msh->tricnt; i += 1) {
			int32_t i1 = msh->triptr[i].i1;
			int32_t i2 = msh->triptr[i].i2;
			int32_t i3 = msh->triptr[i].i3;

			if (!testTriangle(planes, &msh->vtxptr[i1].p, &msh->vtxptr[i2].p, &msh->vtxptr[i3].p)) {
				continue;
			}

			// drop degenerated triangles
			float fA = (pos[i2].x - pos[i1].x) * (pos[i3].y - pos[i1].y);
			float fB = (pos[i2].y - pos[i1].y) * (pos[i3].x - pos[i1].x);
			switch (mode & cull_mode) {
				case cull_back:
					if (fA < fB) {
						continue;
					}
					break;

				case cull_front:
					if (fA > fB) {
						continue;
					}
					break;
			}

			drawLine3d(dst, pos + i1, pos + i2, line_col);
			drawLine3d(dst, pos + i2, pos + i3, line_col);
			drawLine3d(dst, pos + i1, pos + i3, line_col);
			// triangles += 1;
		}
	}
	if (mode & draw_plot) {
		for (size_t i = 0; i < msh->vtxcnt; i += 1) {
			drawPlot3d(dst, pos + i);
		}
	}

	// draw segments (lines)
	for (size_t i = 0; i < msh->segcnt; i += 1) {
		int32_t i1 = msh->segptr[i].p1;
		int32_t i2 = msh->segptr[i].p2;
		drawLine3d(dst, pos + i1, pos + i2, line_col);
	}

	if (mode & draw_box) {
		struct projVtx vvtx = {0};
		struct vector tmp[1] = {0};
		for (size_t i = 0; i < msh->vtxcnt; i += 1) {
			GxMeshVtx v = msh->vtxptr + i;
			mapVertex(&vvtx, proj, vecadd(tmp, &v->p, vecsca(tmp, &v->n, 0.02)));
			drawLine3d(dst, &vvtx, pos + i, norm_col);
		}
		// todo: precalculate min and max vertex coords
		// this should be done in draw segments phase
		drawBbox(dst, msh, proj, bbox_col);
	}

	return triangles;
}
