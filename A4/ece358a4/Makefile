CFLAGS = -c -fPIC -Wall -I.
RUNCFLAGS = -fPIC -Wall -I. -L.

all: rcsapp

rcsapp: mybind.c rcsapp-client.c rcsapp-server.c librcs.a
	g++ $(RUNCFLAGS) -o rcsapp-client rcsapp-client.c -lrcs
	g++ $(RUNCFLAGS) -pthread -o rcsapp-server rcsapp-server.c -lrcs

mybind.o: mybind.c
	gcc $(CFLAGS) -o $@ mybind.c

ucp.o: ucp.c
	gcc $(CFLAGS) -o $@ ucp.c

rcs.o: rcs.cc rcs.h ucp.o
	g++ $(CFLAGS) -o $@ rcs.cc

librcs.a: rcs.o ucp.o mybind.o
	ar rcs -o $@ rcs.o ucp.o mybind.o

clean:
	rm -f *.o *.a *.s rcsapp-server rcsapp-client
