#include "socket_lib.h"

#include <sys/epoll.h>
#include <functional>
#include <map>

struct my_epoll {
private:
	std::map<int, client> client_sockets;
	std::map<int, server> server_sockets;

	int cur_events;
	int fd;
	std::function<void()> error_func;
	int magic_const;

public:
  my_epoll();  
  my_epoll(std::function<void()> error_func);
	~my_epoll();

	int add_client(client item, int flag);
	int add_server(server item);
	int delete_client(client tmp);
	void run();
	void set_magic_const(int val);
};
