#include <malloc.h>
#include "gx_surf.h"


int gx_initSurf(gx_Surf surf, int width, int height, int depth, surfFlags flags) {
	if (flags & Surf_recycle && surf->basePtr != NULL) {
		if (surf->flags & Surf_freeData) {
			surf->flags &= ~Surf_freeData;
			free(surf->basePtr);
		}
		surf->basePtr = NULL;
	}

	surf->x0y0 = 0;
	surf->width = width;
	surf->height = height;
	surf->depth = depth;
	surf->flags = flags & SurfType;

	switch (depth) {
		default:
			return -1;

		case 32:
			surf->scanLen = width * 4;
			surf->pixeLen = 4;
			break;

		case 24:
			surf->scanLen = width * 3;
			surf->pixeLen = 3;
			break;

		case 16:
		case 15:
			surf->scanLen = width * 2;
			surf->pixeLen = 2;
			break;

		case 8:
			surf->scanLen = width;
			surf->pixeLen = 1;
			break;
	}


	if (width > 0 && height > 0) {
		size_t offs = 0;
		size_t size = surf->scanLen * height;

		if ((flags & SurfType) == Surf_3ds) {
			offs = size;
			size += width * height * 4;
		}

		if (!(surf->basePtr = malloc(size))) {
			return -1;
		}

		surf->tempPtr = surf->basePtr + offs;
		surf->flags |= Surf_freeData;
	}
	else {
		surf->basePtr = NULL;
		surf->tempPtr = NULL;
	}

	return 0;
}
int gx_attachSurf(gx_Surf dst, gx_Surf src, gx_Rect roi) {
	char *basePtr;
	struct gx_Rect clip;

	if (roi == NULL) {
		clip.x = clip.y = 0;
		clip.w = src->width;
		clip.h = src->height;
	}
	else {
		clip = *roi;
	}

	if ((basePtr = gx_cliprect(src, &clip))) {
		dst->clipPtr = NULL;
		dst->width = clip.w;
		dst->height = clip.h;
		dst->flags = src->flags & ~Surf_freeData;
		dst->depth = src->depth;
		dst->pixeLen = src->pixeLen;
		dst->scanLen = src->scanLen;
		dst->basePtr = basePtr;

		switch (src->flags & SurfType) {
			case Surf_3ds:
				dst->tempPtr = basePtr + (src->tempPtr - src->basePtr);
				break;

			default:
				dst->tempPtr = src->tempPtr;
				break;
		}
	}
	return basePtr != NULL;
}

