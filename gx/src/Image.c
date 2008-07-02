#include <stdio.h>
#include <string.h>
#include "gx_surf.h"

#define BMP_RLE 0x01
#define BMP_OS2 0x02

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

#pragma pack(1)

typedef int (*bmplnreader)(unsigned char*, FILE*, unsigned);
typedef int (*bmplnwriter)(FILE*, unsigned char*, unsigned);

//	Bitmap line readers
static int bmprln_raw(unsigned char* dst, FILE* src, unsigned cnt) {
	return !fread(dst, cnt, 1, src);
}

static int bmprln_01raw(unsigned char* dst, FILE* src, unsigned cnt) {
	unsigned char* tmp = dst + (cnt<<3) - cnt;
	if(!fread(tmp, cnt, 1, src))return 0;
	while(cnt--){
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
	if(!fread(tmp, cnt, 1, src))return 0;
	while(cnt--){
		*dst++ = (*tmp >> 4) & 0x0f;
		*dst++ = (*tmp >> 0) & 0x0f;
		tmp++;
	}
	return 0;
}

static int bmprln_04rle(unsigned char* dst, FILE* src, unsigned cnt) {
	for(;;){
		if(fread(dst, 2, 1, src) == 0)return 0;
		if(dst[0] == 0){
			switch(dst[1]){
				case 0  : return 1;				// eol
				case 1  : return 1;				// eof
				case 2  : fseek(src,2,SEEK_CUR); break;		// jmp
				default : {
					int pos = 0;
					unsigned char* tmp = dst + ((cnt = dst[1]) >> 0);
					fread(tmp, ((cnt+3)>>1)&(~1), 1, src);
					while(cnt--){
						*dst++ = (*tmp >> (pos = 4-pos)) & 0x0f;
						if(!pos)tmp++;
					}
				}
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
					fread(tmp, (cnt+1)&(~1), 1, src);
					while(cnt--){
						*dst++ = *tmp++;
					}
				}
			}
		}else{								// fill n pixels
			int tmp = dst[1];
			cnt = dst[0];
			while(cnt--){
				*dst++ = tmp;
			}
		}
	}
}

//	Bitmap line writers
static int bmpwln_raw(FILE* dst, unsigned char* src, unsigned cnt) {
	return !fwrite(src, cnt, 1, dst);
}

