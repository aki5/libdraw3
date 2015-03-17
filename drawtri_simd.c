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
fragment(Image *img, short x0, short y0, u32int color, v4int mask)
{
	u32int *pix;
	int off = y0*img->stride + x0*4;
	pix = (u32int *)(img->img + off);
	if(mask[0] == 0)
		pix[0] = color;
	if(mask[1] == 0)
		pix[1] = color;
	if(mask[2] == 0)
		pix[img->stride/4] = color;
	if(mask[3] == 0)
		pix[img->stride/4+1] = color;

}

/*
 *	the top-left rule: move a bottom edge up and the right edge left
 */
static inline v4int
topleft(short *a, short *b)
{
	if(a[1] == b[1] && a[0] < b[0]) /* bottom */
		return vec4ir(-1);
	if(a[1] > b[1]) /* right */
		return vec4ir(-1);
	return vec4ir(0); /* top or left */
}

static void
rect_trisetup(Rect *r, short *a, short *b, short *c)
{
	r->u0 = maxi(r->u0, mini(a[0], mini(b[0], c[0])));
	r->v0 = maxi(r->v0, mini(a[1], mini(b[1], c[1])));
	r->uend = mini(r->uend-1, maxi(a[0], maxi(b[0], c[0]))) + 1;
	r->vend = mini(r->vend-1, maxi(a[1], maxi(b[1], c[1]))) + 1;
}

static inline void
drawtri_horse(Image *img, Rect *r, short *a, short *b, short *c, uchar *color)
{
	v4int abp_y, bcp_y, cap_y, abp, bcp, cap;
	v4int abp_dx, bcp_dx, cap_dx;
	v4int abp_dy, bcp_dy, cap_dy;
	v4int mask;

	short u0, v0;

	enum { ustep = 2, vstep = 2 };

	abp_dx = vec4ir(ori2i_dx(a, b));
	bcp_dx = vec4ir(ori2i_dx(b, c));
	cap_dx = vec4ir(ori2i_dx(c, a));

	abp_dy = vec4ir(ori2i_dy(a, b));
	bcp_dy = vec4ir(ori2i_dy(b, c));
	cap_dy = vec4ir(ori2i_dy(c, a));

	short p[2] = { r->u0, r->v0 };
	abp_y = vec4ir(ori2i(a, b, p)) + vec4i(0, abp_dx[0], abp_dy[0], abp_dx[0]+abp_dy[0]);
	bcp_y = vec4ir(ori2i(b, c, p)) + vec4i(0, bcp_dx[0], bcp_dy[0], bcp_dx[0]+bcp_dy[0]);
	cap_y = vec4ir(ori2i(c, a, p)) + vec4i(0, cap_dx[0], cap_dy[0], cap_dx[0]+cap_dy[0]);

	abp_y += topleft(a, b);
	bcp_y += topleft(b, c);
	cap_y += topleft(c, a);

	abp_dx = abp_dx * ustep;
	bcp_dx = bcp_dx * ustep;
	cap_dx = cap_dx * ustep;

	abp_dy = abp_dy * vstep;
	bcp_dy = bcp_dy * vstep;
	cap_dy = cap_dy * vstep;

	for(v0 = r->v0; v0 < r->vend; v0 += vstep){
		abp = abp_y;
		bcp = bcp_y;
		cap = cap_y;
		for(u0 = r->u0; u0 < r->uend; u0 += ustep){
			mask = (abp | bcp | cap) >> 31;
			if((mask[0]&mask[1]&mask[2]&mask[3]) == 0){
				mask |= vec4i(0, -(u0 == r->uend-1), 0, -(u0 == r->uend-1));
				mask |= vec4i(0, 0, -(v0 == r->vend-1), -(v0 == r->vend-1));
				fragment(img, u0, v0, *(u32int*)color, mask);
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

void
drawtri(Image *img, Rect r, short *a, short *b, short *c, uchar *color)
{
	img->dirty = 1;
	r = cliprect(r, img->r);
	rect_trisetup(&r, a, b, c);
	drawtri_horse(img, &r, a, b, c, color);
}
