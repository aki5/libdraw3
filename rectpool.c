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
addcol(Rectrow *row, intcoord u0, intcoord w)
{
	Rectcol *col;
	int i;

	if(row->ncols == row->acols){
		row->acols += 8;
		row->cols = realloc(row->cols, row->acols * sizeof row->cols[0]);
	}
	for(i = row->ncols; i > 0; i--){
		if(row->cols[i-1].u0 < u0)
			break;
	}
	if(i < row->ncols)
		memmove(row->cols+i+1, row->cols+i, (row->ncols-i) * sizeof row->cols[0]);
	row->ncols++;
	col = row->cols + i;
	memset(col, 0, sizeof col[0]);
	col->u0 = u0;
	col->w = w;
}

static Rectrow *
addrow(Rectpool *rpool, intcoord v0, intcoord h)
{
	Rectrow *row;
	int i;

	if(rpool->nrows == rpool->arows){
		rpool->arows += 8;
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

static intcoord
getsegr(intcoord *segr, int nsegr, int val)
{
	int i;
	intcoord segrval;

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
rectpool_bestfit(Rectpool *rpool, int bestfit)
{
	rpool->bestfit = bestfit;
}

void
rectpool_setsegr(Rectpool *rpool, intcoord *segr, int nsegr)
{
	int i;
	nsegr = nsegr < rpool->asegr ? nsegr : rpool->asegr;
	for(i = 0; i < nsegr; i++)
		rpool->segr[i] = segr[i];
	rpool->nsegr = nsegr;
}

void
rectpool_harmonic(Rectpool *rpool, int max, int n)
{
	int i, j;
	intcoord tmp;
	j = 0;
	for(i = 0; i < n; i++){
		tmp = max/(n-i);
		rpool->segr[j] = tmp > 1 ? tmp : 1;
		if(j == 0 || rpool->segr[j-1] !=  rpool->segr[j])
			j++;
	}
	rpool->nsegr = j;
	for(i = 0; i < rpool->nsegr; i++){
		fprintf(stderr, "harmonic segr[%d] = %d\n", i, rpool->segr[i]);
	}
}


void
initrectpool(Rectpool *rpool, Rect r)
{
	int i;
	intcoord *segr, s, tmp;
	int sum;

	memset(rpool, 0, sizeof rpool[0]);
	segr = malloc(32 * sizeof segr[0]);
	s = 1;
	sum = 0;
	for(i = 0; i < 32; i++){
		sum += s;
		segr[i] = s;
		tmp = s*12/10;
		s = tmp > s ? tmp : s+1;
		if(sum >= recth(&r))
			break;
	}
	if(i > 0){
fprintf(stderr, "sum = %d, i = %d, s = %d\n", sum, i-1, segr[i-1]);
		rpool->segr = segr;
		rpool->asegr = 32;
		rpool->nsegr = i-1;
		rpool->bestfit = 3;
		rpool->r = r;
		addrow(rpool, r.v0, recth(&r));
	} else {
		free(segr);
	}
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

int
rectalloc(Rectpool *rpool, int w, int h, Rect *rp)
{
	Rectrow *row;
	Rectcol *col;
	intcoord wsegr, hsegr;
	int i, j;

	if(w <= 0 || h <= 0)
		return -1;

	if((rpool->bestfit & 2) == 0)
		wsegr = getsegr(rpool->segr, rpool->nsegr, w);
	else
		wsegr = w;

	hsegr = getsegr(rpool->segr, rpool->nsegr, h);

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
				return 0;
			}
		}
	}

	if((rpool->bestfit & 1) == 0)
		return -1;
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
				return 0;
			}
		}
	}

	return -1;
}


void
rectfree(Rectpool *rpool, Rect r)
{
	Rectrow *row;
	Rectcol *col;
	int segrw;
	int i, j;

	if((rpool->bestfit & 2) == 0)
		segrw = getsegr(rpool->segr, rpool->nsegr, rectw(&r));
	else
		segrw = rectw(&r);

	for(i = 0; i < rpool->nrows; i++){
		row = rpool->rows + i;
		if(row->v0 == r.v0){


			addcol(row, r.u0, segrw);

			for(j = 1; j < row->ncols; j++){
				/* try merge current col with the previous one */
				col = row->cols;
				if(col[j-1].u0+col[j-1].w == col[j].u0){
					col[j-1].w += col[j].w;
					if(j < row->ncols-1)
						memmove(col+j, col+j+1, (row->ncols-j-1) * sizeof col[0]);
					row->ncols--;
					j--; /* the horror */
				}
			}
		}

		/* try merge current row with the previous one */
		row = rpool->rows;
		if(i > 0 && rowunused(rpool, row+i) && rowunused(rpool, row+i-1)){
			if(row[i-1].v0+row[i-1].h == row[i].v0){
				row[i-1].h += row[i].h;
				if(i < rpool->nrows-1)
					memmove(row+i, row+i+1, (rpool->nrows-i-1) * sizeof row[0]);
				rpool->nrows--;
				i--; 
			}
		}
	}
}
