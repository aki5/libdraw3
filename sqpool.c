
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

int
centnode(Sqnode *node)
{
	if(node->cent != NULL && node->cent->nodes == node)
		return 1;
	return 0;
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

int uoff = 0;
int voff = 0;

void
drawnodes(Sqnode *node, Sqnode *end)
{
	uchar color[4];
	int fibcur;

	if(node == NULL)
		return;

	for(; node != end; node = node->next){
		if(centnode(node)){
			drawnodes(centloop0(node->cent), node);
			drawnodes(centloop1(node->cent), node);
		}

		fibcur = fib(node->rank);
		idx2color(node->rank, color);
		drawrect(
			&screen,
			rect(node->u0+uoff, node->v0+voff, node->u0+fibcur+uoff, node->v0+fibcur+voff),
			color
		);
	}
}

Sqcent *
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

	/* trace the outer corners clockwise. start by going left (dir=3) */
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

Sqnode *
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

void
divnode(Sqnode *np)
{
	Sqcent *ncent;
	int rank;

	rank = np->rank;
	if(rank < 2)
		return;
	ncent = newcent(rank-1, np->u0 + fib(rank-2), np->v0 + fib(rank-2));
	ncent->nodes[0].next = np->next;
	np->next = ncent->nodes;
	np->rank -= 2;

	return;
}

int
main(void)
{
	int t;
	int mouseid, drag = 0;
	int dragx, dragy;
	uchar black[4] = {0,0,0,255};
	Sqnode *root;
	int rank;

	rank = 15;
	drawinit(fib(rank), fib(rank));
	root = rootnode(rank, 0, 0);

	for(;;){
		Input *inp, *inep;
		if((inp = drawevents(&inep)) != NULL){
			for(; inp < inep; inp++){
				if(keystr(inp, "q"))
					exit(0);
				if(keystr(inp, "d"))
					divnode(root);

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
			}

			drawrect(&screen, screen.r, black);
			drawnodes(root, NULL);
		}
	}
	return 0;
}