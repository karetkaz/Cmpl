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

char *ccStd = NULL;//"src/stdlib.gxc";	// stdlib script file (from gx.ini)
char *ccGfx = NULL;//"src/gfxlib.gxc";	// gfxlib script file (from gx.ini)
char *ccLog = NULL;//"out/debug.out";
char *ccDmp = NULL;//"out/dump.out";
int ccDbg = 0;


char *obj = NULL;		// object filename
char *tex = NULL;		// texture filename
int objLn = 0;			// object fileline
//~ char *sky = 0;	// skyBox

double F_fovy = 30.;
double F_near = 1.;
double F_far = 100.;

int draw = draw_fill | cull_back | disp_info | zero_cbuf | zero_zbuf | swap_buff;
int resx = 800;
int resy = 600;

struct camera cam[1];
static struct vector eye, tgt, up;

struct mesh msh;
struct gx_Surf offs;		// offscreen
struct gx_Surf font;		// font
flip_scr flip = NULL;		// swap buffer method

// selected vertex/light
static int vtx = 0;
static Light rm = NULL;
static struct Light userLights[32];

// get the selected pos/normal reference
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

//#{ extension manager, or somethig like that
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

char* fext(const char* name) {
	char *ext = "";
	char *ptr = (char*)name;
	if (ptr) while (*ptr) {
		if (*ptr == '.')
			ext = ptr + 1;
		ptr += 1;
	}
	return ext;
}

char* readI32(char *ptr, int *outVal) {
	int sgn = 1, val = 0;
	if (!strchr("+-0123456789", *ptr))
		return ptr;
	if (*ptr == '-' || *ptr == '+') {
		sgn = *ptr == '-' ? -1 : +1;
		ptr += 1;
	}
	while (*ptr >= '0' && *ptr <= '9') {
		val = val * 10 + (*ptr - '0');
		ptr += 1;
	}
	if (outVal) *outVal = sgn * val;
	return ptr;
}
char* readFlt(char *str, double *outVal) {
	char *ptr = str;
	double val = 0;
	int sgn = 1, exp = 0;

	//~ ('+'|'-')?
	if (*ptr == '-' || *ptr == '+') {
		sgn = *ptr == '-' ? -1 : 1;
		ptr += 1;
	}

	//~ (0 | ([1-9][0-9]+))?
	if (*ptr == '0') {
		ptr++;
	}
	else while (*ptr >= '0' && *ptr <= '9') {
		val = val * 10 + (*ptr - '0');
		ptr++;
	}

	//~ ('.'[0-9]*)?
	if (*ptr == '.') {
		ptr++;
		while (*ptr >= '0' && *ptr <= '9') {
			val = val * 10 + (*ptr - '0');
			exp -= 1;
			ptr++;
		}
	}

	//~ ([eE]([+-]?)[0-9]+)?
	if (*ptr == 'e' || *ptr == 'E') {
		int tmp = 0;
		ptr = readI32(ptr + 1, &tmp);
		exp += tmp;
	}

	if (outVal) {
		*outVal = sgn * val * pow(10, exp);
	}

	return ptr;
}
char* readF32(char *str, float *outVal) {
	double f64;
	str = readFlt(str, &f64);
	if (outVal) *outVal = f64;
	return str;
}
char* readVec(char *str, vector dst, scalar defw) {
	char *sep = ",";

	char *ptr;

	dst->w = defw;

	str = readF32(str, &dst->x);

	if (!*str) {
		dst->z = dst->y = dst->x;
		return str;
	}

	if (!(ptr = readKVP(str, sep, NULL, NULL)))
		return str;

	str = readF32(ptr, &dst->y);

	if (!(ptr = readKVP(str, sep, NULL, NULL)))
		return str;

	str = readF32(ptr, &dst->z);

	if (!*str) return str;

	if (!(ptr = readKVP(str, sep, NULL, NULL)))
		return str;

	str = readF32(ptr, &dst->w);

	return str;
}

char* readKVP(char *ptr, char *key, char *sep, char *wsp) {	// key = value pair
	if (wsp == NULL) wsp = " \t";

	// match key and skip white spaces
	if (*key) {
		while (*key && *key == *ptr) ++key, ++ptr;
		if (!sep && !strchr(wsp, *ptr)) return 0;
		while (*ptr && strchr(wsp, *ptr)) ++ptr;
	}

	// match sep and skip white spaces
	if (sep && *sep) {
		while (*sep && *sep == *ptr) ++sep, ++ptr;
		while (*ptr && strchr(wsp, *ptr)) ++ptr;
	}

	// if key and separator were mached
	return (*key || (sep && *sep)) ? NULL : ptr;
}

