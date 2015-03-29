#include "os.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftlcdfil.h>
#include "draw3.h"

static FT_Library library;
static FT_Face face;
static int libinit;
static int fontsize = 11;
static int dpi = 96;

int
utf8decode(char *str, int *offp, int len)
{
	uchar *s;

	s = (uchar *)str;
	if(len >= 1 && s[0] <= 0x7f){ /* 0111 1111 */
		*offp = 1;
		return s[0];
	} else if(len >= 2 && s[0] <= 0xdf){ /* 1101 1111 */
		*offp = 2;
		return ((s[0]&0x1f)<<6)|(s[1]&0x3f);
	} else if(len >= 3 && s[0] <= 0xef){ /* 1110 1111 */
		*offp = 3;
		return ((s[0]&0x0f)<<12)|((s[1]&0x3f)<<6)|(s[2]&0x3f);
	} else if(len >= 4 && s[0] <= 0xf7){ /* 1111 0111 */
		*offp = 4;
		return ((s[0]&0x07)<<18)|((s[1]&0x3f)<<12)|((s[2]&0x3f)<<6)|(s[3]&0x3f);
	}

	*offp = 0;
	return -1;
}

int
utf8encode(char *str, int cap, int code)
{
	uchar *s;
	int len;

	s = (uchar *)str;
	if(code <= 0x7f){
		if(cap < 2) return 0;
		s[0] = code;
		s[1] = '\0';
		len = 1;
	} else if(code <= 0x7ff){
		if(cap < 3) return 0;
		s[0] = 0xc0|((code>>6)&0x1f);
		s[1] = 0x80|((code>>0)&0x3f);
		s[2] = '\0';
		len = 2;
	} else if(code <= 0xfff){
		if(cap < 4) return 0;
		s[0] = 0xe0|((code>>12)&0x0f);
		s[1] = 0x80|((code>>6)&0x3f);
		s[2] = 0x80|((code>>0)&0x3f);
		s[3] = '\0';
		len = 3;
	} else if(code <= 0x1fffff){
		if(cap < 5) return 0;
		s[0] = 0xf0|((code>>18)&0x07);
		s[1] = 0x80|((code>>12)&0x3f);
		s[2] = 0x80|((code>>6)&0x3f);
		s[3] = 0x80|((code>>0)&0x3f);
		s[4] = '\0';
		len = 4;
	} else {
		fprintf(stderr, "unicode U%x sequence out of supported range\n", code); 
		return 0; /* fail */
	}
	return len;
}

