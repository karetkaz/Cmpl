#include "g2_argb.h"
#include "g2_surf.h"
#include "math.h"

#include "image.inl"

void g2_drawline(gx_Surf surf, int x0, int y0, int x1, int y1, long col) {
	//~ TODO : replace Bresenham with DDA, resolve clipping 
	int sx, sy, r;
	int dx = x1 - x0;
	int dy = y1 - y0;

	if (dx < 0) {
		dx = -dx;
		sx = -1;
	}
	else {
		sx = 1;
	}

	if (dy < 0) {
		dy = -dy;
		sy = -1;
	}
	else {
		sy = 1;
	}

	if (dx > dy) {
		r = dx >> 1;
		while (x0 != x1) {
			gx_setpixel(surf, x0, y0, col);
			if ((r += dy) > dx) {
				y0 += sy;
				r -= dx;
			}
			x0 += sx;
		}
	}
	else {
		r = dy >> 1;
		while (y0 != y1) {
			gx_setpixel(surf, x0, y0, col);
			if ((r+=dx) > dy) {
				x0 += sx;
				r -= dy;
			}
			y0 += sy;
		}
	}
	gx_setpixel(surf, x0, y0, col);
}
void gx_drawrect(gx_Surf dst, int x0, int y0, int x1, int y1, long col) {
	//~ gx_setblock(dst, x0, y0, x1, y1, col);
	gx_fillrect(dst, x0, y0, x1, y0 + 1, col);
	gx_fillrect(dst, x0, y1, x1 + 1, y1 + 1, col);
	gx_fillrect(dst, x0, y0, x0 + 1, y1, col);
	gx_fillrect(dst, x1, y0, x1 + 1, y1, col);
}

void g2_drawoval(gx_Surf s, int x0, int y0, int x1, int y1, long c/* , long fg */) {
	int dx, dy, sx, sy, r;
	if (x0 > x1) {
		x0 ^= x1;
		x1 ^= x0;
		x0 ^= x1;
	}
	if (y0 > y1) {
		y0 ^= y1;
		y1 ^= y0;
		y0 ^= y1;
	}
	dx = x1 - x0;
	dy = y1 - y0;

	x1 = x0 += dx >> 1;
	x0 += dx & 1;

	dx += dx & 1;
	dy += dy & 1;

	sx = dx * dx;
	sy = dy * dy;
	r = sx * dy >> 2;

	dx = 0;
	dy = r << 1;

	while (y0 < y1) {
		gx_setpixel(s, x0, y0, c);
		gx_setpixel(s, x0, y1, c);
		gx_setpixel(s, x1, y0, c);
		gx_setpixel(s, x1, y1, c);

		if (r >= 0) {
			x0 -= 1;
			x1 += 1;
			r -= dx += sy;
		}

		if (r < 0) {
			y0 += 1;
			y1 -= 1;
			r += dy -= sx;
		}
	}
	gx_setpixel(s, x0, y0, c);
	gx_setpixel(s, x0, y1, c);
	gx_setpixel(s, x1, y0, c);
	gx_setpixel(s, x1, y1, c);
	//~ */
}
void g2_filloval(gx_Surf s, int x0, int y0, int x1, int y1, long c/* , long fg */) {
	int dx, dy, sx, sy, r;
	if (x0 > x1) {
		x0 ^= x1;
		x1 ^= x0;
		x0 ^= x1;
	}
	if (y0 > y1) {
		y0 ^= y1;
		y1 ^= y0;
		y0 ^= y1;
	}
	dx = x1 - x0;
	dy = y1 - y0;

	x1 = x0 += dx >> 1;
	x0 +=dx & 1;

	dx += dx & 1;
	dy += dy & 1;

	sx = dx * dx;
	sy = dy * dy;

	r = sx * dy >> 2;
	dx = 0;
	dy = r << 1;

	while (y0 < y1) {
		gx_fillrect(s, x0, y0, x0 + 1, y1, c);
		gx_fillrect(s, x1, y0, x1 + 1, y1, c);

		if (r >= 0) {
			x0 -= 1;
			x1 += 1;
			r -= dx += sy;
		}

		if (r < 0) {
			y0 += 1;
			y1 -= 1;
			r += dy -= sx;
		}
	}
	gx_fillrect(s, x0, y0, x0 + 1, y1, c);
	gx_fillrect(s, x1, y0, x1 + 1, y1, c);
}

