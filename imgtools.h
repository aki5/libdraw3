#define passto_if(x) if(__builtin_expect(x, 1))
#define goto_if(x) if(__builtin_expect(x, 0))

static inline s32int
rcp17(s32int pshift, s32int q)
{
	s32int tag, z;

	if(q==0){
		return 1<<pshift;
	}

	z = 1 << (pshift-(31-__builtin_clz(q)));
	tag = q >> ((31-2)-__builtin_clz(q));

	if(tag == 7)
		z = z>>1;
	if(tag == 5 || tag == 6 || q == 3)
		z = (z|(z>>1))>>1;

	z = z + (((z<<pshift)-q*z*z)>>pshift);
	z = z + (((z<<pshift)-q*z*z)>>pshift);
	z = z + (((z<<pshift)-q*z*z)>>pshift);

	return z;
}

static inline u32int
rcp32(int pshift, u32int q)
{
	s32int tag;
	s64int z;

	if(q == 0){
		return 1<<pshift;
	}

	z = ((s64int)1 << (pshift-(31-__builtin_clz(q))));
	tag = q >> ((31-2)-__builtin_clz(q));
	if(tag == 7)
		z = z>>1;
	if(tag == 5 || tag == 6 || q == 3)
		z = (z|(z>>1))>>1;

	z = z + (((z<<pshift)-q*z*z)>>pshift);
	z = z + (((z<<pshift)-q*z*z)>>pshift);
	z = z + (((z<<pshift)-q*z*z)>>pshift);

	return z;
}

static inline u32int
blend32(u32int dval, u32int sval, u32int mval)
{
	u32int dtmp, stmp, tmp1, tmp2;

	/* multiply red and blue at the same time */
	dtmp = dval & 0x00ff00ff;
	stmp = sval & 0x00ff00ff;
	tmp1 = dtmp * 255;
	tmp1 += stmp * mval;

	/* divide by 255 */
	tmp1 += 0x00010001;
	tmp1 += (tmp1>>8) & 0x00ff00ff;
	tmp1 >>= 8;
	tmp1 = tmp1 & 0x00ff00ff; 

	/* multiply green and alpha at the same time */
	dtmp = (dval>>8) & 0x00ff00ff;
	stmp = (sval>>8) & 0x00ff00ff;
	tmp2 = dtmp * 255;
	tmp2 += stmp * mval;

	/* divide by 255 */
	tmp2 += 0x00010001;
	tmp2 += (tmp2>>8) & 0x00ff00ff;
	tmp2 = tmp2 & 0xff00ff00; 

	return tmp1 | tmp2;
}

static inline u32int
blend32_under(u32int dval, u32int sval)
{
	u32int mval;
	mval = 255 - (dval >> 24);
	return blend32(dval, sval, mval);
}

static inline u32int
blend32_over(u32int dval, u32int sval)
{
	u32int mval;
	mval = 255 - (sval >> 24);
	return blend32(sval, dval, mval);
}

static inline u32int
blend32_sub(u32int dval, u32int sval)
{
	u32int mval;
	mval = 255 - (sval >> 24);
	return blend32(0, dval, mval);
}

static inline u32int
blend32_mask(u32int dval, u32int sval)
{
	u32int mval;
	mval = sval >> 24;
	return blend32(0, dval, mval);
}

static inline u32int *
img_uvstart(Image *img, int uoff, int voff)
{
	int u0, v0;
	while(uoff < 0)
		uoff += rectw(&img->r);
	while(voff < 0)
		voff += rectw(&img->r);
	//u0 = img->r.u0 + (uoff % rectw(&img->r));
	//v0 = img->r.v0 + (voff % recth(&img->r));
	u0 = (uoff % rectw(&img->r));
	v0 = (voff % recth(&img->r));
	return (u32int*)(img->img + img->stride*v0 + 4*u0);
}

static inline u32int *
img_vstart(Image *img, int uoff)
{
	int u0, v0;
	while(uoff < 0)
		uoff += rectw(&img->r);
	//u0 = img->r.u0 + (uoff % rectw(&img->r));
	u0 = (uoff % rectw(&img->r));
	v0 = img->r.v0;
	return (u32int*)(img->img + img->stride*v0 + 4*u0);
}

static inline int
img_uwrap(Image *img, int uoff)
{
	while(uoff < 0)
		uoff += rectw(&img->r);
	uoff %= rectw(&img->r);
	return rectw(&img->r) - uoff;
}

static inline u32int *
img_end(Image *img)
{
	return (u32int*)(img->img + img->stride*img->r.vend);
}