void
initdrawstr(char *path)
{
	if(!libinit)
		FT_Init_FreeType(&library);
	FT_New_Face(library, path, 0, &face);

	/* TODO: One day we have rgb masks for draw, but today is not that day */
	//FT_Library_SetLcdFilter(library, FT_LCD_FILTER_DEFAULT);

	FT_Set_Char_Size(face, fontsize<<6, 0, dpi, 0); 
	if(0)fprintf(stderr, "face: ascender %ld descender %ld height %ld\n",
		face->size->metrics.ascender,
		face->size->metrics.descender,
		face->size->metrics.height
	);
	
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

static struct {
	int code;
	short uoff;
	short voff;
	short uadv;
	short vadv;
	short width;
	short height;
	Image *img;
} *cache[65536];

void
setfontsize(int size)
{
	int i;
	fontsize = size;
	FT_Set_Char_Size(face, fontsize<<6, 0, dpi, 0);
	for(i = 0; i < nelem(cache); i++){
		if(cache[i] != NULL){
			freeimage(cache[i]->img);
			free(cache[i]);
			cache[i] = NULL;
		}
	}
}

Image *
glyphsetup(int code, short *uoffp, short *voffp, short *uadvp, short *vadvp, short *width, short *height)
{
	Rect rgly;
	Image *glyim;

	if(code >= 0 && code < nelem(cache) && cache[code] != NULL){
		*uoffp = cache[code]->uoff;
		*voffp = cache[code]->voff;
		*uadvp = cache[code]->uadv;
		*vadvp = cache[code]->vadv;
		*width = cache[code]->width;
		*height = cache[code]->height;
		return cache[code]->img;
	}

	if(FT_Load_Char(face, code, FT_LOAD_DEFAULT) != 0) //FT_LOAD_RENDER
		return NULL;

	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL); //FT_RENDER_MODE_LCD);
	rgly = rect(0, 0, face->glyph->bitmap.width, face->glyph->bitmap.rows);
	glyim = allocimage(rgly, NULL);
	loadimage8(glyim, rgly, face->glyph->bitmap.buffer, face->glyph->bitmap.width);

	*uoffp = face->glyph->bitmap_left;
	*voffp = (face->size->metrics.ascender+63)/64-face->glyph->bitmap_top+1;
	*uadvp = (face->glyph->advance.x+63)/64;
	*vadvp = face->glyph->bitmap.rows + face->glyph->bitmap_top;
	*width = face->glyph->bitmap.width;
	*height = face->glyph->bitmap.rows;

	if(code >= 0 && code < nelem(cache)){
		cache[code] = malloc(sizeof cache[0][0]);
		cache[code]->uoff = *uoffp;
		cache[code]->voff = *voffp;
		cache[code]->uadv = *uadvp;
		cache[code]->vadv = *vadvp;
		cache[code]->width = *width;
		cache[code]->height = *height;
		cache[code]->img = glyim;
	}

	return glyim;
}

void
freeglyph(Image *img)
{
	USED(img);
	//freeimage(img);
}

/*
 *	Freetype2 doesn't actually give a reliable global height, so
 *	no matter how much we fudge it, there's always a chance of
 *	drawchar overflowing its rect.
 */
int
linespace(void)
{
	return (face->size->metrics.height+63)/64+1;
//return (face->size->metrics.height+63)/64 + 2;
//return 15*fontsize/10;
}

int
fontem(void)
{
	return face->size->metrics.x_ppem;
}

Rect
drawchar(Image *img, Rect rdst, Image *src, int opcode, int charcode)
{
	Image *glyim;
	Rect rret;
	short uoff, voff, uadv, vadv, width, height;

	rret.u0 = rdst.u0;
	rret.uend = rdst.u0;

	rret.v0 = rdst.v0;
	rret.vend = rdst.v0 + linespace();

	glyim = glyphsetup(charcode, &uoff, &voff, &uadv, &vadv, &width, &height);
	if(glyim == NULL)
		goto out;

	Rect glydst;
	glydst.u0 = rdst.u0 + uoff;
	glydst.v0 = rdst.v0 + voff;
	glydst.uend = glydst.u0 + width;
	glydst.vend = glydst.v0 + height;
	blend(img, glydst, src, glyim, opcode);
	freeglyph(glyim);

	rret.uend += uadv;
out:
	return rret;
}

Rect
drawstr(Image *img, Rect rdst, Image *src, int opcode, char *str, int len)
{
	Rect rret;
	int off, charcode;

	if(len == -1)
		len = strlen(str);

	rret.u0 = rdst.u0;
	rret.uend = rdst.u0;

	rret.v0 = rdst.v0;
	rret.vend = rdst.v0 + linespace();

	for(off = 0; off < len && rdst.u0 < rdst.uend;){
		int doff;
		charcode = utf8decode(str+off, &doff, len-off);
		if(charcode == -1){
			off++;
			continue;
		} else {
			off += doff;
		}
		if(charcode == '\n')
			continue;
		if(charcode == '\t'){
			rdst.u0 += 3 * fontem();
			rret.uend += 3 * fontem();
			continue;
		}

		Rect tr;
		tr = drawchar(img, rdst, src, opcode, charcode);
		rret.u0 += rectw(&tr);
		rdst.u0 = tr.uend;
	}

	return rret;
}
