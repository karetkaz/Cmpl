#include "g2_argb.h"
#include "g2_surf.h"
#include "image.c"
#include "math.h"

#define __TODO__

void gx_drawrect(gx_Surf dst, int x0, int y0, int x1, int y1, long col) {
	//~ gx_setblock(dst, x0, y0, x1, y1, col);
	gx_setblock(dst, x0, y0, x1, y0 + 1, col);
	gx_setblock(dst, x0, y1, x1, y1 + 1, col);
	gx_setblock(dst, x0, y0, x0 + 1, y1, col);
	gx_setblock(dst, x1, y0, x1 + 1, y1, col);
}

void g2_drawoval(gx_Surf s, int x0, int y0, int x1, int y1, long c/* , long fg */) {
	int dx, dy, sx, sy, r;
	if (x0 > x1) {x0 ^= x1; x1 ^= x0; x0 ^= x1;}
	if (y0 > y1) {y0 ^= y1; y1 ^= y0; y0 ^= y1;}
	dx = x1 - x0; dy = y1 - y0;
	x1 = x0 += dx >> 1; x0 +=dx & 1;
	dx += dx & 1; dy += dy & 1;
	sx = dx * dx; sy = dy * dy;
	r = sx * dy >> 2;
	dx = 0; dy = r << 1;
	while (y0 < y1) {
		gx_setpixel(s, x0, y0, c); gx_setpixel(s, x0, y1, c);
		gx_setpixel(s, x1, y0, c); gx_setpixel(s, x1, y1, c);
		if (r >= 0) {
			x0--; x1++;
			r -= dx += sy;
		}
		if (r < 0) {
			y0++; y1--;
			r += dy -= sx;
		}
	}
	gx_setpixel(s, x0, y0, c); gx_setpixel(s, x0, y1, c);
	gx_setpixel(s, x1, y0, c); gx_setpixel(s, x1, y1, c);
	//~ */
}

