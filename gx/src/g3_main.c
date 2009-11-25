#include <math.h>
#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"

#define __WIN_DRV
//~ #define __VBE_DRV

#include "g3_draw.c"

double O = 2;
double N = 1;
double epsilon = 1e-10;
//~ scalar S = 8;			// eye(zpos)

//~ #define guispeed (.5)
double speed = .5;
#define d speed
#define D (10 * speed)
#define r (toRad(d))
#define R (toRad(D))

char *obj = 0;
char *tex = 0;
char *fnt = 0;
//~ char *sky = 0;

int resx = 800;
int resy = 600;

union vector eye, tgt, up;
//~ union matrix proj, view, world;
//~ struct camera Camera, *cam = &Camera;
struct camera cam[1];

//~ struct gx_Surf envt[6];
struct lght *rm = 0;
struct gx_Surf offs;		// offscreen
struct mesh msh;
int vtx = 0;

vector getobjvec(int Normal) {
	if (rm != NULL)
		return Normal ? &rm->dir : &rm->pos;
	if ((vtx %= msh.vtxcnt + 1))
		return (Normal ? msh.nrm : msh.pos) + vtx - 1;
	return NULL;
}

int osdProgress(float);

void readIni() {
	char *is = " ";
	//~ static char sect[1024];		// section;
	static char temp[65536];
	char *ptr = temp, *arg;
	FILE* fin = fopen("gx.ini", "rt");
	obj = 0;
	tex = 0;
	fnt = 0;
	if (fin) for ( ; ; ) {
		int cnt = temp + sizeof(temp) - ptr;

		fgets(ptr, cnt, fin);
		if (feof(fin)) break;
		ptr[strlen(ptr) - 1] = 0;

		if (*ptr == '#') continue;				// Comment
		if (*ptr == '\0') continue;				// Empty line
		if ((arg = readKVP(ptr, "screen.font", "=", is))) {
			int len = strlen(arg);
			strcpy(ptr, arg);
			fnt = ptr;
			ptr += len + 1;
			continue;
		}
		if ((arg = readKVP(ptr, "screen.width", "=", is))) {
			if (*readNum(arg, &resx))
				debug("invalid number: %s", arg);
			continue;
		}
		if ((arg = readKVP(ptr, "screen.height", "=", is))) {
			if (*readNum(arg, &resy))
				debug("invalid number: %s", arg);
			continue;
		}
		if ((arg = readKVP(ptr, "screen.speed", "=", is))) {
			if (*readFlt(arg, &speed))
				debug("invalid number: %s", arg);
			continue;
		}

		if ((arg = readKVP(ptr, "material", "=", is))) {
			int num;
			if (*readNum(arg, &num) == '\0') {
				if (num > sizeof(defmtl)/sizeof(*defmtl)) {
					debug("invalid material: %s", arg);
					num = 0;
				}
				debug("using material: %d", num);
				msh.mtl = defmtl[num];
			}
			else debug("invalid number: %s", ptr);
			continue;
		}
		if ((arg = readKVP(ptr, "texture", "=", is))) {
			int len = strlen(arg);
			strcpy(ptr, arg);
			tex = ptr;
			ptr += len + 1;
			continue;
		}
		if ((arg = readKVP(ptr, "object.size", "=", is))) {
			if (*readFlt(arg, &O))
				debug("invalid number: %s", arg);
			continue;
		}
		if ((arg = readKVP(ptr, "normal.size", "=", is))) {
			if (*readFlt(arg, &N))
				debug("invalid number: %s", arg);
			continue;
		}
		if ((arg = readKVP(ptr, "epsilon", "=", is))) {
			if (*readFlt(arg, &epsilon))
				debug("invalid number: %s", arg);
			continue;
		}
		if ((arg = readKVP(ptr, "object", "=", is))) {
			int len = strlen(arg);
			strcpy(ptr, arg);
			obj = ptr;
			ptr += len + 1;
			continue;
		}
		if ((arg = readKVP(ptr, "object", "{", is))) {
			obj = ptr;
			strncpy(ptr,
			"!\n"
			"flt64 s = st(1);\n"
			"flt64 t = st(2);\n"
			"flt64 x = s;\n"
			"flt64 y = t;\n"
			"flt64 z = 0;\n"
			"\n", cnt);
			ptr += strlen(ptr);
			while (1) {
				cnt = temp + sizeof(temp) - ptr;
				fgets(ptr, cnt, fin);
				if (feof(fin)) break;
				if (*ptr == '}') break;				// Comment
				if (*ptr == '#') continue;			// Comment
				if (*ptr == '\n') continue;			// Empty line
				ptr += strlen(ptr);
			}
			strncpy(ptr, "\nsetPos(x, y, z);\n", cnt);
			ptr += strlen(ptr) + 1;
		}
		/*if ((ptr = readKVP(buff, "skybox", "=", is))) {
			int len = strlen(ptr);
			if (str + len < temp + sizeof(temp)) {
				strcpy(sky = str, ptr);
				str += len + 1;
			}
			continue;
		}//*/
	}
	//~ debug("%s", obj);
}

