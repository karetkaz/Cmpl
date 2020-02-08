#include <cmpl.h>

#include "g3_math.h"
#include "gx_color.h"

#include "gx_gui.h"
#include "gx_surf.h"
#include "g3_draw.h"

struct camera cam[1]; // TODO: singleton like camera
struct gx_Light lights[32];  // max 32 lights

// convert a double to 16 bit fixed point
#define fxp16(__VALUE) ((int32_t)(65536 * (double)(__VALUE)))

static inline rtValue nextValue(nfcContext ctx) {
	return ctx->rt->api.nfcReadArg(ctx, ctx->rt->api.nfcNextArg(ctx));
}

static inline vector a2Vec(vector storage, float32_t values[3]) {
	if (values == NULL) {
		return NULL;
	}
	storage->x = values[0];
	storage->y = values[1];
	storage->z = values[2];
	storage->w = 0;
	return storage;
}

static inline gx_Mesh meshMemMgr(rtContext ctx, gx_Mesh realloc, int size) {
	gx_Mesh result = ctx->api.rtAlloc(ctx, realloc, size);
	if (result == NULL) {
		return NULL;
	}
	memset(result, 0, size);
	return result;
}

static const char *proto_surf_create2d = "gxSurf create(int32 width, int32 height, int32 depth)";
static const char *proto_surf_create3d = "gxSurf create3d(int32 width, int32 height)";
static const char *proto_surf_recycle = "gxSurf recycle(gxSurf recycle, int32 width, int32 height, int32 depth, int32 flags)";
static vmError surf_recycle(nfcContext ctx) {
	gx_Surf result = NULL;
	if (ctx->proto == proto_surf_recycle) {
		gx_Surf recycle = nextValue(ctx).ref;
		int width = nextValue(ctx).i32;
		int height = nextValue(ctx).i32;
		int depth = nextValue(ctx).i32;
		int flags = nextValue(ctx).i32;
		result = gx_createSurf(recycle, width, height, depth, flags);
	}
	if (ctx->proto == proto_surf_create2d) {
		int width = nextValue(ctx).i32;
		int height = nextValue(ctx).i32;
		int depth = nextValue(ctx).i32;
		result = gx_createSurf(NULL, width, height, depth, Surf_2ds);
	}
	if (ctx->proto == proto_surf_create3d) {
		int width = nextValue(ctx).i32;
		int height = nextValue(ctx).i32;
		result = gx_createSurf(NULL, width, height, 32, Surf_3ds);
	}
	if (result == NULL) {
		return nativeCallError;
	}
	rethnd(ctx, result);
	return noError;
}

static const char *proto_surf_destroy = "void destroy(gxSurf surf)";
static vmError surf_destroy(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_destroySurf(surf);
	return noError;
}

static const char *proto_surf_openBmp = "gxSurf openBmp(char path[*], int32 depth)";
static const char *proto_surf_openPng = "gxSurf openPng(char path[*], int32 depth)";
static const char *proto_surf_openJpg = "gxSurf openJpg(char path[*], int32 depth)";
static const char *proto_surf_openFnt = "gxSurf openFnt(char path[*])";
static vmError surf_open(nfcContext ctx) {
	char *path = nextValue(ctx).ref;

	gx_Surf result = NULL;
	if (ctx->proto == proto_surf_openBmp) {
		int depth = nextValue(ctx).i32;
		result = gx_loadBmp(NULL, path, depth);
	}
	else if (ctx->proto == proto_surf_openPng) {
		int depth = nextValue(ctx).i32;
		result = gx_loadPng(NULL, path, depth);
	}
	else if (ctx->proto == proto_surf_openJpg) {
		int depth = nextValue(ctx).i32;
		result = gx_loadJpg(NULL, path, depth);
	}
	else if (ctx->proto == proto_surf_openFnt) {
		result = gx_loadFnt(NULL, path);
	}

	if (result == NULL) {
		return nativeCallError;
	}
	rethnd(ctx, result);
	return noError;
}

static const char *proto_surf_saveBmp = "void saveBmp(gxSurf surf, char path[*], int32 flags)";
static vmError surf_save(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	char *path = nextValue(ctx).ref;
	int flags = nextValue(ctx).i32;

	int result = gx_saveBmp(path, surf, flags);

	if (result != 0) {
		return nativeCallError;
	}
	return noError;
}

static const char *proto_surf_width = "int32 width(gxSurf surf)";
static vmError surf_width(nfcContext ctx) {
	gx_Surf surf = arghnd(ctx, 0);
	reti32(ctx, surf->width);
	return noError;
}

static const char *proto_surf_height = "int32 height(gxSurf surf)";
static vmError surf_height(nfcContext ctx) {
	gx_Surf surf = arghnd(ctx, 0);
	reti32(ctx, surf->height);
	return noError;
}

static const char *proto_surf_depth = "int32 depth(gxSurf surf)";
static vmError surf_depth(nfcContext ctx) {
	gx_Surf surf = arghnd(ctx, 0);
	reti32(ctx, surf->depth);
	return noError;
}


static const char *proto_surf_get = "int32 get(gxSurf surf, int32 x, int32 y)";
static vmError surf_get(nfcContext ctx) {
	int32_t y = argi32(ctx, 0);
	int32_t x = argi32(ctx, 4);
	gx_Surf surf = arghnd(ctx, 8);

	reti32(ctx, gx_getpixel(surf, x, y));
	return noError;
}

static const char *proto_surf_tex = "vec4f tex(gxSurf surf, float32 x, float32 y)";
static vmError surf_tex(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int32_t x = nextValue(ctx).f32 * 65535 * surf->width;
	int32_t y = nextValue(ctx).f32 * 65535 * surf->height;
	struct vector result = vecldc(cast_rgb(gx_getpix16(surf, x, y)));
	retset(ctx, &result, sizeof(struct vector));
	return noError;
}

static const char *proto_surf_set = "void set(gxSurf surf, int32 x, int32 y, uint32 color)";
static vmError surf_set(nfcContext ctx) {
	uint32_t color = argi32(ctx, 0);
	int32_t y = argi32(ctx, 4);
	int32_t x = argi32(ctx, 8);
	gx_Surf surf = arghnd(ctx, 12);

	gx_setpixel(surf, x, y, color);
	return noError;
}

static const char *proto_surf_drawRect = "void drawRect(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 color)";
static vmError surf_drawRect(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_drawRect(surf, x0, y0, x1, y1, color);
	return noError;
}

static const char *proto_surf_fillRect = "void fillRect(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 color)";
static vmError surf_fillRect(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_fillRect(surf, x0, y0, x1, y1, color);
	return noError;
}

static const char *proto_surf_drawOval = "void drawOval(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 color)";
static vmError surf_drawOval(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_drawOval(surf, x0, y0, x1, y1, color);
	return noError;
}

static const char *proto_surf_fillOval = "void fillOval(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 color)";
static vmError surf_fillOval(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_fillOval(surf, x0, y0, x1, y1, color);
	return noError;
}

static const char *proto_surf_drawLine = "void drawLine(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 color)";
static vmError surf_drawLine(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_drawLine(surf, x0, y0, x1, y1, color);
	return noError;
}

static const char *proto_surf_drawBez2 = "void drawBezier(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 x3, int32 y3, int32 color)";
static vmError surf_drawBez2(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	int x2 = nextValue(ctx).i32;
	int y2 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_drawBez2(surf, x0, y0, x1, y1, x2, y2, color);
	return noError;
}