static void g2_drawlineA(gx_Surf surf, int x0, int y0, int x1, int y1, long col) {
	//~ TODO : replace Bresenham with DDA, resolve clipping 
	int dx, dy, sx, sy, r;
	dx = x1 - x0;
	dy = y1 - y0;
	if (dx < 0) { dx = -dx;  sx = -1; } else sx = 1;
	if (dy < 0) { dy = -dy;  sy = -1; } else sy = 1;
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
	} else {
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

static void g2_drawlineB(gx_Surf surf, int x0, int y0, int x1, int y1, long col) {
	//~ TODO : replace Bresenham with DDA, resolve clipping 
	int dx, dy, sx, sy, r;
	dx = x1 - x0;
	dy = y1 - y0;
	if (dx < 0) { dx = -dx;  sx = -1; } else sx = 1;
	if (dy < 0) { dy = -dy;  sy = -1; } else sy = 1;
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
	} else {
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

void g2_drawline(gx_Surf surf, int x0, int y0, int x1, int y1, long col) {
	g2_drawlineA(surf, x0, y0, x1, y1, col);
}

void gx_drawbez2(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, long col/* , int aa = 0 */) {
	int px_2, px_1, px_0 = x0;
	int py_2, py_1, py_0 = y0;
	double t, dt = 1./128;
	px_1 = 2 * (x1 - x0);
	py_1 = 2 * (y1 - y0);
	px_2 = x2 - 2*x1 + x0;
	py_2 = y2 - 2*y1 + y0;
	for (t = dt; t < 1; t += dt) {
		x1 = (((px_2)*t + px_1)*t + px_0);
		y1 = (((py_2)*t + py_1)*t + py_0);
		g2_drawline(dst, x0, y0, x1, y1, col);
		x0 = x1; y0 = y1;
	}
	g2_drawline(dst, x0, y0, x2, y2, col);
}

/*void gx_drawbez3(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col) {
	int px_3, px_2, px_1, px_0 = x0;
	int py_3, py_2, py_1, py_0 = y0;
	double t, dt = 1./512;
	px_1 = 3 * (x1 - x0);
	py_1 = 3 * (y1 - y0);
	px_2 = 3 * (x2 - x1) - px_1;
	py_2 = 3 * (y2 - y1) - py_1;
	px_3 = x3 - px_2 - px_1 - px_0;
	py_3 = y3 - py_2 - py_1 - py_0;
	for (t = dt; t < 1; t += dt) {
		x1 = ((((px_3)*t + px_2)*t + px_1)*t + px_0);
		y1 = ((((py_3)*t + py_2)*t + py_1)*t + py_0);
		g2_drawline(dst, x0, y0, x1, y1, col);
		x0 = x1; y0 = y1;
	}
	g2_drawline(dst, x0, y0, x3, y3, col);
}*/

/*void g2_drawmode(int aa) {
	//~ g2_drawline = aa ? g2_drawlineA : g2_drawlineA;
	//~ g2_drawoval = aa ? g2_drawovalA : g2_drawovalB;
}*/

//{	Color operations
argb rgbclip(argb dst, int min, int max) {	// clamp values between min & max
	if (dst.a < min) dst.a = min; else if (dst.a > max) dst.a = max;
	if (dst.r < min) dst.r = min; else if (dst.r > max) dst.r = max;
	if (dst.g < min) dst.g = min; else if (dst.g > max) dst.g = max;
	if (dst.b < min) dst.b = min; else if (dst.b > max) dst.b = max;
	return dst;
}
argb rgblerp(argb dst, argb src, int t) {	// Linear interpolate
	if (t >= 256) return src;
	else if (t <= 0) return dst;
	dst.r += (t * (src.r - dst.r)) >> 8;
	dst.g += (t * (src.g - dst.g)) >> 8;
	dst.b += (t * (src.b - dst.b)) >> 8;
	return dst;
}
argb rgblerpw(argb dst, argb src, int t) {	// Linear interpolate w
	if (t >= 65536) return src;
	else if (t <= 0) return dst;
	dst.r += (t * (src.r - dst.r)) >> 16;
	dst.g += (t * (src.g - dst.g)) >> 16;
	dst.b += (t * (src.b - dst.b)) >> 16;
	return dst;
}
argb rgblerpd(argb dst, argb src, double t) {	// Linear interpolate
	if (t >= 1.) return src;
	else if (t <= 0.) return dst;
	dst.r += (unsigned char)(t * (src.r - dst.r) + .5);
	dst.g += (unsigned char)(t * (src.g - dst.g) + .5);
	dst.b += (unsigned char)(t * (src.b - dst.b) + .5);
	return dst;
}

//}
//{	Color mixing , Alpha mixing, mask channel, Color lookup
void rgbnop(argb *dst, argb *src, unsigned cnt) {		// none
}
void rgbcpy(argb *dst, argb *src, unsigned cnt) {		// copy
	while (cnt--)
		*dst++ = *src++;
}
void rgbadd(argb *dst, argb *src, unsigned cnt) {		// Add
	//~ dst = min(dst + src, 1);
	register unsigned short tmp;
	while (cnt) {
		tmp = dst->r + src->r; dst->r = tmp > 255 ? 255 : tmp;
		tmp = dst->g + src->g; dst->g = tmp > 255 ? 255 : tmp;
		tmp = dst->b + src->b; dst->b = tmp > 255 ? 255 : tmp;
		//~ tmp = dst->a + src->a; dst->a = tmp > 255 ? 255 : tmp;
		dst++; src++; cnt--;
	}
}
void rgbsub(argb *dst, argb *src, unsigned cnt) {		// Subtract
	//~ dst = max(dst - src, 0);
	register short tmp;
	while (cnt) {
		tmp = dst->r - src->r; dst->r = tmp < 0 ? 0 : tmp;
		tmp = dst->g - src->g; dst->g = tmp < 0 ? 0 : tmp;
		tmp = dst->b - src->b; dst->b = tmp < 0 ? 0 : tmp;
		//~ tmp = dst->a - src->a; dst->a = tmp < 0 ? 0 : tmp;
		dst++; src++; cnt--;
	}
}
void rgbdif(argb *dst, argb *src, unsigned cnt) {		// Difference
	//~ dst = abs(dst - src);
	register short tmp;
	while (cnt) {
		tmp = dst->r - src->r; dst->r = tmp < 0 ? -tmp : tmp;
		tmp = dst->g - src->g; dst->g = tmp < 0 ? -tmp : tmp;
		tmp = dst->b - src->b; dst->b = tmp < 0 ? -tmp : tmp;
		//~ tmp = dst->a - src->a; dst->a = tmp < 0 ? -tmp : tmp;
		dst++; src++; cnt--;
	}
}
void rgbmul(argb *dst, argb *src, unsigned cnt) {		// Multiply
	//~ dst = dst * src;
	register unsigned short tmp;
	while (cnt) {
		tmp = dst->r * (src->r + 1); dst->r = tmp >> 8;
		tmp = dst->g * (src->g + 1); dst->g = tmp >> 8;
		tmp = dst->b * (src->b + 1); dst->b = tmp >> 8;
		//~ tmp = dst->a * (src->a + 1); dst->a = tmp >> 8;
		dst++; src++; cnt--;
	}
}
void rgbdiv(argb *dst, argb *src, unsigned cnt) {		// Divide
	//~ dst = dst / src;
	register unsigned short tmp;
	while (cnt) {
		tmp = (dst->r << 8) / (1 + src->r); dst->r = tmp > 255 ? 255 : tmp;
		tmp = (dst->g << 8) / (1 + src->g); dst->g = tmp > 255 ? 255 : tmp;
		tmp = (dst->b << 8) / (1 + src->b); dst->b = tmp > 255 ? 255 : tmp;
		//~ tmp = (dst->a << 8) / (1 + src->a); dst->a = tmp > 255 ? 255 : tmp;
		dst++; src++; cnt--;
	}
}
void rgbmin(argb *dst, argb *src, unsigned cnt) {		// Darken
	//~ dst = min(dst, src);
	while (cnt) {
		if (dst->r > src->r) dst->r = src->r;
		if (dst->g > src->g) dst->g = src->g;
		if (dst->b > src->b) dst->b = src->b;
		//~ if (dst->a > src->a) dst->a = src->a;
		dst++; src++; cnt--;
	}
}
void rgbmax(argb *dst, argb *src, unsigned cnt) {		// Lighten
	//~ dst = max(dst, src);
	while (cnt) {
		if (dst->r < src->r) dst->r = src->r;
		if (dst->g < src->g) dst->g = src->g;
		if (dst->b < src->b) dst->b = src->b;
		//~ if (dst->a < src->a) dst->a = src->a;
		dst++; src++; cnt--;
	}
}
void rgbscr(argb *dst, argb *src, unsigned cnt) {		// Screen
	//~ dst = 1. - (1. - dst) * (1. - src)[ = dst + src – (dst * src)];
	register unsigned short tmp;
	while (cnt) {
		tmp = ((255 - dst->r) * (255 - src->r + 1)); dst->r = 255 - (tmp >> 8);
		tmp = ((255 - dst->g) * (255 - src->g + 1)); dst->g = 255 - (tmp >> 8);
		tmp = ((255 - dst->b) * (255 - src->b + 1)); dst->b = 255 - (tmp >> 8);
		tmp = ((255 - dst->a) * (255 - src->a + 1)); dst->a = 255 - (tmp >> 8);
		dst++; src++; cnt--;
	}
}
/* and the others
void rgbnot(argb *dst, argb *src, unsigned cnt) {		// Logical NOT
	//~ dst = NOT src;
	dst.col = ~src.col;
	return dst;
}
void rgband(argb *dst, argb *src, unsigned cnt) {		// Logical AND
	//~ dst = dst AND src;
	dst.col &= src.col;
	return dst;
}
void rgbior(argb *dst, argb *src, unsigned cnt) {		// Logical OR
	//~ dst = dst OR src;
	dst.col |= src.col;
	return dst;
}
void rgbxor(argb *dst, argb *src, unsigned cnt) {		// Logical XOR
	//~ dst = dst XOR src;
	dst.col ^= src.col;
	return dst;
}
void rgbovr(argb *dst, argb *src, unsigned cnt) {		// Overlay
	//~ dst = (dst < .5) ? (2 * dst * src) : (1 - 2 * (1 - dst) * (1 - src));
	dst.r = dst.r <= 127 ? \
		(dst.r * (1 + src.r)) >> 7 : \
		255 - (((255 - dst.r) * (256 - src.r)) >> 7);
	dst.g = dst.g <= 127 ? \
		(dst.g * (1 + src.g)) >> 7 : \
		255 - (((255 - dst.g) * (256 - src.g)) >> 7);
	dst.b = dst.b <= 127 ? \
		(dst.b * (1 + src.b)) >> 7 : \
		255 - (((255 - dst.b) * (256 - src.b)) >> 7);
	dst.a = dst.a <= 127 ? \
		(dst.a * (1 + src.a)) >> 7 : \
		255 - (((255 - dst.a) * (256 - src.a)) >> 7);
	return dst;
}

argb rgbrch(argb dst, argb src) {		// Red Channel
	dst.r = src.r;
	return dst;
}

argb rgbgch(argb dst, argb src) {		// Green Channel
	dst.g = src.g;
	return dst;
}

argb rgbbch(argb dst, argb src) {		// Blue Channel
	dst.b = src.b;
	return dst;
}//*/

//##############################################################################
#define amix_mul(__tmp, __lhs, __rhs, __shv) (((__tmp = (__lhs) * (__rhs)) + (__tmp >> (__shv)) + 1) >> (__shv))

static argb amix_nop(argb dst, argb src, unsigned alpha) {	// blend with alpha = 0
	return dst;
}
static argb amix_one(argb dst, argb src, unsigned alpha) {	// blend with alpha = 1
	return src;
}
static argb amix_val(argb dst, argb src, unsigned alpha) {	// blend with alpha (linear interpolate)
	register long tmp;
	dst.r += amix_mul(tmp, alpha, src.r - dst.r, 8);
	dst.g += amix_mul(tmp, alpha, src.g - dst.g, 8);
	dst.b += amix_mul(tmp, alpha, src.b - dst.b, 8);
	return dst;
}
static argb amix_src(argb dst, argb src, unsigned alpha) {	// blend with alpha = src.alpha
	register long tmp;
	alpha = src.a;
	dst.r += amix_mul(tmp, alpha, src.r - dst.r, 8);
	dst.g += amix_mul(tmp, alpha, src.g - dst.g, 8);
	dst.b += amix_mul(tmp, alpha, src.b - dst.b, 8);
	return dst;
}
static argb amix_dst(argb dst, argb src, unsigned alpha) {	// blend with alpha = dst.alpha (for clipping)
	register long tmp;
	alpha = dst.a;
	dst.r += amix_mul(tmp, alpha, src.r - dst.r, 8);
	dst.g += amix_mul(tmp, alpha, src.g - dst.g, 8);
	dst.b += amix_mul(tmp, alpha, src.b - dst.b, 8);
	return dst;
}
static argb amix_mix(argb dst, argb src, unsigned alpha) {	//X blend with alpha = alpha * src.alpha
	register long tmp;
	alpha *= src.a;
	dst.r += amix_mul(tmp, alpha, src.r - dst.r, 16);
	dst.g += amix_mul(tmp, alpha, src.g - dst.g, 16);
	dst.b += amix_mul(tmp, alpha, src.b - dst.b, 16);
	return dst;
}
#undef amix_mul
//##############################################################################
/*static argb clut_mch(clut lut, argb src) {			// lookup master channel
	register argb* lutd = lut->data;
	src.r = lutd[src.r].a;
	src.g = lutd[src.g].a;
	src.b = lutd[src.b].a;
	return src;
}*/

/*static argb clut_rgb(clut lut, argb src) {			// lookup color channels
	register argb* lutd = lut->data;
	src.r = lutd[src.r].r;
	src.g = lutd[src.g].g;
	src.b = lutd[src.b].b;
	return src;
}*/

//##############################################################################

//}
//{	Color lookup
void gx_clutclr(clut lut, int argb_mask) {
	int idx;
	for (idx = 0; idx < 256; ++idx) {
		if (argb_mask & argb_mchn) lut->data[idx].a = idx;
		if (argb_mask & argb_rchn) lut->data[idx].r = idx;
		if (argb_mask & argb_gchn) lut->data[idx].g = idx;
		if (argb_mask & argb_bchn) lut->data[idx].b = idx;
	}
}
void gx_clutinv(clut lut, int argb_mask) {
	int idx;
	for (idx = 0; idx < 256; ++idx) {
		if (argb_mask & argb_mchn) lut->data[idx].a = 255 - lut->data[idx].a;
		if (argb_mask & argb_rchn) lut->data[idx].r = 255 - lut->data[idx].r;
		if (argb_mask & argb_gchn) lut->data[idx].g = 255 - lut->data[idx].g;
		if (argb_mask & argb_bchn) lut->data[idx].b = 255 - lut->data[idx].b;
	}
}
void gx_clutBCG(clut lut, int argb_mask, int brg, int cntr, float gamma) {
	register int idx, val;
	double gval = 1 / gamma;
	double bval = brg / 255.;
	double cval = cntr > 0 ? (255. / (255 - cntr*2)) : ((255 + cntr*2) / 255.);
	for (idx = 0; idx < 256; ++idx) {
		gamma = pow(((bval + idx / 255. - .5) * cval) + .5, gval);
		val = gamma < 0 ? 0 : gamma > 1 ? 255 : (int)(255*gamma);
		if (argb_mask & argb_mchn) lut->data[idx].a = val;
		if (argb_mask & argb_rchn) lut->data[idx].r = val;
		if (argb_mask & argb_gchn) lut->data[idx].g = val;
		if (argb_mask & argb_bchn) lut->data[idx].b = val;
	}
}
/*void gx_cluthisto(clut lut, gx_Surf dst, int argb_mask) {
	argb* ptr;
	double mmc;
	struct gx_Clip clip;
	unsigned i, max = 0, mch[256], rch[256], gch[256], bch[256];
	char* dptr = (char*)gx_makeclip(dst, &clip, 0, 0, dst->width, dst->height);
	if (dst->depth != 32 || dptr == 0) return;
	for (i = 0; i < 256; i++)
		mch[i] = rch[i] = gch[i] = bch[i] = 0;
	while (clip.ymin < clip.ymax) {
		ptr = (argb*)dptr; dptr += dst->scanLen;
		for(i = clip.xmin; i < clip.xmax; ++i, ++ptr) {
			int val = ptr->r;
			if (val < ptr->g) val = ptr->g;
			if (val < ptr->b) val = ptr->b;
			if (++mch[val] > max) max = mch[val];
			++rch[ptr->r];
			++gch[ptr->g];
			++bch[ptr->b];
		}
		clip.ymin++;
	}
	ptr = lut->data;
	mmc = 255. / max;
	for (i = 0; i < 256; i++) {
		ptr[i].a = (double)mch[i] * mmc;
		ptr[i].r = (double)rch[i] * mmc;
		ptr[i].g = (double)gch[i] * mmc;
		ptr[i].b = (double)bch[i] * mmc;
	}
}*/

//}

//{	Gradient fill
#define FLT_MPI 3.14159265358979323846
#define FLT_2PI (2 * FLT_MPI)

//~ float fltabs(float a) {return a < 0 ? -a : a;}
void gx_clutblend(clut lut, int argb_mask, argb c1, argb c2) {
	int idx;
	argb c;
	for (idx = 0; idx < 256; ++idx) {
		//~ c = rgblerp(c1, c2, idx);
		c = rgblerpd(c1, c2, idx/255.);
		if (argb_mask & argb_mchn) lut->data[idx].a = c.a;
		if (argb_mask & argb_rchn) lut->data[idx].r = c.r;
		if (argb_mask & argb_gchn) lut->data[idx].g = c.g;
		if (argb_mask & argb_bchn) lut->data[idx].b = c.b;
	}
}
static float fltmax(float a, float b) {
	return a > b ? a : b;
}

typedef struct gradient_data {
	int	sx, sy, x, y;
	double	dx, dy, l, a;
	double (*gf)(struct gradient_data*);
}gradient_data;

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
	if (ang < 0) ang += FLT_2PI;
	return ang * (1/FLT_2PI);
}
static double gradient_spr_factor(struct gradient_data* g) {
	int x = g->sx - g->x;
	int y = g->sy - g->y;
	double ang = atan2(x , y) - g->a;
	double len = g->l * sqrt(x * x + y * y);
	if (ang < 0) ang += FLT_2PI;
	ang *= 1 / FLT_2PI;
	return len + ang;
}

int gx_gradsurf(gx_Surf dst, gx_Rect rec, clut lut, int gradtype/* , cmix mix, int alphaANDflagsANDgradtype */) {
	gx_Clip clp = gx_getclip(dst);
	struct gradient_data g;
	char* dptr;
	argb *p = lut->data;

	if (dst->depth != 32)
		return -1;

	if (rec) {
		g.sx = rec->x;
		g.sy = rec->y;
		g.x = rec->w == 0 ? 1 : rec->w;
		g.y = rec->h == 0 ? 1 : rec->h;
	} else {
		g.sx = g.sy = 0;
		g.x = dst->width;
		g.y = dst->height;
	}

	if (!(dptr = (char*)gx_getpaddr(dst, clp->l, clp->t)))
		return -2;

	switch (gradtype & gradient_TYP) {
		default : return -3;
		case gradient_lin : {
			g.dx = g.x;
			g.dy = g.y;
			g.l = 1./(g.x*g.x + g.y*g.y);
			g.gf = gradient_lin_factor;
		} break;
		case gradient_rad : {
			g.dx = 1. / (g.x * g.x);
			g.dy = 1. / (g.y * g.y);
			g.gf = gradient_rad_factor;
		} break;
		case gradient_sqr : {
			g.dx = 1. / (g.x * g.x);
			g.dy = 1. / (g.y * g.y);
			g.gf = gradient_sqr_factor;
		} break;
		case gradient_con : {
			g.a = atan2(g.x, g.y) + FLT_MPI;
			g.gf = gradient_con_factor;
			gradtype |= gradient_rep;
		} break;
		case gradient_spr : {
			g.a = atan2(g.x, g.y) + FLT_MPI;
			g.l = 1./sqrt(g.x*g.x + g.y*g.y);
			g.gf = gradient_spr_factor;
			gradtype |= gradient_rep;
		} break;
	}

	//~ initblend(&blend, mix, alpha, 0, flags);

	//~ debug("grad.fn(%d, %d, %d, %d) : %x", clp->xmin, clp->ymin, clp->xmax, clp->ymax, g.gf);
	for(g.y = clp->t; g.y < clp->b; ++g.y) {
		//~ argb tmp[65535];
		register argb* d = (argb*)dptr;
		for(g.x = clp->l; g.x < clp->r; ++g.x) {
			//~ register argb	col, tmp = *d;
			register long	idx, rem;
			double len = g.gf(&g);
			if (!(gradtype & gradient_rep)) {
				if (len >= 1) len = 1;
				else if (len < 0) len = 0;
			}
			len = fmod(len, 1 + 1./ 0xfe0000);
			if (gradtype & gradient_mir) {
				if (len < 0) len = -len;
				if (len >= .5) len = 1 - len;
				len *= 2;
			}
			idx = (int)(0xfe0000 * len);			// len >= 0 && len < 1
			rem = idx & 0xffff;
			idx = (idx >> 16) & 0xff;
			*d++ = rgblerpw(p[idx], p[idx+1], rem);	// lookup table
			//~ if (blend.cmix) col = blend.cmix(tmp, col);
			//~ if (blend.amix) col = blend.amix(tmp, col, blend.alpha);
			//~ *d++ = mask_chn(tmp, col, blend.cmask);
			//~ *d++ = mask_chn(tmp, amix(tmp, cmix(tmp, col)), flags);
		}
		//~ mixpixels(dptr, tmp, clp->r - clp->l);
		dptr += dst->scanLen;
	}
	gx_drawrect(dst, rec->x - rec->w, rec->y - rec->h, rec->x + rec->w, rec->y + rec->h, 0x00ff00ff);
	return 0;

}

//}	Gradient fill */

#ifndef __TODO__
static argb mask_chn(argb dst, argb src, unsigned cmask);	// mask channels
#if defined __WATCOMC__
#pragma aux mask_chn parm [eax] [edx] [ecx] modify [edx ecx] value [eax] = \
	"and	edx, ecx"\
	"not	ecx"\
	"and	eax, ecx"\
	"or	eax, edx";
#else
static argb mask_chn(argb dst, argb src, unsigned cmask) {	// mask channels
	register argb tmp;
	tmp.col = ((dst.col & ~cmask) | (src.col & cmask));
	return tmp;
}
#endif

//{	Color Blender
typedef struct blender_t {		// blender_t
	unsigned	cmask;
	unsigned	alpha;
	void*		cdata;
	argb		(*cmix)(argb, argb);				// color_mix
	argb		(*amix)(argb, argb, unsigned);			// alpha_mix
} *blender;

//~ extern argb fxmrgb(void* mat, argb src);
//~ typedef long* fxmptr;
static void cmix_cpy08(char* dst, char* src, unsigned cnt, blender cblend) {
	while (cnt > 4) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		if (cblend->cmix) sc4 = cblend->cmix(dc4, sc4);
		if (cblend->amix) sc4 = cblend->amix(dc4, sc4, cblend->alpha);
		*(argb*)dst = sc4;
		dst += 4;
		src += 4;
		cnt -= 4;
	}
	if (cnt) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		cnt = -1U << (cnt*8);
		if (cblend->cmix) sc4 = cblend->cmix(dc4, sc4);
		if (cblend->amix) sc4 = cblend->amix(dc4, sc4, cblend->alpha);
		*(argb*)dst = mask_chn(dc4, sc4, cnt);
	}
}

