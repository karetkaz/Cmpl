#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g2_surf.h"
#include "g3_draw.h"

#ifdef __linux__
#define stricmp(__STR1, __STR2) strcasecmp(__STR1, __STR2)
#endif

extern double epsilon;

static inline int HIBIT(unsigned int x) {
	int res = 0;
	if (x != 0) {
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		res = x - (x >> 1);
	}
	return res;
}

scalar backface(vector N, vector p1, vector p2, vector p3) {
	struct vector e1, e2, tmp;
	if (!N) N = &tmp;
	vecsub(&e1, p2, p1);
	vecsub(&e2, p3, p1);
	vecnrm(N, veccrs(N, &e1, &e2));
	N->w = 0;
	return vecdp3(N, p1);
}

int freeMesh(mesh msh) {
	msh->maxtri = msh->tricnt = 0;
	msh->maxvtx = msh->vtxcnt = 0;
	free(msh->pos);
	msh->pos = 0;
	if (msh->nrm) {
		free(msh->nrm);
		msh->nrm = 0;
	}
	if (msh->tex) {
		free(msh->tex);
		msh->tex = 0;
	}
	if (msh->map && msh->freeTex) {
		msh->freeTex = 0;
		free(msh->map);
		msh->map = 0;
	}
	free(msh->triptr);
	msh->triptr = 0;
	return 0;
}
int initMesh(mesh msh, int n) {

	msh->hlplt = -1;
	msh->tricnt = 0;
	msh->segcnt = 0;
	msh->vtxcnt = 0;
	//~ memset(&msh->map, 0, sizeof(struct gx_Surf));

	if (n < 0) {
		return 0;
	}

	msh->lit = NULL;
	msh->map = NULL;
	msh->maxtri = n <= 0 ? 16 : n;
	msh->maxseg = n <= 0 ? 16 : n;
	msh->maxvtx = n <= 0 ? 16 : n;

	//msh->freeTex = 1;
	//msh->map = gx_createSurf(0, 0, 0, 0);

	msh->triptr = (struct tri*)malloc(sizeof(struct tri) * msh->maxtri);
	msh->segptr = (struct seg*)malloc(sizeof(struct seg) * msh->maxseg);
	msh->pos = malloc(sizeof(struct vector) * msh->maxvtx);
	msh->nrm = malloc(sizeof(struct vector) * msh->maxvtx);
	msh->tex = malloc(sizeof(struct texcol) * msh->maxvtx);

	if (!msh->triptr || !msh->pos || !msh->nrm || !msh->tex) {
		//~ freeMesh(msh);
		return -1;
	}

	return 0;
}

/*int setgrp(mesh msh, int idx) {
	if (idx >= msh->maxgrp) {
		msh->maxgrp = HIBIT(idx) * 2;
		msh->grpptr = realloc(msh->pos, sizeof(struct grp*) * msh->maxgrp);
		if (!msh->grpptr) return -1;
	}
	if (msh->grpcnt <= idx) {
		msh->grpcnt = idx + 1;
	}
	return idx;
}*/
int getvtx(mesh msh, int idx) {
	if (idx >= msh->maxvtx) {
		msh->maxvtx = HIBIT(idx) * 2;
		msh->pos = (vector)realloc(msh->pos, sizeof(struct vector) * msh->maxvtx);
		msh->nrm = (vector)realloc(msh->nrm, sizeof(struct vector) * msh->maxvtx);
		msh->tex = (texcol)realloc(msh->tex, sizeof(struct texcol) * msh->maxvtx);
		if (!msh->pos || !msh->nrm || !msh->tex) return -1;
	}
	if (msh->vtxcnt <= idx) {
		msh->vtxcnt = idx + 1;
	}
	return idx;
}

int setvtxD(mesh msh, int idx, int atr, double x, double y, double z) {
	struct vector tmp[1];

	if (getvtx(msh, idx) < 0)
		return -1;

	switch (atr) {
		default:
			return -1;

		case 'P':
		case 'p':
			vecldf(msh->pos + idx, x, y, z, 1);
			break;

		case 'N':
		case 'n':
			vecnrm(msh->nrm + idx, vecldf(tmp, x, y, z, 0));
			break;
	}
	return idx;
}

int setvtxDV(mesh msh, int idx, double pos[3], double nrm[3], double tex[2], long col) {
	if (getvtx(msh, idx) < 0) {
		return -1;
	}

	if (pos != NULL) {
		vecldf(msh->pos + idx, pos[0], pos[1], pos[2], 1);
	}

	if (nrm != NULL) {
		vecldf(msh->nrm + idx, nrm[0], nrm[1], nrm[2], 0);
	}

	if (tex != NULL) {
		msh->tex[idx].s = tex[0] * 65535.;
		msh->tex[idx].t = tex[1] * 65535.;
	}

	return idx;
}
int addvtxDV(mesh msh, double pos[3], double nrm[3], double tex[2]) {
	return setvtxDV(msh, msh->vtxcnt, pos, nrm, tex, 0);
}

int setvtxFV(mesh msh, int idx, float pos[3], float nrm[3], float tex[2]) {
	if (getvtx(msh, idx) < 0) {
		return -1;
	}
	if (pos != NULL) {
		vecldf(msh->pos + idx, pos[0], pos[1], pos[2], 1);
	}
	if (nrm != NULL) {
		vecldf(msh->nrm + idx, nrm[0], nrm[1], nrm[2], 0);
	}
	if (tex != NULL) {
		msh->tex[idx].s = tex[0] * 65535.;
		msh->tex[idx].t = tex[1] * 65535.;
	}
	return idx;
}
int addvtxFV(mesh msh, float pos[3], float nrm[3], float tex[2]) {
	return setvtxFV(msh, msh->vtxcnt, pos, nrm, tex);
}

