#ifndef __GX_SURFACE
#define __GX_SURFACE

#include <stdio.h>
#include <stdint.h>
#include "gx_color.h"

typedef enum {
	ImageType = 0x000f,		// 0-15
	Image2d = 0x0000,		// 2d image (tempPtr:= null)
	ImageIdx = 0x0001,		// indexed (tempPtr: CLUTPtr: palette)
	ImageFnt = 0x0002,		// font (tempPtr: LLUTPtr: fontface)
	Image3d = 0x0003,		// 3d image (tempPtr: zbuff)
	freeData = 0x0100,		// delete basePtr on destroy
	freeImage = 0x0200,		// delete reference on destroy
} ImageFlags;

typedef enum {
	gradient_lin = 0x00,		// Linear
	gradient_rad = 0x01,		// Radial (elliptical)
	gradient_sqr = 0x02,		// Square
	gradient_con = 0x03,		// Conical
	gradient_spr = 0x04,		// Spiral
	mask_type    = 0x07,		// gradient type
	flag_repeat  = 0x08,		// repeat
	flag_alpha   = 0x10,		// write alpha channel only
} GradientFlags;

// Rectangle
typedef struct GxRect {
	int32_t x0;  // left
	int32_t y0;  // top
	int32_t x1;  // right
	int32_t y1;  // bottom
} *GxRect;

// Color Look Up Table (Palette)
typedef struct GxCLut {
	uint32_t count;
	argb data[256];
} *GxCLut;

// Layer Look Up Table
typedef struct GxFLut {
	uint32_t count;            // font faces
	struct GxFace {
		uint8_t pad_x;
		uint8_t pad_y;
		uint16_t width;
		void *basePtr;
	} data[256];
} *GxFLut;

// image structure
typedef struct GxImage {
	uint16_t width;
	uint16_t height;

	uint16_t flags;
	uint8_t depth;
	uint8_t pixelLen;
	uint32_t scanLen;

	char *basePtr;

	union {
		GxCLut CLUTPtr;	// palette
		GxFLut LLUTPtr;	// font-faces
		char *tempPtr; // fixme: remove: clutPtr / cibgPtr (cursor BG data) / ...
	};
} *GxImage;

