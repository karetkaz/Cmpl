#include <math.h>
#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"

#define LOBIT(__VAL) ((__VAL) & -(__VAL))

#define __WIN_DRV
//~ #define __VBE_DRV
#include "g3_draw.c"

vector eye, tgt, up;
scalar O = 4;
scalar S = 8;			// eye(zpos)

#define guispeed (.5)
const scalar d = guispeed;		// move
const scalar D = guispeed * 8;		// fast
const scalar r = toRad(guispeed);	// rotate

int division = 32;
char *obj = 0;
char *tex = 0;
char *fnt = 0;
char *sky = 0;
mesh msh;

gx_Surf envt[6];
struct lght *rm = 0;

int osdProgress(float);

void reload() {
	char *ext;
	int res;
	if (tex && (ext = fext(tex))) {
		res = -2;
		if (stricmp(ext, "bmp") == 0)
			res = gx_loadBMP(&msh.mtl.tex, tex, 32);
		if (stricmp(ext, "jpg") == 0)
			res = gx_loadJPG(&msh.mtl.tex, tex, 32);
		printf("ReadText('%s'): %d\n", tex, res);
	}
	if (obj && *obj != '@') {
		printf("ReadMesh('%s'): %d\n", obj, readMesh(&msh, obj));
	}
	else if (obj) {
		double* (*evalP)(double [3], double [3], double s, double t) = 0;
		if (stricmp(obj, "@Sphere") == 0)
			evalP = evalP_sphere;
		if (stricmp(obj, "@tours") == 0)
			evalP = evalP_tours;
		if (stricmp(obj, "@cone") == 0)
			evalP = evalP_cone;
		if (stricmp(obj, "@apple") == 0)
			evalP = evalP_apple;
		if (stricmp(obj, "@plane") == 0)
			evalP = evalP_plane;
		if (stricmp(obj, "@001") == 0)
			evalP = evalP_001;
		if (stricmp(obj, "@002") == 0)
			evalP = evalP_002;
		if (stricmp(obj, "@003") == 0)
			evalP = evalP_003;
		if (stricmp(obj, "@004") == 0)
			evalP = evalP_004;
		if (stricmp(obj, "@005") == 0)
			evalP = evalP_005;
		if (stricmp(obj, "@006") == 0)
			evalP = evalP_006;
		if (stricmp(obj, "@007") == 0)
			evalP = evalP_007;
		printf("EvalMesh('%s'): %d\n", obj + 1, evalMesh(&msh, evalP, division, division, 0));
	}
}

