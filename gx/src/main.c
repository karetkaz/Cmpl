#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g2_argb.h"
#include "g3_draw.h"
#include "drv_gui.h"

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

double O = 2;
double NormalSize = 1;
double epsilon = 1e-10;
double lightsize = .2;

double speed = .1;
#define MOV_d speed
#define MOV_D (10 * speed)
#define ROT_r (toRad(MOV_d))
#define ROT_R (toRad(MOV_D))

char *ltP = 0;		// light filename or source code starting with ';'
int ltPLn = 0;			// object filename
//~ char *ltS = 0;		// spot light filename or source code starting with ';'
//~ char *ltD = 0;		// directional light filename or source code starting with ';'

char *ini = "app.ini";	// ini filename
char *fnt = NULL;		// font filename
char *ccStd = NULL;//"std.gxc";	// stdlib script file (from gx.ini)
char *ccGfx = NULL;//"gfx.gxc";	// gfxlib script file (from gx.ini)
char *ccLog = NULL;//"debug.out";	// use stderr
char *ccDmp = NULL;//"debug.out";

char *obj = NULL;		// object filename
char *tex = NULL;		// texture filename
int objLn = 0;			// object fileline
//~ char *sky = 0;	// skyBox

int resx = 800;
int resy = 600;

double F_fovy = 30.;
double F_near = 1.;
double F_far = 100.;

int draw = draw_fill | cull_back | disp_info | zero_cbuf | zero_zbuf | swap_buff;// | OnDemand;

union vector eye, tgt, up;
struct camera cam[1];

flip_scr flip = NULL;		// swap buffer method
struct gx_Surf offs;		// offscreen
struct gx_Surf font;		// font
struct mesh msh;

// selected vertex/light
Light rm = 0;
int vtx = 0;

#include "utils.i"
static struct Light userLights[32] = {
#define lz 2
#define lp 2
#define lA 10
	{	// light0(Gray)
		L_on,
		{{.6, .6, .6, 1}},	// ambi
		{{.9, .9, .9, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., 0., 0., 0}},	// attn
		//~ {{-2, +15, -15, 1}},	// pos
		{{+lp, +lp, +lz, 1}},	// pos
		{{0., 0., 0., 0}},	// dir
		0, 32},
		//~ &userLights[1]}, // */
	{	// light1(RGB/R)
		L_on,
		{{.4, .0, .0, 1}},	// ambi
		{{.8, .0, .0, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., 0., 0., 0}},	// attn
		{{+lp, -lp, +lz, 1}},	// pos
		{{-0., +0., -0., 0}},	// dir // is normalized in main()
		lA, 32},
		//~ &userLights[2]}, // */
	{	// light2(RGB/G)
		L_on,
		{{0., .4, .0, 1}},	// ambi
		{{.0, .8, .0, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., .0, 0., 0}},	// attn
		{{-lp, -lp, +lz, 1}},	// pos
		{{-0., +0., -0., 0}},	// dir // is normalized in main()
		lA, 32},
		//~ &userLights[3]}, // */
	{	// light3(RGB/B)
		L_on,
		{{.0, .0, .4, 1}},	// ambi
		{{.0, .0, .8, 1}},	// diff
		{{1., 1., 1., 1}},	// spec
		{{1., .0, 0., 0}},	// attn
		{{-lp, +lp, +lz, 1}},	// pos
		{{-0., +0., -0., 0}},	// dir // is normalized in main()
		lA, 32},
		//~ NULL},
#undef lz
#undef lp
#undef lA
};
vector getobjvec(int Normal) {
	if (rm != NULL)
		return Normal ? &rm->dir : &rm->pos;
	if ((vtx %= msh.vtxcnt + 1))
		return (Normal ? msh.nrm : msh.pos) + vtx - 1;
	return NULL;
}

#include <time.h>
static inline int64_t timenow() {return clock();}
static inline double ticksinsecs(int64_t start) {return (timenow() - start) / (double)(CLOCKS_PER_SEC);}

//{ extension manager, or somethig like that
typedef struct strlist {
	const char *key;
	const char *value;
	struct strlist *next;
} *strlist;

strlist extmgr = NULL;

char* findext(char* ext) {
	strlist exts = extmgr;
	while (exts) {
		if (stricmp(ext, exts->key) == 0)
			return (char*)exts->value;
		exts = exts->next;
	}
	return NULL;
}

//}

