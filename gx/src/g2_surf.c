#include <malloc.h>
#include <string.h>
#include "g2_surf.h"


int gx_initSurf(gx_Surf s, gx_Clip clip, gx_Clut clut, int flags) {
	int tmp = s->depth;

	s->flags = 0;
	s->pixeLen = tmp == 8 ? 1 : tmp == 16 ? 2 : tmp == 24 ? 3 : tmp == 32 ? 4 : 0;

	if (!s->scanLen)
		s->scanLen = s->pixeLen * s->width;

	if ((flags & SURF_NO_MEM) == 0) {

		tmp = s->scanLen * s->height;
		if ((s->basePtr = malloc(tmp))) {

			if ((flags & SURF_DNTCLR) == 0)
				memset(s->basePtr, 0, tmp);
			s->flags |= Surf_mem;
		}
		else
			return -1;
	}
	else
		s->basePtr = 0;
	/*if (clut && (flags & SURF_CPYPAL)) {
		if ((s->clutPtr = malloc(sizeof(struct gx_Clut)))) {
			memcpy(s->clutPtr, clut, sizeof(struct gx_Clut));
			s->flags |= SURF_DELLUT;
		} else return -2;
	} else s->clutPtr = clut;
	if (clip && (flags & SURF_CPYCLP)) {
		if ((s->clipPtr = (gx_Clip)malloc(sizeof(struct gx_Clip)))) {
			memcpy(s->clipPtr, clip, sizeof(struct gx_Clip));
			s->flags |= SURF_DELCLP;
		} else return -3;
	} else s->clipPtr = clip;
	//~ s->movePtr = s->offsPtr = 0;
	//~ gx_initdraw(s, rop2_cpy);*/
	return 0;
}

