
#include "os.h"

enum {
	Newtpsh = 16,
};

int
newton32(s32int q, int *tagp)
{
	s32int tag, z;

	z = 1 << (Newtpsh-(31-__builtin_clz(q)));
	tag = q >> ((31-2)-__builtin_clz(q));

	if(tag == 7)
		z = z>>1;
	if(tag == 5 || tag == 6 || q == 3)
		z = (z|(z>>1))>>1;

	z = z + ((z*((1<<Newtpsh)-q*z))>>Newtpsh);
	z = z + ((z*((1<<Newtpsh)-q*z))>>Newtpsh);

	*tagp = tag;
	return z;
}

int
main(void)
{
	double err;
	int x, tag;
	for(x = 1; x < (1<<Newtpsh); x++){
		int dd = (1<<Newtpsh)/x;
		int newt = newton32(x, &tag);
		err = fabs(1.0*(dd-newt)/dd);
		if(iabs(dd-newt)>1){// && err > maxerr){
			printf("result %7d/%-5d div %-6d newton %-6d tag %d error %6.2f %%\n", (1<<Newtpsh), x, dd, newt, tag, 100.0*err);
		}
	}
	return 0;
}