static void cmix_cpy24(char* dst, char* src, unsigned cnt, blender cblend) {
	unsigned cmask = cblend->cmask & 0x00ffffff;
	while (cnt) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		if (cblend->cmix) sc4 = cblend->cmix(dc4, sc4);
		if (cblend->amix) sc4 = cblend->amix(dc4, sc4, cblend->alpha);
		*(argb*)dst = mask_chn(dc4, sc4, cmask);
		dst += 3;
		src += 3;
		cnt -= 1;
	}
}

static void cmix_cpy32(char* dst, char* src, unsigned cnt, blender cblend) {
	//~ unsigned cmask = cblend->cmask;
	while (cnt) {
		argb dc4 = *(argb*)dst;
		argb sc4 = *(argb*)src;
		if (cblend->cmix) sc4 = cblend->cmix(dc4, sc4);
		if (cblend->amix) sc4 = cblend->amix(dc4, sc4, cblend->alpha);
		*(argb*)dst = mask_chn(dc4, sc4, cblend->cmask);
		dst += 4;
		src += 4;
		cnt -= 1;
	}
}

static void clut_cpy08(char* dst, char* src, unsigned cnt, blender cblend) {
	register argb* lutd = ((clut)(cblend->cdata))->data;
	while (cnt) {
		*dst = lutd[(unsigned)*src].a;
		dst += 1;
		src += 1;
		cnt -= 1;
	}
}

