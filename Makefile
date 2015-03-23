ROOT=..

CFLAGS=-O3 -fomit-frame-pointer -W -Wall
all: sqpool

include libdraw3.mk

sqpool: sqpool.c $(LIBDRAW3)
	$(CC) -DTEST $(CFLAGS) -o $@ sqpool.c $(LIBDRAW3) -lX11 -lXext -lm `freetype-config --libs`

clean:
	rm -f *.a *.o perf.* sqpool

