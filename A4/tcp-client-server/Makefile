CFLAGS = -fPIC -Wall -I. -I../ece358a4/ -L../ece358a4/

all: mybind.c tcp-server.c tcp-client.c
	g++ $(CFLAGS) -o tc tcp-client.c -lrcs
	g++ $(CFLAGS) -pthread -o ts mybind.c tcp-server.c -lrcs

clean:
	rm -rf a.out *.o ts tc core
