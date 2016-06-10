// scripting support

#include "gx_gui.h"
#include "g3_draw.h"
#include "internal.h"

extern double epsilon;

int gx_oilPaintSurf(gx_Surf image, gx_Rect roi, int radius, int intensity) {
	const int w = image->width;
	const int h = image->height;
	struct gx_Surf temp;

	/*bool inRange(int cx, int cy, int i, int j) {
		static int sqrt(int a) {
			int rem = 0;
			int root = 0;
			for (int i = 0; i < 16; i += 1) {
				root <<= 1;
				rem <<= 2;
				rem += a >> 30;
				a <<= 2;
				if (root < rem) {
					root += 1;
					rem -= root;
					root += 1;
				}
			}
			return root >> 1;
		}
		static int32 sqrt2(int32 x) {
			int32 testDiv;
			int32 root = 0;			// Clear root
			int32 remHi = 0;		// Clear high part of partial remainder
			int32 remLo = x;		// Get argument into low part of partial remainder
			int32 count = 16;		// Load loop counter

			for (; count; count -= 1) {
				remHi = (remHi << 2) | (remLo >> 30);		// get 2 bits of arg
				remLo <<= 2;
				root <<= 1;							// Get ready for the next bit in the root
				testDiv = (root << 1) + 1;			// Test radical
				if (remHi >= testDiv) {
					remHi -= testDiv;
					root += 1;
				}
			}
			return root;
		}
		cx -= i;
		cy -= j;
		double d = double.sqrt(cx * cx + cy * cy);
		return d < radius;
		//~ return sqrt(cx * cx + cy * cy) < radius;
	}*/
	int averageR[256];
	int averageG[256];
	int averageB[256];
	int intensityCount[256];

	int x, y, i, j;
	if (intensity > 256) {
		return -1;
	}
	if (gx_initSurf(&temp, w, h, 32, Surf_2ds) != 0) {
		return -1;
	}

	for (y = 0; y < h; y += 1) {
		int top = y - radius;
		int bottom = y + radius;
		if (top < 0) top = 0;
		if (bottom >= h) bottom = h - 1;

		//~ gx_debug("processing line", variant(&y));

		for (x = 0; x < w; x += 1) {
			int left = x - radius;
			int right = x + radius;
			if (left < 0) left = 0;
			if (right >= w) right = w - 1;

			for (i = 0; i < intensity; i += 1) {
				averageR[i] = 0;
				averageG[i] = 0;
				averageB[i] = 0;
				intensityCount[i] = 0;
			}

			for (j = top; j <= bottom; j += 1) {
				for (i = left; i <= right; i += 1) {
					//~ if (!inRange(x, y, i, j))
					//~ continue;

					argb rgb = rgbval(gx_getpixel(image, i, j));

					int r = rgb.r;
					int g = rgb.g;
					int b = rgb.b;
					int intensityIndex = (r + g + b) * intensity / 765;

					intensityCount[intensityIndex] += 1;
					averageR[intensityIndex] += r;
					averageG[intensityIndex] += g;
					averageB[intensityIndex] += b;

				}
			}

			int maxIndex = 0;
			int curMax = intensityCount[maxIndex];
			for (i = 1; i < intensity; i += 1) {
				if (curMax < intensityCount[i]) {
					curMax = intensityCount[i];
					maxIndex = i;
				}
			}

			if (curMax > 0) {
				//~ int rgb = int(clamp_rgb(sumR[maxIndex] / curMax, sumG[maxIndex] / curMax, sumB[maxIndex] / curMax));
				//~ setPixel(dest, row, col, rgb);
				int r = averageR[maxIndex] / curMax;
				int g = averageG[maxIndex] / curMax;
				int b = averageB[maxIndex] / curMax;

				gx_setpixel(&temp, x, y, rgbrgb(r, g, b).val);
			}
		}
	}
	gx_copysurf(image, 0, 0, &temp, NULL);
	gx_destroySurf(&temp);
	(void)roi;
	return 0;
}

//#{ old style arguments
static inline void* poparg(libcContext rt, void *result, size_t size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, rt->argv, size);
	}
	else {
		result = rt->argv;
	}
	*(char**)&rt->argv += padded(size, vm_size);
	return result;
}

#define poparg(__ARGV, __TYPE) (((__TYPE*)(*(char**)&((__ARGV)->argv) += ((sizeof(__TYPE) + 3) & ~3)))[-1])
static inline void* pophnd(libcContext rt) { return poparg(rt, void*); }
static inline int32_t popi32(libcContext rt) { return poparg(rt, int32_t); }
static inline float32_t popf32(libcContext rt) { return poparg(rt, float32_t); }
static inline float64_t popf64(libcContext rt) { return poparg(rt, float64_t); }
#undef poparg

static inline void* popref(libcContext rt) { int32_t p = popi32(rt); return p ? rt->rt->_mem + p : NULL; }
static inline void* popsym(libcContext rt) { int32_t p = popi32(rt); return p ? rtFindSym(rt->rt, p, 0) : NULL; }
static inline char* popstr(libcContext rt) { return popref(rt); }
//#}

rtContext rt = NULL;
symn renderMethod = NULL;
symn mouseCallBack = NULL;
symn keyboardCallBack = NULL;

extern struct camera cam[1];		// camera
extern struct mesh msh;			// mesh
extern struct gx_Surf offs;		// offscreen
extern struct gx_Surf font;		// font

//~ extern struct vector eye, tgt, up;

extern int readorevalMesh(mesh msh, char *src, int line, char *tex, int div);
extern vector getobjvec(int Normal);
extern void meshInfo(mesh msh);

extern int draw;
extern int resx;
extern int resy;
extern flip_scr flip;		// swap buffer method

extern char *obj;		// object filename
extern char *tex;		// texture filename

extern char *ccStd;//"src/stdlib.gxc";	// stdlib script file (from gx.ini)
extern char *ccGfx;//"src/gfxlib.gxc";	// gfxlib script file (from gx.ini)
extern char *ccLog;//"out/debug.out";
extern char *ccDmp;//"out/dump.out";

//#{#region Surfaces
static struct gx_Surf surfaces[256];
static const size_t surfCount = sizeof(surfaces) / sizeof(*surfaces);

static gx_Surf newSurf(int w, int h) {
	gx_Surf hnd = surfaces;
	while (hnd < surfaces + surfCount) {
		if (!hnd->depth) {
			if (gx_initSurf(hnd, w, h, 32, Surf_recycle) == 0) {
				return hnd;
			}
			break;
		}
		hnd += 1;
	}
	return NULL;
}
static gx_Surf getSurf(gx_Surf hnd) {
	if (hnd == NULL)
		return NULL;

	if (hnd == &offs)
		return &offs;

	if (hnd < surfaces || hnd > surfaces + surfCount) {
		gx_debug("invalid surface @%p", hnd);
		return NULL;
	}

	if (hnd->depth == 0) {
		gx_debug("recycled surface @%p", hnd);
		return NULL;
	}
	return hnd;
}
static void delSurf(gx_Surf hnd) {
	if (hnd == &offs) {
		return;
	}

	if (getSurf(hnd) == hnd) {
		gx_destroySurf(hnd);
		hnd->depth = 0;
	}
}

