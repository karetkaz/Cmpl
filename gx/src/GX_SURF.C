#include <malloc.h>
#include <string.h>
#include "gx_surf.h"
#include "gx_color.h"
#include <stdio.h>

extern void colcpy_32cpy();
extern void colcpy_32mix();

int gx_initSurf(gx_Surf s, gx_Clip clip, gx_Clut clut, int flags) {
	int	tmp = s->depth; s->flags = 0;
	s->pixeLen = tmp == 8 ? 1 : tmp == 15 ? 2 : tmp == 16 ? 2 : tmp == 24 ? 3 : tmp == 32 ? 4 : 0;
	if (!s->scanLen) s->scanLen = s->pixeLen * s->width;
	if ((flags & SURF_NO_MEM) == 0) {
		tmp = s->scanLen * s->height;
		if ((s->basePtr = malloc(tmp))) {
			if ((flags & SURF_DNTCLR) == 0)
				memset(s->basePtr, 0, tmp);
			s->flags |= SURF_DELMEM;
		} else return -1;
	} else s->basePtr = 0;
	if (clut && (flags & SURF_CPYPAL)) {
		if ((s->clutPtr = malloc(sizeof(struct gx_Clut_t)))) {
			memcpy(s->clutPtr, clut, sizeof(struct gx_Clut_t));
			s->flags |= SURF_DELLUT;
		} else return -2;
	} else s->clutPtr = clut;
	if (clip && (flags & SURF_CPYCLP)) {
		if ((s->clipPtr = (gx_Clip)malloc(sizeof(struct gx_Clip_t)))) {
			memcpy(s->clipPtr, clip, sizeof(struct gx_Clip_t));
			s->flags |= SURF_DELCLP;
		} else return -3;
	} else s->clipPtr = clip;
	//~ s->movePtr = s->offsPtr = 0;
	gx_initdraw(s, rop2_cpy);
	return 0;
}

void gx_doneSurf(gx_Surf surf) {
	if (surf->clipPtr && (surf->flags & SURF_DELCLP)) free(surf->clipPtr);
	if (surf->basePtr && (surf->flags & SURF_DELMEM)) free(surf->basePtr);
	if (surf->clutPtr && (surf->flags & SURF_DELLUT)) free(surf->clutPtr);
}

gx_Surf gx_createSurf(unsigned width, unsigned height, unsigned depth, int flags) {
	gx_Surf surf = (gx_Surf)malloc(sizeof(struct gx_Surf_t));
	if (surf) {
		surf->width = width;
		surf->height = height;
		surf->depth = depth;
		surf->scanLen = 0;
		if (gx_initSurf(surf, 0, 0, flags)) {
			free(surf);
			return 0;
		}
	}
	return surf;
}

gx_Surf gx_attachSurf(gx_Surf src, gx_Rect roi) {
	struct gx_Rect_t clip;
	void*	basePtr;
	gx_Surf surf = 0;
	if (roi == NULL) {
		clip.x = clip.y = 0;
		clip.w = src->width;
		clip.h = src->height;
	} else clip = *roi;
	if ((basePtr = gx_cliprect(src, &clip))) {
		if ((surf = (gx_Surf)malloc(sizeof(struct gx_Surf_t)))) {
			*surf = *src;
			surf->basePtr = basePtr;
			surf->flags &= ~(SURF_DELCLP | SURF_DELMEM | SURF_DELLUT);
		}
	}
	return surf;
}

void gx_destroySurf(gx_Surf surf) {
	if (surf->clipPtr && (surf->flags & SURF_DELCLP)) free(surf->clipPtr);
	if (surf->basePtr && (surf->flags & SURF_DELMEM)) free(surf->basePtr);
	if (surf->clutPtr && (surf->flags & SURF_DELLUT)) free(surf->clutPtr);
	free(surf);
}

//~ void gx_copysurf(gx_Surf dst, gx_Rect drec, gx_Surf src, gx_Rect srec);
//~ void gx_fillrect(gx_Surf dst, gx_Rect roi, color_t col) {}
//~ void gx_copyrect(gx_Surf dst, int x, int y, gx_Rect roi) {}

