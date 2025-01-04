#include "gx_color.h"
#include "gx_surf.h"

#include <stdlib.h>
#include <math.h>

GxImage createImage(GxImage recycle, int width, int height, int depth, ImageFlags flags) {
	if (recycle == NULL) {
		recycle = (GxImage) malloc(sizeof(struct GxImage));
		if (recycle == NULL) {
			return NULL;
		}
		flags |= freeImage;
	}
	else if (recycle->flags & freeData) {
		free(recycle->basePtr);
	}

	recycle->width = (uint16_t) width;
	recycle->height = (uint16_t) height;
	recycle->depth = (uint8_t) depth;
	recycle->flags = flags;
	recycle->basePtr = NULL;
	recycle->tempPtr = NULL;

	if (width != recycle->width || height != recycle->height) {
		// dimension did not fit into 16 bit integer
		destroyImage(recycle);
		return NULL;
	}

	switch (depth) {
		default:
			destroyImage(recycle);
			return NULL;

		case 32:
			recycle->pixelLen = 4;
			break;

		case 24:
			recycle->pixelLen = 3;
			break;

		case 16:
		case 15:
			recycle->pixelLen = 2;
			break;

		case 8:
			recycle->pixelLen = 1;
			break;
	}

	recycle->scanLen = ((unsigned) width * recycle->pixelLen + 3) & ~3;
	if (width > 0 && height > 0 && depth > 0) {
		size_t size = recycle->scanLen * (size_t) height;
		size_t offs = 0;

		switch (flags & ImageType) {
			default:
				break;

			case Image2d:
				break;

			case ImageIdx:
				offs = size;
				size += sizeof(struct GxCLut);
				break;

			case ImageFnt:
				offs = size;
				size += sizeof(struct GxFLut);
				break;

			case Image3d:
				offs = size;
				size += width * (size_t) height * 4;    // z-buffer
				break;
		}

		if ((recycle->basePtr = malloc(size)) == NULL) {
			destroyImage(recycle);
			return NULL;
		}

		recycle->flags |= freeData;
		if (offs != 0) {
			recycle->tempPtr = recycle->basePtr + offs;
		}
	}

	return recycle;
}

GxImage sliceImage(GxImage recycle, GxImage parent, const GxRect roi) {
	ImageFlags flags = parent->flags & ~(freeImage|freeData);
	if (recycle == NULL) {
		recycle = (GxImage) malloc(sizeof(struct GxImage));
		if (recycle == NULL) {
			return NULL;
		}
		flags |= freeImage;
	}
	else if (recycle->flags & freeData) {
		free(recycle->basePtr);
	}

	recycle->width = parent->width;
	recycle->height = parent->height;
	recycle->depth = parent->depth;
	recycle->flags = flags;
	recycle->pixelLen = parent->pixelLen;
	recycle->scanLen = parent->scanLen;
	recycle->basePtr = parent->basePtr;
	recycle->tempPtr = parent->tempPtr;

	if (roi != NULL) {
		struct GxRect rect = *roi;
		recycle->basePtr = clipRect(parent, &rect);
		recycle->width = rect.x1 < rect.x0 ? 0 : rect.x1 - rect.x0;
		recycle->height = rect.y1 < rect.y0 ? 0 : rect.y1 - rect.y0;
	}
	return recycle;
}

void destroyImage(GxImage image) {
	if (image->flags & freeData) {
		image->flags &= ~freeData;
		if (image->basePtr != NULL) {
			free(image->basePtr);
		}
	}
	image->basePtr = NULL;
	if (image->flags & freeImage) {
		image->flags &= ~freeImage;
		free(image);
	}
}

void* clipRect(GxImage image, GxRect roi) {
	if (roi->x0 < 0) {
		roi->x0 = 0;
	}
	if (roi->y0 < 0) {
		roi->y0 = 0;
	}
	int width = image->width;
	if (roi->x1 > width) {
		roi->x1 = width;
	}

	int height = image->height;
	if (roi->y1 > height) {
		roi->y1 = height;
	}

	if (rectEmpty(*roi)) {
		return NULL;
	}

	return refPixel(image, roi->x0, roi->y0);
}

int fillImage(GxImage image, GxRect roi, void *color, void *extra, bltProc blt) {
	if (blt == NULL) {
		// error: operation is invalid or not implemented
		return -1;
	}

	struct GxRect clip = {
		.x0 = roi ? roi->x0 : 0,
		.y0 = roi ? roi->y0 : 0,
		.x1 = roi ? roi->x1 : image->width,
		.y1 = roi ? roi->y1 : image->height,
	};

	char *dst = clipRect(image, &clip);
	if (dst == NULL) {
		// there are no pixels to set
		return 0;
	}

	int width = clip.x1 - clip.x0;
	for (int y = clip.y0; y < clip.y1; ++y) {
		if (blt(dst, color, extra, width) < 0) {
			return -1;
		};
		dst += image->scanLen;
	}
	return 0;
}

int copyImage(GxImage image, int x, int y, GxImage src, const GxRect roi, void *extra, bltProc blt) {
	if (blt == NULL) {
		// error: operation is invalid or not implemented
		return -1;
	}

	struct GxRect rect = {
		.x0 = roi ? roi->x0 : 0,
		.y0 = roi ? roi->y0 : 0,
		.x1 = roi ? roi->x1 : src->width,
		.y1 = roi ? roi->y1 : src->height,
	};
	if (x < 0) {
		rect.x0 -= x;
	}
	if (y < 0) {
		rect.y0 -= y;
	}
	char *sptr = clipRect(src, &rect);
	if (sptr == NULL) {
		// there is noting to copy
		return 0;
	}

	rectPosition(&rect, x < 0 ? 0 : x, y < 0 ? 0 : y);
	char *dptr = clipRect(image, &rect);
	if (dptr == NULL) {
		// there are no pixels to set
		return 0;
	}

	int width = rect.x1 - rect.x0;
	for (int n = rect.y0; n < rect.y1; ++n) {
		if (blt(dptr, sptr, extra, width) < 0) {
			return -1;
		}
		dptr += image->scanLen;
		sptr += src->scanLen;
	}
	return 0;
}

