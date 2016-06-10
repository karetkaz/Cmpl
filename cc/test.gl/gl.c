//~ http://pyopengl.sourceforge.net/documentation/manual-3.0/index.html
//~ sudo apt-get install freeglut3:i386
#include "plugin.h"
#include <stdlib.h>
#include <GL/glut.h>

#define debugGL(msg, ...) fprintf(stdout, "%s:%d:debug:"msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)

//#{ gl wrapper functions
static vmError glFun_Viewport(libcContext rt) {
	int x = argi32(rt, 4 * 0);
	int y = argi32(rt, 4 * 1);
	int w = argi32(rt, 4 * 2);
	int h = argi32(rt, 4 * 3);
	glViewport(x, y, w, h);
	return 0;
}
static vmError glFun_ShadeModel(libcContext rt) {
	glShadeModel(argi32(rt, 0));
	return 0;
}
static vmError glFun_CullFace(libcContext rt) {
	glCullFace(argi32(rt, 0));
	return 0;
}
static vmError glFun_Enable(libcContext rt) {
	glEnable(argi32(rt, 0));
	return 0;
}
static vmError glFun_Disable(libcContext rt) {
	glDisable(argi32(rt, 0));
	return 0;
}
static vmError glFun_Clear(libcContext rt) {
	glClear(argi32(rt, 0));
	return 0;
}
static vmError glFun_Flush(libcContext rt) {
	glFlush();
	return 0;
}

static vmError glFun_Begin(libcContext rt) {
	glBegin(argi32(rt, 0));
	return 0;
}
static vmError glFun_End(libcContext rt) {
	glEnd();
	return 0;
}

static vmError glFun_Vertex(libcContext rt) {
	float x = argf32(rt, 4 * 0);
	float y = argf32(rt, 4 * 1);
	float z = argf32(rt, 4 * 2);
	float w = argf32(rt, 4 * 3);
	glVertex4f(x, y, z, w);
	return 0;
}
static vmError glFun_Normal(libcContext rt) {
	float x = argf32(rt, 4 * 0);
	float y = argf32(rt, 4 * 1);
	float z = argf32(rt, 4 * 2);
	//~ float w = argf32(rt, 12);	// we have this value on the stack, but we don't need it.
	glNormal3f(x, y, z);
	return 0;
}
static vmError glFun_Color(libcContext rt) {
	float r = argf32(rt, 4 * 0);
	float g = argf32(rt, 4 * 1);
	float b = argf32(rt, 4 * 2);
	float a = argf32(rt, 4 * 3);
	glColor4f(r, g, b, a);
	return 0;
}

// matrix operations
static vmError glFun_MatrixMode(libcContext rt) {
	glMatrixMode(argi32(rt, 0));
	return 0;
}

static vmError glFun_Frustum(libcContext rt) {
	GLdouble left   = argf64(rt, 8 * 0);
	GLdouble right  = argf64(rt, 8 * 1);
	GLdouble bottom = argf64(rt, 8 * 2);
	GLdouble top    = argf64(rt, 8 * 3);
	GLdouble znear  = argf64(rt, 8 * 4);
	GLdouble zfar   = argf64(rt, 8 * 5);
	glFrustum(left, right, bottom, top, znear, zfar);
	return 0;
}
static vmError glFun_Ortho(libcContext rt) {
	GLdouble left   = argf64(rt, 8 * 0);
	GLdouble right  = argf64(rt, 8 * 1);
	GLdouble bottom = argf64(rt, 8 * 2);
	GLdouble top    = argf64(rt, 8 * 3);
	GLdouble znear  = argf64(rt, 8 * 4);
	GLdouble zfar   = argf64(rt, 8 * 5);
	glOrtho(left, right, bottom, top, znear, zfar);
	return 0;
}

