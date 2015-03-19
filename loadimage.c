#include "os.h"
#include "draw3.h"

void
loadimage8(Image *dst, Rect dstr, uchar *srcp, int src_stride)
{
	Rect r;
	uchar *dstp;
	int dst_stride;
	int i;
	int uoff, voff;

	dst->dirty = 1;

	r = cliprect(dstr, dst->r);
	uoff = r.u0 - dstr.u0;
	voff = r.v0 - dstr.v0;

	dst_stride = dst->stride;
	dstp = dst->img + dst_stride*voff + 4*uoff;

	for(i = r.v0; i < r.vend; i++){
		pixcpy_src8(dstp, srcp, r.uend-r.u0);
		srcp += src_stride;
		dstp += dst_stride;
	}
}

