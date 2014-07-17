CFLAGS = -fPIC -Wall -I. -L../ece358a4/ -I../ece358a4/

all: mybind.c tcp-server.c tcp-client.c
	gcc $(CFLAGS) -o tc tcp-client.c -lrcs
	gcc $(CFLAGS) -pthread -o ts mybind.c tcp-server.c -lrcs

clean:
	rm -rf a.out *.o ts tc core
