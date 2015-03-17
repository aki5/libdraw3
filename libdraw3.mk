
DRAW3_HFILES=\
	$(R)/os.h\
	$(R)/draw3.h\
	$(R)/rectpool.h\
	$(R)/sqpool.h\

DRAW3_OFILES=\
	$(R)/drawx11.o\
	$(R)/keysym2ucs.o \
	$(R)/drawtri_simd.o\
	$(R)/drawpoly.o\
	$(R)/drawrect.o\
	$(R)/rectpool.o\
	$(R)/loadimage.o\
	$(R)/sqpool.o\
	$(R)/drawstr.o\

$(R)/%.o: $(R)/%.c
	$(CC) -I$(R) $(CFLAGS) -c -o $@ $<

$(R)/drawstr.o: $(R)/drawstr.c
	$(CC) $(CFLAGS) `freetype-config --cflags` -o $@ -c $<

libdraw3.a: $(DRAW3_OFILES)
	$(AR) r $@  $(DRAW3_OFILES)

$(DRAW_OFILES): $(DRAW_HFILES)