static const char *proto_surf_drawBez3 = "void drawBezier(gxSurf surf, int32 x1, int32 y1, int32 x2, int32 y2, int32 x3, int32 y3, int32 x4, int32 y4, int32 color)";
static vmError surf_drawBez3(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x0 = nextValue(ctx).i32;
	int y0 = nextValue(ctx).i32;
	int x1 = nextValue(ctx).i32;
	int y1 = nextValue(ctx).i32;
	int x2 = nextValue(ctx).i32;
	int y2 = nextValue(ctx).i32;
	int x3 = nextValue(ctx).i32;
	int y3 = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	g2_drawBez3(surf, x0, y0, x1, y1, x2, y2, x3, y3, color);
	return noError;
}

static const char *proto_surf_drawText = "void drawText(gxSurf surf, int32 x, int32 y, gxSurf font, char text[*], int32 color)";
static const char *proto_surf_drawTextRoi = "void drawText(gxSurf surf, const gxRect roi&, gxSurf font, char text[*], int32 color)";
static vmError surf_drawText(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;

	if (ctx->proto == proto_surf_drawTextRoi) {
		gx_Rect roi = nextValue(ctx).ref;
		gx_Surf font = nextValue(ctx).ref;
		char *text = nextValue(ctx).ref;
		uint32_t color = nextValue(ctx).u32;
		g2_drawTextRoi(surf, roi, font, text, color);
		return noError;
	}

	int x = nextValue(ctx).i32;
	int y = nextValue(ctx).i32;
	gx_Surf font = nextValue(ctx).ref;
	char *text = nextValue(ctx).ref;
	uint32_t color = nextValue(ctx).u32;

	g2_drawText(surf, x, y, font, text, color);
	return noError;
}

static const char *proto_surf_clipText = "void clipText(gxSurf font, gxRect rect&, char text[*])";
static vmError surf_clipText(nfcContext ctx) {
	gx_Surf font = nextValue(ctx).ref;
	gx_Rect rect = nextValue(ctx).ref;
	char *text = nextValue(ctx).ref;

	g2_clipText(rect, font, text);
	return noError;
}

static const char *proto_surf_copySurf = "void copy(gxSurf surf, int32 x, int32 y, gxSurf src, const gxRect roi&)";
static vmError surf_copySurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x = nextValue(ctx).i32;
	int y = nextValue(ctx).i32;
	gx_Surf src = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;

	if (gx_blitSurf(surf, x, y, src, roi, NULL, gx_getcbltf(surf->depth, src->depth)) < 0) {
		return nativeCallError;
	}
	return noError;
}

typedef struct bltContext {
	symn callback;
	int32_t alpha;
	rtContext rt;
} *bltContext;
static int blendDstAlphaCallback(argb* dst, argb *src, bltContext ctx, size_t cnt) {
	rtContext rt = ctx->rt;
	int ctxAlpha = ctx->alpha;
	for (size_t i = 0; i < cnt; ++i, ++dst, ++src) {
		struct vector args[2];
		args[0] = vecldc(*src);
		args[1] = vecldc(*dst);
		if (rt->api.invoke(rt, ctx->callback, args, args, NULL) != noError) {
			return -1;
		}
		register argb val = vecrgb(args);
		int alpha = ctxAlpha * dst->a >> 8;
		dst->r = clamp_s8(dst->r + alpha * (val.r - dst->r) / 256);
		dst->g = clamp_s8(dst->g + alpha * (val.g - dst->g) / 256);
		dst->b = clamp_s8(dst->b + alpha * (val.b - dst->b) / 256);
	}
	return 0;
}
static int blendAlphaCallback(argb* dst, argb *src, bltContext ctx, size_t cnt) {
	rtContext rt = ctx->rt;
	int alpha = ctx->alpha;
	for (size_t i = 0; i < cnt; ++i, ++dst, ++src) {
		struct vector args[2];
		args[0] = vecldc(*src);
		args[1] = vecldc(*dst);
		if (rt->api.invoke(rt, ctx->callback, args, args, NULL) != noError) {
			return -1;
		}
		register argb val = vecrgb(args);
		dst->r = clamp_s8(dst->r + alpha * (val.r - dst->r) / 256);
		dst->g = clamp_s8(dst->g + alpha * (val.g - dst->g) / 256);
		dst->b = clamp_s8(dst->b + alpha * (val.b - dst->b) / 256);
	}
	return 0;
}
static int blendDstAlpha(argb* dst, argb *src, bltContext ctx, size_t cnt) {
	int ctxAlpha = ctx->alpha;
	for (size_t i = 0; i < cnt; ++i, ++dst, ++src) {
		int alpha = ctxAlpha * dst->a >> 8;
		dst->r = clamp_s8(dst->r + alpha * (src->r - dst->r) / 256);
		dst->g = clamp_s8(dst->g + alpha * (src->g - dst->g) / 256);
		dst->b = clamp_s8(dst->b + alpha * (src->b - dst->b) / 256);
	}
	return 0;
	(void) ctx;
}
static int blendAlpha(argb* dst, argb *src, bltContext ctx, size_t cnt) {
	int alpha = ctx->alpha;
	for (size_t i = 0; i < cnt; ++i, ++dst, ++src) {
		dst->r = clamp_s8(dst->r + alpha * (src->r - dst->r) / 256);
		dst->g = clamp_s8(dst->g + alpha * (src->g - dst->g) / 256);
		dst->b = clamp_s8(dst->b + alpha * (src->b - dst->b) / 256);
	}
	return 0;
}

static const char *proto_surf_blendSurf = "void blend(gxSurf surf, int32 x, int32 y, const gxSurf src, const gxRect roi&, int32 alpha, bool dstAlpha, vec4f blend(vec4f base, vec4f with))";
static vmError surf_blendSurf(nfcContext ctx) {
	rtContext rt = ctx->rt;
	gx_Surf surf = nextValue(ctx).ref;
	int x = nextValue(ctx).i32;
	int y = nextValue(ctx).i32;
	gx_Surf src = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	int alpha = nextValue(ctx).i32;
	int dstAlpha = nextValue(ctx).i32;
	size_t cbOffs = argref(ctx, rt->api.nfcNextArg(ctx));
	symn callback = rt->api.rtLookup(ctx->rt, cbOffs);

	if (cbOffs != 0 && callback == NULL) {
		rt->api.raise(rt, raiseError, "Invalid callback");
		return nativeCallError;
	}

	if (surf->depth != 32 || src->depth != 32) {
		rt->api.raise(rt, raiseError, "Invalid depth: %d, in function: %T", surf->depth, ctx->sym);
		return nativeCallError;
	}

	struct bltContext arg = {
		.callback = callback,
		.alpha = alpha,
		.rt = rt,
	};

	cblt_proc blt = NULL;
	if (callback != NULL) {
		if (dstAlpha) {
			blt = (cblt_proc) blendDstAlphaCallback;
		} else {
			blt = (cblt_proc) blendAlphaCallback;
		}
	} else {
		if (dstAlpha) {
			blt = (cblt_proc) blendDstAlpha;
		} else {
			blt = (cblt_proc) blendAlpha;
		}
	}

	if (gx_blitSurf(surf, x, y, src, roi, &arg, blt) < 0) {
		return nativeCallError;
	}
	return noError;
}