void reload() {
	char *ext;
	int res;
	if (tex && (ext = fext(tex))) {
		res = -2;
		if (stricmp(ext, "bmp") == 0)
			res = gx_loadBMP(&msh.mtl.tex, tex, 32);
		if (stricmp(ext, "jpg") == 0)
			res = gx_loadJPG(&msh.mtl.tex, tex, 32);
		debug("ReadText('%s'): %d", tex, res);
	}
	if (obj && *obj == '!') {
		debug("EvalMesh('%s'): %d", "!", evalMesh(&msh, obj + 1, 64, 64));
	}
	else if (obj) {
		debug("ReadMesh('%s'): %d", obj, readMesh(&msh, obj));
	}
	/*else if (obj) {
		double* (*evalP)(double [3], double [3], double s, double t) = 0;
		int div = 64;
		char *val = obj + 1;
		char *fun = readNum(val, &div);
		if (stricmp(fun, "Sphere") == 0)
			evalP = evalP_sphere;
		if (stricmp(fun, "tours") == 0)
			evalP = evalP_tours;
		if (stricmp(fun, "cone") == 0)
			evalP = evalP_cone;
		if (stricmp(fun, "apple") == 0)
			evalP = evalP_apple;
		if (stricmp(fun, "plane") == 0)
			evalP = evalP_plane;
		if (stricmp(fun, "_001") == 0)
			evalP = evalP_001;
		if (stricmp(fun, "_002") == 0)
			evalP = evalP_002;
		if (stricmp(fun, "_003") == 0)
			evalP = evalP_003;
		if (stricmp(fun, "_004") == 0)
			evalP = evalP_004;
		if (stricmp(fun, "_005") == 0)
			evalP = evalP_005;
		if (stricmp(fun, "_006") == 0)
			evalP = evalP_006;
		if (stricmp(fun, "_007") == 0)
			evalP = evalP_007;
		printf("EvalMesh('%s'): %d\n", fun, evalMesh(&msh, evalP, div, div, 0));
	}*/
	else {
		msh.vtxcnt = msh.tricnt = msh.segcnt = 0;
		setvtxD(&msh, 0, 'p', 0, 0, -1);
		setvtxD(&msh, 1, 'p', 0, -1, 0);
		setvtxD(&msh, 2, 'p', -1, 0, 0);
		setvtxD(&msh, 0, 'n', 0, 0, -1);
		setvtxD(&msh, 1, 'n', 0, -1, 0);
		setvtxD(&msh, 2, 'n', -1, 0, 0);
		addtri(&msh, 0, 1, 2);
	}
}

