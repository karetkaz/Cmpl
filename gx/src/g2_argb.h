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

/*/ Color Blending

//~ colcpy rgbcpy;		// Normal
//~ colcpy rgbadd;		// Add
//~ colcpy rgbsub;		// Subtract
//~ colcpy rgbdif;		// Difference
//~ colcpy rgbmul;		// Multiply
//~ colcpy rgbdiv;		// Divide
//~ colcpy rgbmin;		// Darken
//~ colcpy rgbmax;		// Lighten
//~ colcpy rgbnot;		// Logical NOT
//~ colcpy rgband;		// Logical AND
//~ colcpy rgbior;		// Logical OR
//~ colcpy rgbxor;		// Logical XOR
//~ colcpy rgbscr;		// Screen
//~ colcpy rgbovr;		// Overlay

//~ cmix rgbrch;		// Red Channel
//~ cmix rgbgch;		// Green Channel
//~ cmix rgbbch;		// Blue Channel

//~ colcpy cpy_32_08;
//~ colcpy cpy_32_15;
//~ colcpy cpy_32_16;
//~ colcpy cpy_32_24;
//~ colcpy cpy_32cpy;
//~ colcpy cpy_32and;
//~ colcpy cpy_32ior;
//~ colcpy cpy_32xor;
//~ colcpy cpy_32mix;		// alpha mix
//~ colcpy cpy_32add;		// Add
//~ colcpy cpy_32sub;		// Subtract
//~ colcpy cpy_32dif;		// Difference
//~ colcpy cpy_32mul;		// Multiply
//~ colcpy cpy_32div;		// Divide
//~ colcpy cpy_32min;		// Darken
//~ colcpy cpy_32max;		// Lighten
//~ colcpy cpy_32scr;		// Screen
//~ colcpy cpy_32ovr;		// Overlay
//~ colcpy cpy_32rch;		// Red Channel
//~ colcpy cpy_32gch;		// Green Channel
//~ colcpy cpy_32bch;		// Blue Channel

//~ void col_32mixmc(void *dst, char* src, int col, unsigned len);
//~ void col_24mixmc(void *dst, char* src, int col, unsigned len);
//~ void col_16mixmc(void *dst, char* src, int col, unsigned len);
//~ void col_15mixmc(void *dst, char* src, int col, unsigned len);
//~ void col_08mixmc(void *dst, char* src, int col, unsigned len);

// Color LookUp Table

//~ void gx_clutclr(clut, int argb_mask);
//~ void gx_clutinv(clut, int argb_mask);
//~ void gx_clutblend(clut, int argb_mask, argb c1, argb c2);
//~ void gx_cluthisto(clut, gx_Surf, int argb_mask);
//~ void gx_clutBCG(clut dst, int cmask, int brg, int cntr, float gamma);
//~ void gx_cmatHSL(cmat dst, float hue, float sat, float lum);

//~ void gx_fillsurf(gx_Surf dst, color_t src, cmix mix, int alpha, int cmask, gx_ppf);
//~ void gx_clutsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, clut lut, int flags);
//~ void gx_cmatsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmat mat, int flags);
//~ void gx_cmixsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmix mix, int alpha, int flags);
*/

typedef enum gradient_type {		// gradient types
	gradient_lin = 0x000000,		// Linear
	gradient_rad = 0x010000,		// Radial (elliptical)
	gradient_sqr = 0x020000,		// Square
	gradient_con = 0x030000,		// Conical
	gradient_spr = 0x040000,		// Spiral
	gradient_max = 0x040000,		// Spiral
	gradient_TYP = 0x0f0000,		// mask to get type of gradient

	gradient_rep = 0x100000,		// repeat
} gradient_type;
gx_extern int gx_gradsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut, int gradtype);

#endif
