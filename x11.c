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
	*u = 1.0f + 0.5f*x*(rectw(r)-1);
	*v = 1.0f - 0.5f*y*(recth(r)-1);
}

static Display *display;
static Visual *visual;
static Window window;
static XImage *shmimage;
static XShmSegmentInfo shminfo;

Input *inputs;
int ninputs;
static int ainputs;
static int animating;

static int width;
static int height;
static int stride;
static uchar *framebuffer;

Image screen;

long keysym2ucs(KeySym key);

static int
shminit(void)
{
	static int ntries;

	shmimage = XShmCreateImage(
		display, visual,
		DefaultDepth(display, 0),
		ZPixmap,
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
		fprintf(stderr, "shminit: shmget fail, tries %d, errno %d: %s\n", ntries, errno, strerror(errno));
		fprintf(stderr, "If you are on a mac, this is probably due to sysctls kern.sysv.shmmax and kern.sysv.shmall being too small\n");
		fprintf(stderr, "Try something like kern.sysv.shmmax=67108864 and kern.sysv.shmall=16384\n");
		XDestroyImage(shmimage);
		return -1;
	}
	shminfo.shmaddr = shmimage->data = shmat(shminfo.shmid, NULL, 0);
	if(shminfo.shmaddr == (void *)-1){
		fprintf(stderr, "shminit: shmat fail, errno %d: %s\n", errno, strerror(errno));
		return -1;
	}
	shminfo.readOnly = False;
	XShmAttach(display, &shminfo);

	framebuffer = (uchar *)shmimage->data;
	stride = shmimage->bytes_per_line;

	if(shmimage->bits_per_pixel < 24){
		screen.r = rect(0,0,shmimage->width,shmimage->height);
		screen.stride = shmimage->width*4;
		screen.len = shmimage->height*screen.stride;
		screen.img = malloc(screen.len);
	} else {
		screen.r = rect(0,0,shmimage->width,shmimage->height);
		screen.stride = shmimage->bytes_per_line;
		screen.img = (uchar *)shmimage->data;
		screen.len = shmimage->bytes_per_line*shmimage->height;
	} 

	ntries++;

	return 0;
}

static void
shmfree(void)
{
	XShmDetach(display, &shminfo);
	XDestroyImage(shmimage);
	XSync(display, 0);
	if(shmdt(shminfo.shmaddr) == -1)
		fprintf(stderr, "shmdt fail!\n");
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
drawflush(Rect r)
{
	if(shmimage->bits_per_pixel == 16){
		int i;
		uchar *src, *dst;
		int dvoff, svoff;
		int iend;

		src = screen.img;
		dst = (uchar*)shmimage->data;
		iend = screen.r.vend;
		svoff = 0;
		dvoff = 0;
		for(i = 0; i < iend; i++){
			pixcpy_dst16(dst+dvoff, src+svoff, screen.stride);
			svoff += screen.stride;
			dvoff += shmimage->bytes_per_line;
		}
	}
	XShmPutImage(
		display, window, DefaultGC(display, 0),
		shmimage,
		r.u0, r.v0,
		r.u0, r.v0, rectw(&r), recth(&r),
		True // generate completion event
	);
}

void
drawanimate(int flag)
{
	animating = flag;
}

Input *
getinputs(Input **ep)
{
	*ep = inputs + ninputs;
	return inputs;
}

static int input_prevmod;

void
addredraw(void)
{
	Input *inp;
	if(ninputs >= ainputs){
		ainputs += ainputs >= 64 ? ainputs : 64;
		inputs = realloc(inputs, ainputs * sizeof inputs[0]);
	}
	inp = inputs + ninputs;
	ninputs++;
	memset(inp, 0, sizeof inp[0]);
	inp->mouse = -1;
	inp->begin = Redraw;
}

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
			inp->begin = KeyStr;
		else
			inp->end = KeyStr;
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
	} else {
		inp->mouse = -1;
	}
}

/*
 *	drawevents2 calls flush to display anything that was drawn.
 */
static Input *
drawevents2(int block, int *ninp)
{
	static int flushing;

	if(!flushing){
		if(screen.dirty){
			drawflush(screen.r);
			flushing = 1;
			screen.dirty = 0;
		}
		ninputs = 0;
	}
	while((block && (flushing || ninputs == 0)) || XPending(display)){
		XEvent ev;
		XNextEvent(display, &ev);
		switch(ev.type){
		case MapNotify:
		case ReparentNotify:
			addredraw();
			continue;
		case Expose:
			addredraw();
			continue;
		case KeyPress:
		case KeyRelease:
			{
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

				u64int mod;
				KeySym keysym;
				int code;
				char keystr[8] = {0};

				keysym = XLookupKeysym(ep, ep->state & (ShiftMask|LockMask));
				if((code = keysym2ucs(keysym)) != -1)
					utf8encode(keystr, sizeof keystr-1, code);
				else
					keystr[0] = '\0';

				switch(keysym){
				case XK_Return:
				case XK_KP_Enter:
					strncpy(keystr, "\n", sizeof keystr-1);
					mod = KeyStr;
					break;
				case XK_Tab:
				case XK_KP_Tab:
				case XK_ISO_Left_Tab:
					strncpy(keystr, "\t", sizeof keystr-1);
					mod = KeyStr;
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
					mod = KeyMeta;	/* mac command key */
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					mod = KeyAlt;
					break;
				case XK_Super_L:
				case XK_Super_R:
fprintf(stderr, "super\n");
					mod = KeySuper;
					break;
				case XK_Hyper_L:
				case XK_Hyper_R:
fprintf(stderr, "hyper\n");
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
				case XK_Caps_Lock:
					mod = KeyCapsLock;
					break;
				default:
					mod = 0;
					break;
				}

				addinput(
					0, 0,
					keystr,
					mod,
					ev.type == KeyPress,
					ev.type == KeyRelease
				);
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
					mod = Mouse0 << ep->button;
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
				}
			}
			continue;	
		case EnterNotify:
		case LeaveNotify:
			fprintf(stderr, "enter/leave\n");
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
			}
			continue;
		case ConfigureNotify:
			{
				XConfigureEvent *ce = &ev.xconfigure;
				if(ce->width != width || ce->height != height){
					shmfree();
					width = ce->width;
					height = ce->height;
					if(shminit() == -1){
						*ninp = 0;
						return NULL;
					}
					addredraw();
				}
				continue;
			}
		}
		if(ev.type == XShmGetEventBase(display) + ShmCompletion){
			flushing = 0;
			if(animating){
				addredraw();
			}
			continue;
		}
		fprintf(stderr, "unknown xevent %d\n", ev.type);
	}

	if(!flushing && ninputs > 0){
		*ninp = ninputs;
		return inputs;
	}

	*ninp = 0;
	return NULL;
}

Input *
drawevents(int *ninp)
{
	return drawevents2(1, ninp);
}

Input *
drawevents_nonblock(int *ninp)
{
	return drawevents2(0, ninp);
}

int
drawfd(void)
{
	return XConnectionNumber(display);
}