gx_Surf gx_createSurf(int width, int height, int depth, surfFlags flags) {
	gx_Surf surf = (gx_Surf)malloc(sizeof(struct gx_Surf));
	if (surf) {
		if (gx_initSurf(surf, width, height, depth, flags)) {
			free(surf);
			return NULL;
		}
		surf->flags |= Surf_freeSurf;
	}
	return surf;
}
void gx_destroySurf(gx_Surf surf) {
	if (surf->flags & Surf_freeData) {
		surf->flags &= ~Surf_freeData;
		if (surf->basePtr != NULL) {
			free(surf->basePtr);
		}
	}
	surf->basePtr = NULL;
	if (surf->flags & Surf_freeSurf) {
		surf->flags &= ~Surf_freeSurf;
		free(surf);
	}
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

void g2_drawrect(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color) {
	g2_fillrect(surf, x1, y1, x2, y1 + 1, color);
	g2_fillrect(surf, x1, y2, x2 + 1, y2 + 1, color);
	g2_fillrect(surf, x1, y1, x1 + 1, y2, color);
	g2_fillrect(surf, x2, y1, x2 + 1, y2, color);
}
void g2_fillrect(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color) {
	gx_Clip roi = gx_getclip(surf);
    int x, y;
	if (x1 > x2) {
        x = x1;
		x1 = x2;
		x2 = x;
	}
	if (x1 < roi->xmin) {
		x1 = roi->xmin;
	}
	if (x2 > roi->xmax) {
		x2 = roi->xmax;
	}
	if (y1 > y2) {
        y = y1;
		y1 = y2;
		y2 = y;
	}
	if (y1 < roi->ymin) {
		y1 = roi->ymin;
	}
	if (y2 > roi->ymax) {
		y2 = roi->ymax;
	}
	// TODO: optimize for hline, vline and pixel.
    // TODO: for hline and block use cblt_proc(void* dst, void* src, void* extra, unsigned count);
    for (y = y1; y < y2; y++) {
        for (x = x1; x < x2; x++) {
			gx_setpixel(surf, x, y, color);
		}
	}
}

void g2_drawoval(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color) {
	int dx, dy, sx, sy, r;
	if (x1 > x2) {
		x1 ^= x2;
		x2 ^= x1;
		x1 ^= x2;
	}
	if (y1 > y2) {
		y1 ^= y2;
		y2 ^= y1;
		y1 ^= y2;
	}
	dx = x2 - x1;
	dy = y2 - y1;

	x2 = x1 += dx >> 1;
	x1 += dx & 1;

	dx += dx & 1;
	dy += dy & 1;

	sx = dx * dx;
	sy = dy * dy;
	r = sx * dy >> 2;

	dx = 0;
	dy = r << 1;

	while (y1 < y2) {
		gx_setpixel(surf, x1, y1, color);
		gx_setpixel(surf, x1, y2, color);
		gx_setpixel(surf, x2, y1, color);
		gx_setpixel(surf, x2, y2, color);

		if (r >= 0) {
			x1 -= 1;
			x2 += 1;
			r -= dx += sy;
		}

		if (r < 0) {
			y1 += 1;
			y2 -= 1;
			r += dy -= sx;
		}
	}
	gx_setpixel(surf, x1, y1, color);
	gx_setpixel(surf, x1, y2, color);
	gx_setpixel(surf, x2, y1, color);
	gx_setpixel(surf, x2, y2, color);
}
void g2_filloval(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color) {
	int dx, dy, sx, sy, r;
	if (x1 > x2) {
		x1 ^= x2;
		x2 ^= x1;
		x1 ^= x2;
	}
	if (y1 > y2) {
		y1 ^= y2;
		y2 ^= y1;
		y1 ^= y2;
	}
	dx = x2 - x1;
	dy = y2 - y1;

	x2 = x1 += dx >> 1;
	x1 +=dx & 1;

	dx += dx & 1;
	dy += dy & 1;

	sx = dx * dx;
	sy = dy * dy;

	r = sx * dy >> 2;
	dx = 0;
	dy = r << 1;

	while (y1 < y2) {
		g2_fillrect(surf, x1, y1, x1 + 1, y2, color);
		g2_fillrect(surf, x2, y1, x2 + 1, y2, color);

		if (r >= 0) {
			x1 -= 1;
			x2 += 1;
			r -= dx += sy;
		}

		if (r < 0) {
			y1 += 1;
			y2 -= 1;
			r += dy -= sx;
		}
	}
	g2_fillrect(surf, x1, y1, x1 + 1, y2, color);
	g2_fillrect(surf, x2, y1, x2 + 1, y2, color);
}

void g2_drawline(gx_Surf surf, int x1, int y1, int x2, int y2, uint32_t color) {
	//~ TODO : replace Bresenham with DDA, resolve clipping
	int sx, sy, r;
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (dx < 0) {
		dx = -dx;
		sx = -1;
	}
	else {
		sx = 1;
	}

	if (dy < 0) {
		dy = -dy;
		sy = -1;
	}
	else {
		sy = 1;
	}

	if (dx > dy) {
		r = dx >> 1;
		while (x1 != x2) {
			gx_setpixel(surf, x1, y1, color);
			if ((r += dy) > dx) {
				y1 += sy;
				r -= dx;
			}
			x1 += sx;
		}
	}
	else {
		r = dy >> 1;
		while (y1 != y2) {
			gx_setpixel(surf, x1, y1, color);
			if ((r+=dx) > dy) {
				x1 += sx;
				r -= dy;
			}
			y1 += sy;
		}
	}
	gx_setpixel(surf, x1, y1, color);
}
void g2_drawbez2(gx_Surf surf, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
	double t, dt = 1. / 128;

	int px_0 = x1;
	int py_0 = y1;
	int px_1 = 2 * (x2 - x1);
	int py_1 = 2 * (y2 - y1);
	int px_2 = x3 - 2 * x2 + x1;
	int py_2 = y3 - 2 * y2 + y1;

	for (t = dt; t < 1; t += dt) {
		x2 = (px_2 * t + px_1) * t + px_0;
		y2 = (py_2 * t + py_1) * t + py_0;
		g2_drawline(surf, x1, y1, x2, y2, color);
		x1 = x2;
		y1 = y2;
	}
	g2_drawline(surf, x1, y1, x3, y3, color);
}
void g2_drawbez3(gx_Surf surf, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, uint32_t color) {
	double t, dt = 1. / 128;

	int px_0 = x1;
	int py_0 = y1;
	int px_1 = 3 * (x2 - x1);
	int py_1 = 3 * (y2 - y1);
	int px_2 = 3 * (x3 - x2) - px_1;
	int py_2 = 3 * (y3 - y2) - py_1;
	int px_3 = x4 - px_2 - px_1 - px_0;
	int py_3 = y4 - py_2 - py_1 - py_0;

	for (t = dt; t < 1; t += dt) {
		x2 = ((px_3 * t + px_2) * t + px_1) * t + px_0;
		y2 = ((py_3 * t + py_2) * t + py_1) * t + py_0;
		g2_drawline(surf, x1, y1, x2, y2, color);
		x1 = x2;
		y1 = y2;
	}
	g2_drawline(surf, x1, y1, x4, y4, color);
}

void g2_clipText(gx_Rect rect, gx_Surf font, const char *text) {
	struct gx_FDIR face;
	int chr, H = 0, W = 0;

	if ((font->flags & SurfType) != Surf_fnt) {
		return;
	}
	if (!font->basePtr) {
		return;
	}

	rect->w = rect->h = 0;

	while ((chr = (*text++ & 255))) {
		face = font->LLUTPtr->data[chr];

		if (H < font->LLUTPtr->height)
			H = font->LLUTPtr->height;

		switch (*text) {
			case '\r': text += text[1] == '\n';
			case '\n': rect->h += H; W = 0; break;

			//~ case '\t': wln += 8; break;

			default:
				W += face.width;
		}

		if (rect->w < W)
			rect->w = W;
		if (rect->h < H)
			rect->h = H;
	}
}
void g2_drawChar(gx_Surf surf, int x, int y, gx_Surf font, char chr, uint32_t color) {
	struct gx_FDIR face;
	struct gx_Rect clip;
	void (*cblt)() = 0;
	char *dptr, *sptr;

	if ((font->flags & SurfType) != Surf_fnt) {
		return;
	}
	if (!font->basePtr) {
		return;
	}

	face = font->LLUTPtr->data[chr & 255];

	clip.x = x + face.pad_x;
	clip.y = y + face.pad_y;
	clip.w = face.width;
	clip.h = font->LLUTPtr->height;

	if (clip.w <= 0 || clip.h <= 0) return;

	if (!(dptr = (char*)gx_cliprect(surf, &clip))) {
		return;
	}
	sptr = (char*)face.basePtr;// + (clip.y - y) * face.width + (clip.x - x);

	if (!(cblt = gx_getcbltf(cblt_set_mix, surf->depth))) {
		return;
	}

	while (clip.h-- > 0) {
		cblt(dptr, sptr, (void*)color, clip.w);
		dptr += surf->scanLen;
		sptr += face.width;
	}
}
void g2_drawText(gx_Surf surf, int x, int y, gx_Surf font, const char *text, uint32_t color) {
	int x0 = x;

	if ((font->flags & SurfType) != Surf_fnt) {
		return;
	}
	if (!font->basePtr) {
		return;
	}

	while (*text) {
		unsigned chr = *text++;
		switch (chr) {
			case '\r':
				if (*text == '\n') text++;
				// fall
			case '\n':
				y += font->LLUTPtr->height;
				x = x0;
				break;

			//~ case '\t': wln += 8; break;

			default:
				g2_drawChar(surf, x, y, font, chr, color);
				x += font->LLUTPtr->data[chr].width;
				break;
		}
	}
}

int gx_copysurf(gx_Surf surf, int x, int y, gx_Surf src, gx_Rect roi) {
	register char *dptr, *sptr;
	register void (*cblt)();
	struct gx_Rect clip;

	clip.x = roi ? roi->x : 0;
	clip.y = roi ? roi->y : 0;
	clip.w = roi ? roi->w : src->width;
	clip.h = roi ? roi->h : src->height;

	if (!(sptr = gx_cliprect(src, &clip))) {
		return -1;
	}

	clip.x = x; clip.y = y;
	if(!(dptr = gx_cliprect(surf, &clip))) {
		return -1;
	}

	if(!(cblt = gx_getcbltf(surf->depth, src->depth)))
		return -1;

	while (clip.h--) {
		cblt(dptr, sptr, NULL, clip.w);
		dptr += surf->scanLen;
		sptr += src->scanLen;
	}

	return 0;
}
int gx_zoomsurf(gx_Surf surf, gx_Rect rect, gx_Surf src, gx_Rect roi, int linear) {
	struct gx_Rect drec, srec;
	int32_t dx, dy, sx, sy, x, y;
	int32_t x0 = 0, y0 = 0;
	char *dptr;

	if (surf->depth != src->depth) {
		return -2;
	}

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
	if (rect == NULL) {
		drec.x = drec.y = 0;
		drec.w = surf->width;
		drec.h = surf->height;
	}
	else {
		drec = *rect;
	}

	if (drec.w <= 0 || drec.h <= 0) {
		return -1;
	}
	if (srec.w <= 0 || srec.h <= 0) {
		return -1;
	}

	dx = (srec.w << 16) / drec.w;
	dy = (srec.h << 16) / drec.h;

	if (drec.x < 0) {
		x0 = -drec.x;
	}

	if (drec.y < 0) {
		y0 = -drec.y;
	}

	if (!(dptr = gx_cliprect(surf, &drec))) {
		return -1;
	}

	// if downsampling use nearest neigbor
	if (dx & 0xfffe0000 || dy & 0xfffe0000) {
		linear = 0;
	}

	x0 = x0 * dx + (srec.x << 16);
	y0 = y0 * dy + (srec.y << 16);

	if (surf->depth == 32) {
		for (y = 0, sy = y0; y < drec.h; ++y, sy += dy) {
			uint32_t *ptr = (void*)dptr;
			for (x = 0, sx = x0; x < drec.w; ++x, sx += dx) {
				*ptr++ = gx_getpix16(src, sx, sy, linear);
			}
			dptr += surf->scanLen;
		}
	}
	else if (surf->depth == 8) {
		for (y = 0, sy = y0; y < drec.h; ++y, sy += dy) {
			char *ptr = (void*)dptr;
			for (x = 0, sx = x0; x < drec.w; ++x, sx += dx) {
				*ptr++ = gx_getpix16(src, sx, sy, linear);
			}
			dptr += surf->scanLen;
		}
	}
	else {
		for (y = 0, sy = y0; y < drec.h; ++y, sy += dy) {
			for (x = 0, sx = x0; x < drec.w; ++x, sx += dx) {
				gx_setpixel(surf, drec.x + x, drec.y + y, gx_getpix16(src, sx, sy, linear));
			}
		}
	}

	return 0;
}

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
	dst->LLUTPtr = (gx_Llut)(ptr + 8 * fsize);
	dst->flags = Surf_freeData | Surf_fnt;

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
	lut = dst->LLUTPtr;
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

/*int gx_fillsurf(gx_Surf surf, gx_Rect rect, uint32_t color) {
	register char *dptr;
	register void (*cblt)();
	struct gx_Rect clip;

	clip.x = rect ? rect->x : 0;
	clip.y = rect ? rect->y : 0;
	clip.w = rect ? rect->w : surf->width;
	clip.h = rect ? rect->h : surf->height;

	if(!(dptr = gx_cliprect(surf, &clip)))
		return -1;

	if(!(cblt = gx_getcbltf(cblt_fill, surf->depth, 0)))
		return 1;

	while (clip.h--) {
		gx_callcbltf(cblt, dptr, NULL, clip.w, (void*)color);
		dptr += surf->scanLen;
	}

	return 0;
}*/
