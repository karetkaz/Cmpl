#include "gx_surf.h"
#include "g3_math.h"

enum {
	draw_fill = 0x00000001,		// draw filled mesh
	draw_wire = 0x00000002,		// draw wireframe
	draw_plot = 0x00000004,		// draw points
	draw_mode = 0x00000007,		// mask

	keep_buff = 0x00000008,		// keep color and depth buffer

	cull_back = 0x00000010,		// cull back faces
	cull_front= 0x00000020,		// cull front faces
	cull_mode = 0x00000030,		// enable culling

	draw_tex  = 0x00000040,		// use texture
	draw_lit  = 0x00000080,		// use lights
	draw_box  = 0x00000100,		// draw bounding box
};

typedef struct GxLight {
	struct GxLight *next;

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
} *GxLight;

typedef struct GxMesh {
	// material
	struct GxMeshMtl {
		GxImage texture;		// texture map
		struct vector emis;		// Emissive
		struct vector ambi;		// Ambient
		struct vector diff;		// Diffuse
		struct vector spec;		// Specular
		scalar spow;			// Shininess
	} mtl;

	// vertices
	size_t maxvtx, vtxcnt;
	struct GxMeshVtx {
		struct vector p; // (x, y, z, 1) position
		struct vector n; // (x, y, z, 0) normal
		struct {
			uint16_t s;
			uint16_t t;
		};
	} *vtxptr;

	// triangles
	size_t maxtri, tricnt;
	struct GxMeshTri {
		size_t i1;
		size_t i2;
		size_t i3;
	} *triptr;

	// polygons
	size_t maxseg, segcnt;
	struct GxMeshSeg {
		size_t p1;
		size_t p2;
	} *segptr;

	/* groups
	size_t maxgrp, grpcnt;
	struct GxMeshGrp {
		signed t1;			// first triangle
		signed t2;			// last triangle
		signed parent;		// parent group of group
		matrix transform;	// transformation matrix
		GxImage map;		// texture map
		struct GxMeshMtl mtl;	// back, fore?
	} *grpptr;
	// */

	uint32_t freeMesh:1;
	uint32_t freeMap:1;
	uint32_t hasTex:1;
	uint32_t hasNrm:1;
} *GxMesh;

typedef struct GxMeshVtx *GxMeshVtx;
//typedef struct GxMeshTri *GxMeshTri;
//typedef struct GxMeshSeg *GxMeshSeg;

GxMesh createMesh(GxMesh recycle, size_t n);
void destroyMesh(GxMesh msh);

int readObj(GxMesh msh, const char *file);
int read3ds(GxMesh msh, const char *file);
int saveObj(GxMesh msh, const char *file);

/**
 * get the axis aligned bounding box of the mesh
 * @param msh mesh
 * @param min output vector containing the min values (x, y, z)
 * @param max output vector containing the max values (x, y, z)
 */
void bboxMesh(GxMesh msh, vector min, vector max);

/**
 * recalculate mesh normals
 * @param msh mesh
 * @param tol tolerance
 * @param center (optional) translate the mesh
 * @param resize (optional) resize the mesh
 */
void normMesh(GxMesh msh, scalar tol, vector center, vector resize);

/**
 * update one vertex of the mesh at the given index
 * @param msh mesh to be updated
 * @param idx index of vertex
 * @param pos update position
 * @param nrm update normal
 * @param tex update texture position
 * @return 0 if failed
 */
int setVtx(GxMesh msh, size_t idx, scalar *pos, scalar *nrm, scalar *tex);

/**
 * Add a new vertex to the mesh
 * @param msh mesh to be updated
 * @param pos update position
 * @param nrm update normal
 * @param tex update texture position
 * @return 0 if failed
 */
static inline int addVtx(GxMesh msh, scalar *pos, scalar *nrm, scalar *tex) {
	return setVtx(msh, msh->vtxcnt, pos, nrm, tex);
}

/**
 * connect 2 points with a segment (line) in the mesh
 * @param msh mesh to be updated
 * @param p1 index of the first vertex
 * @param p2 index of the second vertex
 * @return 0 if not possible
 */
int addSeg(GxMesh msh, size_t p1, size_t p2);

/**
 * connect 3 points (triangle) in the mesh
 * @param msh mesh to be updated
 * @param p1 index of the first vertex
 * @param p2 index of the second vertex
 * @param p3 index of the third vertex
 * @return 0 if not possible
 */
int addTri(GxMesh msh, size_t p1, size_t p2, size_t p3);

/**
 * connect 4 points (add 2 triangles) in the mesh
 * @param msh mesh to be updated
 * @param p1 index of the first vertex
 * @param p2 index of the second vertex
 * @param p3 index of the third vertex
 * @param p4 index of the fourth vertex
 * @return 0 if not possible
 */
static inline int addQuad(GxMesh msh, size_t p1, size_t p2, size_t p3, size_t p4) {
	if (!addTri(msh, p1, p2, p3)) {
		return 0;
	}
	return addTri(msh, p1, p3, p4);
}

int drawMesh(GxImage dst, GxMesh msh, camera cam, GxLight lights, int mode);

// extract the planes (near, far, left, right, top, bottom) from the projection matrix
void getFrustum(struct vector planes[6], matrix mat);

// test if a sphere or triangle is inside or outside the frustum (to test a point use a sphere wit 0 radius)
int testSphere(struct vector planes[6], vector p, scalar r);
int testTriangle(struct vector planes[6], vector p1, vector p2, vector p3);
