#include "os.h"
#include "dmacopy.h"
#include <sys/mman.h>

/*
 *	No verbose commentary here, check out
 *	http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 */

enum {
	CTRL_CS = 0,
	CTRL_CONBLK_AD = 1,
	CTRL_DEBUG = 8,

	CTRL_DISDEBUG = 1<<29,
	CTRL_WAIT_FOR_OUTSTANDING_WRITES = 1<<28,
	CTRL_INT = 1<<2,
	CTRL_END = 1<<1,
	CTRL_ACTIVE = 1<<0,
};

enum {
	DESC_BURST_SHIFT = 12, /* [0..15], transfer burst length */
	DESC_SRC_128BIT = 1<<9,
	DESC_SRC_INC = 1<<8,
	DESC_DST_128BIT = 1<<5,
	DESC_DST_INC = 1<<4,
};

static int
physmap(uchar *ptr, int size, uintptr *offp, uintptr **paddr)
{
	u64int pginfo;
	uintptr i, va, vend;
	int fd, npg;


	if((fd = open("/proc/self/pagemap", O_RDONLY)) == -1)
		return -1;

	va = (uintptr)ptr;
	vend = (va + size + 4095) & ~4095;
	if(offp != NULL)
		*offp = va & 4095;
	va &= ~4095;

	npg = 0;
	for(i = va; i < vend; i += 4096){
		*(volatile uchar *)i;
		npg++;
	}
	if(mlock((void*)va, 4096*npg) == -1)
		return -1;

	if(paddr != NULL){
		*paddr = malloc(npg * sizeof paddr[0][0]);
		memset(*paddr, 0, npg * sizeof paddr[0][0]);
	}
	npg = 0;
	for(i = va; i < vend; i += 4096){
		if(pread(fd, &pginfo, 8, 8*(i/4096)) != 8){
			close(fd);
			return -1;
		}
		if(paddr != NULL){
			/* doc says 55 bits for pfn, kernel whines something about changing the upper bits */
			(*paddr)[npg] = pginfo & 0x7fffffffffffffull;
		}
		npg++;
	}
	close(fd);

	return npg;
}

static u32int *
dmactrl(void)
{
	u32int *ctrl;
	int fd;

	if((fd = open("/dev/mem", O_RDWR)) == -1)
		return NULL;

	ctrl = mmap(NULL, 16*256, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x20007000);
	if(ctrl == MAP_FAILED)
		return NULL;

	return ctrl;	
}

int
initdmacopy(Dmacopy *cpy, uchar *dst, uchar *src, int len)
{
	int npg, ndesc;
	int i;

	memset(cpy, 0, sizeof cpy[0]);
	cpy->dst = dst;
	cpy->src = src;
	cpy->len = len;

	if((cpy->ctrl = dmactrl()) == NULL)
		return -1;

	cpy->chan = cpy->ctrl + (4*0x100)/4;

	cpy->chan[CTRL_CS] = (1<<31); /* reset the thing, typically not necessary. */
	__sync_synchronize();
	cpy->chan[CTRL_DEBUG] = (1<<2)|(1<<1)|(1<<0); /* clear some errors, not that we check them :) */
	__sync_synchronize();

	if((cpy->ndstpg = physmap(dst, len, &cpy->dstoff, &cpy->dstpfn)) == -1)
		return -1;

	if((cpy->nsrcpg = physmap(src, len, &cpy->srcoff, &cpy->srcpfn)) == -1)
		return -1;

	ndesc = cpy->ndstpg + cpy->nsrcpg;
	cpy->desc = mmap(NULL, 32*ndesc, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(cpy->desc == MAP_FAILED)
		return -1;

	if((cpy->ndescpg = physmap((uchar*)cpy->desc, 32*ndesc, NULL, &cpy->descpfn)) == -1){
		fprintf(stderr, "physmap descs: %s\n", strerror(errno));
		return -1;
	}

	u32int *desc;
	uintptr srcoff, dstoff, descoff;
	int off, xferlen, desci;

	srcoff = cpy->srcoff;
	dstoff = cpy->dstoff;
	descoff = 0;

	off = 0;
	desci = 0;
	desc = NULL;
	while(off < len){
		uintptr tmp;
		xferlen = 4096 - (srcoff & 4095);
		tmp = 4096 - (dstoff & 4095);
		xferlen = xferlen < tmp ? xferlen : tmp;
		tmp = len-off;
		xferlen = xferlen < tmp ? xferlen : tmp;

		desc = cpy->desc + 8*desci;
		/*
		 *	A good burst size remains bit of a mystery. Going higher can give gains
		 *	in throughput (seen >500MB/s copy with 8), but it makes the system unstable.
		 *	Yet to see a lockup with <= 4.
		 */
		desc[0] = DESC_SRC_INC | DESC_DST_INC | DESC_SRC_128BIT | DESC_DST_128BIT | (4<<DESC_BURST_SHIFT);
		desc[1] = (cpy->srcpfn[srcoff>>12]<<12) + (srcoff&4095);
		desc[2] = (cpy->dstpfn[dstoff>>12]<<12) + (dstoff&4095);
		desc[3] = xferlen;
		desc[4] = 0;
		desc[5] = (cpy->descpfn[32*(desci+1)>>12]<<12) + (32*(desci+1)&4095);
		desc[6] = 0;
		desc[7] = 0;
		__sync_synchronize(); /* not really needed, the above is regular memory at this point */

		srcoff += xferlen;
		dstoff += xferlen;
		off += xferlen;
		desci++;
	}
	if(desc != NULL)
		desc[5] = 0; /* terminate chain */
	cpy->ndesc = desci;
}

void
freedmacopy(Dmacopy *cpy)
{
	munlock(cpy->dst, cpy->len);
	munlock(cpy->src, cpy->len);
	munlock(cpy->desc, 4096*cpy->ndescpg);
	munmap(cpy->desc, cpy->ndesc*32); // XXX: ndesc is no longer the same it was when we did the mmap
	munmap(cpy->ctrl, 16*256);
	free(cpy->dstpfn);
	free(cpy->srcpfn);
	free(cpy->descpfn);
}

void
dumpdmacopy(Dmacopy *cpy)
{
	u32int *desc;
	int i;
	for(i = 0; i < cpy->ndesc; i++){
		desc = cpy->desc + 8*i;
		fprintf(stderr,
			"desc[%2d]: 0x%x 0x%x 0x%x %4d %4d 0x%x 0x%x 0x%x\n", i,
			desc[0], desc[1], desc[2], desc[3], 
			desc[4], desc[5], desc[6], desc[7]
		);
	}
}

int
dmacopydone(Dmacopy *cpy)
{
	__sync_synchronize();
	return (cpy->chan[CTRL_CS] & 1) == 0;
}

void
startdmacopy(Dmacopy *cpy)
{
	int i;
	while(!dmacopydone(cpy)){
		fprintf(stderr, "startdmacopy: channel is in use!\n");
		usleep(100000);
	}
	cpy->chan[CTRL_CONBLK_AD] = cpy->descpfn[0]<<12;
	__sync_synchronize();
	cpy->chan[CTRL_CS] =
		CTRL_ACTIVE | CTRL_END | CTRL_INT |
		CTRL_WAIT_FOR_OUTSTANDING_WRITES | CTRL_DISDEBUG
		;
	__sync_synchronize();
}
