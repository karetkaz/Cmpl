#include "gx_surf.h"
#include <stdlib.h>

static inline int sign(int i) {
	return (i > 0) - (i < 0);
}

void fillRect(GxImage image, int x0, int y0, int x1, int y1, uint32_t color) {
	if (x0 > x1) {
		int t = x0;
		x0 = x1;
		x1 = t;
	}
	if (y0 > y1) {
		int t = y0;
		y0 = y1;
		y1 = t;
	}

	struct GxRect clip = {
			.x = x0,
			.y = y0,
			.w = x1 - x0,
			.h = y1 - y0,
	};
	bltProc blt = getBltProc(blt_set_col, image->depth);
	char *dptr = (char*) clipRect(image, &clip);
	if (dptr == NULL || blt == NULL) {
		return;
	}

	while (clip.h-- > 0) {
		blt(dptr, NULL, &color, clip.w);
		dptr += image->scanLen;
	}
}
void drawRect(GxImage image, int x0, int y0, int x1, int y1, uint32_t color) {
	if (x0 == x1 || y0 == y1) {
		fillRect(image, x0, y0, x1, y1, color);
		return;
	}

	if (x0 >= x1) {
		int t = x0 + 1;
		x0 = x1 + (x0 > x1);
		x1 = t;
	}
	if (y0 >= y1) {
		int t = y0 + 1;
		y0 = y1 + (y0 > y1);
		y1 = t;
	}

	struct GxRect clip = {
		.x = x0,
		.y = y0,
		.w = x1 - x0,
		.h = y1 - y0,
	};
	bltProc blt = getBltProc(blt_set_col, image->depth);
	char *dptr = (char*) clipRect(image, &clip);
	if (dptr == NULL || blt == NULL) {
		return;
	}

	int top = y0 >= clip.y;
	int left = x0 >= clip.x;
	int right = x1 <= clip.x + clip.w && x0 < x1 - 1;
	int bottom = y1 <= clip.y + clip.h && y0 < y1 - 1;

	if (top) {
		blt(dptr, NULL, &color, clip.w);
		dptr += image->scanLen;
	}
	if (left && right) {
		size_t r = (clip.w - 1) * image->pixelLen;
		for (int y = top; y < clip.h - bottom; y++) {
			blt(dptr, NULL, &color, 1);
			blt(dptr + r, NULL, &color, 1);
			dptr += image->scanLen;
		}
	}
	else if (left) {
		for (int y = top; y < clip.h - bottom; y++) {
			blt(dptr, NULL, &color, 1);
			dptr += image->scanLen;
		}
	}
	else if (right) {
		size_t r = (clip.w - 1) * image->pixelLen;
		for (int y = top; y < clip.h - bottom; y++) {
			blt(dptr + r, NULL, &color, 1);
			dptr += image->scanLen;
		}
	}
	else {
		dptr += (clip.h - top - bottom) * image->scanLen;
	}
	if (bottom) {
		blt(dptr, NULL, &color, clip.w);
	}
}

void fillOval(GxImage image, int x0, int y0, int x1, int y1, uint32_t color) {
	if (x0 == x1 || y0 == y1) {
		fillRect(image, x0, y0, x1, y1, color);
		return;
	}

	y1 -= sign(y1 - y0);
	x1 += x1 < x0;
	x0 += x0 > x1;

	if (x0 > x1) {
		int t = x0;
		x0 = x1;
		x1 = t;
	}
	if (y0 > y1) {
		int t = y0;
		y0 = y1;
		y1 = t;
	}
	int dx = x1 - x0;
	int dy = y1 - y0;

	x1 = x0 += dx / 2;
	x0 += dx & 1;

	dx += dx & 1;
	dy += dy & 1;

	int sx = dx * dx;
	int sy = dy * dy;

	int r = sx * dy / 4;
	int rdy = r * 2;
	int rdx = 0;

	while (y0 < y1) {
		fillRect(image, x0, y0, x1, y0, color);
		fillRect(image, x0, y1, x1, y1, color);

		if (r >= 0) {
			x0 -= 1;
			x1 += 1;
			r -= rdx += sy;
		}

		if (r < 0) {
			y0 += 1;
			y1 -= 1;
			r += rdy -= sx;
		}
	}
	fillRect(image, x0, y0, x1, y1, color);
}
void drawOval(GxImage image, int x0, int y0, int x1, int y1, uint32_t color) {
	if (x0 == x1 || y0 == y1) {
		fillRect(image, x0, y0, x1, y1, color);
		return;
	}

	y1 -= sign(y1 - y0);
	x1 -= sign(x1 - x0);

	if (x0 > x1) {
		int t = x0;
		x0 = x1;
		x1 = t;
	}
	if (y0 > y1) {
		int t = y0;
		y0 = y1;
		y1 = t;
	}
	int dx = x1 - x0;
	int dy = y1 - y0;

	x1 = x0 += dx / 2;
	x0 += dx & 1;

	dx += dx & 1;
	dy += dy & 1;

	int sx = dx * dx;
	int sy = dy * dy;

	int r = sx * dy / 4;
	int rdy = r * 2;
	int rdx = 0;

	while (y0 < y1) {
		setPixel(image, x0, y0, color);
		setPixel(image, x1, y0, color);
		setPixel(image, x0, y1, color);
		setPixel(image, x1, y1, color);

		if (r >= 0) {
			x0 -= 1;
			x1 += 1;
			r -= rdx += sy;
		}
		if (r < 0) {
			y0 += 1;
			y1 -= 1;
			r += rdy -= sx;
		}
	}
	setPixel(image, x0, y0, color);
	setPixel(image, x1, y0, color);
	setPixel(image, x0, y1, color);
	setPixel(image, x1, y1, color);
}

