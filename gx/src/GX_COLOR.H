#ifndef __Color_Space_h
#define __Color_Space_h

#include "gx_surf.h"

typedef enum {				// ARGB Mask
	argb_bchn = 0x000001,		// Blue Channel
	argb_gchn = 0x000002,		// Green Channel
	argb_rchn = 0x000004,		// Red Channel
	argb_achn = 0x000008,		// Alpha Channel
	argb_mchn = 0x000008,		// Master Channel
	argb_rgbc = 0x000007,		// RGB Channels
	argb_allc = 0x00000f,		// All Channels
	argb_srca = 0x000010,		// Source has Alpha
	//~ argb_mlut = 0x000020,		// look up only master
	argb_clut = 0x000100,		// color lookup if argb_achn is set 
	argb_cmat = 0x000200,		// color matrix
} argb_chns;

typedef union {				// ARGB color structutre
	unsigned long col;
	struct {
		unsigned char	b;
		unsigned char	g;
		unsigned char	r;
		unsigned char	a;
	};
	struct {
		unsigned char	b;
		unsigned char	g;
		unsigned char	r;
		unsigned char	a;
	} rgb;
	struct {
		unsigned char	k;
		unsigned char	y;
		unsigned char	m;
		unsigned char	c;
	} cmy;
	struct {
		unsigned char	l;
		unsigned char	s;
		unsigned short	h;
	} hsl;
	struct {
		unsigned char	v;
		unsigned char	s;
		unsigned short	h;
	} hsv;
	struct {
		unsigned char	c1;
		unsigned char	c2;
		unsigned short	y;
	} yiq;
	struct {
		unsigned char	u;
		unsigned char	v;
		unsigned short	y;
	} yuv;
	/*enum {
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

typedef struct clut_t {			// Color Look Up Table (Palette) structure
	unsigned short	count;
	unsigned char	flags;
	unsigned char	trans;
	argb	data[256];
} *clut, *Palette;

typedef enum gradient_type {		// gradient types
	gradient_lin = 0x01,		// Linear
	gradient_rad = 0x02,		// Radial (elliptical)
	gradient_sqr = 0x03,		// Square
	gradient_con = 0x04,		// Conical
	gradient_spr = 0x05,		// Spiral

	gradient_mir = 0x08,		// Miror (reflect)
	gradient_rep = 0x10,		// repeat
	//~ gradient_rev = 0x20,		// reverse
} gradient_type;

typedef struct cmat_t {
	long x[16];
}*cmat;

typedef argb cmix(argb, argb);

#pragma pack(1)

#if __cplusplus
#define gx_extern extern "C"
#else
#define gx_extern extern
#endif

long rgblum(argb);
long rgbcol(argb);
argb rgbrgb(int r, int g, int b);
argb rgbnew(int a, int r, int g, int b);
argb rgblerp(argb, argb, int);
argb rgblerpw(argb, argb, int);
argb rgblerpd(argb, argb, double);

// Color Blending

cmix rgbcpy;		// Normal
cmix rgbadd;		// Add
cmix rgbsub;		// Subtract
cmix rgbdif;		// Difference
cmix rgbmul;		// Multiply
cmix rgbdiv;		// Divide
cmix rgbmin;		// Darken
cmix rgbmax;		// Lighten
cmix rgbnot;		// Logical NOT
cmix rgband;		// Logical AND
cmix rgbior;		// Logical OR
cmix rgbxor;		// Logical XOR
cmix rgbscr;		// Screen
cmix rgbovr;		// Overlay
//~ cmix rgbrch;		// Red Channel
//~ cmix rgbgch;		// Green Channel
//~ cmix rgbbch;		// Blue Channel

// Color LookUp Table

void gx_clutclr(clut, int argb_mask);
void gx_clutinv(clut, int argb_mask);
void gx_clutblend(clut, int argb_mask, argb c1, argb c2);
void gx_cluthisto(clut, gx_Surf, int argb_mask);
void gx_clutBCG(clut dst, int cmask, int brg, int cntr, float gamma);
void gx_cmatHSL(cmat dst, float hue, float sat, float lum);

typedef struct gx_colProc {
	//~ common
	const char *fn_name;
	//~ struct gx_emgr *err;		// Error / callback manager
	//~ struct gx_mmgr *mem;		// Memory manager
	//~ struct gx_pgrm *progress;		// Progress monitor
	//~ gx_callBack *progress;
	//~ int global_state		/* For checking call sequence validity */

	union {
		//~ struct gx_Rect_s roi;
		//~ FILE*		dst;
		void*	_xxx;
	};
	/*union {
		struct mat {				// color / geom matrix
			long _[4][4];			// fixed point matrix
		};
		struct flt {				// filter
			long	n;
			long _[9][9];			// fixed point matrix ???
		};
		struct grd {				// gradient data
			int	sx, sy, x, y;
			double	dx, dy, l, a;
			struct gx_Clut_t lut;		// lookup table
			double (*fn)(struct grd*);
		};
		struct gx_Clut_t lut;			// lookup table
		argb (*mix)(argb, argb);		// blender
	};*/
	unsigned long	cmask;
	unsigned long	alpha;
} *gx_cProc;

void gx_prepare_gradient(gx_cProc);
void gx_prepare_colorlut(gx_cProc);
void gx_prepare_colormat(gx_cProc);
void gx_prepare_blending(gx_cProc);

void gx_process_scanline(gx_cProc);

void gx_process(gx_cProc);
void gx_destroy(gx_cProc);

//~ void gx_fillsurf(gx_Surf dst, color_t src, cmix mix, int alpha, int cmask, gx_ppf);
void gx_clutsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, clut lut, int flags, gx_callBack);
void gx_cmatsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmat mat, int flags, gx_callBack);
void gx_cmixsurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cmix mix, int alpha, int flags, gx_callBack);

void gx_gradsurf(gx_Surf dst, gx_Rect rec, clut lut, int gradtype, cmix mix, int alpha, int argb_mask, gx_callBack);

void gx_saveBmpLut(const char* dst, clut src);
void gx_saveBmpCop(const char* dst, cmix src);

int gx_loadBMPCLUT(clut, char*);

#endif