int addseg(mesh msh, int p1, int p2) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt)
		return -1;

	if (msh->segcnt >= msh->maxseg) {
		if (msh->maxseg == 0) {
			msh->maxseg = 16;
		}
		msh->segptr = (struct seg*)realloc(msh->segptr, sizeof(struct seg) * (msh->maxseg <<= 1));
		if (!msh->segptr) return -2;
	}

	msh->segptr[msh->segcnt].p1 = p1;
	msh->segptr[msh->segcnt].p2 = p2;
	return msh->segcnt++;
}
int addtri(mesh msh, int p1, int p2, int p3) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt || p3 >= msh->vtxcnt) {
		debug("addTri(%d, %d, %d)", p1, p2, p3);
		return -1;
	}

	#define H 1e-10
	if (vecdst(msh->pos + p1, msh->pos + p2) < H)
		return 0;
	if (vecdst(msh->pos + p1, msh->pos + p3) < H)
		return 0;
	if (vecdst(msh->pos + p2, msh->pos + p3) < H)
		return 0;
	#undef H

	if (msh->tricnt >= msh->maxtri) {
		msh->triptr = (struct tri*)realloc(msh->triptr, sizeof(struct tri) * (msh->maxtri <<= 1));
		if (msh->triptr == NULL) {
			return -2;
		}
	}
	msh->triptr[msh->tricnt].i1 = p1;
	msh->triptr[msh->tricnt].i2 = p2;
	msh->triptr[msh->tricnt].i3 = p3;
	return msh->tricnt++;
}
int addquad(mesh msh, int p1, int p2, int p3, int p4) {
	if (addtri(msh, p1, p2, p3) >= 0) {
		return addtri(msh, p3, p4, p1);
	}
	return -1;
}

static int vtxcmp(mesh msh, int i, int j, scalar tol) {
	scalar dif;
	dif = msh->pos[i].x - msh->pos[j].x;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	dif = msh->pos[i].y - msh->pos[j].y;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	dif = msh->pos[i].z - msh->pos[j].z;
	if (tol < (dif < 0 ? -dif : dif)) return 2;
	//~ return msh->tex[i].val != msh->tex[j].val;
	return !(msh->tex && msh->tex[i].val == msh->tex[j].val);
}

void normMesh(mesh msh, scalar tol);

static int prgDefCB(float prec) {return 0;}
static float precent(int i, int n) {return 100. * i / n;}
//~ static float roundto(float x, int n) {double muldiv = pow(10, n); return round(x * muldiv) / muldiv;}

struct growBuffer {
	char *ptr;
	int max;
	//~ int cnt;
	int esz;
};
static void* growBuff(struct growBuffer* buff, int idx) {
	int pos = idx * buff->esz;
	if (pos >= buff->max) {
		buff->max = pos << 1;
		//~ while (pos >= buff->max) buff->max <<= 1;
		//~ debug("realloc(%x, %d, %d):%d", buff->ptr, buff->esz, buff->max, idx);
		buff->ptr = realloc(buff->ptr, buff->max);
	}
	return buff->ptr ? buff->ptr + pos : NULL;
}
static void freeBuff(struct growBuffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->max = 0;
}
static void initBuff(struct growBuffer* buff, int initsize, int elemsize) {
	buff->ptr = 0;
	buff->max = initsize;
	buff->esz = elemsize;
	growBuff(buff, initsize);
}

//~ static float freadf32(FILE *fin) {float result;fread(&result, sizeof(result), 1, fin);return result;}
static char* freadstr(char buff[], int maxlen, FILE *fin) {
	char *ptr = buff;
	for ( ; maxlen > 0; --maxlen) {
		int chr = fgetc(fin);

		if (chr == -1)
			break;

		if (chr == 0)
			break;

		*ptr++ = chr;
	}
	*ptr = 0;
	return buff;
}