void drawLine(GxImage image, int x0, int y0, int x1, int y1, uint32_t color) {
	if (x0 == x1 || y0 == y1) {
		fillRect(image, x0, y0, x1, y1, color);
		return;
	}

	x1 -= sign(x1 - x0);
	y1 -= sign(y1 - y0);
	int dx = x1 - x0;
	int dy = y1 - y0;

	if (abs(dx) > abs(dy)) {
		if (x0 > x1) {
			int x = x0;
			x0 = x1;
			x1 = x;
			y0 = y1;
		}
		int y = (y0 << 16) + 0x8000;
		dy = (dy << 16) / dx;
		if (x1 > image->width) {
			x1 = image->width;
		}
		if (x0 < 0) {
			y -= dy * x0;
			x0 = 0;
		}

		for (int x = x0; x <= x1; x += 1) {
			setPixel(image, x, y >> 16, color);
			y += dy;
		}
		return;
	}

	if (y0 > y1) {
		int y = y0;
		y0 = y1;
		y1 = y;
		x0 = x1;
	}
	int x = (x0 << 16) + 0x8000;
	dx = (dx << 16) / dy;
	if (y1 > image->height) {
		y1 = image->height;
	}
	if (y0 < 0) {
		x -= dx * y0;
		y0 = 0;
	}

	for (int y = y0; y <= y1; y += 1) {
		setPixel(image, x >> 16, y, color);
		x += dx;
	}
}

void drawBez2(GxImage image, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
	int px_0 = x0;
	int py_0 = y0;
	int px_1 = 2 * (x1 - x0);
	int py_1 = 2 * (y1 - y0);
	int px_2 = x2 - 2 * x1 + x0;
	int py_2 = y2 - 2 * y1 + y0;

	double dt = 1. / 128;
	for (double t = dt; t < 1; t += dt) {
		x1 = (px_2 * t + px_1) * t + px_0;
		y1 = (py_2 * t + py_1) * t + py_0;
		drawLine(image, x0, y0, x1, y1, color);
		x0 = x1;
		y0 = y1;
	}
	drawLine(image, x0, y0, x2, y2, color);
}
void drawBez3(GxImage image, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
	int px_0 = x0;
	int py_0 = y0;
	int px_1 = 3 * (x1 - x0);
	int py_1 = 3 * (y1 - y0);
	int px_2 = 3 * (x2 - x1) - px_1;
	int py_2 = 3 * (y2 - y1) - py_1;
	int px_3 = x3 - px_2 - px_1 - px_0;
	int py_3 = y3 - py_2 - py_1 - py_0;

	double dt = 1. / 128;
	for (double t = dt; t < 1; t += dt) {
		x1 = ((px_3 * t + px_2) * t + px_1) * t + px_0;
		y1 = ((py_3 * t + py_2) * t + py_1) * t + py_0;
		drawLine(image, x0, y0, x1, y1, color);
		x0 = x1;
		y0 = y1;
	}
	drawLine(image, x0, y0, x3, y3, color);
}

void clipText(GxRect rect, GxImage font, const char *text) {
	char chr;
	int x = 0;
	int y = 0;
	int x0 = x;

	if ((font->flags & ImageType) != ImageFnt) {
		return;
	}
	if (font->basePtr == NULL) {
		return;
	}

	rect->w = 0;
	rect->h = font->height;
	if (text == NULL) {
		return;
	}

	while ((chr = *text++) != 0) {
		switch (chr) {
			default:
				x += font->LLUTPtr->data[chr & 0xff].width;
				break;

			case '\r':
				if (*text == '\n') {
					// windows style line end (CrLf)
					break;
				}
				// fall through
			case '\n':
				if (*text == '\0') {
					// new line at end ot text
					break;
				}
				y += font->height;
				x = x0;
				break;

				/* todo calculate correct position based on the current width
				case '\t':
					width += 8 * face->width;
					break;
				// */
		}

		if (rect->w < x) {
			rect->w = x;
		}
	}
	rect->h += y;
}
void drawChar(GxImage image, int x, int y, GxImage font, int chr, uint32_t color) {

	if ((font->flags & ImageType) != ImageFnt) {
		return;
	}

	struct GxFace *face = &font->LLUTPtr->data[chr & 255];
	struct GxRect clip;

	clip.x = x + face->pad_x;
	clip.y = y + face->pad_y;
	clip.w = face->width;
	clip.h = font->height;
	if (clip.w <= 0 || clip.h <= 0) {
		return;
	}

	char *sptr = (char*)face->basePtr;
	char *dptr = (char*) clipRect(image, &clip);
	bltProc blt = getBltProc(blt_set_mix, image->depth);
	if (dptr == NULL || sptr == NULL || blt == NULL) {
		return;
	}

	while (clip.h-- > 0) {
		blt(dptr, sptr, &color, clip.w);
		dptr += image->scanLen;
		sptr += font->scanLen;
	}
}
void drawText(GxImage image, int x, int y, GxImage font, const char *text, uint32_t color) {
	int chr, x0 = x;

	if ((font->flags & ImageType) != ImageFnt) {
		return;
	}
	if (!font->basePtr) {
		return;
	}
	if (text == NULL) {
		return;
	}

	while ((chr = *text++) != 0) {
		switch (chr) {
			default:
				drawChar(image, x, y, font, chr, color);
				x += font->LLUTPtr->data[chr].width;
				break;

			case '\r':
				if (*text == '\n') {
					// windows style line end (CrLf)
					break;
				}
				// fall through
			case '\n':
				if (*text == '\0') {
					// new line at end of text
					break;
				}
				y += font->height;
				x = x0;
				break;
		}
	}
}
