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

#include "pvmc.h"
#include <assert.h>

//#{ old style
static inline void* poparg(libcArgs rt, void *result, int size) {
	// if result is not null copy
	if (result != NULL) {
		memcpy(result, rt->argv, size);
	}
	else {
		result = rt->argv;
	}
	rt->argv += padded(size, 4);
	return result;
}

#define poparg(__ARGV, __TYPE) (((__TYPE*)((__ARGV)->argv += ((sizeof(__TYPE) + 3) & ~3)))[-1])
static inline int32_t popi32(libcArgs rt) { return poparg(rt, int32_t); }
static inline int64_t popi64(libcArgs rt) { return poparg(rt, int64_t); }
static inline float32_t popf32(libcArgs rt) { return poparg(rt, float32_t); }
static inline float64_t popf64(libcArgs rt) { return poparg(rt, float64_t); }
#undef poparg

static inline void* popref(libcArgs rt) { int32_t p = popi32(rt); return p ? rt->rt->_mem + p : NULL; }
static inline char* popstr(libcArgs rt) { return popref(rt); }
//#}

state rt = NULL;
symn renderMethod = NULL;
symn mouseCallBack = NULL;
symn keyboardCallBack = NULL;

extern struct camera cam[1];		// camera
extern struct mesh msh;			// mesh
extern struct gx_Surf offs;		// offscreen
extern struct gx_Surf font;		// font

//~ extern struct vector eye, tgt, up;

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
extern int ccDbg;

