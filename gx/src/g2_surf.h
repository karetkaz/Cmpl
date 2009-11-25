#ifndef __graph_h
#define __graph_h
#include <stdio.h>

#define LOBIT(__VAL) ((__VAL) & -(__VAL))
#define debug(msg, ...) fprintf(stderr, "%s:%d:debug:"msg"\n", __FILE__, __LINE__, ##__VA_ARGS__)

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
//~ #define SURF_DELCLP	0X0001
//~ #define SURF_DELLUT	0X0004
#define SURF_DELMEM	0X0001

//~ surface read only flag
//~ if ((flags & 0XFF00) != 0) surface is RDONLY

//~ surface descript flags	rd only surfaces
#define SURF_RDONLY	0X1000
#define SURF_ID_GET	0X0F00
#define SURF_ID_CUR	0X0100
#define SURF_ID_FNT	0X0200
#define SURF_LAYERS	0X0300

typedef enum {			// color bloc transfer
	cblt_conv = 0,
	cblt_copy = 1,
	cblt_fill = 2,
} cblt_type;

typedef enum {
	cblt_cpy = 0,	// or convert
	cblt_and = 1,
	cblt_or  = 2,
	cblt_xor = 3,
	cblt_near = 4,	// zoom linear
	cblt_blin = 5,	// zoom bilinear
} cblt_mode;

typedef union {				// ARGB color structutre
	unsigned long val;
	unsigned long col;
	struct {
		unsigned char	b;
		unsigned char	g;
		unsigned char	r;
		unsigned char	a;
	};
	/*struct {
		unsigned char	b;
		unsigned char	g;
		unsigned char	r;
		unsigned char	a;
	} rgb;
	struct {
		unsigned char	k;
		unsigned char	y;
		unsigned char	m;
		unsigned char	c;
	} cmy;
	struct {
		unsigned char	l;
		unsigned char	s;
		unsigned short	h;
	} hsl;
	struct {
		unsigned char	v;
		unsigned char	s;
		unsigned short	h;
	} hsv;
	struct {
		unsigned char	c1;
		unsigned char	c2;
		unsigned short	y;
	} yiq;
	struct {
		unsigned char	u;
		unsigned char	v;
		unsigned short	y;
	} yuv;
	enum {
		cs_argb = 'argb',
		cs_cmyk = 'cmyk',
		cs_hsv  = '_hsv',
		cs_yuv  = '_yuv',
		//~ cs_xyz  = ' xyz',
		//~ cs_rgb  = ' rgb',
		//~ cs_cmy  = ' cmy',
		//~ cs_hsl  = ' hsl',
		//~ cs_yiq  = ' yiq',
	} argb_cs; // */
} argb;

typedef struct {			// 2d point type short
	unsigned short	x;	//(left)
	unsigned short	y;	//(top)
} gx_P2DS;

typedef struct gx_Clip {			// clipping bound structure [bounding box]
	union {unsigned short	l, xmin;};	//(left)
	union {unsigned short	t, ymin;};	//(top)
	union {unsigned short	r, xmax;};	//(right)
	union {unsigned short	b, ymax;};	//(bottom)
} *gx_Clip;

typedef struct gx_Clut {			// Color Look Up Table (Palette) structure
	unsigned short	count;
	unsigned char	flags;
	unsigned char	trans;
	argb	data[256];
	/*union gx_RGBQ {
		unsigned long col;
		struct {
			unsigned char	b;
			unsigned char	g;
			unsigned char	r;
			unsigned char	a;
		};
	} data[256]; // */
} *gx_Clut, *clut;

typedef union gx_RGBQ RGBQ;

typedef struct gx_Rect {
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
} *gx_Rect;

typedef struct gx_Llut {			// layer Look Up Table
	unsigned short	unused;			// font faces
	unsigned short	height;			// font height
	struct gx_FDIR {
		unsigned char	pad_x;
		unsigned char	pad_y;
		unsigned short	width;
		void*		basePtr;
	} data[256];
} *gx_Llut;

