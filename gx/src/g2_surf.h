#ifndef __graph_h
#define __graph_h

#define LOBIT(__VAL) ((__VAL) & -(__VAL))
#include <stdio.h>
#ifdef _MSC_VER
#define inline __inline
#endif

#define gx_debug(msg, ...) fprintf(stdout, "%s:%d: debug: "msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum surfFlags {
	SurfMask = 0x000f,		// 0-15
	Surf_2ds = 0x0000,		// 2d surface (tempPtr:= mask or palette) (CLUTPtr)
	Surf_3ds = 0x0001,		// 3d surface (tempPtr: zbuff)
	Surf_fnt = 0x0002,		// font (tempPtr: fontface) (LLUTPtr)
	//Surf_tex = 0x0002,		// texture (tempPtr: mipmaps) (LLUTPtr)
	//Surf_cur = 0x0001,		// cursor (tempPtr: temp mem)
	Surf_freeData = 0x0100,		// delete basePtr on destroy
	Surf_freeSurf = 0x0200,		// delete reference on destroy
	Surf_recycle  = 0x0400,
} surfFlags;

typedef enum {			// color bloc transfer
	cblt_conv = 0,
	cblt_copy = 1,
	cblt_fill = 2,
} cblt_type;

typedef union {				// ARGB color structutre
	unsigned long val;
	unsigned long col;
	struct {
		unsigned char	b;
		unsigned char	g;
		unsigned char	r;
		unsigned char	a;
	};
	/*struct {
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

typedef struct {			// 2d point type short
	unsigned short	x;	//(left)
	unsigned short	y;	//(top)
} gx_P2DS;

typedef struct gx_Clip {			// clipping bound structure [bounding box]
	union {unsigned short	l, xmin;};	//(left)
	union {unsigned short	t, ymin;};	//(top)
	union {unsigned short	r, xmax;};	//(right)
	union {unsigned short	b, ymax;};	//(bottom)
} *gx_Clip;

typedef struct gx_Rect {
	signed long	x;
	signed long	y;
	union {
		signed long	w;
		signed long	width;
	};
	union {
		signed long	h;
		signed long	height;
	};
} *gx_Rect;

typedef struct gx_Clut {			// Color Look Up Table (Palette) structure
	unsigned short	count;
	unsigned char	flags;
	unsigned char	trans;
	argb	data[256];
	/*union gx_RGBQ {
		unsigned long col;
		struct {
			unsigned char	b;
			unsigned char	g;
			unsigned char	r;
			unsigned char	a;
		};
	} data[256]; // */
} *gx_Clut;

typedef struct gx_Llut {			// Layer Look Up Table
	unsigned short	unused;			// font faces
	unsigned short	height;			// font height
	struct gx_FDIR {
		unsigned char	pad_x;
		unsigned char	pad_y;
		unsigned short	width;
		void*		basePtr;
	} data[256];
} *gx_Llut;

typedef struct gx_Surf {			// surface structure
	union {
		gx_Clip		clipPtr;
		gx_P2DS		hotSpot;
	};
	unsigned short	width;
	unsigned short	height;
	unsigned short	flags;			// img_fmt:8, mem_mgr:8
	unsigned char	depth;
	unsigned char	pixeLen;
	unsigned long	scanLen;		// scanlen / offsPos / imagecount
	char*			basePtr;		// always base of surface
	union {
		gx_Clut		CLUTPtr;
		gx_Llut		LLUTPtr;
		//~ gx_Llut		facePtr;
		char*		tempPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
	};
	//~ void*		offsPtr;		// offsPtr(scanLen, x, y):(void*)	parm[eax, ecx, edx] value [edx]
	//~ void*		movePtr;		// movePtr(offsPtr,  col):(void)	parm[edi, eax]
//------------------------------------------------ 32 bytes
	//~ void*	fillPtr;		// fillPtr(offsPtr, cnt, col):(void)	parm[edi, ecx, eax]
	//~ void*	resd[3];		// reserved
//------------------------------------------------ 48 bytes
} *gx_Surf;

