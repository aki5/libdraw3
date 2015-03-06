#include "os.h"
#include "draw3.h"

static inline void
drawpixel(uchar *img, int width, short x, short y, u32int color, u32int mask)
{
	u32int *pix;
	int off = y*width+x;
	pix = (u32int *)img+off;
	if(!mask) *pix = color;
}


void
drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color, int subpix)
{
	int abp_y, bcp_y, cap_y, abp, bcp, cap;
	int abp_dx, bcp_dx, cap_dx;
	int abp_dy, bcp_dy, cap_dy;
	short azero, bzero, czero;
	short xstart, xend;
	short ystart, yend;
	short p[2];

	xstart = maxi(0, mini(a[0], mini(b[0], c[0]))) >> subpix;
	ystart = maxi(0, mini(a[1], mini(b[1], c[1]))) >> subpix;
	p[0] = xstart << subpix;
	p[1] = ystart << subpix;
	xend = ((mini((width<<subpix)-1, maxi(a[0], maxi(b[0], c[0])))+(1<<subpix)-1) >> subpix) + 1;
	yend = ((mini((height<<subpix)-1, maxi(a[1], maxi(b[1], c[1])))+(1<<subpix)-1) >> subpix) + 1;

	abp_y = ori2i(a, b, p);
	bcp_y = ori2i(b, c, p);
	cap_y = ori2i(c, a, p);

	abp_dx = ori2i_dx(a, b) << subpix;
	bcp_dx = ori2i_dx(b, c) << subpix;
	cap_dx = ori2i_dx(c, a) << subpix;

	abp_dy = ori2i_dy(a, b) << subpix;
	bcp_dy = ori2i_dy(b, c) << subpix;
	cap_dy = ori2i_dy(c, a) << subpix;

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
drawtris(uchar *img, int width, int height, short *tris, uchar *colors, int ntris, int subpix)
{
	int i;
	for(i = 0; i < ntris; i++){
		short *a, *b, *c;
		a = tris+6*i+0;
		b = tris+6*i+2;
		c = tris+6*i+4;
		drawtri(img, width, height, a, b, c, colors+4*i, subpix);
	}
}
