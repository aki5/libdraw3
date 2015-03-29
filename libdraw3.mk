
#TARGET_ARCH=$(shell uname -m)
#TARGET_ARCH=x86_64

LIBDRAW3=$(ROOT)/libdraw3/libdraw3.a

LIBDRAW3_LIBS=-lX11 -lXext -lm `freetype-config --libs`

DRAW3_HFILES=\
	$(ROOT)/libdraw3/os.h\
	$(ROOT)/libdraw3/draw3.h\
	$(ROOT)/libdraw3/imgtools.h\

	#$(ROOT)/libdraw3/rectpool.h\
	#$(ROOT)/libdraw3/sqpool.h\
	#$(ROOT)/libdraw3/magicu.h\
	#$(ROOT)/libdraw3/dmacopy.h\

DRAW3_HW=\
	$(ROOT)/libdraw3/$(TARGET_ARCH).o\
	
DRAW3_XSHM=\
	$(ROOT)/libdraw3/drawx11.o\
	$(ROOT)/libdraw3/keysym2ucs.o \

DRAW3_LINUXFB=\
	$(ROOT)/libdraw3/drawlinuxfb.o\

DRAW3_OFILES=\
	$(ROOT)/libdraw3/drawpoly.o\
	$(ROOT)/libdraw3/drawrect.o\
	$(ROOT)/libdraw3/rectpool.o\
	$(ROOT)/libdraw3/loadimage.o\
	$(ROOT)/libdraw3/sqpool.o\
	$(ROOT)/libdraw3/drawstr.o\
	$(ROOT)/libdraw3/drawtri.o\
	$(ROOT)/libdraw3/drawcircle.o\
	$(ROOT)/libdraw3/drawellipse.o\
	$(ROOT)/libdraw3/$(TARGET_ARCH).o\
	$(DRAW3_LINUXFB)\

	#$(DRAW3_XSHM)\

$(ROOT)/libdraw3/%.o: $(ROOT)/libdraw3/%.c
	$(CC) -I$(ROOT)/libdraw3 $(CFLAGS) -c -o $@ $<

$(ROOT)/libdraw3/drawstr.o: $(ROOT)/libdraw3/drawstr.c
	$(CC) -I$(ROOT)/libdraw3 $(CFLAGS) `freetype-config --cflags` -o $@ -c $<

$(LIBDRAW3): $(DRAW3_OFILES)
	$(AR) -rv $@  $(DRAW3_OFILES)

$(DRAW3_OFILES): $(DRAW3_HFILES)

