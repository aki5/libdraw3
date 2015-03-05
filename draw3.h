
enum {
	Ssubpix = 0,
};	

extern int width;
extern int height;
extern int stride;
extern uchar *framebuffer;

int drawinit(int w, int h);
void drawflush(void);
int drawhandle(int fd, int doblock);
int drawfd(void);
void drawreq(void);
int drawbusy(void);
int drawhalt(void);

void drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color);
int drawpoly(uchar *img, int width, int height, short *pt, int *poly, int npoly, uchar *color);
void drawtris(uchar *img, int width, int height, short *tris, uchar *colors, int ntris);

void drawline(uchar *img, int width, int height, short *a, short *b, uchar *color);
void idx2color(int idx, uchar *color);

static inline int
signumi(int x)
{
	return (x>>31)|((unsigned)-x>>31);
//	return (x>0)-(x<0); 
}

static inline int
mini(int a, int b)
{
	return a < b ? a : b;
}

static inline int
maxi(int a, int b)
{
	return a > b ? a : b;
}

static inline int
det2i(
	int a, int b,
	int c, int d
){
	return a*d - b*c;
}

static inline int
ori2i(short *pa, short *pb, short *pc)
{
	short a, b, c, d;
	a = pb[0]-pc[0];
	b = pb[1]-pc[1];
	c = pa[0]-pc[0];
	d = pa[1]-pc[1];
	return det2i(a, b, c, d);
}

static inline int
ori2i_dx(short *pa, short *pb)
{
	return (pb[1] - pa[1]);
}

static inline int
ori2i_dy(short *pa, short *pb)
{
	return -(pb[0] - pa[0]);
}

static inline int
ptonseg_col(short *a, short *b, short *p)
{
	return
		p[0] <= maxi(a[0], b[0]) &&
		p[0] >= mini(a[0], b[0]) &&
		p[1] <= maxi(a[1], b[1]) &&
		p[1] >= mini(a[1], b[1]);
}

static inline int
isect2i(short *a, short *b, short *c, short *d)
{
	int abc, abd;
	int cda, cdb;

	abc = signumi(ori2i(a, b, c));
	abd = signumi(ori2i(a, b, d));
	cda = signumi(ori2i(c, d, a));
	cdb = signumi(ori2i(c, d, b));
	if(abc == -abd && cda == -cdb)
		return 1;
	if(abc == 0 && ptonseg_col(a, b, c))
		return 1;
	if(abd == 0 && ptonseg_col(a, b, d))
		return 1;
	if(cda == 0 && ptonseg_col(c, d, a))
		return 1;
	if(cdb == 0 && ptonseg_col(c, d, b))
		return 1;
	return 0;
}

static inline int
polysegisect(short *pt, int *poly, int npoly, int a, int b)
{
	short *pa, *pb;
	short *pc, *pd;
	int j;
	pa = pt + 2*a;
	pb = pt + 2*b;
	for(j = 0; j < npoly; j++){
		int nj = (j+1)%npoly;
		pc = pt + 2*poly[j];
		pd = pt + 2*poly[nj];
		if(a != poly[j] && b != poly[nj] &&a != poly[nj] && b != poly[j] && isect2i(pa, pb, pc, pd))
			return 1;
	}
	return 0;
}
