#include "os.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftlcdfil.h>
#include "draw3.h"

static FT_Library library;
static FT_Face face;
static int libinit;
static int fontsize = 20;

static int
utf8decode(char *str, int *offp, int len)
{
	int off, left;
	uchar *s;

	s = (uchar *)str;
	off = *offp;
	left = len - off;
	if(left >= 1 && s[off] <= 0x7f){ /* 0111 1111 */
		*offp = off + 1;
		return s[off];
	} else if(left >= 2 && s[off] <= 0xdf){ /* 1101 1111 */
		*offp = off + 2;
		return ((s[off]&0x1f)<<6)|(s[off+1]&0x3f);
	} else if(left >= 3 && s[off] <= 0xef){ /* 1110 1111 */
		*offp = off + 3;
		return ((s[off]&0x0f)<<12)|((s[off+1]&0x3f)<<6)|(s[off+2]&0x3f);
	} else if(left >= 4 && s[off] <= 0xf7){ /* 1111 0111 */
		*offp = off + 4;
		return ((s[off]&0x07)<<18)|((s[off+1]&0x3f)<<12)|((s[off+2]&0x3f)<<6)|(s[off+3]&0x3f);
	}

	return -1;
}

void
initdrawstr(char *path)
{
	if(!libinit)
		FT_Init_FreeType(&library);
	FT_New_Face(library, path, 0, &face);
	//FT_Library_SetLcdFilter(library, FT_LCD_FILTER_DEFAULT);
	FT_Set_Char_Size(face, fontsize<<6, 0, 100, 0); /* 50pt, 100dpi, whatever */
}

Image *
allocimage(Rect r, uchar *color)
{
	Image *img;
	int stride, off, len;

	stride = 4*rectw(&r);
	len = stride*recth(&r);
	img = malloc(sizeof img[0] + len);
	memset(img, 0, sizeof img[0]);
	img->r = r;
	img->len = len;
	img->img = (uchar *)(img+1);
	img->stride = stride;
	if(color != NULL){
		for(off = 0; off < len; off += 4){
			img->img[off+0] = color[0];
			img->img[off+1] = color[1];
			img->img[off+2] = color[2];
			img->img[off+3] = color[3];
		}
	}
	return img;
}

void
freeimage(Image *img)
{
	free(img);
}

void
setfontsize(int size)
{
	fontsize = size;
	FT_Set_Char_Size(face, fontsize<<6, 0, 100, 0);
}

Image *
getglyph(int code, short *uoffp, short *voffp, short *uadvp, short *vadvp)
{
	Rect rgly;
	Image *glyim;


	if(FT_Load_Char(face, code, FT_LOAD_DEFAULT) != 0) //FT_LOAD_RENDER
		return NULL;

	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL); //FT_RENDER_MODE_LCD);
	rgly = rect(0, 0, face->glyph->bitmap.width, face->glyph->bitmap.rows);

	glyim = allocimage(rgly, NULL);
	loadimage8(glyim, rgly, face->glyph->bitmap.buffer, face->glyph->bitmap.width);

	*uoffp = face->glyph->bitmap_left;
	*voffp = -face->glyph->bitmap_top;
	*uadvp = face->glyph->advance.x>>6;
	*vadvp = face->glyph->bitmap.rows + face->glyph->bitmap_top;

	return glyim;
}

void
freeglyph(Image *img)
{
	freeimage(img);
}

Rect
drawstr(Image *img, Rect rdst, char *str, int len)
{
	Image *colim;
	Rect rret;
	int off, code;
	short uoff, voff, uadv, vadv;

	if(len == -1)
		len = strlen(str);

	uchar color[4];
	idx2color(2, color);
	colim = allocimage(rect(0,0,1,1), color);

	rret.u0 = rdst.u0;
	rret.uend = rdst.u0;
	rret.v0 = rdst.v0;

	for(off = 0; off < len && rdst.u0 < rdst.uend;){
		code = utf8decode(str, &off, len);
		if(code == -1){
			off++;
			continue;
		}

		Image *glyim;
		glyim = getglyph(code, &uoff, &voff, &uadv, &vadv);

		Rect glydst;
		glydst.u0 = rdst.u0 + uoff;
		glydst.v0 = rdst.v0 + voff;
		glydst.uend = glydst.u0 + rectw(&glyim->r);
		glydst.vend = glydst.v0 + recth(&glyim->r);

		drawblend(img, glydst, colim, glyim);
		freeglyph(glyim);

		rdst.u0 += uadv;
		rret.uend += uadv;
	}

	freeimage(colim);

	rret.vend = rret.v0 + 15*fontsize/10;
	return rret;
}
