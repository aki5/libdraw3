
CFLAGS=-O3 -fomit-frame-pointer

all: libdraw3.a

clean:
	rm -f *.a *.o

R:=.
include libdraw3.mk