void readIni(char *file) {
	enum {
		skip = 1,

		main,

		//~ screen,
		object,			// 

		objlit,			// light properties
		objmtl,			// material properties
		objfun,			// 3d parametric(s, t) surface
		//~ ltPfun,
	} section = 0;

	char *is = " ";

	int line = 0, eof = 0;

	static char temp[65535];

	char *ptr = temp, *arg;
	FILE* fin = fopen(file, "rb");

	char *mtl = NULL;
	//~ char *fun = NULL;

	Light lit = NULL;
	int litidx = 0;

	// todo: use me
	char *ltS, *ltD;

	ccStd = NULL;
	ccGfx = NULL;
	ccLog = NULL;
	ccDmp = NULL;

	obj = 0;
	objLn = 0;

	ltP = 0;
	ltPLn = 0;

	ltS = 0;
	//~ ltSLn = 0;
	ltD = 0;
	//~ ltDLn = 0;

	tex = 0;
	fnt = 0;

	if (fin) while (!eof) {
		int cnt = temp + sizeof(temp) - ptr;

		line += 1;
		if (!fgets(ptr, cnt, fin)) {
			strncpy(ptr, "[eof]", cnt);
			eof = 1;
		}

		if (feof(fin)) {
			strncpy(ptr, "[eof]", cnt);
			eof = 1;
		}

		//~ clear '\n' from end of line
		if ((arg = strchr(ptr, 13)))
			*arg = 0;
		if ((arg = strchr(ptr, 10)))
			*arg = 0;

		if (*ptr == 0) continue;								// Empty line
		if (*ptr == '#' || *ptr == ';') continue;				// Comment

		if (*ptr == '[' && (arg = strchr(ptr, ']'))) {
			char sectName[1024];

			int funend = section == objfun;

			*arg = 0;
			if (arg - ptr > sizeof(sectName)) {
				debug("section name too large");
				abort();
			}
			//~ arg = ptr + 1;
			strncpy(sectName, ptr + 1, sizeof(sectName));
			section = skip;

			if (strcmp(sectName, "main") == 0) {
				section = main;
			}
			else if (strcmp(sectName, "object") == 0) {
				section = object;
			}
			else if (strcmp(sectName, "light") == 0) {
				if (litidx < (sizeof(userLights) / sizeof(*userLights))) {
					lit = &userLights[litidx++];
					lit->attr |= L_on;
					section = objlit;
				}
			}
			else if (mtl && strcmp(mtl, sectName) == 0) {			// object.material
				section = objmtl;
			}

			if (funend) {
				//~ strncpy(ptr, "setPos(x, y, z);\n", cnt);
				strncpy(ptr, "", cnt);
				ptr += strlen(ptr) + 1;
				//~ debug("recorded function: `%s`", fun);
				//~ fun = NULL;
			}

			if (obj && strcmp(obj, sectName) == 0) {				// object.function
				section = objfun;
				objLn = line;
				obj = ptr;
			}
			if (ltP && strcmp(ltP, sectName) == 0) {				// object.pointLight
				section = objfun;
				ltPLn = line;
				ltP = ptr;
			}
			if (ltS && strcmp(ltS, sectName) == 0) {				// object.spotLight
				section = objfun;
				//~ ltSLn = line;
				ltS = ptr;
			}
			if (ltD && strcmp(ltD, sectName) == 0) {				// object.directionalLight
				section = objfun;
				//~ ltDLn = line;
				ltD = ptr;
			}

			if (section >= objfun) {
				//~ fun = ptr;
				strncpy(ptr, ";\n"			// first char is a ! so is not a file
					/* the old way
					"double s = gets();\n"
					"double t = gett();\n"
					"double x = s;\n"
					"double y = t;\n"
					"double z = 0;\n"
					// */
					, cnt);
				ptr += strlen(ptr);
			}

			//~ debug("%s", arg);
			continue;
		}

		if (section == objfun) {		// record function body into buffer
			int len = strlen(ptr);
			ptr[len++] = '\n';
			ptr[len] = 0;
			ptr += len;
			continue;
		}
		if (section == objmtl) {
			if ((arg = readKVP(ptr, "Ka", "=", is))) {
				if (*readVec(arg, &msh.mtl.ambi, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "Kd", "=", is))) {
				if (*readVec(arg, &msh.mtl.diff, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "Ks", "=", is))) {
				if (*readVec(arg, &msh.mtl.spec, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "KS", "=", is))) {
				if (*readF32(arg, &msh.mtl.spow))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			//~ union vector ambi;		// Ambient
			//~ union vector diff;		// Diffuse
			//~ union vector spec;		// Specular
			//~ scalar spow;		// Shin...
			//~ union vector emis;		// Emissive
			debug("invalid light param: %s(%d): %s", file, line, ptr);
		}
		if (section == objlit) {
			if ((arg = readKVP(ptr, "Ka", "=", is))) {
				if (*readVec(arg, &lit->ambi, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "Kd", "=", is))) {
				if (*readVec(arg, &lit->diff, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "Ks", "=", is))) {
				if (*readVec(arg, &lit->spec, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "pos", "=", is))) {
				if (*readVec(arg, &lit->pos, 1))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "dir", "=", is))) {
				if (*readVec(arg, &lit->dir, 0))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "attn", "=", is))) {
				if (*readVec(arg, &lit->attn, 0))
					debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if ((arg = readKVP(ptr, "spot", "=", is))) {
				if (!*(arg = readF32(arg, &lit->sCos)))
					continue;
				if (!(arg = readCmd(arg, ",")))
					continue;
				if (!*(arg = readF32(arg, &lit->sExp)))
					continue;
				debug("invalid format: %s(%d): %s", file, line, arg);
				continue;
			}
			if (strcmp(ptr, "off") == 0) {
				lit->attr = 0;
				continue;
			}
			debug("invalid light param: %s(%d): %s", file, line, ptr);
		}

		if (section == object) {
			if ((arg = readKVP(ptr, "material", "=", is))) {
				mtl = strcpy(ptr, arg);
				ptr += strlen(ptr) + 1;
				continue;
			}
			if ((arg = readKVP(ptr, "texture", "=", is))) {
				tex = strcpy(ptr, arg);
				ptr += strlen(ptr) + 1;
				continue;
			}
			if ((arg = readKVP(ptr, "object", "=", is))) {
				obj = strcpy(ptr, arg);
				ptr += strlen(ptr) + 1;
				continue;
			}

			if ((arg = readKVP(ptr, "light.obj", "=", is))) {
				ltP = strcpy(ptr, arg);
				ptr += strlen(ptr) + 1;
				continue;
			}

			// temporary
			if ((arg = readKVP(ptr, "object.size", "=", is))) {
				if (*readFlt(arg, &O))
					debug("invalid number: %s", arg);
				continue;
			}
			if ((arg = readKVP(ptr, "normal.size", "=", is))) {
				if (*readFlt(arg, &NormalSize))
					debug("invalid number: %s", arg);
				continue;
			}
			if ((arg = readKVP(ptr, "lights.size", "=", is))) {
				if (*readFlt(arg, &lightsize))
					debug("invalid number: %s", arg);
				continue;
			}
		}
		if (section == main) {
			if ((arg = readKVP(ptr, "screen.font", "=", is))) {
				int len = strlen(arg);
				strcpy(ptr, arg);
				fnt = ptr;
				ptr += len + 1;
				continue;
			}
			if ((arg = readKVP(ptr, "screen.width", "=", is))) {
				char *end = readInt(arg, &resx);
				if (*end)
					debug("invalid number: %s:%d", arg, *end);
				continue;
			}
			if ((arg = readKVP(ptr, "screen.height", "=", is))) {
				if (*readInt(arg, &resy))
					debug("invalid number: %s", arg);
				continue;
			}
			/* TODO
			if ((arg = readKVP(ptr, "speed", "=", is))) {
				if (*readFlt(arg, &speed))
					debug("invalid number: %s", arg);
				continue;
			}// */
			if ((arg = readKVP(ptr, "frustum.fovy", "=", is))) {
				if (*readFlt(arg, &F_fovy))
					debug("invalid number: %s", arg);
				continue;
			}

			if ((arg = readKVP(ptr, "frustum.near", "=", is))) {
				if (*readFlt(arg, &F_near))
					debug("invalid number: %s", arg);
				continue;
			}

			if ((arg = readKVP(ptr, "frustum.far", "=", is))) {
				if (*readFlt(arg, &F_far))
					debug("invalid number: %s", arg);
				continue;
			}

			/*if ((arg = readKVP(ptr, "render.ondemand", "=", is))) {
				int value;
				if (*readInt(arg, &value))
					debug("invalid number: %s", arg);
				if (value != 0)
					draw |= OnDemand;
				continue;
			}*/
			if ((arg = readKVP(ptr, "epsilon", "=", is))) {
				if (*readFlt(arg, &epsilon))
					debug("invalid number: %s", arg);
				continue;
			}

			if ((arg = readKVP(ptr, "script.stdlib", "=", is))) {
				ccStd = *arg ? strcpy(ptr, arg) : NULL;
				ptr += strlen(ptr) + 1;
				continue;
			}
			if ((arg = readKVP(ptr, "script.gfxlib", "=", is))) {
				ccGfx = *arg ? strcpy(ptr, arg) : NULL;
				ptr += strlen(ptr) + 1;
				continue;
			}
			if ((arg = readKVP(ptr, "script.logfile", "=", is))) {
				ccLog = *arg ? strcpy(ptr, arg) : NULL;
				ptr += strlen(ptr) + 1;
				continue;
			}
			if ((arg = readKVP(ptr, "script.dumpfile", "=", is))) {
				ccDmp = *arg ? strcpy(ptr, arg) : NULL;
				ptr += strlen(ptr) + 1;
				continue;
			}

			// open file types
			if ((arg = readKVP(ptr, "open", ".", ""))) {
				char *ext = arg;
				int len = strlen(ptr);
				//~ while (*arg && *arg != '=')
				while (*arg) {
					if (strchr(is, *arg))
						break;
					if (*arg == '=')
						break;
					arg++;
				}
				*arg++ = 0;
				if ((arg = readKVP(arg, "", "=", is))) {
					strlist le;

					ptr += len + 1;
					le = (void*)ptr;

					le->key = ext;
					le->value = arg;
					le->next = extmgr;
					extmgr = le;
					ptr += sizeof(struct strlist);
					//~ debug("open(%s): `%s`", ext, arg);
				}
				continue;
			}
		}
	}
	if (litidx > 0) {
		int i = 0;
		for (i = 0; i + 1 < litidx; i++) {
			userLights[i].next = &userLights[i+1];
		}
		msh.lit = userLights;
	}
	//~ for (arg = temp; arg < ptr; arg += 1) {putc(*arg, stdout);}
	//~ debug("ini size: %d", ptr - temp);
	//~ debug("%s", obj);
}

static int textureMesh(mesh msh, char *tex) {
	int res = -1;
	if (tex != NULL) {
		char *ext = fext(tex);
		msh->freeTex = 1;
		msh->map = gx_createSurf(0, 0, 0, 0);
		if (stricmp(ext, "bmp") == 0)
			res = gx_loadBMP(msh->map, tex, 32);
		if (stricmp(ext, "jpg") == 0)
			res = gx_loadJPG(msh->map, tex, 32);
		if (stricmp(ext, "png") == 0)
			res = gx_loadPNG(msh->map, tex, 32);
		debug("ReadText('%s'): %d", tex, res);
	}
	return res;
}
int readorevalMesh(mesh msh, char *src, int line, char *tex, int div) {
	int64_t ticks = timenow();
	int res = -8;
	if (src != NULL) {
		res = textureMesh(msh, tex);
		res = (line && *src == ';') ? evalMesh(msh, div, div, src + 1, ini, line) : readMesh(msh, src);
		debug("readorevalMesh('%s', %s): %d\tTime: %g", *src != ';' ? src : "`script`", tex ? tex : "", res, ticksinsecs(ticks));
	}
	return res;
}

enum Events {
	doScript = 10,
	doSaveMesh = 11,
	doSaveImage = 12,
};