typedef struct gx_Surf {			// surface structure
	union {
		gx_Clip		clipPtr;
		gx_P2DS		hotSpot;
	};
	unsigned short	width;
	unsigned short	height;
	unsigned short	flags;			// img_fmt:8, mem_mgr:8
	unsigned char	depth;
	unsigned char	pixeLen;
	unsigned long	scanLen;		// scanlen / offsPos / imagecount
	void*			basePtr;		// always base of surface
	union {
		gx_Llut		lutLPtr;
		//~ gx_Clut		CLUTPtr;
		//~ gx_Llut		facePtr;
		void*		tempPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
		//~ void*		clutPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
	};
	//~ void*		offsPtr;		// offsPtr(scanLen, x, y):(void*)	parm[eax, ecx, edx] value [edx]
	//~ void*		movePtr;		// movePtr(offsPtr,  col):(void)	parm[edi, eax]
//------------------------------------------------ 32 bytes
	//~ void*	fillPtr;		// fillPtr(offsPtr, cnt, col):(void)	parm[edi, ecx, eax]
	//~ void*	resd[3];		// reserved
//------------------------------------------------ 48 bytes
} *gx_Surf;

typedef unsigned char pattern[8];

#pragma pack(1)

#if __cplusplus
#define gx_extern extern "C"
#else
#define gx_extern extern
#endif

int	gx_loadCUR(gx_Surf, const char*, int);
int gx_loadBMP(gx_Surf, const char*, int depth);
int gx_loadJPG(gx_Surf, const char*, int);

int gx_saveBMP(const char*, gx_Surf, int);
int gx_loadFNT(gx_Surf, const char*);
//~ int gx_loadTTF(gx_Surf, const char*, int height);

gx_Surf gx_createSurf(unsigned width, unsigned height, unsigned depth, int flags);
gx_Surf gx_attachSurf(gx_Surf surf, gx_Rect roi);
int gx_initSurf(gx_Surf surf, gx_Clip clip, gx_Clut clut, int flags);
void gx_doneSurf(gx_Surf surf);
void gx_destroySurf(gx_Surf surf);

gx_extern void* gx_cliprect(gx_Surf, gx_Rect);

gx_extern void* gx_getpaddr(gx_Surf, int, int);				// Get Pixel Address
gx_extern long gx_getpixel(gx_Surf, int, int);				// Get Pixel
gx_extern void gx_setpixel(gx_Surf, int, int, long);		// Set Pixel
gx_extern void gx_sethline(gx_Surf, int, int, int, long);		// Horizontal Line
gx_extern void gx_setvline(gx_Surf, int, int, int, long);		// Verical Line
gx_extern void gx_setblock(gx_Surf, int, int, int, int, long);		// Block (bar)
gx_extern void gx_setpline(gx_Surf, int, int, int, long, pattern);	// Pattern Line

gx_extern long gx_getpnear(gx_Surf, long, long);			// Get Pixel(fixed, fixed)
gx_extern long gx_getpblin(gx_Surf, long, long);			// Get Pixel(fixed, fixed)

typedef void (*cblt_proc)(void);
gx_extern cblt_proc gx_getcbltf(cblt_type, int dstdepth, int opbpp);
gx_extern void gx_callcbltf(cblt_proc, void*, void*, unsigned, void*);


int gx_fillsurf(gx_Surf dst, gx_Rect roi, cblt_mode mode, long col);
//~ void gx_copysurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi);
int gx_copysurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cblt_mode);
int gx_zoomsurf(gx_Surf dst, gx_Rect rect, gx_Surf src, gx_Rect roi, int lin);
//~ int gx_drawCurs(gx_Surf dst, int x, int y, gx_Surf cur, int hide);


void g2_drawline(gx_Surf dst, int x0, int y0, int x1, int y1, long col);
void gx_drawrect(gx_Surf dst, int x0, int y0, int x1, int y1, long col);
void gx_drawoval(gx_Surf dst, int x0, int y0, int x1, int y1, long col);
void gx_drawbez2(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, long col);
void gx_drawbez3(gx_Surf dst, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, long col);

void gx_clipText(gx_Rect roi, gx_Surf fnt, const char *str);
void gx_drawChar(gx_Surf dst, int x, int y, gx_Surf fnt, unsigned chr, long col);
void gx_drawText(gx_Surf dst, int x, int y, gx_Surf fnt, const char *str, long col);

inline gx_Clip const gx_getclip(gx_Surf s) {return s->clipPtr ? s->clipPtr : (gx_Clip)s;}

extern unsigned char gx_buff[];
extern pattern gx_patt[41];

#if __cplusplus
}
#endif

#endif
