
#include "os.h"
#include "draw3.h"
#include "sqpool.h"

static Sqcent *
sqcent(Sqnode *np)
{
	return np->cent;
}

static int
fib(int n)
{
	int i, a, b, t;
	a = 0; b = 1;
	for(i = 0; i < n; i++){
		t = a+b;
		a = b;
		b = t;
	}
	return b;
}

static int
rankfor(int n)
{
	int i, a, b, t;
	a = 0; b = 1;
	for(i = 0; b < n; i++){
		t = a+b;
		a = b;
		b = t;
	}
	return i;
}

int
sqempty(Sqpool *pool)
{
	Sqnode *np;
	int i, n;
	n = 0;
	for(i = 0; i < nelem(pool->free); i++)
		for(np = pool->free[i]; np != NULL; np = np->lnext)
			n++;
	return n == 1;
}

static Sqnode *
centloop0(Sqcent *cent)
{
	return cent->rank > 0 ? cent->nodes+1 : cent->nodes;
}

static Sqnode *
centloop1(Sqcent *cent)
{
	return cent->rank > 0 ? cent->nodes+1 + cent->rank : cent->nodes;
}

static Sqnode *
squnlink(Sqpool *pool, Sqnode *np)
{
	Sqnode *lnext, *lprev;
	lnext = np->lnext;
	lprev = np->lprev;
	if(lnext != NULL)
		lnext->lprev = lprev;
	if(lprev != NULL){
		lprev->lnext = lnext;
	} else {
		pool->free[np->rank] = lnext;
	}
	np->lnext = NULL;
	np->lprev = NULL;
	return np;
}

static Sqnode *
sqpopfree(Sqpool *pool, int rank)
{
	Sqnode *np, *lnext;
	if((np = pool->free[rank]) == NULL)
		return NULL;
	lnext = np->lnext;
	if(lnext != NULL)
		lnext->lprev = NULL;
	pool->free[rank] = lnext;
	return np;
}

static void
sqpushfree(Sqpool *pool, Sqnode *np)
{
	Sqnode *lnext;
	lnext = pool->free[np->rank];
	if(lnext != NULL)
		lnext->lprev = np;
	np->lnext = lnext;
	np->lprev = NULL;
	pool->free[np->rank] = np;
}

