
typedef unsigned char uchar;

extern int width;
extern int height;
extern int stride;
extern uchar *framebuffer;

int drawinit(int w, int h);
void drawflush(void);
int drawhandle(int fd, int doblock);
int drawfd(void);
void drawreq(void);
int drawbusy(void);

void drawtri(uchar *img, int width, int height, short *a, short *b, short *c, uchar *color);
void drawtris(uchar *img, int width, int height, short *tris, uchar *colors, int ntris);
