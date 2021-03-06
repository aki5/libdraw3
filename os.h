
#include <stdio.h> // for debugging, nothing more
#include <stdlib.h>
#include <string.h> // memcpy
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

typedef unsigned char uchar;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef uintptr_t uintptr;

typedef int s32int;
typedef long long s64int;

#define nelem(x) (int)(sizeof(x)/sizeof(x[0]))

static inline double
timenow(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + 1e-6*tv.tv_usec;
}

static inline int
iabs(int x)
{
	return x < 0 ? -x : x;
}

#define USED(x) ((void)x)