typedef enum {
	surfOpGetDepth,
	surfOpGetWidth,
	surfOpGetHeight,
	//~ surfOpGetPixel,
	//~ surfOpGetPixfp,
	//~ surfOpSetPixel,
			surfOpClipRect,

	surfOpDrawLine,
	surfOpDrawBez2,
	surfOpDrawBez3,
	surfOpDrawRect,
	surfOpFillRect,
	surfOpDrawOval,
	surfOpFillOval,
	surfOpDrawText,

	surfOpCopySurf,
	surfOpZoomSurf,
	surfOpClutSurf,
	surfOpCmatSurf,
	surfOpGradSurf,
	surfOpBlurSurf,
	surfOpOilPaint,

	surfOpNewSurf,
	surfOpDelSurf,
	surfOpGetOffScreen,

	surfOpBmpRead,
	surfOpJpgRead,
	surfOpPngRead,
	surfOpBmpWrite,

	surfOpEvalFpCB,
	surfOpFillFpCB,
	surfOpCopyFpCB,
	surfOpEvalPxCB,
	surfOpFillPxCB,
	surfOpCopyPxCB,
} surfFunc;
static vmError surfCall(libcContext rt) {
	switch ((surfFunc)rt->data) {
		case surfOpGetDepth: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				reti32(rt, dst->depth);
				return noError;
			}
			break;
		}
		case surfOpGetWidth: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				reti32(rt, dst->width);
				return noError;
			}
			break;
		}
		case surfOpGetHeight: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				reti32(rt, dst->height);
				return noError;
			}
			break;
		}

		case surfOpClipRect: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				gx_Rect rect = popref(rt);
				void *ptr = gx_cliprect(dst, rect);
				reti32(rt, ptr != NULL);
				return noError;
			}
			break;
		}

		case surfOpDrawLine: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawline(dst, x0, y0, x1, y1, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpDrawBez2: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int x2 = popi32(rt);
				int y2 = popi32(rt);
				int col = popi32(rt);
				g2_drawbez2(dst, x0, y0, x1, y1, x2, y2, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpDrawBez3: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int x2 = popi32(rt);
				int y2 = popi32(rt);
				int x3 = popi32(rt);
				int y3 = popi32(rt);
				int col = popi32(rt);
				g2_drawbez3(dst, x0, y0, x1, y1, x2, y2, x3, y3, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpDrawRect: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawrect(dst, x0, y0, x1, y1, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpFillRect: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_fillrect(dst, x0, y0, x1, y1, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpDrawOval: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawoval(dst, x0, y0, x1, y1, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpFillOval: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_filloval(dst, x0, y0, x1, y1, (uint32_t) col);
				return noError;
			}
			break;
		}
		case surfOpDrawText: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				char *str = popstr(rt);
				int col = popi32(rt);
				g2_drawText(dst, x0, y0, &font, str, (uint32_t) col);
				return noError;
			}
			break;
		}

		case surfOpCopySurf: {
			gx_Surf dst = getSurf(pophnd(rt));
			int x = popi32(rt);
			int y = popi32(rt);
			gx_Surf src = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);

			if (dst && src) {
				gx_copysurf(dst, x, y, src, roi);
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpZoomSurf: {
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect rect = popref(rt);
			gx_Surf src = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			int mode = popi32(rt);

			if (dst && src) {
				gx_zoomsurf(dst, rect, src, roi, mode);
				rethnd(rt, dst);
				return noError;
			}
			break;
		}

		case surfOpEvalPxCB: {		// gxSurf evalSurfrgb(gxSurf dst, gxRect &roi, int callBack(int x, int y));
			rtContext rt_ = rt->rt;
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: evalSurf called with a roi");
			}
			if (dst && callback) {
				#pragma pack(push, 4)
				struct { int32_t x, y; } args;
				#pragma pack(pop)
				char *cBuffY = dst->basePtr;
				for (args.y = 0; args.y < dst->height; args.y += 1) {
					uint32_t *cBuff = (uint32_t*)cBuffY;
					cBuffY += dst->scanLen;
					for (args.x = 0; args.x < dst->width; args.x += 1) {
						if (invoke(rt_, callback, &cBuff[args.x], &args, NULL) != 0) {
							return executionAborted;
						}
					}
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpFillPxCB: {		// gxSurf fillSurfrgb(gxSurf dst, gxRect &roi, int callBack(int col));
			rtContext rt_ = rt->rt;
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: fillSurf called with a roi");
			}
			if (dst && callback) {
				int sx, sy;
				char *cBuffY = dst->basePtr;
				for (sy = 0; sy < dst->height; sy += 1) {
					uint32_t *cBuff = (uint32_t*)cBuffY;
					cBuffY += dst->scanLen;
					for (sx = 0; sx < dst->width; sx += 1) {
						if (invoke(rt_, callback, &cBuff[sx], &cBuff[sx], NULL) != 0) {
							return executionAborted;
						}
					}
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpCopyPxCB: {		// gxSurf copySurfrgb(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, int callBack(int dst, int src));
			rtContext rt_ = rt->rt;
			gx_Surf dst = getSurf(pophnd(rt));
			int x = popi32(rt);
			int y = popi32(rt);
			gx_Surf src = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			if (dst && src) {
				register char *dptr, *sptr;
				struct gx_Rect clip;
				int x1, y1;

				clip.x = roi ? roi->x : 0;
				clip.y = roi ? roi->y : 0;
				clip.w = roi ? roi->w : src->width;
				clip.h = roi ? roi->h : src->height;

				if(!(sptr = (char*)gx_cliprect(src, &clip)))
					return noError;

				clip.x = x;
				clip.y = y;
				if (clip.x < 0)
					clip.w -= clip.x;
				if (clip.y < 0)
					clip.h -= clip.y;

				if(!(dptr = (char*)gx_cliprect(dst, &clip)))
					return noError;

				if (callback) {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					for (y = clip.y; y < y1; y += 1) {
						uint32_t* cbdst = (uint32_t*)dptr;
						uint32_t* cbsrc = (uint32_t*)sptr;
						for (x = clip.x; x < x1; x += 1) {
							#pragma pack(push, 4)
							struct {int32_t dst, src;} args = {
								.dst = *cbdst,
								.src = *cbsrc
							};
							#pragma pack(pop)
							if (invoke(rt_, callback, cbdst, &args, NULL) != 0) {
								return executionAborted;
							}
							cbdst += 1;
							cbsrc += 1;
						}
						dptr += dst->scanLen;
						sptr += src->scanLen;
					}
				}
				else {
					y1 = clip.y + clip.h;
					for (y = clip.y; y < y1; y += 1) {
						memcpy(dptr, sptr, (size_t) (4 * clip.w));
						dptr += dst->scanLen;
						sptr += src->scanLen;
					}
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}

		case surfOpEvalFpCB: {		// gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(float x, float y));
			rtContext rt_ = rt->rt;
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			// step in x, and y direction
			float dx01 = 1.f / dst->width;
			float dy01 = 1.f / dst->height;

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: evalSurf called with a roi");
			}
			if (dst && callback) {
				int sx, sy;
				#pragma pack(push, 4)
				struct { float32_t x, y; } args;
				#pragma pack(pop)
				char *cBuffY = dst->basePtr;
				for (args.y = sy = 0; sy < dst->height; sy += 1, args.y += dy01) {
					uint32_t *cBuff = (uint32_t*)cBuffY;
					cBuffY += dst->scanLen;
					for (args.x = sx = 0; sx < dst->width; sx += 1, args.x += dx01) {
						struct vector result;
						if (invoke(rt_, callback, &result, &args, NULL) != noError) {
							return executionAborted;
						}
						cBuff[sx] = vecrgb(&result).col;
					}
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpFillFpCB: {		// gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));
			rtContext rt_ = rt->rt;
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			if (dst && callback) {
				struct gx_Rect clip;
				char *dptr;
				int sx, sy;

				if (roi == NULL) {
					clip.x = clip.y = 0;
					clip.w = dst->width;
					clip.h = dst->height;
				}
				else {
					clip = *roi;
				}
				if (!(dptr = (char*)gx_cliprect(dst, &clip))) {
					rethnd(rt, dst);
					return noError;
				}
				for (sy = 0; sy < clip.h; sy += 1) {
					uint32_t *cBuff = (uint32_t*)dptr;
					dptr += dst->scanLen;
					for (sx = 0; sx < clip.w; sx += 1) {
						struct vector result;
						struct vector args = vecldc(rgbval(cBuff[sx]));
						if (invoke(rt_, callback, &result, &args, NULL) != 0) {
							return executionAborted;
						}
						cBuff[sx] = vecrgb(&result).col;
					}
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpCopyFpCB: {		// gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, float alpha, vec4f callBack(vec4f dst, vec4f src));
			rtContext rt_ = rt->rt;
			gx_Surf dst = getSurf(pophnd(rt));
			int x = popi32(rt);
			int y = popi32(rt);
			gx_Surf src = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			float alpha = popf32(rt);
			symn callback = popsym(rt);

			if (dst && src) {
				register char *dptr, *sptr;
				struct gx_Rect clip;
				int x1, y1;

				clip.x = roi ? roi->x : 0;
				clip.y = roi ? roi->y : 0;
				clip.w = roi ? roi->w : src->width;
				clip.h = roi ? roi->h : src->height;

				if(!(sptr = (char*)gx_cliprect(src, &clip))) {
					rethnd(rt, dst);
					return noError;
				}

				clip.x = x;
				clip.y = y;
				if (clip.x < 0)
					clip.w -= x;
				if (clip.y < 0)
					clip.h -= y;
				if(!(dptr = (char*)gx_cliprect(dst, &clip))) {
					rethnd(rt, dst);
					return noError;
				}

				if (callback) {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					if (alpha >= 1) {
						for (y = clip.y; y < y1; y += 1) {
							uint32_t* cbdst = (uint32_t*)dptr;
							uint32_t* cbsrc = (uint32_t*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								struct vector result;
								#pragma pack(push, 4)
								struct { struct vector dst, src; } args = {
										.dst = vecldc(rgbval(*cbdst)),
										.src = vecldc(rgbval(*cbsrc))
								};
								#pragma pack(pop)
								if (invoke(rt_, callback, &result, &args, NULL) != 0) {
									return executionAborted;
								}
								*cbdst = vecrgb(&result).col;
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += dst->scanLen;
							sptr += src->scanLen;
						}
					}
					else if (alpha > 0) {
						int alpha255 = (int)(alpha * 255);
						for (y = clip.y; y < y1; y += 1) {
							uint32_t *cbdst = (uint32_t*)dptr;
							uint32_t* cbsrc = (uint32_t*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								struct vector result;
								#pragma pack(push, 4)
								struct { struct vector dst, src; } args = {
									.dst = vecldc(rgbval(*cbdst)),
									.src = vecldc(rgbval(*cbsrc))
								};
								#pragma pack(pop)
								if (invoke(rt_, callback, &result, &args, NULL) != 0) {
									return executionAborted;
								}
								*cbdst = gx_mixcolor(*cbdst, vecrgb(&result).col, alpha255);
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += dst->scanLen;
							sptr += src->scanLen;
						}
					}
					else {
						// alpha <= 0, so do nothing
					}
				}
				else {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					if (alpha >= 1) {
						for (y = clip.y; y < y1; y += 1) {
							memcpy(dptr, sptr, (size_t) (4 * clip.w));
							dptr += dst->scanLen;
							sptr += src->scanLen;
						}
					}
					else if (alpha > 0 && alpha < 1) {
						int alpha255 = (int)(alpha * 255);
						for (y = clip.y; y < y1; y += 1) {
							uint32_t *cbdst = (uint32_t*)dptr;
							uint32_t* cbsrc = (uint32_t*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								*cbdst = gx_mixcolor(*cbdst, *cbsrc, alpha255);
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += dst->scanLen;
							sptr += src->scanLen;
						}
					}
					else {
						// alpha <= 0, so do nothing
					}
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}

		case surfOpNewSurf: {
			int w = popi32(rt);
			int h = popi32(rt);
			rethnd(rt, newSurf(w, h));
			return noError;
		}
		case surfOpDelSurf: {
			delSurf(getSurf(pophnd(rt)));
			return noError;
		}
		case surfOpGetOffScreen: {
			rethnd(rt, &offs);
			return noError;
		}

		case surfOpBmpRead: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				char *fileName = popstr(rt);
				int error = gx_loadBMP(dst, fileName, 32);
				rethnd(rt, dst);
				//~ gx_debug("gx_readBMP(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
			break;
		}
		case surfOpJpgRead: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				char *fileName = popstr(rt);
				int error = gx_loadJPG(dst, fileName, 32);
				rethnd(rt, dst);
				//~ gx_debug("readJpg(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
			break;
		}
		case surfOpPngRead: {
			gx_Surf dst = getSurf(pophnd(rt));
			if (dst != NULL) {
				char *fileName = popstr(rt);
				int error = gx_loadPNG(dst, fileName, 32);
				rethnd(rt, dst);
				//~ gx_debug("gx_readPng(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
			break;
		}
		case surfOpBmpWrite: {
			gx_Surf src = getSurf(pophnd(rt));
			if (src != NULL) {
				char *fileName = popstr(rt);
				int error = gx_saveBMP(fileName, src, 0);
				//~ gx_debug("gx_saveBMP(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
			break;
		}

		case surfOpClutSurf: {
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);

			if (dst && lut) {
				gx_clutsurf(dst, roi, lut);
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpGradSurf: {
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);
			int mode = popi32(rt);
			int repeat = popi32(rt);

			if (dst) {
				gx_gradsurf(dst, roi, lut, (gradient_type) mode, repeat);
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpBlurSurf: {
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			int radius = popi32(rt);

			if (dst) {
				gx_blursurf(dst, roi, radius);
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpCmatSurf: {		// gxSurf cmatSurf(gxSurf dst, gxRect &roi, mat4f &mat);
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			matrix mat = popref(rt);

			if (dst && mat) {
				int i, fxpmat[16];			// wee will do fixed point arithmetic with the matrix
				struct gx_Rect rect;
				char *dptr;

				if (dst->depth != 32)
					return executionAborted;

				rect.x = roi ? roi->x : 0;
				rect.y = roi ? roi->y : 0;
				rect.w = roi ? roi->w : dst->width;
				rect.h = roi ? roi->h : dst->height;

				if(!(dptr = gx_cliprect(dst, &rect)))
					return executionAborted;

				for (i = 0; i < 16; i += 1) {
					fxpmat[i] = mat->d[i] * (1 << 16);
				}

				while (rect.h--) {
					int x;
					argb *cBuff = (argb*)dptr;
					for (x = 0; x < rect.w; x += 1) {
						int r = cBuff->r & 0xff;
						int g = cBuff->g & 0xff;
						int b = cBuff->b & 0xff;
						int ro = (r * fxpmat[ 0] + g * fxpmat[ 1] + b * fxpmat[ 2] + 256*fxpmat[ 3]) >> 16;
						int go = (r * fxpmat[ 4] + g * fxpmat[ 5] + b * fxpmat[ 6] + 256*fxpmat[ 7]) >> 16;
						int bo = (r * fxpmat[ 8] + g * fxpmat[ 9] + b * fxpmat[10] + 256*fxpmat[11]) >> 16;
						//~ cBuff->a = (r * fxpmat[12] + g * fxpmat[13] + b * fxpmat[14] + a * fxpmat[15]) >> 16;
						*cBuff = clamp_rgb(ro, go, bo);
						cBuff += 1;
					}
					dptr += dst->scanLen;
				}
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
		case surfOpOilPaint: {		// gxSurf oilPaint(gxSurf dst, gxRect &roi, int radius, intensity);
			gx_Surf dst = getSurf(pophnd(rt));
			gx_Rect roi = popref(rt);
			int radius = popi32(rt);
			int intensity = popi32(rt);

			if (dst != NULL) {
				gx_oilPaintSurf(dst, roi, radius, intensity);
				rethnd(rt, dst);
				return noError;
			}
			break;
		}
	}
	return executionAborted;
}
static vmError surfSetPixel(libcContext rt) {
	gx_Surf surf;
	if ((surf = getSurf(pophnd(rt)))) {
		unsigned rowy;
		int x = popi32(rt);
		int y = popi32(rt);
		int col = popi32(rt);

		if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
			return noError;
		}

		rowy = y * surf->scanLen;
		if (surf->depth == 32) {
			uint32_t *ptr = (uint32_t*)(surf->basePtr + rowy);
			ptr[x] = (uint32_t) col;
			return noError;
		}
		else if (surf->depth == 8) {
			uint8_t *ptr = (uint8_t*)(surf->basePtr + rowy);
			ptr[x] = (uint8_t) col;
			return noError;
		}

		//~ gx_setpixel(surf, x, y, col);
		//~ return noError;
	}
	return executionAborted;
}
static vmError surfGetPixel(libcContext rt) {
	gx_Surf surf;
	if ((surf = getSurf(pophnd(rt)))) {
		unsigned rowy;
		int x = popi32(rt);
		int y = popi32(rt);

		if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
			reti32(rt, 0);
			return noError;
		}
		rowy = y * surf->scanLen;
		if (surf->depth == 32) {
			uint32_t *ptr = (uint32_t*)(surf->basePtr + rowy);
			reti32(rt, ptr[x]);
			return noError;
		}
		else if (surf->depth == 8) {
			uint8_t *ptr = (uint8_t*)(surf->basePtr + rowy);
			reti32(rt, ptr[x]);
			return noError;
		}

		//~ reti32(rt, gx_getpixel(surf, x, y));
		//~ return noError;
	}
	return executionAborted;
}
static vmError surfGetPixfp(libcContext rt) {
	gx_Surf surf;
	if ((surf = getSurf(pophnd(rt)))) {
		double x = popf64(rt);
		double y = popf64(rt);
		reti32(rt, gx_getpix16(surf, x * 65535, y * 65535, 1));
		return noError;
	}
	return executionAborted;
}
//#}#endregion

//#{#region Mesh & Camera
typedef enum {
	meshOpSetPos,		// void meshPos(int Index, vec3f Value)
	//~ meshOpGetPos,		// vec3f meshPos(int Index)
	meshOpSetNrm,		// void meshNrm(int Index, vec3f Value)
	//~ meshOpGetNrm,		// vec3f meshNrm(int Index)
	meshOpSetTex,		// void meshTex(int Index, vec2f Value)
	//~ meshOpGetTex,		// vec2f meshTex(int Index)
	//~ meshOpSetTri,		// int meshSetTri(int t, int p1, int p2, int p3)
	meshOpAddTri,		// int meshAddTri(int p1, int p2, int p3)
	meshOpAddQuad,		// int meshAddTri(int p1, int p2, int p3, int p4)
	//~ meshOpSetSeg,		// int meshSetSeg(int t, int p1, int p2)
	//~ meshOpAddSeg,		// int meshAddSeg(int p1, int p2)

	//~ meshOpGetFlags,		// int meshFlags()
	//~ meshOpSetFlags,		// int meshFlags(int Value)
	meshOpInit,			// int meshInit(int Capacity)
	meshOpTexture,		// int meshTexture(gxSurf image)

	meshOpRead,			// int meshRead(string file)
	meshOpSave,			// int meshSave(string file)

	meshOpCenter,		// void Center()
	meshOpNormalize,		// void Normalize()

	meshOpInfo,			// void meshInfo()
	meshOphasNrm,		// bool hasNormals
} meshFunc;
static vmError meshCall(libcContext rt) {
	meshFunc fun = (meshFunc)rt->data;
	if (fun == meshOpSetPos) {		// void meshPos(int Index, double x, double y, double z);
		double pos[3];
		int idx = popi32(rt);
		pos[0] = popf64(rt);
		pos[1] = popf64(rt);
		pos[2] = popf64(rt);
		setvtxDV(&msh, idx, pos, NULL, NULL, 0xff00ff);
		return noError;
	}
	if (fun == meshOpSetNrm) {		// void meshNrm(int Index, double x, double y, double z);
		double nrm[3];
		int idx = popi32(rt);
		nrm[0] = popf64(rt);
		nrm[1] = popf64(rt);
		nrm[2] = popf64(rt);
		setvtxDV(&msh, idx, NULL, nrm, NULL, 0xff00ff);
		return noError;
	}
	//~ case meshOpGetPos: {		// vec3f meshPos(int Index)
	//~ case meshOpGetNrm: {		// vec3f meshNrm(int Index)
	if (fun == meshOpSetTex) {			// void meshTex(int Index, double s, double t)
		double pos[2];
		int idx = popi32(rt);
		pos[0] = popf64(rt);
		pos[1] = popf64(rt);
		setvtxDV(&msh, idx, NULL, NULL, pos, 0xff00ff);
		return noError;
	}
	//~ case meshOpGetTex: {		// vec2f meshTex(int Index)
	//~ case meshOpSetTri: {		// int meshSetTri(int t, int p1, int p2, int p3)
	if (fun == meshOpAddTri) {
		int p1 = popi32(rt);
		int p2 = popi32(rt);
		int p3 = popi32(rt);
		addtri(&msh, p1, p2, p3);
		return noError;
	}
	if (fun == meshOpAddQuad) {
		int p1 = popi32(rt);
		int p2 = popi32(rt);
		int p3 = popi32(rt);
		int p4 = popi32(rt);
		//~ addtri(&msh, p1, p2, p3);
		//~ addtri(&msh, p3, p4, p1);
		addquad(&msh, p1, p2, p3, p4);
		return noError;
	}
	//~ case meshOpSetSeg: {		// int meshSetSeg(int t, int p1, int p2)
	//~ case meshOpAddSeg: {		// int meshAddSeg(int p1, int p2)
	//~ case meshOpGetFlags: {		// int meshFlags()
	//~ case meshOpSetFlags: {		// int meshFlags(int Value)

	if (fun == meshOpInfo) {				// void meshInfo();
		meshInfo(&msh);
		return noError;
	}
	if (fun == meshOphasNrm) {				// bool hasNormals;
		reti32(rt, msh.hasNrm);
		return noError;
	}
	if (fun == meshOpInit) {			// int meshInit(int Capacity);
		reti32(rt, initMesh(&msh, popi32(rt)));
		return noError;
	}
	if (fun == meshOpTexture) {
		if (msh.freeTex && msh.map) {
			gx_destroySurf(msh.map);
		}
		msh.map = getSurf(pophnd(rt));
		msh.freeTex = 0;
		return noError;
	}
	if (fun == meshOpRead) {			// int meshRead(string file);
		char* mesh = popstr(rt);
		char* tex = popstr(rt);
		reti32(rt, readorevalMesh(&msh, mesh, 0, tex, 0));
		return noError;
	}
	if (fun == meshOpSave) {			// int meshSave(string file);
		reti32(rt, saveMesh(&msh, popstr(rt)));
		return noError;
	}
	if (fun == meshOpCenter) {		// void Center(float size);
		centMesh(&msh, popf32(rt));
		return noError;
	}
	if (fun == meshOpNormalize) {		// void Normalize(double epsilon);
		normMesh(&msh, popf32(rt));
		return noError;
	}

	return executionAborted;
}

typedef enum {
	camGetPos,
	camSetPos,
	camGetUp,
	camSetUp,
	camGetRight,
	camSetRight,
	camGetForward,
	camSetForward,
	camOpMove,
	camOpRotate,
	camOpLookAt,
} camFunc;
static vmError camCall(libcContext rt) {
	switch ((camFunc)rt->data) {
		case camOpMove: {
			vector dir = popref(rt);
			float32_t cnt = popf32(rt);
			cammov(cam, dir, cnt);
			return noError;
		}
		case camOpRotate: {
			vector dir = popref(rt);
			vector orb = popref(rt);
			float32_t cnt = popf32(rt);
			camrot(cam, dir, orb, cnt);
			return noError;
		}

		case camOpLookAt: {		// cam.lookAt(vec4f eye, vec4f at, vec4f up)
			vector camPos = poparg(rt, NULL, sizeof(struct vector));	// eye
			vector camAt = poparg(rt, NULL, sizeof(struct vector));	// target
			vector camUp = poparg(rt, NULL, sizeof(struct vector));	// up
			camset(cam, camPos, camAt, camUp);
			return noError;
		}

		//#ifndef _MSC_VER
		case camGetPos: {
			retset(rt, &cam->pos, sizeof(struct vector));
			return noError;
		}
		case camSetPos: {
			poparg(rt, &cam->pos, sizeof(struct vector));
			return noError;
		}

		case camGetUp: {
			retset(rt, &cam->dirU, sizeof(struct vector));
			return noError;
		}
		case camSetUp: {
			poparg(rt, &cam->dirU, sizeof(struct vector));
			return noError;
		}

		case camGetRight: {
			retset(rt, &cam->dirR, sizeof(struct vector));
			return noError;
		}
		case camSetRight: {
			poparg(rt, &cam->dirR, sizeof(struct vector));
			return noError;
		}

		case camGetForward: {
			retset(rt, &cam->dirF, sizeof(struct vector));
			return noError;
		}
		//#endif

		case camSetForward: {
			poparg(rt, &cam->dirF, sizeof(struct vector));
			return noError;
		}
	}
	return executionAborted;
}

typedef enum {
	objOpMove,
	objOpRotate,
	objOpSelected,
}objFunc;
static vmError objCall(libcContext rt) {

	switch((objFunc)rt->data) {
		case objOpSelected: {
			reti32(rt, getobjvec(0) != NULL);
			return noError;
		}

		case objOpMove: {
			vector P;		// position
			float32_t mdx = popf32(rt);
			float32_t mdy = popf32(rt);
			if ((P = getobjvec(0))) {
				struct matrix tmp, tmp1, tmp2;
				matldT(&tmp1, &cam->dirU, mdy);
				matldT(&tmp2, &cam->dirR, mdx);
				matmul(&tmp, &tmp1, &tmp2);
				matvph(P, &tmp, P);
			}
			return noError;
		}
		case objOpRotate: {
			vector N;		// normal vector
			float32_t mdx = popf32(rt);
			float32_t mdy = popf32(rt);
			if ((N = getobjvec(1))) {
				struct matrix tmp, tmp1, tmp2;
				matldR(&tmp1, &cam->dirU, mdx);
				matldR(&tmp2, &cam->dirR, mdy);
				matmul(&tmp, &tmp1, &tmp2);
				vecnrm(N, matvp3(N, &tmp, N));
			}
			return noError;
		}
	}
	return executionAborted;
}
//#}#endregion

typedef enum {
	miscSetExitLoop,
	miscOpFlipScreen,
	miscOpSetCbMouse,
	miscOpSetCbKeyboard,
	miscOpSetCbRender,
} miscFunc;
static vmError miscCall(libcContext rt) {
	switch ((miscFunc)rt->data) {
		case miscSetExitLoop: {
			draw = 0;
			return noError;
		}

		case miscOpFlipScreen: {
			if (popi32(rt)) {
				if (flip)
					flip(&offs, 0);
			}
			else {
				draw |= post_swap;
			}
			return noError;
		}

		case miscOpSetCbMouse: {
			mouseCallBack = popsym(rt);
			return noError;
		}

		case miscOpSetCbKeyboard: {
			keyboardCallBack = popsym(rt);
			return noError;
		}

		case miscOpSetCbRender: {
			renderMethod = popsym(rt);
			return noError;
		}

	}
	return executionAborted;
}

struct {
	vmError (*fun)(libcContext);
	void *data;
	char *def;
}
Gui[] = {
	{miscCall, (void*)miscSetExitLoop,		"void exitLoop();"},
	{miscCall, (void*)miscOpFlipScreen,	"void Repaint(bool forceNow);"},
	{miscCall, (void*)miscOpSetCbMouse,	"void setMouseHandler(void handler(int btn, int x, int y));"},
	{miscCall, (void*)miscOpSetCbKeyboard,	"void setKeyboardHandler(void handler(int btn, int ext));"},
	{miscCall, (void*)miscOpSetCbRender,	"void setDrawCallback(void callback());"},
},
Surf[] = {
	{surfCall, (void*)surfOpGetOffScreen,   "gxSurf offScreen;"},
	{surfCall, (void*)surfOpGetWidth,       "int width(gxSurf dst);"},
	{surfCall, (void*)surfOpGetHeight,	"int height(gxSurf dst);"},
	{surfCall, (void*)surfOpGetDepth,	"int depth(gxSurf dst);"},
	{surfCall, (void*)surfOpClipRect,		"bool clipRect(gxSurf surf, gxRect &rect);"},

	{surfGetPixel, NULL,			"int getPixel(gxSurf dst, int x, int y);"},
	{surfGetPixfp, NULL,			"int getPixel(gxSurf dst, double x, double y);"},
	{surfSetPixel, NULL,			"void setPixel(gxSurf dst, int x, int y, int col);"},
	//~ {surfCall, (void*)surfOpGetPixel,		"int getPixel(gxSurf dst, int x, int y);"},
	//~ {surfCall, (void*)surfOpGetPixfp,		"int getPixel(gxSurf dst, double x, double y);"},
	//~ {surfCall, (void*)surfOpSetPixel,		"void setPixel(gxSurf dst, int x, int y, int col);"},

	{surfCall, (void*)surfOpDrawLine,		"void drawLine(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, (void*)surfOpDrawBez2,		"void drawBezier(gxSurf dst, int x0, int y0, int x1, int y1, int x2, int y2, int col);"},
	{surfCall, (void*)surfOpDrawBez3,		"void drawBezier(gxSurf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int col);"},
	{surfCall, (void*)surfOpDrawRect,		"void drawRect(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, (void*)surfOpFillRect,		"void fillRect(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, (void*)surfOpDrawOval,		"void drawOval(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, (void*)surfOpFillOval,		"void fillOval(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, (void*)surfOpDrawText,		"void drawText(gxSurf dst, int x0, int y0, string text, int col);"},

	{surfCall, (void*)surfOpCopySurf,		"gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi);"},
	{surfCall, (void*)surfOpZoomSurf,		"gxSurf zoomSurf(gxSurf dst, gxRect &rect, gxSurf src, gxRect &roi, int mode);"},

	{surfCall, (void*)surfOpCmatSurf,		"gxSurf cmatSurf(gxSurf dst, gxRect &roi, mat4f &mat);"},
	{surfCall, (void*)surfOpClutSurf,		"gxSurf clutSurf(gxSurf dst, gxRect &roi, gxClut &lut);"},
	{surfCall, (void*)surfOpGradSurf,		"gxSurf gradSurf(gxSurf dst, gxRect &roi, gxClut &lut, int mode, bool repeat);"},
	{surfCall, (void*)surfOpBlurSurf,		"gxSurf blurSurf(gxSurf dst, gxRect &roi, int radius);"},
	{surfCall, (void*)surfOpOilPaint,		"gxSurf oilPaintSurf(gxSurf dst, gxRect &roi, int radius, int intensity);"},

	{surfCall, (void*)surfOpBmpRead,		"gxSurf readBmp(gxSurf dst, string fileName);"},
	{surfCall, (void*)surfOpJpgRead,		"gxSurf readJpg(gxSurf dst, string fileName);"},
	{surfCall, (void*)surfOpPngRead,		"gxSurf readPng(gxSurf dst, string fileName);"},
	{surfCall, (void*)surfOpBmpWrite,		"void bmpWrite(gxSurf src, string fileName);"},

	{surfCall, (void*)surfOpNewSurf,		"gxSurf newSurf(int width, int height);"},
	{surfCall, (void*)surfOpDelSurf,		"void delSurf(gxSurf surf);"},

	{surfCall, (void*)surfOpEvalFpCB,		"gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(float x, float y));"},
	{surfCall, (void*)surfOpFillFpCB,		"gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));"},
	{surfCall, (void*)surfOpCopyFpCB,		"gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, float alpha, vec4f callBack(vec4f dst, vec4f src));"},

	{surfCall, (void*)surfOpFillPxCB,		"gxSurf fillSurfrgb(gxSurf dst, gxRect &roi, int callBack(int col));"},
	{surfCall, (void*)surfOpEvalPxCB,		"gxSurf evalSurfrgb(gxSurf dst, gxRect &roi, int callBack(int x, int y));"},
	{surfCall, (void*)surfOpCopyPxCB,		"gxSurf copySurfrgb(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, int callBack(int dst, int src));"},

},
// 3d
Mesh[] = {
	{meshCall, (void*)meshOpSetPos,		"void Pos(int Index, double x, double y, double z);"},
	{meshCall, (void*)meshOpSetNrm,		"void Nrm(int Index, double x, double y, double z);"},
	{meshCall, (void*)meshOpSetTex,		"void Tex(int Index, double s, double t);"},
	//~ {meshCall, (void*)meshOpGetPos,		"vec4f Pos(int Index);"},
	//~ {meshCall, (void*)meshOpGetNrm,		"vec4f Nrm(int Index);"},
	//~ {meshCall, (void*)meshOpGetTex,		"vec2f Tex(int Index);"},
	//~ {meshCall, (void*)meshOpSetTri,		"int SetTri(int t, int p1, int p2, int p3);"},
	{meshCall, (void*)meshOpAddTri,		"void AddFace(int p1, int p2, int p3);"},
	{meshCall, (void*)meshOpAddQuad,		"void AddFace(int p1, int p2, int p3, int p4);"},
	//~ {meshCall, (void*)meshOpSetSeg,		"int SetSeg(int t, int p1, int p2);"},
	//~ {meshCall, (void*)meshOpAddSeg,		"int AddSeg(int p1, int p2);"},
	//~ {meshCall, (void*)meshOpGetFlags,		"int Flags;"},
	//~ {meshCall, (void*)meshOpSetFlags,		"int Flags(int Value);"},
	//~ {meshCall, (void*)meshOpGetTex,		"gxSurf Texture;"},
	{meshCall, (void*)meshOpTexture,		"int Texture(gxSurf image);"},
	{meshCall, (void*)meshOpInit,			"int Init(int Capacity);"},
	{meshCall, (void*)meshOpRead,			"int Read(string file, string texture);"},
	{meshCall, (void*)meshOpSave,			"int Save(string file);"},
	{meshCall, (void*)meshOpCenter,		"void Center(float32 Resize);"},
	{meshCall, (void*)meshOpNormalize,	"void Normalize(float32 Epsilon);"},
	{meshCall, (void*)meshOpInfo,			"void Info();"},
	{meshCall, (void*)meshOphasNrm,		"bool hasNormals;"},
	//~ */
},
Cam[] = {
	{camCall, (void*)camGetPos,			"vec4f Pos;"},
	{camCall, (void*)camGetUp,			"vec4f Up;"},
	{camCall, (void*)camGetRight,		"vec4f Right;"},
	{camCall, (void*)camGetForward,		"vec4f Forward;"},

	{camCall, (void*)camSetPos,			"void Pos(vec4f v);"},
	{camCall, (void*)camSetUp,			"void Up(vec4f v);"},
	{camCall, (void*)camSetRight,		"void Right(vec4f v);"},
	{camCall, (void*)camSetForward,		"void Forward(vec4f v);"},

	{camCall, (void*)camOpMove,			"void Move(vec4f &direction, float32 cnt);"},
	{camCall, (void*)camOpRotate,		"void Rotate(vec4f &direction, vec4f &orbit, float32 cnt);"},
	{camCall, (void*)camOpLookAt,		"void LookAt(vec4f eye, vec4f at, vec4f up);"},
	// */
},
Obj[] = {
	//~ {objCall, (void*)objGetPos,			"vec4f Pos;"},
	//~ {objCall, (void*)objSetPos,			"void Pos(vec4f v);"},
	//~ {objCall, (void*)objGetNrm,			"vec4f Nrm;"},
	//~ {objCall, (void*)objSetNrm,			"void Nrm(vec4f v);"},
	{objCall, (void*)objOpMove,			"void Move(float32 dx, float32 dy);"},
	{objCall, (void*)objOpRotate,			"void Rotate(float32 dx, float32 dy);"},
	{objCall, (void*)objOpSelected,		"bool object;"},
	// */
};

char* strnesc(char *dst, int max, char* src) {
	int i = 0;

	if (dst == NULL || src == NULL) {
		return NULL;
	}

	while (*src && i < max) {
		int chr = *src++;

		switch (chr) {
			default:
				dst[i++] = (char) chr;
				break;

			case '"':
				dst[i++] = '\\';
				dst[i++] = '"';
				break;
			case '\\':
				dst[i++] = '\\';
				dst[i++] = '\\';
				break;
		}
	}
	dst[i] = 0;
	return dst;
}

#ifdef _MSC_VER
#define snprintf(__DST, __MAX, __FMT, ...)  sprintf_s(__DST, __MAX, __FMT, ##__VA_ARGS__)
#endif

symn ccSymValInt(ccContext cc, char *name, symn cls, int *result) {
	struct astNode ast;
	symn sym = ccLookupSym(cc, cls, name);
	if (!sym || !eval(&ast, sym->init)) {
		return NULL;
	}

	if (result != NULL) {
		*result = (int) constint(&ast);
	}

	return sym;
}

symn ccSymValFlt(ccContext cc, char *name, symn cls, double* result) {
	struct astNode ast;
	symn sym = ccLookupSym(cc, cls, name);
	if (!sym || !eval(&ast, sym->init)) {
		return NULL;
	}

	if (result != NULL) {
		*result = constflt(&ast);
	}
	return sym;
}

static void dumpSciTEApi(userContext extra, symn sym) {
	FILE *out = stdout;
	if (sym == NULL) {
		// last symbol
		return;
	}
	if (sym->prms != NULL && sym->call) {
		fputfmt(out, "%-T", sym);
	}
	else {
		fputfmt(out, "%T", sym);
	}
	fputfmt(out, "\n");
	(void)extra;
}

int ccCompile(char *src, int argc, char* argv[], int dbg(dbgContext, vmError, size_t, void*, void*)) {
	symn cls = NULL;
	const int stdwl = 0;
	const int srcwl = 9;
	int err = 0;
	size_t i;

	if (rt == NULL)
		return -29;

	if (ccLog && logfile(rt, ccLog, 0) != 0) {
		gx_debug("can not open file `%s`\n", ccLog);
		return -2;
	}

	if (!ccInit(rt, install_def, NULL)) {
		gx_debug("Internal error\n");
		return -1;
	}

	// install standard library calls
	err = err || !ccAddLib(rt, ccLibStdc, stdwl, ccStd);
	err = err || !ccDefType(rt, "gxSurf", sizeof(gx_Surf), 0);
	err = err || !ccAddUnit(rt, stdwl, __FILE__, __LINE__ + 1,
		"struct gxRect {\n"
			"int32 x;//%x(%d)\n"
			"int32 y;//%y(%d)\n"
			"int32 w;//%width(%d)\n"
			"int32 h;//%height(%d)\n"
		"}\n"
		"define gxRect(int w, int h) = gxRect(0, 0, w, h);\n"
		"\n"
		"\n"
		"// Color Look Up Table (Palette) structure\n"
		"struct gxClut:1 {			// Color Look Up Table (Palette) structure\n"
			"int16	count;\n"
			"uint8	flags;\n"
			"uint8	trans;\n"
			"int32	data[256];\n"
		"}\n"
	);

	for (i = 0; i < sizeof(Surf) / sizeof(*Surf); i += 1) {
		symn libc = ccDefCall(rt, Surf[i].fun, Surf[i].data, Surf[i].def);
		if (libc == NULL) {
			return -1;
		}
	}

	if ((cls = ccBegin(rt, "Gui"))) {
		for (i = 0; i < sizeof(Gui) / sizeof(*Gui); i += 1) {
			symn libc = ccDefCall(rt, Gui[i].fun, Gui[i].data, Gui[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		err = err || !ccAddUnit(rt, stdwl, __FILE__, __LINE__ + 1,
			"define Repaint() = Repaint(false);\n"
		);
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "mesh"))) {
		for (i = 0; i < sizeof(Mesh) / sizeof(*Mesh); i += 1) {
			symn libc = ccDefCall(rt, Mesh[i].fun, Mesh[i].data, Mesh[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "camera"))) {
		for (i = 0; i < sizeof(Cam) / sizeof(*Cam); i += 1) {
			symn libc = ccDefCall(rt, Cam[i].fun, Cam[i].data, Cam[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "selection"))) {
		for (i = 0; i < sizeof(Obj) / sizeof(*Obj); i += 1) {
			symn libc = ccDefCall(rt, Obj[i].fun, Obj[i].data, Obj[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "Gradient"))) {
		err = err || !ccDefInt(rt, "Linear", gradient_lin);
		err = err || !ccDefInt(rt, "Radial", gradient_rad);
		err = err || !ccDefInt(rt, "Square", gradient_sqr);
		err = err || !ccDefInt(rt, "Conical", gradient_con);
		err = err || !ccDefInt(rt, "Spiral", gradient_spr);
		ccEnd(rt, cls, ATTR_stat);
	}

	// it is not an error if library name is not set(NULL)
	if (err == 0 && ccGfx) {
		err = !ccAddUnit(rt, stdwl, ccGfx, 1, NULL);
	}

	if (src != NULL) {
		char tmp[65535];

		if ((cls = ccBegin(rt, "properties"))) {

			symn subcls = NULL;
			if ((subcls = ccBegin(rt, "screen"))) {
				err = err || !ccDefInt(rt, "width", resx);
				err = err || !ccDefInt(rt, "height", resy);
				ccEnd(rt, subcls, ATTR_stat);
			}

			if (!err && obj != NULL) {
				err = !ccDefStr(rt, "object", strnesc(tmp, sizeof(tmp), obj));
			}
			if (!err && tex != NULL) {
				err = !ccDefStr(rt, "texture", strnesc(tmp, sizeof(tmp), tex));
			}
			ccEnd(rt, cls, ATTR_stat);
		}

		/* TODO: add args
		snprintf(tmp, sizeof(tmp), "string args[%d];", argc);
		err = err || !ccDefCode(rt, stdwl, __FILE__, __LINE__ , tmp);
		for (i = 0; i < argc; i += 1) {
			snprintf(tmp, sizeof(tmp), "args[%d] = \"%s\";", i, strnesc(tmpf, sizeof(tmpf), argv[i]));
			err = err || !ccDefCode(rt, stdwl, __FILE__, __LINE__ , tmp);
		}// */
		//~ snprintf(tmp, sizeof(tmp), "string filename = \"%s\";", i, strnesc(tmpf, sizeof(tmpf), argv[i]));
		//~ err = err || !ccDefCode(rt, stdwl, __FILE__, __LINE__ , tmp);
		if (argc == 2) {
			//~ err = err || !ccDefStr(rt, "script", strnesc(tmp, sizeof(tmp), argv[0]));
			err = err || !ccDefStr(rt, "filename", strnesc(tmp, sizeof(tmp), argv[1]));
		}
		err = err || !ccAddUnit(rt, srcwl, src, 1, NULL);
	}
	else {
		logFILE(rt, stdout);
		ccAddUnit(rt, stdwl, __FILE__, __LINE__ , "");
		gencode(rt, 0);
		dumpApi(rt, NULL, dumpSciTEApi);
		return 0;
	}

	// TODO: bug generating faster code
	rt->fastAssign = 0;

	if (err || !gencode(rt, dbg != NULL)) {
		if (ccLog) {
			gx_debug("error compiling(%d), see ligfile: `%s`", err, ccLog);
			logfile(rt, NULL, 0);
		}
		err = -3;
	}

	if (err == 0) {
		ccContext cc = rt->cc;

		draw = 0;

		if ((cls = ccLookupSym(cc, NULL, "Window"))) {
			if (!ccSymValInt(cc, "resx", cls, &resx)) {
				//~ gx_debug("using default value for resx: %d", resx);
			}
			if (!ccSymValInt(cc, "resy", cls, &resy)) {
				//~ gx_debug("using default value for resy: %d", resy);
			}
			if (!ccSymValInt(cc, "draw", cls, &draw)) {
				//~ gx_debug("using default value for renderType: 0x%08x", draw);
			}
		}

		if (ccDmp && logfile(rt, ccDmp, 0) == 0) {
			//~ "\ntags:\n"
			//~ dump(rt, dump_sym | 0xf2, NULL);

			//~ "\ncode:\n"
			//~ dump(rt, dump_ast | 0xff, NULL);

			//~ "\ndasm:\n"
			//~ dump(rt, dump_asm | 0xf9, NULL);
		}

		if (rt->dbg != NULL) {
			//~ rt->dbg->extra = (void*)rt;
			rt->dbg->debug = dbg;
		}
	}

	logFILE(rt, stdout);

	return err;
}

#if 1 // eval mesh with scripting support

static inline double lerp(double a, double b, double t) { return a + t * (b - a); }

static inline double  dv3dot(double lhs[3], double rhs[3]) {
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}
static inline double* dv3cpy(double dst[3], double src[3]) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	return dst;
}
static inline double* dv3sca(double dst[3], double src[3], double rhs) {
	dst[0] = src[0] * rhs;
	dst[1] = src[1] * rhs;
	dst[2] = src[2] * rhs;
	return dst;
}
static inline double* dv3crs(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	return dst;
}
static inline double* dv3nrm(double dst[3], double src[3]) {
	double len = dv3dot(src, src);
	if (len > 0) {
		dv3sca(dst, src, 1. / sqrt(len));
	}
	return dst;
}

typedef struct {
	double s, smin, smax;
	double t, tmin, tmax;
	int isPos, isNrm;
	double pos[3], nrm[3];
} userDataRec, *userData;

static vmError getS(libcContext rt) {
	userData d = rt->extra;
	retf64(rt, lerp(d->smin, d->smax, d->s));
	return noError;
}
static vmError getT(libcContext rt) {
	userData d = rt->extra;
	retf64(rt, lerp(d->tmin, d->tmax, d->t));
	return noError;
}
static vmError setPos(libcContext rt) {
	userData d = rt->extra;
	d->pos[0] = argf64(rt, 8 * 0);
	d->pos[1] = argf64(rt, 8 * 1);
	d->pos[2] = argf64(rt, 8 * 2);
	d->isPos = 1;
	return noError;
}
static vmError setNrm(libcContext rt) {
	userData d = rt->extra;
	d->nrm[0] = argf64(rt, 8 * 0);
	d->nrm[1] = argf64(rt, 8 * 1);
	d->nrm[2] = argf64(rt, 8 * 2);
	d->isNrm = 1;
	return noError;
}

static vmError f64abs(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, fabs(x));
	return noError;
}
static vmError f64sin(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sin(x));
	return noError;
}
static vmError f64cos(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, cos(x));
	return noError;
}
static vmError f64tan(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, tan(x));
	return noError;
}
static vmError f64log(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, log(x));
	return noError;
}
static vmError f64exp(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, exp(x));
	return noError;
}
static vmError f64pow(libcContext rt) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, pow(x, y));
	return noError;
}
static vmError f64sqrt(libcContext rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sqrt(x));
	return noError;
}
static vmError f64atan2(libcContext rt) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, atan2(x, y));
	return noError;
}

int evalMesh(mesh msh, int sdiv, int tdiv, char *src, char *file, int line) {

	char mem[64 << 10];		// 128K memory
	size_t stackSize = sizeof(mem) / 2;
	rtContext rt = rtInit(mem, sizeof(mem));
	ccContext cc = ccInit(rt, installBase, NULL);

	userDataRec ud;
	const int warnlevel = 2;

	char *logf = NULL;//"dump.evalMesh.txt";

	int i, j, err = 0;

	double s, t, ds, dt;	// 0 .. 1

	// pointers, variants and emit are not needed.
	if (cc == NULL) {
		gx_debug("Internal error\n");
		return -1;
	}

	err = err || !ccDefCall(rt, getS, NULL,     "float64 gets();");
	err = err || !ccDefCall(rt, getT, NULL,     "float64 gett();");
	err = err || !ccDefCall(rt, setPos, NULL,   "void setPos(float64 x, float64 y, float64 z);");
	err = err || !ccDefCall(rt, setNrm, NULL,   "void setNrm(float64 x, float64 y, float64 z);");
	err = err || !ccDefCall(rt, f64abs, NULL,   "float64 abs(float64 x);");
	err = err || !ccDefCall(rt, f64sin, NULL,   "float64 sin(float64 x);");
	err = err || !ccDefCall(rt, f64cos, NULL,   "float64 cos(float64 x);");
	err = err || !ccDefCall(rt, f64tan, NULL,   "float64 tan(float64 x);");
	err = err || !ccDefCall(rt, f64log, NULL,   "float64 log(float64 x);");
	err = err || !ccDefCall(rt, f64exp, NULL,   "float64 exp(float64 x);");
	err = err || !ccDefCall(rt, f64sqrt, NULL,  "float64 sqrt(float64 x);");
	err = err || !ccDefCall(rt, f64atan2, NULL, "float64 atan(float64 x, float64 y);");
	err = err || !ccDefCall(rt, f64pow, NULL,   "float64 pow(float64 x, float64 y);");
	err = err || !ccDefFlt(rt, "pi", 3.14159265358979323846264338327950288419716939937510582097494459);
	err = err || !ccDefFlt(rt, "e",  2.71828182845904523536028747135266249775724709369995957496696763);

	err = err || !ccAddUnit(rt, 0, __FILE__, __LINE__ + 1,
		"const double s = gets();\n"
		"const double t = gett();\n"
		"float64 x = s;\n"
		"float64 y = t;\n"
		"float64 z = float64(0);\n"
	);
	err = err || !ccAddUnit(rt, warnlevel, file, line, src);
	err = err || !ccAddUnit(rt, warnlevel, __FILE__, __LINE__, "setPos(x, y, z);\n");
	// */

	// optimize on max level, and generate global variables on stack
	rt->fastInstr = 1;
	rt->fastAssign = 1;
	rt->genGlobals = 0;
	if (err || !gencode(rt, 0)) {
		gx_debug("error compiling(%d), see `%s`", err, logf);
		logfile(rt, NULL, 0);
		return -3;
	}

	ud.smin = ud.tmin = 0;
	ud.smax = ud.tmax = 1;
	if (ccSymValInt(cc, "division", NULL, &tdiv)) {
		sdiv = tdiv;
	}
	ccSymValFlt(cc, "smin", NULL, &ud.smin);
	ccSymValFlt(cc, "smax", NULL, &ud.smax);
	ccSymValInt(cc, "sdiv", NULL, &sdiv);

	ccSymValFlt(cc, "tmin", NULL, &ud.tmin);
	ccSymValFlt(cc, "tmax", NULL, &ud.tmax);
	ccSymValInt(cc, "tdiv", NULL, &tdiv);

	// close log
	logfile(rt, NULL, 0);

	ds = 1. / (sdiv - 1);
	dt = 1. / (tdiv - 1);
	msh->hasTex = msh->hasNrm = 1;
	msh->tricnt = msh->vtxcnt = 0;

	for (t = 0, j = 0; j < tdiv; t += dt, ++j) {
		for (s = 0, i = 0; i < sdiv; s += ds, ++i) {
			double pos[3], nrm[3], tex[2] = {t, s};

			ud.s = s;
			ud.t = t;
			ud.isNrm = 0;
			if (execute(rt, stackSize, &ud) != 0) {
				gx_debug("error");
				return -4;
			}
			dv3cpy(pos, ud.pos);

			if (ud.isNrm) {
				dv3nrm(nrm, ud.nrm);
			}
			else {
				double nds[3], ndt[3];
				ud.s = s + epsilon;
				ud.t = t;
				if (execute(rt, stackSize, &ud) != 0) {
					gx_debug("error");
					return -5;
				}
				dv3cpy(nds, ud.pos);

				ud.s = s;
				ud.t = t + epsilon;
				if (execute(rt, stackSize, &ud) != 0) {
					gx_debug("error");
					return -6;
				}
				dv3cpy(ndt, ud.pos);

				nds[0] = (pos[0] - nds[0]) / epsilon;
				nds[1] = (pos[1] - nds[1]) / epsilon;
				nds[2] = (pos[2] - nds[2]) / epsilon;
				ndt[0] = (pos[0] - ndt[0]) / epsilon;
				ndt[1] = (pos[1] - ndt[1]) / epsilon;
				ndt[2] = (pos[2] - ndt[2]) / epsilon;
				dv3nrm(nrm, dv3crs(nrm, nds, ndt));
			}

			addvtxDV(msh, pos, nrm, tex);
		}
	}

	for (j = 0; j < tdiv - 1; ++j) {
		int l1 = j * sdiv;
		int l2 = l1 + sdiv;
		for (i = 0; i < sdiv - 1; ++i) {
			int v1 = l1 + i, v2 = v1 + 1;
			int v4 = l2 + i, v3 = v4 + 1;
			addquad(msh, v1, v2, v3, v4);
		}
	}
	return 0;
}
#endif
