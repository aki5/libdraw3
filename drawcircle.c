#include "os.h"
#include "draw3.h"
#include "imgtools.h"

void
blendcircle(Image *dst, Rect dstr, Image *src, int opcode, intcoord *p, intcoord rad, int pscl)
{
	u32int *dstp, *dst_end, *dst_ustart, *dst_uend;
	u32int color32;
	int dst_stride;
	int u, v, pu, pv, r2, err2, err2_ustart;
	int tmp;
	float blend_factor;

	tmp = (p[0]-rad) >> pscl;
	if(dstr.u0 < tmp)
		dstr.u0 = tmp;
	tmp = (p[1]-rad) >> pscl;
	if(dstr.v0 < tmp)
		dstr.v0 = tmp;
	tmp = ((p[0]+rad) >> pscl)+1;
	if(dstr.uend > tmp)
		dstr.uend = tmp;
	tmp = ((p[1]+rad) >> pscl)+1;
	if(dstr.vend > tmp)
		dstr.vend = tmp;

	dstr = cliprect(dstr, dst->r);
	if(rectempty(dstr))
		return;

	dst->dirty = 1;
	blend_factor = 255.0f / (float)(1<<pscl);

	dst_stride = dst->stride/4;
	dst_ustart = img_uvstart(dst, dstr.u0, dstr.v0, 1);
	dst_end = dst_ustart + dst_stride*recth(&dstr);
	dst_uend = dst_ustart + rectw(&dstr);

	color32 = *(u32int *)src->img; /* todo: proper source image scanning instead of this hack*/

	r2 = rad*rad;

	pu = p[0];
	pv = p[1];
	u = dstr.u0<<pscl;
	v = dstr.v0<<pscl;
	err2_ustart = (pu-u)*(pu-u)+(pv-v)*(pv-v);
	while(dst_ustart < dst_end){
		dstp = dst_ustart;
		u = dstr.u0<<pscl;
		err2 = err2_ustart; //(pu-u)*(pu-u)+(pv-v)*(pv-v);
		while(dstp < dst_uend){
			passto_if(err2 <= r2){ // more than 50%
				float distf;
				distf = blend_factor * (rad-sqrtf(err2));
				distf = distf < 255.0f ? distf : 255.0f;
				if(opcode == BlendUnder){
					*dstp = blend32_under(*dstp, blend32_mask(color32, (u32int)distf<<24));
				} else if(opcode == BlendOver){
					*dstp = blend32_over(*dstp, blend32_mask(color32, (u32int)distf<<24));
				} else if(opcode == BlendSub){
					*dstp = blend32_sub(*dstp, blend32_mask(color32, (u32int)distf<<24));
				}
			}
			dstp++;
			err2 += (2*u<<pscl) + ((1<<pscl)<<pscl) - (2*pu<<pscl);
			u += 1<<pscl;
		}
		dst_ustart += dst_stride;
		dst_uend += dst_stride;
		err2_ustart += (2*v<<pscl) + ((1<<pscl)<<pscl) - (2*pv<<pscl);
		v += 1<<pscl;
	}
}
