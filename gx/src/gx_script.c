// scripting support
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"
#include "g3_draw.h"
#include "drv_gui.h"

#include "internal.h"
#include <assert.h>

extern double epsilon;

//#{ old style arguments
static inline void* poparg(libcContext rt, void *result, int size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, rt->argv, size);
	}
	else {
		result = rt->argv;
	}
	*(char**)&rt->argv += padded(size, 4);
	return result;
}

#define poparg(__ARGV, __TYPE) (((__TYPE*)(*(char**)&((__ARGV)->argv) += ((sizeof(__TYPE) + 3) & ~3)))[-1])
static inline void* pophnd(libcContext rt) { return poparg(rt, void*); }
static inline int32_t popi32(libcContext rt) { return poparg(rt, int32_t); }
static inline int64_t popi64(libcContext rt) { return poparg(rt, int64_t); }
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
typedef uint32_t gxSurfHnd;
static struct gx_Surf surfaces[256];
static int surfCount = sizeof(surfaces) / sizeof(*surfaces);

static gxSurfHnd newSurf(int w, int h) {
	gxSurfHnd hnd = 0;
	while (hnd < surfCount) {
		if (!surfaces[hnd].depth) {
			surfaces[hnd].width = w;
			surfaces[hnd].height = h;
			surfaces[hnd].depth = 32;
			surfaces[hnd].scanLen = 0;
			if (gx_initSurf(&surfaces[hnd], 0, 0, 0) == 0) {
				//~ if (cpy != NULL) gx_zoomsurf(&surfaces[hnd], NULL, cpy, NULL, 0);
				return hnd + 1;
			}
			break;
		}
		hnd += 1;
	}
	return 0;
}

static void delSurf(gxSurfHnd hnd) {
	if (hnd == -1)
		return;

	if (!hnd || hnd > surfCount)
		return;

	if (surfaces[hnd - 1].depth) {
		gx_doneSurf(&surfaces[hnd - 1]);
		surfaces[hnd - 1].depth = 0;
	}
}

static gx_Surf getSurf(gxSurfHnd hnd) {
	if (hnd == -1)
		return &offs;

	if (!hnd || hnd > surfCount)
		return NULL;

	if (surfaces[hnd - 1].depth) {
		return &surfaces[hnd - 1];
	}
	gx_debug("getSurf(%d): null", hnd);
	return NULL;
}

void surfDone() {
	gxSurfHnd hnd = 0;
	while (hnd < surfCount) {
		if (surfaces[hnd].depth) {
			gx_doneSurf(&surfaces[hnd]);
		}
		hnd += 1;
	}
}

