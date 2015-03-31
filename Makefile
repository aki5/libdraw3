ROOT=..

CFLAGS=-Os -fomit-frame-pointer -W -Wall
TARGET_ARCH=port
ifeq ($(shell uname -m), armv6l)
	TARGET_ARCH=arm6
endif

all: $(ROOT)/libdraw3/libdraw3-x11.a $(ROOT)/libdraw3/libdraw3-linuxfb.a

include libdraw3.mk

sqpool: sqpool.c $(LIBDRAW3_X11)
	$(CC) -DTEST $(CFLAGS) -o $@ sqpool.c $(LIBDRAW3_X11) -lX11 -lXext -lm `freetype-config --libs`

clean:
	rm -f *.a *.o perf.* sqpool dmacopy_test