#ifdef __cplusplus
extern "C" {
#endif

GxImage createImage(GxImage recycle, int width, int height, int depth, ImageFlags flags);
GxImage sliceImage(GxImage recycle, GxImage parent, GxRect roi);
void destroyImage(GxImage image);

static inline int rectEmpty(struct GxRect roi) {
	return roi.x0 >= roi.x1 || roi.y0 >= roi.y1;
}
static inline int rectWidth(struct GxRect rect) {
	return rect.x1 - rect.x0;
}
static inline int rectHeight(struct GxRect rect) {
	return rect.y1 - rect.y0;
}
static inline void rectPosition(GxRect rect, int x, int y) {
	rect->x1 = x + rect->x1 - rect->x0;
	rect->x0 = x;
	rect->y1 = y + rect->y1 - rect->y0;
	rect->y0 = y;
}
void* clipRect(GxImage image, GxRect roi);

// Get Pixel Address
static inline void* refPixel(GxImage image, int x, int y) {
	if ((unsigned) x >= image->width || (unsigned) y >= image->height) {
		return NULL;
	}
	return image->basePtr + ((size_t) y * image->scanLen) + ((size_t) x * image->pixelLen);
}

// Get Pixel Color
static inline uint32_t getPixel(GxImage image, int x, int y) {
	void *address = refPixel(image, x, y);
	if (address == NULL) {
		// out of bounds
		return 0;
	}
	switch (image->depth) {
		default:
			// unknown depth
			return 0;

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
// Set Pixel Color
static inline void setPixel(GxImage image, int x, int y, uint32_t color) {
	void *address = refPixel(image, x, y);
	if (address == NULL) {
		// out of bounds
		return;
	}

	switch (image->depth) {
		case 32:
			// TODO: case 24:
			*(uint32_t*)address = color;
			break;

		case 16:
		case 15:
			*(uint16_t*)address = color;
			break;

		case 8:
			*(uint8_t*)address = color;
			break;
	}
}
// Get Pixel Color using linear interpolation
static inline uint32_t getPixelLinear(GxImage image, int32_t x16, int32_t y16) {
	int32_t lx = x16 >> 8 & 0xff;
	int32_t ly = y16 >> 8 & 0xff;
	int32_t x = x16 >> 16;
	int32_t y = y16 >> 16;

	char *ofs = refPixel(image, x, y);
	if (ofs == NULL) {
		// no pixel, return black
		return 0;
	}

	if (x + 1 >= image->width) {
		// no interpolation, no next pixel
		lx = 0;
	}
	if (y + 1 >= image->height) {
		// no interpolation, no next pixel
		ly = 0;
	}

	switch (image->depth) {
		case 32:
			if (lx && ly) {
				argb p0 = *(argb*)(ofs + 0);
				argb p1 = *(argb*)(ofs + 4);
				ofs += image->scanLen;
				argb p2 = *(argb*)(ofs + 0);
				argb p3 = *(argb*)(ofs + 4);

				p0 = mix_rgb(lx, p0, p1);
				p2 = mix_rgb(lx, p2, p3);
				return mix_rgb(ly, p0, p2).val;
			}
			if (lx) {
				argb p0 = *(argb*)(ofs + 0);
				argb p1 = *(argb*)(ofs + 4);
				return mix_rgb(lx, p0, p1).val;
			}
			if (ly) {
				argb p0 = *(argb*)(ofs + 0);
				argb p1 = *(argb*)(ofs + image->scanLen);
				return mix_rgb(ly, p0, p1).val;
			}
			return *(uint32_t*)ofs;

		case 24:
			// TODO: not implemented
			return *(uint32_t*)ofs;

		case 15:
		case 16:
			// TODO: not implemented
			return *(uint16_t*)ofs;

		case 8:
			if (lx && ly) {
				int p0 = *(uint8_t*)(ofs + 0);
				int p1 = *(uint8_t*)(ofs + 1);
				ofs += image->scanLen;
				int p2 = *(uint8_t*)(ofs + 0);
				int p3 = *(uint8_t*)(ofs + 1);

				p0 = mix_s8(lx, p0, p1);
				p2 = mix_s8(lx, p2, p3);
				return mix_s8(ly, p0, p2);
			}
			if (lx) {
				int p0 = *(uint8_t*)(ofs + 0);
				int p1 = *(uint8_t*)(ofs + 1);
				return mix_s8(lx, p0, p1);
			}
			if (ly) {
				int p0 = *(uint8_t*)(ofs + 0);
				int p1 = *(uint8_t*)(ofs + image->scanLen);
				return mix_s8(ly, p0, p1);
			}
			return *(uint8_t*)ofs;
	}
	return 0;
}

// draw
void fillRect(GxImage image, int x0, int y0, int x1, int y1, int incl, uint32_t color);

void drawChar(GxImage image, int x0, int y0, GxImage font, int chr, uint32_t color);

void clipText(GxRect rect, GxImage font, const char *text);
void drawText(GxImage image, GxRect rect, GxImage font, const char *text, uint32_t color);

int fillImage(GxImage image, const GxRect roi, void *color, void *extra, bltProc blt);
int copyImage(GxImage image, int x, int y, GxImage src, const GxRect roi, void *extra, bltProc blt);
int transformImage(GxImage image, GxRect rect, GxImage src, const GxRect roi, int interpolation, float mat[16]);

// image read write
GxImage loadImg(GxImage dst, const char *src, int depth);
GxImage loadTtf(GxImage dst, const char *src, int height, int firstChar, int lastChars);
GxImage loadBmp(GxImage dst, const char *src, int depth);
#ifndef NO_LIBJPEG
GxImage loadJpg(GxImage dst, const char *src, int depth);
#endif
#ifndef NO_LIBPNG
GxImage loadPng(GxImage dst, const char *src, int depth);
#endif
GxImage loadFnt(GxImage dst, const char *src);
int saveBmp(const char *dst, GxImage src, int flags);


// image effects
int blurImage(GxImage image, int radius, double sigma);

int drawGradient(GxImage dst, GxRect roi, GradientFlags type, int length, uint32_t *colors);

static inline int convertImage(GxImage image, int x, int y, GxImage src, GxRect roi) {
	return copyImage(image, x, y, src, roi, NULL, getBltProc(src->depth, image->depth));
}

static inline int blendImage(GxImage image, int x, int y, GxImage src, GxRect roi, int alpha) {
	if (image->depth != src->depth) {
		// source and destination must have same depth
		return -2;
	}
	return copyImage(image, x, y, src, roi, &alpha, getBltProc(blt_cpy_mix, image->depth));
}

#if __cplusplus
}
#endif

#define LOBIT(__VAL) ((__VAL) & -(__VAL))
#define gx_debug(msg, ...) do { fprintf(stdout, "%s:%d: %s: debug: "msg"\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)

#endif