int transformImage(GxImage image, GxRect rect, GxImage src, GxRect roi, int interpolation, float mat[16]) {
	if (image->depth != src->depth) {
		return -2;
	}

	struct GxRect dstRec = {
		.x0 = rect ? rect->x0 : 0,
		.y0 = rect ? rect->y0 : 0,
		.x1 = rect ? rect->x1 : image->width,
		.y1 = rect ? rect->y1 : image->height,
	};
	struct GxRect srcRec = {
		.x0 = roi ? roi->x0 : 0,
		.y0 = roi ? roi->y0 : 0,
		.x1 = roi ? roi->x1 : src->width,
		.y1 = roi ? roi->y1 : src->height,
	};

	// convert floating point values to fixed point(16.16) values (scale + rotate + translate)
	int32_t xx = mat ? (int32_t) (mat[0] * 65535) : (rectWidth(srcRec) << 16) / rectWidth(dstRec);
	int32_t xy = mat ? (int32_t) (mat[1] * 65535) : 0;
	int32_t xt = (mat ? (int32_t) (mat[3] * 65535) : (srcRec.x0 << 16)) - xx * dstRec.x0;
	int32_t yy = mat ? (int32_t) (mat[5] * 65535) : (rectHeight(srcRec) << 16) / rectHeight(dstRec);
	int32_t yx = mat ? (int32_t) (mat[4] * 65535) : 0;
	int32_t yt = (mat ? (int32_t) (mat[7] * 65535) : (srcRec.y0 << 16)) - yy * dstRec.y0;

	if (!clipRect(image, &dstRec)) {
		// nothing to set
		return 0;
	}
	if (!clipRect(src, &srcRec)) {
		// nothing to get
		return 0;
	}

	if (interpolation == 0 || xx > 0x20000 || yy > 0x20000) {
		for (int y = dstRec.y0; y < dstRec.y1; ++y) {
			for (int x = dstRec.x0; x < dstRec.x1; ++x) {
				int32_t tx = (xx * x + xy * y + xt) >> 16;
				int32_t ty = (yx * x + yy * y + yt) >> 16;
				setPixel(image, x, y, getPixel(src, tx, ty));
			}
		}
		return 1;
	}

	xt -= 0x7fff;
	yt -= 0x7fff;
	for (int y = dstRec.y0; y < dstRec.y1; ++y) {
		for (int x = dstRec.x0; x < dstRec.x1; ++x) {
			int32_t tx = xx * x + xy * y + xt;
			int32_t ty = yx * x + yy * y + yt;
			tx *= !(tx < 0 && tx > -0x8000);
			ty *= !(ty < 0 && ty > -0x8000);
			setPixel(image, x, y, getPixelLinear(src, tx, ty));
		}
	}
	return 1;
}

static inline double gauss(double x, double sigma) {
	double t = x / sigma;
	double SQRT_2_PI_INV = 0.398942280401432677939946059935;
	return SQRT_2_PI_INV * exp(-0.5 * t * t) / sigma;
}

int blurImage(GxImage image, int radius, double sigma) {

	int size = radius * 2 + 1;
	if (image->depth != 32) {
		return -1;
	}
	if (size >= 1024) {
		return -1;
	}
	if (size < 1) {
		return 0;
	}

	double kernelSum = 0;
	double kernelFlt[1024];
	for (int i = 0; i < size; i += 1) {
		kernelFlt[i] = gauss(radius - i, sigma);
		kernelSum += kernelFlt[i];
	}

	uint32_t kernel[1024];
	for (int i = 0; i < size; i++) {
		kernel[i] = 65536 * (kernelFlt[i] / kernelSum);
	}

	int width = image->width;
	int height = image->height;
	GxImage tmp = createImage(NULL, width, height, image->depth, Image2d);

	if (tmp == NULL) {
		return -1;
	}

	// x direction: inout -> temp
	for (int y = 0; y < height; y += 1) {
		for (int x = 0; x < width; x += 1) {
			uint32_t r = 0;
			uint32_t g = 0;
			uint32_t b = 0;
			for (int i = 0; i < size; i += 1) {
				int _x = x + i - radius;
				if (_x >= 0 && _x < width) {
					uint32_t col = getPixel(image, _x, y);
					int32_t _k = kernel[i];
					r += _k * rch(col);
					g += _k * gch(col);
					b += _k * bch(col);
				}
			}
			setPixel(tmp, x, y, sat_rgb(255, r >> 16, g >> 16, b >> 16).val);
		}
	}

	// y direction: temp -> inout
	for (int y = 0; y < height; y += 1) {
		for (int x = 0; x < width; x += 1) {
			uint32_t r = 0;
			uint32_t g = 0;
			uint32_t b = 0;
			for (int i = 0; i < size; i += 1) {
				int _y = y + i - radius;
				if (_y >= 0 || _y < height) {
					uint32_t col = getPixel(tmp, x, _y);
					int32_t _k = kernel[i];
					r += _k * rch(col);
					g += _k * gch(col);
					b += _k * bch(col);
				}
			}
			setPixel(image, x, y, sat_rgb(255, r >> 16, g >> 16, b >> 16).val);
		}
	}
	destroyImage(tmp);
	return 0;
}
