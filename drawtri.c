#include "os.h"
#include "draw3.h"

static inline int
topleft_ccw(short *a, short *b)
{
	if(a[1] == b[1] && a[0] > b[0]) /* top: horizontal, goes left */
		return 0;
	if(a[1] != b[1] && a[1] > b[1]) /* left: non-horizontal, goes down (up in the stupid inverted y screen space) */
		return 0;
	return 1;
}

static inline void
drawpixel(uchar *img, int width, short x, short y, uchar *color)
{
	int off = y*width+x;
	*((unsigned int *)img+off) = *(unsigned int *)color;
}

void
drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color)
{
	short azero, bzero, czero;
	short xstart, xend;
	short ystart, yend;
	short p[2];

	xstart = maxi(0, mini(a[0], mini(b[0], c[0])));
	ystart = maxi(0, mini(a[1], mini(b[1], c[1])));
	xend = mini(width-1, maxi(a[0], maxi(b[0], c[0]))) + 1;
	yend = mini(height-1, maxi(a[1], maxi(b[1], c[1]))) + 1;

	azero = topleft_ccw(a, b);
	bzero = topleft_ccw(b, c);
	czero = topleft_ccw(c, a);

	for(p[1] = ystart; p[1] < yend; p[1]++)
		for(p[0] = xstart; p[0] < xend; p[0]++)
			if(ori2i(a, b, p) >= azero && ori2i(b, c, p) >= bzero && ori2i(c, a, p) >= czero)
				drawpixel(img, width, p[0], p[1], color);
}

void
drawtris(uchar *img, int width, int height, short *tris, uchar *colors, int ntris)
{
	int i;
	for(i = 0; i < ntris; i++){
		short *a, *b, *c;
		a = tris+6*i+0;
		b = tris+6*i+2;
		c = tris+6*i+4;
		drawtri(img, width, height, a, b, c, colors+4*i);
	}
}