//#{#region Surfaces
typedef enum {
//~ surfOpGetDepth,
surfOpGetWidth,
surfOpGetHeight,
//~ surfOpGetPixel,
//~ surfOpGetPixfp,
//~ surfOpSetPixel,
surfOpClipRect,

surfOpDrawLine,
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
	if (hnd == -1U)
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
	debug("getSurf(%d): null", hnd);
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

static int surfSetPixel(libcArgs rt) {
	gx_Surf surf;
	if ((surf = getSurf(popi32(rt)))) {
		unsigned rowy;
		int x = popi32(rt);
		int y = popi32(rt);
		int col = popi32(rt);
		//~ /* this way is faster
		if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
			return 0;
		}

		rowy = y * surf->scanLen;
		if (surf->depth == 32) {
			uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
			ptr[x] = col;
			return 0;
		}
		else if (surf->depth == 8) {
			uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
			ptr[x] = col;
			return 0;
		}// */

		//~ gx_setpixel(surf, x, y, col);
		//~ return 0;
	}
	return -1;
}
static int surfGetPixel(libcArgs rt) {
	gx_Surf surf;
	if ((surf = getSurf(popi32(rt)))) {
		unsigned rowy;
		int x = popi32(rt);
		int y = popi32(rt);
		//~ /* but this way is faster
		if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
			reti32(rt, 0);
			return 0;
		}
		rowy = y * surf->scanLen;
		if (surf->depth == 32) {
			uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
			reti32(rt, ptr[x]);
			return 0;
		}
		else if (surf->depth == 8) {
			uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
			reti32(rt, ptr[x]);
			return 0;
		}// */
		//~ reti32(rt, gx_getpixel(surf, x, y));
		//~ return 0;
	}
	return -1;
}
static int surfGetPixfp(libcArgs rt) {
	gx_Surf surf;
	if ((surf = getSurf(popi32(rt)))) {
		double x = popf64(rt);
		double y = popf64(rt);
		reti32(rt, gx_getpix16(surf, x * 65535, y * 65535, 1));
		return 0;
	}
	return -1;
}
static int surfCall(libcArgs rt) {
	switch ((surfFunc)rt->data) {
		case surfOpGetWidth: {
			gx_Surf surf = getSurf(popi32(rt));
			reti32(rt, surf->width);
			return 0;
		} break;
		case surfOpGetHeight: {
			gx_Surf surf = getSurf(popi32(rt));
			reti32(rt, surf->height);
			return 0;
		} break;

		case surfOpClipRect: {
			gx_Surf surf = getSurf(popi32(rt));
			gx_Rect rect = argref(rt, 0);
			if (surf) {
				void *ptr = gx_cliprect(surf, rect);
				reti32(rt, ptr != NULL);
			}
			return 0;
		} break;

		/*case surfOpGetPixfp: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				double x = popf64(rt);
				double y = popf64(rt);
				reti32(rt, gx_getpix16(surf, x * 65535, y * 65535, 1));
				return 0;
			}
		}
		case surfOpGetPixel: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x = popi32(rt);
				int y = popi32(rt);
				//~ reti32(rt, gx_getpixel(surf, x, y));
				//~ return 0;
				/ * but this way is faster
				if ((unsigned)x >= (unsigned)surf->width || (unsigned)y >= (unsigned)surf->height) {
					reti32(rt, 0);
					return 0;
				}
				int rowy = y * surf->scanLen;
				if (surf->depth == 32) {
					uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
					reti32(rt, ptr[x]);
					return 0;
				}
				else if (surf->depth == 8) {
					uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
					reti32(rt, ptr[x]);
					return 0;
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
					return 0;
				}

				unsigned rowy = y * (unsigned)surf->scanLen;
				if (surf->depth == 32) {
					uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
					ptr[x] = col;
					return 0;
				}
				else if (surf->depth == 8) {
					uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
					ptr[x] = col;
					return 0;
				}// * /

				//~ gx_setpixel(surf, x, y, col);
				//~ return 0;
			}
		}*/

		case surfOpDrawLine: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawline(surf, x0, y0, x1, y1, col);
				return 0;
			}
		} break;
		case surfOpDrawRect: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				gx_drawrect(surf, x0, y0, x1, y1, col);
				return 0;
			}
		} break;
		case surfOpFillRect: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				gx_fillrect(surf, x0, y0, x1, y1, col);
				return 0;
			}
		} break;
		case surfOpDrawOval: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_drawoval(surf, x0, y0, x1, y1, col);
				return 0;
			}
		} break;
		case surfOpFillOval: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				int x1 = popi32(rt);
				int y1 = popi32(rt);
				int col = popi32(rt);
				g2_filloval(surf, x0, y0, x1, y1, col);
				return 0;
			}
		} break;
		case surfOpDrawText: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x0 = popi32(rt);
				int y0 = popi32(rt);
				char *str = popstr(rt);
				int col = popi32(rt);
				gx_drawText(surf, x0, y0, &font, str, col);
				return 0;
			}
		} break;

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
			return 0;
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
			return 0;
		} break;

		case surfOpEvalPxCB: {		// gxSurf evalSurfrgb(gxSurf dst, gxRect &roi, int callBack(int x, int y));
			state rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = mapsym(rt_, popref(rt));
			//~ fputfmt(stdout, "callback is: %-T\n", callback);

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				debug("Unimplemented: evalSurf called with a roi");
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
							return -1;
						}
					}
				}
			}

			reti32(rt, dst);
			return 0;
		} break;
		case surfOpFillPxCB: {		// gxSurf fillSurfrgb(gxSurf dst, gxRect &roi, int callBack(int col));
			state rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = mapsym(rt_, popref(rt));

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				debug("Unimplemented: fillSurf called with a roi");
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
							return -1;
						}
					}
				}
			}
			reti32(rt, dst);
			return 0;
		} break;
		case surfOpCopyPxCB: {		// gxSurf copySurfrgb(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, int callBack(int dst, int src));
			state rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = mapsym(rt_, popref(rt));

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
					return 0;

				clip.x = x;
				clip.y = y;
				if (clip.x < 0)
					clip.w -= clip.x;
				if (clip.y < 0)
					clip.h -= clip.y;

				if(!(dptr = (char*)gx_cliprect(sdst, &clip)))
					return 0;

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
								return -1;
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
			return 0;
		} break;

		case surfOpEvalFpCB: {		// gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(double x, double y));
			state rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = mapsym(rt_, popref(rt));

			gx_Surf sdst = getSurf(dst);

			// step in x, and y direction
			double dx01 = 1. / sdst->width;
			double dy01 = 1. / sdst->height;

			static int first = 1;
			if (roi && first) {
				first = 0;
				debug("Unimplemented: evalSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				double x01, y01;			// x and y in [0, 1)
				char *cBuffY = sdst->basePtr;
				for (y01 = sy = 0; sy < sdst->height; sy += 1, y01 += dy01) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (x01 = sx = 0; sx < sdst->width; sx += 1, x01 += dx01) {
						struct {float64_t x, y;} args = {x01, y01};
						struct vector result;
						if (invoke(rt_, callback, &result, &args, NULL) != 0) {
							//~ dump(s, dump_sym | dump_asm, callback, "error:&-T\n", callback);
							return -1;
						}
						cBuff[sx] = vecrgb(&result).col;
					}
				}
			}
			reti32(rt, dst);
			return 0;
		} break;
		case surfOpFillFpCB: {		// gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));
			state rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = mapsym(rt_, popref(rt));

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				debug("Unimplemented: fillSurf called with a roi");
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
							return -1;
						}
						cBuff[sx] = vecrgb(&result).col;
					}
				}
			}
			reti32(rt, dst);
			return 0;
		} break;
		case surfOpCopyFpCB: {		// gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, float64 alpha, vec4f callBack(vec4f dst, vec4f src));
			state rt_ = rt->rt;
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			double alpha = popf64(rt);
			symn callback = mapsym(rt_, popref(rt));

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
					return 0;

				clip.x = x;
				clip.y = y;
				if (clip.x < 0)
					clip.w -= x;
				if (clip.y < 0)
					clip.h -= y;
				if(!(dptr = (char*)gx_cliprect(sdst, &clip)))
					return 0;

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
									return -1;
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
									return -1;
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
			return 0;
		} break;

		case surfOpNewSurf: {
			int w = popi32(rt);
			int h = popi32(rt);
			gxSurfHnd hnd = newSurf(w, h);
			reti32(rt, hnd);
			return 0;
		} break;
		case surfOpDelSurf: {
			delSurf(popi32(rt));
			return 0;
		} break;

		case surfOpBmpRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int error = gx_loadBMP(surf, fileName, 32);
				reti32(rt, dst);
				//~ debug("gx_readBMP(%s):%d;", fileName, error);
				return error;
			}
		} break;
		case surfOpJpgRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int error = gx_loadJPG(surf, fileName, 32);
				//~ debug("readJpg(%s):%d;", fileName, error);
				reti32(rt, dst);
				return error;
			}
		} break;
		case surfOpPngRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int result = gx_loadPNG(surf, fileName, 32);
				reti32(rt, dst);
				//~ debug("gx_readPng(%s):%d;", fileName, result);
				return result;
			}
		} break;
		case surfOpBmpWrite: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				char *fileName = popstr(rt);
				return gx_saveBMP(fileName, surf, 0);
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
			return 0;
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
					return -1;

				rect.x = roi ? roi->x : 0;
				rect.y = roi ? roi->y : 0;
				rect.w = roi ? roi->w : sdst->width;
				rect.h = roi ? roi->h : sdst->height;

				if(!(dptr = gx_cliprect(sdst, &rect)))
					return -2;

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
			return 0;
		} break;
		case surfOpGradSurf: {		// gxSurf gradSurf(gxSurf dst, gxRect &roi, gxClut &lut, int mode);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);
			int mode = popi32(rt);
			gx_Surf sdst = getSurf(dst);

			if (sdst) {
				gx_gradsurf(sdst, roi, lut, mode);
				reti32(rt, dst);
				return 0;
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
				return 0;
			}
		} break;
	}
	return -1;
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
static int meshCall(libcArgs rt) {
	meshFunc fun = (meshFunc)rt->data;
	if (fun == meshOpSetPos) {		// void meshPos(int Index, double x, double y, double z);
		double pos[3];
		int idx = popi32(rt);
		pos[0] = popf64(rt);
		pos[1] = popf64(rt);
		pos[2] = popf64(rt);
		setvtxDV(&msh, idx, pos, NULL, NULL, 0xff00ff);
		return 0;
	}
	if (fun == meshOpSetNrm) {		// void meshNrm(int Index, double x, double y, double z);
		double nrm[3];
		int idx = popi32(rt);
		nrm[0] = popf64(rt);
		nrm[1] = popf64(rt);
		nrm[2] = popf64(rt);
		setvtxDV(&msh, idx, NULL, nrm, NULL, 0xff00ff);
		return 0;
	}
	//~ case meshOpGetPos: {		// vec3f meshPos(int Index)
	//~ case meshOpGetNrm: {		// vec3f meshNrm(int Index)
	if (fun == meshOpSetTex) {			// void meshTex(int Index, double s, double t)
		double pos[2];
		int idx = popi32(rt);
		pos[0] = popf64(rt);
		pos[1] = popf64(rt);
		setvtxDV(&msh, idx, NULL, NULL, pos, 0xff00ff);
		return 0;
	}
	//~ case meshOpGetTex: {		// vec2f meshTex(int Index)
	//~ case meshOpSetTri: {		// int meshSetTri(int t, int p1, int p2, int p3)
	if (fun == meshOpAddTri) {
		int p1 = popi32(rt);
		int p2 = popi32(rt);
		int p3 = popi32(rt);
		addtri(&msh, p1, p2, p3);
		return 0;
	}
	if (fun == meshOpAddQuad) {
		int p1 = popi32(rt);
		int p2 = popi32(rt);
		int p3 = popi32(rt);
		int p4 = popi32(rt);
		//~ addtri(&msh, p1, p2, p3);
		//~ addtri(&msh, p3, p4, p1);
		addquad(&msh, p1, p2, p3, p4);
		return 0;
	}
	//~ case meshOpSetSeg: {		// int meshSetSeg(int t, int p1, int p2)
	//~ case meshOpAddSeg: {		// int meshAddSeg(int p1, int p2)
	//~ case meshOpGetFlags: {		// int meshFlags()
	//~ case meshOpSetFlags: {		// int meshFlags(int Value)

	if (fun == meshOpInfo) {				// void meshInfo();
		meshInfo(&msh);
		return 0;
	}
	if (fun == meshOphasNrm) {				// bool hasNormals;
		reti32(rt, msh.hasNrm);
		return 0;
	}
	if (fun == meshOpInit) {			// int meshInit(int Capacity);
		reti32(rt, initMesh(&msh, popi32(rt)));
		return 0;
	}
	if (fun == meshOpTexture) {
		if (msh.freeTex && msh.map) {
			gx_destroySurf(msh.map);
		}
		msh.map = getSurf(popi32(rt));
		msh.freeTex = 0;
		//~ debug("msh.map = %p", msh.map);
		//~ textureMesh(&msh, popstr(s));
		return 0;
	}
	if (fun == meshOpRead) {			// int meshRead(string file);
		char* mesh = popstr(rt);
		char* tex = popstr(rt);
		reti32(rt, readorevalMesh(&msh, mesh, 0, tex, 0));
		return 0;
	}
	if (fun == meshOpSave) {			// int meshSave(string file);
		reti32(rt, saveMesh(&msh, popstr(rt)));
		return 0;
	}
	if (fun == meshOpCenter) {		// void Center(float size);
		centMesh(&msh, popf32(rt));
		return 0;
	}
	if (fun == meshOpNormalize) {		// void Normalize(double epsilon);
		normMesh(&msh, popf32(rt));
		return 0;
	}

	return -1;
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
static int camCall(libcArgs rt) {
	switch ((camFunc)rt->data) {
		case camOpMove: {
			vector dir = popref(rt);
			float32_t cnt = popf32(rt);
			cammov(cam, dir, cnt);
			return 0;
		}
		case camOpRotate: {
			vector dir = popref(rt);
			vector orb = popref(rt);
			float32_t cnt = popf32(rt);
			camrot(cam, dir, orb, cnt);
			return 0;
		}

		case camOpLookAt: {		// cam.lookAt(vec4f eye, vec4f at, vec4f up)
			struct vector campos, camat, camup;
			vector camPos = poparg(rt, &campos, sizeof(struct vector));	// eye
			vector camAt = poparg(rt, &camat, sizeof(struct vector));	// target
			vector camUp = poparg(rt, &camup, sizeof(struct vector));	// up
			//~ vector camPos = popref(s);
			//~ vector camAt = popref(s);
			//~ vector camUp = popref(s);
			camset(cam, camPos, camAt, camUp);
			return 0;
		}

		//#ifndef _MSC_VER
		case camGetPos: {
			setret(rt, &cam->pos, sizeof(struct vector));
			return 0;
		}
		case camSetPos: {
			poparg(rt, &cam->pos, sizeof(struct vector));
			return 0;
		}

		case camGetUp: {
			setret(rt, &cam->dirU, sizeof(struct vector));
			return 0;
		}
		case camSetUp: {
			poparg(rt, &cam->dirU, sizeof(struct vector));
			return 0;
		}

		case camGetRight: {
			setret(rt, &cam->dirR, sizeof(struct vector));
			return 0;
		}
		case camSetRight: {
			poparg(rt, &cam->dirR, sizeof(struct vector));
			return 0;
		}

		case camGetForward: {
			setret(rt, &cam->dirF, sizeof(struct vector));
			return 0;
		}
		//#endif

		case camSetForward: {
			poparg(rt, &cam->dirF, sizeof(struct vector));
			return 0;
		}
	}
	return -1;
}

typedef enum {
	objOpMove,
	objOpRotate,
	objOpSelected,
}objFunc;
static int objCall(libcArgs rt) {

	switch((objFunc)rt->data) {
		case objOpSelected: {
			reti32(rt, getobjvec(0) != NULL);
			return 0;
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
			return 0;
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
			return 0;
		}
	}
	return -1;
}
//#}#endregion

typedef enum {
	miscSetExitLoop,
	miscOpFlipScreen,
	miscOpSetCbMouse,
	miscOpSetCbKeyboard,
	miscOpSetCbRender,
} miscFunc;
static int miscCall(libcArgs rt) {
	switch ((miscFunc)rt->data) {
		case miscSetExitLoop: {
			draw = 0;
			return 0;
		}

		case miscOpFlipScreen: {
			if (popi32(rt)) {
				if (flip)
					flip(&offs, 0);
			}
			else {
				draw |= post_swap;
			}
			return 0;
		}

		case miscOpSetCbMouse: {
			mouseCallBack = mapsym(rt->rt, popref(rt));
			return 0;
		}

		case miscOpSetCbKeyboard: {
			keyboardCallBack = mapsym(rt->rt, popref(rt));
			return 0;
		}

		case miscOpSetCbRender: {
			renderMethod = mapsym(rt->rt, popref(rt));
			return 0;
		}

	}
	return -1;
}

struct {
	int (*fun)(libcArgs);
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
	{surfCall, surfOpClipRect,		"bool clipRect(gxSurf surf, gxRect &rect);"},

	{surfGetPixel, 0,			"int getPixel(gxSurf dst, int x, int y);"},
	{surfGetPixfp, 0,			"int getPixel(gxSurf dst, double x, double y);"},
	{surfSetPixel, 0,			"void setPixel(gxSurf dst, int x, int y, int col);"},
	//~ {surfCall, surfOpGetPixel,		"int getPixel(gxSurf dst, int x, int y);"},
	//~ {surfCall, surfOpGetPixfp,		"int getPixel(gxSurf dst, double x, double y);"},
	//~ {surfCall, surfOpSetPixel,		"void setPixel(gxSurf dst, int x, int y, int col);"},

	{surfCall, surfOpDrawLine,		"void drawLine(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpDrawRect,		"void drawRect(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpFillRect,		"void fillRect(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpDrawOval,		"void drawOval(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpFillOval,		"void fillOval(gxSurf dst, int x0, int y0, int x1, int y1, int col);"},
	{surfCall, surfOpDrawText,		"void drawText(gxSurf dst, int x0, int y0, string text, int col);"},

	{surfCall, surfOpCopySurf,		"gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi);"},
	{surfCall, surfOpZoomSurf,		"gxSurf zoomSurf(gxSurf dst, gxRect &rect, gxSurf src, gxRect &roi, int mode);"},

	{surfCall, surfOpCmatSurf,		"gxSurf cmatSurf(gxSurf dst, gxRect &roi, mat4f &mat);"},
	{surfCall, surfOpClutSurf,		"gxSurf clutSurf(gxSurf dst, gxRect &roi, gxClut &lut);"},
	{surfCall, surfOpGradSurf,		"gxSurf gradSurf(gxSurf dst, gxRect &roi, gxClut &lut, int mode);"},
	{surfCall, surfOpBlurSurf,		"gxSurf blurSurf(gxSurf dst, gxRect &roi, int radius);"},

	{surfCall, surfOpBmpRead,		"gxSurf readBmp(gxSurf dst, string fileName);"},
	{surfCall, surfOpJpgRead,		"gxSurf readJpg(gxSurf dst, string fileName);"},
	{surfCall, surfOpPngRead,		"gxSurf readPng(gxSurf dst, string fileName);"},
	{surfCall, surfOpBmpWrite,		"void bmpWrite(gxSurf src, string fileName);"},

	{surfCall, surfOpNewSurf,		"gxSurf newSurf(int width, int height);"},
	{surfCall, surfOpDelSurf,		"void delSurf(gxSurf surf);"},

	{surfCall, surfOpFillFpCB,		"gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));"},
	{surfCall, surfOpEvalFpCB,		"gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(double x, double y));"},
	{surfCall, surfOpCopyFpCB,		"gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, double alpha, vec4f callBack(vec4f dst, vec4f src));"},

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

int ccCompile(char *src, int argc, char* argv[], int (*dbg)(state rt, int pu, void* ip, long* bp, int ss)) {
	symn cls = NULL;
	const int stdwl = 0;
	const int srcwl = 9;
	int i, err = 0;

	if (rt == NULL)
		return -29;

	if (ccLog && logfile(rt, ccLog) != 0) {
		debug("can not open file `%s`\n", ccLog);
		return -2;
	}

	if (!ccInit(rt, creg_def, NULL)) {
		debug("Internal error\n");
		return -1;
	}

	//#{ inlined here for libcall functions
	err = err || !ccAddCode(rt, stdwl, __FILE__, __LINE__ + 1,
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
		err = err || !ccDefInt(rt, "Repeat", gradient_rep);
		ccEnd(rt, cls);
	}

	// install standard library calls
	err = err || !ccAddUnit(rt, install_stdc, stdwl, ccStd);
	err = err || !ccAddType(rt, "gxSurf", sizeof(gxSurfHnd), 0);

	for (i = 0; i < sizeof(Surf) / sizeof(*Surf); i += 1) {
		symn libc = ccAddCall(rt, Surf[i].fun, (void*)Surf[i].data, Surf[i].def);
		if (libc == NULL) {
			return -1;
		}
	}

	if ((cls = ccBegin(rt, "Gui"))) {
		for (i = 0; i < sizeof(Gui) / sizeof(*Gui); i += 1) {
			symn libc = ccAddCall(rt, Gui[i].fun, (void*)Gui[i].data, Gui[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		err = err || !ccAddCode(rt, stdwl, __FILE__, __LINE__ + 1,
			"define Repaint() = Repaint(false);\n"
		);
		ccEnd(rt, cls);
	}
	if ((cls = ccBegin(rt, "mesh"))) {
		for (i = 0; i < sizeof(Mesh) / sizeof(*Mesh); i += 1) {
			symn libc = ccAddCall(rt, Mesh[i].fun, (void*)Mesh[i].data, Mesh[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls);
	}
	if ((cls = ccBegin(rt, "camera"))) {
		for (i = 0; i < sizeof(Cam) / sizeof(*Cam); i += 1) {
			symn libc = ccAddCall(rt, Cam[i].fun, (void*)Cam[i].data, Cam[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls);
	}
	if ((cls = ccBegin(rt, "selection"))) {
		for (i = 0; i < sizeof(Obj) / sizeof(*Obj); i += 1) {
			symn libc = ccAddCall(rt, Obj[i].fun, (void*)Obj[i].data, Obj[i].def);
			if (libc == NULL) {
				return -1;
			}
		}
		ccEnd(rt, cls);
	}

	err = err || !ccAddCode(rt, stdwl, __FILE__, __LINE__, "gxSurf offScreen = emit(gxSurf, i32(-1));");

	// it is not an error if library name is not set(NULL)
	if (err == 0 && ccGfx) {
		err = !ccAddCode(rt, stdwl, ccGfx, 1, NULL);
	}

	if (src != NULL) {
		char tmp[65535];

		if ((cls = ccBegin(rt, "properties"))) {

			symn subcls = NULL;
			if ((subcls = ccBegin(rt, "screen"))) {
				err = err || !ccDefInt(rt, "width", resx);
				err = err || !ccDefInt(rt, "height", resy);
				ccEnd(rt, subcls);
			}

			if (!err && obj != NULL) {
				err = !ccDefStr(rt, "object", strnesc(tmp, sizeof(tmp), obj));
			}
			if (!err && tex != NULL) {
				err = !ccDefStr(rt, "texture", strnesc(tmp, sizeof(tmp), tex));
			}
			ccEnd(rt, cls);
		}

		/* TODO: add args
		snprintf(tmp, sizeof(tmp), "string args[%d];", argc);
		err = err || !ccAddCode(rt, stdwl, __FILE__, __LINE__ , tmp);
		for (i = 0; i < argc; i += 1) {
			snprintf(tmp, sizeof(tmp), "args[%d] = \"%s\";", i, strnesc(tmpf, sizeof(tmpf), argv[i]));
			err = err || !ccAddCode(rt, stdwl, __FILE__, __LINE__ , tmp);
		}// */
		//~ snprintf(tmp, sizeof(tmp), "string filename = \"%s\";", i, strnesc(tmpf, sizeof(tmpf), argv[i]));
		//~ err = err || !ccAddCode(rt, stdwl, __FILE__, __LINE__ , tmp);
		if (argc == 2) {
			//~ err = err || !ccDefStr(rt, "script", strnesc(tmp, sizeof(tmp), argv[0]));
			err = err || !ccDefStr(rt, "filename", strnesc(tmp, sizeof(tmp), argv[1]));
		}
		err = err || !ccAddCode(rt, srcwl, src, 1, NULL);
	}
	else {
		logFILE(rt, stdout);
		ccAddCode(rt, stdwl, __FILE__, __LINE__ , "");
		gencode(rt, 0);
		dump(rt, dump_sym | 0x12, NULL, "#api: replace(`^([^:]*).*$`, `\\1`)\n");
		return 0;
	}

	if (err || !gencode(rt, 0xff)) {
		if (ccLog) {
			debug("error compiling(%d), see ligfile: `%s`", err, ccLog);
			logfile(rt, NULL);
		}
		err = -3;
	}

	if (err == 0) {
		int tmp = 0;
		symn cls;
		ccState cc = rt->cc;

		draw = 0;
		if (ccSymValInt(ccFindSym(cc, NULL, "VmDebug"), &tmp)) {
			ccDbg = tmp != 0;
		}

		if ((cls = ccFindSym(cc, NULL, "Window"))) {
			if (!ccSymValInt(ccFindSym(cc, cls, "resx"), &resx)) {
				//~ debug("using default value for resx: %d", resx);
			}
			if (!ccSymValInt(ccFindSym(cc, cls, "resy"), &resy)) {
				//~ debug("using default value for resy: %d", resy);
			}
			if (!ccSymValInt(ccFindSym(cc, cls, "draw"), &draw)) {
				//~ debug("using default value for renderType: 0x%08x", draw);
			}
		}

		if (ccDmp && logfile(rt, ccDmp) == 0) {
			dump(rt, dump_sym | 0xf2, NULL, "\ntags:\n");
			//~ dump(rt, dump_ast | 0xff, NULL, "\ncode:\n");
			dump(rt, dump_asm | 0xf9, NULL, "\ndasm:\n");
		}

	}

	logFILE(rt, stderr);

	return err;// || execute(rt, NULL);
}