static void clut_cpy24(char* dst, char* src, unsigned cnt, blender cblend) {
	register argb* lutd = ((clut)(cblend->cdata))->data;
	unsigned cmask = cblend->cmask & 0x00ffffff;
	while (cnt) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		sc4.r = lutd[sc4.r].r;
		sc4.g = lutd[sc4.g].g;
		sc4.b = lutd[sc4.b].b;
		*(argb*)dst = mask_chn(dc4, sc4, cmask);
		dst += 3;
		src += 3;
		cnt -= 1;
	}
}

static void clut_cpy32(char* dst, char* src, unsigned cnt, blender cblend) {
	register argb* lutd = ((clut)(cblend->cdata))->data;
	unsigned cmask = cblend->cmask & 0x00ffffff;
	while (cnt) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		sc4.r = lutd[sc4.r].r;
		sc4.g = lutd[sc4.g].g;
		sc4.b = lutd[sc4.b].b;
		*(argb*)dst = mask_chn(dc4, sc4, cmask);
		dst += 4;
		src += 4;
		cnt -= 1;
	}
}

/*
static void cmat_cpy24(char* dst, char* src, unsigned cnt, blender cblend) {
	register fxpmat* matr = (fxpmat*)(cblend->cdata);
	unsigned cmask = cblend->cmask & 0x00ffffff;
	while (cnt) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		sc4.col = fxmrgb(matr, sc4.col);
		*(argb*)dst = mask_chn(dc4, sc4, cmask);
		dst += 3;
		src += 3;
		cnt -= 1;
	}
}

static void cmat_cpy32(char* dst, char* src, unsigned cnt, blender cblend) {
	register fxpmat* matr = (fxpmat*)(cblend->cdata);
	unsigned cmask = cblend->cmask & 0x00ffffff;
	while (cnt) {
		register argb dc4 = *(argb*)dst;
		register argb sc4 = *(argb*)src;
		sc4.col = fxmrgb(matr, sc4.col);
		*(argb*)dst = mask_chn(dc4, sc4, cmask);
		dst += 4;
		src += 4;
		cnt -= 1;
	}
}*/

