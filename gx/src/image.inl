#include <stdio.h>
#include <string.h>
#include "g2_surf.h"

#define BMP_RLE 0x01
#define BMP_OS2 0x02

#pragma pack ( push, 1 )
typedef struct {	// BMP_HDR
	unsigned short	manfact;
	unsigned long	filesize;
	unsigned long	version;
	unsigned long	imgstart;
	unsigned long	hdrsize;
} BMP_HDR;

typedef struct {	// BMP_INF
	unsigned long	width;
	unsigned long	height;
	unsigned short	planes;
	unsigned short	depth;
	unsigned long	encoding;		// 0:no compression;1: RLE 8 bit;2: RLE 4 bit
	unsigned long	imgsize;
	unsigned long	hdpi;
	unsigned long	vdpi;
	unsigned long	usedcol;
	unsigned long	impcol;
	unsigned char	b_enc;
	unsigned char	g_enc;
	unsigned char	r_enc;
	unsigned char	filler;
	// os/2 2+
	unsigned char	res_889[24];
} BMP_INF;

typedef struct {	// BMP_INF2
	unsigned short	width;
	unsigned short	height;
	unsigned short	planes;
	unsigned short	depth;
}BMPINF2;

typedef struct {	// CUR_HDR
	unsigned short	idreserved;
	unsigned short	idtype;
	unsigned short	idcount;
} CUR_HDR;

typedef struct {	// CUR_DIR		CURSORDIRENTRY
	unsigned char	width;
	unsigned char	height;
	unsigned char	colorcount;
	unsigned char	breserved;
	unsigned short	hotspotx;
	unsigned short	hotspoty;
	unsigned long	lbytesinres;
	unsigned long	imageoffset;
} CUR_DIR;

typedef struct {	// CUR_BMP		BITMAPINFOHEADER
	unsigned long	size;
	signed   long	width;
	signed   long	height;
	unsigned short	planes;
	unsigned short	depth;
	unsigned long	encoding;
	unsigned long	imgsize;
	signed   long	hdpi;
	signed   long	vdpi;
	unsigned long	usedcol;
	unsigned long	impcol;
} CUR_BMP;

#pragma pack(pop)

typedef int (*bmplnreader)(unsigned char*, FILE*, unsigned);
typedef int (*bmplnwriter)(FILE*, unsigned char*, unsigned);

static inline size_t FRead(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	int cnt = fread(ptr, size, nmemb, stream);
	if (nmemb != cnt)
		gx_debug("xxx");
	return cnt;
}

//	Bitmap line readers
static int bmprln_raw(unsigned char* dst, FILE* src, unsigned cnt) {
	return !fread(dst, cnt, 1, src);
}

static int bmprln_01raw(unsigned char* dst, FILE* src, unsigned cnt) {
	unsigned char* tmp = dst + (cnt<<3) - cnt;

	if (!fread(tmp, cnt, 1, src))
		return 0;

	while (cnt--) {
		*dst++ = (*tmp >> 7) & 0x01;
		*dst++ = (*tmp >> 6) & 0x01;
		*dst++ = (*tmp >> 5) & 0x01;
		*dst++ = (*tmp >> 4) & 0x01;
		*dst++ = (*tmp >> 3) & 0x01;
		*dst++ = (*tmp >> 2) & 0x01;
		*dst++ = (*tmp >> 1) & 0x01;
		*dst++ = (*tmp >> 0) & 0x01;
		tmp++;
	}
	return 1;
}

static int bmprln_04raw(unsigned char* dst, FILE* src, unsigned cnt) {
	unsigned char* tmp = dst + (cnt);

	if (!fread(tmp, cnt, 1, src))
		return 0;

	while (cnt--) {
		*dst++ = (*tmp >> 4) & 0x0f;
		*dst++ = (*tmp >> 0) & 0x0f;
		tmp++;
	}
	return 1;
}

//~ static int bmprln_01rle(unsigned char* dst, FILE* src, unsigned cnt) ;

