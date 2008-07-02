#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype\\freetype.h"
#pragma library (libttf);

#include "gx_surf.h"

static FT_Library	ft_lib = 0;
static FT_Face		ft_face = 0;

static int	max_y = 0, max_x = 0, idx_y, max_c=0;

/*int gx_initFont(const char *fontfile, int fontsize) {
	int tmp, i, k, j = 0, ft_err;
	if ((ft_err = FT_Init_FreeType(&ft_lib))) {
		//~ Panic( "could not initialize FreeType" );
		return -1;
	}
	if ((ft_err = FT_New_Face(ft_lib, fontfile, 0, &ft_face))) {
		//~ if (ft_err == FT_Err_Unknown_File_Format) {
		//Panic( "the font file could be opened and read, \nbut it appears that its font format is unsupported" );
		//~ }
		//Panic( "another error code means that the font file could not \nbe opened or read, or simply that it is broken..." );
		return -2;
	}
	//~ FT_Get_Char_Index(ft_face, 'A')
	if ( ft_err = FT_Set_Char_Size(
			ft_face,		// handle to face object           * /
			0,			// char_width in 1/64th of points  * /
			64,			// char_height in 1/64th of points * /
			300,			// horizontal device resolution    * /
			300)) {			// vertical device resolution      * /
		//~ Panic( "could not set size" );
		return -3;
	}
	if (ft_err = FT_Set_Pixel_Sizes (
			ft_face,		// handle to face object * /
			0,			// pixel_width           * /
			fontsize)) {		// pixel_height          * /
		//~ Panic( "could not set pixel size" );
		return -4;
	}
	/ *for (i = 0; i < 256; ++i) ftst[i] = 0;
	for (ft_err = i = 0; i < 256; ++i) {
		int chr_idx = FT_Get_Char_Index(ft_face, i);
		if (FT_Load_Glyph(ft_face, chr_idx, 0)) {
			//~ Panic( "load glyph" );
			ft_err = 1;
		}
		for (k = 0; k < j; ++k) if(ftst[j] == chr_idx) break;
		if (k == j) {		// not found
			ftst[j++] = chr_idx;
			max_c += 1;
		}
		tmp = (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
		if (max_y < tmp) {idx_y = i; max_y = tmp;}
		tmp = (ft_face->glyph->metrics.width + 1*ft_face->glyph->metrics.horiBearingX)>>6;
		if (max_x < tmp) max_x = tmp;
		/ *if (FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
			//~ Panic( "could not render face" );
			ft_err |= 2;
		}* /
	}
	return ft_err;//* /
	return 0;
}//*/

