#include "os.h"
#include "draw3.h"
#include "imgtools.h"

#define add_wrap(ptr, d, start, end) ptr = ptr+d; ptr = ptr < end ? ptr : start

void
blend(Image *dst, Rect r, Image *src0, Image *src1)
{
	Rect dstr;
	int uoff, voff;

	dstr = cliprect(r, dst->r);
	if(rectempty(dstr))
		return;

	dst->dirty = 1;

	uoff = dstr.u0-r.u0;
	voff = dstr.v0-r.v0;

	u32int *dstp, *src0p, *src1p;
	u32int *src0_vstart, *src1_vstart;
	u32int *dst_end, *src0_end, *src1_end;
	int src0_uendoff, src1_uendoff;
	int dst_stride, src0_stride, src1_stride;
	u32int *dst_ustart, *src0_ustart, *src1_ustart;
	u32int *dst_uend, *src0_uend, *src1_uend;
	u32int mval;

	src0_vstart = img_vstart(src0, uoff);
	src1_vstart = img_vstart(src1, uoff);

	dst_ustart = img_uvstart(dst, dstr.u0, dstr.v0);
	src0_ustart = img_uvstart(src0, uoff, voff);
	src1_ustart = img_uvstart(src1, uoff, voff);

	__builtin_prefetch(dst_ustart);
	__builtin_prefetch(src0_ustart);
	__builtin_prefetch(src1_ustart);
	__builtin_prefetch(dst_ustart+8);
	__builtin_prefetch(src0_ustart+8);
	__builtin_prefetch(src1_ustart+8);

	dst_stride = dst->stride/4;
	src0_stride = src0->stride/4;
	src1_stride = src1->stride/4;

	dst_end = dst_ustart + recth(&dstr)*dst_stride;
	src0_end = img_end(src0);
	src1_end = img_end(src1);

	dst_uend = dst_ustart + rectw(&dstr);
	src0_uendoff = rectw(&src0->r) - uoff;
	src1_uendoff = rectw(&src1->r) - uoff;

	while(dst_ustart < dst_end){

		src1p = src1_ustart;
		src1_uend = src1_ustart + src1_uendoff;

		src0p = src0_ustart;
		src0_uend = src0_ustart + src0_uendoff;

		dstp = dst_ustart;
		while(dstp < dst_uend){

			__builtin_prefetch(dstp+16);
			__builtin_prefetch(src0p+16);
			__builtin_prefetch(src1p+16);

			mval = *src1p >> 24;
			goto_if(mval > 0){
				*dstp = blend32_over(*dstp, blend32_mask(*src0p, *src1p));
			}
			add_wrap(src1p, 1, src1_ustart, src1_uend);
			add_wrap(src0p, 1, src0_ustart, src0_uend);
			dstp++;
		}

		add_wrap(src1_ustart, src1_stride, src1_vstart, src1_end);
		add_wrap(src0_ustart, src0_stride, src0_vstart, src0_end);

		dst_ustart += dst_stride;
		dst_uend += dst_stride;
	}
}

void
blend2(Image *dst, Rect r, Image *src0, int opcode)
{
	Rect dstr;
	int uoff, voff;

	dstr = cliprect(r, dst->r);
	if(rectempty(dstr))
		return;

	dst->dirty = 1;

	uoff = dstr.u0-r.u0;
	voff = dstr.v0-r.v0;

	u32int *dstp, *src0p;
	u32int *src0_vstart;
	u32int *dst_end, *src0_end;
	int src0_uendoff;
	int dst_stride, src0_stride;
	u32int *dst_ustart, *src0_ustart;
	u32int *dst_uend, *src0_uend;
	u32int mval;

	src0_vstart = img_vstart(src0, uoff);

	dst_ustart = img_uvstart(dst, dstr.u0, dstr.v0);
	src0_ustart = img_uvstart(src0, uoff, voff);

	__builtin_prefetch(dst_ustart);
	__builtin_prefetch(src0_ustart);
	__builtin_prefetch(dst_ustart+8);
	__builtin_prefetch(src0_ustart+8);

	dst_stride = dst->stride/4;
	src0_stride = src0->stride/4;

	dst_end = dst_ustart + recth(&dstr)*dst_stride;
	src0_end = img_end(src0);

	dst_uend = dst_ustart + rectw(&dstr);
	src0_uendoff = rectw(&src0->r) - uoff;

	/*
	 *	The hideous #define
	 */
#define PRELUDE \
	while(dst_ustart < dst_end){\
		src0p = src0_ustart;\
		src0_uend = src0_ustart + src0_uendoff;\
		dstp = dst_ustart;\
		while(dstp < dst_uend){\
			__builtin_prefetch(dstp+16);\
			__builtin_prefetch(src0p+16);
	/*
	 *	insert *dstp = blendfunc(*dstp, *src0p) here
	 */
#define POSTLUDE \
			add_wrap(src0p, 1, src0_ustart, src0_uend);\
			dstp++;\
		}\
		add_wrap(src0_ustart, src0_stride, src0_vstart, src0_end);\
		dst_ustart += dst_stride;\
		dst_uend += dst_stride;\
	}

	if(opcode == BlendOver){
		PRELUDE{
			mval = 255 - (*src0p >> 24);
			goto_if(mval-1 < 254){
				*dstp = blend32_over(*src0p, *dstp);
			} else {
				*dstp = (mval == 0) ? *src0p : *dstp;
			}
		}POSTLUDE
	} else if(opcode == BlendUnder){
		PRELUDE{
			mval = *dstp >> 24;
			goto_if(mval-1 < 254){
				*dstp = blend32_over(*dstp, *src0p);
			} else if(mval == 0){
				*dstp = *src0p;
			}
		}POSTLUDE
	}

#undef PRELUDE
#undef POSTLUDE
	/*
	 *	Phew. I hate #define.
	 */
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
