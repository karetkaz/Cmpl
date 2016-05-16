#include "gx_color.h"
#include "gx_surf.h"

#include <math.h>
#include <stdlib.h>

static inline int min_i32(int a, int b) {return a < b ? a : b;}
static inline int max_i32(int a, int b) {return a > b ? a : b;}
static inline float max_f32(float a, float b) { return a > b ? a : b; }
static inline float clamp_f32(float t, float a, float b) { return t < a ? a : t > b ? b : t; }

int gx_blursurf(gx_Surf img, gx_Rect roi, int radius) {

	int w = img->width;
	int h = img->height;
	int wm = w - 1;
	int hm = h - 1;

	int wh = w * h;
	int maxwh = max_i32(w, h);
	int div = 2 * radius + 1;

	int x, y, i, yi = 0;
	int *r, *g, *b, *vmin, *vmax, *dv;

	if (radius < 1)
		return -1;

	if (roi != NULL)
		return -2;

	r = malloc(sizeof(int) * wh);
	g = malloc(sizeof(int) * wh);
	b = malloc(sizeof(int) * wh);
	vmin = malloc(sizeof(int) * maxwh);
	vmax = malloc(sizeof(int) * maxwh);
	dv = malloc(sizeof(int) * 256 * div);


	for (i = 0; i < 256 * div; i += 1) {
		dv[i] = i / div;
	}

	for (y = 0; y < h; y += 1) {
		int rsum = 0;
		int gsum = 0;
		int bsum = 0;

		for (i = -radius; i <= radius; i += 1) {
			int p = gx_getpixel(img, min_i32(wm, max_i32(i, 0)), y);
			rsum += rch(p);
			gsum += gch(p);
			bsum += bch(p);
		}

		for (x = 0; x < w; x += 1) {
			int p1, p2;
			r[yi] = dv[rsum];
			g[yi] = dv[gsum];
			b[yi] = dv[bsum];

			if (y == 0) {
				vmin[x] = min_i32(x + radius + 1, wm);
				vmax[x] = max_i32(x - radius, 0);
			}

			p1 = gx_getpixel(img, vmin[x], y);
			p2 = gx_getpixel(img, vmax[x], y);

			rsum += rch(p1) - rch(p2);
			gsum += gch(p1) - gch(p2);
			bsum += bch(p1) - bch(p2);

			yi += 1;
		}
	}

	for (x = 0; x < w; x += 1) {
		int rsum = 0;
		int gsum = 0;
		int bsum = 0;
		int yp = -radius * w;

		for (i = -radius; i <= radius; i += 1) {
			int xy = max_i32(0, yp) + x;
			rsum += r[xy];
			gsum += g[xy];
			bsum += b[xy];
			yp += w;
		}

		for (y = 0; y < h; y += 1) {
			int p1, p2;
			gx_setpixel(img, x, y, __rgb(dv[rsum], dv[gsum], dv[bsum]));

			if (x == 0) {
				vmin[y] = min_i32(y + radius + 1, hm) * w;
				vmax[y] = max_i32(y - radius, 0) * w;
			}

			p1 = x + vmin[y];
			p2 = x + vmax[y];

			rsum += r[p1] - r[p2];
			gsum += g[p1] - g[p2];
			bsum += b[p1] - b[p2];
		}
	}

	free(r);
	free(g);
	free(b);
	free(vmin);
	free(vmax);
	free(dv);
	return 0;
}

int gx_clutsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut) {
	argb *lutd = (argb*)lut->data;
	struct gx_Rect rect;
	char *dptr;

	if (dst->depth != 32)
		return -1;

	rect.x = roi ? roi->x : 0;
	rect.y = roi ? roi->y : 0;
	rect.w = roi ? roi->w : dst->width;
	rect.h = roi ? roi->h : dst->height;

	if(!(dptr = gx_cliprect(dst, &rect)))
		return -2;

	while (rect.h--) {
		int x;
		argb *cBuff = (argb*)dptr;
		for (x = 0; x < rect.w; x += 1) {
			//~ cBuff->a = lutd[cBuff->a].a;
			cBuff->r = lutd[cBuff->r].r;
			cBuff->g = lutd[cBuff->g].g;
			cBuff->b = lutd[cBuff->b].b;
			cBuff += 1;
		}
		dptr += dst->scanLen;
	}

	return 0;
}