static int bmpwln_01raw(FILE* dst, unsigned char* src, unsigned cnt) {
	//~ unsigned char* tmp;
	while(cnt--){
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
	while(cnt--){
		int tmp = 0;
		tmp |= (*src++ & 0x0f) << 4;
		tmp |= (*src++ & 0x0f) << 0;
		fputc(tmp,dst);
	}
	return 1;
}

static int bmpwln_04rle(FILE* dst, unsigned char* src, unsigned cnt) {		//TODO
	return -1;		// implement
}

static int bmpwln_08rle(FILE* dst, unsigned char* src, unsigned cnt) {		//TODO
	return -1;		// implement
}

//	open / save image formats
int gx_loadBMP(gx_Surf dst, char* src, int depth) {
	gx_Clut_s	bmppal;			// bitmap palette
	bmplnreader	bmplrd;			// bitmap line reader
	BMP_HDR		bmphdr;			// bitmap header
	BMP_INF		bmpinf;			// bitmap info header
	int		bmplsz;			// bitmap line size
	FILE*		fin;			// bitmap input file
	char*		ptr;			// ptr to surface line
	char*		tmpbuff;		// bitmap temp buffer
	cblt_proc	convln;			// color convereter rutine
	switch (depth) {
		case  8 :
		case 15 :
		case 16 :
		case 24 :
		case 32 :break;
		//~ case -1 : depth = 32; break;
		default : return 7;
	}
	if (!(fin = fopen(src,"rb"))) return 1;
	fread(&bmphdr, sizeof(BMP_HDR), 1, fin);
	switch (bmphdr.manfact) {		// Validid Bitmap Formats
		case 'MB' : break;		// BM -
		case 'AB' : break;		// BA - Bitmap Array
		case 'IC' : break;		// CI - Cursor Icon
		case 'PC' : break;		// CP -
		case 'CI' : break;		// IC -
		case 'TP' : break;		// PT - Pointer
		default   : fclose(fin); return 2;
	}
	fread(&bmpinf, bmphdr.hdrsize-4, 1, fin);
	switch (bmphdr.hdrsize) {
		case 12 : {
			bmpinf.depth = (*(BMPINF2*)&bmpinf).depth;
			bmpinf.planes = (*(BMPINF2*)&bmpinf).planes;
			bmpinf.height = (*(BMPINF2*)&bmpinf).height;
			bmpinf.width = (*(BMPINF2*)&bmpinf).width;
		} break;			// OS/2 1.x;
		case 40 : break;		// Windows;
		case 64 : break;		// OS/2 2+;
		default : fclose(fin); return 3;
	}
	switch (bmpinf.depth) {
		case  1 : {
			fread(&bmppal.data, sizeof(union gx_RGBQ), 2, fin);
			bmplrd = bmprln_01raw;
			bmppal.count = 2;
		} break;
		case  4 : {
			fread(&bmppal.data, sizeof(union gx_RGBQ),  16, fin);
			bmplrd = bmprln_04raw;
			bmppal.count = 16;
		} break;
		case  8 : {
			fread(&bmppal.data, sizeof(union gx_RGBQ), 256, fin);
			bmplrd = bmprln_raw;
			bmppal.count = 256;
		} break;
		case 24 : bmplrd = bmprln_raw; break;
		case 32 : bmplrd = bmprln_raw; break;
		default : fclose(fin); return 4;
	}
	switch (bmpinf.encoding) {
		case  0 : break;		// no compression
		case  1 : bmplrd = bmprln_08rle; break;
		case  2 : bmplrd = bmprln_04rle; break;
		default : fclose(fin); return 5;
	}
	if (!(convln = gx_getcbltf(cblt_conv, depth, bmpinf.depth<8 ? 8 : bmpinf.depth))) {
		fclose(fin);
		return 6;
	}
	dst->depth  = depth;
	dst->width  = bmpinf.width;
	dst->height = bmpinf.height;
	dst->scanLen = 0;
	if (gx_initSurf(dst, 0, &bmppal, SURF_DNTCLR | (depth == 8 ? SURF_CPYPAL : 0))) {
		fclose(fin);
		return 8;
	}
	bmplsz = ((bmpinf.depth * bmpinf.width >> 3) + 3) & (~3);
	/*if (!(tmpbuff = (char*) malloc(bmplsz))) {
		fclose(fin);
		return 9;
	}//*/
	tmpbuff = (char*)gx_buff;
	ptr = (char*)dst->basePtr + dst->scanLen * dst->height;
	fseek(fin, bmphdr.imgstart, SEEK_SET);
	while (bmpinf.height--) {
		ptr -= dst->scanLen;
		bmplrd((unsigned char*)tmpbuff, fin, bmplsz);
		gx_callcbltf(convln, ptr, tmpbuff, bmpinf.width, &bmppal);
	}
	//~ free(tmpbuff);
	fclose(fin);
	return 0;
}

int gx_saveBMP(char* dst, gx_Surf src, int flags) {
	gx_Clut_s	bmppal;			// bitmap palette
	bmplnwriter	bmplwr;			// bitmap line writer
	BMP_HDR		bmphdr;			// bitmap header
	BMP_INF		bmpinf;			// bitmap info header
	int		bmplsz;			// bitmap line size
	FILE*		fout;			// bitmap output file
	char*		tmpbuff;		// bitmap temp buffer
	char*		ptr;			// surface line ptr
	cblt_proc	convln;			// color convereter rutine
	memset(&bmphdr, 0, sizeof(BMP_HDR));
	memset(&bmpinf, 0, sizeof(BMP_INF));
	memset(&bmppal, 0, sizeof(gx_Clut_s));
	flags &= ~BMP_RLE;				// not implemented	//TODO
	switch (src->depth) {
		case  8 : {
			if (src->clutPtr) {
				memcpy(&bmppal, src->clutPtr, sizeof(gx_Clut_s));
				if (bmppal.count <= 16)
					if (bmppal.count == 2)
						bmpinf.depth = 1;
					else bmpinf.depth = 4;
				else bmpinf.depth = 8;
			} else {
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
					} else	bmplwr = bmpwln_04raw;
				} break;
				case 8 : {
					if (flags & BMP_RLE) {
						bmpinf.encoding = 1;
						bmplwr = bmpwln_08rle;
					} else	bmplwr = bmpwln_raw;
				} break;
			}
		}break;
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
	bmphdr.manfact = 'MB';
	bmphdr.hdrsize = (flags & BMP_OS2) ? 64 : 40;
	bmphdr.imgstart = sizeof(BMP_HDR)-4 + bmphdr.hdrsize + (bmppal.count << 2);
	bmphdr.filesize = bmphdr.imgstart + bmpinf.imgsize;
	if (!(convln = gx_getcbltf(cblt_conv, bmpinf.depth < 8 ? 8 : bmpinf.depth, src->depth))) return 3;
	if (!(fout = fopen(dst,"wb"))) return 1;
	/*if (!(tmpbuff = (char*) malloc(bmplsz))) {
		fclose(fout);
		return 4;
	}//*/
	tmpbuff = (char*)gx_buff;
	fwrite(&bmphdr, sizeof(BMP_HDR), 1, fout);
	fwrite(&bmpinf, bmphdr.hdrsize-4, 1, fout);
	fwrite(&bmppal.data, bmppal.count, 4, fout);
	ptr = (char*)src->basePtr + src->height * src->scanLen;
	while (bmpinf.height--) {
		gx_callcbltf(convln, tmpbuff, ptr -= src->scanLen, bmpinf.width, &bmppal);
		bmplwr(fout, (unsigned char*)tmpbuff, bmplsz);
	}
	//~ free(tmpbuff);
	fclose(fout);
	return 0;
}