static int bmprln_04rle(unsigned char* dst, FILE* src, unsigned cnt) {
	for (;;) {
		if (fread(dst, 2, 1, src) == 0)
			return 0;
		if (dst[0] == 0) {
			switch (dst[1]) {
				case 0  : return 1;				// eol
				case 1  : return 1;				// eof
				case 2  : fseek(src,2,SEEK_CUR); break;		// jmp
				default : {
					int pos = 0;
					unsigned char* tmp = dst + ((cnt = dst[1]) >> 0);
					FRead(tmp, ((cnt+3)>>1)&(~1), 1, src);
					while(cnt--) {
						*dst++ = (*tmp >> (pos = 4-pos)) & 0x0f;
						if(!pos)tmp++;
					}
				}
				break;
			}
		}else{								// fill cnt pixels
			int pos=0;
			int tmp = dst[1];
			cnt = dst[0];
			while(cnt--){
				*dst++ = (tmp >> (pos = 4-pos)) & 0x0f;
			}
		}
	}
	return 1;
}

static int bmprln_08rle(unsigned char* dst, FILE* src, unsigned cnt) {
	for(;;){
		if(fread(dst, 2, 1, src) == 0)return 0;
		if(*dst == 0){							// read n pixels
			switch(dst[1]){
				case 0  : return 1;				// eol
				case 1  : return 1;				// eof
				case 2  : fseek(src,2,SEEK_CUR); break;	// jmp
				default : {
					unsigned char* tmp = dst + ((cnt = dst[1])>>1);
					FRead(tmp, (cnt+1)&(~1), 1, src);
					while(cnt--){
						*dst++ = *tmp++;
					}
				}
				break;
			}
		}else{								// fill n pixels
			int tmp = dst[1];
			cnt = dst[0];
			while(cnt--){
				*dst++ = tmp;
			}
		}
	}
	return 1;
}

//	Bitmap line writers
static int bmpwln_raw(FILE* dst, unsigned char* src, unsigned cnt) {
	return !fwrite(src, cnt, 1, dst);
}

static int bmpwln_01raw(FILE* dst, unsigned char* src, unsigned cnt) {
	while (cnt--) {
		int tmp = 0;
		tmp |= (*src++ & 0x01) << 7;
		tmp |= (*src++ & 0x01) << 6;
		tmp |= (*src++ & 0x01) << 5;
		tmp |= (*src++ & 0x01) << 4;
		tmp |= (*src++ & 0x01) << 3;
		tmp |= (*src++ & 0x01) << 2;
		tmp |= (*src++ & 0x01) << 1;
		tmp |= (*src++ & 0x01) << 0;
		fputc(tmp,dst);
	}
	return 1;
}

static int bmpwln_04raw(FILE* dst, unsigned char* src, unsigned cnt) {
	while (cnt--) {
		int tmp = 0;
		tmp |= (*src++ & 0x0f) << 4;
		tmp |= (*src++ & 0x0f) << 0;
		fputc(tmp,dst);
	}
	return 1;
}

static int bmpwln_04rle(FILE* dst, unsigned char* src, unsigned cnt) {return -1;} // TODO
static int bmpwln_08rle(FILE* dst, unsigned char* src, unsigned cnt) {return -1;} // TODO

//	open / save image formats

