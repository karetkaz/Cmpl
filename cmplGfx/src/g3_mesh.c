#include "g3_draw.h"

#include <stdlib.h>
#include <string.h>

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

static char* readKVP(char *ptr, char *key, char *sep, char *wsp) {	// key = value pair
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
static char* readI32(char *ptr, int *outVal) {
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

GxMesh createMesh(GxMesh recycle, size_t n) {
	if (recycle == NULL) {
		recycle = malloc(sizeof(struct GxMesh));
		if (recycle == NULL) {
			gx_debug("out of memory");
			return NULL;
		}
		recycle->freeMesh = 1;
	}
	else {
		free(recycle->triptr);
		free(recycle->segptr);
		free(recycle->vtxptr);
		if (recycle->freeMap) {
			free(recycle->mtl.texture);
		}
		recycle->triptr = NULL;
		recycle->segptr = NULL;
		recycle->vtxptr = NULL;
		recycle->mtl.texture = NULL;
	}

	recycle->freeMap = 0;
	recycle->mtl.texture = NULL;

	if (n < 16) {
		n = 16;
	}

	recycle->tricnt = 0;
	recycle->maxtri = n;
	recycle->triptr = malloc(sizeof(struct GxMeshTri) * n);

	recycle->segcnt = 0;
	recycle->maxseg = n;
	recycle->segptr = malloc(sizeof(struct GxMeshSeg) * n);

	recycle->vtxcnt = 0;
	recycle->maxvtx = n;
	recycle->vtxptr = malloc(sizeof(struct GxMeshVtx) * n);

	if (!recycle->triptr || !recycle->segptr || !recycle->vtxptr) {
		gx_debug("out of memory");
		destroyMesh(recycle);
		return NULL;
	}

	return recycle;
}
void destroyMesh(GxMesh msh) {
	msh->maxvtx = msh->vtxcnt = 0;
	free(msh->vtxptr);
	msh->vtxptr = NULL;

	msh->maxtri = msh->tricnt = 0;
	free(msh->triptr);
	msh->triptr = NULL;

	msh->maxseg = msh->segcnt = 0;
	free(msh->segptr);
	msh->segptr = NULL;

	if (msh->mtl.texture && msh->freeMap) {
		msh->freeMap = 0;
		free(msh->mtl.texture);
		msh->mtl.texture = 0;
	}

	if (msh->freeMesh) {
		free(msh);
	}
}

static GxMeshVtx getVtx(GxMesh msh, size_t idx) {
	if (msh->maxvtx <= idx) {
		msh->maxvtx = HIBIT(idx) * 2;
		msh->vtxptr = realloc(msh->vtxptr, sizeof(struct GxMeshVtx) * msh->maxvtx);
		if (msh->vtxptr == NULL) {
			return NULL;
		}
	}
	if (msh->vtxcnt <= idx) {
		msh->vtxcnt = idx + 1;
	}
	return msh->vtxptr + idx;
}
int setVtx(GxMesh msh, size_t idx, scalar *pos, scalar *nrm, scalar *tex) {
	GxMeshVtx vtx = getVtx(msh, idx);
	if (vtx == NULL) {
		return 0;
	}
	if (pos != NULL) {
		vtx->p.x = pos[0];
		vtx->p.y = pos[1];
		vtx->p.z = pos[2];
		vtx->p.w = 1;
	}
	if (nrm != NULL) {
		vtx->n.x = nrm[0];
		vtx->n.y = nrm[1];
		vtx->n.z = nrm[2];
		vtx->n.w = 0;
		msh->hasNrm = 1;
	}
	if (tex != NULL) {
		vtx->s = (uint16_t) (tex[0] * 65535);
		vtx->t = (uint16_t) (tex[1] * 65535);
		msh->hasTex = 1;
	}
	return 1;
}

int addSeg(GxMesh msh, size_t p1, size_t p2) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt) {
		return 0;
	}

	if (msh->segcnt >= msh->maxseg) {
		if (msh->maxseg == 0) {
			msh->maxseg = 16;
		}
		msh->segptr = realloc(msh->segptr, sizeof(struct GxMeshSeg) * (msh->maxseg <<= 1));
		if (!msh->segptr) {
			return 0;
		}
	}

	msh->segptr[msh->segcnt].p1 = p1;
	msh->segptr[msh->segcnt].p2 = p2;
	msh->segcnt++;
	return 1;
}

