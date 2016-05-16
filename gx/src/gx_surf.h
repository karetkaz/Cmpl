#ifndef __graph_h
#define __graph_h

#define LOBIT(__VAL) ((__VAL) & -(__VAL))
#include <stdio.h>
#include "gx_color.h"

#ifdef _MSC_VER
#define inline __inline
#endif

#define gx_debug(msg, ...) fprintf(stdout, "%s:%d: debug: "msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)

typedef enum surfFlags {
	SurfType = 0x000f,		// 0-15
	Surf_2ds = 0x0000,		// 2d surface (tempPtr:= mask or palette) (CLUTPtr)
	Surf_3ds = 0x0001,		// 3d surface (tempPtr: zbuff)
	Surf_fnt = 0x0002,		// font (tempPtr: fontface) (LLUTPtr)
	//Surf_tex = 0x0002,		// texture (tempPtr: mipmaps) (LLUTPtr)
	//Surf_cur = 0x0001,		// cursor (tempPtr: temp mem)
	Surf_freeData = 0x0100,		// delete basePtr on destroy
	Surf_freeSurf = 0x0200,		// delete reference on destroy
	Surf_recycle  = 0x0400,
} surfFlags;

typedef enum {			// color block transfer
	cblt_conv_32 = 32,
	cblt_conv_24 = 24,
	cblt_conv_16 = 16,
	cblt_conv_15 = 15,
	cblt_conv_08 = 8,
	//cblt_conv_04 = 4,
	//cblt_conv_02 = 2,
	//cblt_conv_01 = 1,

	cblt_cpy_mix = 17,
	cblt_cpy_and = 18,
	cblt_cpy_ior = 19,
	cblt_cpy_xor = 20,

	cblt_set_mix = 25,
	//cblt_set_and = 26,
	//cblt_set_ior = 27,
	//cblt_set_xor = 28,
} cblt_type;

typedef enum {
	gradient_lin = 0x000000,		// Linear
	gradient_rad = 0x010000,		// Radial (elliptical)
	gradient_sqr = 0x020000,		// Square
	gradient_con = 0x030000,		// Conical
	gradient_spr = 0x040000,		// Spiral
} gradient_type;

typedef struct {			// 2d point type short
	uint16_t x;	//(left)
	uint16_t y;	//(top)
} gx_P2DS;

typedef struct gx_Clip {			// clipping bound structure [bounding box]
	union { uint16_t l, xmin; };	//(left)
	union { uint16_t t, ymin; };	//(top)
	union { uint16_t r, xmax; };	//(right)
	union { uint16_t b, ymax; };	//(bottom)
} *gx_Clip;

typedef struct gx_Rect {
	int32_t x;
	int32_t y;
	union { int32_t w, width; };
	union { int32_t h, height; };
} *gx_Rect;

typedef struct gx_Clut {			// Color Look Up Table (Palette) structure
	uint16_t        count;
	uint8_t         flags;
	uint8_t         trans;
	uint32_t        data[256];
} *gx_Clut;

typedef struct gx_Llut {			// Layer Look Up Table
	uint16_t        unused;			// font faces
	uint16_t        height;			// font height
	struct gx_FDIR {
		uint8_t     pad_x;
		uint8_t     pad_y;
		uint16_t    width;
		void*       basePtr;
	} data[256];
} *gx_Llut;

typedef struct gx_Surf {			// surface structure
	union {
		// firstchar, lastchar      // for fonts
		uint32_t    x0y0;			// for drawales
		gx_P2DS     hotSpot;		// for cursors
	};
	uint16_t        width;
	uint16_t        height;
	uint16_t        flags;			// img_fmt:8, mem_mgr:8
	uint8_t         depth;
	uint8_t         pixeLen;
	uint32_t        scanLen;		// scanlen / offsPos / imagecount
	gx_Clip         clipPtr;
	char*           basePtr;		// always base of surface
	union {
		gx_Clut     CLUTPtr;
		gx_Llut     LLUTPtr;
		//~ gx_Llut     facePtr;
		char*       tempPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
	};
} *gx_Surf;

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

int gx_initSurf(gx_Surf surf, int width, int height, int depth, surfFlags flags);
gx_Surf gx_createSurf(int width, int height, int depth, surfFlags flags);
int gx_attachSurf(gx_Surf dst, gx_Surf src, gx_Rect roi);
void gx_destroySurf(gx_Surf surf);

void* gx_cliprect(gx_Surf surf, gx_Rect rect);

static inline gx_Clip gx_getclip(gx_Surf s) {return s->clipPtr ? s->clipPtr : (gx_Clip)s;}