static const char *proto_surf_transformSurf = "void transform(gxSurf surf, const gxRect rect&, gxSurf src, const gxRect roi&, int32 interpolate, float32 mat[16])";
static vmError surf_transformSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect rect = nextValue(ctx).ref;
	gx_Surf src = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	int interpolate = nextValue(ctx).i32;
	float32_t *mat = nextValue(ctx).ref;

	if (surf->depth != src->depth) {
		ctx->rt->api.raise(ctx->rt, raiseError, "Invalid source depth: %d, in function: %T", src->depth, ctx->sym);
		return nativeCallError;
	}

	struct gx_Rect srcRec;
	srcRec.x = srcRec.y = 0;
	srcRec.w = src->width;
	srcRec.h = src->height;
	if (roi != NULL) {
		if (roi->x > 0) {
			srcRec.w -= roi->x;
			srcRec.x = roi->x;
		}
		if (roi->y > 0) {
			srcRec.h -= roi->y;
			srcRec.y = roi->y;
		}
		if (roi->w < srcRec.w)
			srcRec.w = roi->w;
		if (roi->h < srcRec.h)
			srcRec.h = roi->h;
	}

	struct gx_Rect dstRec;
	if (rect != NULL) {
		dstRec = *rect;
	} else {
		dstRec.x = dstRec.y = 0;
		dstRec.w = surf->width;
		dstRec.h = surf->height;
	}

	if (dstRec.w <= 0 || dstRec.h <= 0) {
		return noError;
	}
	if (srcRec.w <= 0 || srcRec.h <= 0) {
		return noError;
	}

	if (dstRec.x < 0) {
		srcRec.x = -dstRec.x;
	}
	if (dstRec.y < 0) {
		srcRec.y = -dstRec.y;
	}

	char *dptr = gx_cliprect(surf, &dstRec);
	if (dptr == NULL) {
		return noError;
	}

	// convert floating point values to fixed point(16.16) values (scale + rotate + translate)
	int32_t xx = mat ? (int32_t) (mat[0] * 65535) : (srcRec.w << 16) / dstRec.w;
	int32_t xy = mat ? (int32_t) (mat[1] * 65535) : 0;
	int32_t xt = mat ? (int32_t) (mat[3] * 65535) : srcRec.x;
	int32_t yy = mat ? (int32_t) (mat[5] * 65535) : (srcRec.h << 16) / dstRec.h;
	int32_t yx = mat ? (int32_t) (mat[4] * 65535) : 0;
	int32_t yt = mat ? (int32_t) (mat[7] * 65535) : srcRec.y;

	if (interpolate == 0) {
		for (int y = 0, sy = srcRec.y; y < dstRec.h; ++y, ++sy) {
			for (int x = 0, sx = srcRec.x; x < dstRec.w; ++x, ++sx) {
				int32_t tx = (xx * sx + xy * sy + xt + 0x8000) >> 16;
				int32_t ty = (yx * sx + yy * sy + yt + 0x8000) >> 16;
				gx_setpixel(surf, dstRec.x + x, dstRec.y + y, gx_getpixel(src, tx, ty));
			}
		}
		return noError;
	}

	for (int y = 0, sy = srcRec.y; y < dstRec.h; ++y, ++sy) {
		for (int x = 0, sx = srcRec.x; x < dstRec.w; ++x, ++sx) {
			int32_t tx = xx * sx + xy * sy + xt;
			int32_t ty = yx * sx + yy * sy + yt;
			gx_setpixel(surf, dstRec.x + x, dstRec.y + y, gx_getpix16(src, tx, ty));
		}
	}
	return noError;
}

static const char *proto_surf_blurSurf = "void blur(gxSurf surf, int32 radius, double sigma)";
static vmError surf_blurSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int radius = nextValue(ctx).i32;
	double sigma = nextValue(ctx).f64;

	if (gx_blurSurf(surf, radius, sigma) != 0) {
		return nativeCallError;
	}
	return noError;
}

static const char *proto_surf_calcHist = "void calcHist(gxSurf surf, const gxRect roi&, uint32 rgb, uint32 lut[256])";
static vmError surf_calcHist(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	uint32_t rgb = nextValue(ctx).u32;
	rtValue lut = nextValue(ctx);

	if (surf->depth != 32) {
		ctx->rt->api.raise(ctx->rt, raiseError, "Invalid depth: %d, in function: %T", surf->depth, ctx->sym);
		return nativeCallError;
	}

	/*if (lut.length != 256) {
		ctx->rt->api.raise(ctx, raiseError, "Invalid table size: %d (should be 256), in function: %T", lut.length, ctx->sym);
		return nativeCallError;
	}*/

	struct gx_Rect rect;
	rect.x = roi ? roi->x : 0;
	rect.y = roi ? roi->y : 0;
	rect.w = roi ? roi->w : surf->width;
	rect.h = roi ? roi->h : surf->height;

	const char *dptr = gx_cliprect(surf, &rect);
	if (dptr == NULL) {
		ctx->rt->api.raise(ctx->rt, raiseVerbose, "Empty roi, in function: %T", ctx->sym);
		return noError;
	}

	// init
	uint32_t histB[256];
	uint32_t histG[256];
	uint32_t histR[256];
	uint32_t histL[256];
	for (int i = 0; i < 256; i += 1) {
		histB[i] = 0;
		histG[i] = 0;
		histR[i] = 0;
		histL[i] = 0;
	}

	for (int y = 0; y < rect.h; y += 1) {
		argb *cBuff = (argb *) dptr;
		for (int x = 0; x < rect.w; x += 1) {
			histL[lum(*cBuff)] += 1;
			histB[cBuff->b] += 1;
			histG[cBuff->g] += 1;
			histR[cBuff->r] += 1;
			cBuff += 1;
		}
		dptr += surf->scanLen;
	}

	uint32_t max = 1;
	uint32_t useB = bch(rgb);
	uint32_t useG = gch(rgb);
	uint32_t useR = rch(rgb);
	uint32_t useL = ach(rgb);
	for (size_t i = 0; i < 256; i += 1) {
		histB[i] = histB[i] * useB >> 8;
		histG[i] = histG[i] * useG >> 8;
		histR[i] = histR[i] * useR >> 8;
		histL[i] = histL[i] * useL >> 8;
		if (max < histB[i]) {
			max = histB[i];
		}
		if (max < histG[i]) {
			max = histG[i];
		}
		if (max < histR[i]) {
			max = histR[i];
		}
		if (max < histL[i]) {
			max = histL[i];
		}
	}

	argb *data = lut.ref;
	for (size_t x = 0; x < 256; x += 1) {
		data[x].b = clamp_u8(histB[x] * 255 / max);
		data[x].g = clamp_u8(histG[x] * 255 / max);
		data[x].r = clamp_u8(histR[x] * 255 / max);
		data[x].a = clamp_u8(histL[x] * 255 / max);
	}

	return noError;
}

