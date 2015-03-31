ROOT=..

CFLAGS=-O3 -fomit-frame-pointer -W -Wall
TARGET_ARCH=port
ifeq ($(shell uname -m), armv6l)
	TARGET_ARCH=arm6
endif

all: $(ROOT)/libdraw3/libdraw3.a

include libdraw3.mk

dmacopy_test: dmacopy_test.o $(LIBDRAW3)
	$(CC) -o $@ dmacopy_test.o $(LIBDRAW3) -lm `freetype-config --libs`

sqpool: sqpool.c $(LIBDRAW3)
	$(CC) -DTEST $(CFLAGS) -o $@ sqpool.c $(LIBDRAW3) -lX11 -lXext -lm `freetype-config --libs`

clean:
	rm -f *.a *.o perf.* sqpool dmacopy_test

