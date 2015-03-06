#include "os.h"
#include "draw3.h"

// vector_size attribute is in bytes
typedef int v4int __attribute__((vector_size(16)));
#define vec4i(a, b, c, d) (v4int){a, b, c, d}

static inline v4int
vec4ir(int v)
{
	return vec4i(v, v, v, v);
}


static inline void
drawpixel(uchar *img, int width, short x0, short y0, u32int color, v4int mask)
{
	u32int *pix;
	int off = y0*width+x0;
	pix = (u32int *)img + off;
	if(mask[0] == 0)
		pix[0] = color;
	if(mask[1] == 0)
		pix[1] = color;
	if(mask[2] == 0)
		pix[width] = color;
	if(mask[3] == 0)
		pix[width+1] = color;

}

static void
bbox_clamp(short *xsp, short *ysp, short *xep, short *yep, int width, int height, short *a, short *b, short *c, int subpix)
{
	*xsp = maxi(0, mini(a[0], mini(b[0], c[0]))) >> subpix;
	*ysp = maxi(0, mini(a[1], mini(b[1], c[1]))) >> subpix;
	*xep = ((mini((width<<subpix)-1, maxi(a[0], maxi(b[0], c[0])))+(1<<subpix)-1) >> subpix) + 1;
	*yep = ((mini((height<<subpix)-1, maxi(a[1], maxi(b[1], c[1])))+(1<<subpix)-1) >> subpix) + 1;
}

static inline void
drawtri_horse(uchar *img, int xstart, int ystart, int xend, int yend, short *a, short *b, short *c, uchar *color, int subpix)
{
	v4int abp_y, bcp_y, cap_y, abp, bcp, cap;
	v4int abp_dx, bcp_dx, cap_dx;
	v4int abp_dy, bcp_dy, cap_dy;
	v4int mask;

	short x0, y0;


	enum { xstep = 2, ystep = 2 };

	abp_dx = vec4ir(ori2i_dx(a, b) << subpix);
	bcp_dx = vec4ir(ori2i_dx(b, c) << subpix);
	cap_dx = vec4ir(ori2i_dx(c, a) << subpix);

	abp_dy = vec4ir(ori2i_dy(a, b) << subpix);
	bcp_dy = vec4ir(ori2i_dy(b, c) << subpix);
	cap_dy = vec4ir(ori2i_dy(c, a) << subpix);

	short p[2] = { xstart << subpix, ystart << subpix };
	abp_y = vec4ir(ori2i(a, b, p)) + vec4i(0, abp_dx[0], abp_dy[0], abp_dx[0]+abp_dy[0]);
	bcp_y = vec4ir(ori2i(b, c, p)) + vec4i(0, bcp_dx[0], bcp_dy[0], bcp_dx[0]+bcp_dy[0]);
	cap_y = vec4ir(ori2i(c, a, p)) + vec4i(0, cap_dx[0], cap_dy[0], cap_dx[0]+cap_dy[0]);

	abp_dx = abp_dx * xstep;
	bcp_dx = bcp_dx * xstep;
	cap_dx = cap_dx * xstep;

	abp_dy = abp_dy * ystep;
	bcp_dy = bcp_dy * ystep;
	cap_dy = cap_dy * ystep;

	for(y0 = ystart; y0 < yend; y0 += ystep){
		abp = abp_y;
		bcp = bcp_y;
		cap = cap_y;
		for(x0 = xstart; x0 < xend; x0 += xstep){
			mask = (abp | bcp | cap) >> 31;
			if((mask[0]&mask[1]&mask[2]&mask[3]) == 0){
//				mask |= vec4i(0, -(x0 == xend-1), 0, -(x0 == xend-1));
//				mask |= vec4i(0, 0, -(y0 == yend-1), -(y0 == yend-1));
				drawpixel(img, width, x0, y0, *(u32int*)color, mask);
			}
			abp += abp_dx;
			bcp += bcp_dx;
			cap += cap_dx;
		}
		abp_y += abp_dy;
		bcp_y += bcp_dy;
		cap_y += cap_dy;
	}
}

static inline int
box_ori(short *a, short *b, short *ba, short *bb, short *bc, short *bd)
{
	if(ori2i(a, b, ba) < 0 && ori2i(a, b, bb) < 0 && ori2i(a, b, bc) < 0 && ori2i(a, b, bd) < 0)
		return 1;
	return 0;
}

void
drawtri_subdiv(uchar *img, int xstart, int ystart, int xend, int yend, short *a, short *b, short *c, uchar *color, int subpix)
{
	int x0, y0, nx0, ny0;
	int nbox, nskip;

	nbox = 0;
	nskip = 0;
	for(y0 = ystart; y0 < yend; y0 = ny0){
		ny0 = y0 + 64;
		ny0 = ny0 < yend ? ny0 : yend;
		for(x0 = xstart; x0 < xend; x0 = nx0){
			nbox++;
			nx0 = x0 + 64;
			nx0 = nx0 < xend ? nx0 : xend;

			short ba[2] = {x0, y0};
			short bb[2] = {x0, ny0-1};
			short bc[2] = {nx0-1, ny0-1};
			short bd[2] = {nx0-1, y0};

			if(box_ori(a, b, ba, bb, bc, bd) || box_ori(b, c, ba, bb, bc, bd) || box_ori(c, a, ba, bb, bc, bd)){
				nskip++;
				continue;
			}
			drawtri_horse(img, x0, y0, nx0, ny0, a, b, c, color, subpix);
		}
	}
}

void
drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color, int subpix)
{
	short xstart, xend;
	short ystart, yend;
	bbox_clamp(&xstart, &ystart, &xend, &yend, width, height, a, b, c, subpix);
	drawtri_subdiv(img, xstart, ystart, xend, yend, a, b, c, color, subpix);
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