typedef enum {
	gradient_lin = 0x000000,		// Linear
	gradient_rad = 0x010000,		// Radial (elliptical)
	gradient_sqr = 0x020000,		// Square
	gradient_con = 0x030000,		// Conical
	gradient_spr = 0x040000,		// Spiral
} gradient_type;

//typedef unsigned char pattern[8];

#pragma pack(1)

#if __cplusplus
#define gx_extern extern "C"
#else
#define gx_extern extern
#endif

int gx_loadBMP(gx_Surf surf, const char *fileName, int depth);
int gx_loadJPG(gx_Surf surf, const char *fileName, int depth);
int gx_loadPNG(gx_Surf surf, const char *fileName, int depth);
int gx_loadFNT(gx_Surf surf, const char *fileName);
//~ int gx_loadTTF(gx_Surf surf, const char *fileName, int height);

int gx_saveBMP(const char *fileName, gx_Surf surf, int flags);

int gx_initSurf(gx_Surf surf, unsigned width, unsigned height, unsigned depth, surfFlags flags);
gx_Surf gx_createSurf(unsigned width, unsigned height, unsigned depth, surfFlags flags);
int gx_attachSurf(gx_Surf dst, gx_Surf src, gx_Rect roi);
void gx_destroySurf(gx_Surf surf);

void* gx_cliprect(gx_Surf surf, gx_Rect rect);

static inline gx_Clip gx_getclip(gx_Surf s) {return s->clipPtr ? s->clipPtr : (gx_Clip)s;}

static inline long gx_mixpixel(long c1, long c2, int alpha) {
	//~ long r = (c1 & 0xff0000) + (alpha * ((c2 & 0xff0000) - (c1 & 0xff0000)) >> 8);
	//~ long g = (c1 & 0xff00) + (alpha * ((c2 & 0xff00) - (c1 & 0xff00)) >> 8);
	//~ long b = (c1 & 0xff) + (alpha * ((c2 & 0xff) - (c1 & 0xff)) >> 8);
	//~ return (r & 0xff0000) | (g & 0xff00) | (b & 0xff);

	long rb = c1 & 0xff00ff, g = c1 & 0x00ff00;
	rb = rb + (alpha * ((c2 & 0xff00ff) - rb) >> 8);
	g = g + (alpha * ((c2 & 0x00ff00) - g) >> 8);
	return (rb & 0xff00ff) | (g & 0x00ff00);
}

static inline char gx_mixgray(char c1, char c2, int alpha) {
	long c = c1 & 0xff;
	c = c + (alpha * ((c2 & 0xff) - c) >> 8);
	return c & 0xff;
}

