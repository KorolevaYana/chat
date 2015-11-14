.PHONY: all

all: server_test

server_test: chat.h my_epoll.h socket_lib.h

SOURCES := server_test.cpp server.cpp client.cpp my_socket.cpp my_epoll.cpp chat.cpp

server_test: $(SOURCES)
	g++ -Wall -Wextra -std=c++11 -g -o $@ $(SOURCES)