//}

static void initblend(blender cblend, cmix mix, unsigned alpha, void* cdata, int flags) {
	cblend->cdata = cdata;
	if (alpha >= 256) {
		if (flags & argb_srca) {
			cblend->amix = amix_src;
		}
		else
			cblend->amix = 0;
	}
	else {
		if (flags & argb_srca) {
			cblend->amix = amix_mix;
		}
		else
			cblend->amix = amix_val;
	}
	cblend->alpha = alpha;
	cblend->cmix = mix;
	cblend->cmask = (flags & argb_achn ? 0xff000000 : 0)|
					(flags & argb_rchn ? 0x00ff0000 : 0)|
					(flags & argb_gchn ? 0x0000ff00 : 0)|
					(flags & argb_bchn ? 0x000000ff : 0);
}

void gx_cmixsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmix mix, int alpha, int flags, gx_callBack cbf) {
	char *dptr, *sptr;
	register int tmp;
	struct gx_Rect_t rect;
	struct gx_Proc_t proc;
	struct blender_t blend;
	gx_Clip clp = dst->clipPtr ? dst->clipPtr : (gx_Clip)dst;
	proc.flag = -1; proc.done = 0;
	if (src->depth != dst->depth) {
		if (cbf) {
			proc.name = "depths ar not equal";
			cbf(&proc);
		}
		return;
	} else if (src == dst) {
		if (cbf) {
			proc.name = "destination = source";
			cbf(&proc);
		}
		return;
	}
	if (dst->depth != 32) {
		if (cbf) {
			proc.name = "depth is not 32 bpp";
			cbf(&proc);
		}
		return;
	}
	if (!roi) {
		rect.x = rect.y = 0;
		rect.w = src->width;
		rect.h = src->height;
	} else {
		rect.x = roi->x < 0 ? 0 : roi->x;
		rect.y = roi->y < 0 ? 0 : roi->y;
		rect.w = roi->w > src->width ? src->width : roi->w;
		rect.h = roi->h > src->height ? src->height : roi->h;
	}
	if ((tmp = x - clp->l) < 0) {
		rect.x -= tmp;
		rect.w += tmp;
		x = clp->l;
	} if ((tmp = clp->r - x) < rect.w) rect.w = tmp;
	if ((tmp = y - clp->t) < 0) {
		rect.y -= tmp;
		rect.h += tmp;
		y = clp->t;
	} if ((tmp = clp->b - y) < rect.h) rect.h = tmp;
	if (rect.w > 0 && rect.h > 0) {
		dptr = (char*)gx_getpaddr(dst, x, y);
		sptr = (char*)gx_getpaddr(src, rect.x, rect.y);
	} else dptr = sptr = 0;
	if (dptr == 0 || sptr == 0) {
		if (cbf) {
			proc.name = "region out of bound";
			cbf(&proc);
		}
		return;
	}
	proc.flag = 0;
	initblend(&blend, mix, alpha, 0, flags);
	if (cbf) {
		proc.name = "Blend Surface";
		proc.done = 0;
		cbf(&proc);
	}
	for (y = rect.h; y; --y) {
		cmix_cpy32(dptr, sptr, rect.w, &blend);
		dptr += dst->scanLen;
		sptr += src->scanLen;
		if (proc.flag) {
			if (proc.flag == -1) break;
			proc.done = (int)((rect.h - y) * (255./rect.h));
			cbf(&proc);
		}
	}
	if (proc.flag) {
		if (proc.flag == -1) {
			//~ proc.name = "aborted";
			proc.done = -1;
		} else {
			//~ proc.name = "done";
			proc.flag = 0;
			proc.done = 255;
		}
		cbf(&proc);
	}
}