int gx_readBMP(gx_Surf dst, FILE* fin, int depth) {
	struct gx_Clut bmppal;			// bitmap palette
	bmplnreader	bmplrd;			// bitmap line reader
	BMP_HDR		bmphdr;			// bitmap header
	BMP_INF		bmpinf;			// bitmap info header
	int			bmplsz;			// bitmap line size
	char*		ptr;			// ptr to surface line
	char	tmpbuff[65535*4];	// bitmap temp buffer
	cblt_proc	convln;			// color convereter rutine

	// check for valid depths
	switch (depth) {
		default:
			return 7;
		case 32:
		case 24:
		case 16:
		case 15:
		case 8:
			break;
	}

	FRead(&bmphdr, sizeof(BMP_HDR), 1, fin);
	switch (bmphdr.manfact) {		// Validid Bitmap Formats
		//~ case 'MB' : break;		// BM -               
		//~ case 'AB' : break;		// BA - Bitmap Array
		//~ case 'IC' : break;		// CI - Cursor Icon
		//~ case 'PC' : break;		// CP -
		//~ case 'CI' : break;		// IC -
		//~ case 'TP' : break;		// PT - Pointer

		case 0x4d42: break;		// BM -               
		case 0x4142: break;		// BA - Bitmap Array
		case 0x4943: break;		// CI - Cursor Icon
		case 0x5043: break;		// CP -
		case 0x4349: break;		// IC -
		case 0x5450: break;		// PT - Pointer

		default   : return 2;
	}
	FRead(&bmpinf, bmphdr.hdrsize-4, 1, fin);
	switch (bmphdr.hdrsize) {
		/*case 12: {
			bmpinf.depth = (*(BMPINF2*)&bmpinf).depth;
			bmpinf.planes = (*(BMPINF2*)&bmpinf).planes;
			bmpinf.height = (*(BMPINF2*)&bmpinf).height;
			bmpinf.width = (*(BMPINF2*)&bmpinf).width;
		} break;			// */// OS/2 1.x;
		case 40: break;		// Windows;
		case 64: break;		// OS/2 2+;
		default: return 3;
	}
	switch (bmpinf.depth) {
		case  1: {
			FRead(&bmppal.data, sizeof(argb), 2, fin);
			bmplrd = bmprln_01raw;
			bmppal.count = 2;
		} break;
		case  4: {
			FRead(&bmppal.data, sizeof(argb),  16, fin);
			bmplrd = bmprln_04raw;
			bmppal.count = 16;
		} break;
		case  8: {
			FRead(&bmppal.data, sizeof(argb), 256, fin);
			bmplrd = bmprln_raw;
			bmppal.count = 256;
		} break;
		case 24: bmplrd = bmprln_raw; break;
		case 32: bmplrd = bmprln_raw; break;
		default: return 4;
	}
	switch (bmpinf.encoding) {
		case  0: break;		// no compression
		case  1: bmplrd = bmprln_08rle; break;
		case  2: bmplrd = bmprln_04rle; break;
		default: return 5;
	}
	if (!(convln = gx_getcbltf(cblt_conv, depth, bmpinf.depth < 8 ? 8 : bmpinf.depth))) {
		return 6;
	}
	if (gx_initSurf(dst, bmpinf.width, bmpinf.height, depth, Surf_recycle)) {
		return 8;
	}
	if (depth == 8) {
		dst->CLUTPtr = &bmppal;
	}

	bmplsz = ((bmpinf.depth * bmpinf.width >> 3) + 3) & (~3);
	ptr = (char*)dst->basePtr + dst->scanLen * dst->height;
	fseek(fin, bmphdr.imgstart, SEEK_SET);
	while (bmpinf.height--) {
		ptr -= dst->scanLen;
		bmplrd((unsigned char*)tmpbuff, fin, bmplsz);
		gx_callcbltf(convln, ptr, tmpbuff, bmpinf.width, &bmppal);
	}
	return 0;
}

int gx_loadBMP(gx_Surf surf, const char* src, int depth) {
	int res;
	FILE *fin = fopen(src, "rb");
	if (!fin) return -1;
	res = gx_readBMP(surf, fin, depth);
	fclose(fin);
	return res;
}

int gx_saveBMP(const char* dst, gx_Surf src, int flags) {
	struct gx_Clut bmppal;			// bitmap palette
	bmplnwriter	bmplwr;			// bitmap line writer
	BMP_HDR		bmphdr;			// bitmap header
	BMP_INF		bmpinf;			// bitmap info header
	int		bmplsz;			// bitmap line size
	FILE*		fout;			// bitmap output file
	char	tmpbuff[65535*4];	// bitmap temp buffer
	char*		ptr;			// surface line ptr
	cblt_proc	convln;			// color convereter rutine
	memset(&bmphdr, 0, sizeof(BMP_HDR));
	memset(&bmpinf, 0, sizeof(BMP_INF));
	memset(&bmppal, 0, sizeof(struct gx_Clut));
	flags &= ~BMP_RLE;				// not implemented	//TODO
	switch (src->depth) {
		case  8 : {
			/*if (src->clutPtr) {
				memcpy(&bmppal, src->clutPtr, sizeof(gx_Clut));
				if (bmppal.count <= 16)
					if (bmppal.count == 2)
						bmpinf.depth = 1;
					else bmpinf.depth = 4;
				else bmpinf.depth = 8;
			}
			else // */
			{
				for (bmplsz = 0; bmplsz < 256; ++bmplsz) {
					bmppal.data[bmplsz].a = 255;
					bmppal.data[bmplsz].r = bmplsz;
					bmppal.data[bmplsz].g = bmplsz;
					bmppal.data[bmplsz].b = bmplsz;
				}
				bmppal.count = 256;
				bmpinf.depth = 8;
			}
			switch (bmpinf.depth) {
				case 1 : bmplwr = bmpwln_01raw; break;
				case 4 : {
					if (flags & BMP_RLE) {
						bmpinf.encoding = 2;
						bmplwr = bmpwln_04rle;
					}
					else
						bmplwr = bmpwln_04raw;
				} break;
				case 8 : {
					if (flags & BMP_RLE) {
						bmpinf.encoding = 1;
						bmplwr = bmpwln_08rle;
					}
					else
						bmplwr = bmpwln_raw;
				} break;
			}
		}break;// */
		case 15 :
		case 16 :
		case 24 :
		case 32 : bmpinf.depth = 24; bmplwr = bmpwln_raw; break;
		default : return 2;
	}
	bmpinf.width = src->width;
	bmpinf.height = src->height;
	bmpinf.planes = 1;
	bmpinf.hdpi = bmpinf.vdpi = 3779;
	bmpinf.imgsize = (bmplsz = ((bmpinf.depth * bmpinf.width >> 3) + 3) & (~3)) * bmpinf.height;
	bmphdr.manfact = 0x4d42; //'MB';
	bmphdr.hdrsize = (flags & BMP_OS2) ? 64 : 40;
	bmphdr.imgstart = sizeof(BMP_HDR)-4 + bmphdr.hdrsize + (bmppal.count << 2);
	bmphdr.filesize = bmphdr.imgstart + bmpinf.imgsize;
	if (!(convln = gx_getcbltf(cblt_conv, bmpinf.depth < 8 ? 8 : bmpinf.depth, src->depth))) return 3;
	if (!(fout = fopen(dst,"wb"))) return 1;
	fwrite(&bmphdr, sizeof(BMP_HDR), 1, fout);
	fwrite(&bmpinf, bmphdr.hdrsize-4, 1, fout);
	fwrite(&bmppal.data, bmppal.count, 4, fout);
	ptr = (char*)src->basePtr + src->height * src->scanLen;
	while (bmpinf.height--) {
		gx_callcbltf(convln, tmpbuff, ptr -= src->scanLen, bmpinf.width, &bmppal);
		bmplwr(fout, (unsigned char*)tmpbuff, bmplsz);
	}
	fclose(fout);
	return 0;
}