//#}

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

		if (!*ptr || *ptr == '#' || *ptr == ';') {				// Comment or empty line
			if (section == objfun) {		// add a newline to script code.
				*ptr++ = '\n';
				*ptr = 0;
			}
			continue;
		}

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
			//~ union vector emis;		// Emissive
			debug("invalid light param: %s(%d): %s", file, line, ptr);
		}
		if (section == objlit) {

			if (strcmp(ptr, "off") == 0) {
				lit->attr = 0;
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
				if (*readVec(arg, &lit->spec, 0))
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
				if (!(arg = readKVP(arg, ",", "", is)))
					continue;
				if (!*(arg = readF32(arg, &lit->sExp)))
					continue;
				debug("invalid format: %s(%d): %s", file, line, arg);
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
				char *end = readI32(arg, &resx);
				if (*end)
					debug("invalid number: %s:%d", arg, *end);
				continue;
			}
			if ((arg = readKVP(ptr, "screen.height", "=", is))) {
				if (*readI32(arg, &resy))
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
void meshInfo(mesh msh) {
	struct vector min, max;
	bboxMesh(msh, &min, &max);
	printf("box min : %f, %f, %f\n", min.x, min.y, min.z);
	printf("box max : %f, %f, %f\n", max.x, max.y, max.z);
	printf("vtx cnt : %d / %d\n", msh->vtxcnt, msh->maxvtx);
	printf("tri cnt : %d / %d\n", msh->tricnt, msh->maxtri);
	//~ printf("vtx cnt : %d / %d\n", msh->vtxcnt, msh->maxvtx);
	//~ printf("tri cnt : %d / %d\n", msh->tricnt, msh->maxtri);
}

#include "pvmc.h"
extern state rt;
extern symn renderMethod;
extern symn mouseCallBack;
extern symn keyboardCallBack;
extern int ccCompile(char *src, int argc, char* argv[]);

enum Events {
	doReload = 10,
	doSaveMesh = 11,
	doSaveImage = 12,
};

static int kbdHND(int key, int state) {
	const scalar sca = 8;
	if (keyboardCallBack) {
		int result = 0;
		//~ struct {int key, state;} args = {key, state};
		vmCall(rt, keyboardCallBack, NULL, &key);
		return result;
	}
	else switch (key) {
		case 27:		// quit
			draw = 0;
			break;

		case 13:
			vecldf(&eye, 0, 0, O * sca, 1);
			camset(cam, &eye, &tgt, &up);
			break;

		case 'O':
			vecldf(&cam->pos, 0, 0, 0, 1);
			break;


		case '*':
			centMesh(&msh, O = 2.);
			break;
		case '+':
			centMesh(&msh, O *= 2.);
			break;
		case '-':
			centMesh(&msh, O /= 2.);
			break;

		case 'w': cammov(cam, &cam->dirF, +MOV_d); break;
		case 'W': cammov(cam, &cam->dirF, +MOV_D); break;
		case 's': cammov(cam, &cam->dirF, -MOV_d); break;
		case 'S': cammov(cam, &cam->dirF, -MOV_D); break;
		case 'd': cammov(cam, &cam->dirR, +MOV_d); break;
		case 'D': cammov(cam, &cam->dirR, +MOV_D); break;
		case 'a': cammov(cam, &cam->dirR, -MOV_d); break;
		case 'A': cammov(cam, &cam->dirR, -MOV_D); break;
		case ' ': cammov(cam, &cam->dirU, +MOV_D); break;
		case 'c': cammov(cam, &cam->dirU, -MOV_D); break;

		case '\t': draw = (draw & ~draw_mode) | ((draw + LOBIT(draw_mode)) & draw_mode); break;
		case '/': draw = (draw & ~cull_mode) | ((draw + LOBIT(cull_mode)) & cull_mode); break;
		case '?': draw = (draw & ~cull_mode) | ((draw - LOBIT(cull_mode)) & cull_mode); break;

		case '`': draw ^= disp_info; break;
		case 'L': draw ^= temp_lght; break;
		case 'b': draw ^= disp_bbox; break;
		case 'Z': draw ^= temp_zbuf; break;

		case 'Q': draw ^= swap_buff; break;

		case 't': draw ^= draw_tex; break;
		case 'T': msh.hasTex = !msh.hasTex; break;

		case 'n': draw ^= disp_norm; break;
		case 'N': msh.hasNrm = !msh.hasNrm; break;

		case 'l': draw ^= draw_lit; break;

		case '~': vtx = 0; rm = NULL; break;
		case '.': vtx += 1; break;
		case '>': vtx -= 1; break;

		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9': {
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

		/*case '\b': {
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

		//~ case 18:		// Ctrl + R
			//~ readIni();
			//~ break;

		// */

		case 16:		// Ctrl + P
			return doSaveImage;

		case 19:		// Ctrl + S
			return doSaveMesh;

		case 18:		// Ctrl + R
			return doReload;

		default:
			debug("kbdHND(key(%d), lkey(%d))", key, state);
			break;
	}
	return 0;
}

static int ratHND(int btn, int mx, int my) {
	if (mouseCallBack) {
		int result = 0;
		//~ struct {int btn, x, y;} args = {btn, mx, my};
		vmCall(rt, mouseCallBack, NULL, &btn);
		return result;
	}
	else {// native mouse handler.
		static int ox, oy;
		int mdx = mx - ox;
		int mdy = my - oy;
		ox = mx; oy = my;
		if (btn == 1) {
			struct matrix tmp;
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
				struct matrix tmp, tmp1, tmp2;
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
				struct matrix tmp, tmp1, tmp2;
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
}

static int dbgCon(state rt, int pu, void* ip, long* bp, int ss) {
	int IP = ((char*)ip) - ((char*)rt->_mem);
	fputfmt(stdout, ">exec:[sp%02d:%08x]@%9.*A\n", ss, bp + ss, IP, ip);
	//~ fputfmt(stdout, ">exec:[pu:%d, sp%02d:%08x]@", pu, ss, bp[0], IP, ip);
	//~ fputopc(stdout, ip, 0x09, IP, rt);
	//~ fputfmt(stdout, "\n");

	return 0;
}

int main(int argc, char* argv[]) {
	static char mem[16 << 20];		// 16MB memory for vm & compiler
	char *script = NULL;			// compile script

	struct matrix proj[1], view[1];
	struct mesh mshLightPoint;		// mesh for Lights
	//~ struct mesh mshLightSpot;		// mesh for spot lights
	//~ struct mesh mshLightDir;		// mesh for directional lights

	#ifdef cubemap
	struct gx_Surf envt[6];
	#endif

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

	if (argc > 1) {
		if (strcmp("-s", argv[1]) == 0) {	// scripts
			if (argc == 2) {
				// need help ?
				rt = rtInit(mem, sizeof(mem));
				ccCompile(NULL, 0, NULL);
				gx_doneSurf(&offs);
				freeMesh(&mshLightPoint);
				freeMesh(&msh);
				return 0;
			}
			else {
				script = argv[2];
				argv += 2;
				argc -= 2;
			}
		}
		else {								// open first argument
			char* arg = argv[1];
			char* ext = fext(arg);
			script = findext(ext);
			debug("opening file(%s with %s): %s", ext, script, arg);

			if (script == NULL) {
				obj = arg;
				if (argc > 2) {
					tex = argv[2];
				}

				// HACK: run scripts.
				if (stricmp(ext, "gxc") == 0) {
					script = arg;
					obj = NULL;
					tex = NULL;
				}
			}
			else {
				argv[0] = script;
			}
		}
	}

	vecldf(&up, 0, 1, 0, 1);
	vecldf(&tgt, 0, 0, 0, 1);
	matidn(view, 1);
	kbdHND(13, 0);

	memset(&font, 0, sizeof(font));
	if (fnt && (e = gx_loadFNT(&font, fnt))) {
		debug("Cannot open font '%s': %d\n", fnt, e);
	}

	if (script != NULL) {
		int64_t ticks;
		rt = rtInit(mem, sizeof(mem));

		ticks = timenow();
		e = ccCompile(script, argc, argv);
		debug("ccCompile(): %d\tTime: %f", e, ticksinsecs(ticks));

		if (e != 0) {
			freeMesh(&mshLightPoint);
			freeMesh(&msh);
			return -21;
		}
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

	if (rt != NULL) {
		int64_t ticks = timenow();
		e = vmExec(rt, ccDbg ? dbgCon : NULL, sizeof(mem)/4);
		debug("vmExecute(): %d\tTime: %f", e, ticksinsecs(ticks));
		if (e != 0) {
			gx_doneSurf(&offs);
			gx_doneSurf(&font);
			freeMesh(&mshLightPoint);
			freeMesh(&msh);
			return -21;
		}
	}
	else {
		readorevalMesh(&msh, obj, objLn, tex, 64);
		centMesh(&msh, O);
		meshInfo(&msh);
	}

	//~ projv_mat(&cam->proj, 10 * .25, (double)resx / resy, .25, 100);
	projv_mat(&cam->proj, F_fovy, (double)resx / resy, F_near, F_far);

	// set the color of the mesh to normals
	g3_drawmesh(NULL, &msh, NULL, cam, draw, 1);

	if (flip)
		flip(&offs, 0);

	// the big main loop
	if (draw) for (fps.cnt = fps.sec = 0; ; fps.cnt++) {
		int now, tris = 0;

		now = time(NULL);
		if (fps.sec != now) {				// fps
			fps.fps = fps.cnt;
			fps.sec = now;
			fps.cnt = 0;
		}

		switch (peekMsg((draw & post_swap) == 0)) {
			case doReload: {
				readIni(ini);
				if (rt == NULL) {
					debug("reloading mesh: %s", obj);
					readorevalMesh(&msh, obj, objLn, tex, 64);
					centMesh(&msh, O);
					meshInfo(&msh);
				}
				else {
					int64_t ticks = timenow();
					e = vmExec(rt, NULL, sizeof(mem)/4);
					debug("vmExecute(): %d\tTime: %g", e, ticksinsecs(ticks));
				}
			} break;

			case doSaveImage:
				gx_saveBMP("screen.bmp", &offs, 0);
				break;

			case doSaveMesh:
				saveMesh(&msh, "mesh.obj");
				break;
		}

		if (!draw) {
			break;
		}

		draw &= ~post_swap;

		if (draw & zero_cbuf) {
			int *cBuff = (void*)offs.basePtr;
			for (e = 0; e < offs.width * offs.height; e += 1)
				cBuff[e] = 0x00000000;
		}
		if (draw & zero_zbuf) {
			int *zBuff = (void*)offs.tempPtr;
			for (e = 0; e < offs.width * offs.height; e += 1)
				zBuff[e] = 0x3fffffff;
		}

		// before render method
		if (renderMethod != NULL) {
			vmCall(rt, renderMethod, NULL, NULL);
		}

		if (draw & (draw_mode | temp_lght | disp_bbox | temp_zbuf | disp_info)) {

			#ifdef cubemap
			matmul(proj, &cam->proj, cammat(view, cam));
			g3_drawenvc(&offs, envt, &cam->dirF, proj, 1000);
			#endif

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
				int *cBuff = (void*)offs.basePtr;
				int *zBuff = (void*)offs.tempPtr;
				for(e = 0; e < offs.width * offs.height; e += 1) {
					long z = zBuff[e];
					z = (z >> 15) & 0x1ff;
					if (z < 256) z = ~z & 0xff;
					cBuff[e] = ~(z << 16 | z << 8 | z);
				}
			}
			if (draw & disp_info) {
				int ln;
				struct gx_Rect box;
				sprintf(str, "Object size: %g, tvx:%d, tri:%d, seg:%d%s%s", O, msh.vtxcnt, msh.tricnt, msh.segcnt, msh.hasTex ? ", tex" : "", msh.hasNrm ? ", nrm" : "");
				gx_clipText(&box, &font, str);
				ln = -box.h;
				gx_drawText(&offs, offs.width - box.w - 10, ln += box.h, &font, str, 0xffffffff);

				sprintf(str, "camera(%g, %g, %g)", cam->pos.x, cam->pos.y, cam->pos.z);
				gx_clipText(&box, &font, str);
				gx_drawText(&offs, offs.width - box.w - 10, ln += box.h, &font, str, 0xffffffff);

				if (getobjvec(0)) {
					vector P = getobjvec(0);
					vector N = getobjvec(1);
					//~ sprintf(str, "light[%d].pos(%g, %g, %g); cos(%g), exp(%g)", rm-Lights, rm->pos.x, rm->pos.y, rm->pos.z, rm->sCos, rm->sExp);
					sprintf(str, "object: P(%.3f, %.3f, %.3f), N(%.3f, %.3f, %.3f)", P->x, P->y, P->z, N->x, N->y, N->z);
					gx_clipText(&box, &font, str); gx_drawText(&offs, offs.width - box.w - 10, ln += box.h, &font, str, 0xffffffff);
					//~ sprintf(str, "camera1(%g, %g, %g)", cam1.pos.x, cam1.pos.y, cam1.pos.z);
					//~ gx_drawText(&offs, SCRW - 8*strlen(str)-10, ln += box.h, &font, str, 0xffffffff);
				}
				for (l = userLights; l; l = l->next) {
					argb lcol = vecrgb(&l->ambi);
					sprintf(str, "light %d : %s", e, l->attr & L_on ? "On" : "Off");
					gx_clipText(&box, &font, str);
					gx_drawText(&offs, offs.width - box.w - 10, ln += box.h, &font, str, lcol.val);
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

	}

	gx_doneSurf(&font);
	gx_doneSurf(&offs);
	freeMesh(&mshLightPoint);
	//~ freeMesh(&mshLightSpot);
	//~ freeMesh(&mshLightDir);
	freeMesh(&msh);

	if (rt != NULL) {
		// debug vm memmory alocations.
		rtAlloc(rt, NULL, 0);
	}

	doneWin();
	return 0;
}