static Sqcent *
newcent(int rank, short u0, short v0)
{
	Sqcent *cent;
	Sqnode *np, *ep;
	int i;
	short u0topleft, v0topleft;
	short fibcur, fibprev;

	if(rank < 0)
		return NULL;

	cent = malloc(sizeof cent[0] + 2*(rank)*sizeof cent->nodes[0]);
	memset(cent, 0, sizeof cent[0] + 2*(rank)*sizeof cent->nodes[0]);
	cent->rank = rank;
	np = cent->nodes;
	np->rank = rank;
	np->cent = cent;
	np->u0 = u0;
	np->v0 = v0;

	if(rank == 0)
		return cent;

	ep = cent->nodes + 2*(rank)+1;
	for(np = cent->nodes; np < ep; np++){
		np->next = np+1;
		np->cent = cent;
	}

	np = cent->nodes+1 + (rank) - 1;
	np->next = cent->nodes;

	np = cent->nodes+1 + 2*(rank) - 1;
	np->next = cent->nodes;

	/*  trace the outer corners counter-clockwise. we start by going up (dir=3) */
	u0 = cent->nodes[0].u0 + fib(rank)-1;
	v0 = cent->nodes[0].v0 - fib(rank-1);
	u0topleft = -(fib(rank-1)-1);
	v0topleft = 0;
	ep = centloop0(cent)-1;
	for(np = centloop1(cent)-1, i = 0; np > ep; np--, i++){
		np->rank = rank-i-1;
		np->u0 = u0 + u0topleft;
		np->v0 = v0 + v0topleft;
		fibcur = fib(np->rank);
		fibprev = fib(np->rank-1);
		switch(i&3){
		case 0: /* left */
			u0 -= fibcur+fibprev-1;
			u0topleft = 0;
			v0topleft = 0;
			break; 
		case 1: /* down */
			v0 += fibcur+fibprev-1;
			u0topleft = 0;
			v0topleft = -(fibprev-1);
			break; 
		case 2: /* right */
			u0 += fibcur+fibprev-1;
			u0topleft = -(fibprev-1);
			v0topleft = -(fibprev-1);
			break; 
		case 3: /* up */
			v0 -= fibcur+fibprev-1;
			u0topleft = -(fibprev-1);
			v0topleft = 0;
			break; 
		}
	}

	/* trace the outer corners clockwise. start by going left (dir=3) */
	u0 = cent->nodes[0].u0 - fib(rank-1);
	v0 = cent->nodes[0].v0 + fib(rank)-1;
	u0topleft = 0;
	v0topleft = -(fib(rank-1)-1);
	ep = centloop1(cent)-1;
	for(np = centloop1(cent)+rank-1, i = 0; np > ep; np--, i++){
		np->rank = rank-i-1;
		np->u0 = u0 + u0topleft;
		np->v0 = v0 + v0topleft;
		fibcur = fib(np->rank);
		fibprev = fib(np->rank-1);
		switch(i&3){
		case 3: /* left */
			u0 -= fibcur+fibprev-1;
			u0topleft = 0;
			v0topleft = -(fibprev-1);
			break; 
		case 2: /* down */
			v0 += fibcur+fibprev-1;
			u0topleft = -(fibprev-1);
			v0topleft = -(fibprev-1);
			break; 
		case 1: /* right */
			u0 += fibcur+fibprev-1;
			u0topleft = -(fibprev-1);
			v0topleft = 0;
			break; 
		case 0: /* up */
			v0 -= fibcur+fibprev-1;
			u0topleft = 0; 
			v0topleft = 0;
			break; 
		}
	}
	return cent;
}

static Sqnode *
rootnode(int rank, short u0, short v0)
{
	Sqnode *np;
	np = malloc(sizeof np[0]);
	memset(np, 0, sizeof np[0]);
	np->rank = rank;
	np->u0 = u0;
	np->v0 = v0;
	return np;
}

static void
splitnode(Sqpool *pool, Sqnode *np)
{
	Sqcent *ncent;
	int rank;

	rank = np->rank;
	if(rank < 2)
		return;
	ncent = newcent(rank-1, np->u0 + fib(rank-2), np->v0 + fib(rank-2));
	ncent->nodes[0].next = np->next;
	ncent->topleft = np;
	np->next = ncent->nodes;
	np->rank -= 2;

	Sqnode *ep;
	ep = ncent->nodes+1 + 2*(rank-1);
	for(np = ncent->nodes; np < ep; np++)
		sqpushfree(pool, np);

	return;
}

static int
freecent(Sqpool *pool, Sqcent *cent)
{
	Sqnode *np, *ep;
	int i;

	/* there needs to be a cent */
	if(cent == NULL)
		return -1;

	/* and it needs to be complete and free */
	if(cent->nodes[0].rank != cent->rank || cent->nodes[0].used)
		return -1;

	/* and so does the parent, TODO: think the prev=NULL through */
	if(cent->topleft == NULL || (cent->topleft->rank != cent->rank-1 || cent->topleft->used))
		return -1;

	/* loops need to be complete and free too */
	ep = cent->nodes+1 + cent->rank;
	for(i = 0, np = centloop0(cent); np < ep; i++, np++){
		if(np->rank != i || np->used){
			return -1;
		}
	}
	ep = cent->nodes+1 + 2*cent->rank;
	for(i = 0, np = centloop1(cent); np < ep; i++, np++){
		if(np->rank != i || np->used){
			return -1;
		}
	}

	/* ready to merge! */
	ep = cent->nodes+1 + 2*cent->rank;
	for(np = cent->nodes; np < ep; np++)
		squnlink(pool, np);

	if(cent->topleft->next != cent->nodes)
		abort();
	cent->topleft->next = cent->nodes[0].next;

	squnlink(pool, cent->topleft);
	cent->topleft->rank += 2;
	sqpushfree(pool, cent->topleft);

	free(cent);

	return 0;
}