void ratHND(int btn, int mx, int my) {
	static int ox, oy;
	int mdx = mx - ox;
	int mdy = my - oy;
	ox = mx; oy = my;
	Mx = mx; My = my;
	if (btn == 1) {
		camera *camera = cam;
		if (mdx) {
			matrix tmp;
			matldR(&tmp, &camera->dirU, mdx * r);
			vecnrm(&camera->dirF, matvph(&camera->dirF, &tmp, &camera->dirF));
			vecnrm(&camera->dirR, matvph(&camera->dirR, &tmp, &camera->dirR));
			veccrs(&camera->dirU, &camera->dirF, &camera->dirR);
			matvph(&camera->pos, &tmp, &camera->pos);
		}
		if (mdy) {
			matrix tmp;
			matldR(&tmp, &camera->dirR, mdy * r);
			vecnrm(&camera->dirF, matvph(&camera->dirF, &tmp, &camera->dirF));
			vecnrm(&camera->dirR, matvph(&camera->dirR, &tmp, &camera->dirR));
			veccrs(&camera->dirU, &camera->dirF, &camera->dirR);
			matvph(&camera->pos, &tmp, &camera->pos);
		}
	}
	else if (btn == 2) {
		if (rm != NULL) {
			camera *camera = cam;
			matrix tmp, tmp1, tmp2;
			matldR(&tmp1, &camera->dirU, mdx * r);
			matldR(&tmp2, &camera->dirR, mdy * r);
			matmul(&tmp, &tmp1, &tmp2);
			matvp3(&rm->dir, &tmp, &rm->dir);
			vecnrm(&rm->dir, &rm->dir);
		}
		else {
			if (mdy) camrot(cam, &cam->dirR, mdy * r);		// turn: up / down
			if (mdx) camrot(cam, &cam->dirU, mdx * r);		// turn: left / right
		}
	}
	else if (btn == 3) {
		if (rm != NULL) {
			camera *camera = cam;
			matrix tmp, tmp1, tmp2;
			matldT(&tmp1, &camera->dirU, -mdy * r);
			matldT(&tmp2, &camera->dirR, mdx * r);
			matmul(&tmp, &tmp1, &tmp2);
			matvph(&rm->pos, &tmp, &rm->pos);
		}
		else {
			camera *camera = cam;
			if (mdy) cammov(camera, &camera->dirF, mdy * d);		// move: forward
			//~ if (mdx) camrot(camera, &camera->dirU, mdx * r);		// turn: left / right
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
		//~ case ']' : draw = (draw & ~pcol_mode) | ((draw + LOBIT(pcol_mode)) & pcol_mode); break;
		case '^' : {
			g3_drawline = (g3_drawline == g3_drawlineA ? g3_drawlineB : g3_drawlineA);
			//~ draw ^= text_afine;
		} break;

		case '`' : draw ^= disp_info; break;
		case 'L' : draw ^= disp_lght; break;
		case 'b' : draw ^= disp_bbox; break;
		case 'Z' : draw ^= disp_zbuf; break;

		case 'n' : draw ^= disp_norm; break;
		case 't' : draw ^= draw_tex; break;

		case 'T' : msh.hasTex = !msh.hasTex; break;
		case 'N' : msh.hasNrm = !msh.hasNrm; break;
		case 'l' : draw ^= draw_lit; break;

		case '0' : case '1' : case '2' :
		case '3' : case '4' : case '5' :
		case '6' : case '7' : case '8' :
		case '9' : if (key - '0' < lights) {
			rm = 0;
			if ((Lights[key - '0'].attr ^= L_on) & L_on) {
				rm = &Lights[key - '0'];
			}
		} break;
		case '[': if (rm) rm->sCos -= 1.; break;
		case ']': if (rm) rm->sCos += 1.; break;
		case '{': if (rm) rm->sExp -= 1.1; break;
		case '}': if (rm) rm->sExp += 1.1; break;

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

		case '\b' : {
			optiMesh(&msh, 1e-15, osdProgress);
			printf("vtx cnt : %d / %d\n", msh.vtxcnt, msh.maxvtx);
			printf("tri cnt : %d / %d\n", msh.tricnt, msh.maxtri);
		} break; // */

		case ':' : {
			printf("saving Mesh ... ");
			save_obj(&msh, "out.obj");
			printf("done\n");
			//~ centMesh(&msh, O);
		} break;
		case ';' : {
			//~ printf("load Mesh ... ");
			reload(&msh, obj, tex);
			centMesh(&msh, O);
			//~ printf("done\n");
		} break;
		/*case '=' : {
			sdivMesh(&msh);
			//~ centMesh(&msh, O);
		} break; // */
	}
	return 0;
}

//~ #define cubemap "dots"
//~ #define cubemap "mars"
//~ #define cubemap "media/text/mars/"

void readIni() {
	char *is = "= \t";
	static char temp[65536], *str = temp;
	char buff[65536], *ptr = NULL;
	FILE* fin = fopen("gx.ini", "rt");
	if (fin) for ( ; ; ) {
		//~ if (ptr) debug("%s", ptr);
		fgets(ptr = buff, sizeof(buff), fin);
		if (feof(fin)) break;
		buff[strlen(buff) - 1] = 0;

		if (*buff == '#') continue;				// Comment
		if (*buff == '\n') continue;			// Empty line
		if ((ptr = readcmd(buff, "division", is))) {
			readNum(ptr, &division);
			continue;
		}
		if ((ptr = readcmd(buff, "font", is))) {
			int len = strlen(ptr);
			if (str + len < temp + sizeof(temp)) {
				strcpy(fnt = str, ptr);
				str += len + 1;
			}
			continue;
		}
		if ((ptr = readcmd(buff, "texture", is))) {
			int len = strlen(ptr);
			if (str + len < temp + sizeof(temp)) {
				strcpy(tex = str, ptr);
				str += len + 1;
			}
			continue;
		}
		if ((ptr = readcmd(buff, "object", is))) {
			int len = strlen(ptr);
			if (str + len < temp + sizeof(temp)) {
				strcpy(obj = str, ptr);
				str += len + 1;
			}
			continue;
		}
		if ((ptr = readcmd(buff, "skybox", is))) {
			int len = strlen(ptr);
			if (str + len < temp + sizeof(temp)) {
				strcpy(sky = str, ptr);
				str += len + 1;
			}
			continue;
		}
	}
}

flip_scr flip;
int (*peek)(int);
int isChanged(float now, float p) {
	static float old = 0;
	if (fabs(old - now) > p)
		old = now;
	return old == now;
}
int isChangedTime(time_t now) {
	static time_t old = 0;
	int result;
	if (result = (old != now))
		old = now;
	return result;
}
int osdProgress(float prec) {
	char str[256];

	static gx_Rect roi;
	if (prec == +0.) {		// begin
		roi.x = roi.y = 0;
		roi.w = roi.h = 0;
		//~ gx_fillsurf(&offs, NULL, gx_fillAmix, 0xa0ffffff);
	}

	if (isChanged(prec, 1e-0)) {
		sprintf(str, "progressing: %.2f%%", prec);
		setCaption(str);
	}
	//~ gx_setblock(&offs, roi.x, roi.y, roi.x + roi.w, roi.y + roi.h, 0x0000ff);
	//~ gx_clipText(&roi, &font, str);
	//~ gx_drawText(&offs, roi.x, roi.y, &font, str, 0xffffff);
	//~ if (noChange(prec, 1e-0))
		//~ flip(&offs, 0/*, &roi */);
	return peek(0);
}

int main(int argc, char* argv[]) {
	struct {
		time_t	sec;
		time_t	cnt;
		int	col;
		int	fps;
	} fps = {0,0,0,0};
	char str[1024];
	void (*done)(void);
	int e;

	//~ if (argc == 2) division = atoi(argv[1]);
	initMesh(&msh, division * division);
	setbuf(stdout, NULL);
	msh.mtl = defmtl[9];

	for (e = 0; e < lights; e += 1) {
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
	bboxMesh(&msh, &tgt, &eye);
	printf("box min : %f, %f, %f\n", tgt.x, tgt.y, tgt.z);
	printf("box max : %f, %f, %f\n", eye.x, eye.y, eye.z);
	printf("vtx cnt : %d / %d\n", msh.vtxcnt, msh.maxvtx);
	printf("tri cnt : %d / %d\n", msh.tricnt, msh.maxtri);
	centMesh(&msh, O);

	vecldf(&up, 0, 1, 0, 1);
	vecldf(&tgt, 0, 0, 0, 1);
	vecldf(&eye, 0, 0, S, 1);

	//~ vecldf(&eye, 0, 0, O * 8, 1);
	camset(cam, &eye, &tgt, &up);

	kbdHND(0, 13);

	projv_mat(&projM, 10, ASPR, 1, 100);

	offs.clipPtr = 0;
	offs.width = SCRW;
	offs.height = SCRH;
	offs.flags = 0;
	offs.depth = 32;
	offs.pixeLen = 4;
	offs.scanLen = SCRW*4;
	offs.clutPtr = 0;
	offs.basePtr = &cBuff;

	if ((e = gx_loadFNT(&font, fnt))) {
		printf("Cannot open font '%s': %d\n", fnt, e);
		gx_doneSurf(&font);
		freeMesh(&msh);
		return -2;
	}

	if (init_WIN(&offs, &flip, &peek, &done, ratHND, kbdHND)) {
		printf("Cannot init surface\n");
		gx_doneSurf(&font);
		freeMesh(&msh);
		return -1;
	}

	for(fps.cnt = 0, fps.sec = time(NULL); peek(1) != -1; fps.cnt++) {
		matrix proj, view;
		vector v[6];
		#define nextln ((ln+=1)*16)
		int tris = 0, ln = -1;

		for (e = 0; e < SCRW * SCRH; e += 1) {		// clear
			cBuff[e] = 0x00000000;
			zBuff[e] = 0x3fffffff;
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
			matidn(&view);
			tris += g3_drawmesh(&msh, &view, cam);
			//~ view.zt += 2*O;
			//~ tris += g3_drawmesh(&msh, &view, cam);
			//~ view.zt -= 4*O;
			//~ tris += g3_drawmesh(&msh, &view, cam);
			//~ view.zt -= 2*O;
			//~ tris += g3_drawmesh(&msh, &view, cam);
		}
		if (draw & disp_zbuf) {
			for(e = 0; e < SCRW*SCRH; e += 1) {
				long z = zBuff[e];
				z = (z >> 15) & 0x1ff;
				if (z < 256) z = ~z & 0xff;
				cBuff[e] = ~(z << 16 | z << 8 | z);
			}
		}
		if (draw & disp_lght) {
			int i = 0;
			matmul(&proj, &projM, cammat(&view, cam));
			for (i = 0; i < lights; i += 1) {
				scalar lsize = .5;
				mappos(v, &proj, &Lights[i].pos);
				if (Lights[i].attr & L_on)
					g3_filloval(v, lsize, lsize, vecrgb(&Lights[i].diff).col);
				g3_drawoval(v, lsize, lsize, vecrgb(&Lights[i].ambi).col);
				if (vecdp3(&Lights[i].dir, &Lights[i].dir)) {
					mappos(v + 1, &proj, vecadd(v + 1, &Lights[i].pos, &Lights[i].dir));
					g3_drawline(v + 0, v + 1, vecrgb(&Lights[i].diff).col);
				}
			}
		}
		if (draw & disp_info) {
			//~ sprintf(str, "fps: %d / tpf(%d)", fps.fps, tris);gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, fps.col);
			sprintf(str, "Object size: %g, tvx:%d, tri:%d%s%s", O, msh.vtxcnt, msh.tricnt, msh.hasTex ? ", tex" : "", msh.hasNrm ? ", nrm" : "");
			gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, fps.col);

			//~ sprintf(str, "lights(Ambi:%d, Diff:%d, Spec:%d)", (draw & litAmbi) != 0, (draw & litDiff) != 0, (draw & litSpec) != 0);
			//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);

			sprintf(str, "camera(%g, %g, %g)", cam->pos.x, cam->pos.y, cam->pos.z);
			gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);

			if (rm) {
				sprintf(str, "light[%d].pos(%g, %g, %g); cos(%g), exp(%g)", rm-Lights, rm->pos.x, rm->pos.y, rm->pos.z, rm->sCos, rm->sExp);
				gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);
				//~ sprintf(str, "camera1(%g, %g, %g)", cam1.pos.x, cam1.pos.y, cam1.pos.z);
				//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);
			}
			for (e = 0; e < lights; e += 1) {
				argb lcol = vecrgb(&Lights[e].ambi);
				sprintf(str, "light %d : %s", e, Lights[e].attr & L_on ? "On" : "Off");
				gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, lcol.val);
			}

			sprintf(str, "%08x", (int)zBuff[SCRC]);
			gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffff00ff);
		}
		if (fps.cnt == 0) {
			sprintf(str, "tvx:%d, tri:%d/%d, fps: %d", msh.vtxcnt, tris, msh.tricnt, fps.fps);
			setCaption(str);
		}

		flip(&offs, 0);

		#undef nextln
	}

	gx_doneSurf(&font);
	freeMesh(&msh);
	done();
	return 0;
}
