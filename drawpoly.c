#include "os.h"
#include "draw3.h"
#include <math.h>

static inline int
polyinside(short *pt, int *poly, int *next, int i, int n, short *a, short *b, short *c)
{
	short *p;
	int j;
	for(j = 0; j < n; j++){
		p = pt + 2*poly[i];
		if(ori2i(a, b, p) >= 0 && ori2i(b, c, p) >= 0 && ori2i(c, a, p) >= 0)
			return 1;
		i = next[i];
	}
	return 0;
}

static uchar red[4] = { 0x80, 0x30, 0xff, 0xff };
static uchar grey[4] = { 0x50, 0x50, 0x50, 0xff };

static uchar red2[4] = { 0x00, 0x00, 0xff, 0xff };
static uchar green2[4] = { 0x00, 0xff, 0x00, 0xff };
static uchar blue2[4] = { 0xff, 0x00, 0x00, 0xff };

static uchar *colors[] = {
		red2,
		green2,
		blue2
};

void
drawline(uchar *img, int width, int height, short *a, short *b, uchar *color)
{
	short c[2], d[2];
	short perp[2];
	int len;
	perp[0] = (b[1]-a[1]);
	perp[1] = -(b[0]-a[0]);
	len = lrint(sqrt(perp[0]*perp[0] + perp[1]*perp[1]));
	perp[0] = (2*perp[0] + len/2)/len;
	perp[1] = (2*perp[1] + len/2)/len;
	c[0] = b[0] + perp[0];
	c[1] = b[1] + perp[1];
	d[0] = a[0] + perp[0];
	d[1] = a[1] + perp[1];
	drawtri(img, width, height, a, b, c, color);
	drawtri(img, width, height, a, c, d, color);
}

/* awk 'BEGIN{N=5; for(i = 0; i < N; i++) printf "%d,%d,\n", 5*-sin(6.2831853*i/N), 5*cos(6.2831853*i/N)}'*/
static short blop[] = {
0,5,
-4,1,
-2,-4,
2,-4,
4,1,
};

void
idx2color(int idx, uchar *color)
{
	int i;
	int r, g, b;
idx += 1;
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

void
drawblop(uchar *img, int width, int height, short *p, uchar *color)
{
	short a[2], b[2];
	int i;
	for(i = 0; i < (int)nelem(blop)/2-1; i++){
		a[0] = p[0] + blop[2*i+0];
		a[1] = p[1] + blop[2*i+1];
		b[0] = p[0] + blop[2*(i+1)+0];
		b[1] = p[1] + blop[2*(i+1)+1];
		drawtri(img, width, height, p, b, a, color);
	}
	a[0] = p[0] + blop[0];
	a[1] = p[1] + blop[1];
	drawtri(img, width, height, p, a, b, color);
}


long
polyarea(short *pt, int *poly, int npoly)
{
	short *a, *b, *c;
	int i;
	long area;
	area = 0;
	a = pt + 2*poly[0];
	for(i = 1; i < npoly-1; i++){
		b = pt + 2*poly[i];
		c = pt + 2*poly[i+1];
		area += ori2i(b, c, a);
	}
	return area;
}

void
showcolors(uchar *img, int width, int height)
{
	short a[2], b[2], c[2], d[2];
	uchar color[4];
	int dim = 10;
	int i, x, y;
	for(i = 0; i < 128; i++){
		x = i % (width/dim);
		y = i / (width/dim);
		a[0] = x*dim;
		a[1] = y*dim;
		b[0] = x*dim;
		b[1] = (y+1)*dim;
		c[0] = (x+1)*dim;
		c[1] = (y+1)*dim;
		d[0] = (x+1)*dim;
		d[1] = y*dim;
		idx2color(i, color);
		drawtri(img, width, height, a, b, c, color);
		drawtri(img, width, height, a, c, d, color);
	}
}

void
debugpoly(uchar *img, int width, int height, short *pt, int *poly, int npoly)
{
	int i;
	short *a, *b = NULL;
	for(i = 0; i < npoly-1; i++){
		a = pt + 2*poly[i];
		b = pt + 2*poly[i+1];
		drawline(img, width, height, a, b, red);
		drawblop(img, width, height, a, colors[i%3]);
	}
	if(npoly > 1){
		a = pt + 2*poly[0];
		drawline(img, width, height, b, a, red);
		drawblop(img, width, height, b, colors[i%3]);
	}
}

int
drawpoly(uchar *img, int width, int height, short *pt, int *poly, int npoly, uchar *color)
{
	int i, ni, nni;
	int nconc;
	short *a, *b, *c;
	int next[npoly];
	int inpoly = npoly;
	int bugger = 0;
	int ci;
	uchar col[4];
	memcpy(col, color, 4);

retry:
	ci = 0;
	for(i = 0; i < npoly-1; i++)
		next[i] = i+1;
	next[i] = 0;

	nconc = 0;
	while(npoly > 2){
		ni = next[i];
		nni = next[ni];
		a = pt + 2*poly[i];
		b = pt + 2*poly[ni];
		c = pt + 2*poly[nni];
		if(ori2i(a, b, c) > 0 && polyinside(pt, poly, next, next[nni], npoly-3, a, b, c) == 0){
			if(0 || bugger){
				fprintf(stderr, "halting\n");
				drawhalt();
				fprintf(stderr, "halt done\n");
				idx2color(ci, col);
				ci++;
			}
			drawtri(img, width, height, a, b, c, col);
			next[i] = nni;
			npoly--;
			nconc = 0;
		} else {
			i = ni;
			nconc++;
		}
		if(nconc > 2*npoly){
			if(!bugger){
				static int dumpseq;
				FILE *fp;
				char fname[32];
				snprintf(fname, sizeof fname, "poly%03d.txt", dumpseq++);
				fp = fopen(fname, "wb");
				for(i = 0; i < inpoly; i++)
					fprintf(fp, "%d %d\n", pt[2*poly[i]], pt[2*poly[i]+1]);
				fclose(fp);
				debugpoly(img, width, height, pt, poly, inpoly);
				npoly = inpoly;
				bugger = 1;
				color = grey;
				showcolors(img, width, height);
				goto retry;
			}
			fprintf(stderr, "bugger all!, area %ld, npolys %d, inpolys %d\n", polyarea(pt, poly, inpoly), npoly, inpoly);
			return -1;
		}
	}

	return 0;
}
