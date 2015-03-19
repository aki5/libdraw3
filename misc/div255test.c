
#include "os.h"

int
div255(u32int m)
{
	//int tmp;
	//return (m*257+256)>>16;
/*
	m |= m << 16;
	m += 0x00010001;
	m += (m>>8)&0x00ff00ff;
	m >>= 8;
	return m & 0xff;
*/
	m = m << 8;

	m += 0x100;
	m += m >> 8;
	m >>= 8;
	return m >> 8;
}

int
main(void)
{
	int i, j;
	int err, maxerr, maxi;
	int nerr, ntot;

	nerr = 0;
	ntot = 0;
	maxerr = 0;
	maxi = -1;
	for(i = 0; i < 256; i++){
		for(j = 0; j < 256; j++){
			if((err = div255(i*j) - (i*j)/255) != 0){
				err = err >= 0 ? err : -err;
				if(err > maxerr){
					maxerr = err;
					maxi = i*j;
				}
				nerr++;
				printf("error %d/255 = %d, need %d\n", i*j, div255(i*j), (i*j)/255);
			}
			ntot++;
		}
	}
	printf("%d errors from %d total, maximum error: %d, %d/255 = %d, should be %d\n", nerr, ntot, maxerr, maxi, div255(maxi), maxi/255);

	return 0;
}

