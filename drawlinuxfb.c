#include "os.h"
#include "draw3.h"
#include "dmacopy.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <termios.h> /* sigh */

Input *inputs;
int ninputs;
static int ainputs;
static int animating;

static int width;
static int height;
static int stride;

static int mousexy[2];
static int mousefd;
static int keybfd;
uchar *framebuffer;

Image *ptrim;
Image *ptrbg;

Image phys;
Image screen;

static struct termios stupid;
static void
drawdie(void)
{
	munmap(framebuffer, screen.len);
	munmap(screen.img, screen.len);
	ioctl(0, KDSKBMODE, K_XLATE);
	tcsetattr(0, TCSAFLUSH, &stupid);
}

int
drawinit(int w, int h)
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo;
	int fd;

	atexit(drawdie);

	if((fd = open("/dev/fb0", O_RDWR)) == 1){
		fprintf(stderr, "drawinit: open /dev/fb0: %s\n", strerror(errno));
		goto errout;
	}

	if(ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) == -1){
		fprintf(stderr, "drawinit: fixed screen info for /dev/fb0: %s\n", strerror(errno));
		return -1;
	}

	if(fixinfo.type != FB_TYPE_PACKED_PIXELS){
		fprintf(stderr, "drawinit: unsupported framebuffer, want %d got %d)\n",
			FB_TYPE_PACKED_PIXELS,
			fixinfo.type
		);
		goto errout;
	}

	switch(fixinfo.visual){
	case FB_VISUAL_TRUECOLOR:
	case FB_VISUAL_DIRECTCOLOR:
		break;
	default:
		fprintf(stderr, "drawinit: visual %d not truecolor or directcolor\n", fixinfo.visual);
		goto errout;
	}