int ratHND1(int btn, int mx, int my) {
	static int ox, oy;
	int mdx = mx - ox;
	int mdy = my - oy;
	ox = mx; oy = my;
	if (btn == 1) {
		union matrix tmp;
		if (mdx) {
			// load rotation matrix
			matldR(&tmp, &cam->dirU, mdx * ROT_R);
			// rotate forward and right camera normal vectors
			vecnrm(&cam->dirF, matvph(&cam->dirF, &tmp, &cam->dirF));
			vecnrm(&cam->dirR, matvph(&cam->dirR, &tmp, &cam->dirR));
			// recalculate UP, as cross product of left and forward
			veccrs(&cam->dirU, &cam->dirF, &cam->dirR);
			//~ rotate position
			matvph(&cam->pos, &tmp, &cam->pos);
		}
		if (mdy) {
			matldR(&tmp, &cam->dirR, mdy * ROT_R);
			vecnrm(&cam->dirF, matvph(&cam->dirF, &tmp, &cam->dirF));
			vecnrm(&cam->dirR, matvph(&cam->dirR, &tmp, &cam->dirR));
			veccrs(&cam->dirU, &cam->dirF, &cam->dirR);
			matvph(&cam->pos, &tmp, &cam->pos);
		}
	}
	else if (btn == 2) {
		vector N;		// normal vector
		if ((N = getobjvec(1))) {
			union matrix tmp, tmp1, tmp2;
			matldR(&tmp1, &cam->dirU, mdx * ROT_r);
			matldR(&tmp2, &cam->dirR, mdy * ROT_r);
			matmul(&tmp, &tmp1, &tmp2);
			vecnrm(N, matvp3(N, &tmp, N));
		}
		else {
			if (mdy) camrot(cam, &cam->dirR, NULL, mdy * ROT_r);		// turn: up / down
			if (mdx) camrot(cam, &cam->dirU, NULL, mdx * ROT_r);		// turn: left / right
		}
	}
	else if (btn == 3) {
		vector P;		// position
		if ((P = getobjvec(0))) {
			union matrix tmp, tmp1, tmp2;
			matldT(&tmp1, &cam->dirU, -mdy * MOV_d);
			matldT(&tmp2, &cam->dirR, mdx * MOV_d);
			matmul(&tmp, &tmp1, &tmp2);
			matvph(P, &tmp, P);
		}
		else {
			if (mdy) cammov(cam, &cam->dirF, mdy * MOV_d);		// move: forward
			//~ if (mdx) camrot(cam, &cam->dirU, mdx * ROT_r);		// turn: left / right
		}
	}
	return 0;
}
int kbdHND(int key/* , int lkey2 */) {
	const scalar sca = 8;
	switch (key) {
		case  27 : return -1;
		case  13 : {
			vecldf(&eye, 0, 0, O * sca, 1);
			camset(cam, &eye, &tgt, &up);
		} break;

		case '*' : centMesh(&msh, O = 2.); break;
		case '+' : centMesh(&msh, O *= 2.); break;
		case '-' : centMesh(&msh, O /= 2.); break;

		case 'O' : vecldf(&cam->pos, 0, 0, 0, 1); break;

		case 'w' : cammov(cam, &cam->dirF, +MOV_d); break;
		case 'W' : cammov(cam, &cam->dirF, +MOV_D); break;
		case 's' : cammov(cam, &cam->dirF, -MOV_d); break;
		case 'S' : cammov(cam, &cam->dirF, -MOV_D); break;
		case 'd' : cammov(cam, &cam->dirR, +MOV_d); break;
		case 'D' : cammov(cam, &cam->dirR, +MOV_D); break;
		case 'a' : cammov(cam, &cam->dirR, -MOV_d); break;
		case 'A' : cammov(cam, &cam->dirR, -MOV_D); break;
		case ' ' : cammov(cam, &cam->dirU, +MOV_D); break;
		case 'c' : cammov(cam, &cam->dirU, -MOV_D); break;

		case '\t': draw = (draw & ~draw_mode) | ((draw + LOBIT(draw_mode)) & draw_mode); break;
		case '/' : draw = (draw & ~cull_mode) | ((draw + LOBIT(cull_mode)) & cull_mode); break;
		case '?' : draw = (draw & ~cull_mode) | ((draw - LOBIT(cull_mode)) & cull_mode); break;

		case '`' : draw ^= disp_info; break;
		case 'L' : draw ^= temp_lght; break;
		case 'b' : draw ^= disp_bbox; break;
		case 'Z' : draw ^= temp_zbuf; break;
		case 'X' : draw ^= disp_oxyz; break;

		case 'Q' : draw ^= swap_buff; break;

		case 't' : draw ^= draw_tex; break;
		case 'T' : msh.hasTex = !msh.hasTex; break;

		case 'n' : draw ^= disp_norm; break;
		case 'N' : msh.hasNrm = !msh.hasNrm; break;

		case 'l' : draw ^= draw_lit; break;

		case '0' : case '1' : case '2' :
		case '3' : case '4' : case '5' :
		case '6' : case '7' : case '8' :
		case '9' : {
			int pos = key - '0';
			Light l = userLights;
			while (l && pos) {
				l = l->next;
				pos -= 1;
			}

			if (l == NULL)
				break;

			rm = NULL;
			if ((l->attr ^= L_on) & L_on) {
				rm = l;
			}
		} break;
		case '~' : vtx = 0; rm = NULL; break;
		case '.' : vtx += 1; break;
		case '>' : vtx -= 1; break;

		/*case '\b' : {
			optiMesh(&msh, epsilon, osdProgress);
			printf("vtx cnt : %d / %d\n", msh.vtxcnt, msh.maxvtx);
			printf("tri cnt : %d / %d\n", msh.tricnt, msh.maxtri);
		} break;

		case '\\': {
			printf("normalize Mesh ...");
			normMesh(&msh, 0);
			printf("done\n");
		} break;
		case '|': {
			printf("normalize Mesh ...");
			normMesh(&msh, epsilon);
			printf("done\n");
		} break;

		case ';' : {
			//~ printf("load Mesh ... ");
			reload(&msh, NULL, NULL, NULL);
			centMesh(&msh, O);
			//~ printf("done\n");
		} break;
		case '"' : {
			printf("subdivide Mesh ...");
			sdivMesh(&msh, 1);
			printf("done\n");
		} break;

		//~ case 18:		// CTrl + r
			//~ readIni();
			//~ break;

		// */

		case 16:		// CTrl + p
			return doSaveImage;

		case 19:		// CTrl + s
			return doSaveMesh;

		case 10:		// CTrl + Cr
			readIni(ini);
			return doScript;

		default:
			debug("kbdHND(key(%d), lkey(%d))", key, 0);//lkey2);
			break;
	}
	return 0;
}

peek_msg peekMsg;
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
static void meshInfo(mesh msh) {
	union vector min, max;
	bboxMesh(msh, &min, &max);
	printf("box min : %f, %f, %f\n", min.x, min.y, min.z);
	printf("box max : %f, %f, %f\n", max.x, max.y, max.z);
	printf("vtx cnt : %d / %d\n", msh->vtxcnt, msh->maxvtx);
	printf("tri cnt : %d / %d\n", msh->tricnt, msh->maxtri);
	//~ printf("vtx cnt : %d / %d\n", msh->vtxcnt, msh->maxvtx);
	//~ printf("tri cnt : %d / %d\n", msh->tricnt, msh->maxtri);
}

#if 1	// compiler
#include "lib/libpvmc/pvmc.h"

//{#region Surfaces

enum surfCalls {
	//~ surfOpGetDepth,
	surfOpGetWidth,
	surfOpGetHeight,

	surfOpGetPixel,
	surfOpGetPixfp,
	surfOpSetPixel,

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

	surfOpClipRect,

	//~ surfOpGetRect,
	//~ surfOpSetRect,
	surfOpNewSurf,
	surfOpDelSurf,

	//~ surfOpSetRectV,
	surfOpBmpWrite,
	surfOpBmpRead,
	surfOpJpgRead,
	surfOpPngRead,

	surfOpEvalPxCB,
	surfOpFillPxCB,
	surfOpCopyPxCB,
	surfOpEvalFpCB,
	surfOpFillFpCB,
	surfOpCopyFpCB,
};

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

