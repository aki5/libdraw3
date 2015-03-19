#include "os.h"
#include "draw3.h"

typedef struct Samp2d Samp2d;
struct Samp2d {
	int u;
	int u0;
	int ustep;
	int uoff;
	int umod;
	int ustride;

	int v;
	int v0;
	int vstep;
	int voff;
	int vmod;
	int vstride;
};

static inline void
initsamp(
	Samp2d *samp,
	int uoff, int u0, int ustep, int umod, int ustride,
	int voff, int v0, int vstep, int vmod, int vstride
){
	samp->u0 = ustride*u0;
	samp->umod = ustride*umod;
	samp->ustep = ustride*ustep;
	samp->uoff = ustride*(u0 + (uoff - u0) % umod);
	samp->u = samp->uoff;
	samp->ustride = ustride;

	samp->v0 = vstride*v0;
	samp->vmod = vstride*vmod;
	samp->vstep = vstride*vstep;
	samp->voff = vstride*(v0 + (voff - v0) % vmod);
	samp->v = samp->voff;
	samp->vstride = vstride;
}

static inline void
uresetsamp(Samp2d *samp)
{
	samp->u = samp->uoff;
}

static inline void
ustepsamp(Samp2d *samp)
{
//samp->u = samp->u0 + (samp->u + samp->ustep - samp->u0)%samp->umod;
	int a, b, m;
	a = samp->u + samp->ustep - samp->u0;
	b = samp->u + samp->ustep - samp->u0 - samp->umod;
	m = b>>31;
	samp->u = samp->u0 + ((a&m)|(b&~m));
}

static inline void
vstepsamp(Samp2d *samp)
{
//samp->v = samp->v0 + (samp->v + samp->vstep - samp->v0)%samp->vmod;
	int a, b, m;
	a = samp->v + samp->vstep - samp->v0;
	b = samp->v + samp->vstep - samp->v0 - samp->vmod;
	m = b>>31;
	samp->v = samp->v0 + ((a&m)|(b&~m));
}

static inline uchar *
imagepix(Image *img, short u, short v)
{
	u = img->r.u0 + (u - img->r.u0) % rectw(&img->r);
	v = img->r.v0 + (v - img->r.v0) % recth(&img->r);
	return img->img + img->stride*v + 4*u;
}


static inline uchar
blend1(uchar dst, uchar src, uchar a)
{
//	return (dst*(255-a) + src*a)/255;

	u16int tmp;
	tmp = (dst*(255-a) + src*a);
	return (tmp*257+256)>>16;
//	return ((tmp+1)*257)>>16;
//	tmp += 1; return (tmp+(tmp>>8))>>8;
}

typedef int s32vec __attribute__((vector_size(16)));

static inline void
blend4(uchar *dst, uchar *src, uchar *mask)
{
	s32vec svec = (s32vec){src[0], src[1], src[2], src[3]};
	s32vec mvec = (s32vec){mask[0], mask[1], mask[2], mask[3]};
	s32vec dvec = (s32vec){dst[0], dst[1], dst[2], dst[3]};

	dvec = (dvec*(255-mvec) + svec*mvec)/255;

	dst[0] = dvec[0];
	dst[1] = dvec[1];
	dst[2] = dvec[2];
	dst[3] = dvec[3];
}

void
drawblend(Image *dst, Rect r, Image *src, Image *mask)
{
	Samp2d msamp, ssamp;
	Rect dstr;
	uchar *dstp, *srcp, *maskp;
	short u, v;

	dst->dirty = 1;
	dstr = cliprect(r, dst->r);

	if(dstr.u0 >= dstr.uend || dstr.v0 >= dstr.vend)
		return;

	initsamp(
		&ssamp,
		dstr.u0-r.u0, src->r.u0, 1, rectw(&src->r), 4,
		dstr.v0-r.v0, src->r.v0, 1, recth(&src->r), src->stride
	);

	initsamp(
		&msamp,
		dstr.u0-r.u0, mask->r.u0, 1, rectw(&mask->r), 4,
		dstr.v0-r.v0, mask->r.v0, 1, recth(&mask->r), mask->stride
	);

	for(v = dstr.v0; v < dstr.vend; v++){
		uresetsamp(&ssamp);
		uresetsamp(&msamp);
		for(u = dstr.u0; u < dstr.uend; u++){
			dstp = dst->img + 4*u + dst->stride*v;
			srcp = src->img + ssamp.u + ssamp.v;
			maskp = mask->img + msamp.u + msamp.v;
			blend4(dstp, srcp, maskp);
			ustepsamp(&ssamp);
			ustepsamp(&msamp);
		}
		vstepsamp(&ssamp);
		vstepsamp(&msamp);
	}
}


void
drawrect(Image *dst, Rect r, uchar *color)
{
	Rect dstr;

	short u, v;
	u32int *pix, cval;
	int uoff, voff;

	dstr = cliprect(r, dst->r);

	if(dstr.u0 >= dstr.uend || dstr.v0 >= dstr.vend)
		return;

	dst->dirty = 1;
	cval = *(u32int*)color;
	uoff = dstr.u0*4;
	voff = dstr.v0*dst->stride;
	for(v = dstr.v0; v < dstr.vend; v++){
		uoff = dstr.u0*4;
		for(u = dstr.u0; u < dstr.uend; u++){
			pix = (u32int*)(dst->img + voff + uoff);
			*pix = cval;
			uoff += 4;
		}
		voff += dst->stride;
	}
}
