#ifndef __Color_Space_h
#define __Color_Space_h

#include "g2_surf.h"

static inline argb rgbval(int val) {
	argb res;
	res.val = val;
	return res;
}
static inline long rgblum(argb col) {
	return (col.r * 76 + col.g * 150 + col.b * 29) >> 8;
}
static inline argb rgbrgb(int r, int g, int b) {
	argb res;
	//~ res.a = 0xff;
	res.r = r & 0xff;
	res.g = g & 0xff;
	res.b = b & 0xff;
	return res;
}
static inline argb rgbclamp(int r, int g, int b) {
	argb res;
	//~ res.a = 0xff;
	res.r = r < 0 ? 0 : r > 255 ? 255 : r;
	res.g = g < 0 ? 0 : g > 255 ? 255 : g;
	res.b = b < 0 ? 0 : b > 255 ? 255 : b;
	return res;
}
static inline argb rgbset(int a, int r, int g, int b) {
	argb res;
	res.a = a & 0xff;
	res.r = r & 0xff;
	res.g = g & 0xff;
	res.b = b & 0xff;
	return res;
}
static inline argb rgbmix16(argb lhs, argb rhs, int fpa16) {
	argb res;
	register long tmp;
	tmp = (lhs.b - rhs.b) * fpa16;
	res.b = rhs.b + ((tmp + (tmp >> 16) + 1) >> 16);
	tmp = (lhs.g - rhs.g) * fpa16;
	res.g = rhs.g + ((tmp + (tmp >> 16) + 1) >> 16);
	tmp = (lhs.r - rhs.r) * fpa16;
	res.r = rhs.r + ((tmp + (tmp >> 16) + 1) >> 16);
	return res;
}

typedef enum gradient_type {		// gradient types
	gradient_lin = 0x000000,		// Linear
	gradient_rad = 0x010000,		// Radial (elliptical)
	gradient_sqr = 0x020000,		// Square
	gradient_con = 0x030000,		// Conical
	gradient_spr = 0x040000,		// Spiral
	gradient_max = 0x040000,		// last gradient type
	gradient_TYP = 0x070000,		// mask to get type of gradient

	gradient_rep = 0x080000,		// repeat
} gradient_type;
gx_extern int gx_gradsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut, int gradtype);

#endif
