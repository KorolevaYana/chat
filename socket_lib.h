#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <functional>
#include <cstdio>

struct client;
struct server;

struct my_socket {
private:
	int fd;
	sockaddr_in addr;
	char* ip;
	int port;
	std::function<int(my_socket)> task;
	std::function<void()> error_func;

public:
	my_socket();
	my_socket(const my_socket& other);
  my_socket(const char* ip, int port, 
			std::function<int(my_socket)> task, std::function<void()> error_func);

	~my_socket();

	void complete_task(my_socket task_arg);
	my_socket& operator =(const my_socket& other);
  
	int get_fd() const;
	std::function<void()> get_error_func() const;
	void close_socket();

	friend client;
	friend server;
};

struct client {
private:
	my_socket heart;

public:
	client();
	client(const my_socket& heart);
  ~client();

	int connect_heart();	
	void complete_task(my_socket task_arg);
	my_socket get_heart();

};



struct server {
private:
	my_socket heart;
	std::function<int(my_socket)> set_task;
	std::function<void()> set_error;
	int max_connected;

public:
	server();
	server(const my_socket& heart, int max_connected,
			std::function<int(my_socket)> set_task, std::function<void()> set_error);
	~server();
	int bind_heart();
	client accept_heart();
	my_socket get_heart();
};

