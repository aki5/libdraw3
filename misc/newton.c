
#include "os.h"

enum {
	Newtpsh = 16,
};

static inline s32int
rcp32(s32int pshift, s32int q)
{

	s32int tag, z;

	z = 1 << (pshift-(31-__builtin_clz(q)));
	tag = q >> ((31-2)-__builtin_clz(q));

	if(tag == 7)
		z = z>>1;
	if(tag == 5 || tag == 6 || q == 3)
		z = (z|(z>>1))>>1;

	z = z + ((z*((1<<pshift)-q*z))>>pshift);
	z = z + ((z*((1<<pshift)-q*z))>>pshift);
	z = z + ((z*((1<<pshift)-q*z))>>pshift);

	return z;
}

int
main(void)
{
	double err;
	int x, tag;
	for(x = 1; x < (1<<Newtpsh); x++){
		int dd = (1<<Newtpsh)/x;
		int newt = rcp32(Newtpsh, x);
		err = fabs(1.0*(dd-newt)/dd);
		if(iabs(dd-newt)>1){// && err > maxerr){
			printf("result %7d/%-5d div %-6d newton %-6d tag %d error %6.2f %%\n", (1<<Newtpsh), x, dd, newt, tag, 100.0*err);
		}
	}
	return 0;
}