int gx_initFont(const char *fontfile, int fontsize) {
	int tmp, i, j = 0, bits = 0, ft_err, chr_idx[256] = { 0 };
	struct gx_Llut_t flut;
	if (( ft_err = FT_Init_FreeType( &ft_lib ) )) {
		//~ Panic( "could not initialize FreeType" );
		return -1;
	}
	if (( ft_err = FT_New_Face( ft_lib, fontfile, 0, &ft_face) )) {
		if (ft_err == FT_Err_Unknown_File_Format) {
			//Panic( "the font file could be opened and read, \nbut it appears that its font format is unsupported" );
		} else	//Panic( "another error code means that the font file could not \nbe opened or read, or simply that it is broken..." );
		return -2;
	}
	if (( ft_err = FT_Set_Char_Size( ft_face, 0, 64, 300, 300) )) {
		//~ Panic( "could not set size" );
		return -3;
	}
	if (( ft_err = FT_Set_Pixel_Sizes( ft_face, 0, fontsize) )) {
		//~ Panic( "could not set pixel size" );
		return -4;
	}
	for ( i = 0; i < 256; ++i ) {
		chr_idx[i] = 0;
		flut.data[i].pad_x = 0;
		flut.data[i].pad_y = 0;
		flut.data[i].height = 0;
		flut.data[i].width = 0;
	}
	for ( i = 0; i < 256; ++i ) {
		chr_idx[i] = FT_Get_Char_Index(ft_face, i);
		for ( j = 0; j < i; ++j ) if ( chr_idx[i] == chr_idx[j] ) break;
		if ( i == j ) {				// add new entry
			if (FT_Load_Glyph(ft_face, chr_idx[i], 0)) {
				//~ Panic( "load glyph" );
				//~ return -100;
			}
			flut.data[i].pad_x = ft_face->glyph->metrics.horiBearingX >> 6;
			flut.data[i].pad_y = ft_face->glyph->metrics.vertBearingY >> 6;
			flut.data[i].height = ft_face->glyph->metrics.height >> 6;
			flut.data[i].width = ft_face->glyph->metrics.width >> 6;
			bits += flut.data[i].height * flut.data[i].width;

			//~ tmp = flut.data[i].height + flut.data[i].pad_y;
			//~ if (max_y < tmp) {idx_y = i; max_y = tmp;}

			//~ tmp = flut.data[i].pad_x + flut.data[i].width;
			//~ if (max_x < tmp) max_x = tmp;

			tmp = (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
			if (max_y < tmp) {idx_y = i; max_y = tmp;}

			tmp = (ft_face->glyph->metrics.width + 1*ft_face->glyph->metrics.horiBearingX)>>6;
			if (max_x < tmp) max_x = tmp;
			max_c += 1;
			
		} else {
			flut.data[i] = flut.data[j];
		}
		/*if (FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
			//~ Panic( "could not render face" );
			ft_err |= 2;
		}*/
	}
	return ft_err;//*/
	return 0;
}

//~ extern void setpix(void fn(void), void* dst, long col);
extern long mixpix(void fn(void), long col2, long col1, long alpha8);
//~ #pragma aux setpix parm [edx] [edi] [eax] modify [edx] = "call edx";
#pragma aux mixpix parm [ebx] [eax] [edx] [ecx] modify [edx] value [eax] = "call ebx";

void gx_drawChar(gx_Surf dst, int x, int y, long col, char chr) {
	void (*mixpfn)(void);
	if (ft_face == 0) return;
	switch (dst->depth) {
		extern void mixcol_32(void);
		extern void mixcol_24(void);
		extern void mixcol_16(void);
		extern void mixcol_08(void);
		case 32 : mixpfn = mixcol_32; break;
		case 24 : mixpfn = mixcol_24; break;
		case 16 : mixpfn = mixcol_16; break;
		case  8 : mixpfn = mixcol_08; break;
		default : return;
	}
	if ( FT_Load_Char( ft_face, chr, FT_LOAD_RENDER) == 0 ) {
		int w, h, py = y - ft_face->glyph->bitmap_top;
		unsigned char* src = ft_face->glyph->bitmap.buffer;
		for (h = ft_face->glyph->bitmap.rows; h; --h, ++py) {
			int px = x + ft_face->glyph->bitmap_left;
			for (px = x, w = ft_face->glyph->bitmap.width; w; --w, ++px) {
				gx_setpixel(dst, px, py, mixpix(mixpfn, gx_getpixel(dst, px, py), -1, *src));
				src++;
			}
		}
	}
}

void gx_drawText(gx_Surf dst, int x, int y, long col, const char* str) {
	void (*mixpfn)(void);
	if (ft_face == 0) return;
	switch (dst->depth) {
		extern void mixcol_32(void);
		extern void mixcol_24(void);
		extern void mixcol_16(void);
		extern void mixcol_08(void);
		case 32 : mixpfn = mixcol_32; break;
		case 24 : mixpfn = mixcol_24; break;
		case 16 : mixpfn = mixcol_16; break;
		case  8 : mixpfn = mixcol_08; break;
		default : return;
	}
	/*while ( *str ) {
		if ( FT_Load_Char( ft_face, *str, FT_LOAD_RENDER) == 0 ) {
			int w, h, py = y - ft_face->glyph->bitmap_top;
			unsigned char* src = ft_face->glyph->bitmap.buffer;
			y -= (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
			//~ y -= ft_face->glyph->linearVertAdvance >> 8;
			for (h = ft_face->glyph->bitmap.rows; h; --h, ++py) {
				int px = x + ft_face->glyph->bitmap_left;
				for (px = x, w = ft_face->glyph->bitmap.width; w; --w, ++px) {
					gx_setpixel(dst, px, py, mixpix(mixpfn, gx_getpixel(dst, px, py), -1, *src));
					src++;
				}
			}
			//~ y -= ft_face->glyph->metrics.height >> 6;
			//~ x += ft_face->glyph->advance.x >> 6;
			//~ y -= ft_face->height >> 8;
			//~ y -= max_y;
		}
		str++;
	}*/
	{
	int w, h, py, i, xxx = x;
	unsigned char* src;
	for (i=0; i< 256; ++i) {
		FT_Load_Char( ft_face, i, FT_LOAD_RENDER);
		py = y - ft_face->glyph->bitmap_top;
		src = ft_face->glyph->bitmap.buffer;
		//~ y -= (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
		//~ y -= ft_face->glyph->linearVertAdvance >> 8;
		for (h = ft_face->glyph->bitmap.rows; h; --h, ++py) {
			int px = x + ft_face->glyph->bitmap_left;
			for (px = x, w = ft_face->glyph->bitmap.width; w; --w, ++px) {
				gx_setpixel(dst, px, py, mixpix(mixpfn, gx_getpixel(dst, px, py), -1, *src));
				src++;
			}
		}
		//~ y -= ft_face->glyph->metrics.height >> 6;
		x += max_x;//ft_face->glyph->advance.x >> 6;
		//~ y -= ft_face->height >> 8;
		if ((i & 31) == 31) {
			y += max_y;
			x = xxx;
		}
	}
	//~ max_c = 200;
	FT_Load_Char( ft_face, ((max_c/100)%10)+'0', FT_LOAD_RENDER);
	py = y - ft_face->glyph->bitmap_top;
	src = ft_face->glyph->bitmap.buffer;
	//~ y -= (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
	//~ y -= ft_face->glyph->linearVertAdvance >> 8;
	for (h = ft_face->glyph->bitmap.rows; h; --h, ++py) {
		int px = x + ft_face->glyph->bitmap_left;
		for (px = x, w = ft_face->glyph->bitmap.width; w; --w, ++px) {
			gx_setpixel(dst, px, py, mixpix(mixpfn, gx_getpixel(dst, px, py), -1, *src));
			src++;
		}
	}
	x += max_x;
	FT_Load_Char( ft_face, ((max_c/10)%10)+'0', FT_LOAD_RENDER);
	py = y - ft_face->glyph->bitmap_top;
	src = ft_face->glyph->bitmap.buffer;
	//~ y -= (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
	//~ y -= ft_face->glyph->linearVertAdvance >> 8;
	for (h = ft_face->glyph->bitmap.rows; h; --h, ++py) {
		int px = x + ft_face->glyph->bitmap_left;
		for (px = x, w = ft_face->glyph->bitmap.width; w; --w, ++px) {
			gx_setpixel(dst, px, py, mixpix(mixpfn, gx_getpixel(dst, px, py), -1, *src));
			src++;
		}
	}
	x += max_x;
	FT_Load_Char( ft_face, ((max_c)%10)+'0', FT_LOAD_RENDER);
	py = y - ft_face->glyph->bitmap_top;
	src = ft_face->glyph->bitmap.buffer;
	//~ y -= (ft_face->glyph->metrics.height + 1*ft_face->glyph->metrics.vertBearingY)>>6;
	//~ y -= ft_face->glyph->linearVertAdvance >> 8;
	for (h = ft_face->glyph->bitmap.rows; h; --h, ++py) {
		int px = x + ft_face->glyph->bitmap_left;
		for (px = x, w = ft_face->glyph->bitmap.width; w; --w, ++px) {
			gx_setpixel(dst, px, py, mixpix(mixpfn, gx_getpixel(dst, px, py), -1, *src));
			src++;
		}
	}
	x += max_x;
	}//*/
}

/*int main(int argc, char* argv[]) {
	int		ft_err;
	FT_Library	ft_lib;
	FT_Face		ft_face;

	char*  filename = "arial.ttf";

	struct gx_Surf_t img1;

	if ((ft_err = FT_Init_FreeType(&ft_lib)))
		Panic( "could not initialize FreeType" );
	ft_err = FT_New_Face(ft_lib, filename, 0, &ft_face);
	if (ft_err == FT_Err_Unknown_File_Format) {
		Panic( "the font file could be opened and read, \n\
		but it appears that its font format is unsupported" );
	} else if (ft_err) {
		Panic( "another error code means that the font file could not \n\
		be opened or read, or simply that it is broken..." );
	}
	//~ FT_Get_Char_Index(ft_face, 'A')
	ft_err = FT_Set_Char_Size(
			ft_face,		/ * handle to face object           * /
			0,			/ * char_width in 1/64th of points  * /
			16*64,			/ * char_height in 1/64th of points * /
			300,			/ * horizontal device resolution    * /
			300);			/ * vertical device resolution      * /
	if (ft_err) {
		Panic( "could not set size" );
	}
	ft_err = FT_Set_Pixel_Sizes(
			ft_face,		/ * handle to face object * /
			0,			/ * pixel_width           * /
			900 );			/ * pixel_height          * /
	if (ft_err) {
		Panic( "could not set pixel size" );
	}
	if (FT_Load_Glyph(ft_face, FT_Get_Char_Index(ft_face, 'x'), 0)) {
		Panic( "load glyph" );
	}
	if (FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
		Panic( "could not render face" );
	}
	img1.scanLen = img1.width = ft_face->glyph->bitmap.width;
	img1.height = ft_face->glyph->bitmap.rows;
	img1.depth = 8;
	gx_initSurf(&img1, 0, 0, SURF_NO_MEM);
	img1.basePtr = (void*)ft_face->glyph->bitmap.buffer;
	gx_saveBMP("tmp.bmp", &img1, 0);
	gx_doneSurf(&img1);
	return 0;
}*/

