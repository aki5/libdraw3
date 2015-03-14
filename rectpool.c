#include "os.h"
#include "draw3.h"
#include "rectpool.h"

static int
rowunused(Rectpool *rpool, Rectrow *row)
{
	return	row->ncols == 1 &&
			row->cols[0].u0 == rpool->r.u0 &&
			row->cols[0].u0+row->cols[0].w == rpool->r.uend;
}

static void
addcol(Rectrow *row, short u0, short w)
{
	Rectcol *col;
	int i;

	if(row->ncols == row->acols){
		row->acols += 32;
		row->cols = realloc(row->cols, row->acols * sizeof row->cols[0]);
	}
	for(i = row->ncols; i > 0; i--){
		if(row->cols[i-1].u0 < u0)
			break;
		fprintf(stderr, "cols[%d]:%d >= u0:%d\n", i-1, row->cols[i-1].u0, u0);
	}
fprintf(stderr, "i landed on cols[%d]:%d, u0:%d\n", i, row->cols[i].u0, u0);
	if(i < row->ncols)
		memmove(row->cols+i+1, row->cols+i, (row->ncols-i) * sizeof row->cols[0]);
	row->ncols++;
	col = row->cols + i;
	memset(col, 0, sizeof col[0]);
	col->u0 = u0;
	col->w = w;
}

static Rectrow *
addrow(Rectpool *rpool, short v0, short h)
{
	Rectrow *row;
	int i;

	if(rpool->nrows == rpool->arows){
		rpool->arows += 32;
		rpool->rows = realloc(rpool->rows, rpool->arows * sizeof rpool->rows[0]);
	}

	for(i = rpool->nrows; i > 0; i--){
		if(rpool->rows[i-1].v0 < v0)
			break;
	}
	if(i < rpool->nrows)
		memmove(rpool->rows+i+1, rpool->rows+i, (rpool->nrows-i) * sizeof rpool->rows[0]);
	rpool->nrows++;
	row = rpool->rows + i;
	memset(row, 0, sizeof row[0]);
	row->v0 = v0;
	row->h = h;
	addcol(row, rpool->r.u0, rpool->r.uend - rpool->r.u0);
	return row;
}

static short
getsegr(short *segr, int nsegr, int val)
{
	int i;
	short segrval;

	segrval = -1;
	for(i = 0; i < nsegr; i++){
		if(segr[i] >= val){
			segrval = segr[i];
			break;
		}
	}
	return segrval == -1 ? val : segrval;
}

void
initrectpool(Rectpool *rpool, Rect r) //, short *wsegr, short *hsegr, int nsegr)
{
	int i;
	Rectrow *row;
	short *segr, s;

	memset(rpool, 0, sizeof rpool[0]);
	segr = malloc(30 * sizeof segr[0]);
	s = 6;
	for(i = 0; i < 30; i++){
fprintf(stderr, "segr[%d]=%d\n", i, s);
		segr[i] = s;
		s = s*13/10;
		if(s >= recth(&r)/2)
			break;
	}
	rpool->wsegr = segr;
	rpool->hsegr = segr;
	rpool->nsegr = i;

	rpool->r = r;
	row = addrow(rpool, r.v0, recth(&r));
}

void
freerectpool(Rectpool *rpool)
{
	int i;
	for(i = 0; i < rpool->nrows; i++)
		if(rpool->rows[i].acols > 0)
			free(rpool->rows[i].cols);
	free(rpool->rows);
	memset(rpool, 0, sizeof rpool[0]);
}

void
dumprows(Rectpool *rpool)
{
	Rectrow *row;
	Rectcol *col;
	int i, j;
	for(i = 0; i < rpool->nrows; i++){
		row = rpool->rows + i;
fprintf(stderr, "row[%d]=%d,%d", i, row->v0, row->v0+row->h);
		col = row->cols;
		for(j = 0; j < row->ncols; j++){
fprintf(stderr, " col[%d]=%d,%d", j, col[j].u0, col[j].u0+col[j].w);
		}
fprintf(stderr, "\n");
	}
}

