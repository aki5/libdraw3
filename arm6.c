#include "os.h"
#include "draw3.h"

/*
 *	Use prefetches and ldm/stm on the raspberry, they really go a lot faster.
 *	The ginormous register pressure they create makes it hard to have just
 *	ldm/stm inline routines: gcc runs out of registers and generates tons of
 *	spills.
 *
 *	Not to blame gcc, it's a tough task for people too.
 */

void
pixcpy_src8(uchar *dst, uchar *src, int nsrc)
{
	int i;

	/*
	 *	ldm/stm needs word alignment, so do that for now.
	 *	would be nice to try and cache align output.. but oh well
	 */
	for(i = 0; i < nsrc && ((uintptr)src & 3) != 0; i++){
		u32int a;
		a = *src;
		a |= a<<8;
		a |= a<<16;
		*(u32int*)dst = a;
		dst += 4;
		src += 1;
	}

	nsrc -= 7;
	for(; i < nsrc; i += 8){
		asm(
			"pld [%[src], #64];"

			"ldm %[src], {r7, r12};"

			"and r4, r7, #0xff;"
			"and r5, r7, #0xff00;"
			"and r6, r7, #0xff0000;"
			"and r7, r7, #0xff000000;"
			"and r8, r12, #0xff;"
			"and r9, r12, #0xff00;"
			"and r10, r12, #0xff0000;"
			"and r12, r12, #0xff000000;"

			"orr r4, r4, lsl #8;"
			"orr r5, r5, lsl #8;"
			"orr r6, r6, lsl #8;"
			"orr r7, r7, lsl #8;"
			"orr r8, r8, lsl #8;"
			"orr r9, r9, lsl #8;"
			"orr r10, r10, lsl #8;"
			"orr r12, r12, lsl #8;"

			"orr r4, r4, lsl #16;"
			"orr r5, r5, lsl #16;"
			"orr r6, r6, lsl #16;"
			"orr r7, r7, lsl #16;"
			"orr r8, r8, lsl #16;"
			"orr r9, r9, lsl #16;"
			"orr r10, r10, lsl #16;"
			"orr r12, r12, lsl #16;"

			"stm %[dst], {r4, r5, r6, r7, r8, r9, r10, r12};"
			:
			: [dst]"r"(dst), [src]"r"(src)
			: "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r12"
		);
		src += 8;
		dst += 32;
	}
	nsrc += 7;
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
	asm(
		"pld [%[src], #0];"
		"pld [%[src], #32];"
		:
		: [src]"r"(src)
		:
	);
	nbytes -= 31;
	for(i = 0; i < nbytes; i += 32){
		asm(
			"pld [%[src], #64];"

			"ldm %[src], {r4, r5, r6, r7, r8, r9, r10, r12};"

			/* r3 = pixels a, b */
			"and r14, r4, #0xf8;"
			"mov r3, r14, lsr #3;"
			"and r14, r4, #0xfc00;"
			"orr r3, r14, lsr #5;"
			"and r14, r4, #0xf80000;"
			"orr r3, r14, lsr #8;"

			"and r14, r5, #0xf8;"
			"orr r3, r14, lsl #13;"
			"and r14, r5, #0xfc00;"
			"orr r3, r14, lsl #11;"
			"and r14, r5, #0xf80000;"
			"orr r3, r14, lsl #8;"

			/* r4 = pixels c, d */
			"and r14, r6, #0xf8;"
			"mov r4, r14, lsr #3;"
			"and r14, r6, #0xfc00;"
			"orr r4, r14, lsr #5;"
			"and r14, r6, #0xf80000;"
			"orr r4, r14, lsr #8;"

			"and r14, r7, #0xf8;"
			"orr r4, r14, lsl #13;"
			"and r14, r7, #0xfc00;"
			"orr r4, r14, lsl #11;"
			"and r14, r7, #0xf80000;"
			"orr r4, r14, lsl #8;"

			/* r5 = pixels e, f */
			"and r14, r8, #0xf8;"
			"mov r5, r14, lsr #3;"
			"and r14, r8, #0xfc00;"
			"orr r5, r14, lsr #5;"
			"and r14, r8, #0xf80000;"
			"orr r5, r14, lsr #8;"

			"and r14, r9, #0xf8;"
			"orr r5, r14, lsl #13;"
			"and r14, r9, #0xfc00;"
			"orr r5, r14, lsl #11;"
			"and r14, r9, #0xf80000;"
			"orr r5, r14, lsl #8;"

			/* r6 = pixels g, h */
			"and r14, r10, #0xf8;"
			"mov r6, r14, lsr #3;"
			"and r14, r10, #0xfc00;"
			"orr r6, r14, lsr #5;"
			"and r14, r10, #0xf80000;"
			"orr r6, r14, lsr #8;"

			"and r14, r12, #0xf8;"
			"orr r6, r14, lsl #13;"
			"and r14, r12, #0xfc00;"
			"orr r6, r14, lsl #11;"
			"and r14, r12, #0xf80000;"
			"orr r6, r14, lsl #8;"

			"stm %[dst], {r3, r4, r5, r6};"
			:
			: [dst]"r"(dst), [src]"r"(src)
			: "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r12", "r14"
		);
		src += 32;
		dst += 16;
	}
	nbytes += 31;
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
	register u32int
		a asm("r4"),
		b asm("r5"),
		c asm("r6"),
		d asm("r7"),
		e asm("r8"),
		f asm("r9"),
		g asm("r10"),
		h asm("r12");
	register int
		i asm("r14");

	a = *(u32int*)val;

	/* align to cache-line */
	for(i = 0; i < nbytes && ((uintptr)dst & 31) != 0; i += 4){
		*(u32int*)dst = a;
		dst += 4;
	}

	/* do full cache lines */
	nbytes -= 31;
	if(nbytes > 0){
		b = c = d = e = f = g = h = a;
		for(; i < nbytes; i += 32){
			asm(
				"stm %[dst], {%[a], %[b], %[c], %[d], %[e], %[f], %[g], %[h]};"
				:
				:[dst]"r"(dst),
				 [a]"r"(a), [b]"r"(b), [c]"r"(c), [d]"r"(d),
				 [e]"r"(e), [f]"r"(f), [g]"r"(g), [h]"r"(h)
			);
			dst += 32;
		}
	}
	nbytes += 31;

	/* the remainder */
	for(; i < nbytes; i += 4){
		*(u32int*)dst = a;
		dst += 4;
	}
}

