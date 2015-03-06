
DRAW3_HFILES=\
	$(R)/os.h\
	$(R)/draw3.h\

DRAW3_OFILES=\
	$(R)/drawx11.o\
	$(R)/drawtri_simd.o\
	$(R)/drawpoly.o\

$(R)/%.o: $(R)/%.c
	$(CC) -I$(R) $(CFLAGS) -c -o $@ $<

libdraw3.a: $(DRAW3_OFILES)
	$(AR) r $@  $(DRAW3_OFILES)

$(DRAW_OFILES): $(DRAW_HFILES)