void gx_copysurf1(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi) {
	gx_Clip clp = dst->clipPtr ? dst->clipPtr : (gx_Clip)dst;
	struct gx_Rect_t rect;
	char *dptr, *sptr;
	register int tmp;
	void (*cblt)() = 0;
	if (!roi) {
		rect.x = rect.y = 0;
		rect.w = src->width;
		rect.h = src->height;
	} else {
		rect = *roi;
		if (rect.x < 0) {
			x -= rect.x;
			rect.w += rect.x;
			rect.x = 0;
		} if (rect.w + rect.x > src->width) rect.w = src->width - rect.x;
		if (rect.y < 0) {
			y -= rect.y;
			rect.h += rect.y;
			rect.y = 0;
		} if (rect.h + rect.y > src->height) rect.h = src->height - rect.y;
	}
	if ((tmp = x - clp->l) < 0) {
		x = clp->l;
		rect.x -= tmp;
		rect.w += tmp;
	}
	if ((tmp = clp->r - x) < rect.w) rect.w = tmp;
	if ((tmp = y - clp->t) < 0) {
		y = clp->t;
		rect.y -= tmp;
		rect.h += tmp;
	}
	if ((tmp = clp->b - y) < rect.h) rect.h = tmp;
	if (rect.w > 0 && rect.h > 0) {
		dptr = (char*)gx_getpaddr(dst, x, y);
		sptr = (char*)gx_getpaddr(src, rect.x, rect.y);
		cblt = gx_getcbltf(cblt_conv, dst->depth, src->depth);
	}
	if (dptr == 0 || sptr == 0 || cblt == 0) return;
	while (rect.h--) {
		gx_callcbltf(cblt, dptr, sptr, rect.w, 0);
		dptr += dst->scanLen;
		sptr += src->scanLen;
	}
}

void gx_copysurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi) {
	struct gx_Rect_t clip;
	char *dptr, *sptr;
	void (*cblt)() = 0;
	clip.x = x; clip.w = src->width;
	clip.y = y; clip.h = src->height;
	if (roi != NULL) {
		if (roi->x > 0) {
			x -= roi->x;
			clip.w -= roi->x;
		}
		if (roi->y > 0) {
			y -= roi->y;
			clip.h -= roi->y;
		}
		if (roi->w < clip.w) clip.w = roi->w;
		if (roi->h < clip.h) clip.h = roi->h;
	}
	if(clip.w <= 0 || clip.h <= 0) return;
	dptr = (char*)gx_cliprect(dst, &clip);
	sptr = (char*)gx_getpaddr(src, clip.x - x, clip.y - y);
	cblt = gx_getcbltf(cblt_conv, dst->depth, src->depth);
	if (dptr == 0 || sptr == 0 || cblt == 0) return;
	while (clip.h--) {
		gx_callcbltf(cblt, dptr, sptr, clip.w, 0);
		dptr += dst->scanLen;
		sptr += src->scanLen;
	}
}

extern void callcolcpy(void colcpy(void), void* dst, void* src, unsigned cnt);
#pragma aux callcolcpy\
		parm  [eax] [edi] [esi] [ecx]\
		modify [eax edx ecx esi edi]\
		value [] = "call ebx";

extern long mixpix(void fn(void), long col2, long col1, long alpha8);
extern void setpix(void fn(void), void* dst, long col);

#pragma aux setpix parm [edx] [edi] [eax] modify [edx] = "call edx";
#pragma aux mixpix parm [ebx] [eax] [edx] [ecx] modify [edx] value [eax] = "call ebx";
//~ #pragma aux sumpix parm [ebx] [esi] [edx] [ecx] modify [edx ecx] value [eax] = "call ebx";

#define bilinear

void gx_zoomsurf(gx_Surf dst, gx_Rect rect, gx_Surf src, gx_Rect roi, int lin) {
	struct gx_Rect_t drec, srec;
	//~ void (*mixpfn)(void)=0;
	void (*setpfn)(void)=0;
	long (*getpix)(gx_Surf, long, long);
	long dx, dy, sx, sy, y;
	char *d, *dptr;
	srec.x = srec.y = 0;
	srec.w = src->width;
	srec.h = src->height;
	if (roi) {
		if (roi->x > 0) {
			srec.w -= roi->x;
			srec.x = roi->x;
		}
		if (roi->y > 0) {
			srec.h -= roi->y;
			srec.y = roi->y;
		}
		if (roi->w < srec.w) srec.w = roi->w;
		if (roi->h < srec.h) srec.h = roi->h;
	}
	if (rect == 0) {
		drec.x = drec.y = 0;
		drec.w = dst->width;
		drec.h = dst->height;
	} else drec = *rect;
	switch (dst->depth) {
		extern void setpix_32(void);
		extern void setpix_24(void);
		extern void setpix_16(void);
		extern void setpix_08(void);

		case 32 : setpfn = setpix_32; break;
		case 24 : setpfn = setpix_24; break;
		case 16 : setpfn = setpix_16; break;
		case  8 : setpfn = setpix_08; break;
		default : return;
	}
	if (drec.w <= 0 || drec.h <= 0) return;
	if (srec.w <= 0 || srec.h <= 0) return;
	if (!(dptr = (char*)gx_cliprect(dst, &drec))) return;
	dx = (srec.w - 1 << 16) / drec.w;
	dy = (srec.h - 1 << 16) / drec.h;

	if (dx & 0xfffe0000 || dy & 0xfffe0000) {
		getpix = gx_getpnlin;
	}
	else getpix = lin ? gx_getpnlin : gx_getpnear;

	for (y=0,sy = srec.y << 16; drec.h; --drec.h,++y) {
		long x = 0;
		for (d = dptr, sx = srec.x << 16; x < drec.w; ++x) {
			//~ long c00 = gx_getpnear(src, sx, sy);
			//~ long c00 = getpix(src, sx, sy);
			setpix(setpfn, d, getpix(src, sx, sy));
			d += dst->pixeLen; sx += dx;
		}
		dptr += dst->scanLen; sy += dy;
	}
}

