CFLAGS = -c -fPIC -Wall -I.

# ucp.o:
# 	g++ $(CFLAGS) -o ucp.o 

all: librcs.a test.o mybind.o

mybind.o: mybind.c
	gcc $(CFLAGS) -o $@ mybind.c

ucp.o: ucp.c
	gcc $(CFLAGS) -o $@ ucp.c

rcs.o: rcs.cc rcs.h ucp.o
	g++ $(CFLAGS) -o $@ rcs.cc

librcs.a: rcs.o ucp.o mybind.o
	ar rcs -o $@ rcs.o ucp.o mybind.o

test.o: test.cc rcs.h
	g++ -fPIC -Wall -I. -L. -o $@ test.cc -lrcs

clean:
	rm -f *.o *.a *.s
