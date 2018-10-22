#include <cmpl.h>

#include "g3_math.h"
#include "gx_color.h"

#include "gx_gui.h"
#include "gx_surf.h"
#include "g3_draw.h"

struct camera cam[1]; // TODO: singleton like camera

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
	gx_Surf surf = nextValue(ctx).ref;
	reti32(ctx, surf->width);
	return noError;
}

static const char *proto_surf_height = "int32 height(gxSurf surf)";
static vmError surf_height(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	reti32(ctx, surf->height);
	return noError;
}

static const char *proto_surf_depth = "int32 depth(gxSurf surf)";
static vmError surf_depth(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	reti32(ctx, surf->depth);
	return noError;
}


static const char *proto_surf_get = "int32 get(gxSurf surf, int x, int y)";
static vmError surf_get(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x = nextValue(ctx).i32;
	int y = nextValue(ctx).i32;

	reti32(ctx, gx_getpixel(surf, x, y));
	return noError;
}

static const char *proto_surf_set = "void set(gxSurf surf, int x, int y, uint32 color)";
static vmError surf_set(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int32_t x = nextValue(ctx).i32;
	int32_t y = nextValue(ctx).i32;
	uint32_t color = nextValue(ctx).u32;

	gx_setpixel(surf, x, y, color);
	return noError;
}

static const char *proto_surf_drawRect = "void drawRect(gxSurf surf, int x1, int y1, int x2, int y2, int32 color)";
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

static const char *proto_surf_fillRect = "void fillRect(gxSurf surf, int x1, int y1, int x2, int y2, int32 color)";
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

static const char *proto_surf_drawOval = "void drawOval(gxSurf surf, int x1, int y1, int x2, int y2, int32 color)";
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

static const char *proto_surf_fillOval = "void fillOval(gxSurf surf, int x1, int y1, int x2, int y2, int32 color)";
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

static const char *proto_surf_drawLine = "void drawLine(gxSurf surf, int x1, int y1, int x2, int y2, int32 color)";
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

static const char *proto_surf_drawBez2 = "void drawBezier(gxSurf surf, int x1, int y1, int x2, int y2, int x3, int y3, int32 color)";
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

static const char *proto_surf_drawBez3 = "void drawBezier(gxSurf surf, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int32 color)";
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

static const char *proto_surf_drawText = "void drawText(gxSurf surf, int x, int y, gxSurf font, char text[*], int32 color)";
static vmError surf_drawText(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
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

static const char *proto_surf_copySurf = "void copySurf(gxSurf surf, int x, int y, gxSurf src, const gxRect roi&)";
static vmError surf_copySurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	int x = nextValue(ctx).i32;
	int y = nextValue(ctx).i32;
	gx_Surf src = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;

	gx_copySurf(surf, x, y, src, roi);
	return noError;
}

static const char *proto_surf_zoomSurf = "void zoomSurf(gxSurf surf, const gxRect rect&, gxSurf src, const gxRect roi&, int interpolate)";
static vmError surf_zoomSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect rect = nextValue(ctx).ref;
	gx_Surf src = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	int interpolate = nextValue(ctx).i32;

	gx_zoomSurf(surf, rect, src, roi, interpolate);
	return noError;
}

static const char *proto_surf_cLutSurf = "void lookup(gxSurf surf, const gxRect roi&, uint32 lut[])";
static vmError surf_cLutSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	rtValue lut = nextValue(ctx);
	struct gx_Clut cLut;
	cLut.flags = 0;
	cLut.trans = 0;
	cLut.count = lut.length < 256 ? lut.length : 256;
	memcpy(&cLut.data, lut.ref, cLut.count * sizeof(uint32_t));
	gx_cLutSurf(surf, roi, &cLut);
	return noError;
}

