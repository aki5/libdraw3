#include "os.h"
/*
 *	this comes from hacker's delight, with the 2014 bugfix included
 *
 *	adjusted typography a bit, got rid of the return struct.
 *	algorithm is untouched
 */
void
magicu(u32int d, u32int *mulp, int *addp, int *shiftp)
{                          // Must have 1 <= d <= 2**32-1.
	int p, gt = 0;
	u32int nc, delta, q1, r1, q2, r2;

	*addp = 0;             // Initialize "add" indicator.
	nc = -1 - (-d)%d;       // u32int arithmetic here.
	p = 31;                 // Init. p.
	q1 = 0x80000000/nc;     // Init. q1 = 2**p/nc.
	r1 = 0x80000000 - q1*nc;// Init. r1 = rem(2**p, nc).
	q2 = 0x7FFFFFFF/d;      // Init. q2 = (2**p - 1)/d.
	r2 = 0x7FFFFFFF - q2*d; // Init. r2 = rem(2**p - 1, d).
	do {
		p = p + 1;
		if (q1 >= 0x80000000)
			gt = 1;  // Means q1 > delta.
		if (r1 >= nc - r1) {
			q1 = 2*q1 + 1;            // Update q1.
			r1 = 2*r1 - nc;
		} else {
			q1 = 2*q1;
			r1 = 2*r1;
		}
		if (r2 + 1 >= d - r2) {
			if (q2 >= 0x7FFFFFFF)
				*addp = 1;
			q2 = 2*q2 + 1;            // Update q2.
			r2 = 2*r2 + 1 - d;
		} else {
			if (q2 >= 0x80000000)
				*addp = 1;
			q2 = 2*q2;
			r2 = 2*r2 + 1;
		}
		delta = d - 1 - r2;
	} while (gt == 0 && (q1 < delta || (q1 == delta && r1 == 0)));

	*mulp = q2 + 1;
	*shiftp = p - 32;
}
