
enum {
	MaxMouse = 8,
	Mouse0 = 1<<0,
	LastMouse = Mouse0<<MaxMouse,
	AnyMouse = (LastMouse<<1)-Mouse0,
	DAnyKey = ~AnyMouse, // X11 defines AnyKey, so we can not.

	Animate = 1<<9,
	KeyStr = 1<<10,

	KeyCapsLock = 1<<11,
	KeyAlt = 1<<12,
	KeyBackSpace = 1<<13,
	KeyBreak = 1<<14,
	KeyControl = 1<<15,
	KeyDel = 1<<16,
	KeyDown = 1<<17,
	KeyEnd = 1<<18,
	KeyEnter = 1<<19,
	KeyHome = 1<<20,
	KeyHyper = 1<<21,
	KeyIns = 1<<22,
	KeyLeft = 1<<23,
	KeyMeta = 1<<24,
	KeyPageDown = 1<<25,
	KeyPageUp = 1<<26,
	KeyRight = 1<<27,
	KeyShift = 1<<28,
	KeySuper = 1<<29,
	KeyTab = 1<<30,
	KeyUp = 1<<31,
};

typedef struct Rect Rect;
typedef struct Image Image;
typedef struct Input Input;

struct Rect {
	int u0;
	int v0;
	int uend;
	int vend;
};

struct Image {
	Rect r;
	int stride;
	int len;
	int dirty;
	uchar *img;
};

struct Input {
	int mouse;
	short xy[2];
	u64int begin;
	u64int on;
	u64int end;
	char str[5];
};

static inline int
ptinrect(short *uv, Rect *r)
{
	if(uv[0] >= r->u0 && uv[0] < r->uend && uv[1] >= r->v0 && uv[1] < r->vend)
		return 1;
	return 0;
}

/*
static inline Rect
offsetrect(Rect r, short *p)
{
	r.u0 += p[0];
	r.v0 += p[1];
	r.uend += p[0];
	r.vend += p[0];
}
*/

static inline int
ptinellipse(short *uv, short *c, short *d, int rad)
{
	short d1[2], d2[2];
	int mag1, mag2;

	d1[0] = uv[0] - c[0];
	d1[1] = uv[1] - c[1];
	d2[0] = uv[0] - d[0];
	d2[1] = uv[1] - d[1];

	mag1 = d1[0]*d1[0] + d1[1]*d1[1];
	mag2 = d2[0]*d2[0] + d2[1]*d2[1];
	return (mag1+mag2) <= 2*rad*rad;
}

static inline Rect
cliprect(Rect r, Rect cr)
{
	if(r.u0 < cr.u0)
		r.u0 = cr.u0;
	if(r.v0 < cr.v0)
		r.v0 = cr.v0;
	if(r.uend > cr.uend)
		r.uend = cr.uend;
	if(r.vend > cr.vend)
		r.vend = cr.vend;
	return r;
}

static inline int
rectisect(Rect a, Rect b)
{
	return a.u0 < b.uend && b.u0 < a.uend && a.v0 < b.vend && b.v0 < a.vend;
}

static inline int rectw(Rect *r){ return r->uend-r->u0; }
static inline int recth(Rect *r){ return r->vend-r->v0; }
static inline void rectmidpt(Rect *r, short *pt){ pt[0] = (r->uend+r->u0)/2; pt[1] = (r->vend+r->v0)/2; }

#define pt(a, b) (short[]){a, b}
#define rect(a, b, c, d) (Rect){.u0=a, .v0=b, .uend=c, .vend=d}
#define color(r, g, b, a) (uchar[]){b, g, r, a}

extern Input *inputs;
extern int ninputs;

extern Image screen;

static inline int
mousebegin(Input *inp)
{
	if((inp->begin & AnyMouse) != 0)
		return inp->mouse;
	return -1;
}

static inline int
mouseend(Input *inp)
{
	if((inp->end & AnyMouse) != 0)
		return inp->mouse;
	return -1;
}

static inline int
mousemove(Input *inp)
{
	return inp->mouse;
}

static inline int
keystr(Input *inp, char *str)
{
	return ((inp->begin & KeyStr) != 0 && !strcmp(inp->str, str));
}

static inline int
keypress(Input *inp, int mask)
{
	return (inp->begin & mask) != 0;
}

static inline int
keyend(Input *inp, int mask)
{
	return (inp->end & mask) != 0;
}

static inline int
animate(Input *inp)
{
	return (inp->begin & Animate) != 0;
}


int drawinit(int w, int h);
Input *drawevents(Input **inepp);
Input *drawevents_nonblock(Input **inepp);

int drawfd(void);
void drawreq(void);
int drawbusy(void);
int drawhalt(void);

//void drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color, int subpix);
void drawtri(Image *img, Rect r, short *a, short *b, short *c, uchar *color);
int drawpoly(uchar *img, int width, int height, short *pt, int *poly, int npoly, uchar *color, int subpix);
void drawtris(uchar *img, int width, int height, short *tris, uchar *colors, int ntris, int subpix);
void drawrect(Image *img, Rect r, uchar *color);
void drawblend(Image *dst, Rect r, Image *src, Image *mask);

void drawanimate(int flag);
void loadimage8(Image *img, Rect clipr, uchar *data, int stride);
void loadimage24(Image *img, Rect clipr, uchar *data, int stride);

void initdrawstr(char *path);
Rect drawstr(Image *img, Rect r, char *str, int len);
void setfontsize(int size);

static inline void
drawpixel(Image *img, short *pt, uchar *color)
{
	drawrect(img, rect(pt[0],pt[1],pt[0]+1,pt[1]+1), color);
}

void drawline(uchar *img, int width, int height, short *a, short *b, uchar *color);
void idx2color(int idx, uchar *color);
Input *getinputs(Input **ep);

static inline int
signumi(int x)
{
	return (x>>31)|((unsigned)-x>>31);
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
dot2i(short *a, short *b)
{
	return a[0]*b[0] + a[1]*b[1];
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
nori2i(short *pa, short *pb, short *pc)
{
	short a, b, c, d;
	a = pb[0]-pa[0];
	b = pb[1]-pa[1];
	c = pc[0]-pa[0];
	d = pc[1]-pa[1];
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

static inline int
polyarea(short *pt, int *poly, int npoly, short *p)
{
	short *a, *b;
	int area;
	int i, ni;
	area = 0;
	for(i = 0; i < npoly; i++){
		ni = (i == npoly-1) ? 0 : i+1;
		a = pt + 2*poly[i];
		b = pt + 2*poly[ni];
		area += ori2i(a, b, p);
	}
	return area;
}

static inline int
ptarea(short *pt, int npt, short *p)
{
	short *a, *b;
	int area;
	int i, ni;
	area = 0;
	for(i = 0; i < npt; i++){
		ni = (i == npt-1) ? 0 : i+1;
		a = pt + 2*i;
		b = pt + 2*ni;
		area += ori2i(a, b, p);
	}
	return area;
}