void gx_drawCurs(gx_Surf dst, int x, int y, gx_Surf cur, int hide) {
	char *d, *s, *dptr, *sptr, tmp[32*32*4];		// 4096 bytes
	struct gx_Rect_t clp;
	void (*rdbfn)();
	void (*wtbfn)();

	if (cur->flags & SURF_ID_GET != SURF_ID_CUR) return;

	rdbfn = gx_getcbltf(cblt_conv, 32, dst->depth);
	wtbfn = gx_getcbltf(cblt_conv, dst->depth, 32);
	if (rdbfn == 0 || wtbfn == 0) return;

	x -= cur->hotSpot.x;
	y -= cur->hotSpot.y;
	clp.x = x; clp.y = y;
	clp.w = cur->width;
	clp.h = cur->height;

	dptr = (char*)gx_cliprect(dst, &clp);
	sptr = (char*)gx_getpaddr(cur, clp.x - x, clp.y - y);
	if (clp.w <= 0 || clp.h <= 0) return;
	if (dptr == 0  ||  sptr == 0) return;

	if (hide & 1) for (d = dptr, s = sptr, y = 0; y < clp.h; ++y) {
		gx_callcbltf(rdbfn, s, d, clp.w, 0);
		d += dst->scanLen; s += cur->scanLen;
	}
	if (hide & (2|4)) {
		if (hide & 2) {
			memcpy(tmp, cur->basePtr, 32*32*4);
			gx_callcbltf(colcpy_32mix, tmp, cur->clutPtr, 32*32, (void*)0);
			sptr = tmp + (sptr - (char*)cur->basePtr);
		}
		for (d = dptr, s = sptr, y = 0; y < clp.h; ++y) {
			gx_callcbltf(wtbfn, d, s, clp.w, 0);
			d += dst->scanLen; s += cur->scanLen;
		}
	}
}
//~ void gx_copySurf();
//~ void gx_copyRect();

int  gx_loadFNT(gx_Surf dst, const char* fontfile) {
	gx_Llut lut;
	char tmp[4096], *ptr;
	int fsize, i;
	FILE *fin = fopen(fontfile, "rb");
	if (fin == 0) return -1;

	fread(tmp, 4096, 1, fin);
	fsize = ftell(fin);
	fclose(fin);
	dst->flags = SURF_ID_FNT | SURF_DELMEM | SURF_DELLUT;
	if(!(dst->clutPtr = malloc(sizeof(struct gx_Llut_t)))) {
		return -2;
	}
	if(!(dst->basePtr = malloc(fsize*8))) {
		free(dst->clutPtr);
		return -3;
	}
	ptr = (char*)dst->basePtr;
	for (i = 0; i < fsize; ++i, ptr += 8) {
		ptr[0] = (tmp[i] & 0X80) ? 255 : 0;
		ptr[1] = (tmp[i] & 0X40) ? 255 : 0;
		ptr[2] = (tmp[i] & 0X20) ? 255 : 0;
		ptr[3] = (tmp[i] & 0X10) ? 255 : 0;
		ptr[4] = (tmp[i] & 0X08) ? 255 : 0;
		ptr[5] = (tmp[i] & 0X04) ? 255 : 0;
		ptr[6] = (tmp[i] & 0X02) ? 255 : 0;
		ptr[7] = (tmp[i] & 0X01) ? 255 : 0;
	}
	lut = (gx_Llut)dst->clutPtr;
	lut->max_w = 8;
	lut->max_h = fsize >> 8;
	fsize = lut->max_w * lut->max_h;
	ptr = (char*)dst->basePtr;
	for (i = 0; i < 256; ++i, ptr += fsize) {
		lut->data[i].pad_x = 0;
		lut->data[i].pad_y = 0;
		lut->data[i].width = lut->max_w;
		lut->data[i].height = lut->max_h;
		lut->data[i].basePtr = ptr;
	}
	dst->width = 8;
	dst->height = 256 * lut->max_h;
	dst->clipPtr = 0;
	dst->depth = 8;
	dst->pixeLen = 1;
	dst->scanLen = 8;
	gx_initdraw(dst, rop2_cpy);
	
	//~ dst->clutPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
	//~ dst->basePtr;		// always base of surface
	//~ dst->offsPtr;		// offsPtr(scanLen, x, y):(void*)	parm[eax, ecx, edx] value [edx]
	//~ dst->movePtr;		// movePtr(offsPtr,  col):(void)	parm[edi, eax]
	return 0;
}

