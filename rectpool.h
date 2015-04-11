
typedef struct Rectpool Rectpool;
typedef struct Rectrow Rectrow;
typedef struct Rectcol Rectcol;

struct Rectcol {
	intcoord u0;
	intcoord w;
};

struct Rectrow {
	intcoord v0;
	intcoord h;
	Rectcol *cols;
	int ncols;
	int acols;
};

struct Rectpool {
	int bestfit;
	Rect r;
	intcoord *segr;
	int nsegr, asegr;
	Rectrow *rows;
	int nrows, arows;
};

void initrectpool(Rectpool *rpool, Rect r); //, intcoord *wsegr, intcoord *hsegr, int nsegr);
void freerectpool(Rectpool *rpool);
int rectalloc(Rectpool *rpool, int w, int h, Rect *rp);
void rectfree(Rectpool *rpool, Rect r);
void rectpool_nosweat(Rectpool *rpool, int nosweat);
void rectpool_setsegr(Rectpool *rpool, intcoord *segr, int nsegr);
void rectpool_harmonic(Rectpool *rpool, int max, int n);