static const char *proto_surf_cLutSurf = "void colorMap(gxSurf surf, const gxRect roi&, const uint32 lut[256])";
static vmError surf_cLutSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	rtValue lut = nextValue(ctx);
	int useLuminosity = 0;

	if (surf->depth != 32) {
		ctx->rt->api.raise(ctx->rt, raiseError, "Invalid depth: %d, in function: %T", surf->depth, ctx->sym);
		return nativeCallError;
	}

	/*if (lut.length != 256) {
		ctx->rt->api.raise(ctx, raiseError, "Invalid table size: %d (should be 256), in function: %T", lut.length, ctx->sym);
		return nativeCallError;
	}*/

	struct gx_Rect rect;
	rect.x = roi ? roi->x : 0;
	rect.y = roi ? roi->y : 0;
	rect.w = roi ? roi->w : surf->width;
	rect.h = roi ? roi->h : surf->height;

	argb *lutPtr = (argb *) lut.ref;
	char *dstPtr = gx_cliprect(surf, &rect);
	if (dstPtr == NULL) {
		ctx->rt->api.raise(ctx->rt, raiseVerbose, "Empty roi, in function: %T", ctx->sym);
		return noError;
	}

	for (int i = 0; i < 256; ++i) {
		if (lutPtr[i].a != i) {
			useLuminosity = 1;
			break;
		}
	}

	if (!useLuminosity) {
		for (int y = 0; y < rect.h; y += 1) {
			argb *cBuff = (argb *) dstPtr;
			for (int x = 0; x < rect.w; x += 1) {
				cBuff->b = lutPtr[cBuff->b].b;
				cBuff->g = lutPtr[cBuff->g].g;
				cBuff->r = lutPtr[cBuff->r].r;
				cBuff += 1;
			}
			dstPtr += surf->scanLen;
		}
		return noError;
	}

	/*static const int32_t RGB2XYZ[12] = {
		fxp16(0.412453), fxp16(0.357580), fxp16(0.180423), 0,
		fxp16(0.212671), fxp16(0.715160), fxp16(0.072169), 0,
		fxp16(0.019334), fxp16(0.119193), fxp16(0.950227), 0,
	};
	static const int32_t XYZ2RGB[12] = {
		fxp16(+3.240479), fxp16(-1.537150), fxp16(-0.498535), 0,
		fxp16(-0.969256), fxp16(+1.875992), fxp16(+0.041556), 0,
		fxp16(+0.055648), fxp16(-0.204043), fxp16(+1.057311), 0,
	};

	static const int32_t RGB2YIQ[] = {
		fxp16(0.299), fxp16( 0.587), fxp16( 0.114), 0,
		fxp16(0.596), fxp16(-0.275), fxp16(-0.321), 0,
		fxp16(0.212), fxp16(-0.523), fxp16( 0.311), 0,
	};
	static const int32_t YIQ2RGB[] = {
		fxp16(1), fxp16( 0.956), fxp16( 0.621), 0,
		fxp16(1), fxp16(-0.272), fxp16(-0.647), 0,
		fxp16(1), fxp16(-1.105), fxp16( 1.702), 0,
	};*/

	static const int32_t RGB2LUV[] = {
		fxp16( 0.299), fxp16( 0.587), fxp16( 0.114), 0,
		fxp16(-0.147), fxp16(-0.289), fxp16( 0.437), 0,
		fxp16( 0.615), fxp16(-0.515), fxp16(-0.100), 0,
	};
	static const int32_t LUV2RGB[] = {
		fxp16(1), fxp16( 0.000), fxp16( 1.140), 0,
		fxp16(1), fxp16(-0.394), fxp16(-0.581), 0,
		fxp16(1), fxp16( 2.028), fxp16( 0.000), 0,
	};

	// use alpha channel as luminosity channel lookup for mapping
	const int32_t *rgb2luv = RGB2LUV;
	const int32_t *luv2rgb = LUV2RGB;
	for (int y = 0; y < rect.h; y += 1) {
		argb *cBuff = (argb *) dstPtr;

		for (int x = 0; x < rect.w; x += 1) {
			int32_t r = lutPtr[cBuff->r].r;
			int32_t g = lutPtr[cBuff->g].g;
			int32_t b = lutPtr[cBuff->b].b;

			int32_t l = (r * rgb2luv[0x0] + g * rgb2luv[0x1] + b * rgb2luv[0x2]) >> 16;
			int32_t u = (r * rgb2luv[0x4] + g * rgb2luv[0x5] + b * rgb2luv[0x6]) >> 16;
			int32_t v = (r * rgb2luv[0x8] + g * rgb2luv[0x9] + b * rgb2luv[0xa]) >> 16;

			l = lutPtr[clamp_s8(l)].a;

			cBuff->r = clamp_s8((l * luv2rgb[0x0] + u * luv2rgb[0x1] + v * luv2rgb[0x2]) >> 16);
			cBuff->g = clamp_s8((l * luv2rgb[0x4] + u * luv2rgb[0x5] + v * luv2rgb[0x6]) >> 16);
			cBuff->b = clamp_s8((l * luv2rgb[0x8] + u * luv2rgb[0x9] + v * luv2rgb[0xa]) >> 16);
			cBuff += 1;
		}
		dstPtr += surf->scanLen;
	}
	return noError;
}

static const char *proto_surf_cMatSurf = "void colorMat(gxSurf surf, const gxRect roi&, const float32 mat[16])";
static vmError surf_cMatSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	rtValue mat = nextValue(ctx);

	if (surf->depth != 32) {
		ctx->rt->api.raise(ctx->rt, raiseError, "Invalid depth: %d, in function: %T", surf->depth, ctx->sym);
		return nativeCallError;
	}

	/*if (mat.length != 16) {
		ctx->rt->api.raise(ctx, raiseError, "Invalid matrix size: %d (should be 16), in function: %T", surf->depth, ctx->sym);
		return nativeCallError;
	}*/

	// convert floating point values to fixed point(16.16) values
	int32_t cMat[16];
	float32_t *matPtr = mat.ref;
	for (int i = 0; i < 16; i++) {
		cMat[i] = fxp16(matPtr[i]);
	}

	struct gx_Rect rect;
	rect.x = roi ? roi->x : 0;
	rect.y = roi ? roi->y : 0;
	rect.w = roi ? roi->w : surf->width;
	rect.h = roi ? roi->h : surf->height;

	char *dptr = gx_cliprect(surf, &rect);
	if (dptr == NULL) {
		ctx->rt->api.raise(ctx->rt, raiseVerbose, "Empty roi, in function: %T", ctx->sym);
		return noError;
	}

	for (int y = 0; y < rect.h; y += 1) {
		argb *cBuff = (argb *) dptr;
		for (int x = 0; x < rect.w; x += 1) {
			int r = cBuff->r;
			int g = cBuff->g;
			int b = cBuff->b;
			const int a = 256;
			cBuff->r = clamp_s8((r * cMat[0x0] + g * cMat[0x1] + b * cMat[0x2] + a * cMat[0x3]) >> 16);
			cBuff->g = clamp_s8((r * cMat[0x4] + g * cMat[0x5] + b * cMat[0x6] + a * cMat[0x7]) >> 16);
			cBuff->b = clamp_s8((r * cMat[0x8] + g * cMat[0x9] + b * cMat[0xa] + a * cMat[0xb]) >> 16);
			cBuff += 1;
		}
		dptr += surf->scanLen;
	}
	return noError;
}

