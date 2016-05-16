#include "gx_surf.h"
#include "g3_mesh.h"

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
	disp_norm = 0x00040000,		// normals
	disp_bbox = 0x00080000,		// Bounding Box
	temp_lght = 0x00100000,		// lights
	temp_zbuf = 0x00200000,
	temp_debug= 0x00400000,		// print positions

	swap_buff = 0x01000000,
	post_swap = 0x02000000,		// post swap_buff,
	//~ OnDemand =  0x02000000,
};

void g3_drawline(gx_Surf dst, vector p1, vector p2, uint32_t c);

int g3_drawmesh(gx_Surf dst, mesh msh, matrix objm, camera cam, int mode, double norm);

int g3_drawbbox(gx_Surf dst, mesh msh, matrix objm, camera cam);

int g3_drawenvc(gx_Surf dst, struct gx_Surf img[6], vector view, matrix proj, double size);

void g3_drawOXYZ(gx_Surf dst, camera cam, double n);

scalar backface(vector N, vector p1, vector p2, vector p3);
