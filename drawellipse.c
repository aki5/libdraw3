
#include "os.h"
#include "draw3.h"
#include "imgtools.h"

void
drawellipse(Image *dst, Rect dstr, short *a, short *b, short rad, int pscl, uchar *color)
{
	u32int *dstp, *dst_end, *dst_ustart, *dst_uend;
	u32int color32;
	int dst_stride;
	int u, v;
	short ab[2];
	short tmp;


	/* todo: do the bounding box right. this one's way too large */
	tmp = (mini(a[0], b[0])-rad) >> pscl;
	if(dstr.u0 < tmp)
		dstr.u0 = tmp;
	tmp = (mini(a[1], b[1])-rad) >> pscl;
	if(dstr.v0 < tmp)
		dstr.v0 = tmp;
	tmp = ((maxi(a[0], b[0])+rad) >> pscl) + 1;
	if(dstr.uend > tmp)
		dstr.uend = tmp;
	tmp = ((maxi(a[1], b[1])+rad) >> pscl) + 1;
	if(dstr.vend > tmp)
		dstr.vend = tmp;

	ab[0] = b[0]-a[0];
	ab[1] = b[1]-a[1];
	rad += sqrtf(ab[0]*ab[0] + ab[1]*ab[1]);

	dstr = cliprect(dstr, dst->r);

	dst_stride = dst->stride/4;
	dst_ustart = img_uvstart(dst, dstr.u0, dstr.v0);
	dst_end = dst_ustart + dst_stride*recth(&dstr);
	dst_uend = dst_ustart + rectw(&dstr);
	color32 = *(u32int *)color;

	u = dstr.u0 << pscl;
	v = dstr.v0 << pscl;

	float blend_limit, blend_factor;
	blend_limit = (float)(1<<pscl);
	blend_factor = 255.0f / blend_limit;

	while(dst_ustart < dst_end){
		dstp = dst_ustart;
		u = dstr.u0 << pscl;
		while(dstp < dst_uend){
			float adst, bdst, dst;
			int adst2, bdst2;
			adst2 = (u-a[0])*(u-a[0]) + (v-a[1])*(v-a[1]);
			bdst2 = (u-b[0])*(u-b[0]) + (v-b[1])*(v-b[1]);
			adst = sqrtf(adst2);
			bdst = sqrtf(bdst2);
			dst = rad-(adst+bdst);
			if(dst > 0.0f){
				if(dst < blend_limit){ // todo: work out the math here. this fudge doesn't really work
					dst *= blend_factor;
					*dstp = blend32(*dstp, color32, (int)dst);
				} else {
					*dstp = color32;
				}
			}
			dstp++;
			u += 1<<pscl;
		}
		dst_ustart += dst_stride;
		dst_uend += dst_stride;
		v += 1<<pscl;
	}
}