void gx_drawbez2(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, long col) {
	double t, dt = 1. / 128;

	int px_0 = x0;
	int py_0 = y0;
	int px_1 = 2 * (x1 - x0);
	int py_1 = 2 * (y1 - y0);
	int px_2 = x2 - 2 * x1 + x0;
	int py_2 = y2 - 2 * y1 + y0;

	for (t = dt; t < 1; t += dt) {
		x1 = (px_2 * t + px_1) * t + px_0;
		y1 = (py_2 * t + py_1) * t + py_0;
		g2_drawline(dst, x0, y0, x1, y1, col);
		x0 = x1;
		y0 = y1;
	}
	g2_drawline(dst, x0, y0, x2, y2, col);
}
void gx_drawbez3(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col) {
	double t, dt = 1. / 128;

	int px_0 = x0;
	int py_0 = y0;
	int px_1 = 3 * (x1 - x0);
	int py_1 = 3 * (y1 - y0);
	int px_2 = 3 * (x2 - x1) - px_1;
	int py_2 = 3 * (y2 - y1) - py_1;
	int px_3 = x3 - px_2 - px_1 - px_0;
	int py_3 = y3 - py_2 - py_1 - py_0;

	for (t = dt; t < 1; t += dt) {
		x1 = ((px_3 * t + px_2) * t + px_1) * t + px_0;
		y1 = ((py_3 * t + py_2) * t + py_1) * t + py_0;
		g2_drawline(dst, x0, y0, x1, y1, col);
		x0 = x1;
		y0 = y1;
	}
	g2_drawline(dst, x0, y0, x3, y3, col);
}

//#{	Gradient fill
#define F64_MPI 3.14159265358979323846
#define F64_2PI (2 * F64_MPI)

static inline float fltmax(float a, float b) {return a > b ? a : b;}
static inline float fltClamp(float t, float a, float b) {return t < a ? a : t > b ? b : t;}

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
	return sqrt(fltmax(x*x*g->dx, y*y*g->dy));
}
static double gradient_con_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	double ang = g->a - atan2(x , y);
	if (ang < 0)
		ang += F64_2PI;
	return ang * (1. / F64_2PI);
}
static double gradient_spr_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	double ang = atan2(x , y) - g->a;
	double len = g->l * sqrt(x * x + y * y);

	if (ang < 0)
		ang += F64_2PI;

	ang *= 1. / F64_2PI;
	return len + ang;
}

int gx_gradsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut, int gradtype/* , cmix mix, int alphaANDflagsANDgradtype */) {
	gx_Clip clp = gx_getclip(dst);
	struct gradient_data g;
	argb *p = lut->data;
	char* dptr;

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

	if (!(dptr = (char*)gx_getpaddr(dst, clp->l, clp->t)))
		return -2;

	switch (gradtype & gradient_TYP) {
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
			g.a = atan2(g.x, g.y) + F64_MPI;
			g.gf = gradient_con_factor;
			gradtype |= gradient_rep;
			break;

		case gradient_spr:
			g.a = atan2(g.x, g.y) + F64_MPI;
			g.l = 1. / sqrt(g.x*g.x + g.y*g.y);
			g.gf = gradient_spr_factor;
			gradtype |= gradient_rep;
			break;

	}

	if (gradtype & gradient_rep) {
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

				double len = fltClamp(g.gf(&g), 0., 1.);
				len = fmod(len, 1 + 1. / 0x7fffffff);
				*d++ = p[(int)(len * 0xff) & 0xff];
			}
			dptr += dst->scanLen;
		}
	}
	return 0;

}

//#}	Gradient fill */