// Get Pixel Address
static inline void* gx_getpaddr(gx_Surf surf, int x, int y) {
	if ((unsigned)x >= surf->width || (unsigned)y > surf->height) {
		return NULL;
	}
	return surf->basePtr + (y * surf->scanLen) + (x * surf->pixeLen);

}
// Get Pixel Color
static inline uint32_t gx_getpixel(gx_Surf surf, int x, int y) {
	void *address = gx_getpaddr(surf, x, y);
	if (address != NULL) {
		switch (surf->depth) {
			case 32:
			case 24:
				return *(uint32_t*)address;
			case 15:
			case 16:
				return *(uint16_t*)address;
			case 8:
				return *(uint8_t*)address;
		}
	}
	return -1;
}
// Set Pixel Color
static inline void gx_setpixel(gx_Surf surf, int x, int y, uint32_t color) {
	void *address = gx_getpaddr(surf, x, y);
	if (address != NULL) {
		switch (surf->depth) {
			case 32:
			//case 24:
				*(uint32_t*)address = color;
			case 16:
			case 15:
				*(uint16_t*)address = color;
			case 8:
				*(uint8_t*)address = color;
		}
	}
}
// Get Pixel Color linear
static inline uint32_t gx_getpix16(gx_Surf surf, int32_t fpx, int32_t fpy, int linear) {
	int32_t lx, x = (uint32_t)fpx >> 16;
	int32_t ly, y = (uint32_t)fpy >> 16;
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
			uint32_t p0, p1, p2, p3;
			p0 = *(uint32_t*)(ofs + 0);
			p1 = *(uint32_t*)(ofs + 4);
			ofs += surf->scanLen;
			p2 = *(uint32_t*)(ofs + 0);
			p3 = *(uint32_t*)(ofs + 4);

			p0 = gx_mixcolor(p0, p1, lx);
			p2 = gx_mixcolor(p2, p3, lx);
			return gx_mixcolor(p0, p2, ly);
		}
		else if (lx) {
			uint32_t p0 = *(uint32_t*)(ofs + 0);
			uint32_t p1 = *(uint32_t*)(ofs + 4);

			return gx_mixcolor(p0, p1, lx);
		}
		else if (ly) {
			uint32_t p0 = *(uint32_t*)(ofs + 0);
			uint32_t p1 = *(uint32_t*)(ofs + surf->scanLen);

			return gx_mixcolor(p0, p1, ly);
		}
		return *(uint32_t*)ofs;
	}
	else if (surf->depth == 8) {
		if (lx && ly) {
			uint8_t p0, p1, p2, p3;
			p0 = *(uint8_t*)(ofs + 0);
			p1 = *(uint8_t*)(ofs + 4);
			ofs += surf->scanLen;
			p2 = *(uint8_t*)(ofs + 0);
			p3 = *(uint8_t*)(ofs + 4);

			p0 = gx_mixcolor(p0, p1, lx);
			p2 = gx_mixcolor(p2, p3, lx);
			return gx_mixcolor(p0, p2, ly);
		}
		else if (lx) {
			uint8_t p0 = *(uint8_t*)(ofs + 0);
			uint8_t p1 = *(uint8_t*)(ofs + 4);

			return gx_mixcolor(p0, p1, lx);
		}
		else if (ly) {
			uint8_t p0 = *(uint8_t*)(ofs + 0);
			uint8_t p1 = *(uint8_t*)(ofs + surf->scanLen);

			return gx_mixcolor(p0, p1, ly);
		}
		return *(uint32_t*)ofs;
	}

	return gx_getpixel(surf, x, y);
}

void g2_drawrect(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color);
void g2_fillrect(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color);

void g2_drawoval(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color);
void g2_filloval(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color);

void g2_drawline(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color);
void g2_drawbez2(gx_Surf surf, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
void g2_drawbez3(gx_Surf surf, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, uint32_t color);

void g2_clipText(gx_Rect rect, gx_Surf font, const char *text);
void g2_drawChar(gx_Surf surf, int x, int y, gx_Surf font, char chr, uint32_t color);
void g2_drawText(gx_Surf surf, int x, int y, gx_Surf font, const char *text, uint32_t color);

//- int gx_fillsurf(gx_Surf surf, gx_Rect rect, uint32_t color);
int gx_copysurf(gx_Surf surf, int x, int y, gx_Surf src, gx_Rect roi);
int gx_zoomsurf(gx_Surf surf, gx_Rect rect, gx_Surf src, gx_Rect roi, int linear);

// image effects
int gx_clutsurf(gx_Surf surf, gx_Rect roi, gx_Clut lut);
int gx_cmatsurf(gx_Surf surf, gx_Rect roi, uint32_t mat[16]);
int gx_gradsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut, gradient_type gradtype, int repeat);
int gx_blursurf(gx_Surf surf, gx_Rect roi, int radius);

//~ TODO: remove Internal functions
typedef void (*cblt_proc)(void* dst, void *src, void *lut, size_t cnt);
cblt_proc gx_getcbltf(cblt_type, int srcDepth);

void colcpy_32_abgr(void* dst, void *src, void *lut, size_t cnt);
void colcpy_32_bgr(void* dst, void *src, void *lut, size_t cnt);
void colcpy_32_24(void* dst, void *src, void *lut, size_t cnt);
void colcpy_32_08(void* dst, void *src, void *lut, size_t cnt);

void colset_32mix(void* dst, void *src, void *lut, size_t cnt);

// TODO: remove Utility functions
char* fext(const char* name);
char* readI32(char *ptr, int *outVal);
char* readF32(char *str, float *outVal);
//~ char* readVec(char *str, vector dst, scalar defw);
//~ read a (key = value) pair, return the value string
char* readKVP(char *ptr, char *key, char *sep, char *wsp);

// Removed
//~ extern pattern gx_patt[41];
//~ int gx_loadCUR(gx_Surf, const char*, int);
//~ int gx_drawCurs(gx_Surf dst, int x, int y, gx_Surf cur, int hide);

#if __cplusplus
}
#endif
#endif