void ratHND(int btn, int mx, int my) {
	static int ox, oy;
	int mdx = mx - ox;
	int mdy = my - oy;
	ox = mx; oy = my;
	if (btn == 1) {
		if (mdx) {
			union matrix tmp;
			matldR(&tmp, &cam->dirU, mdx * R);
			vecnrm(&cam->dirF, matvph(&cam->dirF, &tmp, &cam->dirF));
			vecnrm(&cam->dirR, matvph(&cam->dirR, &tmp, &cam->dirR));
			veccrs(&cam->dirU, &cam->dirF, &cam->dirR);
			matvph(&cam->pos, &tmp, &cam->pos);
		}
		if (mdy) {
			union matrix tmp;
			matldR(&tmp, &cam->dirR, mdy * R);
			vecnrm(&cam->dirF, matvph(&cam->dirF, &tmp, &cam->dirF));
			vecnrm(&cam->dirR, matvph(&cam->dirR, &tmp, &cam->dirR));
			veccrs(&cam->dirU, &cam->dirF, &cam->dirR);
			matvph(&cam->pos, &tmp, &cam->pos);
		}
	}
	else if (btn == 2) {
		vector N;
		if ((N = getobjvec(1))) {
			union matrix tmp, tmp1, tmp2;
			matldR(&tmp1, &cam->dirU, mdx * r);
			matldR(&tmp2, &cam->dirR, mdy * r);
			matmul(&tmp, &tmp1, &tmp2);
			vecnrm(N, matvp3(N, &tmp, N));
		}
		else {
			if (mdy) camrot(cam, &cam->dirR, mdy * r);		// turn: up / down
			if (mdx) camrot(cam, &cam->dirU, mdx * r);		// turn: left / right
		}
	}
	else if (btn == 3) {
		vector P;
		if ((P = getobjvec(0))) {
			union matrix tmp, tmp1, tmp2;
			matldT(&tmp1, &cam->dirU, -mdy * d);
			matldT(&tmp2, &cam->dirR, mdx * d);
			matmul(&tmp, &tmp1, &tmp2);
			matvph(P, &tmp, P);
		}
		else {
			if (mdy) cammov(cam, &cam->dirF, mdy * d);		// move: forward
			//~ if (mdx) camrot(cam, &cam->dirU, mdx * r);		// turn: left / right
		}
	}
}