gx_Surf gx_createSurf(unsigned width, unsigned height, unsigned depth, int flags) {
	gx_Surf surf = (gx_Surf)malloc(sizeof(struct gx_Surf));
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

int gx_asubSurf(gx_Surf dst, gx_Surf src, gx_Rect roi) {
	char *basePtr;
	struct gx_Rect clip;

	if (roi == NULL) {
		clip.x = clip.y = 0;
		clip.w = src->width;
		clip.h = src->height;
	}
	else
		clip = *roi;

	if ((basePtr = gx_cliprect(src, &clip))) {
		dst->clipPtr = NULL;
		dst->width = clip.w;
		dst->height = clip.h;
		dst->flags = src->flags & ~Surf_mem;
		dst->depth = src->depth;
		dst->pixeLen = src->pixeLen;
		dst->scanLen = src->scanLen;
		dst->basePtr = basePtr;

		switch (src->flags & SurfMask) {
			case Surf_3ds:
				//~ dst->lutLPtr = (void*)(((char*)dst->lutLPtr) + ((char*)basePtr - (char*)src->basePtr));
				dst->tempPtr = basePtr + (src->tempPtr - src->basePtr);
				break;

			default:
				dst->tempPtr = src->tempPtr;
				break;
		}
	}
	return basePtr != NULL;
}

/*gx_Surf gx_attachSurf(gx_Surf src, gx_Rect roi) {
	struct gx_Rect clip;
	void*	basePtr;
	gx_Surf surf = 0;

	if (roi == NULL) {
		clip.x = clip.y = 0;
		clip.w = src->width;
		clip.h = src->height;
	}
	else
		clip = *roi;

	// TODO: use sub surf init
	if ((basePtr = gx_cliprect(src, &clip))) {
		if ((surf = (gx_Surf)malloc(sizeof(struct gx_Surf)))) {
			*surf = *src;
			surf->basePtr = basePtr;
			surf->flags &= ~Surf_mem;
		}
	}
	return surf;
}// */



void gx_doneSurf(gx_Surf surf) {
	if (surf->basePtr && (surf->flags & Surf_mem)) {
		free(surf->basePtr);
		surf->basePtr = 0;
	}
}

void gx_destroySurf(gx_Surf surf) {
	gx_doneSurf(surf);
	free(surf);
}



void* gx_cliprect(gx_Surf surf, gx_Rect roi) {
	gx_Clip clp = gx_getclip(surf);

	roi->w += roi->x;
	roi->h += roi->y;

	if (clp->l > roi->x)
		roi->x = clp->l;
	if (clp->t > roi->y)
		roi->y = clp->t;
	if (clp->r < roi->w)
		roi->w = clp->r;
	if (clp->b < roi->h)
		roi->h = clp->b;

	if ((roi->w -= roi->x) <= 0)
		return NULL;

	if ((roi->h -= roi->y) <= 0)
		return NULL;

	return gx_getpaddr(surf, roi->x, roi->y);
}



static inline int min(int a, int b) {return a < b ? a : b;}
static inline int max(int a, int b) {return a > b ? a : b;}
static inline long rch(long xrgb) {return xrgb >> 16 & 0xff;}
static inline long gch(long xrgb) {return xrgb >>  8 & 0xff;}
static inline long bch(long xrgb) {return xrgb >>  0 & 0xff;}
static inline long __rgb(int r, int g, int b) {return r << 16 | g << 8 | b;}

int gx_blursurf(gx_Surf img, gx_Rect roi, int radius) {

	int w = img->width;
	int h = img->height;
	int wm = w - 1;
	int hm = h - 1;

	int wh = w * h;
	int maxwh = max(w, h);
	int div = 2 * radius + 1;

	int x, y, i, yi = 0;
	int *r, *g, *b, *vmin, *vmax, *dv;

	if (radius < 1)
		return -1;

	if (roi != NULL)
		return -2;

	r = malloc(sizeof(int) * wh);
	g = malloc(sizeof(int) * wh);
	b = malloc(sizeof(int) * wh);
	vmin = malloc(sizeof(int) * maxwh);
	vmax = malloc(sizeof(int) * maxwh);
	dv = malloc(sizeof(int) * 256 * div);


	for (i = 0; i < 256 * div; i += 1) {
		dv[i] = i / div;
	}

	for (y = 0; y < h; y += 1) {
		int rsum = 0;
		int gsum = 0;
		int bsum = 0;

		for (i = -radius; i <= radius; i += 1) {
			int p = gx_getpixel(img, min(wm, max(i, 0)), y);
			rsum += rch(p);
			gsum += gch(p);
			bsum += bch(p);
		}

		for (x = 0; x < w; x += 1) {
			int p1, p2;
			r[yi] = dv[rsum];
			g[yi] = dv[gsum];
			b[yi] = dv[bsum];

			if (y == 0) {
				vmin[x] = min(x + radius + 1, wm);
				vmax[x] = max(x - radius, 0);
			}

			p1 = gx_getpixel(img, vmin[x], y);
			p2 = gx_getpixel(img, vmax[x], y);

			rsum += rch(p1) - rch(p2);
			gsum += gch(p1) - gch(p2);
			bsum += bch(p1) - bch(p2);

			yi += 1;
		}
	}

	for (x = 0; x < w; x += 1) {
		int rsum = 0;
		int gsum = 0;
		int bsum = 0;
		int yp = -radius * w;

		for (i = -radius; i <= radius; i += 1) {
			int xy = max(0, yp) + x;
			rsum += r[xy];
			gsum += g[xy];
			bsum += b[xy];
			yp += w;
		}

		for (y = 0; y < h; y += 1) {
			int p1, p2;
			gx_setpixel(img, x, y, __rgb(dv[rsum], dv[gsum], dv[bsum]));

			if (x == 0) {
				vmin[y] = min(y + radius + 1, hm) * w;
				vmax[y] = max(y - radius, 0) * w;
			}

			p1 = x + vmin[y];
			p2 = x + vmax[y];

			rsum += r[p1] - r[p2];
			gsum += g[p1] - g[p2];
			bsum += b[p1] - b[p2];
		}
	}

	free(r);
	free(g);
	free(b);
	free(vmin);
	free(vmax);
	free(dv);
	return 0;
}

int gx_clutsurf(gx_Surf dst, gx_Rect roi, gx_Clut lut) {
	argb* lutd = lut->data;
	struct gx_Rect rect;
	char *dptr;

	if (dst->depth != 32)
		return -1;

	rect.x = roi ? roi->x : 0;
	rect.y = roi ? roi->y : 0;
	rect.w = roi ? roi->w : dst->width;
	rect.h = roi ? roi->h : dst->height;

	if(!(dptr = gx_cliprect(dst, &rect)))
		return -2;

	while (rect.h--) {
		int x;
		argb *cBuff = (argb*)dptr;
		for (x = 0; x < rect.w; x += 1) {
			//~ cBuff->a = lutd[cBuff->a].a;
			cBuff->r = lutd[cBuff->r].r;
			cBuff->g = lutd[cBuff->g].g;
			cBuff->b = lutd[cBuff->b].b;
			cBuff += 1;
		}
		dptr += dst->scanLen;
	}

	return 0;
}

int gx_fillsurf(gx_Surf dst, gx_Rect roi, cblt_mode mode, long col) {
	register char *dptr;
	register void (*cblt)();
	struct gx_Rect clip;

	clip.x = roi ? roi->x : 0;
	clip.y = roi ? roi->y : 0;
	clip.w = roi ? roi->w : dst->width;
	clip.h = roi ? roi->h : dst->height;

	if(!(dptr = gx_cliprect(dst, &clip)))
		return -1;

	if(!(cblt = gx_getcbltf(cblt_fill, dst->depth, 0)))
		return 1;

	while (clip.h--) {
		gx_callcbltf(cblt, dptr, NULL, clip.w, (void*)col);
		dptr += dst->scanLen;
	}

	return 0;
}

int gx_copysurf(gx_Surf dst, int x, int y, gx_Surf src, gx_Rect roi, cblt_mode mode) {
	register char *dptr, *sptr;
	register void (*cblt)();
	struct gx_Rect clip;

	clip.x = roi ? roi->x : 0;
	clip.y = roi ? roi->y : 0;
	clip.w = roi ? roi->w : src->width;
	clip.h = roi ? roi->h : src->height;

	if (!(sptr = gx_cliprect(src, &clip)))
		return -1;

	clip.x = x; clip.y = y;
	if(!(dptr = gx_cliprect(dst, &clip)))
		return -1;


	switch (mode) {
		case cblt_cpy : {
			if(!(cblt = gx_getcbltf(cblt_conv, dst->depth, src->depth)))
				return -1;
		} break;
		case cblt_and : {
			if (dst->depth != src->depth)
				return -1;
			if(!(cblt = gx_getcbltf(cblt_copy, dst->depth, cblt_and)))
				return -1;
		} break;
		case cblt_or  : {
			if (dst->depth != src->depth)
				return -1;
			if(!(cblt = gx_getcbltf(cblt_copy, dst->depth, cblt_or)))
				return -1;
		} break;
		case cblt_xor : {
			if (dst->depth != src->depth)
				return -1;
			if(!(cblt = gx_getcbltf(cblt_conv, dst->depth, cblt_xor)))
				return -1;
		} break;
		default : return -1;
	}

	while (clip.h--) {
		gx_callcbltf(cblt, dptr, sptr, clip.w, NULL);
		dptr += dst->scanLen;
		sptr += src->scanLen;
	}

	return 0;
}

int gx_zoomsurf(gx_Surf dst, gx_Rect rect, gx_Surf src, gx_Rect roi, int lin) {
	struct gx_Rect drec, srec;
	long dx, dy, sx, sy, x, y;
	long x0 = 0, y0 = 0;
	char *dptr;

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
		if (roi->w < srec.w)
			srec.w = roi->w;
		if (roi->h < srec.h)
			srec.h = roi->h;
	}
	if (rect == 0) {
		drec.x = drec.y = 0;
		drec.w = dst->width;
		drec.h = dst->height;
	}
	else
		drec = *rect;

	if (drec.w <= 0 || drec.h <= 0)
		return -1;
	if (srec.w <= 0 || srec.h <= 0)
		return -1;

	//~ if (drec.x < 0)
		//~ drec.w += drec.x;
	//~ if (drec.y < 0)
		//~ drec.h += drec.y;

	dx = ((srec.w - 0) << 16) / (drec.w - 0);
	dy = ((srec.h - 0) << 16) / (drec.h - 0);

	if (drec.x < 0) {
		x0 = -drec.x;
	}

	if (drec.y < 0) {
		y0 = -drec.y;
	}

	if (!(dptr = gx_cliprect(dst, &drec)))
		return -1;

	// if downsampling use nearest neigbor
	if (dx & 0xfffe0000 || dy & 0xfffe0000) {
		lin = 0;
	}

	x0 = x0 * dx + (srec.x << 16);
	y0 = y0 * dy + (srec.y << 16);

	for (y = 0, sy = y0; y < drec.h; ++y, sy += dy) {
		long *ptr = (long*)dptr;
		for (x = 0, sx = x0; x < drec.w; ++x, sx += dx) {
			*ptr++ = gx_getpix16(src, sx, sy, lin);
			//~ gx_setpixel(dst, drec.x + x, drec.y + y, gx_getpix16(src, sx, sy, lin));
		}
		dptr += dst->scanLen;
	}

	return 0;
}