/*int gx_loadCUR(gx_Surf dst, const char* src, int flags) {
	FILE		*fin;
	CUR_HDR		curhdr;
	CUR_DIR		curdir;
	CUR_BMP		curbmp;
	struct gx_Clut bmppal;			// bitmap palette
	char		*ptr, *tmp;		// ptr to surface line
	int x, y, c, i = 0;
	if (!(fin = fopen(src,"rb"))) return 1;
	fread(&curhdr, sizeof(CUR_HDR), 1, fin);
	fseek(fin, sizeof(CUR_DIR)*(flags & 0XFF), SEEK_CUR);
	fread(&curdir, sizeof(CUR_DIR), 1, fin);
	fseek(fin, curdir.imageoffset, SEEK_SET);
	fread(&curbmp, sizeof(CUR_BMP), 1, fin);
	if (curbmp.encoding) {
		fclose(fin);
		return 5;
	}
	dst->depth  = 32;
	dst->width  = curdir.width;
	dst->height = curdir.height;
	dst->scanLen = curbmp.width * 8;
	if (gx_initSurf(dst, 0, 0, SURF_DNTCLR)) {
		fclose(fin);
		return 8;
	}
	tmp = ptr = (char*)dst->basePtr + dst->scanLen * dst->height;
	dst->scanLen = curbmp.width * 4;
	dst->tempPtr = (char*)dst->basePtr + dst->scanLen * dst->height;
	dst->flags |= Surf_cur;
	dst->hotSpot.x = curdir.hotspotx;
	dst->hotSpot.y = curdir.hotspoty;
	switch (curbmp.depth) {
		case  1 : {
			fread(&bmppal.data, sizeof(argb), 2, fin);
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x, ++i) {
					if ((i & 7) == 0) c = fgetc(fin); else c <<= 1;
					((argb*)ptr)[x] = bmppal.data[(c>>7) & 0x01];
				}
			}
		} break;
		case  4 : {
			fread(&bmppal.data, sizeof(argb),  16, fin);
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x, ++i) {
					if ((i & 1) == 0) c = fgetc(fin); else c <<= 4;
					((argb*)ptr)[x] = bmppal.data[(c>>4) & 0x0f];
				}
			}
		} break;
		case  8 : {
			fread(&bmppal.data, sizeof(argb), 256, fin);
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x) {
					c = fgetc(fin);
					((argb*)ptr)[x] = bmppal.data[c];
				}
			}
		} break;
		case 24 : {
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x) {
					((argb*)ptr)[x].b = fgetc(fin);
					((argb*)ptr)[x].g = fgetc(fin);
					((argb*)ptr)[x].r = fgetc(fin);
					((argb*)ptr)[x].a = 0;
				}
			}
		} break;
		case 32 : {
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x) {
					((argb*)ptr)[x].b = fgetc(fin);
					((argb*)ptr)[x].g = fgetc(fin);
					((argb*)ptr)[x].r = fgetc(fin);
					((argb*)ptr)[x].a = fgetc(fin);
				}
			}
		} break;
		default : fclose(fin); return 4;
	}
	if (curbmp.depth != 32) {
		ptr = tmp;
		for(i = y = 0; y < curdir.height; ++y) {
			ptr -= dst->scanLen;
			for(x = 0; x < curdir.width; ++x, ++i) {
				if ((i & 7) == 0) c = fgetc(fin); else c <<= 1;
				((argb*)ptr)[x].a = ((c>>7) & 0x01) ? 0 : 255;
			}
		}
	}
	fclose(fin);
	return 0;
}// */