void gx_clutsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, clut lut, int flags, gx_callBack cbf) {
	char *dptr, *sptr;
	register int tmp;
	struct gx_Rect_t rect;
	struct gx_Proc_t proc;
	struct blender_t blend;
	gx_Clip clp = dst->clipPtr ? dst->clipPtr : (gx_Clip)dst;
	proc.flag = -1; proc.done = 0;
	if (src->depth != dst->depth) {
		if (cbf) {
			proc.name = "depths ar not equal";
			cbf(&proc);
		}
		return;
	}
	if (dst->depth != 32) {
		if (cbf) {
			proc.name = "depth is not 32 bpp";
			cbf(&proc);
		}
		return;
	}
	if (!roi) {
		rect.x = rect.y = 0;
		rect.w = src->width;
		rect.h = src->height;
	} else {
		rect.x = roi->x < 0 ? 0 : roi->x;
		rect.y = roi->y < 0 ? 0 : roi->y;
		rect.w = roi->w > src->width ? src->width : roi->w;
		rect.h = roi->h > src->height ? src->height : roi->h;
	}
	if ((tmp = x - clp->l) < 0) {
		rect.x -= tmp;
		rect.w += tmp;
		x = clp->l;
	} if ((tmp = clp->r - x) < rect.w) rect.w = tmp;
	if ((tmp = y - clp->t) < 0) {
		rect.y -= tmp;
		rect.h += tmp;
		y = clp->t;
	} if ((tmp = clp->b - y) < rect.h) rect.h = tmp;
	if (rect.w > 0 && rect.h > 0) {
		if (src == dst) {
			dptr = sptr = (char*)gx_getpaddr(src, rect.x, rect.y);
		} else {
			dptr = (char*)gx_getpaddr(dst, x, y);
			sptr = (char*)gx_getpaddr(src, rect.x, rect.y);
		}
	} else dptr = 0;
	if (dptr == 0 || sptr == 0) {
		if (cbf) {
			proc.name = "region out of clip";
			cbf(&proc);
		}
		return;
	}
	proc.flag = 0;
	initblend(&blend, 0, -1U, lut, flags);
	if (cbf) {
		proc.name = "Color Lookup Surface";
		proc.done = 0;
		cbf(&proc);
	}
	for (y = rect.h; y; --y) {
		clut_cpy32(dptr, sptr, rect.w, &blend);
		dptr += dst->scanLen;
		sptr += src->scanLen;
		if (proc.flag) {
			if (proc.flag == -1) break;
			proc.done = (int)((rect.h - y) * (255./rect.h));
			cbf(&proc);
		}
	}
	if (proc.flag) {
		if (proc.flag == -1) {
			//~ proc.name = "aborted";
			proc.done = -1;
		} else {
			//~ proc.name = "done";
			proc.flag = 0;
			proc.done = 255;
		}
		cbf(&proc);
	}
}

