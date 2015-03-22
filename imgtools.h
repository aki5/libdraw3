
#define passto_if(x) if(__builtin_expect(x, 1))
#define goto_if(x) if(__builtin_expect(x, 0))

static inline u32int
blend32(u32int dval, u32int sval, u32int mval)
{
	u32int dtmp, stmp, tmp1, tmp2;

	/* multiply red and blue at the same time */
	dtmp = dval & 0x00ff00ff;
	stmp = sval & 0x00ff00ff;
	tmp1 = dtmp * (255-mval);
	tmp1 += stmp * mval;

	/* divide by 255 */
	tmp1 += 0x00010001;
	tmp1 += (tmp1>>8) & 0x00ff00ff;
	tmp1 >>= 8;
	tmp1 = tmp1 & 0x00ff00ff; 

	/* green goes alone */
	dtmp = dval & 0xff00;
	stmp = sval & 0xff00;
	tmp2 = dtmp * (255-mval);
	tmp2 += stmp * mval;

	/* divide by 255 */
	tmp2 += 0x0100;
	tmp2 += (tmp1>>8);
	tmp2 >>= 8;
	tmp2 = tmp2 & 0xff00;

	return tmp1 | tmp2;
}

static inline u32int *
img_uvstart(Image *img, int uoff, int voff)
{
	int u0, v0;
	u0 = img->r.u0 + (uoff % rectw(&img->r));
	v0 = img->r.v0 + (voff % recth(&img->r));
	return (u32int*)(img->img + img->stride*v0 + 4*u0);
}

static inline u32int *
img_vstart(Image *img, int uoff)
{
	int u0, v0;
	u0 = img->r.u0 + (uoff % rectw(&img->r));
	v0 = img->r.v0;
	return (u32int*)(img->img + img->stride*v0 + 4*u0);
}

static inline u32int *
img_end(Image *img)
{
	return (u32int*)(img->img + img->stride*img->r.vend);
}
