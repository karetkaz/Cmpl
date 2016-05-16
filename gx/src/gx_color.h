#ifndef __Color_Space_h
#define __Color_Space_h

#include <stdint.h>

typedef union {				// ARGB color structutre
	uint32_t val;
	uint32_t col;
	struct {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
	/*struct {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	} rgb;
	struct {
		uint8_t k;
		uint8_t y;
		uint8_t m;
		uint8_t c;
	} cmy;
	struct {
		uint8_t  l;
		uint8_t  s;
		uint16_t h;
	} hsl;
	struct {
		uint8_t  v;
		uint8_t  s;
		uint16_t h;
	} hsv;
	struct {
		uint8_t c1;
		uint8_t c2;
		uint16_t y;
	} yiq;
	struct {
		uint8_t u;
		uint8_t v;
		uint16_t y;
	} yuv;
	enum {
		cs_argb = 'argb',
		cs_cmyk = 'cmyk',
		cs_hsv  = '_hsv',
		cs_yuv  = '_yuv',
		//~ cs_xyz  = ' xyz',
		//~ cs_rgb  = ' rgb',
		//~ cs_cmy  = ' cmy',
		//~ cs_hsl  = ' hsl',
		//~ cs_yiq  = ' yiq',
	} argb_cs; // */
} argb;

static inline uint32_t rch(uint32_t xrgb) { return xrgb >> 16 & 0xff; }
static inline uint32_t gch(uint32_t xrgb) { return xrgb >>  8 & 0xff; }
static inline uint32_t bch(uint32_t xrgb) { return xrgb >>  0 & 0xff; }

static inline uint32_t gx_mixcolor(uint32_t c1, uint32_t c2, int alpha) {
	//~ uint32_t r = (c1 & 0xff0000) + (alpha * ((c2 & 0xff0000) - (c1 & 0xff0000)) >> 8);
	//~ uint32_t g = (c1 & 0xff00) + (alpha * ((c2 & 0xff00) - (c1 & 0xff00)) >> 8);
	//~ uint32_t b = (c1 & 0xff) + (alpha * ((c2 & 0xff) - (c1 & 0xff)) >> 8);
	//~ return (r & 0xff0000) | (g & 0xff00) | (b & 0xff);

	uint32_t rb = c1 & 0xff00ff, g = c1 & 0x00ff00;
	rb = rb + (alpha * ((c2 & 0xff00ff) - rb) >> 8);
	g = g + (alpha * ((c2 & 0x00ff00) - g) >> 8);
	return (rb & 0xff00ff) | (g & 0x00ff00);
}

static inline uint32_t gx_mixgray(uint32_t c1, uint32_t c2, int alpha) {
	uint32_t c = c1 & 0xff;
	c = c + (alpha * ((c2 & 0xff) - c) >> 8);
	return c & 0xff;
}

static inline uint32_t clamp_col(int32_t val) {
	if (val > 255) val = 255;
	if (val < 0) val = 0;
	return val;
}

static inline uint32_t rgblum(argb col) {
	return (col.r * 76 + col.g * 150 + col.b * 29) >> 8;
}

static inline argb rgbval(uint32_t val) {
	argb res;
	res.val = val;
	return res;
}
static inline argb rgbrgb(int r, int g, int b) {
	argb res;
	//~ res.a = 0xff;
	res.r = r & 0xff;
	res.g = g & 0xff;
	res.b = b & 0xff;
	return res;
}
static inline argb clamp_rgb(int r, int g, int b) {
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
static inline uint32_t __rgb(int r, int g, int b) { return r << 16 | g << 8 | b; }
static inline uint32_t __argb(int a, int r, int g, int b) { return a << 24 | r << 16 | g << 8 | b; }

static inline argb rgbmix16(argb lhs, argb rhs, int fpa16) {
	argb res;
	register uint32_t tmp;
	tmp = (lhs.b - rhs.b) * fpa16;
	res.b = rhs.b + ((tmp + (tmp >> 16) + 1) >> 16);
	tmp = (lhs.g - rhs.g) * fpa16;
	res.g = rhs.g + ((tmp + (tmp >> 16) + 1) >> 16);
	tmp = (lhs.r - rhs.r) * fpa16;
	res.r = rhs.r + ((tmp + (tmp >> 16) + 1) >> 16);
	return res;
}
#endif
