
typedef struct Rectpool Rectpool;
typedef struct Rectrow Rectrow;
typedef struct Rectcol Rectcol;

struct Rectcol {
	short u0;
	short w;
};

struct Rectrow {
	short v0;
	short h;
	Rectcol *cols;
	int ncols;
	int acols;
};

struct Rectpool {
	Rect r;
	short *wsegr;
	short *hsegr;
	int nsegr;
	Rectrow *rows;
	int nrows, arows;
};

void initrectpool(Rectpool *rpool, Rect r); //, short *wsegr, short *hsegr, int nsegr);
void freerectpool(Rectpool *rpool);
int rectalloc(Rectpool *rpool, int w, int h, Rect *rp);
void rectfree(Rectpool *rpool, Rect r);