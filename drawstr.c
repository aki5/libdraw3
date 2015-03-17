#include "os.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftlcdfil.h>
#include "draw3.h"

static FT_Library library;
static FT_Face face;
static int libinit;
static int fontsize = 200;

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

void
drawstr(Image *img, Rect r, char *str, int len)
{
	uchar *data;
	int off, code;
	short u0, v0, uend, vend;

	if(len == -1)
		len = strlen(str);

	for(off = 0; off < len && r.u0 < r.uend;){
		code = utf8decode(str, &off, len);
		if(code == -1){
			off++;
			continue;
		}

		if(FT_Load_Char(face, code, FT_LOAD_DEFAULT) != 0) //FT_LOAD_RENDER
			continue;
		FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL); //FT_RENDER_MODE_LCD);
		data = face->glyph->bitmap.buffer;
		u0 = r.u0 + face->glyph->bitmap_left;
		v0 = r.v0 + 4+fontsize - face->glyph->bitmap_top;
		uend = u0 + face->glyph->bitmap.width;
		vend = v0 + face->glyph->bitmap.rows;

		Rect rr = cliprect(rect(u0, v0, uend, vend), r);
		loadimage8(img, rr, data, face->glyph->bitmap.width);
		//loadimage24(img, rr, data, face->glyph->bitmap.width);

		r.u0 += (face->glyph->advance.x>>6);
	}
}