// Get Pixel Address
static inline void* gx_getpaddr(gx_Surf surf, int x, int y) {
	if ((unsigned)x >= surf->width || (unsigned)y > surf->height) {
		return NULL;
	}
	return surf->basePtr + (y * surf->scanLen) + (x * surf->pixeLen);

}
// Get Pixel Color
static inline long gx_getpixel(gx_Surf surf, int x, int y) {
	void *address = gx_getpaddr(surf, x, y);
	if (address != NULL) {
		switch (surf->depth) {
			case 32:
			case 24:
				return *(long*)address;
			case 15:
			case 16:
				return *(short*)address;
			case 8:
				return *(char*)address;
		}
	}
	return -1;
}
// Set Pixel Color
static inline void gx_setpixel(gx_Surf surf, int x, int y, long color) {
	void *address = gx_getpaddr(surf, x, y);
	if (address != NULL) {
		switch (surf->depth) {
			case 32:
			//case 24:
				*(long*)address = color;
			case 16:
			case 15:
				*(short*)address = color;
			case 8:
				*(char*)address = color;
		}
	}
}
// Get Pixel Color linear
static inline long gx_getpix16(gx_Surf surf, long fpx, long fpy, int linear) {
	int lx, x = (unsigned)fpx >> 16;
	int ly, y = (unsigned)fpy >> 16;
	char *ofs = gx_getpaddr(surf, x, y);

	if (ofs == NULL) {
		return 0;
	}

	lx = fpx >> 8 & 0xff;
	ly = fpy >> 8 & 0xff;
	if (x + 1 >= surf->width) {
		lx = 0;
	}
	if (y + 1 >= surf->height) {
		ly = 0;
	}
	if (!linear) {
		lx = ly = 0;
	}

	if (surf->depth == 32) {
		if (lx && ly) {
			long p0, p1, p2, p3;
			p0 = *(long*)(ofs + 0);
			p1 = *(long*)(ofs + 4);
			ofs += surf->scanLen;
			p2 = *(long*)(ofs + 0);
			p3 = *(long*)(ofs + 4);

			p0 = gx_mixpixel(p0, p1, lx);
			p2 = gx_mixpixel(p2, p3, lx);
			return gx_mixpixel(p0, p2, ly);
		}
		else if (lx) {
			long p0 = *(long*)(ofs + 0);
			long p1 = *(long*)(ofs + 4);

			return gx_mixpixel(p0, p1, lx);
		}
		else if (ly) {
			long p0 = *(long*)(ofs + 0);
			long p1 = *(long*)(ofs + surf->scanLen);

			return gx_mixpixel(p0, p1, ly);
		}
		return *(long*)ofs;
	}
	else if (surf->depth == 8) {
		if (lx && ly) {
			char p0, p1, p2, p3;
			p0 = *(char*)(ofs + 0);
			p1 = *(char*)(ofs + 4);
			ofs += surf->scanLen;
			p2 = *(char*)(ofs + 0);
			p3 = *(char*)(ofs + 4);

			p0 = gx_mixpixel(p0, p1, lx);
			p2 = gx_mixpixel(p2, p3, lx);
			return gx_mixpixel(p0, p2, ly);
		}
		else if (lx) {
			char p0 = *(char*)(ofs + 0);
			char p1 = *(char*)(ofs + 4);

			return gx_mixpixel(p0, p1, lx);
		}
		else if (ly) {
			char p0 = *(char*)(ofs + 0);
			char p1 = *(char*)(ofs + surf->scanLen);

			return gx_mixpixel(p0, p1, ly);
		}
		return *(long*)ofs;
	}

	return gx_getpixel(surf, x, y);
}

void g2_drawrect(gx_Surf surf, int x1, int y1, int x2, int y2, long color);
void g2_fillrect(gx_Surf surf, int x1, int y1, int x2, int y2, long color);

void g2_drawoval(gx_Surf surf, int x1, int y1, int x2, int y2, long color);
void g2_filloval(gx_Surf surf, int x1, int y1, int x2, int y2, long color);

void g2_drawline(gx_Surf surf, int x1, int y1, int x2, int y2, long color);
void g2_drawbez2(gx_Surf surf, int x1, int y1, int x2, int y2, int x3, int y3, long color);
void g2_drawbez3(gx_Surf surf, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, long color);

void g2_clipText(gx_Rect rect, gx_Surf font, const char *text);
void g2_drawChar(gx_Surf surf, int x, int y, gx_Surf font, unsigned chr, long color);
void g2_drawText(gx_Surf surf, int x, int y, gx_Surf font, const char *text, long color);

//- int gx_fillsurf(gx_Surf surf, gx_Rect rect, long color);
int gx_copysurf(gx_Surf surf, int x, int y, gx_Surf src, gx_Rect roi);
int gx_zoomsurf(gx_Surf surf, gx_Rect rect, gx_Surf src, gx_Rect roi, int linear);

// image effects
int gx_clutsurf(gx_Surf surf, gx_Rect roi, gx_Clut lut);
int gx_cmatsurf(gx_Surf surf, gx_Rect roi, long mat[16]);
int gx_gradsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut, gradient_type gradtype, int repeat);
int gx_blursurf(gx_Surf surf, gx_Rect roi, int radius);

//~ TODO: remove Internal functions
typedef void (*cblt_proc)(void* dst, void* src, unsigned count, void* extra);
gx_extern cblt_proc gx_getcbltf(cblt_type, int dstdepth, int opbpp);
gx_extern void gx_callcbltf(cblt_proc, void* dst, void* src, unsigned count, void* extra);

// Removed
//~ extern pattern gx_patt[41];
//~ int gx_loadCUR(gx_Surf, const char*, int);
//~ int gx_drawCurs(gx_Surf dst, int x, int y, gx_Surf cur, int hide);

#if __cplusplus
}
#endif
#endif
