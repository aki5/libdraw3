#include "os.h"
#include "draw3.h"
#include "imgtools.h"

#define add_wrap(ptr, d, start, end) ptr = ptr+d; ptr = ptr < end ? ptr : start

void
drawblend(Image *dst, Rect r, Image *src, Image *mask)
{
	Rect dstr;
	int uoff, voff;

	dstr = cliprect(r, dst->r);
	if(rectempty(dstr))
		return;

	dst->dirty = 1;

	uoff = dstr.u0-r.u0;
	voff = dstr.v0-r.v0;

	u32int *dstp, *srcp, *maskp;
	u32int *src_vstart, *mask_vstart;
	u32int *dst_end, *src_end, *mask_end;
	int src_uendoff, mask_uendoff;
	int dst_stride, src_stride, mask_stride;
	u32int *dst_ustart, *src_ustart, *mask_ustart;
	u32int *dst_uend, *src_uend, *mask_uend;
	u32int mval;

	src_vstart = img_vstart(src, uoff);
	mask_vstart = img_vstart(mask, uoff);

	dst_ustart = img_uvstart(dst, dstr.u0, dstr.v0);
	src_ustart = img_uvstart(src, uoff, voff);
	mask_ustart = img_uvstart(mask, uoff, voff);

	__builtin_prefetch(dst_ustart);
	__builtin_prefetch(src_ustart);
	__builtin_prefetch(mask_ustart);
	__builtin_prefetch(dst_ustart+8);
	__builtin_prefetch(src_ustart+8);
	__builtin_prefetch(mask_ustart+8);

	dst_stride = dst->stride/4;
	src_stride = src->stride/4;
	mask_stride = mask->stride/4;

	dst_end = dst_ustart + recth(&dstr)*dst_stride;
	src_end = img_end(src);
	mask_end = img_end(mask);

	dst_uend = dst_ustart + rectw(&dstr);
	src_uendoff = rectw(&src->r) - uoff;
	mask_uendoff = rectw(&mask->r) - uoff;

	while(dst_ustart < dst_end){

		maskp = mask_ustart;
		mask_uend = mask_ustart + mask_uendoff;

		srcp = src_ustart;
		src_uend = src_ustart + src_uendoff;

		dstp = dst_ustart;
		while(dstp < dst_uend){

			__builtin_prefetch(dstp+16);
			__builtin_prefetch(srcp+16);
			__builtin_prefetch(maskp+16);

			/* the secret to fast alpha blending is avoiding it */
			mval = *maskp >> 24;
			goto_if(mval-1 < 254){
				u32int sval;
				//sval = premul32(*srcp, mval);
				sval = *srcp;
				*dstp = blend32(*dstp, sval, mval);
			} else if(mval == 255){
				*dstp = *srcp;
			}

			add_wrap(maskp, 1, mask_ustart, mask_uend);
			add_wrap(srcp, 1, src_ustart, src_uend);
			dstp++;
		}

		add_wrap(mask_ustart, mask_stride, mask_vstart, mask_end);
		add_wrap(src_ustart, src_stride, src_vstart, src_end);

		dst_ustart += dst_stride;
		dst_uend += dst_stride;
	}
}

void
drawblend_back(Image *dst, Rect r, Image *src)
{
	Rect dstr;
	int uoff, voff;

	dstr = cliprect(r, dst->r);
	if(rectempty(dstr))
		return;

	dst->dirty = 1;

	uoff = dstr.u0-r.u0;
	voff = dstr.v0-r.v0;

	u32int *dstp, *srcp;
	u32int *src_vstart;
	u32int *dst_end, *src_end;
	int src_uendoff;
	int dst_stride, src_stride;
	u32int *dst_ustart, *src_ustart;
	u32int *dst_uend, *src_uend;
	u32int mval;

	src_vstart = img_vstart(src, uoff);

	dst_ustart = img_uvstart(dst, dstr.u0, dstr.v0);
	src_ustart = img_uvstart(src, uoff, voff);

	__builtin_prefetch(dst_ustart);
	__builtin_prefetch(src_ustart);
	__builtin_prefetch(dst_ustart+8);
	__builtin_prefetch(src_ustart+8);

	dst_stride = dst->stride/4;
	src_stride = src->stride/4;

	dst_end = dst_ustart + recth(&dstr)*dst_stride;
	src_end = img_end(src);

	dst_uend = dst_ustart + rectw(&dstr);
	src_uendoff = rectw(&src->r) - uoff;

	while(dst_ustart < dst_end){

		srcp = src_ustart;
		src_uend = src_ustart + src_uendoff;

		dstp = dst_ustart;
		while(dstp < dst_uend){

			__builtin_prefetch(dstp+16);
			__builtin_prefetch(srcp+16);

			/* the secret to fast alpha blending is avoiding it */
			mval = 255-(*dstp >> 24);
			goto_if(mval-1 < 254){
				*dstp = blend32(*dstp, *srcp, mval);
			} else if(mval == 255){
				*dstp = *srcp;
			}

			add_wrap(srcp, 1, src_ustart, src_uend);
			dstp++;
		}

		add_wrap(src_ustart, src_stride, src_vstart, src_end);

		dst_ustart += dst_stride;
		dst_uend += dst_stride;
	}
}

void
drawrect(Image *dst, Rect r, uchar *color)
{
	Rect dstr;
	uchar *dstp;
	int i, nbytes;
	int dst_stride;

	dstr = cliprect(r, dst->r);

	if(dstr.u0 >= dstr.uend || dstr.v0 >= dstr.vend)
		return;

	dst->dirty = 1;
	dst_stride = dst->stride;
	dstp = dst->img + dst_stride*dstr.v0 + 4*dstr.u0;
	nbytes = 4*rectw(&dstr);
	for(i = dstr.v0; i < dstr.vend; i++){
		pixset(dstp, color, nbytes);
		dstp += dst->stride;
	}
}
