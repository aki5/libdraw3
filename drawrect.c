#include "os.h"
#include "draw3.h"

void
drawrect(Image *img, Rect r, uchar *color)
{
	int u, v;
	u32int *pix, cval;

	cliprect(&r, img->r);

	cval = *(u32int*)color;
	for(v = r.v0; v < r.vend; v++){
		for(u = r.u0; u < r.uend; u++){
			pix = (u32int*)(img->img + v*img->stride) + u;
			*pix = cval;
		}
	}
}
