#include <malloc.h>
#include <string.h>
#include "g2_surf.h"
#include "g2_argb.h"
#include <stdio.h>

extern void colcpy_32cpy();
extern void colcpy_32mix();

int gx_initSurf(gx_Surf *s, gx_Clip *clip, gx_Clut *clut, int flags) {
	int	tmp = s->depth; s->flags = 0;
	s->pixeLen = tmp == 8 ? 1 : tmp == 15 ? 2 : tmp == 16 ? 2 : tmp == 24 ? 3 : tmp == 32 ? 4 : 0;
	if (!s->scanLen) s->scanLen = s->pixeLen * s->width;
	// TODO: if (flags & Reuse) then ...
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
		if ((s->clipPtr = (gx_Clip*)malloc(sizeof(struct gx_Clip_t)))) {
			memcpy(s->clipPtr, clip, sizeof(struct gx_Clip_t));
			s->flags |= SURF_DELCLP;
		} else return -3;
	} else s->clipPtr = clip;
	//~ s->movePtr = s->offsPtr = 0;
	//~ gx_initdraw(s, rop2_cpy);
	return 0;
}

void gx_doneSurf(gx_Surf *surf/* , int flags */) {
	if (surf->clipPtr && (surf->flags & SURF_DELCLP)) free(surf->clipPtr);
	if (surf->basePtr && (surf->flags & SURF_DELMEM)) free(surf->basePtr);
	if (surf->clutPtr && (surf->flags & SURF_DELLUT)) free(surf->clutPtr);
}

