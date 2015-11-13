#include "chat.h"

chat meow;

void catch_signal(int sig_num) {
	printf("Signal number: %d\n", sig_num);
	meow.get_epoll().set_magic_const(1);
}

int main() {
	signal(SIGINT, catch_signal);
  if (meow.add_server("127.0.0.1", 40000)) {
		return 0;
	}
	meow.epoll_run();
	return 0;
}
