all: TCP

TCP: TCPClient TCPServer

TCPClient: TCPClient.cc
	g++ -o client TCPClient.cc

TCPServer: TCPServer.c
	g++ -o server TCPServer.c

clean:
	rm client server
