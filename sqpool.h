

typedef struct Sqpool Sqpool;
typedef struct Sqcent Sqcent;
typedef struct Sqnode Sqnode;

struct Sqpool {
	Sqcent *cents;
	Sqnode *root;
	Sqnode *free[32];
};

struct Sqnode {
	Sqcent *cent; /* this is redundant */
	Sqnode *next; /* a parent link */
	Sqnode *lnext;
	Sqnode *lprev;
	short u0;
	short v0;
	short rank : 8, used : 2;
};

struct Sqcent {
	Sqnode *topleft;
	int rank;
	Sqnode nodes[1];
};

Sqnode *sqalloc(Sqpool *p, int w);
void sqfree(Sqpool *p, Sqnode *np);
void sqinit(Sqpool *p, int rank);
int sqempty(Sqpool *pool);
