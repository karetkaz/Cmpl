#include "g2_surf.h"

#include "vecmat.h"
#define toRad(__A) ((__A) * (3.14159265358979323846 / 180))

enum {
	//~ draw_none = 0x00000000,		//
	draw_plot = 0x00000001,		//
	draw_wire = 0x00000002,		//
	draw_fill = 0x00000003,		//
	draw_mode = 0x00000003,		// mask

	zero_cbuf = 0x00000004,		// clear color buffer
	zero_zbuf = 0x00000008,		// clear z buffer

	//~ cull_none = 0x00000000,		// no cull
	cull_back = 0x00000010,		// front face
	cull_front= 0x00000020,		// back face
	cull_all  = 0x00000030,		// back & front face
	cull_mode = 0x00000030,		// enable culling

	draw_tex  = 0x00000040,		// use texture
	draw_lit  = 0x00000080,		// use lights
	disp_info = 0x00010000,
	disp_oxyz = 0x00020000,
	disp_norm = 0x00040000,		// normals
	disp_bbox = 0x00080000,		// Bounding Box
	temp_lght = 0x00100000,		// lights
	temp_zbuf = 0x00200000,

	swap_buff = 0x01000000,
	post_swap = 0x02000000,		// post swap_buff,
	//~ OnDemand =  0x02000000,
};

typedef union texcol {
	struct {
		unsigned short s;
		unsigned short t;
	};
	struct {
		unsigned short s;
		unsigned short t;
	} tex;
	struct {
		unsigned char b;
		unsigned char g;
		unsigned char r;
		unsigned char a;
	};
	unsigned long val;
	argb rgb;
} *texcol;

typedef struct Material {		// material
	union vector emis;		// Emissive
	union vector ambi;		// Ambient
	union vector diff;		// Diffuse
	union vector spec;		// Specular
	scalar spow;			// Shininess
} *Material;

typedef struct Light {		// lights
	enum {						// Type
		L_off  = 0x0000,		// light is off
		L_on   = 0x0001,		// light is on
		//~ L_attn = 0x0002,		// attenuate
		//~ L_dir  = 0x0004,		// is directional
		//~ L_spot = 0x0008,		// is directional
	} attr;
	union vector	ambi;		// Ambient
	union vector	diff;		// Diffuse
	union vector	spec;		// Specular
	union vector	attn;		// Attenuation
	union vector	pos;		// position
	union vector	dir;		// direction
	scalar	sCos;
	scalar	sExp;
	struct Light	*next;
	struct Material mtl;
} *Light;

typedef struct mesh {
	Light lit;				// mesh lights
	gx_Surf map;		// texture map
	struct Material mtl;	// back, fore?

	/*struct mat {
		struct gx_Surf img;
		struct Material mtl;
	} *mtlptr;// */

	signed maxvtx, vtxcnt;	// vertices
	signed maxtri, tricnt;	// triangles
	signed maxseg, segcnt;	// segments
	signed maxgrp, grpcnt;	// grouping
	union vector *pos;		// (x, y, z, 1) position
	union vector *nrm;		// (x, y, z, 0) normal
	union texcol *tex;		// (s, t, 0, 0) tetxure|color
	//~ union texcol *col;		// xrgb precalculated colors
	struct tri {			// triangle list
		// signed id;	// groupId
		signed i1;
		signed i2;
		signed i3;
	} *triptr;
	struct seg {			// line list
		signed p1;
		signed p2;
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

	signed hlplt;			// highlight
	int hasTex:1;// := map != null
	int hasNrm:1;// := nrm != null
	int freeTex:1;
	int padded:29;// padding
} *mesh;

// Utils
char* fext(const char* name);
char* readInt(char *ptr, int *outVal);
char* readFlt(char *str, double *outVal);
char* readF32(char *str, float *outVal);
char* readKVP(char *ptr, char *cmd, char *skp, char *sep);
char* readCmd(char *ptr, char *cmd) ;
//~ char* readVec(char *str, vector dst, scalar defw);

extern int g3_init(gx_Surf offs, int w, int h);
//~ extern void (*g3_drawline)(gx_Surf dst, vector p1, vector p2, long c);
//~ extern void g3_drawlineA(gx_Surf dst, vector p1, vector p2, long c);
extern void g3_drawline(gx_Surf dst, vector p1, vector p2, long c);
extern int g3_drawmesh(gx_Surf dst, mesh msh, matrix objm, camera cam, int mode, double norm);
extern int g3_drawbbox(gx_Surf dst, mesh msh, matrix objm, camera cam);
int g3_drawenvc(gx_Surf dst, struct gx_Surf img[6], vector view, matrix proj, double size);
void g3_drawOXYZ(gx_Surf dst, camera cam, double n);

int freeMesh(mesh msh);
int initMesh(mesh msh, int n);
int readMesh(mesh msh, const char* file);
int saveMesh(mesh msh, const char* file);
//int evalMesh(mesh msh, char *obj, int sdiv, int tdiv);
int evalMesh(mesh msh, int sdiv, int tdiv, char *src, char *file, int line);
int evalSphere(mesh msh, int sdiv, int tdiv);

void sdivMesh(mesh msh, int smooth);
void optiMesh(mesh msh, scalar tol, int prgCB(float prec));

void bboxMesh(mesh msh, vector min, vector max);
void centMesh(mesh msh, scalar size);
void normMesh(mesh msh, scalar tol);
int setvtxDV(mesh msh, int idx, double pos[3], double nrm[3], double tex[2], long col);

int addseg(mesh msh, int p1, int p2);
int addtri(mesh msh, int p1, int p2, int p3);
int addquad(mesh msh, int p1, int p2, int p3, int p4);

scalar backface(vector N, vector p1, vector p2, vector p3);
