#include "os.h"
#include "draw3.h"

void
idx2color(int idx, uchar *color)
{
	int i;
	int r, g, b;
idx += 9;
	r = 0;
	g = 0;
	b = 0;
	for(i = 7; i >= 0 ; i--){
		r |= (idx&1) << i; idx >>= 1;
		g |= (idx&1) << i; idx >>= 1;
		b |= (idx&1) << i; idx >>= 1;
	}
	color[0] = b;
	color[1] = g;
	color[2] = r;
	color[3] = 0xff;
}