static vmError glFun_LoadMatrix(libcContext rt) {		// LoadMatrix(double x[16])
	double* ptr = argref(rt, 0);
	glLoadMatrixd(ptr);
	return 0;
}
static vmError glFun_MultMatrix(libcContext rt) {		// MultMatrix(double x[16])
	double* ptr = argref(rt, 0);
	glMultMatrixd(ptr);
	return 0;
}
static vmError glFun_LoadIdentity(libcContext rt) {
	glLoadIdentity();
	return 0;
}
static vmError glFun_PushMatrix(libcContext rt) {
	glPushMatrix();
	return 0;
}
static vmError glFun_PopMatrix(libcContext rt) {
	glPopMatrix();
	return 0;
}

static vmError glFun_Rotate(libcContext rt) {
	GLdouble angle = argf64(rt, 8 * 0);
	GLdouble x = argf64(rt, 8 * 1);
	GLdouble y = argf64(rt, 8 * 2);
	GLdouble z = argf64(rt, 8 * 3);
	glRotated(angle, x, y, z);
	return 0;
}
static vmError glFun_Scale(libcContext rt) {
	GLdouble x = argf64(rt, 8 * 0);
	GLdouble y = argf64(rt, 8 * 1);
	GLdouble z = argf64(rt, 8 * 2);
	glScaled(x, y, z);
	return 0;
}
static vmError glFun_Translate(libcContext rt) {
	GLdouble x = argf64(rt, 8 * 0);
	GLdouble y = argf64(rt, 8 * 1);
	GLdouble z = argf64(rt, 8 * 2);
	glTranslated(x, y, z);
	return 0;
}
//#} opengl api

//#{ glu wrapper functions
static vmError gluFun_LookAt(libcContext rt) {
	GLdouble eyex = argf64(rt, 8 * 0);
	GLdouble eyey = argf64(rt, 8 * 1);
	GLdouble eyez = argf64(rt, 8 * 2);

	GLdouble tox = argf64(rt, 8 * 3);
	GLdouble toy = argf64(rt, 8 * 4);
	GLdouble toz = argf64(rt, 8 * 5);

	GLdouble upx = argf64(rt, 8 * 6);
	GLdouble upy = argf64(rt, 8 * 7);
	GLdouble upz = argf64(rt, 8 * 8);

	gluLookAt(eyex, eyey, eyez, tox, toy, toz, upx, upy, upz);
	return 0;
}
//#}

//#{ glut handlers
static int btn = 0;

static rtContext rt = NULL;

// script side callback functions
static symn onMouse = NULL;
static symn onMotion = NULL;
static symn onKeyboard = NULL;
static symn onDisplay = NULL;
static symn onReshape = NULL;

static void mouse(int _btn, int state, int x, int y) {
	if (onMouse && rt) {
		#pragma pack(push, 4)
		struct {int32_t btn, x, y;} args = {btn, x, y};
		#pragma pack(pop)

		//~ _btn = 1 << _btn;
		_btn += 1;

		switch (state) {

			case GLUT_UP:
				btn &= ~_btn;
				break;

			case GLUT_DOWN:
				btn |= _btn;
				break;
		}

		rt->api.invoke(rt, onMouse, NULL, &args, NULL);
	}
}
static void motion(int x, int y) {
	if (onMotion && rt) {
		#pragma pack(push, 4)
		struct {int32_t btn, x, y;} args = {btn, x, y};
		#pragma pack(pop)
		rt->api.invoke(rt, onMotion, NULL, &args, NULL);
	}
}
static void keyboard(unsigned char key, int x, int y) {
	if (onKeyboard && rt) {
		#pragma pack(push, 4)
		struct {int32_t key;} args = {key};
		#pragma pack(pop)
		rt->api.invoke(rt, onKeyboard, NULL, &args, NULL);
	}
	//~ debugGL("keyboard(key:%d)", key);
}
static void display(void) {
	if (onDisplay && rt) {
		rt->api.invoke(rt, onDisplay, NULL, NULL, NULL);
	}
}
static void reshape(int x, int y) {
	if (onReshape && rt) {
		#pragma pack(push, 4)
		struct {int x, y;} args = {x, y};
		#pragma pack(pop)
		rt->api.invoke(rt, onReshape, NULL, &args, NULL);
	}
}
//#}

