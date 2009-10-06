#ifndef __graph_h
#define __graph_h
#include <stdio.h>

//~ gx_initSurf flags	[do not alloc memory; do not clear allocated memory, alloc palette, alloc clip]
#define SURF_NO_MEM	0X0004
#define SURF_DNTCLR	0X8000
#define SURF_CPYPAL	0X0002
#define SURF_CPYCLP	0X0008

// read the cursor mask
#define CURS_GET 1
#define CURS_MIX 2
#define CURS_PUT 4
// hide : CURS_PUT
// show : CURS_MIX + CURS_PUT

//~ surface alloced flags	delete [clip; memory; palette]
#define SURF_DELCLP	0X0001
#define SURF_DELMEM	0X0002
#define SURF_DELLUT	0X0004

//~ surface read only flag
//~ if ((flags & 0XFF00) != 0) surface is RDONLY

//~ surface descript flags	rd only surfaces
#define SURF_RDONLY	0X1000
#define SURF_ID_GET	0X0F00
#define SURF_ID_CUR	0X0100
#define SURF_ID_FNT	0X0200
#define SURF_LAYERS	0X0300

typedef enum cblt_type {			// color bloc transfer
	cblt_conv = 0,
	cblt_copy = 1,
	cblt_fill = 2,
} cblt_type;

typedef enum cblt_mode {
	cblt_cpy = 0,	// or convert
	cblt_and = 1,
	cblt_or  = 2,
	cblt_xor = 3,
	cblt_near = 4,	// zoom linear
	cblt_blin = 5,	// zoom bilinear
} cblt_mode;

typedef struct gx_P2DS_t {			// 2d point type short
	unsigned short	x;	//(left)
	unsigned short	y;	//(top)
} gx_P2DS;

typedef struct gx_Clip_t {			// clipping bound structure [bounding box]
	union {unsigned short	l, xmin;};	//(left)
	union {unsigned short	t, ymin;};	//(top)
	union {unsigned short	r, xmax;};	//(right)
	union {unsigned short	b, ymax;};	//(bottom)
} gx_Clip;//, *gx_BBox;

typedef struct gx_Clut_t {			// Color Look Up Table (Palette) structure
	unsigned short	count;
	unsigned char	flags;
	unsigned char	trans;
	union gx_RGBQ {
		unsigned long col;
		struct {
			unsigned char	b;
			unsigned char	g;
			unsigned char	r;
			unsigned char	a;
		};
	} data[256];
} gx_Clut;

typedef union gx_RGBQ RGBQ;

typedef struct gx_Rect_t {
	signed long	x;
	signed long	y;
	union {
		signed long	w;
		signed long	width;
	};
	union {
		signed long	h;
		signed long	height;
	};
} gx_Rect;

typedef struct gx_Surf_t {			// surface structure
	union {
	gx_Clip*	clipPtr;
	gx_P2DS		hotSpot;
	};
	unsigned short	width;
	unsigned short	height;
	unsigned short	flags;			// img_fmt:8, mem_mgr:8
	unsigned char	depth;
	unsigned char	pixeLen;
	unsigned long	scanLen;		// scanlen / offsPos / imagecount
	void*		clutPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
	void*		basePtr;		// always base of surface
	//~ void*		offsPtr;		// offsPtr(scanLen, x, y):(void*)	parm[eax, ecx, edx] value [edx]
	//~ void*		movePtr;		// movePtr(offsPtr,  col):(void)	parm[edi, eax]
//------------------------------------------------ 32 bytes
	//~ void*	fillPtr;		// fillPtr(offsPtr, cnt, col):(void)	parm[edi, ecx, eax]
	//~ void*	resd[3];		// reserved
//------------------------------------------------ 48 bytes
} gx_Surf;

typedef struct gx_Llut_t {			// layer Look Up Table
	unsigned short	max_w;			// maximum width
	unsigned short	max_h;			// maximum height
	struct gx_FDIR {
		unsigned short	pad_x;
		unsigned short	pad_y;
		unsigned short	width;
		unsigned short	height;
		void*		basePtr;
	} data[256];
} gx_Llut;

typedef struct gx_Proc_t {			// process structure
	// start, halt, error, calculate precent done
	signed long	flag;			// -1 on exit	(name = "error message"), done = 0 => error;
						// -1 on exit	(name = "error message"), done = 255 => abort;
						//  0 on init	(name = "function name"); done = 0
						//  0 on done	(name = "function name"); done = 255
						//  1 on prec	(name = "function name"), done = 0..255 (done * 100./255 -> result in %)
						// writeback -1 for cancel
	const char*	name;
	signed long	size;
	signed long	done;			// 0..255 ()
	void (*cbfn)(struct gx_Proc_t*);	// CallBack function
} gx_Proc;

typedef void (*gx_callBack)(gx_Proc*);

typedef unsigned char pattern[8];

#pragma pack(1)