int
rectalloc(Rectpool *rpool, int w, int h, Rect *rp)
{
	Rectrow *row;
	Rectcol *col;
	short wsegr, hsegr;
	int i, j;

	if(w <= 0 || h <= 0)
		return -1;

	wsegr = getsegr(rpool->wsegr, rpool->nsegr, w);
	hsegr = getsegr(rpool->hsegr, rpool->nsegr, h);

	/* try exact height */
	for(i = 0; i < rpool->nrows; i++){
		row = rpool->rows + i;
		if(row->h == hsegr){
			for(j = 0; j < row->ncols; j++){
				col = row->cols + j;
				if(col->w >= wsegr){
					/* found one, crack it if we can. */
					*rp = rect(col->u0, row->v0, col->u0+w, row->v0+h);
					col->u0 += wsegr;
					col->w -= wsegr;
					if(col->w == 0){
						memmove(row->cols+j, row->cols+j+1, (row->ncols-j-1) * sizeof row->cols[0]);
						row->ncols--;
					}
					dumprows(rpool);
					return 0;
				}
			}
		}
	}
	/* no exact height, split a new one off of a bigger but empty one */
	for(i = 0; i < rpool->nrows; i++){
		row = rpool->rows + i;

		if(row->h > hsegr && rowunused(rpool, row)){
			addrow(rpool, row->v0+hsegr, row->h-hsegr);

			row = rpool->rows + i;
			row->h = hsegr;

			col = row->cols;
			if(col->w >= wsegr){
				*rp = rect(col->u0, row->v0, col->u0+w, row->v0+h);
				col->u0 += wsegr;
				col->w -= wsegr;
				if(col->w == 0){
					memmove(row->cols, row->cols+1, (row->ncols-1) * sizeof row->cols[0]);
					row->ncols--;
				}
				dumprows(rpool);
				return 0;
			}
		}
	}
	fprintf(stderr, "rectalloc: desperation for %d,%d\n", wsegr, hsegr);
	/* final resort: best fit */
	int minh = -1, minhi = -1;
	for(i = 0; i < rpool->nrows; i++){
		row = rpool->rows + i;
		for(j = 0; j < row->ncols; j++){
			if(row->cols[j].w >= wsegr){
				if(row->h >= hsegr){
					if(minh < row->h){
						minh = row->h;
						minhi = i;
					}
				}
			}
		}
	}
	if(minhi != -1){
		row = rpool->rows + minhi;
		for(j = 0; j < row->ncols; j++){
			col = row->cols + j;
			if(col->w >= wsegr){
				*rp = rect(col->u0, row->v0, col->u0+w, row->v0+h);
				col->u0 += wsegr;
				col->w -= wsegr;
				if(col->w == 0){
					memmove(row->cols+j, row->cols+j+1, (row->ncols-j-1) * sizeof row->cols[0]);
					row->ncols--;
				}
				dumprows(rpool);
				return 0;
			}
		}
	}
	dumprows(rpool);

	/* no luck */
	return -1;
}


void
rectfree(Rectpool *rpool, Rect r)
{
	Rectrow *row;
	Rectcol *col;
	int segrw;
	int i, j;

	segrw = getsegr(rpool->wsegr, rpool->nsegr, rectw(&r));

fprintf(stderr, "rectfree row %d, %d,%d\n", r.v0, rectw(&r), recth(&r));

	for(i = 0; i < rpool->nrows; i++){
		row = rpool->rows + i;
		if(row->v0 == r.v0){
			int merged;
			merged = 0;

			addcol(row, r.u0, segrw);

			for(j = 1; j < row->ncols; j++){
				/* try merge current col with the previous one */
				col = row->cols;
				if(col[j-1].u0+col[j-1].w == col[j].u0){
fprintf(stderr, "col merge %d with %d\n", j-1, j);
					col[j-1].w += col[j].w;
					if(j < row->ncols-1)
						memmove(col+j, col+j+1, (row->ncols-j-1) * sizeof col[0]);
					row->ncols--;
					j--; /* the horror */
				}
			}
		}
		for(j = 0; j < row->ncols; j++){
			col = row->cols;
			if(col[j].w == 0){
				fprintf(stderr, "############ row[%d] col[%d] became 0!\n", i, j);
dumprows(rpool);
				abort();
			}
		}


		/* try merge current row with the previous one */
		row = rpool->rows;
		if(i > 0 && rowunused(rpool, row+i) && rowunused(rpool, row+i-1)){
			if(row[i-1].v0+row[i-1].h == row[i].v0){
fprintf(stderr, "row merge %d with %d\n", i-1, i);
				row[i-1].h += row[i].h;
				if(i < rpool->nrows-1)
					memmove(row+i, row+i+1, (rpool->nrows-i-1) * sizeof row[0]);
				rpool->nrows--;
				i--; 
			}
		}

	}
dumprows(rpool);

}
