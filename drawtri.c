#include "os.h"
#include "draw3.h"
#include "imgtools.h"

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
drawtri_horse(Image *dst, Rect *dstr, short *a, short *b, short *c, int pscl, uchar *color)
{
        u32int *dstp;
        u32int *dst_ustart;
        u32int *dst_uend;
        u32int *dst_end;
	u32int color32;
        int dst_stride;

	int abp_y, bcp_y, cap_y, abp, bcp, cap;
	int abp_dx, bcp_dx, cap_dx;
	int abp_dy, bcp_dy, cap_dy;

	dst_stride = dst->stride/4;
	dst_ustart = img_uvstart(dst, dstr->u0, dstr->v0);
	dst_end = dst_ustart + dst_stride*recth(dstr);
	dst_uend = dst_ustart + rectw(dstr);
	color32 = *(u32int *)color;

	abp_dx = ori2i_dx(a, b) << pscl;
	bcp_dx = ori2i_dx(b, c) << pscl;
	cap_dx = ori2i_dx(c, a) << pscl;

	abp_dy = ori2i_dy(a, b) << pscl;
	bcp_dy = ori2i_dy(b, c) << pscl;
	cap_dy = ori2i_dy(c, a) << pscl;

	/* top-left corner of our scan region needs to be scaled */
	short pstart[2] = {
		dstr->u0 << pscl,
		dstr->v0 << pscl
	};
	abp_y = ori2i(a, b, pstart);
	bcp_y = ori2i(b, c, pstart);
	cap_y = ori2i(c, a, pstart);

	if(pscl == 0){
		abp_y += topleft(a, b);
		bcp_y += topleft(b, c);
		cap_y += topleft(c, a);
	} /* else: todo.. */

	while(dst_ustart < dst_end){
		abp = abp_y;
		bcp = bcp_y;
		cap = cap_y;

		dstp = dst_ustart;
		while(dstp < dst_uend){
			passto_if(((abp | bcp | cap) >> 31) == 0){ // less than 50%.
#if 0
				*dstp = color32;
#else
				int need_aa;
				need_aa  = abp-abp_dx;
				need_aa |= bcp-bcp_dx;
				need_aa |= cap-cap_dx;
				need_aa |= abp-abp_dy;
				need_aa |= bcp-bcp_dy;
				need_aa |= cap-cap_dy;

				need_aa |= abp+abp_dx;
				need_aa |= bcp+bcp_dx;
				need_aa |= cap+cap_dx;
				need_aa |= abp+abp_dy;
				need_aa |= bcp+bcp_dy;
				need_aa |= cap+cap_dy;

				goto_if((need_aa>>31) == -1){ // extremely rare

if(0)
printf(
"abp %d bcp %d cap %d "
"abp_dx %d bcp_dx %d cap_dx %d "
"abp_dy %d bcp_dy %d cap_dy %d "
"\n",
abp, bcp, cap,
abp_dx, bcp_dx, cap_dx,
abp_dy, bcp_dy, cap_dy
);
					int tmp, xx;
					tmp = 255;
					xx = 255;
					if(abp_dx != 0)
						xx = 255*abp/iabs(abp_dx);
					tmp = mini(tmp, xx);
					if(bcp_dx != 0)
						xx = 255*bcp/iabs(bcp_dx);
					tmp = mini(tmp, xx);
					if(cap_dx != 0)
						xx = 255*cap/iabs(cap_dx);
					tmp = mini(tmp, xx);

					if(abp_dy != 0)
						xx = 255*abp/iabs(abp_dy);
					tmp = mini(tmp, xx);
					if(bcp_dy != 0)
						xx = 255*bcp/iabs(bcp_dy);
					tmp = mini(tmp, xx);
					if(cap_dy != 0)
						xx = 255*cap/iabs(cap_dy);
					tmp = mini(tmp, xx);

					*dstp = blend32(*dstp, color32, tmp);
				} else {
					*dstp = color32;
				}
#endif
			}
			abp += abp_dx;
			bcp += bcp_dx;
			cap += cap_dx;
			dstp++;
		}

		abp_y += abp_dy;
		bcp_y += bcp_dy;
		cap_y += cap_dy;

		dst_ustart += dst_stride;
		dst_uend += dst_stride;
	}
}

static void
rect_trisetup(Rect *r, short *a, short *b, short *c, int pscl)
{
	r->u0 = maxi(r->u0 << pscl, mini(a[0], mini(b[0], c[0])));
	r->v0 = maxi(r->v0 << pscl, mini(a[1], mini(b[1], c[1])));
	r->uend = mini((r->uend-1) << pscl, maxi(a[0], maxi(b[0], c[0])));
	r->vend = mini((r->vend-1) << pscl, maxi(a[1], maxi(b[1], c[1])));
	r->u0 = r->u0 >> pscl;
	r->v0 = r->v0 >> pscl;
	r->uend = (r->uend >> pscl) + 1;
	r->vend = (r->vend >> pscl) + 1;
}

void
drawtri(Image *dst, Rect r, short *a, short *b, short *c, uchar *color)
{
	dst->dirty = 1;
	r = cliprect(r, dst->r);
	rect_trisetup(&r, a, b, c, 0);
	drawtri_horse(dst, &r, a, b, c, 0, color);
}

void
drawtri_pscl(Image *dst, Rect r, short *a, short *b, short *c, int pscl, uchar *color)
{
	dst->dirty = 1;
	r = cliprect(r, dst->r);
	rect_trisetup(&r, a, b, c, pscl);
	drawtri_horse(dst, &r, a, b, c, pscl, color);
}
