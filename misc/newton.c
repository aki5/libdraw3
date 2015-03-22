
#include "os.h"

enum {
	Newtpsh = 32,
};

u32int
rcp17(int pshift, u32int q)
{
	s32int tag, z;

	if(q == 0)
		return -1;

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

u32int
rcp32(int pshift, u32int q)
{
	s32int tag;
	s64int z;

	if(q == 0)
		return -1;

	z = ((s64int)1 << (pshift-(63-__builtin_clzl(q))));
	tag = q >> ((63-2)-__builtin_clzl(q));
	if(tag == 7)
		z = z>>1;
	if(tag == 5 || tag == 6 || q == 3)
		z = (z|(z>>1))>>1;

	z = z + (((z<<pshift)-q*z*z)>>pshift);
	z = z + (((z<<pshift)-q*z*z)>>pshift);
	z = z + (((z<<pshift)-q*z*z)>>pshift);

	return z;
}


int
main(void)
{
	double err, maxerr;
	s64int x;
	maxerr = 0.0;
	for(x = 4; x < ((s64int)1<<Newtpsh); x++){

		int dd = ((s64int)1<<Newtpsh)/x;
		s64int newt = rcp32(Newtpsh, x);
		err = fabs(1.0*(dd-newt)/dd);
		if(iabs(dd-newt)>1){// && err > maxerr){
			maxerr = maxerr > err ? maxerr : err;
			printf("result %7lld/%-5lld div %-6d newton %-6lld error %6.2f %% maxerr %6.2f %%\n", ((s64int)1<<Newtpsh), x, dd, newt, 100.0*err, 100.0*maxerr);
		}
	}
	return 0;
}