static vmError surfSetPixel(libcContext rt) {
	gx_Surf surf;
	if ((surf = getSurf(popi32(rt)))) {
		unsigned rowy;
		int x = popi32(rt);
		int y = popi32(rt);
		int col = popi32(rt);
		//~ /* this way is faster
		if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
			return noError;
		}

		rowy = y * surf->scanLen;
		if (surf->depth == 32) {
			uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
			ptr[x] = col;
			return noError;
		}
		else if (surf->depth == 8) {
			uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
			ptr[x] = col;
			return noError;
		}// */

		//~ gx_setpixel(surf, x, y, col);
		//~ return noError;
	}
	return executionAborted;
}
static vmError surfGetPixel(libcContext rt) {
	gx_Surf surf;
	if ((surf = getSurf(popi32(rt)))) {
		unsigned rowy;
		int x = popi32(rt);
		int y = popi32(rt);
		//~ /* but this way is faster
		if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
			reti32(rt, 0);
			return noError;
		}
		rowy = y * surf->scanLen;
		if (surf->depth == 32) {
			uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
			reti32(rt, ptr[x]);
			return noError;
		}
		else if (surf->depth == 8) {
			uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
			reti32(rt, ptr[x]);
			return noError;
		}// */
		//~ reti32(rt, gx_getpixel(surf, x, y));
		//~ return noError;
	}
	return executionAborted;
}
static vmError surfGetPixfp(libcContext rt) {
	gx_Surf surf;
	if ((surf = getSurf(popi32(rt)))) {
		double x = popf64(rt);
		double y = popf64(rt);
		reti32(rt, gx_getpix16(surf, x * 65535, y * 65535, 1));
		return noError;
	}
	return executionAborted;
}
static vmError surfCall(libcContext rt) {
	gx_Surf surf;
	switch ((surfFunc)rt->data) {
		case surfOpGetWidth:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				reti32(rt, surf->width);
				return noError;
			}
			break;
		case surfOpGetHeight:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				reti32(rt, surf->height);
				return noError;
			}
			break;
		case surfOpGetDepth:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				reti32(rt, surf->depth);
				return noError;
			}
			break;

		case surfOpClipRect:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				gx_Rect rect = popref(rt);
				void *ptr = gx_cliprect(surf, rect);
				reti32(rt, ptr != NULL);
				return noError;
			}
			break;

		/*case surfOpGetPixfp: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				double x = popf64(rt);
				double y = popf64(rt);
				reti32(rt, gx_getpix16(surf, x * 65535, y * 65535, 1));
				return noError;
			}
		}
		case surfOpGetPixel: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x = popi32(rt);
				int y = popi32(rt);
				//~ reti32(rt, gx_getpixel(surf, x, y));
				//~ return noError;
				/ * but this way is faster
				if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
					reti32(rt, 0);
					return noError;
				}
				int rowy = y * surf->scanLen;
				if (surf->depth == 32) {
					uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
					reti32(rt, ptr[x]);
					return noError;
				}
				else if (surf->depth == 8) {
					uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
					reti32(rt, ptr[x]);
					return noError;
				}// * /
			}
		}
		case surfOpSetPixel: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x = popi32(rt);
				int y = popi32(rt);
				int col = popi32(rt);
				//~ / * this way is faster
				if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
					return noError;
				}

				unsigned rowy = y * (unsigned)surf->scanLen;
				if (surf->depth == 32) {
					uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
					ptr[x] = col;
					return noError;
				}
				else if (surf->depth == 8) {
					uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
					ptr[x] = col;
					return noError;
				}// * /

				//~ gx_setpixel(surf, x, y, col);
				//~ return noError;
			}
		}*/

		case surfOpDrawLine:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawline(surf, x0, y0, x1, y1, col);
				return noError;
			}
			break;
		case surfOpDrawBez2:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int x2 = popi32(rt);
				int y2 = popi32(rt);
				int col = popi32(rt);
				g2_drawbez2(surf, x0, y0, x1, y1, x2, y2, col);
				return noError;
			}
			break;
		case surfOpDrawBez3:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int x2 = popi32(rt);
				int y2 = popi32(rt);
				int x3 = popi32(rt);
				int y3 = popi32(rt);
				int col = popi32(rt);
				g2_drawbez3(surf, x0, y0, x1, y1, x2, y2, x3, y3, col);
				return noError;
			}
			break;
		case surfOpDrawRect:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				gx_drawrect(surf, x0, y0, x1, y1, col);
				return noError;
			}
			break;
		case surfOpFillRect:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				gx_fillrect(surf, x0, y0, x1, y1, col);
				return noError;
			}
			break;
		case surfOpDrawOval:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawoval(surf, x0, y0, x1, y1, col);
				return noError;
			}
			break;
		case surfOpFillOval:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_filloval(surf, x0, y0, x1, y1, col);
				return noError;
			}
			break;
		case surfOpDrawText:
			surf = getSurf(popi32(rt));
			if (surf != NULL) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				char *str = popstr(rt);
				int col = popi32(rt);
				gx_drawText(surf, x0, y0, &font, str, col);
				return noError;
			}
			break;

		case surfOpCopySurf: {		// gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi);
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);

			gx_Surf sdst = getSurf(dst);
			gx_Surf ssrc = getSurf(src);

			if (sdst && ssrc) {
				gx_copysurf(sdst, x, y, ssrc, roi, 0);
			}
			reti32(rt, dst);
			return noError;
		} break;
		case surfOpZoomSurf: {		// gxSurf zoomSurf(gxSurf dst, gxRect &rect, gxSurf src, gxRect &roi, int mode)
			gxSurfHnd dst = popi32(rt);
			gx_Rect rect = popref(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			int mode = popi32(rt);
			gx_Surf sdst = getSurf(dst);
			gx_Surf ssrc = getSurf(src);

			if (sdst && ssrc) {
				gx_zoomsurf(sdst, rect, ssrc, roi, mode);
			}
			//~ else dst = 0;
			reti32(rt, dst);
			return noError;
		} break;

		case surfOpEvalPxCB: {		// gxSurf evalSurfrgb(gxSurf dst, gxRect &roi, int callBack(int x, int y));
			rtContext rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);
			//~ fputfmt(stdout, "callback is: %-T\n", callback);

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: evalSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				char *cBuffY = sdst->basePtr;
				for (sy = 0; sy < sdst->height; sy += 1) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (sx = 0; sx < sdst->width; sx += 1) {
						struct {int32_t x, y;} args = {sx, sy};
						if (invoke(rt_, callback, &cBuff[sx], &args, NULL) != 0) {
							//~ dump(s, dump_sym | dump_asm, callback, "error:&-T\n", callback);
							return executionAborted;
						}
					}
				}
			}

			reti32(rt, dst);
			return noError;
		} break;
		case surfOpFillPxCB: {		// gxSurf fillSurfrgb(gxSurf dst, gxRect &roi, int callBack(int col));
			rtContext rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: fillSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				char *cBuffY = sdst->basePtr;
				for (sy = 0; sy < sdst->height; sy += 1) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (sx = 0; sx < sdst->width; sx += 1) {
						struct {int32_t col;} args = {cBuff[sx]};
						if (invoke(rt_, callback, &cBuff[sx], &args, NULL) != 0) {
							//~ dump(s, dump_sym | dump_asm, callback, "error:&-T\n", callback);
							return executionAborted;
						}
					}
				}
			}
			reti32(rt, dst);
			return noError;
		} break;
		case surfOpCopyPxCB: {		// gxSurf copySurfrgb(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, int callBack(int dst, int src));
			rtContext rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			gx_Surf sdst = getSurf(dst);
			gx_Surf ssrc = getSurf(src);

			if (sdst && ssrc) {
				register char *dptr, *sptr;
				struct gx_Rect clip;
				int x1, y1;

				clip.x = roi ? roi->x : 0;
				clip.y = roi ? roi->y : 0;
				clip.w = roi ? roi->w : ssrc->width;
				clip.h = roi ? roi->h : ssrc->height;

				if(!(sptr = (char*)gx_cliprect(ssrc, &clip)))
					return noError;

				clip.x = x;
				clip.y = y;
				if (clip.x < 0)
					clip.w -= clip.x;
				if (clip.y < 0)
					clip.h -= clip.y;

				if(!(dptr = (char*)gx_cliprect(sdst, &clip)))
					return noError;

				if (callback) {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					for (y = clip.y; y < y1; y += 1) {
						long* cbdst = (long*)dptr;
						long* cbsrc = (long*)sptr;
						for (x = clip.x; x < x1; x += 1) {
							struct {int32_t dst, src;} args = {*cbdst, *cbsrc};
							if (invoke(rt_, callback, cbdst, &args, NULL) != 0) {
								//~ dump(s, dump_sym | dump_asm, callback, "error:&-T\n", callback);
								return executionAborted;
							}
							cbdst += 1;
							cbsrc += 1;
						}
						dptr += sdst->scanLen;
						sptr += ssrc->scanLen;
					}
				}
				else {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					for (y = clip.y; y < y1; y += 1) {
						memcpy(dptr, sptr, 4 * clip.w);
						dptr += sdst->scanLen;
						sptr += ssrc->scanLen;
					}
				}

			}
			reti32(rt, dst);
			return noError;
		} break;

		case surfOpEvalFpCB: {		// gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(float x, float y));
			rtContext rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			gx_Surf sdst = getSurf(dst);

			// step in x, and y direction
			float dx01 = 1. / sdst->width;
			float dy01 = 1. / sdst->height;

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: evalSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				float x01, y01;			// x and y in [0, 1)
				char *cBuffY = sdst->basePtr;
				for (y01 = sy = 0; sy < sdst->height; sy += 1, y01 += dy01) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (x01 = sx = 0; sx < sdst->width; sx += 1, x01 += dx01) {
						struct {float32_t x, y;} args = {x01, y01};
						struct vector result;
						if (invoke(rt_, callback, &result, &args, NULL) != 0) {
							//~ dump(s, dump_sym | dump_asm, callback, "error:&-T\n", callback);
							return executionAborted;
						}
						cBuff[sx] = vecrgb(&result).col;
					}
				}
			}
			reti32(rt, dst);
			return noError;
		} break;
		case surfOpFillFpCB: {		// gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));
			rtContext rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = popsym(rt);

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				gx_debug("Unimplemented: fillSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				char *cBuffY = sdst->basePtr;
				for (sy = 0; sy < sdst->height; sy += 1) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (sx = 0; sx < sdst->width; sx += 1) {
						struct vector result;
						struct vector args = vecldc(rgbval(cBuff[sx]));
						if (invoke(rt_, callback, &result, &args, NULL) != 0) {
							//~ dump(s, dump_sym | dump_asm, callback, "error:&-T\n", callback);
							return executionAborted;
						}
						cBuff[sx] = vecrgb(&result).col;
					}
				}
			}
			reti32(rt, dst);
			return noError;
		} break;
		case surfOpCopyFpCB: {		// gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, float alpha, vec4f callBack(vec4f dst, vec4f src));
			rtContext rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			float alpha = popf32(rt);
			symn callback = popsym(rt);

			gx_Surf sdst = getSurf(dst);
			gx_Surf ssrc = getSurf(src);

			if (sdst && ssrc) {
				register char *dptr, *sptr;
				//~ struct gx_Rect dclp, sclp;
				struct gx_Rect clip;
				int x1, y1;

				clip.x = roi ? roi->x : 0;
				clip.y = roi ? roi->y : 0;
				clip.w = roi ? roi->w : ssrc->width;
				clip.h = roi ? roi->h : ssrc->height;

				if(!(sptr = (char*)gx_cliprect(ssrc, &clip)))
					return noError;

				clip.x = x;
				clip.y = y;
				if (clip.x < 0)
					clip.w -= x;
				if (clip.y < 0)
					clip.h -= y;
				if(!(dptr = (char*)gx_cliprect(sdst, &clip)))
					return noError;

				if (callback) {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					if (alpha > 0 && alpha < 1) {
						int alpha16 = alpha * (1 << 16);
						for (y = clip.y; y < y1; y += 1) {
							long* cbdst = (long*)dptr;
							long* cbsrc = (long*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								struct vector result;
								struct {struct vector dst, src;} args;/* = {
									vecldc(rgbval(*cbdst)),
									vecldc(rgbval(*cbsrc))
								};// */
								args.dst = vecldc(rgbval(*cbdst));
								args.src = vecldc(rgbval(*cbsrc));
								if (invoke(rt_, callback, &result, &args, NULL) != 0) {
									return executionAborted;
								}
								*(argb*)cbdst = rgbmix16(*(argb*)cbdst, vecrgb(&result), alpha16);
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += sdst->scanLen;
							sptr += ssrc->scanLen;
						}
					}
					else if (alpha >= 1){
						for (y = clip.y; y < y1; y += 1) {
							long* cbdst = (long*)dptr;
							long* cbsrc = (long*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								struct vector result;
								struct {struct vector dst, src;} args;/* = {
									vecldc(rgbval(*cbdst)),
									vecldc(rgbval(*cbsrc))
								};// */
								args.dst = vecldc(rgbval(*cbdst));
								args.src = vecldc(rgbval(*cbsrc));
								if (invoke(rt_, callback, &result, &args, NULL) != 0) {
									return executionAborted;
								}
								*cbdst = vecrgb(&result).col;
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += sdst->scanLen;
							sptr += ssrc->scanLen;
						}
					}
				}
				else {
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					if (alpha > 0 && alpha < 1) {
						int alpha16 = alpha * (1 << 16);
						for (y = clip.y; y < y1; y += 1) {
							long* cbdst = (long*)dptr;
							long* cbsrc = (long*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								*(argb*)cbdst = rgbmix16(*(argb*)cbdst, *(argb*)cbsrc, alpha16);
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += sdst->scanLen;
							sptr += ssrc->scanLen;
						}
					}
					else if (alpha >= 1) {
						for (y = clip.y; y < y1; y += 1) {
							memcpy(dptr, sptr, 4 * clip.w);
							dptr += sdst->scanLen;
							sptr += ssrc->scanLen;
						}
					}
				}

			}
			reti32(rt, dst);
			return noError;
		} break;

		case surfOpNewSurf: {
			int w = popi32(rt);
			int h = popi32(rt);
			gxSurfHnd hnd = newSurf(w, h);
			reti32(rt, hnd);
			return noError;
		} break;
		case surfOpDelSurf: {
			delSurf(popi32(rt));
			return noError;
		} break;

		case surfOpBmpRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int error = gx_loadBMP(surf, fileName, 32);
				reti32(rt, dst);
				//~ gx_debug("gx_readBMP(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
		} break;
		case surfOpJpgRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int error = gx_loadJPG(surf, fileName, 32);
				reti32(rt, dst);
				//~ gx_debug("readJpg(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
		} break;
		case surfOpPngRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int error = gx_loadPNG(surf, fileName, 32);
				reti32(rt, dst);
				//~ gx_debug("gx_readPng(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
		} break;
		case surfOpBmpWrite: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				char *fileName = popstr(rt);
				int error = gx_saveBMP(fileName, surf, 0);
				//~ gx_debug("gx_saveBMP(%s):%d;", fileName, error);
				return error ? executionAborted : noError;
			}
		} break;

		case surfOpClutSurf: {		// gxSurf clutSurf(gxSurf dst, gxRect &roi, gxClut &lut);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);
			gx_Surf sdst = getSurf(dst);

			if (sdst && lut) {
				gx_clutsurf(sdst, roi, lut);
			}
			reti32(rt, dst);
			return noError;
		} break;
		case surfOpCmatSurf: {		// gxSurf cmatSurf(gxSurf dst, gxRect &roi, mat4f &mat);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			matrix mat = popref(rt);

			gx_Surf sdst = getSurf(dst);

			if (sdst && mat) {
				int i, fxpmat[16];			// wee will do fixed point arithmetic with the matrix
				struct gx_Rect rect;
				char *dptr;

				if (sdst->depth != 32)
					return executionAborted;

				rect.x = roi ? roi->x : 0;
				rect.y = roi ? roi->y : 0;
				rect.w = roi ? roi->w : sdst->width;
				rect.h = roi ? roi->h : sdst->height;

				if(!(dptr = gx_cliprect(sdst, &rect)))
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
						*cBuff = rgbclamp(ro, go, bo);
						cBuff += 1;
					}
					dptr += sdst->scanLen;
				}
			}
			reti32(rt, dst);
			return noError;
		} break;
		case surfOpGradSurf: {		// gxSurf gradSurf(gxSurf dst, gxRect &roi, gxClut &lut, int mode);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);
			int mode = popi32(rt);
			int repeat = popi32(rt);
			gx_Surf sdst = getSurf(dst);

			if (repeat) {
				mode |= gradient_rep;
			}
			if (sdst) {
				gx_gradsurf(sdst, roi, lut, mode);
				reti32(rt, dst);
				return noError;
			}
		} break;
		case surfOpBlurSurf: {		// gxSurf blurSurf(gxSurf dst, gxRect &roi, int radius);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			int radius = popi32(rt);

			gx_Surf sdst = getSurf(dst);

			if (sdst) {
				gx_blursurf(sdst, roi, radius);
				reti32(rt, dst);
				return noError;
			}
		} break;
		case surfOpOilPaint: {		// gxSurf oilPaint(gxSurf dst, gxRect &roi, int radius, intensity);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			int radius = popi32(rt);
			int intensity = popi32(rt);

			gx_Surf sdst = getSurf(dst);

			if (sdst != NULL) {
				int gx_oilPaintSurf(gx_Surf image, gx_Rect roi, int radius, int intensity);
				gx_oilPaintSurf(sdst, roi, radius, intensity);
				reti32(rt, dst);
				return noError;
			}
		} break;
	}
	return executionAborted;
}
//#}#endregion

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
	temp.width = w;
	temp.height = h;
	temp.depth = 32;
	temp.scanLen = 0;
	if (intensity > 256) {
		return -1;
	}
	if (gx_initSurf(&temp, 0, 0, 0) != 0) {
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
				//~ int rgb = int(rgbClamp(sumR[maxIndex] / curMax, sumG[maxIndex] / curMax, sumB[maxIndex] / curMax));
				//~ setPixel(dest, row, col, rgb);
				int r = averageR[maxIndex] / curMax;
				int g = averageG[maxIndex] / curMax;
				int b = averageB[maxIndex] / curMax;

				gx_setpixel(&temp, x, y, rgbrgb(r, g, b).val);
			}
		}
	}
	gx_copysurf(image, 0, 0, &temp, NULL, 0);
	gx_doneSurf(&temp);
	return 0;
}

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
		msh.map = getSurf(popi32(rt));
		msh.freeTex = 0;
		//~ gx_debug("msh.map = %p", msh.map);
		//~ textureMesh(&msh, popstr(s));
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
	int data;
	char* def;
}* def,
Gui[] = {
	{miscCall, miscSetExitLoop,		"void exitLoop();"},
	{miscCall, miscOpFlipScreen,	"void Repaint(bool forceNow);"},
	{miscCall, miscOpSetCbMouse,	"void setMouseHandler(void handler(int btn, int x, int y));"},
	{miscCall, miscOpSetCbKeyboard,	"void setKeyboardHandler(void handler(int btn, int ext));"},
	{miscCall, miscOpSetCbRender,	"void setDrawCallback(void callback());"},
},
Surf[] = {
	{surfCall, surfOpGetWidth,		"int width(gxSurf dst);"},
	{surfCall, surfOpGetHeight,	"int height(gxSurf dst);"},
	{surfCall, surfOpGetDepth,	"int depth(gxSurf dst);"},
	{surfCall, surfOpClipRect,		"bool clipRect(gxSurf surf, gxRect &rect);"},

	{surfGetPixel, 0,			"int getPixel(gxSurf dst, int x, int y);"},
	{surfGetPixfp, 0,			"int getPixel(gxSurf dst, double x, double y);"},
	{surfSetPixel, 0,			"void setPixel(gxSurf dst, int x, int y, int col);"},
	//~ {surfCall, surfOpGetPixel,		"int getPixel(gxSurf dst, int x, int y);"},
	//~ {surfCall, surfOpGetPixfp,		"int getPixel(gxSurf dst, double x, double y);"},
	//~ {surfCall, surfOpSetPixel,		"void setPixel(gxSurf dst, int x, int y, int col);"},

	{surfCall, surfOpDrawLine,		"void drawLine(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpDrawBez2,		"void drawBezier(gxSurf dst, int x0, int y0, int x1, int y1, int x2, int y2, int col);"},
	{surfCall, surfOpDrawBez3,		"void drawBezier(gxSurf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int col);"},
	{surfCall, surfOpDrawRect,		"void drawRect(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpFillRect,		"void fillRect(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpDrawOval,		"void drawOval(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpFillOval,		"void fillOval(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpDrawText,		"void drawText(gxSurf dst, int x0, int y0, string text, int col);"},

	{surfCall, surfOpCopySurf,		"gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi);"},
	{surfCall, surfOpZoomSurf,		"gxSurf zoomSurf(gxSurf dst, gxRect &rect, gxSurf src, gxRect &roi, int mode);"},

	{surfCall, surfOpCmatSurf,		"gxSurf cmatSurf(gxSurf dst, gxRect &roi, mat4f &mat);"},
	{surfCall, surfOpClutSurf,		"gxSurf clutSurf(gxSurf dst, gxRect &roi, gxClut &lut);"},
	{surfCall, surfOpGradSurf,		"gxSurf gradSurf(gxSurf dst, gxRect &roi, gxClut &lut, int mode, bool repeat);"},
	{surfCall, surfOpBlurSurf,		"gxSurf blurSurf(gxSurf dst, gxRect &roi, int radius);"},
	{surfCall, surfOpOilPaint,		"gxSurf oilPaintSurf(gxSurf dst, gxRect &roi, int radius, int intensity);"},

	{surfCall, surfOpBmpRead,		"gxSurf readBmp(gxSurf dst, string fileName);"},
	{surfCall, surfOpJpgRead,		"gxSurf readJpg(gxSurf dst, string fileName);"},
	{surfCall, surfOpPngRead,		"gxSurf readPng(gxSurf dst, string fileName);"},
	{surfCall, surfOpBmpWrite,		"void bmpWrite(gxSurf src, string fileName);"},

	{surfCall, surfOpNewSurf,		"gxSurf newSurf(int width, int height);"},
	{surfCall, surfOpDelSurf,		"void delSurf(gxSurf surf);"},

	{surfCall, surfOpFillFpCB,		"gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));"},
	{surfCall, surfOpEvalFpCB,		"gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(float x, float y));"},
	{surfCall, surfOpCopyFpCB,		"gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, float alpha, vec4f callBack(vec4f dst, vec4f src));"},

	{surfCall, surfOpFillPxCB,		"gxSurf fillSurfrgb(gxSurf dst, gxRect &roi, int callBack(int col));"},
	{surfCall, surfOpEvalPxCB,		"gxSurf evalSurfrgb(gxSurf dst, gxRect &roi, int callBack(int x, int y));"},
	{surfCall, surfOpCopyPxCB,		"gxSurf copySurfrgb(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, int callBack(int dst, int src));"},

},
// 3d
Mesh[] = {
	{meshCall, meshOpSetPos,		"void Pos(int Index, double x, double y, double z);"},
	{meshCall, meshOpSetNrm,		"void Nrm(int Index, double x, double y, double z);"},
	{meshCall, meshOpSetTex,		"void Tex(int Index, double s, double t);"},
	//~ {meshCall, meshOpGetPos,		"vec4f Pos(int Index);"},
	//~ {meshCall, meshOpGetNrm,		"vec4f Nrm(int Index);"},
	//~ {meshCall, meshOpGetTex,		"vec2f Tex(int Index);"},
	//~ {meshCall, meshOpSetTri,		"int SetTri(int t, int p1, int p2, int p3);"},
	{meshCall, meshOpAddTri,		"void AddFace(int p1, int p2, int p3);"},
	{meshCall, meshOpAddQuad,		"void AddFace(int p1, int p2, int p3, int p4);"},
	//~ {meshCall, meshOpSetSeg,		"int SetSeg(int t, int p1, int p2);"},
	//~ {meshCall, meshOpAddSeg,		"int AddSeg(int p1, int p2);"},
	//~ {meshCall, meshOpGetFlags,		"int Flags;"},
	//~ {meshCall, meshOpSetFlags,		"int Flags(int Value);"},
	//~ {meshCall, meshOpGetTex,		"gxSurf Texture;"},
	{meshCall, meshOpTexture,		"int Texture(gxSurf image);"},
	{meshCall, meshOpInit,			"int Init(int Capacity);"},
	{meshCall, meshOpRead,			"int Read(string file, string texture);"},
	{meshCall, meshOpSave,			"int Save(string file);"},
	{meshCall, meshOpCenter,		"void Center(float32 Resize);"},
	{meshCall, meshOpNormalize,	"void Normalize(float32 Epsilon);"},
	{meshCall, meshOpInfo,			"void Info();"},
	{meshCall, meshOphasNrm,		"bool hasNormals;"},
	//~ */
},
Cam[] = {
	{camCall, camGetPos,			"vec4f Pos;"},
	{camCall, camGetUp,			"vec4f Up;"},
	{camCall, camGetRight,			"vec4f Right;"},
	{camCall, camGetForward,		"vec4f Forward;"},

	{camCall, camSetPos,			"void Pos(vec4f v);"},
	{camCall, camSetUp,			"void Up(vec4f v);"},
	{camCall, camSetRight,			"void Right(vec4f v);"},
	{camCall, camSetForward,		"void Forward(vec4f v);"},

	{camCall, camOpMove,			"void Move(vec4f &direction, float32 cnt);"},
	{camCall, camOpRotate,			"void Rotate(vec4f &direction, vec4f &orbit, float32 cnt);"},
	{camCall, camOpLookAt,			"void LookAt(vec4f eye, vec4f at, vec4f up);"},
	// */
},
Obj[] = {
	//~ {objCall, objGetPos,			"vec4f Pos;"},
	//~ {objCall, objSetPos,			"void Pos(vec4f v);"},
	//~ {objCall, objGetNrm,			"vec4f Nrm;"},
	//~ {objCall, objSetNrm,			"void Nrm(vec4f v);"},
	{objCall, objOpMove,			"void Move(float32 dx, float32 dy);"},
	{objCall, objOpRotate,			"void Rotate(float32 dx, float32 dy);"},
	{objCall, objOpSelected,		"bool object;"},
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
				dst[i++] = chr;
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

char* strncatesc(char *dst, int max, char* src) {
	int i = 0;

	while (dst[i])
		i += 1;

	return strnesc(dst + i, max - i, src);
}

#ifdef _MSC_VER
#define snprintf(__DST, __MAX, __FMT, ...)  sprintf_s(__DST, __MAX, __FMT, ##__VA_ARGS__)
#endif

// TODO: to be removed
int ccSymValInt(symn sym, int* res) {
	struct astNode ast;
	if (sym && eval(&ast, sym->init)) {
		*res = (int)constint(&ast);
		return 1;
	}
	return 0;
}
// TODO: to be removed
int ccSymValFlt(symn sym, double* res) {
	struct astNode ast;
	if (sym && eval(&ast, sym->init)) {
		*res = constflt(&ast);
		return 1;
	}
	return 0;
}

int ccCompile(char *src, int argc, char* argv[], int dbg(userContext, vmError, size_t ss, void* sp, void* caller, void* callee)) {
	symn cls = NULL;
	const int stdwl = 0;
	const int srcwl = 9;
	int i, err = 0;

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

	//#{ inlined here for libcall functions
	err = err || !ccDefCode(rt, stdwl, __FILE__, __LINE__ + 1,
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
	);//#} */

	if ((cls = ccBegin(rt, "Gradient"))) {
		err = err || !ccDefInt(rt, "Linear", gradient_lin);
		err = err || !ccDefInt(rt, "Radial", gradient_rad);
		err = err || !ccDefInt(rt, "Square", gradient_sqr);
		err = err || !ccDefInt(rt, "Conical", gradient_con);
		err = err || !ccDefInt(rt, "Spiral", gradient_spr);
		ccEnd(rt, cls, ATTR_stat);
	}

	// install standard library calls
	err = err || !ccAddUnit(rt, ccUnitStdc, stdwl, ccStd);
	err = err || !ccDefType(rt, "gxSurf", sizeof(gxSurfHnd), 0);

	for (i = 0; i < sizeof(Surf) / sizeof(*Surf); i += 1) {
		symn libc = ccDefCall(rt, Surf[i].fun, (void*)Surf[i].data, Surf[i].def);
		if (libc == NULL) {
			return -1;
		}
	}

	if ((cls = ccBegin(rt, "Gui"))) {
		for (i = 0; i < sizeof(Gui) / sizeof(*Gui); i += 1) {
			symn libc = ccDefCall(rt, Gui[i].fun, (void*)Gui[i].data, Gui[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		err = err || !ccDefCode(rt, stdwl, __FILE__, __LINE__ + 1,
			"define Repaint() = Repaint(false);\n"
		);
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "mesh"))) {
		for (i = 0; i < sizeof(Mesh) / sizeof(*Mesh); i += 1) {
			symn libc = ccDefCall(rt, Mesh[i].fun, (void*)Mesh[i].data, Mesh[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "camera"))) {
		for (i = 0; i < sizeof(Cam) / sizeof(*Cam); i += 1) {
			symn libc = ccDefCall(rt, Cam[i].fun, (void*)Cam[i].data, Cam[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls, ATTR_stat);
	}
	if ((cls = ccBegin(rt, "selection"))) {
		for (i = 0; i < sizeof(Obj) / sizeof(*Obj); i += 1) {
			symn libc = ccDefCall(rt, Obj[i].fun, (void*)Obj[i].data, Obj[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls, ATTR_stat);
	}

	err = err || !ccDefCode(rt, stdwl, __FILE__, __LINE__, "gxSurf offScreen = emit(gxSurf, int32(-1));");

	// it is not an error if library name is not set(NULL)
	if (err == 0 && ccGfx) {
		err = !ccDefCode(rt, stdwl, ccGfx, 1, NULL);
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
		err = err || !ccDefCode(rt, srcwl, src, 1, NULL);
	}
	else {
		logFILE(rt, stdout);
		ccDefCode(rt, stdwl, __FILE__, __LINE__ , "");
		gencode(rt, 0);
		fprintf(stdout, "#api: replace(`^([^:]*).*$`, `\\1`)\n");
		//~ dump(rt, dump_sym | 0x33, NULL);
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
		symn cls;
		ccContext cc = rt->cc;

		draw = 0;

		if ((cls = ccLookupSym(cc, NULL, "Window"))) {
			if (!ccSymValInt(ccLookupSym(cc, cls, "resx"), &resx)) {
				//~ gx_debug("using default value for resx: %d", resx);
			}
			if (!ccSymValInt(ccLookupSym(cc, cls, "resy"), &resy)) {
				//~ gx_debug("using default value for resy: %d", resy);
			}
			if (!ccSymValInt(ccLookupSym(cc, cls, "draw"), &draw)) {
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
			//~ rt->dbg->function = dbg;
		}
	}

	logFILE(rt, stdout);

	return err;// || execute(rt, NULL);
}

#if 1 // eval mesh with scripting support

static inline double lerp(double a, double b, double t) {return a + t * (b - a);}

inline double  dv3dot(double lhs[3], double rhs[3]) {
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}
inline double* dv3cpy(double dst[3], double src[3]) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	return dst;
}
inline double* dv3sca(double dst[3], double src[3], double rhs) {
	dst[0] = src[0] * rhs;
	dst[1] = src[1] * rhs;
	dst[2] = src[2] * rhs;
	return dst;
}
inline double* dv3sub(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[0] - rhs[0];
	dst[1] = lhs[1] - rhs[1];
	dst[2] = lhs[2] - rhs[2];
	return dst;
}
inline double* dv3crs(double dst[3], double lhs[3], double rhs[3]) {
	dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
	dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
	dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
	return dst;
}
inline double* dv3nrm(double dst[3], double src[3]) {
	double len = dv3dot(src, src);
	if (len > 0) {
		//~ len = 1. / sqrt(len);
		//~ dst[0] = src[0] * len;
		//~ dst[1] = src[1] * len;
		//~ dst[2] = src[2] * len;
		len = sqrt(len);
		dst[0] = src[0] / len;
		dst[1] = src[1] / len;
		dst[2] = src[2] / len;
	}
	return dst;
}

typedef struct userData {
	double s, smin, smax;
	double t, tmin, tmax;
	int isPos, isNrm;
	double pos[3], nrm[3];
} *userData;

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
	static char mem[32 << 10];		// 32K memory
	rtContext rt = rtInit(mem, sizeof(mem));
	struct userData ud;
	const int warnlevel = 2;
	const int stacksize = sizeof(mem) / 2;

	char *logf = NULL;//"dump.evalMesh.txt";

	int i, j, err = 0;

	double s, t, ds, dt;	// 0 .. 1

	// pointers, variants and emit are not needed.
	if (!ccInit(rt, installBase, NULL)) {
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

	err = err || !ccDefCode(rt, 0, __FILE__, __LINE__ + 1,
		"const double s = gets();\n"
		"const double t = gett();\n"
		"float64 x = s;\n"
		"float64 y = t;\n"
		"float64 z = float64(0);\n"
	);
	err = err || !ccDefCode(rt, warnlevel, file, line, src);
	err = err || !ccDefCode(rt, warnlevel, __FILE__, __LINE__, "setPos(x, y, z);\n");
	// */

	// optimize on max level, and generate global variables on stack
	if (err || !gencode(rt, 0)) {
		gx_debug("error compiling(%d), see `%s`", err, logf);
		logfile(rt, NULL, 0);
		return -3;
	}

	/* dump
	fputfmt(stdout, "init(ro: %d"
		", ss: %d, sm: %d, pc: %d, px: %d"
		", size.meta: %d, size.code: %d, size.data: %d"
		//~ ", pos: %d"
	");\n", rt->vm.ro, rt->vm.ss, rt->vm.sm, rt->vm.pc, rt->vm.px, rt->vm.size.meta, rt->vm.size.code, rt->vm.size.data, rt->vm.pos);

	logfile(rt, "dump.bin");
	dump(rt, dump_sym | 0x01, NULL, "\ntags:\n");
	dump(rt, dump_ast | 0x00, NULL, "\ncode:\n");
	dump(rt, dump_asm | 0x19, NULL, "\ndasm:\n");
	dump(rt, dump_bin | 0x10, NULL, "\ndump:\n");
	//~ */

	// close log
	logfile(rt, NULL, 0);

	#define findint(__ENV, __NAME, _OUT_VAL) ccSymValInt(ccLookupSym(__ENV, NULL, __NAME), _OUT_VAL)
	#define findflt(__ENV, __NAME, _OUT_VAL) ccSymValFlt(ccLookupSym(__ENV, NULL, __NAME), _OUT_VAL)

	if (findint(rt->cc, "division", &tdiv)) {
		sdiv = tdiv;
	}

	ud.smin = ud.tmin = 0;
	ud.smax = ud.tmax = 1;

	findflt(rt->cc, "smin", &ud.smin);
	findflt(rt->cc, "smax", &ud.smax);
	findint(rt->cc, "sdiv", &sdiv);

	findflt(rt->cc, "tmin", &ud.tmin);
	findflt(rt->cc, "tmax", &ud.tmax);
	findint(rt->cc, "tdiv", &tdiv);

	//~ cs = lookup_nz(env->cc, "closedS");
	//~ ct = lookup_nz(env->cc, "closedT");

	//~ gx_debug("s(min:%lf, max:%lf, div:%d%s)", ud.smin, ud.smax, sdiv, /* cs ? ", closed" : */ "");
	//~ gx_debug("t(min:%lf, max:%lf, div:%d%s)", ud.tmin, ud.tmax, tdiv, /* ct ? ", closed" : */ "");
	//~ vmInfo(env->vm);

	ds = 1. / (sdiv - 1);
	dt = 1. / (tdiv - 1);
	msh->hasTex = msh->hasNrm = 1;
	msh->tricnt = msh->vtxcnt = 0;

	for (t = 0, j = 0; j < tdiv; t += dt, ++j) {
		for (s = 0, i = 0; i < sdiv; s += ds, ++i) {
			double pos[3], nrm[3], tex[2];
			double ds[3], dt[3];
			tex[0] = t;
			tex[1] = s;

			ud.s = s;
			ud.t = t;
			ud.isNrm = 0;
			if (execute(rt, stacksize, &ud) != 0) {
				gx_debug("error");
				return -4;
			}
			dv3cpy(pos, ud.pos);

			if (ud.isNrm) {
				dv3nrm(nrm, ud.nrm);
			}
			else {
				ud.s = s + epsilon;
				ud.t = t;
				if (execute(rt, stacksize, &ud) != 0) {
					gx_debug("error");
					return -5;
				}
				dv3cpy(ds, ud.pos);

				ud.s = s;
				ud.t = t + epsilon;
				if (execute(rt, stacksize, &ud) != 0) {
					gx_debug("error");
					return -6;
				}
				dv3cpy(dt, ud.pos);

				ds[0] = (pos[0] - ds[0]) / epsilon;
				ds[1] = (pos[1] - ds[1]) / epsilon;
				ds[2] = (pos[2] - ds[2]) / epsilon;
				dt[0] = (pos[0] - dt[0]) / epsilon;
				dt[1] = (pos[1] - dt[1]) / epsilon;
				dt[2] = (pos[2] - dt[2]) / epsilon;
				dv3nrm(nrm, dv3crs(nrm, ds, dt));
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
