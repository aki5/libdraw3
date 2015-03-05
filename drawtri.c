#include "os.h"
#include "draw3.h"

static inline int
topleft_ccw(short *a, short *b)
{
	return 0;
	if(a[1] == b[1] && a[0] > b[0]) /* top: horizontal, goes left */
		return 0;
	if(a[1] != b[1] && a[1] > b[1]) /* left: non-horizontal, goes down (up in the stupid inverted y screen space) */
		return 0;
	//return -(1<<Ssubpix);
	return -(1<<Ssubpix);
}

static inline void
drawpixel(uchar *img, int width, short x, short y, u32int color, u32int mask)
{
	u32int *pix;
	int off = y*width+x;
	pix = (u32int *)img+off;
	if(!mask) *pix = color;
}


void
drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color)
{
	int abp_y, bcp_y, cap_y, abp, bcp, cap;
	int abp_dx, bcp_dx, cap_dx;
	int abp_dy, bcp_dy, cap_dy;
	short azero, bzero, czero;
	short xstart, xend;
	short ystart, yend;
	short p[2];

	p[0] = maxi(0, mini(a[0], mini(b[0], c[0])));
	p[1] = maxi(0, mini(a[1], mini(b[1], c[1])));
	xstart = p[0] >> Ssubpix;
	ystart = p[1] >> Ssubpix;
	xend = (mini((width<<Ssubpix)-1, maxi(a[0], maxi(b[0], c[0]))) >> Ssubpix) + 1;
	yend = (mini((height<<Ssubpix)-1, maxi(a[1], maxi(b[1], c[1]))) >> Ssubpix) + 1;

	azero = topleft_ccw(a, b);
	bzero = topleft_ccw(b, c);
	czero = topleft_ccw(c, a);

	abp_y = ori2i(a, b, p) - azero;
	bcp_y = ori2i(b, c, p) - bzero;
	cap_y = ori2i(c, a, p) - czero;

	abp_dx = ori2i_dx(a, b) << Ssubpix;
	bcp_dx = ori2i_dx(b, c) << Ssubpix;
	cap_dx = ori2i_dx(c, a) << Ssubpix;

	abp_dy = ori2i_dy(a, b) << Ssubpix;
	bcp_dy = ori2i_dy(b, c) << Ssubpix;
	cap_dy = ori2i_dy(c, a) << Ssubpix;

	for(p[1] = ystart; p[1] < yend; p[1]++){
		abp = abp_y;
		bcp = bcp_y;
		cap = cap_y;
		for(p[0] = xstart; p[0] < xend; p[0]++){
			drawpixel(img, width, p[0], p[1], *(u32int*)color, (abp|bcp|cap)>>31);
			abp += abp_dx;
			bcp += bcp_dx;
			cap += cap_dx;
		}
		abp_y += abp_dy;
		bcp_y += bcp_dy;
		cap_y += cap_dy;
	}
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