void gx_cmatsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmat mat, int flags, gx_callBack cbf) {
	char *dptr, *sptr;
	register int tmp;
	struct gx_Rect_t rect;
	struct gx_Proc_t proc;
	struct blender_t blend;
	gx_Clip clp = dst->clipPtr ? dst->clipPtr : (gx_Clip)dst;
	proc.flag = -1; proc.done = 0;
	if (src->depth != dst->depth) {
		if (cbf) {
			proc.name = "depths ar not equal";
			cbf(&proc);
		}
		return;
	}
	if (dst->depth != 32) {
		if (cbf) {
			proc.name = "depth is not 32 bpp";
			cbf(&proc);
		}
		return;
	}
	if (!roi) {
		rect.x = rect.y = 0;
		rect.w = src->width;
		rect.h = src->height;
	} else {
		rect.x = roi->x < 0 ? 0 : roi->x;
		rect.y = roi->y < 0 ? 0 : roi->y;
		rect.w = roi->w > src->width ? src->width : roi->w;
		rect.h = roi->h > src->height ? src->height : roi->h;
	}
	if ((tmp = x - clp->l) < 0) {
		rect.x -= tmp;
		rect.w += tmp;
		x = clp->l;
	} if ((tmp = clp->r - x) < rect.w) rect.w = tmp;
	if ((tmp = y - clp->t) < 0) {
		rect.y -= tmp;
		rect.h += tmp;
		y = clp->t;
	} if ((tmp = clp->b - y) < rect.h) rect.h = tmp;
	if (rect.w > 0 && rect.h > 0) {
		if (src == dst) {
			dptr = sptr = (char*)gx_getpaddr(src, rect.x, rect.y);
		} else {
			dptr = (char*)gx_getpaddr(dst, x, y);
			sptr = (char*)gx_getpaddr(src, rect.x, rect.y);
		}
	} else dptr = 0;
/*	if (!roi) {
		rect.x = rect.y = 0;
		rect.w = src->width;
		rect.h = src->height;
	} else rect = *roi;
	if (src == dst) {
		//~ rect.x = x; rect.y = y;
		dptr = sptr = (char*)gx_cliprect(src, &rect);
	} else {
		sptr = (char*)gx_cliprect(src, &rect);
		rect.x = x; rect.y = y;
		dptr = (char*)gx_cliprect(dst, &rect);
	}
*/
	if (dptr == 0 || sptr == 0) {
		if (cbf) {
			proc.name = "region out of bound";
			cbf(&proc);
		}
		return;
	}
	proc.flag = 0;
	initblend(&blend, 0, -1U, mat, flags);
	if (cbf) {
		proc.name = "Color Lookup Surface";
		proc.done = 0;
		cbf(&proc);
	}
	for (y = rect.h; y; --y) {
		cmat_cpy32(dptr, sptr, rect.w, &blend);
		dptr += dst->scanLen;
		sptr += src->scanLen;
		if (proc.flag) {
			if (proc.flag == -1) break;
			proc.done = (int)((rect.h - y) * (255./rect.h));
			cbf(&proc);
		}
	}
	if (proc.flag) {
		if (proc.flag == -1) {
			//~ proc.name = "aborted";
			proc.done = -1;
		} else {
			//~ proc.name = "done";
			proc.flag = 0;
			proc.done = 255;
		}
		cbf(&proc);
	}
}