gx_Surf* gx_createSurf(unsigned width, unsigned height, unsigned depth, int flags) {
	gx_Surf *surf = (gx_Surf*)malloc(sizeof(struct gx_Surf_t));
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

gx_Surf* gx_attachSurf(gx_Surf *src, gx_Rect *roi) {
	struct gx_Rect_t clip;
	void*	basePtr;
	gx_Surf *surf = 0;
	if (roi == NULL) {
		clip.x = clip.y = 0;
		clip.w = src->width;
		clip.h = src->height;
	} else clip = *roi;
	if ((basePtr = gx_cliprect(src, &clip))) {
		if ((surf = (gx_Surf*)malloc(sizeof(struct gx_Surf_t)))) {
			*surf = *src;
			surf->basePtr = basePtr;
			surf->flags &= ~(SURF_DELCLP | SURF_DELMEM | SURF_DELLUT);
		}
	}
	return surf;
}

void gx_destroySurf(gx_Surf *surf) {
	if (surf->clipPtr && (surf->flags & SURF_DELCLP)) free(surf->clipPtr);
	if (surf->basePtr && (surf->flags & SURF_DELMEM)) free(surf->basePtr);
	if (surf->clutPtr && (surf->flags & SURF_DELLUT)) free(surf->clutPtr);
	free(surf);
}

/*void* gx_cliprect(gx_Surf surf, gx_Rect roi) {
	gx_Clip clp = surf->clipPtr ? surf->clipPtr : (gx_Clip)surf;
	roi->w += roi->x;
	roi->h += roi->y;
	if (clp->l > roi->x) roi->x = clp->l;
	if (clp->t > roi->y) roi->y = clp->t;
	if (clp->r < roi->w) roi->w = clp->r;
	if (clp->b < roi->h) roi->h = clp->b;
	if ((roi->w -= roi->x) <= 0) return 0;
	if ((roi->h -= roi->y) <= 0) return 0;
	return gx_getpaddr(surf, roi->x, roi->y);
}// */

void gx_copysurf1(gx_Surf *dst, int x, int y, gx_Surf *src, gx_Rect *roi) {
	gx_Clip *clp = dst->clipPtr ? dst->clipPtr : (gx_Clip*)dst;
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

void gx_copysurf2(gx_Surf *dst, int x, int y, gx_Surf *src, gx_Rect *roi) {
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
	//~ if(clip.w <= 0 || clip.h <= 0) return;
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

int gx_copysurf(gx_Surf *dst, int x, int y, gx_Surf *src, gx_Rect *roi, cblt_mode mode) {
	register char *dptr, *sptr;
	register void (*cblt)();
	struct gx_Rect_t clip;

	clip.x = roi ? roi->x : 0;
	clip.y = roi ? roi->y : 0;
	clip.w = roi ? roi->w : src->width;
	clip.h = roi ? roi->h : src->height;

	if(!(sptr = (char*)gx_cliprect(src, &clip))) return -1;	// we have W & H

	switch (mode) {

		case cblt_cpy : {
			clip.x = x; clip.y = y;
			if(!(dptr = (char*)gx_cliprect(dst, &clip))) return -1;	// we have W & H
			if(!(cblt = gx_getcbltf(cblt_conv, dst->depth, src->depth))) return -1;
		} break;

		case cblt_and : {
			clip.x = x; clip.y = y;
			if (dst->depth != src->depth) return -1;
			if(!(dptr = (char*)gx_cliprect(dst, &clip))) return -1;	// we have W & H
			if(!(cblt = gx_getcbltf(cblt_copy, dst->depth, cblt_and))) return -1;
		} break;

		case cblt_or  : {
			clip.x = x; clip.y = y;
			if (dst->depth != src->depth) return -1;
			if(!(dptr = (char*)gx_cliprect(dst, &clip))) return -1;	// we have W & H
			if(!(cblt = gx_getcbltf(cblt_copy, dst->depth, cblt_or))) return -1;
		} break;

		case cblt_xor : {
			clip.x = x; clip.y = y;
			if (dst->depth != src->depth) return -1;
			if(!(dptr = (char*)gx_cliprect(dst, &clip))) return -1;	// we have W & H
			if(!(cblt = gx_getcbltf(cblt_conv, dst->depth, cblt_xor))) return -1;
		} break;

		default : return -1;
	}

	while (clip.h--) {
		gx_callcbltf(cblt, dptr, sptr, clip.w, 0);
		dptr += dst->scanLen;
		sptr += src->scanLen;
	}

	return 0;
}

int gx_fillsurf(gx_Surf *dst, gx_Rect *roi, cblt_mode mode, long col) {
	register char *dptr;
	register void (*cblt)();
	struct gx_Rect_t clip;

	clip.x = roi ? roi->x : 0;
	clip.y = roi ? roi->y : 0;
	clip.w = roi ? roi->w : dst->width;
	clip.h = roi ? roi->h : dst->height;

	if(!(dptr = (char*)gx_cliprect(dst, &clip))) return -1;
	if(!(cblt = gx_getcbltf(cblt_fill, dst->depth, 0))) return 1;

	while (clip.h--) {
		gx_callcbltf(cblt, dptr, NULL, clip.w, (void*)col);
		dptr += dst->scanLen;
		//~ sptr += src->scanLen;
	}

	return 0;
}

extern void callcolcpy(void colcpy(void), void* dst, void* src, unsigned cnt);
#pragma aux callcolcpy\
		parm  [eax] [edi] [esi] [ecx]\
		modify [eax edx ecx esi edi]\
		value [] = "call ebx";

void gx_zoomsurf2(gx_Surf *dst, gx_Rect *rect, gx_Surf *src, gx_Rect *roi, int lin) {
	long (*getpix)(gx_Surf*, long, long) = lin ? gx_getpblin : gx_getpnear;
	struct gx_Rect_t drec, srec;
	long dx, dy, sx, sy, x, y;
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
		if (roi->w < srec.w) srec.w = roi->w;
		if (roi->h < srec.h) srec.h = roi->h;
	}
	if (rect == 0) {
		drec.x = drec.y = 0;
		drec.w = dst->width;
		drec.h = dst->height;
	} else drec = *rect;
	if (drec.w <= 0 || drec.h <= 0) return;
	if (srec.w <= 0 || srec.h <= 0) return;
	if (!(dptr = (char*)gx_cliprect(dst, &drec))) return;
	dx = ((srec.w - 1) << 16) / drec.w;
	dy = ((srec.h - 1) << 16) / drec.h;

	if (dx & 0xfffe0000 || dy & 0xfffe0000) {	// here we have some problems
		//~ char tmpbuff[65535];
		getpix = gx_getpnear;
	}

	for (y = 0, sy = srec.y << 16; y < drec.h; ++y, sy += dy) {
		register long *ptr = (long *)dptr; dptr += dst->scanLen;
		for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx) {
			*ptr++ = getpix(src, sx, sy);
			gx_setpixel(dst, x, y, getpix(src, sx, sy));
		}
	}
}

inline argb mix2(argb p1, argb p2, long l) {
	p1.r += (l * (p2.r - p1.r)) >> 16;
	p1.g += (l * (p2.g - p1.g)) >> 16;
	p1.b += (l * (p2.b - p1.b)) >> 16;
	return p1;
}

inline argb mix4(argb p1, argb p2, argb p3, argb p4, long lx, long ly) {
	return mix2(mix2(p1, p2, lx), mix2(p3, p4, lx), ly);
}

void gx_zoomsurf(gx_Surf *dst, gx_Rect *rect, gx_Surf *src, gx_Rect *roi, int lin) {
	struct gx_Rect_t drec, srec;
	long dx, dy, sx, sy, x, y;
	char *dln, *sln;

	if (roi == 0) {
		srec.x = srec.y = 0;
		srec.w = src->width;
		srec.h = src->height;
	} else srec = *roi;

	if (rect == 0) {
		drec.x = drec.y = 0;
		drec.w = dst->width;
		drec.h = dst->height;
	} else drec = *rect;

	if (!(dln = (char*)gx_cliprect(dst, &drec))) return;
	if (!(sln = (char*)gx_cliprect(src, &srec))) return;

	dx = ((srec.w - 1) << 16) / drec.w;
	dy = ((srec.h - 1) << 16) / drec.h;
	//~ #define xxkhk 0XFFFF0000
	#define xxkhk 0XFFFE0000
	if (lin == 0) {
		for (y = 0, sy = 0; y < drec.h; ++y) {
			long *spx = (long *)sln;
			long *dpx = (long *)dln;
			for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx) {
				*dpx++ = spx[sx >> 16];
			}
			dln += dst->scanLen;
			if ((sy += dy) & 0xffff0000) {
				sln += (sy >> 16) * src->scanLen;
				sy &= 0x0000ffff;
			}
		}
	}
	else if (dx & xxkhk && dy & xxkhk) {
		int i, j, cnt = (dx >> 16) * (dy >> 16);
		//~ return;
		for (y = sy = 0; y < drec.h; ++y) {
			argb *dpx = (argb*)dln;
			argb *sum = (argb*)sln;

			for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx, dpx++) {
				char *lns = (char *)&sum[sx >> 16];
				long r = 0, g = 0, b = 0;
				for (j = 0; j < dy >> 16; j++) {
					argb *spx = (argb *)lns;
					for (i = 0; i < dx >> 16; i++) {
						r += spx->r;
						g += spx->g;
						b += spx->b;
						spx++;
					}
					lns += src->scanLen;
				}
				dpx->r = r / cnt;
				dpx->g = g / cnt;
				dpx->b = b / cnt;
				//~ *dpx = sum[sx >> 16];
			}
			dln += dst->scanLen;
			if ((sy += dy) & 0xffff0000) {
				sln += (sy >> 16) * src->scanLen;
				sy &= 0x0000ffff;
			}
		}
	}
	else if (dx & xxkhk) {
		int i, len = (dx >> 16) + 3;
		//~ return;
		for (y = sy = 0; y < drec.h; ++y) {
			argb *dpx = (argb*)dln;
			argb *sum = (argb*)sln;
			for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx) {
				argb *ln1 = (argb *)&sum[sx >> 16];
				argb *ln2 = (argb *)((char*)ln1 + src->scanLen);
				argb col;
				long r = 0, g = 0, b = 0;
				for (i = 0; i < len; i++, ln1++, ln2++) {
					col = mix2(ln1[0], ln2[0], sy & 0xffff);
					r += col.r; g+= col.g; b += col.b;
				}
				col.r = r / len;
				col.g = g / len;
				col.b = b / len;
				//~ col = mix2(col, mix2(ln1[0], ln2[0], sy & 0xffff), sx & 0xffff);
				//~ dpx->r = r / len;
				//~ dpx->g = g / len;
				//~ dpx->b = b / len;
				*dpx++ = col;
			}
			dln += dst->scanLen;
			if ((sy += dy) & 0xffff0000) {
				sln += (sy >> 16) * src->scanLen;
				sy &= 0x0000ffff;
			}
		}
	}
	else if (dy & xxkhk) {
		int i, len = dy >> 16;
		//~ return;
		for (y = 0, sy = srec.y << 16; y < drec.h; ++y, sy += dy) {
			register unsigned long *sptr = gx_getpaddr(src, srec.x, sy >> 16);
			register argb *d = (argb *)dln;
			for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx) {
				argb *ln1 = (argb *)&sptr[sx >> 16];
				long r = 0, g = 0, b = 0;
				for (i = 0; i < len; i++) {
					argb c = mix2(ln1[0], ln1[1], sx & 0xffff);
					ln1 = (argb *)((char*)ln1 + src->scanLen);
					r += c.r; g+= c.g; b += c.b;
				}
				d->r = r / len;
				d->g = g / len;
				d->b = b / len;
				d++;
			}
			dln += dst->scanLen;
		}
	}
	else for (y = sy = 0; y < drec.h; ++y) {
		argb *dpx = (argb *)dln;
		//~ return;
		for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx) {
			argb *ln1 = &((argb *)sln)[sx >> 16];
			argb *ln2 = (argb *)((char*)ln1 + src->scanLen);
			*dpx++ = mix4(ln1[0], ln1[1], ln2[0], ln2[1], sx & 0xffff, sy & 0xffff);
		}
		dln += dst->scanLen;
		if ((sy += dy) & 0xffff0000) {
			sln += (sy >> 16) * src->scanLen;
			sy &= 0x0000ffff;
		}
	}// */

	/*for (y = 0, sy = srec.y << 16; y < drec.h; ++y, sy += dy) {
		register long *ptr = (long *)dptr; dptr += dst->scanLen;
		for (x = 0, sx = srec.x << 16; x < drec.w; ++x, sx += dx) {
			*ptr++ = getpix(src, sx, sy);
			gx_setpixel(dst, x, y, getpix(src, sx, sy));
		}
	}// */

}

