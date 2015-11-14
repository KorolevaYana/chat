#include "socket_lib.h"

server::server(){}

server::server(const my_socket& heart, int max_connected,
		std::function<int(my_socket)> set_task): 
	heart(heart), max_connected(max_connected) {
	this->set_task = set_task;
}

server::~server() {
}

int server::bind_heart() {
	int option = 1;
	setsockopt(this->heart.fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if (bind(this->heart.fd, (sockaddr*)&(this->heart.addr), sizeof(this->heart.addr)) < 0) {
		printf("Can't bind: %m.\n");
		return 1;
	}
	int r = listen(this->heart.fd, this->max_connected);
	fprintf (stderr, "listen (fd = %d) = %d\n", this->heart.fd, r);
	return 0;
}

client server::accept_heart() {
	my_socket tmp;
	int addr_len = sizeof(tmp.addr);
  tmp.fd = accept(this->heart.fd, (sockaddr*)&tmp.addr, (socklen_t*)&addr_len);
	if (tmp.fd < 0) {
		printf("Can't accept. %m\n");
	} else {
		this->heart.complete_task(this->heart);
		tmp.task = this->set_task;
	}
	return client(tmp);
}

my_socket server::get_heart() {
	return heart;
}