static const char *proto_surf_gradient = "void gradient(gxSurf surf, const gxRect roi&, int32 type, int alpha, bool repeat, bool invert, uint32 lut...)";
static vmError surf_gradient(nfcContext ctx) {
	struct gx_Clut lut;
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect rect = nextValue(ctx).ref;
	gradient_type type = nextValue(ctx).i32;
	int alpha = nextValue(ctx).i32;
	int repeat = nextValue(ctx).i32;
	int invert = nextValue(ctx).i32;
	rtValue colors = nextValue(ctx);
	uint32_t *colorArr = colors.ref;

	lut.count = 256;
	lut.flags = lut.trans = 0;

	int mid = alpha < 0 ? 0 : 255;
	alpha = 256 - clamp_u8(alpha > 0 ? alpha : -alpha);
	int dt = ((colors.length - 1) << 16) / (lut.count - 1);
	for (int i = 0; i < lut.count; i += 1) {
		int t = i * dt;
		int32_t c1 = colorArr[(t >> 16)];
		int32_t c2 = colorArr[(t >> 16) + 1];
		int32_t val = c1 + ((t & 0xffff) * (c2 - c1 + 1) >> 16);
		lut.data[i].val = clamp_s8((val - mid) * 255 / alpha + mid) << 24;
	}

	if (invert) {
		for (int i = 0; i < lut.count; i += 1) {
			lut.data[i].val = ~lut.data[i].val;
		}
	}
	if (repeat) {
		type |= flag_repeat;
	}
	type |= flag_alpha;
	gx_gradSurf(surf, rect, &lut, type);
	return noError;
}

static const char *proto_mesh_create = "gxMesh create(int32 size)";
static const char *proto_mesh_recycle = "gxMesh recycle(gxMesh recycle, int32 size)";
static vmError mesh_recycle(nfcContext ctx) {
	gx_Mesh recycle = NULL;
	int size = 32;
	if (ctx->proto == proto_mesh_recycle) {
		recycle = nextValue(ctx).ref;
		size = nextValue(ctx).i32;
	}
	if (ctx->proto == proto_mesh_create) {
		recycle = meshMemMgr(ctx->rt, NULL, sizeof(struct gx_Mesh));
		size = nextValue(ctx).i32;
	}
	gx_Mesh result = g3_createMesh(recycle, (size_t) size);
	if (result == NULL) {
		return nativeCallError;
	}
	retref(ctx, vmOffset(ctx->rt, result));
	return noError;
}

static const char *proto_mesh_destroy = "void destroy(gxMesh mesh)";
static vmError mesh_destroy(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;
	g3_destroyMesh(mesh);
	meshMemMgr(ctx->rt, mesh, 0);
	return noError;
}

static const char *proto_surf_drawMesh = "int32 drawMesh(gxSurf surf, gxMesh mesh, int32 mode)";
static vmError surf_drawMesh(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Mesh mesh = nextValue(ctx).ref;
	int32_t mode = nextValue(ctx).i32;

	reti32(ctx, g3_drawMesh(surf, mesh, NULL, cam, lights, mode));
	return noError;
}

static const char *proto_mesh_openObj = "gxMesh openObj(char path[*])";
static const char *proto_mesh_open3ds = "gxMesh open3ds(char path[*])";
static vmError mesh_open(nfcContext ctx) {
	char *path = nextValue(ctx).ref;

	gx_Mesh result = meshMemMgr(ctx->rt, NULL, sizeof(struct gx_Mesh));
	if (result == NULL) {
		return nativeCallError;
	}
	memset(result, 0, sizeof(struct gx_Mesh));
	g3_createMesh(result, 20);
	result->hasTex = result->hasNrm = 0;
	result->tricnt = result->vtxcnt = 0;
	if (ctx->proto == proto_mesh_openObj) {
		if (!g3_readObj(result, path)) {
			return nativeCallError;
		}
	}
	else if (ctx->proto == proto_mesh_open3ds) {
		if (!g3_read3ds(result, path)) {
			return nativeCallError;
		}
	}
	else {
		meshMemMgr(ctx->rt, result, 0);
		return nativeCallError;
	}

	retref(ctx, vmOffset(ctx->rt, result));
	return noError;
}

static const char *proto_mesh_saveObj = "void saveObj(gxMesh mesh, char path[*])";
static vmError mesh_save(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;
	char *path = nextValue(ctx).ref;

	if (mesh == NULL) {
		return nativeCallError;
	}

	if (!g3_saveObj(mesh, path)) {
		return nativeCallError;
	}
	return noError;
}

static const char *proto_mesh_normalize = "void normalize(gxMesh mesh, float32 tolerance, float32 center[3], float32 resize[3])";
static vmError mesh_normalize(nfcContext ctx) {
	struct vector args[2];
	gx_Mesh mesh = nextValue(ctx).ref;
	float32_t tolerance = nextValue(ctx).f32;
	vector center = a2Vec(&args[0], nextValue(ctx).ref);
	vector resize = a2Vec(&args[1], nextValue(ctx).ref);
	normMesh(mesh, tolerance, center, resize);
	return noError;
}

static const char *proto_mesh_addVertex = "int32 addVertex(gxMesh mesh, float32 x, float32 y, float32 z)";
static vmError mesh_addVertex(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;
	scalar pos[3];
	pos[0] = nextValue(ctx).f32;
	pos[1] = nextValue(ctx).f32;
	pos[2] = nextValue(ctx).f32;
	if (!addvtx(mesh, pos, NULL, NULL)) {
		// return nativeCallError;
		reti32(ctx, -1);
		return noError;
	}
	reti32(ctx, mesh->vtxcnt - 1);
	return noError;
}

static const char *proto_mesh_addFace3 = "int32 addFace(gxMesh mesh, int32 v1, int32 v2, int32 v3)";
static const char *proto_mesh_addFace4 = "int32 addFace(gxMesh mesh, int32 v1, int32 v2, int32 v3, int32 v4)";
static vmError mesh_addFace(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;
	int res = -1;
	if (ctx->proto == proto_mesh_addFace3) {
		int v1 = nextValue(ctx).i32;
		int v2 = nextValue(ctx).i32;
		int v3 = nextValue(ctx).i32;
		res = addtri(mesh, v1, v2, v3);
	}
	if (ctx->proto == proto_mesh_addFace4) {
		int v1 = nextValue(ctx).i32;
		int v2 = nextValue(ctx).i32;
		int v3 = nextValue(ctx).i32;
		int v4 = nextValue(ctx).i32;
		res = addquad(mesh, v1, v2, v3, v4);
	}
	if (!res) {
		// return nativeCallError;
		reti32(ctx, -1);
		return noError;
	}
	reti32(ctx, mesh->tricnt - 1);
	return noError;
}

static const char *proto_mesh_setVertexPos = "bool setVertex(gxMesh mesh, int32 idx, float32 x, float32 y, float32 z)";
static const char *proto_mesh_setVertexNrm = "bool setNormal(gxMesh mesh, int32 idx, float32 x, float32 y, float32 z)";
static const char *proto_mesh_setVertexTex = "bool setTexture(gxMesh mesh, int32 idx, float32 s, float32 t)";
static vmError mesh_setVertex(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;
	int32_t idx = nextValue(ctx).i32;
	int res = 0;
	if (ctx->proto == proto_mesh_setVertexPos) {
		scalar pos[3];
		pos[0] = nextValue(ctx).f32;
		pos[1] = nextValue(ctx).f32;
		pos[2] = nextValue(ctx).f32;
		res = setvtx(mesh, idx, pos, NULL, NULL);
	}
	if (ctx->proto == proto_mesh_setVertexNrm) {
		scalar nrm[3];
		nrm[0] = nextValue(ctx).f32;
		nrm[1] = nextValue(ctx).f32;
		nrm[2] = nextValue(ctx).f32;
		res = setvtx(mesh, idx, NULL, nrm, NULL);
	}
	if (ctx->proto == proto_mesh_setVertexTex) {
		scalar tex[2];
		tex[0] = nextValue(ctx).f32;
		tex[1] = nextValue(ctx).f32;
		res = setvtx(mesh, idx, NULL, NULL, tex);
	}
	reti32(ctx, res != 0);
	return noError;
}

