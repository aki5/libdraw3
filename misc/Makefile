
CFLAGS=-O3 -fomit-frame-pointer -W -Wall -I..

PROGS=newton div255test

all: $(PROGS)

newton: newton.o
	$(CC) -o $@ $<

div255test: div255test.o
	$(CC) -o $@ $<

clean:
	rm -f *.o $(PROGS)
