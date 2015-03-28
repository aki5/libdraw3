
LIBDRAW3=$(ROOT)/libdraw3/libdraw3.a
LIBDRAW3_LIBS=-lX11 -lXext -lm `freetype-config --libs`

DRAW3_HFILES=\
	$(ROOT)/libdraw3/os.h\
	$(ROOT)/libdraw3/draw3.h\
	$(ROOT)/libdraw3/rectpool.h\
	$(ROOT)/libdraw3/sqpool.h\
	$(ROOT)/libdraw3/magicu.h\
	$(ROOT)/libdraw3/imgtools.h\

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
	$(ROOT)/libdraw3/drawlinuxfb.o\
	$(ROOT)/libdraw3/port.o\
	#$(ROOT)/libdraw3/drawx11.o\
	#$(ROOT)/libdraw3/keysym2ucs.o \
	#$(ROOT)/libdraw3/arm6.o\
	#$(ROOT)/libdraw3/magicu.o\
	#$(ROOT)/libdraw3/drawtri_simd.o\

$(ROOT)/libdraw3/%.o: $(ROOT)/libdraw3/%.c
	$(CC) -I$(ROOT)/libdraw3 $(CFLAGS) -c -o $@ $<

$(ROOT)/libdraw3/drawstr.o: $(ROOT)/libdraw3/drawstr.c
	$(CC) $(CFLAGS) `freetype-config --cflags` -o $@ -c $<

$(LIBDRAW3): $(DRAW3_OFILES)
	$(AR) -rv $@  $(DRAW3_OFILES)

$(DRAW3_OFILES): $(DRAW3_HFILES)