typedef struct mainLoopArgs {
	rtContext rt;
	symn callback;
	vmError error;
	int64_t timeout;
	gxWindow window;
	struct {
		int32_t y;
		int32_t x;
		int32_t button;
		int32_t action;
		vmOffs closure;
	} event;

} *mainLoopArgs;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
static void exitMainLoop(mainLoopArgs args) {
	emscripten_cancel_main_loop();
	destroyWindow(args->window);
}
#else

static void exitMainLoop(mainLoopArgs args) {
	args->timeout = -1;
}
#endif

static void mainLoopCallback(mainLoopArgs args) {
	rtContext rt = args->rt;
	symn callback = args->callback;
	args->event.action = getWindowEvent(args->window, &args->event.button, &args->event.x, &args->event.y);
	if (args->event.action == WINDOW_CLOSE) {
		// window is closing, quit loop
		if (callback != NULL) {
			args->error = rt->api.invoke(rt, callback, NULL, &args->event, NULL);
		}
		return exitMainLoop(args);
	}
	if (args->event.action == 0) {
		// skip unknown events
		int64_t now = timeMillis();
		if (now < args->timeout) {
			return parkThread();
		}
		args->event.action = EVENT_TIMEOUT;
	}
	if (callback != NULL) {
		int32_t timeout;
		args->error = rt->api.invoke(rt, callback, &timeout, &args->event, args);
		if (args->error != noError || timeout < 0) {
			return exitMainLoop(args);
		}
		if (timeout >= 0) {
			if (timeout == 0) {
				args->timeout = INT64_MAX;
			}
			else {
				args->timeout = timeout + timeMillis();
			}
		}
	}
	else {
		if (args->event.button == 27) {
			// if there is no callback, exit wit esc key
			return exitMainLoop(args);
		}
		args->timeout = INT64_MAX;
	}
	flushWindow(args->window);
}

static const char *proto_window_show = "void showWindow(gxSurf surf, pointer closure, int32 onEvent(pointer closure, int32 action, int32 button, int32 x, int32 y))";
static vmError window_show(nfcContext ctx) {
	rtContext rt = ctx->rt;
	gx_Surf offScreen = nextValue(ctx).ref;
	size_t cbClosure = argref(ctx, rt->api.nfcNextArg(ctx));
	size_t cbOffs = argref(ctx, rt->api.nfcNextArg(ctx));

	struct mainLoopArgs args;
	args.rt = rt;
	args.callback = rt->api.rtLookup(ctx->rt, cbOffs);
	args.error = noError;
	args.timeout = 0;
	args.window = NULL;
	args.event.closure = (vmOffs) cbClosure;
	args.event.action = WINDOW_INIT;
	args.event.button = 0;
	args.event.x = 0;
	args.event.y = 0;

	if (args.callback != NULL) {
		vmError error = rt->api.invoke(rt, args.callback, &args.timeout, &args.event, &args);
		if (error != noError || args.timeout < 0) {
			return error;
		}
	}
	args.window = createWindow(offScreen);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg((void*)mainLoopCallback, &args, 0, 1);
#else
	for ( ; args.timeout >= 0; ) {
		mainLoopCallback(&args);
	}
#endif
	destroyWindow(args.window);
	return args.error;
}

static const char *proto_window_title = "void setTitle(char title[*])";
static vmError window_title(nfcContext ctx) {
	char *title = nextValue(ctx).ref;
	mainLoopArgs args = (mainLoopArgs) ctx->extra;
	if (args != NULL && args->window != NULL) {
		setWindowText(args->window, title);
	}
	return noError;
}


static const char *proto_camera_lookAt = "void lookAt(float32 eye[3], float32 at[3], float32 up[3])";
//static const char *proto_camera_ortho = "void orthographic(float32 left, float32 right, float32 bottom, float32 top, float32 near, float32 far)";
//static const char *proto_camera_perspective = "void perspective(float32 left, float32 right, float32 bottom, float32 top, float32 near, float32 far)";
static const char *proto_camera_projection = "void projection(float32 fovy, float32 aspect, float32 near, float32 far)";
static const char *proto_camera_move = "void move(float32 direction[3], float32 amount)";
static const char *proto_camera_rotate = "void rotate(float32 direction[3], float32 orbit[3], float32 angle)";
static const char *proto_camera_readUp = "void readUp(float32 result[3])";
static const char *proto_camera_readRight = "void readRight(float32 result[3])";
static const char *proto_camera_readForward = "void readForward(float32 result[3])";
static vmError camera_mgr(nfcContext ctx) {
	struct vector temArgs[3];
	if (ctx->proto == proto_camera_readUp) {
		float32_t *result = nextValue(ctx).ref;
		result[0] = cam->dirU.x;
		result[1] = cam->dirU.y;
		result[2] = cam->dirU.z;
		return noError;
	}
	if (ctx->proto == proto_camera_readRight) {
		float32_t *result = nextValue(ctx).ref;
		result[0] = cam->dirR.x;
		result[1] = cam->dirR.y;
		result[2] = cam->dirR.z;
		return noError;
	}
	if (ctx->proto == proto_camera_readForward) {
		float32_t *result = nextValue(ctx).ref;
		result[0] = cam->dirF.x;
		result[1] = cam->dirF.y;
		result[2] = cam->dirF.z;
		return noError;
	}

	if (ctx->proto == proto_camera_rotate) {
		vector dir = a2Vec(temArgs + 0, nextValue(ctx).ref);
		vector orbit = a2Vec(temArgs + 1, nextValue(ctx).ref);
		float32_t angle = nextValue(ctx).f32;
		camrot(cam, dir, orbit, angle);
		return noError;
	}
	if (ctx->proto == proto_camera_move) {
		vector dir = a2Vec(temArgs + 0, nextValue(ctx).ref);
		float32_t amount = nextValue(ctx).f32;
		cammov(cam, dir, amount);
		return noError;
	}

	if (ctx->proto == proto_camera_lookAt) {
		vector eye = a2Vec(temArgs + 0, nextValue(ctx).ref);
		vector at = a2Vec(temArgs + 1, nextValue(ctx).ref);
		vector up = a2Vec(temArgs + 2, nextValue(ctx).ref);
		camset(cam, eye, at, up);
		return noError;
	}

	if (ctx->proto == proto_camera_projection) {
		float32_t fovy = nextValue(ctx).f32;
		float32_t aspect = nextValue(ctx).f32;
		float32_t near = nextValue(ctx).f32;
		float32_t far = nextValue(ctx).f32;
		projv_mat(&cam->proj, fovy, aspect, near, far);
		return noError;
	}

	return illegalState;
}


