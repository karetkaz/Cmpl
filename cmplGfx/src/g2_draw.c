#include "gx_surf.h"

void fillRect(GxImage image, int x0, int y0, int x1, int y1, int incl, uint32_t color) {
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

	struct GxRect rect = {
		.x0 = x0,
		.y0 = y0,
		.x1 = x1 + incl,
		.y1 = y1 + incl,
	};

	bltProc blt = getBltProc(blt_set_col, image->depth);
	char *dptr = (char*) clipRect(image, &rect);
	if (dptr == NULL || blt == NULL) {
		return;
	}

	int width = rect.x1 - rect.x0;
	for (int y = rect.y0; y < rect.y1; ++y) {
		blt(dptr, &color, NULL, width);
		dptr += image->scanLen;
	}
}

void drawChar(GxImage image, int x0, int y0, GxImage font, int chr, uint32_t color) {

	if ((font->flags & ImageType) != ImageFnt) {
		return;
	}

	struct GxFace *face = &font->LLUTPtr->data[chr & 255];
	struct GxRect clip = {
		.x0 = x0 + face->pad_x,
		.y0 = y0 + face->pad_y,
		.x1 = clip.x0 + face->width,
		.y1 = clip.y0 + font->height,
	};

	bltProc blt = getBltProc(blt_set_mix, image->depth);
	char *dptr = (char *) clipRect(image, &clip);
	char *sptr = (char *) face->basePtr;

	if (dptr == NULL || sptr == NULL || blt == NULL) {
		return;
	}

	int width = clip.x1 - clip.x0;
	for (int y = clip.y0; y < clip.y1; ++y) {
		blt(dptr, &color, sptr, width);
		dptr += image->scanLen;
		sptr += font->scanLen;
	}
}

// todo: no unicode support, only ascii text
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

	rect->x1 = rect->x0;
	rect->y1 = rect->y0 + font->height;
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

		if (rect->x1 - rect->x0 < x) {
			rect->x1 = rect->x0 + x;
		}
	}
	rect->y1 += y;
}
void drawText(GxImage image, GxRect rect, GxImage font, const char *text, uint32_t color) {
	if ((font->flags & ImageType) != ImageFnt) {
		return;
	}
	if (!font->basePtr) {
		return;
	}
	if (text == NULL) {
		return;
	}

	int x = rect->x0;
	int y = rect->y0;
	for (int chr = *text; chr != 0; chr = *++text) {
		switch (chr) {
			default:
				drawChar(image, x, y, font, chr, color);
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
					// new line at end of text
					break;
				}
				y += font->height;
				x = rect->x0;
				break;
		}
	}
}
