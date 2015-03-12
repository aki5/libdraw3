#include "os.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "draw3.h"


/*
 *	if the rect is 640x480, normalized display coordinates (ndc) are:
 *		ndc [-1, 1] => rect [0, 639]
 *		ndc [-1, 1] => rect [439, 0]
 */
void
ndc2rect(Rect *r, float x, float y, int *u, int *v)
{
	*u = 1.0f + 0.5f*x*(durect(r)-1);
	*v = 1.0f - 0.5f*y*(dvrect(r)-1);
}

static Display *display;
static Visual *visual;
static Window window;
static XImage *shmimage;
static XShmSegmentInfo shminfo;

static int needflush;
static int flushing;

Input *inputs;
int ninputs;
static int ainputs;

static int width;
static int height;
static int stride;
static uchar *framebuffer;

Image screen;

long keysym2ucs(KeySym key);

static char *
utf8_keysym(KeySym key)
{
	static __thread char utf8key[5];
	long rune;
	if((rune = keysym2ucs(key)) == -1)
		return NULL;
	if(rune <= 0x7f){
		utf8key[0] = rune;
		utf8key[1] = '\0';
	} else if(rune <= 0x7ff){
		utf8key[0] = 0xc0|((rune>>6)&0x1f);
		utf8key[1] = 0x80|((rune>>0)&0x3f);
		utf8key[2] = '\0';
	} else if(rune <= 0xfff){
		utf8key[0] = 0xe0|((rune>>12)&0x0f);
		utf8key[1] = 0x80|((rune>>6)&0x3f);
		utf8key[2] = 0x80|((rune>>0)&0x3f);
		utf8key[3] = '\0';
	} else if(rune <= 0x1fffff){
		utf8key[0] = 0xf0|((rune>>18)&0x07);
		utf8key[1] = 0x80|((rune>>12)&0x3f);
		utf8key[2] = 0x80|((rune>>6)&0x3f);
		utf8key[3] = 0x80|((rune>>0)&0x3f);
		utf8key[4] = '\0';
	} else {
		fprintf(stderr, "unicode U%x sequence out of supported range\n", rune); 
		return NULL; /* fail */
	}
	return utf8key;
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

	screen.stride = shmimage->bytes_per_line;
	screen.img = (uchar *)shmimage->data;
	screen.len = shmimage->bytes_per_line*shmimage->height;
	screen.r = rect(0,0,shmimage->width,shmimage->height);

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
	XSelectInput(
		display, window,
		EnterWindowMask|
		LeaveWindowMask|
		PointerMotionMask|
		KeyPressMask|
		KeyReleaseMask|
		ButtonPressMask|
		ButtonReleaseMask|
		ExposureMask|
		StructureNotifyMask|
		0
	);
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
			;
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

Input *
getinputs(Input **ep)
{
	*ep = inputs + ninputs;
	return inputs;
}

static int input_prevmod;

void
addinput(int x, int y, char *utf8key, u64int mod, int isbegin, int isend)
{
	Input *inp;
	if(ninputs >= ainputs){
		ainputs += ainputs >= 64 ? ainputs : 64;
		inputs = realloc(inputs, ainputs * sizeof inputs[0]);
	}
	inp = inputs + ninputs;
	ninputs++;
	memset(inp, 0, sizeof inp[0]);
	inp->on = input_prevmod;

	if(utf8key != NULL){
		strncpy(inp->str, utf8key, sizeof(inp->str)-1);
		inp->str[sizeof(inp->str)-1] = '\0';
		if(isbegin)
			inp->begin = KeyUtf8;
		else
			inp->end = KeyUtf8;
	}
	if(mod != 0){
		if(isbegin){
			inp->begin = mod;
			inp->on |= mod;
			inp->end = 0;
		} else if(isend) {
			inp->begin = 0;
			inp->on &= ~mod;
			inp->end = mod;
		}
	}
	input_prevmod = inp->on;
	if((mod & AnyMouse) != 0){
		inp->mouse = mod & AnyMouse;
		inp->xy[0] = x;
		inp->xy[1] = y;
	}
}


int
drawhandle(int fd, int block)
{
	static int hold_inputs;
	int redraw;
	if(fd != XConnectionNumber(display)){
		fprintf(stderr, "x11handle: passed fd does not match display\n");
		return -1;
	}
	redraw = 0;
	if(!hold_inputs)
		ninputs = 0;
	while(block || XPending(display)){
		XEvent ev;
		block = 0;
		XNextEvent(display, &ev);
		switch(ev.type){
		case MapNotify:
		case ReparentNotify:
			continue;
		case Expose:
			redraw = 1;
			continue;
		case KeyPress:
		case KeyRelease:
			{
				Input *input;
				XKeyEvent *ep = &ev.xkey;
				if(0)fprintf(
					stderr,
					"key %s %d (%x) '%c' at (%d,%d) state %x\n",
					ev.type == KeyPress ? "pressed" : "released",
					ep->keycode,
					ep->keycode,
					isprint(ep->keycode) ? ep->keycode : '.',
					ep->x, ep->y,
					ep->state
				);

				KeySym keysym;
				keysym = XKeycodeToKeysym(display, ep->keycode, 0);

				u64int mod;
				switch(keysym){
				case XK_Return:
				case XK_KP_Enter:
					mod = KeyEnter;
					break;
				case XK_Tab:
					mod = KeyTab;
					break;
				case XK_Break:
					mod = KeyBreak;
					break;
				case XK_BackSpace:
					mod = KeyBackSpace;
					break;
				case XK_Down:
					mod = KeyDown;
					break;
				case XK_End:
					mod = KeyEnd;
					break;
				case XK_Home:
					mod = KeyHome;
					break;
				case XK_Left:
					mod = KeyLeft;
					break;
				case XK_Page_Down:
					mod = KeyPageDown;
					break;
				case XK_Page_Up:
					mod = KeyPageUp;
					break;
				case XK_Right:
					mod = KeyRight;
					break;
				case XK_Up:
					mod = KeyUp;
					break;
				case XK_Shift_L:
				case XK_Shift_R:
					mod = KeyShift;
					break;
				case XK_Control_L:
				case XK_Control_R:
					mod = KeyControl;
					break;
				case XK_Meta_L:
				case XK_Meta_R:
					mod = KeyMeta;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					mod = KeyAlt;
					break;
				case XK_Super_L:
				case XK_Super_R:
					mod = KeySuper;
					break;
				case XK_Hyper_L:
				case XK_Hyper_R:
					mod = KeyHyper;
					break;
				case XK_KP_Insert:
				case XK_Insert:
					mod = KeyIns;
					break;
				case XK_KP_Delete:
				case XK_Delete:
					mod = KeyDel;
					break;
				default:
					mod = 0;
					break;
				}

				addinput(
					0, 0,
					utf8_keysym(keysym),
					mod,
					ev.type == KeyPress,
					ev.type == KeyRelease
				);
				redraw = 1;
			}
			continue;
		case ButtonPress:
		case ButtonRelease:
			{
				XButtonEvent *ep = &ev.xbutton;
				u64int mod;
				switch(ep->button){
				case 0:case 1:case 2:case 3:case 4:
				case 5:case 6:case 7:case 9:case 10:
					mod = Mouse0 + ep->button;
					break;
				default:
					fprintf(stderr, "unsupported mouse button %d\n", ep->button);
					mod = 0;
					break;
				}
				if(mod != 0){
					addinput(
						ep->x, ep->y,
						NULL,
						mod,
						ev.type == ButtonPress,
						ev.type == ButtonRelease
					);
					redraw = 1;
				}
			}
			continue;	
		case EnterNotify:
		case LeaveNotify:
			{
				XCrossingEvent *ep = &ev.xcrossing;
				if(0)fprintf(
					stderr,
					"%s at (%d,%d) state %x\n",
					ev.type == EnterNotify ? "enter" : "leave",
					ep->x, ep->y,
					ep->state
				);
			}
			continue;
		case MotionNotify:
			if(input_prevmod != 0){
				XMotionEvent *ep = &ev.xmotion;
				u64int m;
				if(0)fprintf(
					stderr,
					"motion at (%d,%d) state %x\n",
					ep->x, ep->y,
					ep->state
				);
				for(m = Mouse0; m <= LastMouse; m <<= 1){
					addinput(
						ep->x, ep->y,
						NULL,
						m,
						0,0
					);
				}
				redraw = 1;
			}
			continue;
		case ConfigureNotify:
			{
				XConfigureEvent *ce = &ev.xconfigure;
				if(ce->width != width || ce->height != height){
					shmfree();
					width = ce->width;
					height = ce->height;
					if(shminit() == -1)
						return -1;
					redraw = 1;
				}
				continue;
			}
		}
		if(ev.type == XShmGetEventBase(display) + ShmCompletion){
			flushing = 0;
			if(needflush){
				hold_inputs = 0;
				redraw = 1;
			}
			continue;
		}
		fprintf(stderr, "unknown xevent %d\n", ev.type);
	}
	if(flushing){
		needflush = redraw;
		hold_inputs = 1;
		return 0;
	}
	hold_inputs = 0;
	return redraw;
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