static const char *proto_lights_enabled = "bool enabled(int32 light)";
static const char *proto_lights_enable = "void enable(int32 light, bool on)";
static const char *proto_lights_position = "void position(int32 light, float x, float y, float z)";
static const char *proto_lights_ambient = "void ambient(int32 light, float r, float g, float b)";
static const char *proto_lights_diffuse = "void diffuse(int32 light, float r, float g, float b)";
static const char *proto_lights_specular = "void specular(int32 light, float r, float g, float b)";
static const char *proto_lights_attenuation = "void attenuation(int32 light, float constant, float linear, float quadratic)";
static vmError lights_manager(nfcContext ctx) {
	size_t light = nextValue(ctx).i32;

	if (light >= (sizeof(lights) / sizeof(*lights))) {
		return nativeCallError;
	}

	if (ctx->proto == proto_lights_enabled) {
		reti32(ctx, lights[light].attr);
		return noError;
	}

	if (ctx->proto == proto_lights_enable) {
		int on = nextValue(ctx).i32;
		lights[light].attr = on;
		return noError;
	}

	if (ctx->proto == proto_lights_position) {
		float32_t x = nextValue(ctx).f32;
		float32_t y = nextValue(ctx).f32;
		float32_t z = nextValue(ctx).f32;
		vecldf(&lights[light].pos, x, y, z, 1);
		return noError;
	}

	if (ctx->proto == proto_lights_ambient) {
		float32_t r = nextValue(ctx).f32;
		float32_t g = nextValue(ctx).f32;
		float32_t b = nextValue(ctx).f32;
		vecldf(&lights[light].ambi, r, g, b, 1);
		return noError;
	}

	if (ctx->proto == proto_lights_diffuse) {
		float32_t r = nextValue(ctx).f32;
		float32_t g = nextValue(ctx).f32;
		float32_t b = nextValue(ctx).f32;
		vecldf(&lights[light].diff, r, g, b, 1);
		return noError;
	}

	if (ctx->proto == proto_lights_specular) {
		float32_t r = nextValue(ctx).f32;
		float32_t g = nextValue(ctx).f32;
		float32_t b = nextValue(ctx).f32;
		vecldf(&lights[light].spec, r, g, b, 0);
		return noError;
	}

	if (ctx->proto == proto_lights_attenuation) {
		float32_t constant = nextValue(ctx).f32;
		float32_t linear = nextValue(ctx).f32;
		float32_t quadratic = nextValue(ctx).f32;
		vecldf(&lights[light].attn, constant, linear, quadratic, 0);
		return noError;
	}

	return illegalState;
}

static const char *proto_material_ambient = "void ambient(gxMesh mesh, float r, float g, float b)";
static const char *proto_material_diffuse = "void diffuse(gxMesh mesh, float r, float g, float b)";
static const char *proto_material_specular = "void specular(gxMesh mesh, float r, float g, float b)";
static const char *proto_material_shine = "void shine(gxMesh mesh, float value)";
static const char *proto_material_texture = "void texture(gxMesh mesh, gxSurf surf)";
static vmError mesh_material(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;

	if (ctx->proto == proto_material_ambient) {
		float32_t r = nextValue(ctx).f32;
		float32_t g = nextValue(ctx).f32;
		float32_t b = nextValue(ctx).f32;
		vecldf(&mesh->mtl.ambi, r, g, b, 1);
		return noError;
	}

	if (ctx->proto == proto_material_diffuse) {
		float32_t r = nextValue(ctx).f32;
		float32_t g = nextValue(ctx).f32;
		float32_t b = nextValue(ctx).f32;
		vecldf(&mesh->mtl.diff, r, g, b, 1);
		return noError;
	}

	if (ctx->proto == proto_material_specular) {
		float32_t r = nextValue(ctx).f32;
		float32_t g = nextValue(ctx).f32;
		float32_t b = nextValue(ctx).f32;
		vecldf(&mesh->mtl.spec, r, g, b, 0);
		return noError;
	}

	if (ctx->proto == proto_material_shine) {
		mesh->mtl.spow = nextValue(ctx).f32;
		return noError;
	}

	if (ctx->proto == proto_material_texture) {
		mesh->map = nextValue(ctx).ref;
		return noError;
	}

	return illegalState;
}

symn typSigned(rtContext rt, int size) {
	switch (size) {
		default:
			break;

		case 1:
			return rt->api.ccLookup(rt, NULL, "int8");

		case 2:
			return rt->api.ccLookup(rt, NULL, "int16");

		case 4:
			return rt->api.ccLookup(rt, NULL, "int32");

		case 8:
			return rt->api.ccLookup(rt, NULL, "int64");
	}
	return NULL;
}

#define offsetOf(__TYPE, __FIELD) ((size_t) &((__TYPE*)NULL)->__FIELD)
#define sizeOf(__TYPE, __FIELD) sizeof(((__TYPE*)NULL)->__FIELD)