//#{ glut wrapper functions
static vmError glutFun_DisplayCB(libcContext rt) {
	onDisplay = argsym(rt, 0);
	return 0;
}
static vmError glutFun_ReshapeCB(libcContext rt) {
	onReshape = argsym(rt, 0);
	return 0;
}
static vmError glutFun_MouseCB(libcContext rt) {
	onMouse = argsym(rt, 0);
	return 0;
}
static vmError glutFun_MotionCB(libcContext rt) {
	onMotion = argsym(rt, 0);
	return 0;
}
static vmError glutFun_KeyboardCB(libcContext rt) {
	onKeyboard = argsym(rt, 0);
	return 0;
}

static vmError glutFun_Reshape(libcContext rt) {
	int width = argi32(rt, 0);
	int height = argi32(rt, 4);
	glutReshapeWindow(width, height);
	return 0;
}
static vmError glutFun_PostRedisplay(libcContext rt) {
	glutPostRedisplay();
	return 0;
}

static vmError glutFun_MainLoop(libcContext rt) {
	int argc = 0;
	char* argv[] = {""};
	int width = argi32(rt, 0);
	int height = argi32(rt, 4);

	glutInit(&argc, argv);
	glutCreateWindow("Window");
	glutReshapeWindow(width, height);
	glutKeyboardFunc(&keyboard);
	glutMouseFunc(&mouse);
	glutMotionFunc(&motion);
	glutDisplayFunc(&display);
	glutReshapeFunc(&reshape);
	glutMainLoop();
	return 0;
}
static vmError glutFun_ExitLoop(libcContext rt) {
	//~ glutLeaveMainLoop();
	exit(0);
	return 0;
}
//#} glut api