static int read_obj(mesh msh, const char* file) {
	FILE *fin;

	char *ws = " \t";
	char buff[65536];
	int posi = 0, texi = 0;
	int nrmi = 0, line = 0;
	struct growBuffer nrmb;
	struct growBuffer texb;

	if (!(fin = fopen(file, "rb")))
		return -1;

	initBuff(&nrmb, 64, 3 * sizeof(float));
	initBuff(&texb, 64, 2 * sizeof(float));

	for ( ; ; ) {
		char *ptr = buff;

		line++;
		if (!fgets(buff, sizeof(buff), fin))
			break;

		if (feof(fin)) break;

		// remove line end character
		if ((ptr = strchr(buff, 13)))
			*ptr = 0;

		if ((ptr = strchr(buff, 10)))
			*ptr = 0;

		if (*buff == '#')				// Comment
			continue;
		if (*buff == '\0')				// Empty line
			continue;

		// Grouping:
		if (readKVP(buff, "g", NULL, ws)) continue;		// Group name
		if (readKVP(buff, "s", NULL, ws)) continue;		// Smoothing group
		if (readKVP(buff, "o", NULL, ws)) continue;		// Object name
		//~ if (readKVP(buff, "mg", NULL, ws)) continue;	// Merging group

		// Display/render attributes:
		if (readKVP(buff, "usemtl", NULL, ws)) continue;		// Material name
		if (readKVP(buff, "mtllib", NULL, ws)) continue;		// Material library
		//~ if (readKVP(buff, "bevel", NULL, ws)) continue;	// Bevel interpolation
		//~ if (readKVP(buff, "c_interp", NULL, ws)) continue;	// Color interpolation
		//~ if (readKVP(buff, "d_interp", NULL, ws)) continue;	// Dissolve interpolation
		//~ if (readKVP(buff, "lod", NULL, ws)) continue;		// Level of detail
		//~ if (readKVP(buff, "shadow_obj", NULL, ws)) continue;	// Shadow casting
		//~ if (readKVP(buff, "trace_obj", NULL, ws)) continue;	// Ray tracing
		//~ if (readKVP(buff, "ctech", NULL, ws)) continue;	// Curve approximation technique
		//~ if (readKVP(buff, "stech", NULL, ws)) continue;	// Surface approximation technique

		// Vertex data:
		if ((ptr = readKVP(buff, "v", NULL, ws))) {		// Geometric vertices
			float vtx[3];
			sscanf(ptr, "%f%f%f", vtx + 0, vtx + 1, vtx + 2);
			setvtxFV(msh, posi, vtx, NULL, NULL);
			posi += 1;
			continue;
		}
		if ((ptr = readKVP(buff, "vt", NULL, ws))) {	// Texture vertices
			float *vtx = growBuff(&texb, texi);
			if (!vtx) {debug("memory"); break;}

			sscanf(ptr, "%f%f", vtx + 0, vtx + 1);
			if (vtx[0] < 0) vtx[0] = -vtx[0];
			if (vtx[1] < 0) vtx[1] = -vtx[1];
			if (vtx[0] > 1) vtx[0] = 0;
			if (vtx[1] > 1) vtx[1] = 0;

			if (vtx[0] < 0 || vtx[0] > 1 || vtx[1] < 0 || vtx[1] > 1) {
				break;
			}

			texi += 1;
			continue;
		}
		if ((ptr = readKVP(buff, "vn", NULL, ws))) {	// Vertex normals
			float *vtx = growBuff(&nrmb, nrmi);
			if (!vtx) {debug("memory"); break;}

			sscanf(ptr, "%f%f%f", vtx + 0, vtx + 1, vtx + 2);
			nrmi += 1;
			continue;
		}
		//~ if (readKVP(buff, "vp", NULL, ws)) continue;		// Parameter space vertices

		// Elements:
		if ((ptr = readKVP(buff, "f", NULL, ws))) {			// Face
			int i, v[4], vn[4], vt[4];

			for (i = 0; *ptr != 0; i++) {

				while (*ptr && strchr(" \t", *ptr)) ptr++;		// skip white spaces
				if (*ptr == '\0') break;				// end of line

				if (i > 4) break;						// error
				if (!strchr("+-0123456789", *ptr)) {	// error
					fprintf(stderr, "unsuported line: `%s`\n", ptr);
					break;
				}

				v[i] = vn[i] = vt[i] = 0;
				ptr = readI32(ptr, &v[i]);				// position index
				if (*ptr == '/') {						// texture index
					ptr += 1;
					if (*ptr && strchr("+-0123456789", *ptr)) {
						ptr = readI32(ptr, &vt[i]);
					}
				}
				if (*ptr == '/') {						// normal index
					ptr += 1;
					if (*ptr && strchr("+-0123456789", *ptr)) {
						ptr = readI32(ptr, &vn[i]);
					}
				}

				//~ debug("face fragment: vtx(%d), nrm(%d), tex(%d) `%s`", v[i], vn[i], vt[i], ptr);

				if (v[i]) v[i] += v[i] < 0 ? posi : -1;
				if (vn[i]) vn[i] += vn[i] < 0 ? nrmi : -1;
				if (vt[i]) vt[i] += vt[i] < 0 ? texi : -1;

				if (v[i] > posi || v[i] < 0) break;
				if (vn[i] > nrmi || vn[i] < 0) break;
				//~ if (vt[i] > texi || vt[i] < 0) break;
				if (vt[i] > texi || vt[i] < 0) vt[i] = 0;

				if (texi) {
					setvtxFV(msh, v[i], NULL, NULL, growBuff(&texb, vt[i]));
				}
				if (nrmi) {
					setvtxFV(msh, v[i], NULL, growBuff(&nrmb, vn[i]), NULL);
				}
			}

			if (*ptr != '\0')
				break;

			if (i == 3 && addtri(msh, v[0], v[1], v[2]) < 0)
				break;

			if (i == 4 && addquad(msh, v[0], v[1], v[2], v[3]) < 0)
				break;

			continue;
		}

		//~ if (readKVP(buff, "p", NULL, ws)) continue;		// Point
		//~ if (readKVP(buff, "l", NULL, ws)) continue;		// Line
		//~ if (readKVP(buff, "curv", NULL, ws)) continue;		// Curve
		//~ if (readKVP(buff, "curv2", NULL, ws)) continue;		// 2D curve
		//~ if (readKVP(buff, "surf", NULL, ws)) continue;		// Surface

		// Connectivity between free-form surfaces:
		//~ if (readKVP(buff, "con", NULL, ws)) continue;		// Connect

		// Free-form curve/surface attributes:
		//~ if (readKVP(buff, "deg", NULL, ws)) continue;		// Degree
		//~ if (readKVP(buff, "bmat", NULL, ws)) continue;	// Basis matrix
		//~ if (readKVP(buff, "step", NULL, ws)) continue;	// Step size
		//~ if (readKVP(buff, "cstype", NULL, ws)) continue;	// Curve or surface type

		// Free-form curve/surface body statements:
		//~ if (readKVP(buff, "parm", NULL, ws)) continue;	// Parameter values
		//~ if (readKVP(buff, "trim", NULL, ws)) continue;	// Outer trimming loop
		//~ if (readKVP(buff, "hole", NULL, ws)) continue;	// Inner trimming loop
		//~ if (readKVP(buff, "scrv", NULL, ws)) continue;	// Special curve
		//~ if (readKVP(buff, "sp", NULL, ws)) continue;		// Special point
		//~ if (readKVP(buff, "end", NULL, ws)) continue;		// End statement
		fprintf(stderr, "unsuported line: `%s`\n", buff);
		break;
	}

	freeBuff(&texb);
	freeBuff(&nrmb);

	if (!feof(fin)) {
		fprintf(stderr, "%s:%d: pos(%d), nrm(%d), tex(%d) `%s`\n", file, line, posi, nrmi, texi, buff);
		fclose(fin);
		return -2;
	}

	msh->hasNrm = nrmi != 0;
	msh->hasTex = texi != 0;

	fclose(fin);
	return 0;
}
static int read_3ds(mesh msh, const char* file) {
	FILE *fin;
	char buff[65536];

	unsigned short chunk_id;
	unsigned int chunk_len;
	unsigned short qty;

	int i, vtxi = 0;
	int posi = 0, texi = 0;
	//~ int mtli = 0, obji = 0;				// material, mesh index

	if (!(fin = fopen (file, "rb"))) return -1;

	for ( ; ; ) {
		if (!fread(&chunk_id, 2, 1, fin)) //Read the chunk header
			break;
		if (!fread(&chunk_len, 4, 1, fin)) //Read the lenght of the chunk
			break;
		if (feof(fin))
			break;
		switch (chunk_id) {
			//#{ Color chunks
				//~ 0x0010 : Rgb (float)
				//~ 0x0011 : Rgb (byte)
				//~ 0x0012 : Rgb (byte) gamma corrected
				//~ 0x0013 : Rgb (float) gamma corrected
			//#}
			//#{ Percent chunks
				//~ 0x0030 : percent (int)
				//~ 0x0031 : percent (float)
			//#}
			//#{ 0x4D4D: Main chunk
				//~ 0x0002 : 3DS-Version
				//#{ 0x3D3D : 3D editor chunk
					//~ 0x0100 : One unit
					//~ 0x1100 : Background bitmap
					//~ 0x1101 : Use background bitmap
					//~ 0x1200 : Background color
					//~ 0x1201 : Use background color
					//~ 0x1300 : Gradient colors
					//~ 0x1301 : Use gradient
					//~ 0x1400 : Shadow map bias
					//~ 0x1420 : Shadow map size
					//~ 0x1450 : Shadow map sample range
					//~ 0x1460 : Raytrace bias
					//~ 0x1470 : Raytrace on
					//~ 0x2100 : Ambient color
					//~ 0x2200 : Fog
					//~ 0x2201 : Use fog
					//~ 0x2210 : Fog background
					//~ 0x2300 : Distance queue
						//~ 0x2310 : Dim background
					//~ 0x2301 : Use distance queue
					//~ 0x2302 : Layered fog options
					//~ 0x2303 : Use layered fog
					//~ 0x3D3E : Mesh version
					//#{ 0x4000 : Object block
						//~ 0x4010 : Object hidden
						//~ 0x4012 : Object doesn't cast
						//~ 0x4013 : Matte object
						//~ 0x4015 : External process on
						//~ 0x4017 : Object doesn't receive shadows
						//#{ 0x4100 : Triangular mesh
							//~ 0x4110 : Vertices list
							//~ 0x4120 : Faces description
								//~ 0x4130 : Faces material list
							//~ 0x4140 : Mapping coordinates list
								//~ 0x4150 : Smoothing group list
							//~ 0x4160 : Local coordinate system
							//~ 0x4165 : Object color in editor
							//~ 0x4181 : External process name
							//~ 0x4182 : External process parameters
						//#}
						//#{ 0x4600 : Light
							//#{ 0x4610 : Spotlight
								//~ 0x4627 : Spot raytrace
								//~ 0x4630 : Light shadowed
								//~ 0x4641 : Spot shadow map
								//~ 0x4650 : Spot show cone
								//~ 0x4651 : Spot is rectangular
								//~ 0x4652 : Spot overshoot
								//~ 0x4653 : Spot map
								//~ 0x4656 : Spot roll
								//~ 0x4658 : Spot ray trace bias
							//#}
							//~ 0x4620 : Light off
							//~ 0x4625 : Attenuation on
							//~ 0x4659 : Range start
							//~ 0x465A : Range end
							//~ 0x465B : Multiplier
						//#}
						//~ 0x4700 : Camera
					//#}
					//#{ 0x7001 : Window settings
						//~ 0x7011 : Window description #2 ...
						//~ 0x7012 : Window description #1 ...
						//~ 0x7020 : Mesh windows ...
					//#}
					//#{ 0xAFFF : Material block
						//~ 0xA000 : Material name
						//~ 0xA010 : Ambient color
						//~ 0xA020 : Diffuse color
						//~ 0xA030 : Specular color
						//~ 0xA040 : Shininess percent
						//~ 0xA041 : Shininess strength percent
						//~ 0xA050 : Transparency percent
						//~ 0xA052 : Transparency falloff percent
						//~ 0xA053 : Reflection blur percent
						//~ 0xA081 : 2 sided
						//~ 0xA083 : Add trans
						//~ 0xA084 : Self illum
						//~ 0xA085 : Wire frame on
						//~ 0xA087 : Wire thickness
						//~ 0xA088 : Face map
						//~ 0xA08A : In tranc
						//~ 0xA08C : Soften
						//~ 0xA08E : Wire in units
						//~ 0xA100 : Render type
						//~ 0xA240 : Transparency falloff percent present
						//~ 0xA250 : Reflection blur percent present
						//~ 0xA252 : Bump map present (true percent)
						//~ 0xA200 : Texture map 1
						//~ 0xA33A : Texture map 2
						//~ 0xA210 : Opacity map
						//~ 0xA230 : Bump map
						//~ 0xA33C : Shininess map
						//~ 0xA204 : Specular map
						//~ 0xA33D : Self illum. map
						//~ 0xA220 : Reflection map
						//~ 0xA33E : Mask for texture map 1
						//~ 0xA340 : Mask for texture map 2
						//~ 0xA342 : Mask for opacity map
						//~ 0xA344 : Mask for bump map
						//~ 0xA346 : Mask for shininess map
						//~ 0xA348 : Mask for specular map
						//~ 0xA34A : Mask for self illum. map
						//~ 0xA34C : Mask for reflection map
						//#{ Sub-chunks for all maps:
							//~ 0xA300 : Mapping filename
							//~ 0xA351 : Mapping parameters
							//~ 0xA353 : Blur percent
							//~ 0xA354 : V scale
							//~ 0xA356 : U scale
							//~ 0xA358 : U offset
							//~ 0xA35A : V offset
							//~ 0xA35C : Rotation angle
							//~ 0xA360 : RGB Luma/Alpha tint 1
							//~ 0xA362 : RGB Luma/Alpha tint 2
							//~ 0xA364 : RGB tint R
							//~ 0xA366 : RGB tint G
							//~ 0xA368 : RGB tint B
						//#}
					//#}
				//#}
				//#{ 0xB000 : Keyframer chunk
					//~ 0xB001 : Ambient light information block
					//~ 0xB002 : Mesh information block
					//~ 0xB003 : Camera information block
					//~ 0xB004 : Camera target information block
					//~ 0xB005 : Omni light information block
					//~ 0xB006 : Spot light target information block
					//~ 0xB007 : Spot light information block
					//#{ 0xB008 : Frames (Start and End)
						//~ 0xB010 : Object name, parameters and hierarchy father
						//~ 0xB013 : Object pivot point
						//~ 0xB015 : Object morph angle
						//~ 0xB020 : Position track
						//~ 0xB021 : Rotation track
						//~ 0xB022 : Scale track
						//~ 0xB023 : FOV track
						//~ 0xB024 : Roll track
						//~ 0xB025 : Color track
						//~ 0xB026 : Morph track
						//~ 0xB027 : Hotspot track
						//~ 0xB028 : Falloff track
						//~ 0xB029 : Hide track
						//~ 0xB030 : Hierarchy position
					//#}
				//#}
			//#} Main chunk
			case 0x4d4d: break;//debug("Main chunk"); break;
				case 0x3d3d: break;//debug("3D editor chunk"); break;
					//~ case 0x0100: // One unit
					//~ case 0x1100: // Background bitmap
					//~ case 0x1101: // Use background bitmap
					//~ case 0x1200: // Background color
					//~ case 0x1201: // Use background color
					//~ case 0x1300: // Gradient colors
					//~ case 0x1301: // Use gradient
					//~ case 0x1400: // Shadow map bias
					//~ case 0x1420: // Shadow map size
					//~ case 0x1450: // Shadow map sample range
					//~ case 0x1460: // Raytrace bias
					//~ case 0x1470: // Raytrace on
					//~ case 0x2100: // Ambient color
					//~ case 0x2200: // Fog
					//~ case 0x2201: // Use fog
					//~ case 0x2210: // Fog background
					//~ case 0x2300: // Distance queue
					//~ case 0x2310: // Dim background
					//~ case 0x2301: // Use distance queue
					//~ case 0x2302: // Layered fog options
					//~ case 0x2303: // Use layered fog
					//~ case 0x3D3E: // Mesh version
						//~ goto skipchunk;// */

					case 0x0002: goto skipchunk;		// 3DS-Version
					case 0xAFFF: goto skipchunk;		// Material block

					case 0x4000: {		// Object block
						char *objName = freadstr(buff, sizeof(buff), fin);
						vtxi = posi;
						//objName = objName;
						(void)objName;
						//~ debug("Object: '%s'", objName);
					} break;
					case 0x4100: break;	// Triangular mesh
					case 0x4110: {		// Vertices list
						float dat[3];
						if (fread(&qty, sizeof(qty), 1, fin) != 1)
							goto invChunk;
						for (i = 0; i < qty; i++) {
							if (fread(dat, sizeof(float), 3, fin) != 3)
								goto invChunk;
							if (setvtxFV(msh, vtxi + i, dat, NULL, NULL) < 0) return 1;
							//~ debug("pos[%d](%f, %f, %f)", vtxi + i, dat[0], dat[1], dat[2]);
						}
						posi += qty;
					} break;
					case 0x4120: {		// Faces description
							unsigned short dat[4];
							if (fread (&qty, sizeof(qty), 1, fin) != 1)
								goto invChunk;
							for (i = 0; i < qty; i++) {
								if (fread(&dat, sizeof(unsigned short), 4, fin) != 4)
									goto invChunk;
								addtri(msh, vtxi + dat[0], vtxi + dat[1], vtxi + dat[2]);
							}
							//~ debug("faces: %d, %d, %d", vtxi, posi, texi);
					} break;
					case 0x4130:		// Faces material list
						goto skipchunk;
					case 0x4140: {		// Mapping coordinates list
						float dat[3];
						if (fread (&qty, sizeof(qty), 1, fin) != 1)
							goto invChunk;
						for (i = 0; i < qty; i++) {
							if (fread(dat, sizeof(float), 2, fin) != 2)
								goto invChunk;
							dat[1] = 1 - dat[1];
							if (setvtxFV(msh, vtxi + i, NULL, NULL, dat) < 0) return 2;
						}
						texi += qty;
					} break;
					case 0x4150:		// Smoothing group list
					case 0x4160:		// Local coordinate system
					case 0x4165:		// Object color in editor
					case 0x4181:		// External process name
					case 0x4182:		// External process parameters
				case 0xB000:			// Keyframer chunk
					goto skipchunk;
			invChunk:
			default:
				debug("unknown Chunk: ID: 0x%04x; Len: %d", chunk_id, chunk_len);
			skipchunk:
				fseek(fin, chunk_len - 6, SEEK_CUR);
				break;
		}
	}

	fclose (fin);
	msh->hasNrm = 1;
	normMesh(msh, 0);
	msh->hasTex = texi != 0;

	return 0;
}
static int save_obj(mesh msh, const char* file) {
	FILE* out;
	unsigned i;
	//~ const int prec = 2;

	if (!(out = fopen(file, "wt"))) {
		debug("fopen(%s, 'wt')", file);
		return -1;
	}

	for (i = 0; i < msh->vtxcnt; ++i) {
		vector pos = msh->pos + i;
		vector nrm = msh->nrm + i;
		scalar s = msh->tex[i].s / 65535.;
		scalar t = msh->tex[i].t / 65535.;
		if (msh->hasTex && msh->hasNrm)
			fprintf(out, "#vertex: %d\n", i);
		//~ fprintf(out, "v  %.*g %.*g %.*g\n", prec, pos->x, prec, pos->y, prec, pos->z);
		//~ fprintf(out, "v  %g %g %g\n", roundto(pos->x, prec), roundto(pos->y, prec), roundto(pos->z, prec));
		fprintf(out, "v  %g %g %g\n", pos->x, pos->y, pos->z);
		if (msh->hasTex) fprintf(out, "vt %f %f\n", s, -t);
		if (msh->hasNrm) fprintf(out, "vn %f %f %f\n", nrm->x, nrm->y, nrm->z);
	}

	for (i = 0; i < msh->tricnt; ++i) {
		int i1 = msh->triptr[i].i1 + 1;
		int i2 = msh->triptr[i].i2 + 1;
		int i3 = msh->triptr[i].i3 + 1;
		if (i + 1 < msh->tricnt) {
			int I1 = msh->triptr[i+1].i1 + 1;
			int i4 = msh->triptr[i+1].i2 + 1;
			int I3 = msh->triptr[i+1].i3 + 1;
			if (i1 == I3 && i3 == I1) {
				if (msh->hasTex && msh->hasNrm) fprintf(out, "f  %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", i1, i1, i1, i2, i2, i2, i3, i3, i3, i4, i4, i4);
				else if (msh->hasNrm) fprintf(out, "f  %d//%d %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3, i4, i4);
				else if (msh->hasTex) fprintf(out, "f  %d/%d %d/%d %d/%d %d/%d\n", i1, i1, i2, i2, i3, i3, i4, i4);
				else fprintf(out, "f  %d %d %d %d\n", i1, i2, i3, i4);
				++i;
				continue;
			}
		}
		if (msh->hasTex && msh->hasNrm) fprintf(out, "f  %d/%d/%d %d/%d/%d %d/%d/%d\n", i1, i1, i1, i2, i2, i2, i3, i3, i3);
		else if (msh->hasNrm) fprintf(out, "f  %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3);
		else if (msh->hasTex) fprintf(out, "f  %d/%d %d/%d %d/%d\n", i1, i1, i2, i2, i3, i3);
		else fprintf(out, "f  %d %d %d\n", i1, i2, i3);
	}

	for (i = 0; i < msh->segcnt; ++i) {
		int i1 = msh->segptr[i].p1 + 1;
		int i2 = msh->segptr[i].p2 + 1;
		fprintf(out, "l %d %d\n", i1, i2);
	}

	fclose(out);

	return 0;
}

int readMesh(mesh msh, const char* file) {
	char *ext = file ? fext(file) : "";
	msh->hasTex = msh->hasNrm = 0;
	msh->tricnt = msh->vtxcnt = 0;

	if (stricmp(ext, "obj") == 0)
		return read_obj(msh, file);

	if (stricmp(ext, "3ds") == 0)
		return read_3ds(msh, file);

	//~ debug("mesh '%s' not found", file);
	return -9;
}
int saveMesh(mesh msh, const char* file) {
	char *ext = file ? fext(file) : "";

	if (strcmp(ext, "obj") == 0)
		return save_obj(msh, file);

	debug("invalid extension");
	return -9;
}

void bboxMesh(mesh msh, vector min, vector max) {
	unsigned i;
	if (msh->vtxcnt == 0) {
		vecldf(min, 0, 0, 0, 0);
		vecldf(max, 0, 0, 0, 0);
		return;
	}

	*min = *max = msh->pos[0];

	for (i = 1; i < msh->vtxcnt; i += 1) {
		vecmax(max, max, &msh->pos[i]);
		vecmin(min, min, &msh->pos[i]);
	}
}

void centMesh(mesh msh, scalar size) {
	unsigned i;
	struct vector min, max, use;

	bboxMesh(msh, &min, &max);

	vecsca(&use, vecadd(&use, &max, &min), 1./2);		// scale

	for (i = 0; i < msh->vtxcnt; i += 1) {
		vecsub(&msh->pos[i], &msh->pos[i], &use);
	}

	if (size == 0) {
		return;
	}

	vecsub(&use, &max, &min);		// resize
	if (use.x < use.y)
		use.x = use.y;
	if (use.x < use.z)
		use.x = use.z;
	size = 2 * size / use.x;

	for (i = 0; i < msh->vtxcnt; i += 1) {
		vecsca(&msh->pos[i], &msh->pos[i], size);
		msh->pos[i].w = 1;
	}
}

/** recalculate mesh normals
 * 
 */
void normMesh(mesh msh, scalar tolerance) {
	struct vector tmp;
	unsigned i, j;
	for (i = 0; i < msh->vtxcnt; i += 1)
		vecldf(&msh->nrm[i], 0, 0, 0, 1);

	for (i = 0; i < msh->tricnt; i += 1) {
		struct vector nrm;
		int i1 = msh->triptr[i].i1;
		int i2 = msh->triptr[i].i2;
		int i3 = msh->triptr[i].i3;
		backface(&nrm, &msh->pos[i1], &msh->pos[i2], &msh->pos[i3]);
		vecadd(&msh->nrm[i1], &msh->nrm[i1], &nrm);
		vecadd(&msh->nrm[i2], &msh->nrm[i2], &nrm);
		vecadd(&msh->nrm[i3], &msh->nrm[i3], &nrm);
	}

	if (tolerance != 0) {
		for (i = 1; i < msh->vtxcnt; i += 1) {
			for (j = 0; j < i; j += 1) {
				if (vtxcmp(msh, i, j, tolerance) < 2) {
					vecadd(&tmp, &msh->nrm[j], &msh->nrm[i]);
					msh->nrm[i] = tmp;
					msh->nrm[j] = tmp;
					break;
				}
			}
		}
	}

	for (i = 0; i < msh->vtxcnt; i += 1) {
		//~ vecsca(&msh->vtxptr[i].nrm, &msh->vtxptr[i].nrm, 1./msh->vtxptr[i].nrm.w);
		msh->nrm[i].w = 0;
		vecnrm(&msh->nrm[i], &msh->nrm[i]);
	}
}

/**	subdivide mesh
 *	TODO: not bezier curve => patch eval
**/
static void pntri(struct vector res[10], mesh msh, int i1, int i2, int i3) {
	#define P1 (msh->pos + i1)
	#define P2 (msh->pos + i2)
	#define P3 (msh->pos + i3)
	#define N1 (msh->nrm + i1)
	#define N2 (msh->nrm + i2)
	#define N3 (msh->nrm + i3)
	struct vector tmp[3];
	scalar w12 = vecdp3(vecsub(tmp, P2, P1), N1);
	scalar w13 = vecdp3(vecsub(tmp, P3, P1), N1);
	scalar w21 = vecdp3(vecsub(tmp, P1, P2), N2);
	scalar w23 = vecdp3(vecsub(tmp, P3, P2), N2);
	scalar w31 = vecdp3(vecsub(tmp, P1, P3), N3);
	scalar w32 = vecdp3(vecsub(tmp, P2, P3), N3);

	vector b300 = veccpy(res + 0, P1);
	vector b030 = veccpy(res + 1, P2);
	vector b003 = veccpy(res + 2, P3);

	vector b210 = vecsca(res + 3, vecsub(tmp, vecadd(tmp, vecsca(tmp, P1, 2), P2), vecsca(tmp + 1, N1, w12)), 1./3);
	vector b120 = vecsca(res + 4, vecsub(tmp, vecadd(tmp, vecsca(tmp, P2, 2), P1), vecsca(tmp + 1, N2, w21)), 1./3);
	vector b021 = vecsca(res + 5, vecsub(tmp, vecadd(tmp, vecsca(tmp, P2, 2), P3), vecsca(tmp + 1, N2, w23)), 1./3);
	vector b012 = vecsca(res + 6, vecsub(tmp, vecadd(tmp, vecsca(tmp, P3, 2), P2), vecsca(tmp + 1, N3, w32)), 1./3);
	vector b102 = vecsca(res + 7, vecsub(tmp, vecadd(tmp, vecsca(tmp, P3, 2), P1), vecsca(tmp + 1, N3, w31)), 1./3);
	vector b201 = vecsca(res + 8, vecsub(tmp, vecadd(tmp, vecsca(tmp, P1, 2), P3), vecsca(tmp + 1, N1, w13)), 1./3);

	vector E = vecsca(tmp + 1, vecadd(tmp, vecadd(tmp, vecadd(tmp, vecadd(tmp, vecadd(tmp, b210, b120), b021), b012), b102), b201), 1./6);
	vector V = vecsca(tmp + 2, vecadd(tmp, vecadd(tmp, P1, P2), P3), 1./3);
	vector b111 = vecsca(res + 9, vecadd(tmp, E, vecsub(tmp, E, V)), 1./2);

	(void)b300;
	(void)b030;
	(void)b003;

	(void)b210;
	(void)b120;
	(void)b021;
	(void)b012;
	(void)b102;
	(void)b201;

	(void)b111;

	#undef P1
	#undef P2
	#undef P3
	#undef N1
	#undef N2
	#undef N3
}

static vector bezexp(struct vector res[4], vector p0, vector c0, vector c1, vector p1) {
	struct vector tmp[5];
	p0 = veccpy(tmp + 1, p0);
	c0 = veccpy(tmp + 2, c0);
	p1 = veccpy(tmp + 3, p1);
	c1 = veccpy(tmp + 4, c1);

	veccpy(res + 0, p0);
	vecsca(res + 1, vecsub(tmp, c0, p0), 3);
	vecsub(res + 2, vecsca(tmp, vecsub(tmp, c1, c0), 3), res+1);
	vecsub(res + 3, vecsub(tmp, vecsub(tmp, p1, res+2), res+1), res+0);
	return res;
}
static vector bezevl(struct vector res[1], struct vector p[4], scalar t) {
	//~ p = (((p3 * t + p2) * t + p1) * t + p0);
	vecadd(res, vecsca(res, p+3, t), p+2);
	vecadd(res, vecsca(res, res, t), p+1);
	vecadd(res, vecsca(res, res, t), p+0);
	return res;
}

// subdivide using bezier
int sdvtxBP(mesh msh, int a, int b, vector c1, vector c2) {
	struct vector tmp[6];//, res[3];
	int r = getvtx(msh, msh->vtxcnt);
	vecnrm(msh->nrm + r, vecadd(tmp, msh->nrm + a, msh->nrm + b));
	bezevl(msh->pos + r, bezexp(tmp, msh->pos + a, c1, c2, msh->pos + b), .5);
	return r;
}
int sdvtx(mesh msh, int a, int b) {
	struct vector tmp[1];
	int r = getvtx(msh, msh->vtxcnt);
	vecnrm(msh->nrm + r, vecadd(tmp, msh->nrm + a, msh->nrm + b));
	vecsca(msh->pos + r, vecadd(tmp, msh->pos + a, msh->pos + b), .5);
	return r;
}

void sdivMesh(mesh msh, int smooth) {
	unsigned i, n = msh->tricnt;
	if (smooth) for (i = 0; i < n; i++) {
		struct vector pn[11];
		unsigned i1 = msh->triptr[i].i1;
		unsigned i2 = msh->triptr[i].i2;
		unsigned i3 = msh->triptr[i].i3;
		unsigned i4, i5, i6;
		pntri(pn, msh, i1, i2, i3);
		i4 = sdvtxBP(msh, i1, i2, pn + 3, pn + 4);
		i5 = sdvtxBP(msh, i2, i3, pn + 5, pn + 6);
		i6 = sdvtxBP(msh, i3, i1, pn + 7, pn + 8);

		msh->triptr[i].i2 = i4;
		msh->triptr[i].i3 = i6;
		addtri(msh, i4, i2, i5);
		addtri(msh, i5, i3, i6);
		addtri(msh, i4, i5, i6);
	}
	else for (i = 0; i < n; i++) {
		signed i1 = msh->triptr[i].i1;
		signed i2 = msh->triptr[i].i2;
		signed i3 = msh->triptr[i].i3;
		signed i4 = sdvtx(msh, i1, i2);
		signed i5 = sdvtx(msh, i2, i3);
		signed i6 = sdvtx(msh, i3, i1);

		msh->triptr[i].i2 = i4;
		msh->triptr[i].i3 = i6;
		addtri(msh, i4, i2, i5);
		addtri(msh, i5, i3, i6);
		addtri(msh, i4, i5, i6);
	}
}
// */

void optiMesh(mesh msh, scalar tol, int prgCB(float prec)) {
	unsigned i, j, k, n;
	if (prgCB == NULL)
		prgCB = prgDefCB;
	prgCB(0);
	for (n = i = 1; i < msh->vtxcnt; i += 1) {
		if (prgCB(precent(i, msh->vtxcnt)) == -1) break;
		for (j = 0; j < n && vtxcmp(msh, i, j, tol) != 0; j += 1);

		if (j == n) {
			msh->pos[n] = msh->pos[i];
			msh->nrm[n] = msh->nrm[i];
			msh->tex[n] = msh->tex[i];
			n++;
		}
		//~ else vecadd(&msh->vtxptr[j].nrm, &msh->vtxptr[j].nrm, &msh->vtxptr[i].nrm);
		for (k = 0; k < msh->tricnt; k += 1) {
			if (msh->triptr[k].i1 == i) msh->triptr[k].i1 = j;
			if (msh->triptr[k].i2 == i) msh->triptr[k].i2 = j;
			if (msh->triptr[k].i3 == i) msh->triptr[k].i3 = j;
		}
	}
	msh->vtxcnt = n;
	if (prgCB(100) == -1) return;
	//~ printf("vtx cnt %d\n", msh->vtxcnt);

	//~ for (j = 0; j < n; j += 1) vecnrm(&msh->vtxptr[j].nrm, &msh->vtxptr[j].nrm);
}// */

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

#if 1 // eval mesh with scripting support
typedef struct userData {
	double s, smin, smax;
	double t, tmin, tmax;
	int isPos, isNrm;
	double pos[3], nrm[3];
} *userData;

#include "pvmc.h"

static int getS(libcArgs rt) {
	userData d = rt->extra;
	retf64(rt, lerp(d->smin, d->smax, d->s));
	return 0;
}
static int getT(libcArgs rt) {
	userData d = rt->extra;
	retf64(rt, lerp(d->tmin, d->tmax, d->t));
	return 0;
}
static int setPos(libcArgs rt) {
	userData d = rt->extra;
	d->pos[0] = argf64(rt, 8 * 0);
	d->pos[1] = argf64(rt, 8 * 1);
	d->pos[2] = argf64(rt, 8 * 2);
	d->isPos = 1;
	return 0;
}
static int setNrm(libcArgs rt) {
	userData d = rt->extra;
	d->nrm[0] = argf64(rt, 8 * 0);
	d->nrm[1] = argf64(rt, 8 * 1);
	d->nrm[2] = argf64(rt, 8 * 2);
	d->isNrm = 1;
	return 0;
}

static int f64abs(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, fabs(x));
	return 0;
}
static int f64sin(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sin(x));
	return 0;
}
static int f64cos(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, cos(x));
	return 0;
}
static int f64tan(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, tan(x));
	return 0;
}
static int f64log(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, log(x));
	return 0;
}
static int f64exp(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, exp(x));
	return 0;
}
static int f64pow(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, pow(x, y));
	return 0;
}
static int f64sqrt(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	retf64(rt, sqrt(x));
	return 0;
}
static int f64atan2(libcArgs rt) {
	float64_t x = argf64(rt, 0);
	float64_t y = argf64(rt, 8);
	retf64(rt, atan2(x, y));
	return 0;
}