int cmplInit(rtContext rt) {

	struct {
		vmError (*func)(nfcContext);
		const char *proto;
	}
	nfcSurf[] = {
		{surf_recycle,  proto_surf_create2d},
		{surf_recycle,  proto_surf_create3d},
		{surf_recycle,  proto_surf_recycle},
		{surf_destroy,  proto_surf_destroy},
		{surf_open,     proto_surf_openBmp},
		{surf_open,     proto_surf_openPng},
		{surf_open,     proto_surf_openJpg},
		{surf_open,     proto_surf_openFnt},
		{surf_save,     proto_surf_saveBmp},

		{surf_width,    proto_surf_width},
		{surf_height,   proto_surf_height},
		{surf_depth,    proto_surf_depth},
		{surf_get,      proto_surf_get},
		{surf_set,      proto_surf_set},
		{surf_tex,      proto_surf_tex},
		{surf_drawRect, proto_surf_drawRect},
		{surf_fillRect, proto_surf_fillRect},
		{surf_drawOval, proto_surf_drawOval},
		{surf_fillOval, proto_surf_fillOval},
		{surf_drawLine, proto_surf_drawLine},
		{surf_drawBez2, proto_surf_drawBez2},
		{surf_drawBez3, proto_surf_drawBez3},
		{surf_clipText, proto_surf_clipText},
		{surf_drawText, proto_surf_drawText},
		{surf_drawText, proto_surf_drawTextRoi},
		{surf_copySurf, proto_surf_copySurf},
		{surf_blendSurf, proto_surf_blendSurf},
		{surf_transformSurf, proto_surf_transformSurf},
		{surf_blurSurf, proto_surf_blurSurf},
		{surf_cLutSurf, proto_surf_cLutSurf},
		{surf_cMatSurf, proto_surf_cMatSurf},
		{surf_gradient, proto_surf_gradient},
		{surf_calcHist, proto_surf_calcHist},
		{surf_drawMesh, proto_surf_drawMesh},
	},
	nfcWindow[] = {
		{window_show, proto_window_show},
		{window_title, proto_window_title},
	},
	nfcCamera[] = {
		{camera_mgr, proto_camera_projection},
		{camera_mgr, proto_camera_lookAt},

		{camera_mgr, proto_camera_readUp},
		{camera_mgr, proto_camera_readRight},
		{camera_mgr, proto_camera_readForward},

		{camera_mgr, proto_camera_rotate},
		{camera_mgr, proto_camera_move},
	},
	nfcMesh[] = {
		{mesh_recycle, proto_mesh_create},
		{mesh_recycle, proto_mesh_recycle},
		{mesh_destroy, proto_mesh_destroy},
		{mesh_open, proto_mesh_openObj},
		{mesh_open, proto_mesh_open3ds},
		{mesh_save, proto_mesh_saveObj},
		{mesh_normalize, proto_mesh_normalize},
		{mesh_addVertex, proto_mesh_addVertex},
		{mesh_addFace, proto_mesh_addFace3},
		{mesh_addFace, proto_mesh_addFace4},
		{mesh_setVertex, proto_mesh_setVertexPos},
		{mesh_setVertex, proto_mesh_setVertexNrm},
		{mesh_setVertex, proto_mesh_setVertexTex},
		{mesh_material, proto_material_ambient},
		{mesh_material, proto_material_diffuse},
		{mesh_material, proto_material_specular},
		{mesh_material, proto_material_shine},
		{mesh_material, proto_material_texture},
	}, nfcLights[] = {
		{lights_manager, proto_lights_enabled},
		{lights_manager, proto_lights_enable},
		{lights_manager, proto_lights_position},
		{lights_manager, proto_lights_ambient},
		{lights_manager, proto_lights_diffuse},
		{lights_manager, proto_lights_specular},
		{lights_manager, proto_lights_attenuation},
	};

	ccContext cc = rt->cc;

	// rectangle in 2d
	rt->api.ccAddUnit(cc, NULL, NULL, 0, "struct gxRect:1 {\n"
		"	int32 x;\n"
		"	int32 y;\n"
		"	int32 w;\n"
		"	int32 h;\n"
		"}\n"
	);

	// surfaces are allocated outside the vm, and are handler types
	symn symSurf = rt->api.ccAddType(cc, "gxSurf", sizeof(gx_Surf), 0);

	// meshes are allocated inside the vm, and are reference types
	symn symMesh = rt->api.ccAddType(cc, "gxMesh", sizeof(struct gx_Mesh), 1);

	if (rt->api.ccExtend(cc, symMesh)) {
		for (size_t i = 0; i < sizeof(nfcMesh) / sizeof(*nfcMesh); i += 1) {
			if (!rt->api.ccAddCall(cc, nfcMesh[i].func, nfcMesh[i].proto)) {
				return 1;
			}
		}

		// clear color and depth buffers
		rt->api.ccDefInt(cc, "clearDepth", zero_zbuf);
		rt->api.ccDefInt(cc, "clearColor", zero_cbuf);

		// backface culling
		rt->api.ccDefInt(cc, "cullBack", cull_back);
		rt->api.ccDefInt(cc, "cullFront", cull_front);

		rt->api.ccDefInt(cc, "drawPlot", draw_plot);
		rt->api.ccDefInt(cc, "drawWire", draw_wire);
		rt->api.ccDefInt(cc, "drawFill", draw_fill);
		rt->api.ccDefInt(cc, "drawMode", draw_mode);

		rt->api.ccDefInt(cc, "useTexture", draw_tex);
		rt->api.ccDefInt(cc, "useLights", draw_lit);

		symn vtxCount = rt->api.ccDefVar(cc, "vertices", typSigned(rt, sizeOf(struct gx_Mesh, vtxcnt)));
		symn triCount = rt->api.ccDefVar(cc, "triangles", typSigned(rt, sizeOf(struct gx_Mesh, tricnt)));
		symn segCount = rt->api.ccDefVar(cc, "segments", typSigned(rt, sizeOf(struct gx_Mesh, segcnt)));

		ccKind symKind = symMesh->kind;
		symMesh->kind &= ~ATTR_stat;
		symMesh->kind |= ATTR_cnst;
		rt->api.ccEnd(cc, symMesh);
		symMesh->kind = symKind;

		vtxCount->offs = offsetOf(struct gx_Mesh, vtxcnt);
		triCount->offs = offsetOf(struct gx_Mesh, tricnt);
		segCount->offs = offsetOf(struct gx_Mesh, segcnt);
	}

	// type: gxSurf
	if (rt->api.ccExtend(cc, symSurf)) {
		for (size_t i = 0; i < sizeof(nfcSurf) / sizeof(*nfcSurf); i += 1) {
			if (!rt->api.ccAddCall(cc, nfcSurf[i].func, nfcSurf[i].proto)) {
				return 1;
			}
		}
		rt->api.ccEnd(cc, symSurf);
	}

	symn symCam = rt->api.ccBegin(cc, "camera");
	if (symCam != NULL) {
		for (size_t i = 0; i < sizeof(nfcCamera) / sizeof(*nfcCamera); i += 1) {
			if (!rt->api.ccAddCall(cc, nfcCamera[i].func, nfcCamera[i].proto)) {
				return 1;
			}
		}
		rt->api.ccEnd(cc, symCam);
	}

	symn symLights = rt->api.ccBegin(cc, "lights");
	if (symLights != NULL) {
		for (size_t i = 0; i < sizeof(nfcLights) / sizeof(*nfcLights); i += 1) {
			if (!rt->api.ccAddCall(cc, nfcLights[i].func, nfcLights[i].proto)) {
				return 1;
			}
		}
		rt->api.ccEnd(cc, symLights);
	}


	symn gui = rt->api.ccBegin(cc, "Gui");
	if (gui != NULL) {
		rt->api.ccDefInt(cc, "KEY_PRESS", KEY_PRESS);
		rt->api.ccDefInt(cc, "KEY_RELEASE", KEY_RELEASE);
		rt->api.ccDefInt(cc, "MOUSE_PRESS", MOUSE_PRESS);
		rt->api.ccDefInt(cc, "MOUSE_MOTION", MOUSE_MOTION);
		rt->api.ccDefInt(cc, "MOUSE_RELEASE", MOUSE_RELEASE);
		rt->api.ccDefInt(cc, "FINGER_PRESS", FINGER_PRESS);
		rt->api.ccDefInt(cc, "FINGER_MOTION", FINGER_MOTION);
		rt->api.ccDefInt(cc, "FINGER_RELEASE", FINGER_RELEASE);
		rt->api.ccDefInt(cc, "EVENT_TIMEOUT", EVENT_TIMEOUT);

		rt->api.ccDefInt(cc, "WINDOW_INIT", WINDOW_INIT);
		rt->api.ccDefInt(cc, "WINDOW_CLOSE", WINDOW_CLOSE);
		rt->api.ccDefInt(cc, "WINDOW_ENTER", WINDOW_ENTER);
		rt->api.ccDefInt(cc, "WINDOW_LEAVE", WINDOW_LEAVE);

		rt->api.ccDefInt(cc, "KEY_MASK_SHIFT", KEY_MASK_SHIFT);
		rt->api.ccDefInt(cc, "KEY_MASK_CONTROL", KEY_MASK_CONTROL);

		for (size_t i = 0; i < sizeof(nfcWindow) / sizeof(*nfcWindow); i += 1) {
			if (!rt->api.ccAddCall(cc, nfcWindow[i].func, nfcWindow[i].proto)) {
				return 1;
			}
		}
		rt->api.ccEnd(cc, gui);
	}

	if (1) {
		struct vector eye, tgt, up;
		vecldf(&eye, 0, 0, 2.0f, 1);
		vecldf(&tgt, 0, 0, 0, 1);
		vecldf(&up, 0, 1, 0, 1);

		projv_mat(&cam->proj, 30, 1, 1, 100);
		camset(cam, &eye, &tgt, &up);

		memset(lights, 0, sizeof(lights));
		for (size_t i = 1; i < sizeof(lights) / sizeof(*lights); ++i) {
			lights[i - 1].next = lights + i;
		}
	}
	return 0;
}