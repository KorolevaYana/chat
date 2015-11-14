#include "socket_lib.h"

my_socket::my_socket() {}

my_socket::my_socket(const my_socket& other) {
	this->fd = other.fd;
	this->addr = other.addr;
	this->task = other.task;
}

my_socket::my_socket(const char* ip, int port,
		std::function<int(my_socket)> task) {
  this->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->fd == -1) {
		printf("Problems with creating socket\n");
	} else {
		this->addr.sin_family = AF_INET;
		inet_aton(ip, &(this->addr).sin_addr);
		addr.sin_port = htons(port);
	  this->task = task;
	}
}

my_socket::~my_socket() {
}

void my_socket::close_socket() {
	close(this->fd);
}

void my_socket::complete_task(my_socket task_arg) {
	if (this->task(task_arg)) {
		printf("Can't complete task.\n");
	}
}

my_socket& my_socket::operator =(const my_socket& other) {
	this->fd = other.fd;
	this->addr = other.addr;
	this->task = other.task;
	return *this;
}

int my_socket::get_fd() const {
	return this->fd;
}