static int surfCall(state rt) {
	switch (rt->fdata) {
		default: return -1;

		case surfOpGetWidth:
		case surfOpGetHeight: {
			gx_Surf surf = getSurf(popi32(rt));
			if (surf) switch (rt->fdata) {
				case surfOpGetWidth: setret(rt, int32_t, surf->width); break;
				case surfOpGetHeight: setret(rt, int32_t, surf->height); break;
				default: return -1;
			}
			return 0;
		} return -1;

		case surfOpClipRect: {
			gx_Surf surf = getSurf(popi32(rt));
			gx_Rect rect = popref(rt);
			if (surf) {
				void *ptr = gx_cliprect(surf, rect);
				setret(rt, int32_t, ptr != NULL);
			}
			return 0;
		} return -1;

		case surfOpGetPixfp: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				double x = popf64(rt);
				double y = popf64(rt);
				setret(rt, int32_t, gx_getpix16(surf, x * 65535, y * 65535, 1));
				return 0;
			}
		} return -1;
		case surfOpGetPixel: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x = popi32(rt);
				int y = popi32(rt);
				setret(rt, int32_t, gx_getpixel(surf, x, y));
				return 0;
				/* but this way is faster
				if ((unsigned)x > (unsigned)surf->width || (unsigned)y > (unsigned)surf->height) {
					setret(rt, int32_t, 0);
					return 0;
				}
				int rowy = y * surf->scanLen;
				if (surf->depth == 32) {
					uint32_t *ptr = (uint32_t*)((char*)surf->basePtr + rowy);
					setret(rt, int32_t, ptr[x]);
					return 0;
				}
				else if (surf->depth == 8) {
					uint8_t *ptr = (uint8_t*)((char*)surf->basePtr + rowy);
					setret(rt, int32_t, ptr[x]);
					return 0;
				}// */
			}
		} return -1;
		case surfOpSetPixel: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				int x = popi32(rt);
				int y = popi32(rt);
				int col = popi32(rt);
				gx_setpixel(surf, x, y, col);
				return 0;
				/* but this way is faster, but it crashes memory
				if ((unsigned)x > (unsigned)surf->width || (unsigned)y > (unsigned)surf->height) {
					return -2;
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
				}// */
			}
		} return -1;

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
		} return -1;
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
		} return -1;
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
		} return -1;
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
		} return -1;
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
		} return -1;
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
		} return -1;

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
			setret(rt, gxSurfHnd, dst);
			return 0;
		} return -1;
		case surfOpZoomSurf: {		// int gx_zoomsurf(gx_Surf dst, gx_Rect rect, gx_Surf src, gx_Rect roi, int lin)
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
			setret(rt, gxSurfHnd, dst);
			return 0;
		} return -1;

		case surfOpEvalPxCB: {		// gxSurf evalSurfrgb(gxSurf dst, gxRect &roi, int callBack(int x, int y));
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = findref(rt, popref(rt));
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
				char *retptr = rt->retv;
				char *argptr = rt->argv;
				for (sy = 0; sy < sdst->height; sy += 1) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (sx = 0; sx < sdst->width; sx += 1) {
						if (vmCall(rt, callback, sx, sy) != 0) {
							//~ debug("error");
							//~ dump(s, dump_sym | dump_asm, callback, "error:\n");
							return -1;
						}
						cBuff[sx] = getret(rt, long);
					}
				}
				rt->retv = retptr;
				rt->argv = argptr;
			}
			setret(rt, gxSurfHnd, dst);
		} break;
		case surfOpFillPxCB: {		// gxSurf fillSurfrgb(gxSurf dst, gxRect &roi, int callBack(int col));
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = findref(rt, popref(rt));

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				debug("Unimplemented: fillSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				char *cBuffY = sdst->basePtr;
				char *retptr = rt->retv;
				char *argptr = rt->argv;
				for (sy = 0; sy < sdst->height; sy += 1) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (sx = 0; sx < sdst->width; sx += 1) {
						if (vmCall(rt, callback, cBuff[sx]) != 0)
							return -1;
						cBuff[sx] = getret(rt, long);
					}
				}
				rt->retv = retptr;
				rt->argv = argptr;
			}
			setret(rt, gxSurfHnd, dst);
		} break;
		case surfOpCopyPxCB: {		// gxSurf copySurfrgb(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, int callBack(int dst, int src));
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = findref(rt, popref(rt));

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
					char *retptr = rt->retv;
					char *argptr = rt->argv;
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					for (y = clip.y; y < y1; y += 1) {
						long* cbdst = (long*)dptr;
						long* cbsrc = (long*)sptr;
						for (x = clip.x; x < x1; x += 1) {
							if (vmCall(rt, callback, *cbdst, *cbsrc) != 0)
								return -1;
							*cbdst = getret(rt, long);
							cbdst += 1;
							cbsrc += 1;
						}
						dptr += sdst->scanLen;
						sptr += ssrc->scanLen;
					}
					rt->retv = retptr;
					rt->argv = argptr;
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
			setret(rt, gxSurfHnd, dst);
		} break;

		case surfOpEvalFpCB: {		// gxSurf evalSurf(gxSurf dst, gxRect &roi, vec4f callBack(double x, double y));
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = findref(rt, popref(rt));

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
				char *retptr = rt->retv;
				char *argptr = rt->argv;
				for (y01 = sy = 0; sy < sdst->height; sy += 1, y01 += dy01) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (x01 = sx = 0; sx < sdst->width; sx += 1, x01 += dx01) {
						if (vmCall(rt, callback, x01, y01) != 0)
							return -1;
						cBuff[sx] = vecrgb(retptr(rt, union vector)).col;
					}
				}
				rt->retv = retptr;
				rt->argv = argptr;
			}
			setret(rt, gxSurfHnd, dst);
		} break;
		case surfOpFillFpCB: {		// gxSurf fillSurf(gxSurf dst, gxRect &roi, vec4f callBack(vec4f col));
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			symn callback = findref(rt, popref(rt));

			gx_Surf sdst = getSurf(dst);

			static int first = 1;
			if (roi && first) {
				first = 0;
				debug("Unimplemented: fillSurf called with a roi");
			}
			if (sdst && callback) {
				int sx, sy;
				char *cBuffY = sdst->basePtr;
				char *retptr = rt->retv;
				char *argptr = rt->argv;
				for (sy = 0; sy < sdst->height; sy += 1) {
					long *cBuff = (long*)cBuffY;
					cBuffY += sdst->scanLen;
					for (sx = 0; sx < sdst->width; sx += 1) {
						if (vmCall(rt, callback, vecldc(rgbval(cBuff[sx]))) != 0)
							return -1;
						cBuff[sx] = vecrgb(retptr(rt, union vector)).col;
					}
				}
				rt->retv = retptr;
				rt->argv = argptr;
			}
			setret(rt, gxSurfHnd, dst);
		} break;
		case surfOpCopyFpCB: {		// gxSurf copySurf(gxSurf dst, int x, int y, gxSurf src, gxRect &roi, float64 alpha, vec4f callBack(vec4f dst, vec4f src));
			gxSurfHnd dst = popi32(rt);
			int x = popi32(rt);
			int y = popi32(rt);
			gxSurfHnd src = popi32(rt);
			gx_Rect roi = popref(rt);
			double alpha = popf64(rt);
			symn callback = findref(rt, popref(rt));

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
					char *retptr = rt->retv;
					char *argptr = rt->argv;
					x1 = clip.x + clip.w;
					y1 = clip.y + clip.h;
					if (alpha > 0 && alpha < 1) {
						int alpha16 = alpha * (1 << 16);
						for (y = clip.y; y < y1; y += 1) {
							long* cbdst = (long*)dptr;
							long* cbsrc = (long*)sptr;
							for (x = clip.x; x < x1; x += 1) {
								if (vmCall(rt, callback, vecldc(rgbval(*cbdst)), vecldc(rgbval(*cbsrc))) != 0)
									return -1;
								*(argb*)cbdst = rgbmix16(*(argb*)cbdst, vecrgb(retptr(rt, union vector)), alpha16);
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
								if (vmCall(rt, callback, vecldc(rgbval(*cbdst)), vecldc(rgbval(*cbsrc))) != 0)
									return -1;
								*cbdst = vecrgb(retptr(rt, union vector)).col;
								cbdst += 1;
								cbsrc += 1;
							}
							dptr += sdst->scanLen;
							sptr += ssrc->scanLen;
						}
					}
					rt->retv = retptr;
					rt->argv = argptr;
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
			setret(rt, gxSurfHnd, dst);
		} break;

		case surfOpNewSurf: {
			int w = popi32(rt);
			int h = popi32(rt);
			gxSurfHnd hnd = newSurf(w, h);
			setret(rt, gxSurfHnd, hnd);
		} break;
		case surfOpDelSurf: {
			delSurf(popi32(rt));
		} break;

		case surfOpBmpRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int result = gx_loadBMP(surf, fileName, 32);
				setret(rt, gxSurfHnd, dst);
				//~ debug("gx_readBMP(%s):%d;", fileName, result);
				return result;
			}
		} return -1;
		case surfOpJpgRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int result = gx_loadJPG(surf, fileName, 32);
				setret(rt, gxSurfHnd, dst);
				//~ debug("readJpg(%s):%d;", fileName, result);
				return result;
			}
		} return -1;
		case surfOpPngRead: {
			gx_Surf surf;
			gxSurfHnd dst = popi32(rt);
			if ((surf = getSurf(dst))) {
				char *fileName = popstr(rt);
				int result = gx_loadPNG(surf, fileName, 32);
				setret(rt, gxSurfHnd, dst);
				//~ debug("gx_readPng(%s):%d;", fileName, result);
				return result;
			}
		} return -1;
		case surfOpBmpWrite: {
			gx_Surf surf;
			if ((surf = getSurf(popi32(rt)))) {
				char *fileName = popstr(rt);
				return gx_saveBMP(fileName, surf, 0);
			}
		} return -1;

		case surfOpClutSurf: {		// gxSurf clutSurf(gxSurf dst, gxRect &roi, gxClut &lut);
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);
			gx_Surf sdst = getSurf(dst);

			if (sdst && lut) {
				gx_clutsurf(sdst, roi, lut);
			}
			setret(rt, gxSurfHnd, dst);
			return 0;
		} return -1;
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
			setret(rt, gxSurfHnd, dst);
			return 0;
		} return -1;
		case surfOpGradSurf: {		// gxSurf gradSurf(gxSurf dst, gxRect &roi, gxClut &lut, int mode)
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			gx_Clut lut = popref(rt);
			int mode = popi32(rt);
			gx_Surf sdst = getSurf(dst);

			if (sdst) {
				gx_gradsurf(sdst, roi, lut, mode);
				setret(rt, gxSurfHnd, dst);
				return 0;
			}
		} return -1;

		case surfOpBlurSurf: {		// gxSurf blurSurf(gxSurf dst, gxRect &roi, int radius)
			gxSurfHnd dst = popi32(rt);
			gx_Rect roi = popref(rt);
			int radius = popi32(rt);

			gx_Surf sdst = getSurf(dst);

			if (sdst) {
				gx_blursurf(sdst, roi, radius);
				setret(rt, gxSurfHnd, dst);
				return 0;
			}
		} return -1;

	}
	return 0;
}
//}#endregion

