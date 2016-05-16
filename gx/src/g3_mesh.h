#include "gx_surf.h"
#include "g3_math.h"

typedef struct texcol {
	union {
	struct {
		uint16_t s;
		uint16_t t;
	};
	struct {
		uint16_t s;
		uint16_t t;
	} tex;
	struct {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
	uint32_t val;
	argb rgb;
	};
} *texcol;

typedef struct Material {
	struct vector emis;		// Emissive
	struct vector ambi;		// Ambient
	struct vector diff;		// Diffuse
	struct vector spec;		// Specular
	scalar spow;			// Shininess
} *Material;

typedef struct Light {
	struct Light *next;
	struct Material mtl;
	//
	struct vector ambi;		// Ambient
	struct vector diff;		// Diffuse
	struct vector spec;		// Specular
	struct vector attn;		// Attenuation
	struct vector pos;		// position
	struct vector dir;		// direction

	scalar sCos, sExp;			// spot light related
	enum {						// Type
		L_off  = 0x0000,		// light is off
		L_on   = 0x0001,		// light is on
		//~ L_attn = 0x0002,		// attenuate
		//~ L_dir  = 0x0004,		// is directional
		//~ L_spot = 0x0008,		// is directional
	} attr;
} *Light;

typedef struct mesh {
	Light lit;				// mesh lights?
	gx_Surf map;			// texture map
	struct Material mtl;	// back, fore?

	long maxvtx, vtxcnt;	// vertices
	long maxtri, tricnt;	// triangles
	long maxseg, segcnt;	// segments
	//size_t maxgrp, grpcnt;	// grouping
	struct vector *pos;		// (x, y, z, 1) position
	struct vector *nrm;		// (x, y, z, 0) normal
	struct texcol *tex;		// (s, t, 0, 0) tetxure|color
	//~ struct texcol *col;		// xrgb precalculated colors
	struct tri {			// triangle list
		// signed id;	// groupId
		long i1;
		long i2;
		long i3;
	} *triptr;
	struct seg {			// line list
		long p1;
		long p2;
	} *segptr;
	/* groups
	struct grp {			// group list
		signed t1;			// first triangle in triptr
		signed t2;			// last triangle
		signed parent;		// parent group of group
		matrix transform;	// transformation matrix
		gx_Surf map;		// texture map
		Material mtl;		// back, fore?
	} *grpptr; */

	long hlplt;			// highlight
	int hasTex:1;// := map != null
	uint32_t hasNrm:1;// := nrm != null
	uint32_t freeTex:1;
	uint32_t padded:29;// padding
} *mesh;

void freeMesh(mesh msh);
long initMesh(mesh msh, long n);
int readMesh(mesh msh, const char* file);
int saveMesh(mesh msh, const char* file);

void sdivMesh(mesh msh, int smooth);
void optiMesh(mesh msh, scalar tol, int prgCB(float prec));

void bboxMesh(mesh msh, vector min, vector max);
void centMesh(mesh msh, scalar size);
void normMesh(mesh msh, scalar tol);
long setvtxDV(mesh msh, long idx, double pos[3], double nrm[3], double tex[2], long col);
long addvtxDV(mesh msh, double pos[3], double nrm[3], double tex[2]);

long addseg(mesh msh, long p1, long p2);
long addtri(mesh msh, long p1, long p2, long p3);
long addquad(mesh msh, long p1, long p2, long p3, long p4);

scalar backface(vector N, vector p1, vector p2, vector p3);

int evalMesh(mesh msh, int sdiv, int tdiv, char *src, char *file, int line);
