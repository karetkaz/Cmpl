//~ gcc -m32 -g2 -ggdb -Wall -Wpadded -shared -o ../../gl.dll gl.c -I ../../src/ -lglut32 -lopengl32
//~ gcc -m32 -g2 -ggdb -Wall -Wpadded -shared -o ../../gl.so gl.c -I ../../src/ -lglut -lGL
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

stateApi api = NULL;
symn onDisplay = NULL;
symn onReshape = NULL;
symn onMouse = NULL;
symn onMotion = NULL;
symn onKeyboard = NULL;

#define windim 300.
#define debugGL(msg, ...) fprintf(stdout, "%s:%d:debug:"msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)
//~ struct {float x, y;} pos[3] = {{0, 1}, {-1, -1}, {1, -1}};

int btn = 0;
void mouse(int _btn, int state, int x, int y) {
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

	if (api && onMouse) {
		api->invoke(api->rt, onMouse, btn, x, y);
	}
}

void motion(int x, int y) {
	if (api && onMotion) {
		api->invoke(api->rt, onMotion, btn, x, y);
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (api && onKeyboard) {
		api->invoke(api->rt, onKeyboard, key);
	}
}

void display() {
	if (api && onDisplay) {
		api->invoke(api->rt, onDisplay);
	}
}

void reshape(int x, int y) {
	if (api && onReshape) {
		api->invoke(api->rt, onReshape, (int)x, (int)y);
	}
}

enum glFuns {
	glFun_Clear,
	glFun_Viewport,
	glFun_Begin,
	glFun_End,
	glFun_Flush,

	glFun_Vertex,
	glFun_Normal,
	glFun_Color,
};
static int glCall(state rt) {
	switch (rt->fdata) {

		case glFun_Viewport: {
			int x = popi32(rt);
			int y = popi32(rt);
			int w = popi32(rt);
			int h = popi32(rt);
			glViewport(x, y, w, h);
		} return 0;

		case glFun_Clear:
			glClear(popi32(rt));
			return 0;

		case glFun_Begin:
			glBegin(popi32(rt));
			return 0;

		case glFun_End:
			glEnd();
			return 0;

		case glFun_Flush:
			glFlush();
			return 0;

		case glFun_Vertex: {
			float x = popf32(rt);
			float y = popf32(rt);
			float z = popf32(rt);
			float w = popf32(rt);
			glVertex4f(x, y, z, w);
		} return 0;

		case glFun_Normal: {
			float x = popf32(rt);
			float y = popf32(rt);
			float z = popf32(rt);
			glNormal3f(x, y, z);
		} return 0;

		case glFun_Color: {
			float x = popf32(rt);
			float y = popf32(rt);
			float z = popf32(rt);
			float a = popf32(rt);
			glColor4f(x, y, z, a);
		} return 0;
	}
	return -1;
};

enum glutFuns {
	glutFun_Display,
	glutFun_Reshape,

	glutFun_Mouse,
	glutFun_Motion,
	glutFun_Keyboard,

	glutFun_PostRedisplay,
	glutFun_MainLoop,
};
static int glutCall(state rt) {
	switch (rt->fdata) {

		case glutFun_Display:
			onDisplay = api->findref(rt, popref(rt));
			return 0;

		case glutFun_Reshape:
			onReshape = api->findref(rt, popref(rt));
			return 0;

		case glutFun_Mouse:
			onMouse = api->findref(rt, popref(rt));
			return 0;

		case glutFun_Motion:
			onMotion = api->findref(rt, popref(rt));
			return 0;

		case glutFun_Keyboard:
			onKeyboard = api->findref(rt, popref(rt));
			return 0;

		case glutFun_PostRedisplay:
			glutPostRedisplay();
			return 0;

		case glutFun_MainLoop: {
			int argc = 0;
			char* argv[] = {""};
			glutInit(&argc, argv);
			glutCreateWindow("GLUT Test");
			glutKeyboardFunc(&keyboard);
			glutMouseFunc(&mouse);
			glutMotionFunc(&motion);
			glutDisplayFunc(&display);
			glutReshapeFunc(&reshape);
			glutMainLoop();
		} return 0;
	}
	return -1;
}

int apiMain(stateApi _api) {
	symn nsp;	// namespace
	struct {
		int (*fun)(state);
		int n;
		char *def;
	}
	defsGl[] = {
		{glCall, glFun_Viewport,	"void Viewport(int x, int y, int width, int height);"},
		{glCall, glFun_Clear,		"void Clear(int32 mode);"},
		{glCall, glFun_Begin,		"void Begin(int32 mode);"},
		{glCall, glFun_End,			"void End();"},
		{glCall, glFun_Flush,		"void Flush();"},
		{glCall, glFun_Vertex,		"void Vertex(float32 x, float32 y, float32 z, float32 w);"},
		{glCall, glFun_Color,		"void Color(float32 x, float32 y, float32 z, float32 a);"},
		{glCall, glFun_Normal,		"void Normal(float32 x, float32 y, float32 z);"},
		//~ {glCall, glFun_,		"(float32 x, float32 y, float32 z, float32 w);"}
	},
	defsGlut[] = {
		// handlers
		{glutCall, glutFun_Display,			"void Display(void callback());"},
		{glutCall, glutFun_Reshape,			"void Reshape(void callback(int x, int y));"},
		{glutCall, glutFun_Mouse,			"void Mouse(void callback(int btn, int x, int y));"},
		{glutCall, glutFun_Motion,			"void Motion(void callback(int btn, int x, int y));"},
		{glutCall, glutFun_Keyboard,		"void Keyboard(void callback(int btn));"},

		{glutCall, glutFun_PostRedisplay,	"void PostRedisplay();"},

		{glutCall, glutFun_MainLoop,		"void MainLoop();"},
	};

	api = _api;
	if ((nsp = api->ccBegin(api->rt, "gl"))) {
		int i, err = 0;
		err = err || !api->ccDefineInt(api->rt, "ColorBuffer", GL_COLOR_BUFFER_BIT);
		err = err || !api->ccDefineInt(api->rt, "DepthBuffer", GL_DEPTH_BUFFER_BIT);

		//~ primitives
		//~ #define GL_POINTS				0x0000
		//~ #define GL_LINES				0x0001
		//~ #define GL_LINE_LOOP			0x0002
		//~ #define GL_LINE_STRIP			0x0003
		//~ #define GL_TRIANGLES			0x0004
		//~ #define GL_TRIANGLE_STRIP		0x0005
		//~ #define GL_TRIANGLE_FAN			0x0006
		//~ #define GL_QUADS				0x0007
		//~ #define GL_QUAD_STRIP			0x0008
		//~ #define GL_POLYGON				0x0009

		err = err || !api->ccDefineInt(api->rt, "Points", GL_POINTS);
		err = err || !api->ccDefineInt(api->rt, "Lines", GL_LINES);
		err = err || !api->ccDefineInt(api->rt, "LineLoop", GL_LINE_LOOP);
		err = err || !api->ccDefineInt(api->rt, "LineStrip", GL_LINE_STRIP);
		err = err || !api->ccDefineInt(api->rt, "Triangles", GL_TRIANGLES);
		err = err || !api->ccDefineInt(api->rt, "TriangleStrip", GL_TRIANGLE_STRIP);
		err = err || !api->ccDefineInt(api->rt, "TriangleFan", GL_TRIANGLE_FAN);
		err = err || !api->ccDefineInt(api->rt, "Quads", GL_QUADS);
		err = err || !api->ccDefineInt(api->rt, "QuadStrip", GL_QUAD_STRIP);
		err = err || !api->ccDefineInt(api->rt, "Polygon", GL_POLYGON);

		if (err) {
			return -2;
		}

		for (i = 0; i < sizeof(defsGl) / sizeof(*defsGl); i += 1) {
			if (!api->libcall(api->rt, defsGl[i].fun, defsGl[i].n, defsGl[i].def)) {
				return -1;
			}
		}
		api->ccAddText(api->rt, __FILE__, __LINE__ + 1, 0,
			"define Vertex(float32 x, float32 y, float32 z) = Vertex(x, y, z, 1);"
			"define Color(float32 x, float32 y, float32 z) = Color(x, y, z, 1);"
		);
		api->ccEnd(api->rt, nsp);
	}
	if ((nsp = api->ccBegin(api->rt, "glut"))) {
		int i;
		for (i = 0; i < sizeof(defsGlut) / sizeof(*defsGlut); i += 1) {
			if (!api->libcall(api->rt, defsGlut[i].fun, defsGlut[i].n, defsGlut[i].def)) {
				return -1;
			}
		}
		api->ccEnd(api->rt, nsp);
	}
	return 0;
}