//{#region Mesh & Camera
enum meshCalls {
	//~ meshGet,		// vec3f meshGet(int index)
	//~ meshOpAddVtx,		// void meshAddVertex(vec3f pos, vec3f nrm, vec2f tex)

	meshOpSetPos,		// void meshPos(int Index, vec3f Value)
	meshOpGetPos,		// vec3f meshPos(int Index)
	meshOpSetNrm,		// void meshNrm(int Index, vec3f Value)
	meshOpGetNrm,		// vec3f meshNrm(int Index)
	meshOpSetTex,		// void meshTex(int Index, vec2f Value)
	//~ meshOpGetTex,		// vec2f meshTex(int Index)

	meshOpSetTri,		// int meshSetTri(int t, int p1, int p2, int p3)
	meshOpAddTri,		// int meshAddTri(int p1, int p2, int p3)
	meshOpAddQuad,		// int meshAddTri(int p1, int p2, int p3, int p4)
	meshOpSetSeg,		// int meshSetSeg(int t, int p1, int p2)
	meshOpAddSeg,		// int meshAddSeg(int p1, int p2)

	meshOpGetFlags,		// int meshFlags()
	meshOpSetFlags,		// int meshFlags(int Value)

	meshOpInit,			// int meshInit(int Capacity)
	meshOpTexture,		// int meshTexture(gxSurf image)
	meshOpRead,			// int meshRead(string file)
	meshOpSave,			// int meshSave(string file)
	meshOpCenter,		// void Center()
	meshOpNormalize,	// void Normalize()

	meshOpInfo,			// void meshInfo()

	//~ meshOpGetCap,		// int meshCap()	// capacity
	//~ meshOpGetLen,		// int meshLen()	// length

};
enum camCalls {
	camOpMove,
	camOpRotate,
	camOpLookAt,

	camGetPos,
	camSetPos,

	camGetUp,
	camSetUp,

	camGetRight,
	camSetRight,

	camGetForward,
	camSetForward,
};
enum objCalls {
	objOpMove,
	objOpRotate,
	objOpSelected,
};

static int meshCall(state rt) {
	switch (rt->fdata) {
		default: return -1;
		case meshOpSetPos: {		// void meshPos(int Index, double x, double y, double z);
			double pos[3];
			int idx = popi32(rt);
			pos[0] = popf64(rt);
			pos[1] = popf64(rt);
			pos[2] = popf64(rt);
			setvtxDV(&msh, idx, pos, NULL, NULL, 0xff00ff);
		} break;
		case meshOpSetNrm: {		// void meshNrm(int Index, double x, double y, double z);
			double nrm[3];
			int idx = popi32(rt);
			nrm[0] = popf64(rt);
			nrm[1] = popf64(rt);
			nrm[2] = popf64(rt);
			setvtxDV(&msh, idx, NULL, nrm, NULL, 0xff00ff);
		} break;
		//~ case meshOpGetPos: {		// vec3f meshPos(int Index)
		//~ case meshOpGetNrm: {		// vec3f meshNrm(int Index)
		case meshOpSetTex: {			// void meshTex(int Index, vec2f Value)
			double pos[2];
			int idx = popi32(rt);
			pos[0] = popf64(rt);
			pos[1] = popf64(rt);
			setvtxDV(&msh, idx, NULL, NULL, pos, 0xff00ff);
		} break;
		//~ case meshOpGetTex: {		// vec2f meshTex(int Index)
		//~ case meshOpSetTri: {		// int meshSetTri(int t, int p1, int p2, int p3)
		case meshOpAddTri: {			// int meshAddTri(int p1, int p2, int p3)
			int p1 = popi32(rt);
			int p2 = popi32(rt);
			int p3 = popi32(rt);
			addtri(&msh, p1, p2, p3);
		} break;
		case meshOpAddQuad: {			// int meshAddTri(int p1, int p2, int p3, p4)
			int p1 = popi32(rt);
			int p2 = popi32(rt);
			int p3 = popi32(rt);
			int p4 = popi32(rt);
			//~ addtri(&msh, p1, p2, p3);
			//~ addtri(&msh, p3, p4, p1);
			addquad(&msh, p1, p2, p3, p4);
		} break;
		//~ case meshOpSetSeg: {		// int meshSetSeg(int t, int p1, int p2)
		//~ case meshOpAddSeg: {		// int meshAddSeg(int p1, int p2)
		//~ case meshOpGetFlags: {		// int meshFlags()
		//~ case meshOpSetFlags: {		// int meshFlags(int Value)

		case meshOpInfo: {				// void meshInfo();
			meshInfo(&msh);
		} break;
		case meshOpInit: {			// int meshInit(int Capacity);
			setret(rt, int32_t, initMesh(&msh, popi32(rt)));
		} break;
		case meshOpTexture: {
			msh.map = getSurf(popi32(rt));
			msh.freeTex = 0;
			debug("msh.map = %p", msh.map);
			//~ textureMesh(&msh, popstr(s));
		} break;
		case meshOpRead: {			// int meshRead(string file);
			char* mesh = popstr(rt);
			char* tex = popstr(rt);
			setret(rt, int32_t, readorevalMesh(&msh, mesh, 0, tex, 0));
		} break;
		case meshOpSave: {			// int meshSave(string file);
			setret(rt, int32_t, saveMesh(&msh, popstr(rt)));
		} break;
		case meshOpCenter: {		// void Center(float size);
			centMesh(&msh, popf32(rt));
		} break;
		case meshOpNormalize: {		// void Normalize(double epsilon);
			normMesh(&msh, popf32(rt));
		} break;
	}

	return 0;
}
static int camCall(state rt) {
	switch (rt->fdata) {
		default: return -1;

		case camOpMove: {
			vector dir = popref(rt);
			float32_t cnt = popf32(rt);
			cammov(cam, dir, cnt);
		} break;
		case camOpRotate: {
			vector dir = popref(rt);
			vector orb = popref(rt);
			float32_t cnt = popf32(rt);
			camrot(cam, dir, orb, cnt);
		} break;

		case camOpLookAt: {		// cam.lookAt(vec4f eye, vec4f at, vec4f up)
			union vector campos, camat, camup;
			vector camPos = poparg(rt, &campos, sizeof(union vector));	// eye
			vector camAt = poparg(rt, &camat, sizeof(union vector));	// target
			vector camUp = poparg(rt, &camup, sizeof(union vector));	// up
			//~ vector camPos = popref(s);
			//~ vector camAt = popref(s);
			//~ vector camUp = popref(s);

			camset(cam, camPos, camAt, camUp);
			break;
		}
		//~ case camOpSet: break;			// cam.set(vec4f pos, vec4f up, vec4f right, vec4f forward)

		case camGetPos: setret(rt, union vector, cam->pos); break;
		case camSetPos: poparg(rt, &cam->pos, sizeof(union vector)); break;

		case camGetUp: setret(rt, union vector, cam->dirU); break;
		case camSetUp: poparg(rt, &cam->dirU, sizeof(union vector)); break;

		case camGetRight: setret(rt, union vector, cam->dirR); break;
		case camSetRight: poparg(rt, &cam->dirR, sizeof(union vector)); break;

		case camGetForward: setret(rt, union vector, cam->dirF); break;
		case camSetForward: poparg(rt, &cam->dirF, sizeof(union vector)); break;
	}

	return 0;
}
static int objCall(state rt) {
	switch (rt->fdata) {
		default: return -1;

		case objOpMove: {
			vector P;		// position
			float32_t mdx = popf32(rt);
			float32_t mdy = popf32(rt);
			if ((P = getobjvec(0))) {
				union matrix tmp, tmp1, tmp2;
				matldT(&tmp1, &cam->dirU, mdy);
				matldT(&tmp2, &cam->dirR, mdx);
				matmul(&tmp, &tmp1, &tmp2);
				matvph(P, &tmp, P);
			}
		} break;
		case objOpRotate: {
			vector N;		// normal vector
			float32_t mdx = popf32(rt);
			float32_t mdy = popf32(rt);
			if ((N = getobjvec(1))) {
				union matrix tmp, tmp1, tmp2;
				matldR(&tmp1, &cam->dirU, mdx);
				matldR(&tmp2, &cam->dirR, mdy);
				matmul(&tmp, &tmp1, &tmp2);
				vecnrm(N, matvp3(N, &tmp, N));
			}
		} break;
		case objOpSelected: {
			setret(rt, int, getobjvec(0) != NULL); break;
		} break;

	}

	return 0;
}
//}#endregion

static void ccline(state rt, char *file, int line) {
	return ccSource(rt->cc, file, line);
}
static int  cctext(state rt, int wl, char *file, int line, char *buff) {
	if (!ccOpen(rt, srcText, buff))
		return -2;

	ccline(rt, file, line);
	return parse(rt->cc, 0, wl);
}
static int  ccfile(state rt, int wl, char *file) {
	if (!ccOpen(rt, srcFile, file))
		return -1;

	//~ ccline(s, file, 1);
	return parse(rt->cc, 0, wl);
}