#if 0 || !defined(_MSC_VER)

//~ int conv_rgb2xrgb(void* dst, void* src, int cnt){}
/* reciclable
static void conv_1bw2xrgb(char* dst, char* src, int cnt) {
	while (cnt >= 0) {
		int col = *src;
		dst[0] = dst[1] = dst[2] = ~(((col >> 7) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 6) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 5) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 4) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 3) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 2) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 1) & 0x01) -1);
		dst += 4;
		dst[0] = dst[1] = dst[2] = ~(((col >> 0) & 0x01) -1);
		dst += 4;
		src += 1;
		cnt -= 8;
	}
}
static void conv_2bw2xrgb(char* dst, char* src, int cnt) {
	while (cnt >= 0) {
		int col = *src;
		dst[0] = dst[1] = dst[2] = ((col >> 6) & 0x03) << 6;
		dst += 4;
		dst[0] = dst[1] = dst[2] = ((col >> 4) & 0x03) << 6;
		dst += 4;
		dst[0] = dst[1] = dst[2] = ((col >> 2) & 0x03) << 6;
		dst += 4;
		dst[0] = dst[1] = dst[2] = ((col >> 0) & 0x03) << 6;
		dst += 4;
		src += 1;
		cnt -= 4;
	}
}
static void conv_4bw2xrgb(char* dst, char* src, int cnt) {
	while (cnt >= 0) {
		int col = *src;
		dst[0] = dst[1] = dst[2] = ((col >> 4) & 0x0f) << 4;
		dst += 4;
		dst[0] = dst[1] = dst[2] = ((col >> 0) & 0x0f) << 4;
		dst += 4;
		src += 1;
		cnt -= 2;
	}
}
// */
static void conv_8bw2xrgb(unsigned char* dst, unsigned char* src, int cnt) {
	while (cnt >= 0) {
		dst[0] = dst[1] = dst[2] = src[0];
		dst += 4;
		src += 1;
		cnt -= 1;
	}
}
static void conv_bgr2xrgb(unsigned char* dst, unsigned char* src, int cnt) {
	while (cnt >= 0) {
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst += 4;
		src += 3;
		cnt -= 1;
	}
}
static void conv_abgr2argb(unsigned char* dst, unsigned char* src, int cnt) {
	while (cnt >= 0) {
		//*(int*)dst = *(int*)src;
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst[3] = src[3];
		dst += 4;
		src += 4;
		cnt -= 1;
	}
}
#pragma pack(push, 8)
#pragma GCC diagnostic ignored "-Wpadded"
#include "jpeg.h"
int gx_loadJPG(gx_Surf dst, const char* src, int depth) {
	void (*conv_2xrgb)(unsigned char* dst, unsigned char* src, int cnt) = NULL;
	unsigned char buff[65535*4];		// bitmap temp buffer
	unsigned char *tmpbuff = buff;
	unsigned char* ptr;			// ptr to surface line
	FILE*		fin;

	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr jerr;
	/* More stuff */

	/* In this example we want to open the input file before doing anything else,
	* so that the setjmp() error recovery below can assume the file is open.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to read binary files.
	*/
	if ((fin = fopen(src, "rb")) == NULL)
		return -1;
	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, fin);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */
	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */
	(void) jpeg_start_decompress(&cinfo);

	if (depth == 32) switch (cinfo.jpeg_color_space) {
		case JCS_UNKNOWN:	/* error/unspecified */
			break;

		case JCS_GRAYSCALE:	/* monochrome */
			conv_2xrgb = conv_8bw2xrgb;
			break;

		case JCS_RGB:		/* red/green/blue */
		case JCS_YCbCr:		/* Y/Cb/Cr (also known as YUV) */
		case JCS_CMYK:		/* C/M/Y/K */
		case JCS_YCCK:		/* Y/Cb/Cr/K */
			conv_2xrgb = conv_bgr2xrgb;
			break;
	}
	if (!conv_2xrgb) {
		fclose(fin);
		return 6;
	}

	if (gx_initSurf(dst, cinfo.output_width, cinfo.output_height, depth, Surf_recycle)) {
		fclose(fin);
		return 8;
	}
	/* Step 6: while (scan lines remain to be read) */
	ptr = (void*)dst->basePtr;
	while (cinfo.output_scanline < cinfo.output_height) {
		(void) jpeg_read_scanlines(&cinfo, &tmpbuff, 1);
		conv_2xrgb(ptr, tmpbuff, dst->width);
		//~ gx_callcbltf(convln, ptr, tmpbuff, dst->width, NULL);
		ptr += dst->scanLen;
	}

	/* Step 7: Finish decompression */
	(void) jpeg_finish_decompress(&cinfo);

	/* Step 8: Release JPEG decompression object */
	jpeg_destroy_decompress(&cinfo);

	fclose(fin);
	return 0;
}
#pragma pack ( pop )