//{	Color matrix
fxmptr fxmHSL_lum(fxmptr dst, float val) {
	val /= 255;
	return fxmldf(dst,\
		1, 0, 0, val,
		0, 1, 0, val,
		0, 0, 1, val,
		0, 0, 0,  1);
}

fxmptr fxmHSL_sat(fxmptr dst, float val) {
	float x = 1 + (val > 0 ? 3*val/100 : val/100);
	return fxmldf(dst,\
		0.3086*(1-x)+x, .6094*(1-x), .0820*(1-x),0,
		0.3086*(1-x), .6094*(1-x)+x, .0820*(1-x),0,
		0.3086*(1-x), .6094*(1-x), .0820*(1-x)+x,0,
		0, 0, 0, 1);
}

fxmptr fxmHSL_hue(fxmptr dst, float val) {
	float cV = cos(val*(3.141592653589793/180));
	float sV = sin(val*(3.141592653589793/180));
	float lR = 0.213;
	float lG = 0.715;
	float lB = 0.072;
	return fxmldf(dst,
		lR+cV*(1-lR)+sV*(-lR), lG+cV*(-lG)+sV*(-lG),lB+cV*(-lB)+sV*(1-lB),0,
		lR+cV*(-lR)+sV*(0.143),lG+cV*(1-lG)+sV*(0.140),lB+cV*(-lB)+sV*(-0.283),0,
		lR+cV*(-lR)+sV*(lR-1), lG+cV*(-lG)+sV*(lG),lB+cV*(1-lB)+sV*(lB),0,
		0, 0, 0, 1);
}

void gx_cmatHSL(cmat dst, float hue, float sat, float lum) {
	fxpmat tmp;
	fxmidn((fxmptr)dst);
	if (hue) fxmmul((fxmptr)dst, (fxmptr)dst, fxmHSL_hue(&tmp, hue));
	if (sat) fxmmul((fxmptr)dst, (fxmptr)dst, fxmHSL_sat(&tmp, sat));
	if (lum) fxmmul((fxmptr)dst, (fxmptr)dst, fxmHSL_lum(&tmp, lum));
}

//}

//{	misc	:	gx_saveBmpCop, gx_saveBmpLut

//~ /*
void draw_rect(gx_Surf dst, gx_Rect rec, long c) {
	int x0 = rec->x;
	int y0 = rec->y;
	int x1 = rec->w + x0;
	int y1 = rec->h + y0;
	void *tmp = dst->movePtr;
	gx_vline(dst, x0, y0, y1, c);
	gx_vline(dst, x1, y0, y1, c);
	gx_hline(dst, x0, y0, x1, c);
	gx_hline(dst, x0, y1, x1, c);
	dst->movePtr = tmp;
}

void gx_saveBmpCop(const char* name, cmix cop){
	int i,j;
	gx_Surf surf = 0;
	if (cop && (surf = gx_createSurf(256, 256, 32, 0))) {
		gx_initdraw(surf, rop2_cpy);
		for(j = 0; j < 256; j++) {
			for(i = 0; i < 256; i++) {
				long color = rgbcol(cop(rgbrgb(j, j, j), rgbrgb(i, i, i)));
				gx_setpixel(surf, j, 255-i, color);
				//~ gx_setpixel(s,j,255-i,gx_mapcolor(s,*(int*)&tmp));
			}
		}
		gx_saveBMP((char*)name, surf, 0);
		gx_destroySurf(surf);
	}
}

void gx_saveBmpLut(const char* name, clut lut) {
	int j;
	gx_Surf surf = 0;
	long k = rgbcol(rgbnew(0xff, 0x00, 0x00, 0x00));
	long r = rgbcol(rgbnew(0xff, 0xff, 0x00, 0x00));
	long g = rgbcol(rgbnew(0xff, 0x00, 0xff, 0x00));
	long b = rgbcol(rgbnew(0xff, 0x00, 0x00, 0xff));
	long m = rgbcol(rgbnew(0xff, 0xff, 0xff, 0xff));
	if (lut && (surf = gx_createSurf(256, 256, 32, 0))) {
		for(j=0;j<256;j++)
			gx_hline(surf, 0, j, 256, m);
		for(j=0;j<256;j++) {
			gx_vline(surf, j, 255-lut->data[j].a, 256, k);
			//~ gx_vline(surf, j, 255-lut->data[j].r, 256, r);
			//~ gx_vline(surf, j, 255-lut->data[j].g, 256, g);
			//~ gx_vline(surf, j, 255-lut->data[j].b, 256, b);
			gx_setpixel(surf, j, 255-lut->data[j].r, r);
			gx_setpixel(surf, j, 255-lut->data[j].g, g);
			gx_setpixel(surf, j, 255-lut->data[j].b, b);
		}
		gx_saveBMP((char*)name, surf, 0);
		gx_destroySurf(surf);
	}
}// */
//}

#endif