/*void gx_drawCurs(gx_Surf dst, int x, int y, gx_Surf cur, int hide) {
	//~ extern void colcpy_32mix();

	char *d, *s, *dptr, *sptr, tmp[32*32*4];		// 4096 bytes
	struct gx_Rect clp;
	void (*rdbfn)();
	void (*wtbfn)();

	if (cur->flags && SURF_ID_GET != SURF_ID_CUR) return;

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
		gx_callcbltf(rdbfn, s, d, clp.w, NULL);
		d += dst->scanLen; s += cur->scanLen;
	}
	if (hide & (2|4)) {
		if (hide & 2) {
			memcpy(tmp, cur->basePtr, 32*32*4);
			gx_callcbltf(colcpy_32mix, tmp, cur->tempPtr, 32*32, NULL);
			sptr = tmp + (sptr - (char*)cur->basePtr);
		}
		for (d = dptr, s = sptr, y = 0; y < clp.h; ++y) {
			gx_callcbltf(wtbfn, d, s, clp.w, NULL);
			d += dst->scanLen; s += cur->scanLen;
		}
	}
}// */

int gx_loadFNT(gx_Surf dst, const char* fontfile) {
	gx_Llut lut;
	int fsize, i;
	char tmp[4096], *ptr;
	FILE *fin = fopen(fontfile, "rb");

	dst->basePtr = NULL;

	if (fin == 0) return -1;
	fsize = fread(tmp, 1, sizeof(tmp), fin);
	fclose(fin);

	if(!(ptr = malloc(8 * fsize + sizeof(struct gx_Llut)))) {
		return -3;
	}

	dst->basePtr = ptr;
	dst->lutLPtr = (gx_Llut)(ptr + 8 * fsize);
	dst->flags = Surf_mem | Surf_fnt;

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

	ptr = dst->basePtr;
	lut = dst->lutLPtr;
	lut->unused = 256;
	lut->height = fsize >> 8;
	fsize = 8 * lut->height;
	for (i = 0; i < 256; ++i, ptr += fsize) {
		lut->data[i].pad_x = 0;
		lut->data[i].pad_y = 0;
		lut->data[i].width = 8;
		lut->data[i].basePtr = ptr;
	}

	dst->width = 8;
	dst->height = 256 * lut->height;
	dst->clipPtr = 0;
	dst->depth = 8;
	dst->pixeLen = 1;
	dst->scanLen = 8;
	return 0;
}