#include "png.h"
int gx_loadPNG(gx_Surf dst, const char* src, int depth) {
	void (*conv_2xrgb)(unsigned char* dst, unsigned char* src, int cnt) = NULL;
	unsigned char tmpbuff[65535*4];	// bitmap temp buffer
	unsigned char* ptr;			// ptr to surface line
	FILE*		fin;

	int			pngwidth, pngheight, pngdepth = 0;
	int			color_type, bit_depth;

	// 8 is the maximum size that can be checked
	png_structp png_ptr;
	png_infop info_ptr;
	int y, number_of_passes;
	unsigned char header[8];

	/* open file and test for it being a png */
	if ((fin = fopen(src, "rb")) == NULL)
		return -1;

	y = fread(header, 1, 8, fin);

	if (y != 8 || png_sig_cmp(header, 0, 8))
		return -2;

	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		//~ abort_("[read_png_file] png_create_read_struct failed");
		fclose(fin);
		return -3;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		//~ abort_("[read_png_file] png_create_info_struct failed");
		fclose(fin);
		return -4;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		//~ abort_("[read_png_file] Error during init_io");
		fclose(fin);
		return -5;
	}

	png_init_io(png_ptr, fin);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	pngwidth = png_get_image_width(png_ptr, info_ptr);
	pngheight = png_get_image_height(png_ptr, info_ptr);
	//~ color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	pngdepth = bit_depth * png_get_channels(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	//gx_destroySurf(dst, 1);
	if (gx_initSurf(dst, pngwidth, pngheight, depth, Surf_recycle)) {
		fclose(fin);
		return 8;
	}

	// todo: if palette is present ????
	if (depth == 32) switch (pngdepth) {
		//~ case 1: conv_2xrgb = conv_1bw2xrgb; break;
		//~ case 2: conv_2xrgb = conv_2bw2xrgb; break;
		//~ case 4: conv_2xrgb = conv_4bw2xrgb; break;
		//~ case 8: conv_2xrgb = conv_gray2xrgb; break;
		default: conv_2xrgb = conv_bgr2xrgb; break;
		case 32: conv_2xrgb = conv_abgr2argb; break;
	}
	if (!conv_2xrgb) {
		fclose(fin);
		return 6;
	}// */

	number_of_passes = png_set_interlace_handling(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	//~ if (bit_depth == 16)
		//~ png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	//~ png_set_bgr(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr))) {
		//~ abort_("[read_png_file] Error during read_image");
		fclose(fin);
		return -6;
	}
	while (number_of_passes > 0) {
		ptr = (void*)dst->basePtr;
		for (y = 0; y < dst->height; y += 1) {
			png_read_row(png_ptr, tmpbuff, NULL);
			conv_2xrgb(ptr, tmpbuff, dst->width);
			ptr += dst->scanLen;
		}
		number_of_passes -= 1;
	}

	png_read_end(png_ptr, NULL);
	//png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	fclose(fin);
	return 0;
}

#else
int gx_loadJPG(gx_Surf dst, const char* src, int depth) {return -1;}
int gx_loadPNG(gx_Surf dst, const char* src, int depth) {return -1;}

#endif
//##############################################################################
