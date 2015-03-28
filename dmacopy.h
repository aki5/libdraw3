
typedef struct Dmacopy Dmacopy;

struct Dmacopy {
	uintptr *srcpfn;
	uintptr *dstpfn;
	uintptr *descpfn;
	uintptr srcoff;
	uintptr dstoff;
	int ndstpg;
	int nsrcpg;
	int ndescpg;

	u32int *ctrl;
	u32int *chan;
	u32int *desc;
	int ndesc;

	uchar *src;
	uchar *dst;
	int len;
};

int initdmacopy(Dmacopy *cpy, uchar *dst, uchar *src, int len);
void freedmacopy(Dmacopy *cpy);
int dmacopydone(Dmacopy *cpy);
int startdmacopy(Dmacopy *cpy);
