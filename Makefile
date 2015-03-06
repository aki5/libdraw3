R:=.
include libdraw3.mk

all: libdraw3.a

clean:
	rm -f *.a $(DRAW3_OFILES)