//#{	Gradient fill

struct gradient_data {
	int	sx, sy, x, y;
	double	dx, dy, l, a;
	double (*gf)(struct gradient_data*);
};

static double gradient_lin_factor(struct gradient_data* g) {
	int x = g->x - g->sx;
	int y = g->y - g->sy;
	return g->l * (g->dx * x + g->dy * y);
}
static double gradient_rad_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	return sqrt(x*x*g->dx + y*y*g->dy);
}
static double gradient_sqr_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	return sqrt(max_f32(x*x*g->dx, y*y*g->dy));
}
static double gradient_con_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	double ang = g->a - atan2(x , y);
	if (ang < 0) {
		ang += 2 * M_PI;
	}
	return ang * (1. / (2 * M_PI));
}
static double gradient_spr_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	double ang = atan2(x , y) - g->a;
	double len = g->l * sqrt(x * x + y * y);

	if (ang < 0) {
		ang += 2 * M_PI;
	}
	ang *= 1. / (2 * M_PI);
	return len + ang;
}

int gx_gradsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut, gradient_type gradtype, int repeat) {
	gx_Clip clp = gx_getclip(dst);
	struct gradient_data g;
	argb *p = (argb*)lut->data;
	char *dptr;

	if (dst->depth != 32)
		return -1;

	if (roi) {
		g.sx = roi->x;
		g.sy = roi->y;
		g.x = roi->w ? roi->w : 1;
		g.y = roi->h ? roi->h : 1;
	}
	else {
		g.sx = g.sy = 0;
		g.x = dst->width;
		g.y = dst->height;
	}

	switch (gradtype) {
		default:
			return -3;

		case gradient_lin:
			g.dx = g.x;
			g.dy = g.y;
			g.l = 1. / (g.x * g.x + g.y * g.y);
			g.gf = gradient_lin_factor;
			break;

		case gradient_rad:
			g.dx = 1. / (g.x * g.x);
			g.dy = 1. / (g.y * g.y);
			g.gf = gradient_rad_factor;
			break;

		case gradient_sqr:
			g.dx = 1. / (g.x * g.x);
			g.dy = 1. / (g.y * g.y);
			g.gf = gradient_sqr_factor;
			break;

		case gradient_con:
			g.a = atan2(g.x, g.y) + M_PI;
			g.gf = gradient_con_factor;
			repeat = 1;
			break;

		case gradient_spr:
			g.a = atan2(g.x, g.y) + M_PI;
			g.l = 1. / sqrt(g.x*g.x + g.y*g.y);
			g.gf = gradient_spr_factor;
			repeat = 1;
			break;

	}

	if (!(dptr = (char*)gx_getpaddr(dst, clp->l, clp->t))) {
		return -2;
	}

	if (repeat) {
		for(g.y = clp->t; g.y < clp->b; ++g.y) {
			register argb* d = (argb*)dptr;
			for(g.x = clp->l; g.x < clp->r; ++g.x) {

				double len = g.gf(&g);
				len = fmod(len, 1 + 1. / 0x7fffffff);
				*d++ = p[(int)(len * 0xff) & 0xff];
			}
			dptr += dst->scanLen;
		}
	}
	else {
		for(g.y = clp->t; g.y < clp->b; ++g.y) {
			register argb* d = (argb*)dptr;
			for(g.x = clp->l; g.x < clp->r; ++g.x) {

				double len = clamp_f32(g.gf(&g), 0., 1.);
				len = fmod(len, 1 + 1. / 0x7fffffff);
				*d++ = p[(int)(len * 0xff) & 0xff];
			}
			dptr += dst->scanLen;
		}
	}
	return 0;
}

//#}	Gradient fill */
