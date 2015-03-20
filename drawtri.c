#include "os.h"
#include "draw3.h"

static inline void
fragment(Image *img, short x0, short y0, u32int color)
{
	u32int *pix;
	int off = y0*img->stride + x0*4;
	pix = (u32int *)(img->img + off);
	pix[0] = color;
}

/*
 *	the top-left rule: move a bottom edge up and the right edge left
 */
static inline int
topleft(short *a, short *b)
{
	if(a[1] == b[1] && a[0] < b[0]) /* bottom */
		return -1;
	if(a[1] > b[1]) /* right */
		return -1;
	return 0; /* top or left */
}

static inline void
drawtri_horse(Image *img, Rect *r, short *a, short *b, short *c, uchar *color)
{
	int abp_y, bcp_y, cap_y, abp, bcp, cap;
	int abp_dx, bcp_dx, cap_dx;
	int abp_dy, bcp_dy, cap_dy;
	int mask;

	short u0, v0;

	abp_dx = ori2i_dx(a, b);
	bcp_dx = ori2i_dx(b, c);
	cap_dx = ori2i_dx(c, a);

	abp_dy = ori2i_dy(a, b);
	bcp_dy = ori2i_dy(b, c);
	cap_dy = ori2i_dy(c, a);

	short p[2] = { r->u0, r->v0 };
	abp_y = ori2i(a, b, p);
	bcp_y = ori2i(b, c, p);
	cap_y = ori2i(c, a, p);

	abp_y += topleft(a, b);
	bcp_y += topleft(b, c);
	cap_y += topleft(c, a);

	abp_dx = abp_dx;
	bcp_dx = bcp_dx;
	cap_dx = cap_dx;

	abp_dy = abp_dy;
	bcp_dy = bcp_dy;
	cap_dy = cap_dy;

	for(v0 = r->v0; v0 < r->vend; v0 += 1){
		abp = abp_y;
		bcp = bcp_y;
		cap = cap_y;
		for(u0 = r->u0; u0 < r->uend; u0 += 1){
			mask = (abp | bcp | cap) >> 31;
			if(mask == 0)
				fragment(img, u0, v0, *(u32int*)color);
			abp += abp_dx;
			bcp += bcp_dx;
			cap += cap_dx;
		}
		abp_y += abp_dy;
		bcp_y += bcp_dy;
		cap_y += cap_dy;
	}
}


static void
rect_trisetup(Rect *r, short *a, short *b, short *c)
{
	r->u0 = maxi(r->u0, mini(a[0], mini(b[0], c[0])));
	r->v0 = maxi(r->v0, mini(a[1], mini(b[1], c[1])));
	r->uend = mini(r->uend-1, maxi(a[0], maxi(b[0], c[0]))) + 1;
	r->vend = mini(r->vend-1, maxi(a[1], maxi(b[1], c[1]))) + 1;
}

void
drawtri(Image *img, Rect r, short *a, short *b, short *c, uchar *color)
{
	img->dirty = 1;
	r = cliprect(r, img->r);
	rect_trisetup(&r, a, b, c);
	drawtri_horse(img, &r, a, b, c, color);
}
