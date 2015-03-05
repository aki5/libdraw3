
CFLAGS=-O2 -fomit-frame-pointer -W -Wall

libdraw3.a: drawx11.o drawtri.o drawpoly.o
	$(AR) r $@  drawx11.o drawtri.o drawpoly.o

drawtest: drawtest.o libdraw3.a
	$(CC) -o $@ drawtest.o libdraw3.a -lX11 -lXext

clean:
	rm -f *.a *.o drawtest