#ifdef _WIN32
__declspec( dllexport )
#endif
int ccvmInit(rtContext _rt) {

	struct {
		vmError (*fun)(libcContext);
		char *def;
	}
	libcGl[] = {
		{glFun_Viewport,        "void Viewport(int x, int y, int width, int height);"},
		{glFun_Enable,          "void Enable(int32 mode);"},
		{glFun_Disable,         "void Disable(int32 mode);"},
		{glFun_ShadeModel,      "void ShadeModel(int mode);"},
		{glFun_CullFace,        "void CullFace(int mode);"},

		{glFun_Clear,			"void Clear(int32 Buffer);"},
		{glFun_Begin,			"void Begin(int32 Primitive);"},
		{glFun_End,				"void End();"},
		{glFun_Flush,			"void Flush();"},

		{glFun_Vertex,			"void Vertex(float32 x, float32 y, float32 z, float32 w);"},
		{glFun_Normal,			"void Normal(float32 x, float32 y, float32 z);"},
		{glFun_Color,			"void Color(float32 r, float32 g, float32 b, float32 a);"},

		// Matrix operations
		{glFun_MatrixMode,		"void MatrixMode(int mode);"},
		{glFun_LoadMatrix,		"void LoadMatrix(double x[16]);"},
		{glFun_MultMatrix,		"void MultMatrix(double x[16]);"},
		{glFun_LoadIdentity,	"void LoadIdentity();"},
		{glFun_PushMatrix,		"void PushMatrix();"},
		{glFun_PopMatrix,		"void PopMatrix();"},
		{glFun_Rotate,			"void Rotate(float64 angle, float64 x, float64 y, float64 z);"},
		{glFun_Scale,			"void Scale(float64 angle, float64 x, float64 y, float64 z);"},
		{glFun_Translate,		"void Translate(float64 angle, float64 x, float64 y, float64 z);"},

		{glFun_Frustum,			"void Frustum(float64 left, float64 right, float64 bottom, float64 top, float64 znear, float64 zfar);"},
		{glFun_Ortho,			"void Ortho(float64 left, float64 right, float64 bottom, float64 top, float64 znear, float64 zfar);"},
		//~ {glFun_,			"(float32 x, float32 y, float32 z, float32 w);"},
	},
	libcGlu[] = {
		{gluFun_LookAt,			"void LookAt(float64 eyeX, float64 eyeY, float64 eyeZ, float64 centerX, float64 centerY, float64 centerZ, float64 upX, float64 upY, float64 upZ );"},
		//~ {gluFun_,			"(float32 x, float32 y, float32 z, float32 w);"},
	},
	libcGlut[] = {
		// handlers
		{glutFun_DisplayCB,		"void OnDisplay(void callback());"},
		{glutFun_ReshapeCB,		"void OnReshape(void callback(int x, int y));"},
		{glutFun_MouseCB,		"void OnMouse(void callback(int btn, int x, int y));"},
		{glutFun_MotionCB,		"void OnMotion(void callback(int btn, int x, int y));"},
		{glutFun_KeyboardCB,	"void OnKeyboard(void callback(int btn));"},

		{glutFun_PostRedisplay,	"void PostRedisplay();"},
		{glutFun_Reshape,		"void Reshape(int width, int height);"},
		{glutFun_MainLoop,		"void MainLoop(int width, int height);"},
		{glutFun_ExitLoop,		"void LeaveMainLoop();"},
		//~ {glutFun_,			"();"},
	};

	struct {
		size_t value;
		char *name;
	}
	defsGl[] = {
		// enable / disable
		{GL_DEPTH_TEST,			"DepthTest"},		// If enabled, do depth comparisons and update the depth buffer. Note that even if the depth buffer exists and the depth mask is non-zero, the depth buffer is not updated if the depth test is disabled. See glDepthFunc and
		{GL_LINE_SMOOTH,		"LineSmooth"},		// If enabled, draw lines with correct filtering. Otherwise, draw aliased lines. See glLineWidth.
		{GL_CULL_FACE,			"CullFace"},		// 
		//~ {GL_, ""},		// 

		// CullFace
		{GL_FRONT,				"Front"},			// 
		{GL_BACK,				"Back"},			// 
		{GL_FRONT_AND_BACK,		"FrontAndBack"},	// 

		// ShadeModel
		{GL_FLAT,				"Flat"},		// 
		{GL_SMOOTH,				"Smooth"},		// 

		// clear
		{GL_COLOR_BUFFER_BIT,	"ColorBuffer"},		// Indicates the buffers currently enabled for color writing.
		{GL_DEPTH_BUFFER_BIT,	"DepthBuffer"},		// Indicates the depth buffer.
		{GL_ACCUM_BUFFER_BIT,	"AccumBuffer"},		// Indicates the accumulation buffer.
		{GL_STENCIL_BUFFER_BIT,	"StencilBuffer"},	// Indicates the stencil buffer.

		// matrix mode
		{GL_MODELVIEW,			"Modelview"},		// Applies subsequent matrix operations to the modelview matrix stack.
		{GL_PROJECTION,			"Projection"},		// Applies subsequent matrix operations to the projection matrix stack.
		{GL_TEXTURE,			"Texture"},			// Applies subsequent matrix operations to the texture matrix stack.
		{GL_COLOR,				"Color"},			// Applies subsequent matrix operations to the color matrix stack.

		// primitives
		{GL_POINTS,				"Points"},			// Treats each vertex as a single point. Vertex n defines point n . N points are drawn.
		{GL_LINES,				"Lines"},			// Treats each pair of vertices as an independent line segment. Vertices 2 n - 1 and 2 n define line n . N / 2 lines are drawn.
		{GL_LINE_STRIP,			"LineStrip"},		// Draws a connected group of line segments from the first vertex to the last. Vertices n and n + 1 define line n . N - 1 lines are drawn.
		{GL_LINE_LOOP,			"LineLoop"},		// Draws a connected group of line segments from the first vertex to the last, then back to the first. Vertices n and n + 1 define line n . The last line, however, is defined by vertices N and 1 . N lines are drawn.
		{GL_TRIANGLES,			"Triangles"},		// Treats each triplet of vertices as an independent triangle. Vertices 3 n - 2 , 3 n - 1 , and 3 n define triangle n . N / 3 triangles are drawn.
		{GL_TRIANGLE_STRIP,		"TriangleStrip"},	// Draws a connected group of triangles. One triangle is defined for each vertex presented after the first two vertices. For odd n , vertices n , n + 1 , and n + 2 define triangle n . For even n , vertices n + 1 , n , and n + 2 define triangle n . N - 2 triangles are drawn.
		{GL_TRIANGLE_FAN,		"TriangleFan"},		// Draws a connected group of triangles. One triangle is defined for each vertex presented after the first two vertices. Vertices 1 , n + 1 , and n + 2 define triangle n . N - 2 triangles are drawn.
		{GL_QUADS, 				"Quads"},			// Treats each group of four vertices as an independent quadrilateral. Vertices 4 n - 3 , 4 n - 2 , 4 n - 1 , and 4 n define quadrilateral n . N / 4 quadrilaterals are drawn.
		{GL_QUAD_STRIP,			"QuadStrip"},		// Draws a connected group of quadrilaterals. One quadrilateral is defined for each pair of vertices presented after the first pair. Vertices 2 n - 1 , 2 n , 2 n + 2 , and 2 n + 1 define quadrilateral n . N / 2 - 1 quadrilaterals are drawn. Note that the order in which vertices are used to construct a quadrilateral from strip data is different from that used with independent data.
		{GL_POLYGON,			"Polygon"},			// Draws a single, convex polygon. Vertices 1 through N define this polygon.
	},
	defsGlu[] = {{ 0, NULL }},
	defsGlut[] = {{ 0, NULL }};

	int i;
	symn nsp;	// namespace

	rt = _rt;
	//~ debugGL("Initializing OpenGl");
	if ((nsp = rt->api.ccBegin(rt, "gl"))) {

		for (i = 0; i < sizeof(defsGl) / sizeof(*defsGl); i += 1) {
			if (!rt->api.ccDefInt(rt, defsGl[i].name, defsGl[i].value)) {
				return +1;
			}
		}

		for (i = 0; i < sizeof(libcGl) / sizeof(*libcGl); i += 1) {
			if (!rt->api.ccDefCall(rt, libcGl[i].fun, NULL, libcGl[i].def)) {
				return -1;
			}
		}

		//~ add some inlines
		if (!rt->api.ccAddUnit(rt, 0, __FILE__, __LINE__ + 1,
			"define Vertex(float32 x, float32 y, float32 z) = Vertex(x, y, z, 1.);\n"
			"define Color(float32 r, float32 g, float32 b) = Color(r, g, b, 1.);\n"
		)) {
			return -1;
		}

		rt->api.ccEnd(rt, nsp);
	}
	if ((nsp = rt->api.ccBegin(rt, "glu"))) {
		for (i = 0; i < sizeof(defsGlu) / sizeof(*defsGlu); i += 1) {
			if (defsGlu[i].name == NULL) {
				continue;
			}
			if (!rt->api.ccDefInt(rt, defsGlu[i].name, defsGlu[i].value)) {
				return -2;
			}
		}
		for (i = 0; i < sizeof(libcGlu) / sizeof(*libcGlu); i += 1) {
			if (!rt->api.ccDefCall(rt, libcGlu[i].fun, NULL, libcGlu[i].def)) {
				return -2;
			}
		}
		rt->api.ccEnd(rt, nsp);
	}
	if ((nsp = rt->api.ccBegin(rt, "glut"))) {
		for (i = 0; i < sizeof(defsGlut) / sizeof(*defsGlut); i += 1) {
			if (defsGlut[i].name == NULL) {
				continue;
			}
			if (!rt->api.ccDefInt(rt, defsGlut[i].name, defsGlut[i].value)) {
				return -3;
			}
		}
		for (i = 0; i < sizeof(libcGlut) / sizeof(*libcGlut); i += 1) {
			if (!rt->api.ccDefCall(rt, libcGlut[i].fun, NULL, libcGlut[i].def)) {
				return -3;
			}
		}
		rt->api.ccEnd(rt, nsp);
	}
	return 0;
}