retry_varinfo:
	if(ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) == -1){
		fprintf(stderr, "drawinit: variable screen info for /dev/fb0: %s\n", strerror(errno));
		goto errout;
	}

	switch(varinfo.bits_per_pixel){
	case 32:
		break;
	default:
		fprintf(stderr, "drawinit: %d bits per pixel not supported (want 32)\n", varinfo.bits_per_pixel);
		goto errout;
	}

	if(0)
	if(varinfo.yres_virtual < 2*varinfo.yres){
		varinfo.xres_virtual = varinfo.xres;
		varinfo.yres_virtual = varinfo.yres;
		varinfo.width = varinfo.xres;
		varinfo.height = varinfo.yres;
		varinfo.xoffset = 0;
		varinfo.yoffset = 0;
		if(ioctl(fd, FBIOPUT_VSCREENINFO, &varinfo) == -1){
			fprintf(stderr, "drawinit: could not set up double buffering, %s\n", strerror(errno));
			goto errout;
		}
		goto retry_varinfo;
	}

	framebuffer = mmap(NULL, fixinfo.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	memset(framebuffer, 0, fixinfo.smem_len); /* just fault it in.. */
	close(fd);

	screen.r = rect(0, 0, varinfo.xres, varinfo.yres);
	screen.len = fixinfo.smem_len;
	screen.stride = fixinfo.line_length;
	screen.img = mmap(NULL, screen.len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	memset(screen.img, 0, screen.len); /* just to fault it in.. */

	/* just for the mouse symbol */
	phys.r = screen.r;
	phys.len = screen.len;
	phys.stride = screen.stride;
	phys.img = framebuffer;


	tcgetattr(0, &stupid);
	ioctl(0, KDSKBMODE, K_UNICODE);

	if((mousefd = open("/dev/input/mice", O_RDONLY)) == -1){
		fprintf(stderr, "drawinit: could not open /dev/input/mice: %s\n", strerror(errno));
		goto errout;
	}
	keybfd = 0;

	ptrim = allocimage(rect(0,0,16,16), color(80,100,180,180));
	ptrbg = allocimage(rect(0,0,16,16), color(0,0,0,0));

	return mousefd;

errout:
	if(fd != -1)
		close(fd);
	return -1;
}

static void
ptrdraw(int pt[2])
{
	screen.r.u0 = pt[0];
	screen.r.v0 = pt[1];
	blend2(
		ptrbg,
		ptrbg->r,
		&screen,
		BlendCopy
	);
	screen.r.u0 = 0;
	screen.r.v0 = 0;
	blend2(
		&screen,
		rect(pt[0], pt[1], pt[0]+rectw(&ptrim->r), pt[1]+recth(&ptrim->r)),
		ptrim,
		BlendOver
	);
}

static void
ptrundraw(int pt[2])
{
	blend2(
		&screen,
		rect(pt[0], pt[1], pt[0]+rectw(&ptrim->r), pt[1]+recth(&ptrim->r)),
		ptrbg,
		BlendCopy
	);
}

static void
ptrflush(Rect r)
{
	screen.r.u0 = r.u0;
	screen.r.v0 = r.v0;
	blend2(
		&phys,
		r,
		&screen,
		BlendCopy
	);
	screen.r.u0 = 0;
	screen.r.v0 = 0;
}

void
drawflush(Rect r)
{
	ptrdraw(mousexy);
	memcpy(framebuffer, screen.img, screen.len);
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
drawevents2(int block, Input **inepp)
{
	struct timeval timeout;
	fd_set rset;
	int i, n;

	if(screen.dirty){
		drawflush(screen.r);
		screen.dirty = 0;
	}
	ninputs = 0;

	FD_ZERO(&rset);
	FD_SET(mousefd, &rset);
	FD_SET(keybfd, &rset);
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	n = select(mousefd+1, &rset, NULL, NULL, &timeout);

	while((block && ninputs == 0)){ // || event pending

		if(FD_ISSET(keybfd, &rset)){
			char buf[16];
			n = read(keybfd, buf, sizeof buf-1);
			if(n > 0){
				buf[n] = '\0';
				fprintf(stderr, "got '%s'\n", buf);
			}
		}

		if(FD_ISSET(mousefd, &rset)){
			Rect flushr;
			static uchar mev0;
			uchar mev[3];
			if((n = read(mousefd, mev, sizeof mev)) != sizeof mev){
				fprintf(stderr, "drawevents2: partial mouse: %d: %s\n", n, strerror(errno));
			} else {
				flushr.u0 = mousexy[0];
				flushr.v0 = mousexy[1];
				ptrundraw(mousexy);
				mousexy[0] += (signed char)mev[1];
				mousexy[1] -= (signed char)mev[2]; 
				ptrdraw(mousexy);
				flushr.uend = mousexy[0];
				flushr.vend = mousexy[1];
				if(flushr.uend < flushr.u0){
					short tmp;
					tmp = flushr.uend;
					flushr.uend = flushr.u0;
					flushr.u0 = tmp;
				}
				if(flushr.vend < flushr.v0){
					short tmp;
					tmp = flushr.vend;
					flushr.vend = flushr.v0;
					flushr.v0 = tmp;
				}
				flushr.uend += rectw(&ptrim->r);
				flushr.vend += recth(&ptrim->r);
				ptrflush(flushr);

				/* I dislike the following too, but it does get the job done */
				if((mev0&1) == 0 && (mev[0]&1) == 1)
					addinput(mousexy[0], mousexy[1], NULL, Mouse1, 1, 0);
				if((mev0&1) == 1 && (mev[0]&1) == 0)
					addinput(mousexy[0], mousexy[1], NULL, Mouse1, 0, 1);
				if((mev0&1) == 1 && (mev[0]&1) == 1)
					addinput(mousexy[0], mousexy[1], NULL, Mouse1, 0, 0);

				if((mev0&2) == 0 && (mev[0]&2) == 2)
					addinput(mousexy[0], mousexy[1], NULL, Mouse3, 1, 0);
				if((mev0&2) == 2 && (mev[0]&2) == 0)
					addinput(mousexy[0], mousexy[1], NULL, Mouse3, 0, 1);
				if((mev0&2) == 2 && (mev[0]&2) == 2)
					addinput(mousexy[0], mousexy[1], NULL, Mouse3, 0, 0);

				if((mev0&4) == 0 && (mev[0]&4) == 4)
					addinput(mousexy[0], mousexy[1], NULL, Mouse2, 1, 0);
				if((mev0&4) == 4 && (mev[0]&4) == 0)
					addinput(mousexy[0], mousexy[1], NULL, Mouse2, 0, 1);
				if((mev0&4) == 4 && (mev[0]&4) == 4)
					addinput(mousexy[0], mousexy[1], NULL, Mouse2, 0, 0);
				mev0 = mev[0];
			}
		}

		FD_ZERO(&rset);
		FD_SET(mousefd, &rset);
		FD_SET(keybfd, &rset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
		//n = select(mousefd+1, &rset, NULL, NULL, &timeout);
		n = select(mousefd+1, &rset, NULL, NULL, NULL);

		//addredraw(); /* faking everything here. */
	}

	if(ninputs > 0){
		*inepp = inputs+ninputs;
		return inputs;
	}

	*inepp = NULL;
	return NULL;
}

Input *
drawevents(Input **inepp)
{
	return drawevents2(1, inepp);
}

Input *
drawevents_nonblock(Input **inepp)
{
	return drawevents2(0, inepp);
}

int
drawfd(void)
{
	return -1;
}