state rt = NULL;
symn mouseCallBack = NULL;
symn renderMethod = NULL;

enum miscCalls {
	miscSetExitLoop,
	//~ miscSetAnimation,
	miscOpSetCbMouse,
	miscOpSetCbRender,
	//~ miscOpInitScreen,
	miscOpFlipScreen,
	//~ miscOpConsumeEvents,
};
static int miscCall(state rt) {
	symn cb = NULL;
	switch (rt->fdata) {
		default: return -1;

		case miscSetExitLoop: {
			draw = 0;
			/*if (popi32(s)) {
				draw &= ~OnDemand;
			}
			else {
				draw |= OnDemand;
			}*/
		} break;

		/*case miscSetAnimation: {
			if (popi32(s)) {
				draw &= ~OnDemand;
			}
			else {
				draw |= OnDemand;
			}
		} break;*/

		case miscOpFlipScreen: {
			if (popi32(rt)) {
				if (flip)
					flip(&offs, 0);
			}
			else {
				draw |= post_swap;
			}
		} break;

		case miscOpSetCbMouse: {
			cb = mouseCallBack = findref(rt, popref(rt));
		} break;

		case miscOpSetCbRender: {
			cb = renderMethod = findref(rt, popref(rt));
		} break;

		/* unused stuff

		case miscOpInitScreen: {
			resx = popi32(s);
			resy = popi32(s);
			draw = popi32(s);
			g3_init(&offs, resx, resy);
		} break;

		case miscOpConsumeEvents: {
			if (peekMsg)
				peekMsg(0);
		} break;*/

		//~ case miscOpShowWin: break;
	}

	//~ if (cb != NULL) fputfmt(stdout, "setCallBack(%-T)\n", cb);
	(void)cb;

	return 0;
}

/*static int setVar(state rt) {
	// void guiVar(bool &ref, string display);		// check
	// void guiVar(int32 &ref, string display);		// spin
	// void guiVar(float64 &ref, string display);	// slider
	// void guiVar(string &ref, string display);	// textbox
	// void guiVar(void cb(), string display);		// button
	symn cb = NULL;
	switch (rt->fdata) {
		default:
			return -1;
	}
	fputfmt(stdout, "guiVar(%-T)\n", cb);
	return 0;
}

static int setCallBack(state rt) {
	symn cb = NULL;
	switch (rt->fdata) {
		case miscOpSetCbMouse: {
			cb = mouseCallBack = findref(rt, popref(rt));
		} break;
		case miscOpSetCbRender: {
			cb = renderMethod = findref(rt, popref(rt));
		} break;
		default:
			return -1;
	}
	fputfmt(stdout, "setCallBack(%-T)\n", cb);
	return 0;
}

// */

static int ratHND(int btn, int mx, int my) {
	if (mouseCallBack)
		vmCall(rt, mouseCallBack, btn, mx, my);
	else {
		ratHND1(btn, mx, my);
	}
	return 0;
}

struct {
	int (*fun)(state);
	int offs;
	char *def;
} *def,
Gui[] = {
	//~ {miscCall2, miscOpConsumeEvents,	"void consumeEvents();"},
	//~ {miscCall, miscSetAnimation,	"void Animated(bool animate);"},
	{miscCall, miscSetExitLoop,		"void exitLoop();"},
	//~ {miscCall, miscOpInitScreen,	"void initScreen(int resx, int resy, int draw);"},
	{miscCall, miscOpSetCbMouse,	"void setMouseHandler(void handler(int btn, int x, int y));"},
	{miscCall, miscOpSetCbRender,	"void setDrawCallback(void callback());"},
	//~ {miscCall, miscSetAnimation,	"void postRepaint(bool animate);"},
	{miscCall, miscOpFlipScreen,	"void Repaint(bool forceNow);"},
},
Surf[] = {
	{surfCall, surfOpGetWidth,		"int width(gxSurf dst);"},
	{surfCall, surfOpGetHeight,		"int height(gxSurf dst);"},
	{surfCall, surfOpClipRect,		"bool clipRect(gxSurf surf, gxRect &rect);"},

	{surfCall, surfOpGetPixel,		"int getPixel(gxSurf dst, int x, int y);"},
	{surfCall, surfOpGetPixfp,		"int getPixel(gxSurf dst, double x, double y);"},
	{surfCall, surfOpSetPixel,		"void setPixel(gxSurf dst, int x, int y, int col);"},

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
	{meshCall, meshOpSetSeg,		"int SetSeg(int t, int p1, int p2);"},
	{meshCall, meshOpAddSeg,		"int AddSeg(int p1, int p2);"},
	{meshCall, meshOpGetFlags,		"int Flags;"},
	{meshCall, meshOpSetFlags,		"int Flags(int Value);"},
	//~ {meshCall, meshOpGetTex,		"gxSurf Texture;"},
	{meshCall, meshOpTexture,		"int Texture(gxSurf file);"},
	{meshCall, meshOpInit,			"int Init(int Capacity);"},
	{meshCall, meshOpRead,			"int Read(string file, string texture);"},
	{meshCall, meshOpSave,			"int Save(string file);"},
	{meshCall, meshOpCenter,		"void Center(float32 Resize);"},
	{meshCall, meshOpNormalize,		"void Normalize(float32 Epsilon);"},
	{meshCall, meshOpInfo,			"void Info();"},
},
Cam[] = {
	{camCall, camGetPos,			"vec4f Pos;"},
	{camCall, camGetUp,				"vec4f Up;"},
	{camCall, camGetRight,			"vec4f Right;"},
	{camCall, camGetForward,		"vec4f Forward;"},

	{camCall, camSetPos,			"void Pos(vec4f v);"},
	{camCall, camSetUp,				"void Up(vec4f v);"},
	{camCall, camSetRight,			"void Right(vec4f v);"},
	{camCall, camSetForward,		"void Forward(vec4f v);"},

	{camCall, camOpMove,			"void Move(vec4f &direction, float32 cnt);"},
	{camCall, camOpRotate,			"void Rotate(vec4f &direction, vec4f &orbit, float32 cnt);"},
	//~ cam.lookAt(vec4f eye, vec4f at, vec4f up)
	{camCall, camOpLookAt,			"void LookAt(vec4f eye, vec4f at, vec4f up);"},
},
Obj[] = {
	//~ {objCall, objGetPos,			"vec4f Pos();"},
	//~ {objCall, objSetPos,			"void Pos(vec4f v);"},
	//~ {objCall, objGetNrm,			"vec4f Nrm();"},
	//~ {objCall, objSetNrm,			"void Nrm(vec4f v);"},
	{objCall, objOpMove,			"void Move(float32 dx, float32 dy);"},
	{objCall, objOpRotate,			"void Rotate(float32 dx, float32 dy);"},
	{objCall, objOpSelected,		"bool object;"},
};

