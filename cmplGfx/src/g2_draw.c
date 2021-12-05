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

	struct GxRect clip = {
			.x = x0,
			.y = y0,
			.w = x1 - x0 + incl,
			.h = y1 - y0 + incl,
	};
	bltProc blt = getBltProc(blt_set_col, image->depth);
	char *dptr = (char*) clipRect(image, &clip);
	if (dptr == NULL || blt == NULL) {
		return;
	}

	while (clip.h-- > 0) {
		blt(dptr, &color, NULL, clip.w);
		dptr += image->scanLen;
	}
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
		blt(dptr, &color, sptr, clip.w);
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
