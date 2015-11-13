#include "my_epoll.h"
#include <signal.h>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <cstring>

struct chat {
private:
	my_epoll new_epoll;
	std::vector<std::pair<std::string, std::string>> messages;
public:
	chat();
	int add_server(char* ip, int port);
	void epoll_run();
	my_epoll& get_epoll();
};