#if __cplusplus
extern "C"
{
#endif

extern int	gx_loadCUR(gx_Surf*, const char*, int);
//~ extern gx_Surf*	gx_openBMP(const char*, int depth);
extern int	gx_loadBMP(gx_Surf*, const char*, int depth);
extern int	gx_readBMP(gx_Surf*, FILE*, int depth);

extern int	gx_loadJPG(gx_Surf*, const char*, int);

extern int	gx_saveBMP(char*, gx_Surf*, int);
extern int	gx_loadFNT(gx_Surf*, const char*);
//~ extern int	gx_loadTTF(gx_Surf*, const char*, int);
extern int	gx_initFont(const char* fontfile, int fontsize);

extern void	gx_drawChar(gx_Surf *dst, int x, int y, gx_Surf *fnt, char chr, long col);
extern void	gx_drawText(gx_Surf *dst, int x, int y, gx_Surf *fnt, const char *str, long col);
extern void gx_clipText(gx_Rect *roi, gx_Surf *fnt, const char *str);

extern int	gx_initSurf(gx_Surf *surf, gx_Clip *clip, gx_Clut *clut, int flags);
extern void	gx_doneSurf(gx_Surf *surf);

extern gx_Surf*	gx_createSurf(unsigned width, unsigned height, unsigned depth, int flags);
extern gx_Surf*	gx_attachSurf(gx_Surf *surf, gx_Rect *Roi);
extern void gx_destroySurf(gx_Surf *surf);

//~ extern int	gx_locksurf(gx_Surf, long lock);
//~ extern long	gx_mapcolor(gx_Surf, long);				// Get Color in internal format

extern void*	gx_cliprect(gx_Surf*, gx_Rect*);
extern void*	gx_getpaddr(gx_Surf*, int, int);			// Get Pixel Address
extern long	gx_getpixel(gx_Surf*, int, int);			// Get Pixel
extern long	gx_getpnear(gx_Surf*, long, long);			// Get Pixel
extern long	gx_getpblin(gx_Surf*, long, long);			// Get Pixel
extern void	gx_setpixel(gx_Surf*, int, int, long);			// Set Pixel

//~ extern void	gx_sethline(gx_Surf, int, int, int, long);		// Horizontal Line
//~ extern void	gx_setvline(gx_Surf, int, int, int, long);		// Verical Line
extern void	gx_setblock(gx_Surf*, int, int, int, int, long);		// Block (bar)
//~ extern void	gx_setpline(gx_Surf, int, int, int, long, pattern);	// Pattern Line

typedef void (*cblt_proc)(void);
extern cblt_proc gx_getcbltf(cblt_type, int dstdepth, int opbpp);
extern void gx_callcbltf(cblt_proc, void*, void*, unsigned, void*);

//~ void gx_drawrect(gx_Surf dst, gx_Rect roi, color_t col);			fill rect
void gx_fillrect(gx_Surf *dst, gx_Rect *roi, long col);			//fill rect
//~ void gx_copyrect(gx_Surf dst, int x, int y, gx_Rect roi);			copy rect

//~ void gx_copysurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi=0);	copy surf
//~ void gx_zoomsurf(gx_Surf dst, gx_Rect drec, gx_Surf src, gx_Rect roi=0);	zoom surf
//~ void gx_masksurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi=0);	mask surf

/**	gx_clearsurf : clear surface
 * \param :	@arg : destination surface
 * 			@arg : array of rectangles
 * 			@arg : rectangle count
 * 			@arg : color
 * \note :	if (rect == NULL) clear the entire surface
 * \return : @nothing
**/

extern int gx_fillsurf(gx_Surf *dst, gx_Rect *roi, cblt_mode mode, long col);
//~ extern void	gx_fillsurf(gx_Surf *dst, gx_Rect *roi, cblt_mode mode, int xxx);
//~ extern void	gx_fillsurf(gx_Surf dst, gx_Rect roi, long src, int xxx);
//~ extern void	gx_copysurf(gx_Surf dst, gx_Rect roi, gx_Surf src, int xxx);

//~ extern void	gx_copysurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi);

extern int	gx_copysurf(gx_Surf *dst, int x, int y, gx_Surf *src, gx_Rect *roi, cblt_mode);
extern void	gx_zoomsurf(gx_Surf *dst, gx_Rect *rect, gx_Surf *src, gx_Rect *roi, int lin);
extern void	gx_drawCurs(gx_Surf *dst, int x, int y, gx_Surf *cur, int hide);


void g2_drawline(gx_Surf *dst, int x0, int y0, int x1, int y1, long col);
void gx_drawrect(gx_Surf *dst, int x0, int y0, int x1, int y1, long col);
void gx_drawoval(gx_Surf *dst, int x0, int y0, int x1, int y1, long col);
void gx_drawbez2(gx_Surf *dst, int x0, int y0, int x1, int y1, int x2, int y2, long col);
void gx_drawbez3(gx_Surf *dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col);

inline gx_Clip const * const gx_getclip(gx_Surf *s) {return s->clipPtr ? s->clipPtr : (gx_Clip*)s;}

extern unsigned char gx_buff[];
extern pattern gx_patt[41];

#if __cplusplus
}
#endif

#endif
