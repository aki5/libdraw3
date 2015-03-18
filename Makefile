
CFLAGS=-O3 -fomit-frame-pointer -W -Wall
#CFLAGS=-g -W -Wall

all: libdraw3.a

sqpool: sqpool.c libdraw3.a
	$(CC) -DTEST $(CFLAGS) -o $@ sqpool.c ./libdraw3.a -lX11 -lXext -lm `freetype-config --libs`

clean:
	rm -f *.a *.o perf.* sqpool

R:=.
include libdraw3.mk