int addTri(GxMesh msh, size_t p1, size_t p2, size_t p3) {
	if (p1 >= msh->vtxcnt || p2 >= msh->vtxcnt || p3 >= msh->vtxcnt) {
		return 0;
	}

	if (msh->tricnt >= msh->maxtri) {
		msh->triptr = realloc(msh->triptr, sizeof(struct GxMeshTri) * (msh->maxtri <<= 1));
		if (msh->triptr == NULL) {
			return 0;
		}
	}
	msh->triptr[msh->tricnt].i1 = p1;
	msh->triptr[msh->tricnt].i2 = p2;
	msh->triptr[msh->tricnt].i3 = p3;
	msh->tricnt++;
	return 1;
}

static int cmpVtx(GxMesh msh, size_t i, size_t j, scalar tolerance) {
	scalar dif = 0;
	GxMeshVtx v1 = &msh->vtxptr[i];
	GxMeshVtx v2 = &msh->vtxptr[j];
	dif = v1->p.x - v2->p.x;
	if (tolerance < (dif < 0 ? -dif : dif)) return 2;
	dif = v1->p.y - v2->p.y;
	if (tolerance < (dif < 0 ? -dif : dif)) return 2;
	dif = v1->p.z - v2->p.z;
	if (tolerance < (dif < 0 ? -dif : dif)) return 2;
	return !(v1->s == v2->s && v1->t == v2->t);
}

struct Buffer {
	char *ptr;
	size_t max;
	size_t esz;
};
static void* growBuff(struct Buffer* buff, int idx) {
	size_t pos = idx * buff->esz;
	if (pos >= buff->max) {
		buff->max = pos << 1;
		buff->ptr = realloc(buff->ptr, buff->max);
	}
	return buff->ptr ? buff->ptr + pos : NULL;
}
static void destroyBuff(struct Buffer* buff) {
	free(buff->ptr);
	buff->ptr = 0;
	buff->max = 0;
}
static void initBuff(struct Buffer* buff, int initSize, int elementSize) {
	buff->ptr = 0;
	buff->max = initSize;
	buff->esz = elementSize;
	growBuff(buff, initSize);
}

static char* freadStr(char *buff, int maxLength, FILE *input) {
	char *ptr = buff;
	for ( ; maxLength > 0; --maxLength) {
		int chr = fgetc(input);

		if (chr == -1)
			break;

		if (chr == 0)
			break;

		*ptr++ = chr;
	}
	*ptr = 0;
	return buff;
}

