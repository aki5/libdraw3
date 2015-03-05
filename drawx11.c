#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef unsigned char uchar;

static Display *display;
static Visual *visual;
static Window window;
static XImage *shmimage;
static XShmSegmentInfo shminfo;

static int needflush;
static int flushing;

int width;
int height;
int stride;
uchar *framebuffer;

static inline void
to16bpp(void)
{
	int i, j, ysoff, ydoff;
	uchar *spix;
	ushort *dpix;

	ysoff = 0;
	ydoff = 0;
	for(j = 0; j < height; j++){
		spix = framebuffer + ysoff;
		dpix = (ushort *)(shmimage->data + ydoff);
		for(i = 0; i < width; i++, spix += 4, dpix++){
			uchar r, g, b;
			b = spix[0] >> 3;
			g = spix[1] >> 2;
			r = spix[2] >> 3;
			*dpix = (r << 11) | (g << 5) | b;
		}
		ysoff += width;
		ydoff += shmimage->bytes_per_line;
	}
}

static int
shminit(void)
{
	shmimage = XShmCreateImage(
		display, visual,
		DefaultDepth(display, 0), ZPixmap,
		NULL, &shminfo, width, height
	);
	if(shmimage == NULL){
		fprintf(stderr, "shminit: cannot create shmimage\n");
		return -1;
	}
	shminfo.shmid = shmget(
		IPC_PRIVATE, shmimage->bytes_per_line*shmimage->height,
		IPC_CREAT | 0600
	);
	if(shminfo.shmid == -1){
		fprintf(stderr, "shminit: shmget fail\n");
		return -1;
	}
	shminfo.shmaddr = shmimage->data = shmat(shminfo.shmid, NULL, 0);
	if(shminfo.shmaddr == (void *)-1){
		fprintf(stderr, "shminit: shmat fail\n");
		return -1;
	}
	shminfo.readOnly = False;
	XShmAttach(display, &shminfo);

	framebuffer = (uchar *)shmimage->data;
	stride = shmimage->bytes_per_line;
	return 0;
}

static void
shmfree(void)
{
	XShmDetach(display, &shminfo);
	XDestroyImage(shmimage);
	if(shmdt(shminfo.shmaddr) == -1)
		fprintf(stderr, "shmdt fail!\n");
	XSync(display, 0);
	if(shmctl(shminfo.shmid, IPC_RMID, NULL) == -1)
		fprintf(stderr, "shmctl remove fail!\n");
}

int
drawinit(int w, int h)
{
	atexit(shmfree);
	if((display = XOpenDisplay(NULL)) == NULL){
		fprintf(stderr, "cannot open display!\n");
		return -1;
	}

	width = w > 0 ? w : 640;
	height = h > 0 ? h : 360;

	visual = DefaultVisual(display, 0);
	window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, width, height, 1, 0, 0);

	if(visual->class != TrueColor){
		fprintf(stderr, "Cannot handle non true color visual ...\n");
		return -1;
	}

	shminit();
	XSelectInput(display, window, KeyPressMask|ButtonPressMask|ExposureMask|StructureNotifyMask);
	XMapWindow(display, window);
	return XConnectionNumber(display);
}

void
drawflush(void)
{
	if(flushing){
		needflush = 1;
	} else {
		if((uchar *)shmimage->data != framebuffer)
			to16bpp();
		XShmPutImage(display, window, DefaultGC(display, 0), shmimage, 0, 0, 0, 0, width, height, True);
		flushing = 1;
		needflush = 0;
	}
}

void
drawreq(void)
{
	needflush = 1;
}

int
drawbusy(void)
{
	return flushing;
}

int
drawhandle(int fd, int block)
{
	int flags;
	if(fd != XConnectionNumber(display)){
		fprintf(stderr, "x11handle: passed fd does not match display\n");
		return -1;
	}
	flags = 0;
	while(block || XPending(display)){
		XEvent ev;
		block = 0;
		XNextEvent(display, &ev);
		switch(ev.type){
		case MapNotify:
		case ReparentNotify:
			continue;
		case Expose:
			flags |= 1; /* need redraw */
			continue;
		case KeyPress:
			flags |= 4;
			continue;
		case ButtonPress:
			exit(0);
		case ConfigureNotify:{
				XConfigureEvent *ce = &ev.xconfigure;
				if(ce->width != width || ce->height != height){
					shmfree();
					width = ce->width;
					height = ce->height;
					if(shminit() == -1)
						return -1;
					flags |= 2;
				}
				continue;
			}
		}
		if(ev.type == XShmGetEventBase(display) + ShmCompletion){
			flushing = 0;
			if(needflush)
				flags |= 1;
			continue;
		}
		fprintf(stderr, "unknown xevent %d\n", ev.type);
	}
	return flags;
}

int
drawhalt(void)
{
	drawflush();
	while((drawhandle(XConnectionNumber(display), 1) & 4) == 0)
		;
	return 0;
}

int
drawfd(void)
{
	return XConnectionNumber(display);
}