void gx_drawCurs(gx_Surf *dst, int x, int y, gx_Surf *cur, int hide) {
	char *d, *s, *dptr, *sptr, tmp[32*32*4];		// 4096 bytes
	struct gx_Rect_t clp;
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

int  gx_loadFNT(gx_Surf *dst, const char* fontfile) {
	gx_Llut *lut;
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
	lut = (gx_Llut*)dst->clutPtr;
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
	//~ gx_initdraw(dst, rop2_cpy);
	
	//~ dst->clutPtr;		// clutPtr / cibgPtr (cursor BG data) / ...
	//~ dst->basePtr;		// always base of surface
	//~ dst->offsPtr;		// offsPtr(scanLen, x, y):(void*)	parm[eax, ecx, edx] value [edx]
	//~ dst->movePtr;		// movePtr(offsPtr,  col):(void)	parm[edi, eax]
	return 0;
}

void gx_drawChar1(gx_Surf *dst, int x, int y, gx_Surf *fnt, char chr, long col) {
	gx_Llut *lut = (gx_Llut*)fnt->clutPtr;
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

void gx_drawChar(gx_Surf *dst, int x, int y, gx_Surf *fnt, char chr, long col) {
	struct gx_FDIR face = (*(gx_Llut*)fnt->clutPtr).data[(unsigned)chr];
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

void gx_clipText(gx_Rect *roi, gx_Surf *fnt, const char *str) {
	struct gx_FDIR face;
	int H = 0, W = 0;
	roi->w = 0;
	roi->h = 0;
	while (*str) {
		unsigned chr = *str++;
		face = (*(gx_Llut*)fnt->clutPtr).data[chr];
		if (H < face.height) H = face.height;
		switch (*str) {
			//~ case '\r': str += str[1] == '\n';
			case '\n': roi->h += H; W = 0; break;

			//~ case '\t': wln += 8; break;

			default:
				face = (*(gx_Llut*)fnt->clutPtr).data[chr];
				W += face.width;
		}

		if (roi->w < W)
			roi->w = W;
		if (roi->h < H)
			roi->h = H;
	}
}

void gx_drawText(gx_Surf *dst, int x, int y, gx_Surf *fnt, const char *str, long col) {
	char chr;
	while ((chr = *str++)) {
		gx_drawChar(dst, x, y, fnt, chr, col);
		x += ((gx_Llut*)fnt->clutPtr)->data[(unsigned)chr].width;
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