static void
mergenode(Sqpool *pool, Sqnode *topleft)
{
	Sqcent *cent;
	while(topleft != NULL){
		while(topleft->next != NULL){
			cent = sqcent(topleft->next);
			if(cent == sqcent(topleft)) /* no children */
				break;
			if(freecent(pool, cent) == -1) /* could not merge */
				break;
		}
		cent = sqcent(topleft);
		if(cent == NULL)
			break;
		topleft = cent->topleft; /* up the chain */
	}
}

void
sqinit(Sqpool *p, int rank)
{
	Sqnode *np;
	memset(p, 0, sizeof p[0]);
	np = rootnode(rank, 0, 0);
	p->free[rank] = np;
	p->root = np;
}

Sqnode *
sqalloc(Sqpool *pool, int w)
{
	Sqnode *np;
	int i, wrank;

	wrank = rankfor(w);
	if(pool->free[wrank] == NULL){
		if(wrank == 0 && pool->free[wrank+1] != NULL){
			wrank++;
			goto found;
		}
		for(i = wrank+1; i < nelem(pool->free); i++){
			if((np = sqpopfree(pool, i)) != NULL){
				splitnode(pool, np);
				sqpushfree(pool, np);
				break;
			}
		}
	}
found:
	if((np = sqpopfree(pool, wrank)) != NULL){
		np->used++;
		return np;
	}
	return NULL;
}


void
sqfree(Sqpool *pool, Sqnode *np)
{
	np->used = 0;
	sqpushfree(pool, np);
	mergenode(pool, np);
}

#ifdef TEST

static int dd = 2;
static int lspace = 200;
static int uoff = 0;
static int voff = 0;

static int
centnode(Sqnode *node)
{
	Sqcent *cent;
	cent = sqcent(node);
	if(node != NULL && cent != NULL && cent->nodes == node)
		return 1;
	return 0;
}

static void
drawsqnodes(Sqnode *node, Sqnode *end)
{
	uchar color[4];
	int fibcur;

	if(node == NULL)
		return;

	for(; node != end; node = node->next){
		if(centnode(node)){
			drawsqnodes(centloop0(sqcent(node)), node);
			drawsqnodes(centloop1(sqcent(node)), node);
		}

		fibcur = fib(node->rank);
		if(node->used){
			color[0] = 50;
			color[1] = 50;
			color[2] = 20;
			color[3] = 0xff;
		} else {
			idx2color(node->rank, color);
		}
		drawrect(
			&screen,
			rect(node->u0+uoff, node->v0+voff, node->u0+fibcur+uoff, node->v0+fibcur+voff),
			color
		);
	}
}

static int
drawsqlist(Sqnode *np, int u0, int v0, int d)
{
	int u, v, i;
	uchar color[4];
	u = u0;
	v = v0;
	i = 1;
	for(; np != NULL; np = np->lnext, i++){
		idx2color(np->rank, color);
		drawrect(&screen, rect(u, v, u+d, v+d), color);
		u += d+1;
		if(i >= lspace){
			u = u0;
			v += d+1;
			i = 0;
		}
	}
	return v+d+1;
}

void
drawsqpool(Sqpool *pool)
{
	int i, d, u, v;
	drawsqnodes(pool->root, NULL);
return;
	d = dd;
	u = uoff-lspace*(d+1);
	v = voff;
	for(i = 0; i < nelem(pool->free); i++)
		v = drawsqlist(pool->free[i], u, v, d);
}

double
exprand(double x)
{
	double z;

	do {
		z = drand48();
	} while(z == 0.0);
	return -x * log(z);
}

/*
 *	this came from the internet, works for now,
 *	replace with something understood.
 */
double
paretorand(double a, double b)
{
	double y1, y2, y3;
	y1 = 1.0 - drand48();
	y2 = -(1.0/b)*(log(y1));
	y3 = pow(a, exp(y2));
	return y3;
}