int evalMesh(mesh msh, int sdiv, int tdiv, char *src, char *file, int line) {
	static char mem[32 << 10];		// 32K memory
	state rt = rtInit(mem, sizeof(mem));
	struct userData ud;
	const int warnlevel = 2;
	const int stacksize = sizeof(mem) / 2;

	char *logf = NULL;//"dump.evalMesh.txt";

	int i, j, err = 0;

	double s, t, ds, dt;	// 0 .. 1

	// pointers, variants and emit are not needed.
	if (!ccInit(rt, creg_base, NULL)) {
		debug("Internal error\n");
		return -1;
	}

	err = err || !ccAddCall(rt, getS, NULL,     "float64 gets();");
	err = err || !ccAddCall(rt, getT, NULL,     "float64 gett();");
	err = err || !ccAddCall(rt, setPos, NULL,   "void setPos(float64 x, float64 y, float64 z);");
	err = err || !ccAddCall(rt, setNrm, NULL,   "void setNrm(float64 x, float64 y, float64 z);");
	err = err || !ccAddCall(rt, f64abs, NULL,   "float64 abs(float64 x);");
	err = err || !ccAddCall(rt, f64sin, NULL,   "float64 sin(float64 x);");
	err = err || !ccAddCall(rt, f64cos, NULL,   "float64 cos(float64 x);");
	err = err || !ccAddCall(rt, f64tan, NULL,   "float64 tan(float64 x);");
	err = err || !ccAddCall(rt, f64log, NULL,   "float64 log(float64 x);");
	err = err || !ccAddCall(rt, f64exp, NULL,   "float64 exp(float64 x);");
	err = err || !ccAddCall(rt, f64sqrt, NULL,  "float64 sqrt(float64 x);");
	err = err || !ccAddCall(rt, f64atan2, NULL, "float64 atan(float64 x, float64 y);");
	err = err || !ccAddCall(rt, f64pow, NULL,   "float64 pow(float64 x, float64 y);");
	err = err || !ccDefFlt(rt, "pi", 3.14159265358979323846264338327950288419716939937510582097494459);
	err = err || !ccDefFlt(rt, "e",  2.71828182845904523536028747135266249775724709369995957496696763);

	err = err || !ccAddCode(rt, warnlevel, __FILE__, __LINE__ + 1,
		"const double s = gets();\n"
		"const double t = gett();\n"
		"double x = s;\n"
		"double y = t;\n"
		"double z = 0;\n"
	);
	err = err || !ccAddCode(rt, warnlevel, file, line, src);
	err = err || !ccAddCode(rt, warnlevel, __FILE__, __LINE__, "setPos(x, y, z);\n");
	// */

	// optimize on max level, and generate global variables on stack
	if (err || !gencode(rt, cgen_glob | 0xff)) {
		debug("error compiling(%d), see `%s`", err, logf);
		logfile(rt, NULL);
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
	logfile(rt, NULL);

	#define findint(__ENV, __NAME, _OUT_VAL) ccSymValInt(ccFindSym(__ENV, NULL, __NAME), _OUT_VAL)
	#define findflt(__ENV, __NAME, _OUT_VAL) ccSymValFlt(ccFindSym(__ENV, NULL, __NAME), _OUT_VAL)

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

	//~ debug("s(min:%lf, max:%lf, div:%d%s)", ud.smin, ud.smax, sdiv, /* cs ? ", closed" : */ "");
	//~ debug("t(min:%lf, max:%lf, div:%d%s)", ud.tmin, ud.tmax, tdiv, /* ct ? ", closed" : */ "");
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
			if (vmExec(rt, &ud, stacksize) != 0) {
				debug("error");
				return -4;
			}
			dv3cpy(pos, ud.pos);

			if (ud.isNrm) {
				dv3nrm(nrm, ud.nrm);
			}
			else {
				ud.s = s + epsilon;
				ud.t = t;
				if (vmExec(rt, &ud, stacksize) != 0) {
					debug("error");
					return -5;
				}
				dv3cpy(ds, ud.pos);

				ud.s = s;
				ud.t = t + epsilon;
				if (vmExec(rt, &ud, stacksize) != 0) {
					debug("error");
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