int kbdHND(int lkey, int key) {
	const scalar sca = 8;
	switch (key) {
		case  27 : return -1;
		case  13 : {
			vecldf(&eye, 0, 0, O * sca, 1);
			camset(cam, &eye, &tgt, &up);
		} break;

		case '*' : centMesh(&msh, O = 4.); break;
		case '+' : centMesh(&msh, O *= 2.); break;
		case '-' : centMesh(&msh, O /= 2.); break;

		case 'O' : vecldf(&cam->pos, 0, 0, 0, 1); break;

		case 'w' : cammov(cam, &cam->dirF, +d); break;
		case 'W' : cammov(cam, &cam->dirF, +D); break;
		case 's' : cammov(cam, &cam->dirF, -d); break;
		case 'S' : cammov(cam, &cam->dirF, -D); break;
		case 'd' : cammov(cam, &cam->dirR, +d); break;
		case 'D' : cammov(cam, &cam->dirR, +D); break;
		case 'a' : cammov(cam, &cam->dirR, -d); break;
		case 'A' : cammov(cam, &cam->dirR, -D); break;
		case ' ' : cammov(cam, &cam->dirU, +D); break;
		case 'c' : cammov(cam, &cam->dirU, -D); break;

		case '\t': draw = (draw & ~draw_mode) | ((draw + LOBIT(draw_mode)) & draw_mode); break;
		case '/' : draw = (draw & ~cull_mode) | ((draw + LOBIT(cull_mode)) & cull_mode); break;
		case '?' : draw = (draw & ~cull_mode) | ((draw - LOBIT(cull_mode)) & cull_mode); break;
		//~ case ']' : draw = (draw & ~pcol_mode) | ((draw + LOBIT(pcol_mode)) & pcol_mode); break;
		case '^' : {
			g3_drawline = (g3_drawline == g3_drawlineA ? g3_drawlineB : g3_drawlineA);
			//~ draw ^= text_afine;
		} break;

		case '~' : draw ^= disp_info; break;
		case 'L' : draw ^= disp_lght; break;
		case 'b' : draw ^= disp_bbox; break;
		case 'Z' : draw ^= disp_zbuf; break;

		case 'n' : draw ^= disp_norm; break;
		case 't' : draw ^= draw_tex; break;

		case 'T' : msh.hasTex = !msh.hasTex; break;
		case 'N' : msh.hasNrm = !msh.hasNrm; break;
		case 'l' : draw ^= draw_lit; break;

		case '`' : vtx = 0; rm = NULL; break;
		case '0' : case '1' : case '2' :
		case '3' : case '4' : case '5' :
		case '6' : case '7' : case '8' :
		case '9' : if (key - '0' < lights) {
			rm = 0;
			if ((Lights[key - '0'].attr ^= L_on) & L_on) {
				rm = &Lights[key - '0'];
			}
		} break;
		case '.' : vtx += 1; break;
		case '>' : vtx -= 1; break;


		case '[': if (rm) rm->sCos -= 1.; break;
		case ']': if (rm) rm->sCos += 1.; break;
		case '{': if (rm) rm->sExp -= 1.1; break;
		case '}': if (rm) rm->sExp += 1.1; break;

		case '\b' : {
			optiMesh(&msh, 1e-15, osdProgress);
			printf("vtx cnt : %d / %d\n", msh.vtxcnt, msh.maxvtx);
			printf("tri cnt : %d / %d\n", msh.tricnt, msh.maxtri);
		} break; // */

		case '\\': {
			printf("normalize Mesh ...");
			normMesh(&msh, 0);
			printf("done\n");
		} break;
		case '|': {
			printf("normalize Mesh ...");
			normMesh(&msh, 1e-30);
			printf("done\n");
		} break;

		case ';' : {
			//~ printf("load Mesh ... ");
			reload(&msh, obj, tex);
			centMesh(&msh, O);
			//~ printf("done\n");
		} break;
		case '\'' : 
		case '"' : {
			printf("subdivide Mesh ...");
			sdivMesh(&msh, 1);
			printf("done\n");
		}

		case 18:		// CTrl + r
			readIni();
			break;

		case 16:		// CTrl + p
			gx_saveBMP("mesh.bmp", &offs, 0);
			break;

		case 19:		// CTrl + s
			save_obj(&msh, "mesh.obj");
			break;

		default: debug("kbdHND(lkey(%d), key(%d))", lkey, key);
	}
	return 0;
}

flip_scr flip;
int (*peekMsg)(int);
int isChanged(float now, float p) {
	static float old = 0;
	if (fabs(old - now) > p)
		old = now;
	return old == now;
}
int osdProgress(float prec) {
	char str[256];
	if (isChanged(prec, 1e-0)) {
		sprintf(str, "progressing: %.2f%%", prec);
		setCaption(str);
	}
	return peekMsg(0);
}