void gx_drawChar1(gx_Surf dst, int x, int y, gx_Surf fnt, char chr, long col) {
	gx_Llut  lut = (gx_Llut)fnt->clutPtr;
	//struct gx_Surf_t face = *fnt;
	struct gx_Rect_t clip;
	char *dptr, *sptr=0;
	void (*cblt)() = 0;
	//~ clip.x = face.hotSpot.x = lut->data[(unsigned)chr].pad_x + x;
	//~ clip.y = face.hotSpot.y = lut->data[(unsigned)chr].pad_y + y;
	//~ clip.w = face.width = lut->data[(unsigned)chr].width;
	//~ clip.h = face.height = lut->data[(unsigned)chr].height;
	//~ face.basePtr = lut->data[(unsigned)chr].basePtr;
	//~ face.scanLen = face.width; face.pixeLen = 1;
	clip.x = lut->data[(unsigned)chr].pad_x + x;
	clip.y = lut->data[(unsigned)chr].pad_y + y;
	clip.w = lut->data[(unsigned)chr].width;
	clip.h = lut->data[(unsigned)chr].height;
	if(clip.w <= 0 || clip.h <= 0) return;

	dptr = (char*)gx_cliprect(dst, &clip);
	//~ sptr = (char*)gx_getpaddr(&face, clip.x - x, clip.y - y);
	
	cblt = gx_getcbltf(cblt_conv, dst->depth, 8);
	if (dptr == 0 || sptr == 0 || cblt == 0) return;
	while (clip.h--) {
		gx_callcbltf(cblt, dptr, sptr, clip.w, (void*)col);
		dptr += dst->scanLen;
		//~ sptr += face.scanLen;
	}
	
}

void gx_drawChar(gx_Surf dst, int x, int y, gx_Surf fnt, char chr, long col) {
	struct gx_FDIR face = (*(gx_Llut)fnt->clutPtr).data[(unsigned)chr];
	struct gx_Rect_t clip;
	void (*cblt)() = 0;
	char *dptr, *sptr;

	clip.x = face.pad_x += x;
	clip.y = face.pad_y += y;
	clip.w = face.width;
	clip.h = face.height;
	if(clip.w <= 0 || clip.h <= 0) return;

	if (!(dptr = (char*)gx_cliprect(dst, &clip))) return;
	sptr = (char*)face.basePtr + (clip.y - face.pad_y) * face.width + (clip.x - face.pad_x);
	switch (dst->depth) {
		extern void colset_32mix(void);
		extern void colset_24mix(void);
		extern void colset_16mix(void);
		extern void colset_08mix(void);

		case 32 : cblt = colset_32mix; break;
		default : return;
	}
	while (clip.h--) {
		gx_callcbltf(cblt, dptr, sptr, clip.w, (void*)col);
		dptr += dst->scanLen;
		sptr += face.width;
	}
}

void gx_drawText(gx_Surf dst, int x, int y, gx_Surf fnt, const char *str, long col) {
	char chr;
	while ((chr = *str++)) {
		gx_drawChar(dst, x, y, fnt, chr, col);
		x += ((gx_Llut)fnt->clutPtr)->data[(unsigned)chr].width;
	}
}