int readObj(GxMesh msh, const char *file) {
	FILE *fin;

	char *ws = " \t";
	char buff[65536], *ptr;
	int posi = 0, texi = 0;
	int nrmi = 0, line = 0;
	struct Buffer nrmb;
	struct Buffer texb;

	if (!(fin = fopen(file, "rb"))) {
		gx_debug("file open error: %s", file);
		return 0;
	}

	initBuff(&nrmb, 64, 3 * sizeof(float));
	initBuff(&texb, 64, 2 * sizeof(float));

	for ( ; ; ) {
		line++;
		if (!fgets(buff, sizeof(buff), fin)) {
			break;
		}

		if (feof(fin)) {
			break;
		}

		// remove line end character
		if ((ptr = strchr(buff, 13))) {
			*ptr = 0;
		}

		if ((ptr = strchr(buff, 10))) {
			*ptr = 0;
		}

		if (*buff == '#') {                // Comment
			continue;
		}
		if (*buff == '\0') {               // Empty line
			continue;
		}

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
			struct vector vtx;
			sscanf(ptr, "%f%f%f", &vtx.x, &vtx.y, &vtx.z);
			if (!setVtx(msh, posi, vtx.v, NULL, NULL)) {
				gx_debug("out of memory");
				break;
			}
			posi += 1;
			continue;
		}
		if ((ptr = readKVP(buff, "vt", NULL, ws))) {	// Texture vertices
			float *vtx = growBuff(&texb, texi);
			if (vtx == NULL) {
				gx_debug("out of memory");
				break;
			}

			sscanf(ptr, "%f%f", vtx + 0, vtx + 1);
			if (vtx[0] < 0) vtx[0] = -vtx[0];
			if (vtx[1] < 0) vtx[1] = -vtx[1];
			if (vtx[0] > 1) vtx[0] = 0;
			if (vtx[1] > 1) vtx[1] = 0;

			if (vtx[0] < 0 || vtx[0] > 1 || vtx[1] < 0 || vtx[1] > 1) {
				gx_debug("input error");
				break;
			}

			texi += 1;
			continue;
		}
		if ((ptr = readKVP(buff, "vn", NULL, ws))) {	// Vertex normals
			float *vtx = growBuff(&nrmb, nrmi);
			if (vtx == NULL) {
				gx_debug("out of memory");
				break;
			}

			sscanf(ptr, "%f%f%f", vtx + 0, vtx + 1, vtx + 2);
			nrmi += 1;
			continue;
		}
		//~ if (readKVP(buff, "vp", NULL, ws)) continue;		// Parameter space vertices

		// Elements:
		if ((ptr = readKVP(buff, "f", NULL, ws))) {			// Face
			int i, v[4], vn[4], vt[4];

			for (i = 0; *ptr != 0; i++) {
				while (*ptr && strchr(" \t", *ptr)) {
					// skip over white spaces
					ptr++;
				}
				if (*ptr == '\0') {
					// end of line
					break;
				}

				if (i > 4) {
					gx_debug("input error");
					break;
				}
				if (!strchr("+-0123456789", *ptr)) {
					gx_debug("input error");
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

				if (v[i]) v[i] += v[i] < 0 ? posi : -1;
				if (vn[i]) vn[i] += vn[i] < 0 ? nrmi : -1;
				if (vt[i]) vt[i] += vt[i] < 0 ? texi : -1;

				if (v[i] > posi || v[i] < 0) break;
				if (vn[i] > nrmi || vn[i] < 0) break;
				if (vt[i] > texi || vt[i] < 0) break;

				if (texi) {
					if (!setVtx(msh, v[i], NULL, NULL, growBuff(&texb, vt[i]))) {
						gx_debug("input error");
						break;
					}
				}
				if (nrmi) {
					if (!setVtx(msh, v[i], NULL, growBuff(&nrmb, vn[i]), NULL)) {
						gx_debug("input error");
						break;
					}
				}
			}

			if (*ptr != '\0') {
				break;
			}

			if (i == 3 && !addTri(msh, v[0], v[1], v[2])) {
				gx_debug("input error");
				break;
			}

			if (i == 4 && !addQuad(msh, v[0], v[1], v[2], v[3])) {
				gx_debug("input error");
				break;
			}
			continue;
		}

		if (readKVP(buff, "p", NULL, ws)) continue;			// Point
		if (readKVP(buff, "l", NULL, ws)) continue;			// Line
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
		gx_debug("input error");
		break;
	}

	destroyBuff(&texb);
	destroyBuff(&nrmb);

	if (!feof(fin)) {
		gx_debug("%s:%d: pos(%d), nrm(%d), tex(%d) `%s`", file, line, posi, nrmi, texi, buff);
		fclose(fin);
		return 0;
	}

	msh->hasNrm = nrmi != 0;
	msh->hasTex = texi != 0;

	fclose(fin);
	return 1;
}
int read3ds(GxMesh msh, const char *file) {

	FILE *fin = fopen(file, "rb");
	if (fin == NULL) {
		gx_debug("file open error: %s", file);
		return 0;
	}

	int vtxi = 0;
	int posi = 0;
	int texi = 0;
	for ( ; ; ) {
		//Read the chunk header
		unsigned short chunk_id;
		if (!fread(&chunk_id, 2, 1, fin)) {
			gx_debug("input error");
			break;
		}

		//Read the lenght of the chunk
		unsigned int chunk_len;
		if (!fread(&chunk_len, 4, 1, fin)) {
			gx_debug("input error");
			break;
		}

		if (feof(fin)) {
			break;
		}
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
			case 0x4d4d: break;//gx_debug("Main chunk"); break;
				case 0x3d3d: break;//gx_debug("3D editor chunk"); break;
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
						char buff[65536];
						char *objName = freadStr(buff, sizeof(buff), fin);
						(void)objName;
						vtxi = posi;
					} break;
					case 0x4100: break;	// Triangular mesh
					case 0x4110: {		// Vertices list
						struct vector dat;
						unsigned short qty;
						if (fread(&qty, sizeof(qty), 1, fin) != 1)
							goto invChunk;
						for (int i = 0; i < qty; i++) {
							if (fread(&dat, sizeof(float), 3, fin) != 3) {
								goto invChunk;
							}
							dat.w = 0;
							if (!setVtx(msh, vtxi + i, dat.v, NULL, NULL)) {
								return 0;
							}
						}
						posi += qty;
					} break;
					case 0x4120: {		// Faces description
							unsigned short dat[4];
						unsigned short qty;
							if (fread (&qty, sizeof(qty), 1, fin) != 1)
								goto invChunk;
							for (int i = 0; i < qty; i++) {
								if (fread(&dat, sizeof(unsigned short), 4, fin) != 4) {
									goto invChunk;
								}
								if (!addTri(msh, vtxi + dat[0], vtxi + dat[1], vtxi + dat[2])) {
									return 0;
								}
							}
					} break;
					case 0x4130:		// Faces material list
						goto skipchunk;
					case 0x4140: {		// Mapping coordinates list
						float dat[3];
						unsigned short qty;
						if (fread (&qty, sizeof(qty), 1, fin) != 1) {
							goto invChunk;
						}
						for (int i = 0; i < qty; i++) {
							if (fread(dat, sizeof(float), 2, fin) != 2) {
								goto invChunk;
							}
							dat[1] = 1 - dat[1];
							if (!setVtx(msh, vtxi + i, NULL, NULL, dat)) {
								return 0;
							}
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
				gx_debug("unimplemented chunk: 0x%04x; length: %d", chunk_id, chunk_len);
			skipchunk:
				fseek(fin, chunk_len - 6, SEEK_CUR);
				break;
		}
	}

	normMesh(msh, 0, NULL, NULL);
	msh->hasNrm = 1;
	msh->hasTex = texi != 0;

	fclose(fin);
	return 1;
}
int saveObj(GxMesh msh, const char *file) {
	FILE* out = fopen(file, "wt");
	if (out == NULL) {
		gx_debug("file open error: %s", file);
		return 0;
	}

	for (size_t i = 0; i < msh->vtxcnt; ++i) {
		if (msh->hasTex && msh->hasNrm) {
			fprintf(out, "#vertex: %ld\n", (long) i);
		}

		GxMeshVtx vtx = msh->vtxptr + i;
		fprintf(out, "v  %g %g %g\n", vtx->p.x, vtx->p.y, vtx->p.z);

		if (msh->hasTex) {
			scalar s = vtx->s / 65535.;
			scalar t = vtx->t / 65535.;
			fprintf(out, "vt %f %f\n", s, -t);
		}

		if (msh->hasNrm) {
			fprintf(out, "vn %f %f %f\n", vtx->n.x, vtx->n.y, vtx->n.z);
		}
	}

	for (size_t i = 0; i < msh->tricnt; ++i) {
		int i1 = msh->triptr[i].i1 + 1;
		int i2 = msh->triptr[i].i2 + 1;
		int i3 = msh->triptr[i].i3 + 1;
		if (i + 1 < msh->tricnt) {
			int I1 = msh->triptr[i+1].i1 + 1;
			int i4 = msh->triptr[i+1].i2 + 1;
			int I3 = msh->triptr[i+1].i3 + 1;
			if (i1 == I3 && i3 == I1) {
				if (msh->hasTex && msh->hasNrm) {
					fprintf(out, "f  %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", i1, i1, i1, i2, i2, i2, i3, i3, i3, i4, i4, i4);
				}
				else if (msh->hasNrm) {
					fprintf(out, "f  %d//%d %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3, i4, i4);
				}
				else if (msh->hasTex) {
					fprintf(out, "f  %d/%d %d/%d %d/%d %d/%d\n", i1, i1, i2, i2, i3, i3, i4, i4);
				}
				else {
					fprintf(out, "f  %d %d %d %d\n", i1, i2, i3, i4);
				}
				++i;
				continue;
			}
		}
		if (msh->hasTex && msh->hasNrm) {
			fprintf(out, "f  %d/%d/%d %d/%d/%d %d/%d/%d\n", i1, i1, i1, i2, i2, i2, i3, i3, i3);
		}
		else if (msh->hasNrm) {
			fprintf(out, "f  %d//%d %d//%d %d//%d\n", i1, i1, i2, i2, i3, i3);
		}
		else if (msh->hasTex) {
			fprintf(out, "f  %d/%d %d/%d %d/%d\n", i1, i1, i2, i2, i3, i3);
		}
		else {
			fprintf(out, "f  %d %d %d\n", i1, i2, i3);
		}
	}

	for (size_t i = 0; i < msh->segcnt; ++i) {
		int i1 = msh->segptr[i].p1 + 1;
		int i2 = msh->segptr[i].p2 + 1;
		fprintf(out, "l %d %d\n", i1, i2);
	}

	fclose(out);
	return 1;
}

void bboxMesh(GxMesh msh, vector min, vector max) {
	if (msh->vtxcnt == 0) {
		vecldf(min, 0, 0, 0, 0);
		vecldf(max, 0, 0, 0, 0);
		return;
	}

	*min = *max = msh->vtxptr[0].p;

	for (size_t i = 1; i < msh->vtxcnt; i += 1) {
		vecmax(max, max, &msh->vtxptr[i].p);
		vecmin(min, min, &msh->vtxptr[i].p);
	}
}

void normMesh(GxMesh msh, scalar tolerance, vector recenter, vector resize) {
	if (msh->vtxcnt == 0) {
		return;
	}
	struct vector min = msh->vtxptr[0].p;
	struct vector max = msh->vtxptr[0].p;

	for (size_t i = 0; i < msh->vtxcnt; i += 1) {
		GxMeshVtx vtx = msh->vtxptr + i;
		vecldf(&vtx->n, 0, 0, 0, 1);
		vecmax(&max, &max, &vtx->p);
		vecmin(&min, &min, &vtx->p);
	}

	for (size_t i = 0; i < msh->tricnt; i += 1) {
		struct vector nrm;
		size_t i1 = msh->triptr[i].i1;
		size_t i2 = msh->triptr[i].i2;
		size_t i3 = msh->triptr[i].i3;
		GxMeshVtx v1 = msh->vtxptr + i1;
		GxMeshVtx v2 = msh->vtxptr + i2;
		GxMeshVtx v3 = msh->vtxptr + i3;
		backface(&nrm, &v1->p, &v2->p, &v3->p);
		vecadd(&v1->n, &v1->n, &nrm);
		vecadd(&v2->n, &v2->n, &nrm);
		vecadd(&v3->n, &v3->n, &nrm);
	}

	if (tolerance != 0) {
		for (size_t i = 1; i < msh->vtxcnt; i += 1) {
			for (size_t j = 0; j < i; j += 1) {
				if (cmpVtx(msh, i, j, tolerance) < 2) {
					struct vector nrm;
					vecadd(&nrm, &msh->vtxptr[j].n, &msh->vtxptr[i].n);
					msh->vtxptr[i].n = nrm;
					msh->vtxptr[j].n = nrm;
					break;
				}
			}
		}
	}

	struct vector move = vec(0, 0, 0, 0);
	if (recenter != NULL) {
		vecadd(&move, &max, &min);
		vecsca(&move, &move, 1.f / 2);
		vecadd(&move, &move, recenter);
	}

	struct vector size = vec(1, 1, 1, 0);
	if (resize != NULL) {
		vecsub(&size, &max, &min);
		scalar asp = 0;
		if (asp < size.x) {
			asp = size.x;
		}
		if (asp < size.y) {
			asp = size.y;
		}
		if (asp < size.z) {
			asp = size.z;
		}

		if (resize->x != 0) {
			size.x = resize->x / asp;
		}
		if (resize->y != 0) {
			size.y = resize->y / asp;
		}
		if (resize->z != 0) {
			size.z = resize->z / asp;
		}
	}

	// normalize normals, move
	for (size_t i = 0; i < msh->vtxcnt; i += 1) {
		GxMeshVtx vtx = msh->vtxptr + i;
		vecsub(&vtx->p, &vtx->p, &move);
		vecmul(&vtx->p, &vtx->p, &size);
		vtx->p.w = 1;
		vecnrm(&vtx->n, &vtx->n);
		vtx->n.w = 0;
	}
}