int main(int argc, char* argv[]) {
	struct gx_Surf font;		// font

	struct {
		time_t	sec;
		time_t	cnt;
		int	col;
		int	fps;
	} fps = {0,0,0,0};
	char str[1024];
	void (*done)(void);
	struct gx_Rect box;
	int e;

	initMesh(&msh, 1024);
	setbuf(stdout, NULL);
	msh.mtl = defmtl[12];

	for (e = 1; e < lights; e += 1) {
		//~ vecneg(&Lights[e].dir, &Lights[e].pos); // lookat(0,0,0)
		vecnrm(&Lights[e].dir, &Lights[e].dir);
		//~ vecldf(&Lights[e].dir, 0, 0, 0, 0);
		//~ vecldf(&Lights[e].attn, 1, 1, 0, 0);
	}
	//~ Lights[0].pos.x = -4.42;
	//~ Lights[0].pos.y = +9.96;
	//~ Lights[0].pos.z = -1.56;

	#ifdef cubemap
		printf ("load image : %d\n", gx_loadBMP(envt + 0, cubemap"positive_z.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 1, cubemap"negative_z.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 2, cubemap"positive_x.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 3, cubemap"negative_x.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 4, cubemap"positive_y.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 5, cubemap"negative_y.bmp", 32));
	#endif

	readIni();

	if (argc == 2) {		// drop in
		obj = argv[1];
	}
	else if (argc == 3) {		// drop in
		obj = argv[1];
		tex = argv[2];
	}
	else while (--argc > 0) {
		char *cmd = *++argv;
		if (strncmp(cmd, "-o", 2) == 0) {
			obj = cmd + 2;
		}
		else if (strncmp(cmd, "-t", 2) == 0) {
			tex = cmd + 2;
		}
		else {
			printf("usage: program [-o<obj filename> t<texture filename> f<division><function>]\n");
		}
	}

	reload();
	//~ initMesh(&mesh, 16);
	bboxMesh(&msh, &tgt, &eye);
	printf("box min : %f, %f, %f\n", tgt.x, tgt.y, tgt.z);
	printf("box max : %f, %f, %f\n", eye.x, eye.y, eye.z);
	printf("vtx cnt : %d / %d\n", msh.vtxcnt, msh.maxvtx);
	printf("tri cnt : %d / %d\n", msh.tricnt, msh.maxtri);
	centMesh(&msh, O);

	vecldf(&up, 0, 1, 0, 1);
	vecldf(&tgt, 0, 0, 0, 1);

	//~ vecldf(&eye, 0, 0, 1, 1);
	//~ camset(cam, &eye, &tgt, &up);
	kbdHND(0, 13);

	if ((e = gx_loadFNT(&font, fnt))) {
		debug("Cannot open font '%s': %d\n", fnt, e);
		gx_doneSurf(&font);
		freeMesh(&msh);
		return -2;
	}

	if ((e = g3_init(&offs, resx, resy))) {
		debug("Error: %d\n", e);
		gx_doneSurf(&font);
		gx_doneSurf(&offs);
		freeMesh(&msh);
		return -2;
	}

	projv_mat(&cam->proj, 10, (double)resx / resy, 5, 100);

	if (init_WIN(&offs, &flip, &peekMsg, &done, ratHND, kbdHND)) {
		printf("Cannot init surface\n");
		gx_doneSurf(&font);
		gx_doneSurf(&offs);
		freeMesh(&msh);
		return -1;
	}

	for(fps.cnt = 0, fps.sec = time(NULL); peekMsg(1) != -1; fps.cnt++) {
		union matrix proj[1], view[1];
		union vector v[6];
		#define nextln ((ln += box.h) - box.h)
		int tris = 0, ln = 0;

		for (e = 0; e < scrw * scrh; e += 1) {		// clear
			cBuff[e] = 0x00000000;
			//~ zBuff[e] = 0x3fffffff;
			zBuff[e] = 0x00ffffff;
		}
		if (fps.sec != time(NULL)) {				// fps
			fps.col = fps.cnt < 30 ? 0xff0000 : fps.cnt < 50 ? 0xffff00 : 0x00ff00;
			fps.sec = time(NULL);
			fps.fps = fps.cnt;
			fps.cnt = 0;
		}

		#ifdef cubemap
		matmul(&proj, &projM, cammat(&view, cam));
		g3_drawenvc(envt, &cam->dirF, &proj, 1000);
		#endif

		if (draw & disp_mesh) {
			matidn(view);
			msh.hlplt = vtx;
			tris += g3_drawmesh(&msh, view, cam);
			//~ view.zt += 2*O;
			//~ tris += g3_drawmesh(&msh, &view, cam);
			//~ view.zt -= 4*O;
			//~ tris += g3_drawmesh(&msh, &view, cam);
			//~ view.zt -= 2*O;
			//~ tris += g3_drawmesh(&msh, &view, cam);
		}
		if (draw & disp_zbuf) {
			for(e = 0; e < scrw*scrh; e += 1) {
				long z = zBuff[e];
				z = (z >> 15) & 0x1ff;
				if (z < 256) z = ~z & 0xff;
				cBuff[e] = ~(z << 16 | z << 8 | z);
			}
		}
		if (draw & disp_lght) {
			int i = 0;
			cammat(view, cam);
			matmul(proj, &cam->proj, view);
			for (i = 0; i < lights; i += 1) {
				scalar lsize = .5;
				mappos(v, proj, &Lights[i].pos);
				if (Lights[i].attr & L_on)
					g3_filloval(v, lsize, lsize, vecrgb(&Lights[i].diff).col);
				g3_drawoval(v, lsize, lsize, vecrgb(&Lights[i].ambi).col);
				if (vecdp3(&Lights[i].dir, &Lights[i].dir)) {
					mappos(v + 1, proj, vecadd(v + 1, &Lights[i].pos, &Lights[i].dir));
					g3_drawline(v + 0, v + 1, vecrgb(&Lights[i].diff).col);
				}
			}
		}
		if (draw & disp_info) {
			sprintf(str, "Object size: %g, tvx:%d, tri:%d, seg:%d%s%s", O, msh.vtxcnt, msh.tricnt, msh.segcnt, msh.hasTex ? ", tex" : "", msh.hasNrm ? ", nrm" : "");
			gx_clipText(&box, &font, str); gx_drawText(&offs, scrw - box.w - 10, nextln, &font, str, 0xffffffff);

			sprintf(str, "camera(%g, %g, %g)", cam->pos.x, cam->pos.y, cam->pos.z);
			gx_clipText(&box, &font, str); gx_drawText(&offs, scrw - box.w - 10, nextln, &font, str, 0xffffffff);

			if (getobjvec(0)) {
				vector P = getobjvec(0);
				vector N = getobjvec(1);
				//~ sprintf(str, "light[%d].pos(%g, %g, %g); cos(%g), exp(%g)", rm-Lights, rm->pos.x, rm->pos.y, rm->pos.z, rm->sCos, rm->sExp);
				sprintf(str, "object: P(%.3f, %.3f, %.3f), N(%.3f, %.3f, %.3f)", P->x, P->y, P->z, N->x, N->y, N->z);
				gx_clipText(&box, &font, str); gx_drawText(&offs, scrw - box.w - 10, nextln, &font, str, 0xffffffff);
				//~ sprintf(str, "camera1(%g, %g, %g)", cam1.pos.x, cam1.pos.y, cam1.pos.z);
				//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);
			}
			for (e = 0; e < lights; e += 1) {
				argb lcol = vecrgb(&Lights[e].ambi);
				sprintf(str, "light %d : %s", e, Lights[e].attr & L_on ? "On" : "Off");
				gx_clipText(&box, &font, str); gx_drawText(&offs, scrw - box.w - 10, nextln, &font, str, lcol.val);
			}

		}
		//~ g3_drawOXYZ(cam, 1);
		if (fps.cnt == 0) {
			sprintf(str, "tvx:%d, tri:%d/%d, fps: %d", msh.vtxcnt, tris, msh.tricnt, fps.fps);
			setCaption(str);
		}

		flip(&offs, 0);
		//~ sleep(0);

		#undef nextln
	}

	gx_doneSurf(&font);
	gx_doneSurf(&offs);
	freeMesh(&msh);
	done();
	return 0;
}
