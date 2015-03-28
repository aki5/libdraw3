#include "os.h"
#include "dmacopy.h"

int
main(void)
{
	double st, et;
	Dmacopy cpy;
	uchar *a, *b;
	int i, align = 4096, size = 16*1024*1024;

	st = timenow();
	a = malloc(size+align);
	b = malloc(size+align);

	a += align - ((uintptr)a & (align-1));
	b += align - ((uintptr)b & (align-1));

	for(i = 0; i < size; i++){
		a[i] = i/4096;
		b[i] = i/4096+1;
	}
	et = timenow();
	fprintf(stderr, "alloc and set values: %f sec\n", et-st);

	st = timenow();
	if(initdmacopy(&cpy, a, b, size) == -1){
		fprintf(stderr, "initdmacopy fail\n");
		exit(1);
	}
	et = timenow();
	fprintf(stderr, "initdmacopy: %f sec\n", et-st);

	//dumpdmacopy(&cpy);
	for(i = 0; ; i++){
		st = timenow();
		startdmacopy(&cpy);
		while(!dmacopydone(&cpy))
			usleep(10000);
		et = timenow();
		fprintf(stderr, "round %d: %f sec, %f MB/s\n", i, et-st, (1e-6*size)/(et-st));
	}

	st = timenow();
	freedmacopy(&cpy);
	et = timenow();
	fprintf(stderr, "freedmacopy: %f sec\n", et-st);

	st = timenow();
	for(i = 0; i < size; i++)
		if(a[i] != b[i])
			fprintf(stderr, "bugger %d\n", i);
	et = timenow();
	fprintf(stderr, "checking results: %f sec\n", et-st);
	fprintf(stderr, "done\n");
	return 0;
}
