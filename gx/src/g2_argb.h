#ifndef __Color_Space_h
#define __Color_Space_h

#include "g2_surf.h"

typedef enum {						// ARGB Mask
	argb_AMask = 0x0000ff,		// alphaMask
	argb_srca  = 0x000100,		// Source has Alpha

	argb_bchn  = 0x001000,		// Blue Channel
	argb_gchn  = 0x002000,		// Green Channel
	argb_rchn  = 0x004000,		// Red Channel
	argb_achn  = 0x008000,		// Alpha Channel
	argb_mchn  = 0x008000,		// Master Channel
	argb_rgbc  = 0x007000,		// RGB Channels
	argb_allc  = 0x00f000,		// All Channels

} argb_chns;

typedef enum gradient_type {		// gradient types
	gradient_lin = 0x000000,		// Linear
	gradient_rad = 0x010000,		// Radial (elliptical)
	gradient_sqr = 0x020000,		// Square
	gradient_con = 0x030000,		// Conical
	gradient_spr = 0x040000,		// Spiral
	gradient_max = 0x040000,		// Spiral
	gradient_TYP = 0x0f0000,		// mask to get type of gradient

	gradient_mir = 0x100000,		// Miror (reflect)
	gradient_rep = 0x200000,		// repeat
	//~ gradient_rev = 0x20,		// reverse
} gradient_type;

typedef argb cmix(argb, argb);
//~ typedef void colput(argb*, argb);
typedef void colset(argb*, argb, unsigned);
typedef void colcpy(argb*, argb*, unsigned);

#pragma pack(1)

#if __cplusplus
#define gx_extern extern "C"
#else
#define gx_extern extern
#endif

inline long rgblum(argb col) {
	return (col.r * 76 + col.g * 150 + col.b * 29) >> 8;
}
inline argb rgbrgb(int r, int g, int b) {
	argb res;
	//~ res.a = 0xff;
	res.r = r & 0xff;
	res.g = g & 0xff;
	res.b = b & 0xff;
	return res;
}
inline argb rgbval(int val) {
	argb res;
	res.val = val;
	return res;
}
inline argb rgbset(int a, int r, int g, int b) {
	argb res;
	res.a = a & 0xff;
	res.r = r & 0xff;
	res.g = g & 0xff;
	res.b = b & 0xff;
	return res;
}

argb rgblerp(argb, argb, int);
argb rgblerpw(argb, argb, int);
argb rgblerpd(argb, argb, double);

// Color Blending

colcpy rgbcpy;		// Normal
colcpy rgbadd;		// Add
colcpy rgbsub;		// Subtract
colcpy rgbdif;		// Difference
colcpy rgbmul;		// Multiply
colcpy rgbdiv;		// Divide
colcpy rgbmin;		// Darken
colcpy rgbmax;		// Lighten
colcpy rgbnot;		// Logical NOT
colcpy rgband;		// Logical AND
colcpy rgbior;		// Logical OR
colcpy rgbxor;		// Logical XOR
colcpy rgbscr;		// Screen
colcpy rgbovr;		// Overlay
//~ cmix rgbrch;		// Red Channel
//~ cmix rgbgch;		// Green Channel
//~ cmix rgbbch;		// Blue Channel

colcpy cpy_32_08;
colcpy cpy_32_15;
colcpy cpy_32_16;
colcpy cpy_32_24;
colcpy cpy_32cpy;
colcpy cpy_32and;
colcpy cpy_32ior;
colcpy cpy_32xor;
colcpy cpy_32mix;		// alpha mix
colcpy cpy_32add;		// Add
colcpy cpy_32sub;		// Subtract
colcpy cpy_32dif;		// Difference
colcpy cpy_32mul;		// Multiply
colcpy cpy_32div;		// Divide
colcpy cpy_32min;		// Darken
colcpy cpy_32max;		// Lighten
colcpy cpy_32scr;		// Screen
colcpy cpy_32ovr;		// Overlay
colcpy cpy_32rch;		// Red Channel
colcpy cpy_32gch;		// Green Channel
colcpy cpy_32bch;		// Blue Channel

void col_32mixmc(void *dst, char* src, int col, unsigned len);
void col_24mixmc(void *dst, char* src, int col, unsigned len);
void col_16mixmc(void *dst, char* src, int col, unsigned len);
void col_15mixmc(void *dst, char* src, int col, unsigned len);
void col_08mixmc(void *dst, char* src, int col, unsigned len);

// Color LookUp Table

void gx_clutclr(clut, int argb_mask);
void gx_clutinv(clut, int argb_mask);
void gx_clutblend(clut, int argb_mask, argb c1, argb c2);
void gx_cluthisto(clut, gx_Surf, int argb_mask);
void gx_clutBCG(clut dst, int cmask, int brg, int cntr, float gamma);
//~ void gx_cmatHSL(cmat dst, float hue, float sat, float lum);

//~ void gx_fillsurf(gx_Surf dst, color_t src, cmix mix, int alpha, int cmask, gx_ppf);
void gx_clutsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, clut lut, int flags);
//~ void gx_cmatsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmat mat, int flags);
void gx_cmixsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmix mix, int alpha, int flags);

int gx_gradsurf(gx_Surf dst, gx_Rect rec, clut lut, int gradtype/* , cmix mix, int alpha, int argb_mask */);

void gx_saveBmpLut(const char* dst, clut src);
void gx_saveBmpCop(const char* dst, cmix src);

int gx_loadBMPCLUT(clut, char*);

#endif
