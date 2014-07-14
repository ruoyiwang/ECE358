CFLAGS = -c -fPIC -Wall -I.

# ucp.o:
# 	g++ $(CFLAGS) -o ucp.o 

ucp.o: ucp.c
	g++ $(CFLAGS) -o $@ ucp.c

rcs.o: rcs.cc rcs.h ucp.o
	g++ $(CFLAGS) -o $@ rcs.cc

librcs.a: rcs.o
	ar rcs -o $@ rcs.o

test.o: test.cc rcs.h
	g++ -fPIC -Wall -I. -L. -o $@ test.cc -lrcs

clean:
	rm -f *.o *.a *.s
