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
	char *dptr = clipRect(image, &rect);
	if (dptr == NULL || blt == NULL) {
		return;
	}

	int width = rect.x1 - rect.x0;
	for (int y = rect.y0; y < rect.y1; ++y) {
		blt(dptr, &color, NULL, width);
		dptr += image->scanLen;
	}
}

int drawChar(GxImage image, int x0, int y0, GxImage font, uint32_t chr, uint32_t color) {
	if ((font->flags & ImageType) != ImageFnt) {
		return 0;
	}
	if (chr > 255) {
		// glyph missing from the cache, render it directly from the backing font
		return drawGlyph(image, x0, y0, font, chr, color);
	}

	struct GxFace *face = &font->LLUTPtr->data[chr];
	if (image == NULL) {
		// measure only
		return face->width;
	}

	struct GxRect clip = {
		.x0 = x0 + face->pad_x,
		.y0 = y0 + face->pad_y,
		.x1 = x0 + face->pad_x + face->width,
		.y1 = y0 + face->pad_y + font->height,
	};

	bltProc blt = getBltProc(blt_set_mix, image->depth);
	char *dptr = clipRect(image, &clip);
	char *sptr = face->basePtr;

	if (dptr == NULL || sptr == NULL || blt == NULL) {
		return face->width;
	}

	int width = clip.x1 - clip.x0;
	for (int y = clip.y0; y < clip.y1; ++y) {
		blt(dptr, &color, sptr, width);
		dptr += image->scanLen;
		sptr += font->scanLen;
	}
	return face->width;
}

void clipText(GxRect rect, GxImage font, const uint32_t *codepage, const char *text, size_t length) {
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

	int x = 0;
	int y = 0;

	for (size_t i = 0; i < length; ) {
		uint32_t chr;
		if (codepage != NULL) {
			chr = codepage[(uint8_t) text[i++]];
		}
		else {
			chr = decodeUtf8(text, length, &i);
		}
		switch (chr) {
			default:
				// measure only
				x += drawChar(NULL, 0, 0, font, chr, 0);
				break;

			case 0:
				return;

			case '\r':
				if (text[i] == '\n') {
					// windows style line end (CrLf)
					break;
				}
				// fall through
			case '\n':
				if (text[i] == '\0') {
					// new line at end ot text
					break;
				}
				y += font->height;
				x = 0;
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
void drawText(GxImage image, GxRect rect, GxImage font, const uint32_t *codepage, const char *text, size_t length, uint32_t color) {
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
	for (size_t i = 0; i < length; ) {
		uint32_t chr;
		if (codepage != NULL) {
			chr = codepage[(uint8_t) text[i++]];
		}
		else {
			chr = decodeUtf8(text, length, &i);
		}
		switch (chr) {
			default:
				x += drawChar(image, x, y, font, chr, color);
				break;

			case 0:
				return;

			case '\r':
				if (text[i] == '\n') {
					// windows style line end (CrLf)
					break;
				}
				// fall through
			case '\n':
				if (text[i] == '\0') {
					// new line at end of text
					break;
				}
				y += font->height;
				x = rect->x0;
				break;
		}
	}
}
