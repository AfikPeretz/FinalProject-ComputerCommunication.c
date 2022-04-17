all: udp-server udp-client
 
udp-server: udp-server.o
	g++ -o udp-server udp-server.o

udp-server.o: udp-server.cpp
	g++ -std=c++11 -c udp-server.cpp

udp-client: udp-client.o
	g++ -o udp-client udp-client.o

udp-client.o: udp-client.cpp
	g++ -std=c++11 -c udp-client.cpp

.PHONY: clean all

clean:
	rm *.o udp-server udp-client