void gx_drawChar(gx_Surf dst, int x, int y, gx_Surf fnt, unsigned chr, long col) {
	struct gx_FDIR face;
	struct gx_Rect clip;
	void (*cblt)() = 0;
	char *dptr, *sptr;

	if (!fnt->basePtr)
		return;

	//~ if (fnt->flags & SurfMask != Surf_fnt) return;

	face = fnt->lutLPtr->data[chr & 255];

	clip.x = x + face.pad_x;
	clip.y = y + face.pad_y;
	clip.w = face.width;
	clip.h = fnt->lutLPtr->height;
	if (clip.w <= 0 || clip.h <= 0) return;

	if (!(dptr = (char*)gx_cliprect(dst, &clip))) return;
	sptr = (char*)face.basePtr;// + (clip.y - y) * face.width + (clip.x - x);

	switch (dst->depth) {
		extern void colset_32mix(void);
		extern void colset_24mix(void);
		extern void colset_16mix(void);
		extern void colset_08mix(void);
		case 32 : cblt = colset_32mix; break;
		default : return;
	}

	while (clip.h-- > 0) {
		gx_callcbltf(cblt, dptr, sptr, clip.w, (void*)col);
		dptr += dst->scanLen;
		sptr += face.width;
	}
}

void gx_drawText(gx_Surf dst, int x, int y, gx_Surf fnt, const char *str, long col) {
	int x0 = x;

	if (!fnt->basePtr)
		return;

	while (*str) {
		unsigned chr = *str++;
		switch (chr) {
			case '\r':
				if (*str == '\n') str++;
				// fall
			case '\n':
				y += fnt->lutLPtr->height;
				x = x0;
				break;

			//~ case '\t': wln += 8; break;

			default:
				gx_drawChar(dst, x, y, fnt, chr, col);
				x += fnt->lutLPtr->data[chr].width;
				break;
		}
	}
}

void gx_clipText(gx_Rect roi, gx_Surf fnt, const char *str) {
	struct gx_FDIR face;
	int chr, H = 0, W = 0;

	if (!fnt->basePtr)
		return;

	roi->w = roi->h = 0;

	while ((chr = (*str++ & 255))) {
		face = fnt->lutLPtr->data[chr];

		if (H < fnt->lutLPtr->height)
			H = fnt->lutLPtr->height;

		switch (*str) {
			case '\r': str += str[1] == '\n';
			case '\n': roi->h += H; W = 0; break;

			//~ case '\t': wln += 8; break;

			default:
				W += face.width;
		}

		if (roi->w < W)
			roi->w = W;
		if (roi->h < H)
			roi->h = H;
	}
}



/*pattern gx_patt[] = {
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
};// */