int
main(int argc, char *argv[])
{
	int t;
	int mouseid, drag = 0;
	int dragx, dragy;
	uchar black[4] = {0,0,0,255};

	Sqpool sqpool;
	Sqnode **nodes;
	int nnodes, anodes, rank;
	int freeing;
	int speedup;

	rank = 16;
	drawinit(5+lspace*(dd+1)+fib(rank), fib(rank));
	uoff = 5+lspace*(dd+1); //fib(rank);
	//root = rootnode(rank, 0, 0);

	if(argc > 1)
		initdrawstr(argv[1]);

	speedup = 1;
	anodes = 10000;
	nodes = malloc(anodes * sizeof nodes[0]);
	nnodes = 0;

	sqinit(&sqpool, rank);
	drawanimate(1);
	srand48((long)(1e3*timenow()));
	freeing = 0;
	for(;;){
		Input *inp, *inep;
		if((inp = drawevents(&inep)) != NULL){
			for(; inp < inep; inp++){
				int i;

				if(keystr(inp, "q") || keypress(inp, KeyDel))
					exit(0);
				if(keystr(inp, "f"))
					freeing = 1;
				if(keystr(inp, "a"))
					freeing = 0;
				if(keystr(inp, "p"))
					freeing = 2;
				if(keystr(inp, "s"))
					speedup /= 10;
				if(keystr(inp, "S"))
					speedup = speedup > 0 ? speedup*10 : 1;

				if(keystr(inp, "r")){
					for(i = 0; i < nnodes; i++){
						Sqnode *tmp;
						int j;

						j = lrand48() % nnodes;
						tmp = nodes[j];
						nodes[j] = nodes[i];
						nodes[i] = tmp;
					}
				}
				//if(keystr(inp, "a"))
				if(!drag && (t = mousebegin(inp)) != -1){
					mouseid = t;
					dragx = inp->xy[0];
					dragy = inp->xy[1];
					drag = 1;
				}
				if(drag && mousemove(inp) == mouseid){
					uoff += inp->xy[0] - dragx;
					voff += inp->xy[1] - dragy;
					dragx = inp->xy[0];
					dragy = inp->xy[1];
					if(mouseend(inp) == mouseid)
						drag = 0;
				}

				if(animate(inp)){
					if(freeing == 0){
						for(i = 0; i < speedup; i++){
							double er;
							//er = exprand(1.99);
							er = paretorand(1.67, 4.0);
							//er = 9.0*drand48()*drand48();
							//er = 50.0*drand48();
							//er = er < 20.0 ? er : 1.0;
							//er = 1.0;
							er = er < 256.0 ? er : 256.0;
							if(nnodes >= anodes){
								anodes += anodes;
								nodes = realloc(nodes, anodes * sizeof nodes[0]);
							}
							nodes[nnodes] = sqalloc(&sqpool, (int)er);
							if(nodes[nnodes] != NULL){
								nnodes++;
							} else {
								for(i = 0; i < nnodes; i++){
									Sqnode *tmp;
									int j;

									j = lrand48() % nnodes;
									tmp = nodes[j];
									nodes[j] = nodes[i];
									nodes[i] = tmp;
								}
								freeing++;
								break;
							}
						}
					}
					if(freeing == 1){
						for(i = 0; i < speedup; i++){
							if(nnodes > 0){
								sqfree(&sqpool, nodes[nnodes-1]);
								nnodes--;
							} else {
								if(sqempty(&sqpool))
									freeing = 0;
								else
									freeing++; // pause
							}
						}
					}
				}

			}

			drawrect(&screen, screen.r, black);
			//drawsqpool(&sqpool);

			static int count;
			int n;
			char msg[32];
			n = snprintf(msg, sizeof msg, "sgi%03d", count++);
			drawstr(&screen, rect(uoff, voff, screen.r.uend, screen.r.vend), msg, n);
		}
	}
	return 0;
}
#endif