char* strnesc(char *dst, int max, char* src) {
	int i = 0;

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

static int ccCompile(char *src, int argc, char* argv[]) {
	symn cls = NULL;
	const int strwl = 9;
	const int srcwl = 9;
	int i, err = 0;

	if (rt == NULL)
		return -29;

	/*if (!ccInit(rt, cerg_all)) {
		debug("Internal error\n");
		return -1;
	}*/

	if (ccLog && logfile(rt, ccLog) != 0) {
		debug("can not open file `%s`\n", ccLog);
		return -2;
	}// */

	//{ inlined here for libcall functions
	err = err || cctext(rt, strwl, __FILE__, __LINE__ + 1,
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
		"struct:1 gxClut {			// Color Look Up Table (Palette) structure\n"
			"int16	count;\n"
			"uint8	flags;\n"
			"uint8	trans;\n"
			"int32	data[256];\n"
		"}\n"
	);//} */

	if ((cls = ccBegin(rt, "Gradient"))) {
		err = err || !ccDefineInt(rt, "Linear", gradient_lin);
		err = err || !ccDefineInt(rt, "Radial", gradient_rad);
		err = err || !ccDefineInt(rt, "Square", gradient_sqr);
		err = err || !ccDefineInt(rt, "Conical", gradient_con);
		err = err || !ccDefineInt(rt, "Spiral", gradient_spr);
		err = err || !ccDefineInt(rt, "Repeat", gradient_rep);
		ccEnd(rt, cls);
	}

	// install standard library calls
	err = err || install_stdc(rt, ccStd, strwl);
	err = err || !installtyp(rt, "gxSurf", sizeof(gxSurfHnd));

	for (i = 0; i < sizeof(Surf) / sizeof(*Surf); i += 1) {
		err = err || !libcall(rt, Surf[i].fun, Surf[i].offs, Surf[i].def);
	}

	if ((cls = ccBegin(rt, "Gui"))) {
		for (i = 0; i < sizeof(Gui) / sizeof(*Gui); i += 1) {
			err = err || !libcall(rt, Gui[i].fun, Gui[i].offs, Gui[i].def);
		}
		err = err || cctext(rt, strwl, __FILE__, __LINE__ + 1,
			"define Repaint() = Repaint(false);\n"
		);
		ccEnd(rt, cls);
	}
	if ((cls = ccBegin(rt, "mesh"))) {
		for (i = 0; i < sizeof(Mesh) / sizeof(*Mesh); i += 1) {
			err = err || !libcall(rt, Mesh[i].fun, Mesh[i].offs, Mesh[i].def);
		}
		ccEnd(rt, cls);
	}
	if ((cls = ccBegin(rt, "camera"))) {
		for (i = 0; i < sizeof(Cam) / sizeof(*Cam); i += 1) {
			err = err || !libcall(rt, Cam[i].fun, Cam[i].offs, Cam[i].def);
		}
		ccEnd(rt, cls);
	}
	if ((cls = ccBegin(rt, "selection"))) {
		for (i = 0; i < sizeof(Obj) / sizeof(*Obj); i += 1) {
			err = err || !libcall(rt, Obj[i].fun, Obj[i].offs, Obj[i].def);
		}
		ccEnd(rt, cls);
	}

	err = err || cctext(rt, strwl, __FILE__, __LINE__, "gxSurf offScreen = emit(gxSurf, i32(-1));\n");

	// it is not an error if library name is not set(NULL)
	if (ccGfx && ccOpen(rt, srcFile, ccGfx)) {
		err = err || parse(rt->cc, 0, strwl);
	}

	/*if ((cls = ccBegin(rt, "rt"))) {
		install(args);
		install(env);
		install(exceptions Stack)
		ccEnd(rt, cls);
	}*/

	if (src != NULL) {
		char tmp[65535], tmpf[4096];

		if ((cls = ccBegin(rt, "properties"))) {

			symn subcls = NULL;
			if ((subcls = ccBegin(rt, "screen"))) {
				err = err || !ccDefineInt(rt, "width", resx);
				err = err || !ccDefineInt(rt, "height", resy);
				ccEnd(rt, subcls);
			}// */

			err = err || !ccDefineStr(rt, "texture", strnesc(tmpf, sizeof(tmpf), tex));
			//~ snprintf(tmp, sizeof(tmp), "static string texture = \"%s\";", strnesc(tmpf, sizeof(tmpf), tex));
			//~ err = err || cctext(rt, strwl, __FILE__, __LINE__ , tmp);

			ccEnd(rt, cls);
		}

		//~ snprintf(tmp, sizeof(tmp), "string texture = \"%s\";", strnesc(tmpf, sizeof(tmpf), tex));
		//~ err = err || cctext(rt, strwl, __FILE__, __LINE__ , tmp);

		snprintf(tmp, sizeof(tmp), "string args[%d];", argc);
		err = err || cctext(rt, strwl, __FILE__, __LINE__ , tmp);

		for (i = 0; i < argc; i += 1) {
			snprintf(tmp, sizeof(tmp), "args[%d] = \"%s\";", i, strnesc(tmpf, sizeof(tmpf), argv[i]));
			err = err || cctext(rt, strwl, __FILE__, __LINE__ , tmp);
		}
		err = err || ccfile(rt, srcwl, src);
	}
	else {
		logFILE(rt, stdout);
		cctext(rt, strwl, __FILE__, __LINE__ , "");
		gencode(rt, 0);
		dump(rt, dump_sym | 0x12, NULL, "#api: replace('\\([^)]\\):.*$', '\\1')\n");
		return 0;
	}

	if (err || gencode(rt, 2) != 0) {
		if (ccLog)
			debug("error compiling(%d), see `%s`", err, ccLog);
		if (ccLog) logfile(rt, NULL);
		err = -3;
	}

	if (err == 0) {
		//~ int tmp = 0;
		symn cls;

		draw = 0;
		if (!symvalint(findsym(rt->cc, NULL, "resx"), &resx)) {
			//~ debug("using default value for resx: %d", resx);
		}
		if (!symvalint(findsym(rt->cc, NULL, "resy"), &resy)) {
			//~ debug("using default value for resy: %d", resy);
		}
		if (!symvalint(findsym(rt->cc, NULL,  "renderType"), &draw)) {
			//~ debug("using default value for renderType: 0x%08x", draw);
		}
		//~ if (symvalint(findsym(rt->cc, NULL, "DebugExit"), &tmp) && tmp) {
			//~ debug("using DebugExit: %d", tmp);
		//~ }// */
		//~ libcSwapExit(rt, tmp ? libCallExitDebug : libCallExitQuiet);

		//~ if (!findfun(rt->cc, "onMouseDrag", &draw)) {}
		if ((cls = findsym(rt->cc, NULL, "Window"))) {
			if (!symvalint(findsym(rt->cc, cls, "resx"), &resx)) {
				//~ debug("using default value for resx: %d", resx);
			}
			if (!symvalint(findsym(rt->cc, cls, "resy"), &resy)) {
				//~ debug("using default value for resy: %d", resy);
			}
			if (!symvalint(findsym(rt->cc, cls, "draw"), &draw)) {
				//~ debug("using default value for renderType: 0x%08x", draw);
			}
		}

		if (ccDmp && logfile(rt, ccDmp) == 0) {
			dump(rt, dump_sym | 0xf2, NULL, "\ntags:\n");
			//~ dump(rt, dump_ast | 0xff, "\ncode:\n");
			dump(rt, dump_asm | 0xf9, NULL, "\ndasm:\n");
		}

	}// */

	logFILE(rt, stderr);

	return err;// || vmExec(rt, NULL);
}

#endif

int main(int argc, char* argv[]) {
	union matrix proj[1], view[1];
	//~ char *script = NULL;
	struct mesh mshLightPoint;		// mesh for Lights
	//~ struct mesh mshLightSpot;		// mesh for spot lights
	//~ struct mesh mshLightDir;		// mesh for directional lights
	#ifdef cubemap
	struct gx_Surf envt[6];
	#endif

	static char mem[16 << 20];		// 16MB memory for compiler

	//~ clock_t clk;
	struct {
		time_t	sec;
		int	cnt;
		int	fps; // ,col
	} fps = {0, 0, 0};

	char str[1024];
	Light l;
	int e;

	setbuf(stdout, NULL);
	initMesh(&msh, 1024);
	initMesh(&mshLightPoint, 100);
	//~ evalSphere(&mshLightPoint, 10, 10);

	//~ evalSphere(&mshLightSpot, 10, 10);		// TODO: this shoud be a cone
	//~ evalSphere(&mshLightDir, 10, 10);		// TODO: this shoud be a cilinder

	msh.lit = userLights;
	for (l = userLights; l; l = l->next) {
		vecnrm(&l->dir, &l->dir);
	}// */

	#ifdef cubemap
		printf ("load image : %d\n", gx_loadBMP(envt + 0, cubemap"positive_z.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 1, cubemap"negative_z.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 2, cubemap"positive_x.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 3, cubemap"negative_x.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 4, cubemap"positive_y.bmp", 32));
		printf ("load image : %d\n", gx_loadBMP(envt + 5, cubemap"negative_y.bmp", 32));
	#endif

	readIni(ini);

	if (ltP != NULL) // point Light
		readorevalMesh(&mshLightPoint, ltP, ltPLn, NULL, 10);

	//~ if (ltS != NULL) // spot Light
		//~ readorevalMesh(&mshLightSpot, ltS, ltSLn, NULL, 10);

	//~ if (ltD != NULL) // directional Light
		//~ readorevalMesh(&mshLightDir, ltD, ltDLn, NULL, 10);

	if (argc > 1 && strcmp("-s", argv[1]) == 0) {	// scripts
		if (argc > 2) {
			char *script = argv[2];
			int64_t ticks = timenow();
			rt = rtInit(mem, sizeof(mem));
			e = ccCompile(script, argc - 2, argv + 2);
			debug("ccCompile(): %d\tTime: %f", e, ticksinsecs(ticks));
			if (e != 0) {
				debug("Error: %d", e);
				gx_doneSurf(&offs);
				freeMesh(&mshLightPoint);
				freeMesh(&msh);
				return -21;
			}
		}
		else {	// need help ?
			rt = rtInit(mem, sizeof(mem));
			ccCompile(NULL, 0, NULL);
			gx_doneSurf(&offs);
			freeMesh(&mshLightPoint);
			freeMesh(&msh);
			return 0;
		}
		/*if (argc == 2) {	// script help
			rt = rtInit(mem, sizeof(mem));
			ccCompile(NULL, 0, NULL);
			gx_doneSurf(&offs);
			freeMesh(&mshLightPoint);
			freeMesh(&msh);
			return 0;
		}
		if (argc == 3) {	// script it
			char *script = argv[2];
			int64_t ticks = timenow();
			rt = rtInit(mem, sizeof(mem));
			e = ccCompile(script, 0, NULL);
			debug("ccCompile(): %d\tTime: %g", e, ticksinsecs(ticks));
			if (e != 0) {
				debug("Error: %d", e);
				gx_doneSurf(&offs);
				freeMesh(&mshLightPoint);
				freeMesh(&msh);
				return -21;
			}
		}*/
	}
	else if (argc >= 2) {		// drop in: one object
		char *arg = argv[1];
		char *ext = fext(arg);
		char *ssc = findext(ext);
		debug("opening file(%s with %s): %s", ext, ssc, arg);

		if (ssc != NULL) {
			int64_t ticks = timenow();
			rt = rtInit(mem, sizeof(mem));
			argv[0] = ssc;
			e = ccCompile(ssc, argc, argv);
			debug("ccCompile(): %d\tTime: %g", e, ticksinsecs(ticks));
			if (e != 0) {
				debug("Error: %d", e);
				gx_doneSurf(&offs);
				freeMesh(&mshLightPoint);
				freeMesh(&msh);
				return -21;
			}
		}
		else {
			obj = arg;
		}
	}
	/*else if (argc == 3) {		// drop in: object + texture ?
		obj = argv[1];
		tex = argv[2];
	}*/

	if (rt == NULL) {
		readorevalMesh(&msh, obj, objLn, tex, 64);
		centMesh(&msh, O);
		meshInfo(&msh);
	}

	vecldf(&up, 0, 1, 0, 1);
	vecldf(&tgt, 0, 0, 0, 1);
	matidn(view, 1);
	kbdHND(13);

	memset(&font, 0, sizeof(font));
	if (fnt && (e = gx_loadFNT(&font, fnt))) {
		debug("Cannot open font '%s': %d\n", fnt, e);
	}

	if ((e = g3_init(&offs, resx, resy))) {
		debug("Error: %d\n", e);
		gx_doneSurf(&font);
		gx_doneSurf(&offs);
		freeMesh(&msh);
		return -2;
	}

	if (initWin(draw ? &offs : NULL, &flip, &peekMsg, ratHND, kbdHND)) {
		printf("Cannot init surface\n");
		gx_doneSurf(&font);
		gx_doneSurf(&offs);
		freeMesh(&msh);
		return -1;
	}

	if (rt) {
		int64_t ticks = timenow();
		e = vmExec(rt, NULL);
		debug("vmExecute(): %d\tTime: %f", e, ticksinsecs(ticks));
		rtAlloc(rt, NULL, 0);
	}

	// set the color of the mesh to normals
	//~ projv_mat(&cam->proj, 10 * .25, (double)resx / resy, .25, 100);
	projv_mat(&cam->proj, F_fovy, (double)resx / resy, F_near, F_far);

	g3_drawmesh(NULL, &msh, NULL, cam, draw, 1);	// precalc normal colors

	if (flip)
		flip(&offs, 0);

	// the big main loop
	if (draw) for (fps.cnt = fps.sec = 0; ; fps.cnt++) {
		int msg, now, tris = 0, ln = 0;
		#define nextln ((ln += box.h) - box.h)

		now = time(NULL);
		if (fps.sec != now) {				// fps
			fps.fps = fps.cnt;
			fps.sec = now;
			fps.cnt = 0;
		}

		msg = peekMsg((draw & post_swap) == 0);
		draw &= ~post_swap;

		if (msg == -1)
			break;

		switch (msg) {
			case doScript: {		// this should be an "execute callback" from script
				int64_t ticks = timenow();
				e = vmExec(rt, NULL);
				debug("vmExecute(): %d\tTime: %g", e, ticksinsecs(ticks));
			} break;

			case doSaveImage:
				gx_saveBMP("mesh.bmp", &offs, 0);
				break;

			case doSaveMesh:
				saveMesh(&msh, "mesh.obj");
				break;
		}// */

		if (draw & zero_cbuf) {
			int *cBuff = offs.basePtr;
			for (e = 0; e < offs.width * offs.height; e += 1)
				cBuff[e] = 0x00000000;
		}
		if (draw & zero_zbuf) {
			int *zBuff = offs.tempPtr;
			for (e = 0; e < offs.width * offs.height; e += 1)
				zBuff[e] = 0x3fffffff;
		}

		#ifdef cubemap
		matmul(proj, &cam->proj, cammat(view, cam));
		g3_drawenvc(&offs, envt, &cam->dirF, proj, 1000);
		#endif

		// before render method
		if (renderMethod != NULL) {
			vmCall(rt, renderMethod);
		}

		if (draw & (draw_mode | disp_bbox | temp_lght | temp_zbuf | disp_info | disp_oxyz)) {
			if (draw & draw_mode) {
				msh.hlplt = vtx;
				tris += g3_drawmesh(&offs, &msh, NULL, cam, draw, draw & disp_norm ? NormalSize : 0);
			}
			if (draw & disp_bbox) {
				g3_drawbbox(&offs, &msh, NULL, cam);
			}
			if (draw & temp_lght) {
				cammat(view, cam);
				matmul(proj, &cam->proj, view);
				for (l = userLights; l; l = l->next) {
					if (l->attr & L_on) {
						matidn(view, 1);
						view->x.x = lightsize;
						view->y.y = lightsize;
						view->z.z = lightsize;
						view->x.w = l->pos.x;
						view->y.w = l->pos.y;
						view->z.w = l->pos.z;
						mshLightPoint.mtl.emis = l->diff;
						g3_drawmesh(&offs, &mshLightPoint, view, cam, draw_fill+draw_lit+cull_back, 0);//
					}
				}
			}
			if (draw & temp_zbuf) {
				int *cBuff = offs.basePtr;
				int *zBuff = offs.tempPtr;
				for(e = 0; e < offs.width * offs.height; e += 1) {
					long z = zBuff[e];
					z = (z >> 15) & 0x1ff;
					if (z < 256) z = ~z & 0xff;
					cBuff[e] = ~(z << 16 | z << 8 | z);
				}
			}
			if (draw & disp_info) {
				struct gx_Rect box;
				sprintf(str, "Object size: %g, tvx:%d, tri:%d, seg:%d%s%s", O, msh.vtxcnt, msh.tricnt, msh.segcnt, msh.hasTex ? ", tex" : "", msh.hasNrm ? ", nrm" : "");
				gx_clipText(&box, &font, str);
				gx_drawText(&offs, offs.width - box.w - 10, nextln, &font, str, 0xffffffff);

				sprintf(str, "camera(%g, %g, %g)", cam->pos.x, cam->pos.y, cam->pos.z);
				gx_clipText(&box, &font, str);
				gx_drawText(&offs, offs.width - box.w - 10, nextln, &font, str, 0xffffffff);

				if (getobjvec(0)) {
					vector P = getobjvec(0);
					vector N = getobjvec(1);
					//~ sprintf(str, "light[%d].pos(%g, %g, %g); cos(%g), exp(%g)", rm-Lights, rm->pos.x, rm->pos.y, rm->pos.z, rm->sCos, rm->sExp);
					sprintf(str, "object: P(%.3f, %.3f, %.3f), N(%.3f, %.3f, %.3f)", P->x, P->y, P->z, N->x, N->y, N->z);
					gx_clipText(&box, &font, str); gx_drawText(&offs, offs.width - box.w - 10, nextln, &font, str, 0xffffffff);
					//~ sprintf(str, "camera1(%g, %g, %g)", cam1.pos.x, cam1.pos.y, cam1.pos.z);
					//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, nextln, &font, str, 0xffffffff);
				}
				for (l = userLights; l; l = l->next) {
					argb lcol = vecrgb(&l->ambi);
					sprintf(str, "light %d : %s", e, l->attr & L_on ? "On" : "Off");
					gx_clipText(&box, &font, str);
					gx_drawText(&offs, offs.width - box.w - 10, nextln, &font, str, lcol.val);
				}
			}
			if (draw & disp_oxyz) {
				struct gx_Surf scr;
				struct gx_Rect roi;
				roi.w = offs.width / 8;
				roi.h = offs.height / 8;
				roi.x = offs.width - roi.w;
				roi.y = offs.height - roi.h;
				if (gx_asubSurf(&scr, &offs, &roi)) {
					g3_drawOXYZ(&scr, cam, 1);
				}
			}
		}

		if (fps.cnt == 0) {
			//~ int col = fps.fps < 30 ? 0xff0000 : fps.fps < 50 ? 0xffff00 : 0x00ff00;
			//~ sprintf(str, "tvx:%d, tri:%d/%d, fps: %d", msh.vtxcnt, tris, msh.tricnt, fps.fps);
			sprintf(str, "FPS: %d, Vertices: %d, Polygons: %d/%d", fps.fps, msh.vtxcnt, tris, msh.tricnt);
			setCaption(str);
		}
		if (draw & swap_buff) {
			flip(&offs, 0);
		}

		#undef nextln
	}// */

	gx_doneSurf(&font);
	gx_doneSurf(&offs);
	freeMesh(&mshLightPoint);
	//~ freeMesh(&mshLightSpot);
	//~ freeMesh(&mshLightDir);
	freeMesh(&msh);

	if (rt != NULL) {
		rtAlloc(rt, NULL, 0);
	}

	doneWin();
	return 0;
}