unsigned char gx_buff[65536*4];
pattern	gx_patt[]={
	{0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00},	//  0 |	====	line
	{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},	//  1 |	 / /	liteslash
	{0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc1, 0x83},	//  2 |	////	slash
	{0x07, 0x83, 0xc1, 0xe0, 0x70, 0x38, 0x1c, 0x0e},	//  3 |	\\\\	bkslash
	{0x5a, 0x2d, 0x96, 0x4b, 0xa5, 0xd2, 0x69, 0xb4},	//  4 |	 \ \	litebkslash
	{0xff, 0x88, 0x88, 0x88, 0xff, 0x88, 0x88, 0x88},	//  5 |		hatch
	{0x18, 0x24, 0x42, 0x81, 0x81, 0x42, 0x24, 0x18},	//  6 |	xxxx	xhatch
	{0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33, 0xcc, 0x33},	//  7 |		interleave
	{0x80, 0x00, 0x08, 0x00, 0x80, 0x00, 0x08, 0x00},	//  8 |		widedot
	{0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00},	//  9 |		closedot
	{0xBB, 0x5F, 0xAE, 0x5D, 0xBA, 0x75, 0xEA, 0xF5},	// 10 | Bricks
	{0xAA, 0x7D, 0xC6, 0x47, 0xC6, 0x7F, 0xBE, 0x55},	// 11 | Buttons
	{0x78, 0x31, 0x13, 0x87, 0xe1, 0xc8, 0x8c, 0x1e},	// 12 | Cargo Net
	{0x52, 0x29, 0x84, 0x42, 0x94, 0x29, 0x42, 0x84},	// 13 | Circuits
	{0x28, 0x44, 0x92, 0xAB, 0xD6, 0x6C, 0x38, 0x10},	// 14 | Cobblestones
	{0x82, 0x01, 0x01, 0x01, 0xAB, 0x55, 0xAA, 0x55},	// 15 | Colosseum
	{0x1E, 0x8C, 0xD8, 0xFD, 0xBF, 0x1B, 0x31, 0x78},	// 16 | Daisies
	{0x3E, 0x07, 0xE1, 0x07, 0x3E, 0x70, 0xC3, 0x70},	// 17 | Dizzy
	{0x56, 0x59, 0xA6, 0x9A, 0x65, 0x95, 0x6A, 0xA9},	// 18 | Field Effect
	{0xFE, 0x02, 0xFA, 0x8A, 0xBA, 0xA2, 0xBE, 0x80},	// 19 | Key
	{0xEF, 0xEF, 0x0E, 0xFE, 0xFE, 0xFE, 0xE0, 0xEF},	// 20 | Live Wire
	{0xF0, 0xF0, 0xF0, 0xF0, 0xAA, 0x55, 0xAA, 0x55},	// 21 | Palid
	{0xD7, 0x93, 0x28, 0xD7, 0x28, 0x93, 0xD5, 0xD7},	// 22 | Rounder
	{0xE1, 0x2A, 0x25, 0x92, 0x55, 0x98, 0x3E, 0xF7},	// 23 | Scales
	{0xAE, 0x4D, 0xEF, 0xFF, 0x08, 0x4D, 0xAE, 0x4D},	// 24 | Stone
	{0xF8, 0x74, 0x22, 0x47, 0x8F, 0x17, 0x22, 0x71},	// 25 | Thatches
	{0x45, 0x82, 0x01, 0x00, 0x01, 0x82, 0x45, 0xAA},	// 26 | Tile
	{0x87, 0x07, 0x06, 0x04, 0x00, 0xF7, 0xE7, 0xC7},	// 27 | Triangles
	{0x4D, 0x9A, 0x08, 0x55, 0xEF, 0x9A, 0x4D, 0x9A},	// 28 | Waffl's Revenge
	{0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f, 0x00},	// 29
	{0x00, 0x50, 0x72, 0x20, 0x00, 0x05, 0x27, 0x02},	// 30
	{0x20, 0x50, 0x88, 0x50, 0x20, 0x00, 0x00, 0x00},	// 31
	{0x02, 0x07, 0x07, 0x02, 0x20, 0x50, 0x50, 0x20},	// 32
	{0xe0, 0x80, 0x8e, 0x88, 0xea, 0x0a, 0x0e, 0x00},	// 33
	{0x82, 0x44, 0x28, 0x11, 0x28, 0x44, 0x82, 0x01},	// 34
	{0x40, 0xc0, 0xc8, 0x78, 0x78, 0x48, 0x00, 0x00},	// 35
	{0x14, 0x0c, 0xc8, 0x79, 0x9e, 0x13, 0x30, 0x28},	// 36
	{0xf8, 0x74, 0x22, 0x47, 0x8f, 0x17, 0x22, 0x0b},	// 37
	{0x00, 0x00, 0x54, 0x7c, 0x7c, 0x38, 0x92, 0x0c},	// 38
	{0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0xf0},	// 39
	{0x88, 0x54, 0x22, 0x45, 0x88, 0x15, 0x22, 0x51}	// 40
};
