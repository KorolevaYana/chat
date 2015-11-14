#include "my_epoll.h" 

my_epoll::my_epoll() {
	this->fd = epoll_create(10);
	if (this->fd < 0) {
		printf("Can't create epoll socket: %m\n");
	} else {
		this->cur_events = 0;
		this->magic_const = 0;
	}
}

my_epoll::~my_epoll() {
	close(this->fd);
}
 
int my_epoll::add_client(client item, int flag) {
	struct epoll_event ev;

	ev.events = EPOLLRDHUP | EPOLLHUP;
	if (flag == 0) {
		ev.events |= EPOLLIN;
	} else if (flag == 1) {
		ev.events |= EPOLLOUT;
	} else {
		printf("Invalid flag.\n");
		return 1;
	}
	
	int item_fd = item.get_heart().get_fd();

	ev.data.u64 = 0;
	ev.data.fd = item_fd;

	if (epoll_ctl(this->fd, EPOLL_CTL_ADD, item_fd, &ev) == -1) {
		printf("Can't add client. %m\n");
		return 1;
	}
	client_sockets[item_fd] = item;
	this->cur_events++;
	return 0;
}

int my_epoll::add_server(server item) {

	struct epoll_event ev;
	ev.events = EPOLLIN;

	int item_fd = item.get_heart().get_fd();

	ev.data.u64 = 0;
	ev.data.fd = item_fd;
	if (epoll_ctl(this->fd, EPOLL_CTL_ADD, item_fd, &ev) == -1) {
		printf("Can't add server. %m\n");
		return 1;
	}
	server_sockets[item_fd] = item;
	this->cur_events++;
	return 0;
}

int my_epoll::delete_client(client item) {
	if (epoll_ctl(this->fd, EPOLL_CTL_DEL, item.get_heart().get_fd(), 0) == -1) {
		printf("Can't delete client. %m\n");
		return 1;
	}
	client_sockets.erase(item.get_heart().get_fd());
	close(item.get_heart().get_fd());
	return 0;
}

void my_epoll::set_magic_const(int val) {
	this->magic_const = val;
}

void my_epoll::run() { 
	struct epoll_event events[100];
  while (1) {
		int cnt = epoll_wait(fd, events, 100, -1);
		if (this->magic_const) {
	  	for (auto i = client_sockets.begin(); i != client_sockets.end(); i++) {
				close((*i).first);
			}
		  for (auto i = server_sockets.begin(); i != server_sockets.end(); i++) {
				close((*i).first);
			}
			
			server_sockets.clear();
			client_sockets.clear();
			break;
		}
		if (cnt == -1) { 
			if (errno != EINTR) {
				printf("Problems with epoll_wait: %m");
				return;
			} else {
				continue;
			}
		}

		for (int i = 0; i < cnt; i++) {
			int tmp_fd = events[i].data.fd;
			int flag = events[i].events;
			if (server_sockets.find(tmp_fd) != server_sockets.end()) {
				server tmp = server_sockets[tmp_fd];
				if(add_client(tmp.accept_heart(), 0)) {
					return;
				} 
			} else {
				client tmp = client_sockets[tmp_fd];
				
				if (flag & EPOLLRDHUP || flag & EPOLLHUP) {
					printf("Disconnected.\n");
					if (delete_client(tmp)) {
						return;
					}
				} else {
					tmp.complete_task(tmp.get_heart());
					if (delete_client(tmp)) {
						return;
					}
				}
			}
		}

	}
}


