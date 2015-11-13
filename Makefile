all:
	g++ -Wall -Wextra -std=c++11 -g -o server_test server_test.cpp server.cpp client.cpp my_socket.cpp my_epoll.cpp chat.cpp