int gx_loadCUR(gx_Surf dst, char* src, int flags) {
	FILE		*fin;
	CUR_HDR		curhdr;
	CUR_DIR		curdir;
	CUR_BMP		curbmp;
	gx_Clut_s	bmppal;			// bitmap palette
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
	dst->clutPtr = (char*)dst->basePtr + dst->scanLen * dst->height;
	dst->flags |= SURF_ID_CUR;
	dst->hotSpot.x = curdir.hotspotx;
	dst->hotSpot.y = curdir.hotspoty;
	switch (curbmp.depth) {
		case  1 : {
			fread(&bmppal.data, sizeof(union gx_RGBQ), 2, fin);
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x, ++i) {
					if ((i & 7) == 0) c = fgetc(fin); else c <<= 1;
					((union gx_RGBQ *)ptr)[x] = bmppal.data[(c>>7) & 0x01];
				}
			}
		} break;
		case  4 : {
			fread(&bmppal.data, sizeof(union gx_RGBQ),  16, fin);
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x, ++i) {
					if ((i & 1) == 0) c = fgetc(fin); else c <<= 4;
					((union gx_RGBQ *)ptr)[x] = bmppal.data[(c>>4) & 0x0f];
				}
			}
		} break;
		case  8 : {
			fread(&bmppal.data, sizeof(union gx_RGBQ), 256, fin);
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x) {
					c = fgetc(fin);
					((union gx_RGBQ *)ptr)[x] = bmppal.data[c];
				}
			}
		} break;
		case 24 : {
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x) {
					((union gx_RGBQ *)ptr)[x].b = fgetc(fin);
					((union gx_RGBQ *)ptr)[x].g = fgetc(fin);
					((union gx_RGBQ *)ptr)[x].r = fgetc(fin);
					((union gx_RGBQ *)ptr)[x].a = 0;
				}
			}
		} break;
		case 32 : {
			for(y = 0; y < curdir.height; ++y) {
				ptr -= dst->scanLen;
				for(x = 0; x < curdir.width; ++x) {
					((union gx_RGBQ *)ptr)[x].b = fgetc(fin);
					((union gx_RGBQ *)ptr)[x].g = fgetc(fin);
					((union gx_RGBQ *)ptr)[x].r = fgetc(fin);
					((union gx_RGBQ *)ptr)[x].a = fgetc(fin);
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
				((union gx_RGBQ *)ptr)[x].a = ((c>>7) & 0x01) ? 0 : 255;
			}
		}
	}
	fclose(fin);
	return 0;
}

#pragma pack(8)
#include "libs/libjpeg/jpeglib.h"
#pragma library (libjpeg);

int gx_loadJPG(gx_Surf dst, char* src, int depth) {
	FILE*		fin;
	cblt_proc	convln;			// color convereter rutine
	char*		tmpbuff;		// bitmap temp buffer
	char*		ptr;			// ptr to surface line
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

	if (!(convln = gx_getcbltf(cblt_conv, depth, cinfo.jpeg_color_space == JCS_GRAYSCALE ? 8 : 24))) {
		fclose(fin);
		return 6;
	}
	dst->depth  = depth;
	dst->width  = cinfo.output_width;
	dst->height = cinfo.output_height;
	dst->scanLen = 0;
	if (gx_initSurf(dst, 0, 0, SURF_DNTCLR)) {
		fclose(fin);
		return 8;
	}
	/*if (!(tmpbuff = (char*) malloc(bmplsz))) {
		fclose(fin);
		return 9;
	}//*/

	/* Step 6: while (scan lines remain to be read) */
	tmpbuff = (char*)gx_buff;
	ptr = (char*)dst->basePtr;
	while (cinfo.output_scanline < cinfo.output_height) {
		(void) jpeg_read_scanlines(&cinfo, &tmpbuff, 1);
		gx_callcbltf(convln, ptr, tmpbuff, dst->width, (void*)0);
		ptr += dst->scanLen;
	}
	//~ free(tmpbuff);

	/* Step 7: Finish decompression */
	(void) jpeg_finish_decompress(&cinfo);

	/* Step 8: Release JPEG decompression object */
	jpeg_destroy_decompress(&cinfo);

	fclose(fin);
	return 0;
}

//##############################################################################