static const char *proto_surf_cMatSurf = "void multiply(gxSurf surf, const gxRect roi&, float32 mat[16])";
static vmError surf_cMatSurf(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Rect roi = nextValue(ctx).ref;
	float32_t *mat = nextValue(ctx).ref;
	int32_t cMat[16];
	for (int i = 0; i < 16; i++) {
		cMat[i] = (int32_t) (mat[i] * 65535);
	}
	gx_cMatSurf(surf, roi, cMat);
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

static const char *proto_surf_drawMesh = "void drawMesh(gxSurf surf, gxMesh mesh)";
static vmError surf_drawMesh(nfcContext ctx) {
	gx_Surf surf = nextValue(ctx).ref;
	gx_Mesh mesh = nextValue(ctx).ref;

	int mode = zero_cbuf | zero_zbuf | draw_fill | cull_back;
	g3_drawMesh(surf, mesh, NULL, cam, mode);
	//g3_drawBbox(surf, mesh, NULL, cam);
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
		g3_readObj(result, path);
	}
	else if (ctx->proto == proto_mesh_open3ds) {
		g3_read3ds(result, path);
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

	g3_saveObj(mesh, path);
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

static const char *proto_mesh_addVertex = "void addVertex(gxMesh mesh, float32 x, float32 y, float32 z)";
static vmError mesh_addVertex(nfcContext ctx) {
	gx_Mesh mesh = nextValue(ctx).ref;
	float32_t x = nextValue(ctx).f32;
	float32_t y = nextValue(ctx).f32;
	float32_t z = nextValue(ctx).f32;
	struct vector pos;
	addvtx(mesh, vecldf(&pos, x, y, z, 0), NULL, NULL);
	return noError;
}


static const char *proto_window_show = "void showWindow(gxSurf surf, int onEvent(int action, int button, int x, int y))";
static vmError window_show(nfcContext ctx) {
	rtContext rt = ctx->rt;
	gx_Surf offScreen = nextValue(ctx).ref;
	size_t cbOffs = argref(ctx, rt->api.nfcNextArg(ctx));
	symn callback = rt->api.rtLookup(ctx->rt, cbOffs);

	int waitEvent = 1;
	flip_scr flipScreen = NULL;
	peek_msg peekMessage = NULL;
	initWin(offScreen, &flipScreen, &peekMessage);
	for ( ; ; ) {
		struct {
			int32_t y;
			int32_t x;
			int32_t button;
			int32_t action;
		} event = {
			.action = waitEvent,
			.button = 0,
			.x = 0,
			.y = 0
		};

		int msg = peekMessage(&event.action, &event.button, &event.x, &event.y);
		if (msg < 0) {
			break;
		}
		if (callback != NULL) {
			rt->api.invoke(rt, callback, &waitEvent, &event, NULL);
			if (waitEvent < 0) {
				break;
			}
		}
		else {
			if (event.action == KEY_RELEASE && event.button == 27) {
				break;
			}
		}

		flipScreen(offScreen);
	}
	if (callback != NULL) {
		struct {
			int32_t y;
			int32_t x;
			int32_t button;
			int32_t action;
		} event = {
			.action = WINDOW_CLOSE,
			.button = 0,
			.x = 0,
			.y = 0
		};

		rt->api.invoke(rt, callback, NULL, &event, NULL);
	}
	doneWin();
	return noError;
}

static const char *proto_camera_lookAt = "void lookAt(float32 eye[3], float32 at[3], float32 up[3])";
//static const char *proto_camera_ortho = "void orthographic(float32 left, float32 right, float32 bottom, float32 top, float32 near, float32 far)";
//static const char *proto_camera_persp = "void perspective(float32 left, float32 right, float32 bottom, float32 top, float32 near, float32 far)";
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

symn typSigned(rtContext rt, int size) {
	char *typeName = "void";
	switch (size) {
		case 1:
			typeName = "int8";
			break;
		case 2:
			typeName = "int16";
			break;
		case 4:
			typeName = "int32";
			break;
		case 8:
			typeName = "int64";
			break;
	}
	return rt->api.ccLookup(rt, NULL, typeName);
}

#define offsetOf(__TYPE, __FIELD) ((size_t) &((__TYPE*)NULL)->__FIELD)
#define sizeOf(__TYPE, __FIELD) sizeof(((__TYPE*)NULL)->__FIELD)

int cmplInit(rtContext rt) {

	struct {
		vmError (*func)(nfcContext);
		const char *proto;
	} gfx[] = {
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
		{surf_drawRect, proto_surf_drawRect},
		{surf_fillRect, proto_surf_fillRect},
		{surf_drawOval, proto_surf_drawOval},
		{surf_fillOval, proto_surf_fillOval},
		{surf_drawLine, proto_surf_drawLine},
		{surf_drawBez2, proto_surf_drawBez2},
		{surf_drawBez3, proto_surf_drawBez3},
		{surf_clipText, proto_surf_clipText},
		{surf_drawText, proto_surf_drawText},
		{surf_copySurf, proto_surf_copySurf},
		{surf_zoomSurf, proto_surf_zoomSurf},
		{surf_cLutSurf, proto_surf_cLutSurf},
		{surf_cMatSurf, proto_surf_cMatSurf},
		{surf_drawMesh, proto_surf_drawMesh},
	}, win[] = {
		{window_show, proto_window_show},
		//{surf_, proto_surf_},
	}, nfcCam[] = {
		{camera_mgr, proto_camera_projection},
		{camera_mgr, proto_camera_lookAt},

		{camera_mgr, proto_camera_readUp},
		{camera_mgr, proto_camera_readRight},
		{camera_mgr, proto_camera_readForward},

		{camera_mgr, proto_camera_rotate},
		{camera_mgr, proto_camera_move},
//		{cam_, proto_cam},
	}, mesh[] = {
		{mesh_recycle, proto_mesh_create},
		{mesh_recycle, proto_mesh_recycle},
		{mesh_destroy, proto_mesh_destroy},
		{mesh_open, proto_mesh_openObj},
		{mesh_open, proto_mesh_open3ds},
		{mesh_save, proto_mesh_saveObj},
		{mesh_normalize, proto_mesh_normalize},
		{mesh_addVertex, proto_mesh_addVertex},
//		{mesh_add, proto_mesh_add},
		// init
		// destroy
		// readObj
		// read3ds
		// saveObj
	};

	ccContext cc = rt->cc;

	// rectangle in 2d
	rt->api.ccAddCode(cc, __FILE__, __LINE__, "struct gxRect:1 {\n"
		"	int32 x;\n"
		"	int32 y;\n"
		"	int32 w;\n"
		"	int32 h;\n"
		"}\n"
	);

	// meshes are allocated inside the vm, and are reference types
	symn symMesh = rt->api.ccAddType(cc, "gxMesh", sizeof(struct gx_Mesh), 1);
	if (rt->api.ccExtend(cc, symMesh)) {
		for (int i = 0; i < sizeof(mesh) / sizeof(*mesh); i += 1) {
			if (!rt->api.ccAddCall(cc, mesh[i].func, mesh[i].proto)) {
				return 1;
			}
		}

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
	symn symSurf = rt->api.ccAddType(cc, "gxSurf", sizeof(gx_Surf), 0);
	if (rt->api.ccExtend(cc, symSurf)) {
		for (int i = 0; i < sizeof(gfx) / sizeof(*gfx); i += 1) {
			if (!rt->api.ccAddCall(cc, gfx[i].func, gfx[i].proto)) {
				return 1;
			}
		}
		rt->api.ccEnd(cc, symSurf);
	}

	symn symCam = rt->api.ccBegin(cc, "camera");
	if (symCam != NULL) {
		for (int i = 0; i < sizeof(nfcCam) / sizeof(*nfcCam); i += 1) {
			if (!rt->api.ccAddCall(cc, nfcCam[i].func, nfcCam[i].proto)) {
				return 1;
			}
		}
		rt->api.ccEnd(cc, symCam);
	}


	symn gui = rt->api.ccBegin(cc, "Gui");
	if (gui != NULL) {
		rt->api.ccDefInt(cc, "KEY_PRESS", KEY_PRESS);
		rt->api.ccDefInt(cc, "KEY_RELEASE", KEY_RELEASE);
		rt->api.ccDefInt(cc, "MOUSE_PRESS", MOUSE_PRESS);
		rt->api.ccDefInt(cc, "MOUSE_MOTION", MOUSE_MOTION);
		rt->api.ccDefInt(cc, "MOUSE_RELEASE", MOUSE_RELEASE);
		rt->api.ccDefInt(cc, "WINDOW_CLOSE", WINDOW_CLOSE);
		rt->api.ccDefInt(cc, "KEY_MASK_SHIFT", KEY_MASK_SHIFT);
		rt->api.ccDefInt(cc, "KEY_MASK_CONTROL", KEY_MASK_CONTROL);

		for (int i = 0; i < sizeof(win) / sizeof(*win); i += 1) {
			if (!rt->api.ccAddCall(cc, win[i].func, win[i].proto)) {
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
	}
	return 0;
}