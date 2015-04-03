ROOT=..

CFLAGS=-Os -fomit-frame-pointer -W -Wall -I/opt/local/include
TARGET_ARCH=port
ifeq ($(shell uname -m), armv6l)
	TARGET_ARCH=arm6
endif

TARGETS=
ifeq ($(shell uname -s), Darwin)
	TARGETS+=$(ROOT)/libdraw3/libdraw3-x11.a
endif
ifeq ($(shell uname -s), Linux)
	TARGETS+=$(ROOT)/libdraw3/libdraw3-x11.a
	TARGETS+=$(ROOT)/libdraw3/libdraw3-linuxfb.a
endif

all: $(TARGETS)

include libdraw3.mk

sqpool: sqpool.c $(LIBDRAW3_X11)
	$(CC) -DTEST $(CFLAGS) -o $@ sqpool.c $(LIBDRAW3_X11) -lX11 -lXext -lm `freetype-config --libs`

clean:
	rm -f *.a *.o perf.* sqpool dmacopy_test

