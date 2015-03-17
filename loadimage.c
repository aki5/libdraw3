#include "os.h"
#include "draw3.h"

void
loadimage(Image *img, Rect clipr, uchar *data)
{
	Rect r;
	int u, v;
	int uoff, voff;
	u32int *pix;
	int dstride;

	r = cliprect(clipr, img->r);
	dstride = clipr.uend - clipr.u0;
	uoff = r.u0 - clipr.u0;
	voff = r.v0 - clipr.v0;

	img->dirty = 1;
	for(v = r.v0; v < r.vend; v++){
		for(u = r.u0; u < r.uend; u++){
			pix = (u32int*)(img->img + v*img->stride) + u;
			*pix = *((u32int*)data + (v+voff)*dstride + (u+uoff));
		}
	}
}

void
loadimage8(Image *img, Rect clipr, uchar *data, int stride)
{
	Rect r;
	int u, v;
	int uoff, voff;
	uchar *pix;

	r = cliprect(clipr, img->r);
	uoff = r.u0 - clipr.u0;
	voff = r.v0 - clipr.v0;

	img->dirty = 1;
	for(v = r.v0; v < r.vend; v++){
		for(u = r.u0; u < r.uend; u++){
			uchar grey8;
			pix = img->img + v*img->stride + 4*u;
			grey8 = *((uchar*)data + ((v-r.v0)+voff)*stride + ((u-r.u0)+uoff));
#if 0
			pix[0] = pix[0]*(255-grey8)/255 + grey8 ;
			pix[1] = pix[1]*(255-grey8)/255 + grey8 ;
			pix[2] = pix[2]*(255-grey8)/255 + grey8 ;
			pix[3] = pix[3]*(255-grey8)/255 + grey8 ;
#else
			pix[0] = grey8;
			pix[1] = grey8;
			pix[2] = grey8;
			pix[3] = grey8;
#endif
		}

	}
}

void
loadimage24(Image *img, Rect clipr, uchar *data, int stride)
{
	Rect r;
	int u, v;
	int uoff, voff;
	int srcu, srcv;
	uchar *pix;

	r = cliprect(clipr, img->r);
	uoff = r.u0 - clipr.u0;
	voff = r.v0 - clipr.v0;
	img->dirty = 1;
	for(v = r.v0; v < r.vend; v++){
		for(u = r.u0; u < r.uend; u++){
			uchar *grey24;
			pix = img->img + v*img->stride + 4*u;
			srcu = (u-r.u0)+uoff;
			srcv = (v-r.v0)+voff;
			grey24 = data + srcv*stride + 3*srcu;
#if 0
			pix[0] = pix[0]*(255-grey8)/255 + grey8 ;
			pix[1] = pix[1]*(255-grey8)/255 + grey8 ;
			pix[2] = pix[2]*(255-grey8)/255 + grey8 ;
			pix[3] = pix[3]*(255-grey8)/255 + grey8 ;
#else
			pix[0] = grey24[0];
			pix[1] = grey24[1];
			pix[2] = grey24[2];
			pix[3] = 0xff;
#endif
		}
	}
}
