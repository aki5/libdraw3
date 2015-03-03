
CFLAGS=-g

libdraw3.a: drawx11.o drawtri.o
	$(AR) r $@  drawx11.o drawtri.o

drawtest: drawtest.o libdraw3.a
	$(CC) -o $@ drawtest.o libdraw3.a -lX11 -lXext

clean:
	rm -f *.a *.o drawtest
