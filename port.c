#include "os.h"
#include "draw3.h"


void
pixcpy_src8(uchar *dst, uchar *src, int nsrc)
{
	int i;

	for(i = 0; i < nsrc && ((uintptr)src & 3) != 0; i++){
		u32int a;
		a = *src;
		a |= a<<8;
		a |= a<<16;
		*(u32int*)dst = a;
		dst += 4;
		src += 1;
	}

	/* insert cacheline copier here  */

	for(; i < nsrc; i++){
		u32int a;
		a = *src;
		a |= a<<8;
		a |= a<<16;
		*(u32int*)dst = a;
		dst += 4;
		src += 1;
	}
}

void
pixcpy_dst16(uchar *dst, uchar *src, int nbytes)
{
	int i;

	i = 0;
	/* insert cacheline copier here  */
	nbytes -= 3;
	for(; i < nbytes; i += 4){
		u32int pix;
		pix = *(u32int*)src;
		*(u16int*)dst = ((pix & 0xf8)>>3) | ((pix & 0xfc00)>>5) | ((pix & 0xf80000)>>8);
		src += 4;
		dst += 2;
	}
}

void
pixset(uchar *dst, uchar *val, int nbytes)
{
	u32int a;
	int i;

	a = *(u32int*)val;

	/* align to cache-line */
	for(i = 0; i < nbytes && ((uintptr)dst & 31) != 0; i += 4){
		*(u32int*)dst = a;
		dst += 4;
	}

	/* insert cacheline copier here  */

	/* the remainder */
	for(; i < nbytes; i += 4){
		*(u32int*)dst = a;
		dst += 4;
	}
}

