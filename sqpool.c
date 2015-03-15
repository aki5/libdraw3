
#include "os.h"
#include "draw3.h"

typedef struct Sqcent Sqcent;
typedef struct Sqnode Sqnode;

struct Sqnode {
	Sqcent *cent;
	Sqnode *next;
	int u0;
	int v0;
	int rank;
	int dir;
};

struct Sqcent {
	Sqcent *next;
	Sqcent *prev;
	int rank;
	Sqnode nodes[1];
};

int
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

Sqnode *
centloop0(Sqcent *cent)
{
	return cent->rank > 0 ? cent->nodes+1 : cent->nodes;
}

Sqnode *
centloop1(Sqcent *cent)
{
	return cent->rank > 0 ? cent->nodes+1 + cent->rank : cent->nodes;
}

int uoff = 320;
int voff = 200;

void
drawcent(Sqcent *cent)
{
	Sqnode *np;
	uchar color[4];
	int fibcur;

	for(; cent != NULL; cent = cent->next){
		np = cent->nodes;
		printf("cent rank %d\n", cent->rank);
		fibcur = fib(np->rank);

		for(np = centloop0(cent); np != cent->nodes; np = np->next){
			fibcur = fib(np->rank);
			idx2color(np->rank, color);
			printf(
				"	loop0 rank %d (%d,%d) dir %d fibcur %d\n",
				np->rank, np->u0, np->v0, np->dir, fibcur
			);
			drawrect(
				&screen,
				rect(np->u0+uoff, np->v0+voff, np->u0+fibcur+uoff, np->v0+fibcur+voff),
				color
			);
		}
		for(np = centloop1(cent); np != cent->nodes; np = np->next){
			fibcur = fib(np->rank);
			idx2color(np->rank, color);
			printf(
				"	loop1 rank %d (%d,%d) dir %d fibcur %d\n",
				np->rank, np->u0, np->v0, np->dir, fibcur
			);
			drawrect(
				&screen,
				rect(np->u0+uoff, np->v0+voff, np->u0+fibcur+uoff, np->v0+fibcur+voff),
				color
			);
		}
		np = cent->nodes;
		fibcur = fib(np->rank);
		idx2color(np->rank, color);
		drawrect(
			&screen,
			rect(np->u0+uoff, np->v0+voff, np->u0+fibcur+uoff, np->v0+fibcur+voff),
			color
		);
	}
}

Sqcent *
newcent(int rank, short u0, short v0)
{
	Sqcent *cent;
	Sqnode *np, *ep;
	int a, b, t, i;
	short u0topleft, v0topleft;
	short fibcur, fibprev;

	if(rank < 0)
		return NULL;

	cent = malloc(sizeof cent[0] + 2*(rank)*sizeof cent->nodes[0]);
	memset(cent, 0, sizeof cent[0] + 2*(rank)*sizeof cent->nodes[0]);
	cent->rank = rank;
	np = cent->nodes;
	np->rank = rank;
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

	/* u0, v0 trace the outer corner pixels. we start by going up (dir=3) */
	u0 = cent->nodes[0].u0 + fib(rank)-1;
	v0 = cent->nodes[0].v0 - fib(rank-1);
	u0topleft = -(fib(rank-1)-1);
	v0topleft = 0;
	ep = centloop0(cent)-1;
	for(np = centloop1(cent)-1, i = 0; np > ep; np--, i++){
		np->rank = rank-i-1;
		np->dir = i&3;
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

	/* u0, v0 trace the outer corner pixels. we start by going left (dir=3) */
	u0 = cent->nodes[0].u0 - fib(rank-1);
	v0 = cent->nodes[0].v0 + fib(rank)-1;
	u0topleft = 0;
	v0topleft = -(fib(rank-1)-1);
	ep = centloop1(cent)-1;
	for(np = centloop1(cent)+rank-1, i = 0; np > ep; np--, i++){
		np->rank = rank-i-1;
		np->dir = i&3;
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

Sqcent *
rootcent(int rank, short u0, short v0)
{
	Sqcent *cent;
	cent = newcent(0, u0, v0);
	cent->rank = 0;
	cent->nodes[0].rank = rank;
	return cent;
}

Sqcent *
divcent(Sqcent *cent)
{
	Sqcent *ncent;
	Sqnode *np;
	int rank;

	np = cent->nodes;
	rank = np->rank;
	if(rank < 2)
		return NULL;

	ncent = newcent(rank-1, np->u0 + fib(rank-2), np->v0 + fib(rank-2));
	ncent->next = cent->next;
	ncent->prev = cent;
	cent->next = ncent;
	cent->nodes[0].rank -= 2;

	return cent;
}

int
main(void)
{
	int i, t;
	int mouseid, drag = 0;
	int dragx, dragy;
	uchar black[4] = {0,0,0,255};

	drawinit(uoff + 8*uoff/5, voff + 8*voff/5);
	for(i = 0; i < 7; i++){
		Sqcent *cent;
		if((cent = newcent(i, 0, 0)) != NULL){
			drawcent(cent);
			free(cent);
		}
	}
	for(;;){
		Input *inp, *inep;
		if((inp = drawevents(&inep)) != NULL){
			for(; inp < inep; inp++){
				if(keystr(inp, "q"))
					exit(0);
				if(!drag && (t = mousebegin(inp)) != -1){
					mouseid = t;
fprintf(stderr, "ding, mousebegin %d\n", mouseid);
					dragx = inp->xy[0];
					dragy = inp->xy[1];
					drag = 1;
				}
				if(drag && mousemove(inp) == mouseid){
fprintf(stderr, "move, mouseid %d\n", mouseid);
					uoff += inp->xy[0] - dragx;
					voff += inp->xy[1] - dragy;
					dragx = inp->xy[0];
					dragy = inp->xy[1];
					if(mouseend(inp) == mouseid){
fprintf(stderr, "dong, mouseid %d\n", mouseid);
						drag = 0;
					}
				}
			}
			Sqcent *cent, *cp;
			drawrect(&screen, screen.r, black);
			if((cent = rootcent(13, 0, 0)) != NULL){
				for(cp = cent; cp != NULL; cp = cp->next)
					divcent(cp);
				drawcent(cent);
				free(cent);
			}
		}
	}
	return 0;
}
