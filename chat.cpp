#include "chat.h"

int get_file_size(FILE *input);
int write_ans(std::string& w_buffer, int content_length);
int write_file(std::string& w_buffer, FILE* request, int file_size);
int send_all(const char* w_buffer, my_socket tmp, size_t buffer_size);
int add_symb(bool text, bool user, std::string &s1, std::string &s2, char symb);
std::pair<std::string, std::string> parse_message(char* s, int shift);

chat::chat() {
}

my_epoll& chat::get_epoll() {
	return this->new_epoll;
}

void chat::epoll_run() {
	new_epoll.run();
}

int chat::add_server(char* ip, int port) {
	my_socket meow(ip, port, [](my_socket smth){
			  printf("Accepted!\n");
				return 0;
			});

	server reading(meow, 10, [this](my_socket tmp) {
				char buffer[1024];
				std::string r_buffer;
				ssize_t tmp_read;
				while((tmp_read = read(tmp.get_fd(), buffer, 1024)) != 0) {
					if (tmp_read < 0) {
					  if (errno != EAGAIN) {
						  printf("Reading problems: %m\n");
							return 1;
						}
					} else {
					  for (int i = 0; i < (int)tmp_read; i++) {
						  r_buffer.push_back(buffer[i]);
						}
					}
			
					int tmp_size = (int)r_buffer.size();
					if (tmp_size >= 4) {
					 	if (r_buffer[tmp_size - 1] == r_buffer[tmp_size - 3] && r_buffer[tmp_size - 1] == '\n') {
					  	if (r_buffer[tmp_size - 2] == r_buffer[tmp_size - 4] && r_buffer[tmp_size - 2] == '\r') {
					  	 	break;
							}
						}
					}
				}

				if (tmp_read == -1) {
				  printf("Reading problems: %m\n");
					return 1;
				}

  			char file_name[1000];
				if (sscanf(r_buffer.data(), "GET %s HTTP/1.1\r\n", file_name) < 0) {
				 	printf("Wrong http request.\n");
					return 1;
				}

				std::string w_buffer;

				if (!strcmp(file_name, "/")) {
				  strcpy(file_name, "lal.html");

					FILE *request = fopen(file_name, "r");
					if (request == NULL) {
					  printf("problems with opening file %s", file_name);
						return 1;
					}

					int file_size = get_file_size(request);
			  	if (write_file(w_buffer, request, file_size)) {
						printf("Writing problems: %m\n");
					  fclose(request);
						return 1;
					}
					fclose(request);
				} else if (!strncmp(file_name, "/send", 5)) {
					messages.push_back(parse_message(file_name, 5));
					
					if (!write_ans(w_buffer, 0)) {
					  printf("Writing problems: %m\n");
					  return 1;
					}
				}	else if (!strcmp(file_name, "/chat")) {
					std::string json;
					json += "{messages:[";
					for (int i = std::max((int)messages.size() - 80, 0); i < (int)messages.size(); i++) {
						json += "{user:\"" + messages[i].first + "\",text:\"" + messages[i].second + "\"}";
						if (i != (int)messages.size() - 1) {
							json += ",";
						}
					}
				  json+="]}";
				
					if(!write_ans(w_buffer, json.length())) {
						printf("Writing problems: %m\n");
						return 1;
					}
					w_buffer += json;
				} else {
				  printf("Wrong file requested.\n");
					return 1;
				}
				
				if (send_all(w_buffer.c_str(), tmp, (int)w_buffer.size())) {
					printf("Sending problems: %m\n");
					return 1;
				}				
 				return 0;
			});

  if (reading.bind_heart()) {
		return 1;
	}

  new_epoll.add_server(reading);
	return 0;
}

//================================================================

int get_file_size(FILE *input) {
	fseek(input, 0, SEEK_END);
	int bytes = ftell(input);
	fseek(input, 0, SEEK_SET);
	return bytes;
}

int write_ans(std::string& w_buffer, int content_length) {
	 char tmp[100];
   int res = sprintf(tmp,
	  			"HTTP/1.1 200 OK \r\n"
					"Content-Length: %d \r\n"
					"Connection: close \r\n"
					"Cache-Control: no-cache,no-store \r\n"
					"\r\n", content_length);
	 if (res) {
		 w_buffer += tmp;
	 }
	 return res;
}

int write_file(std::string& w_buffer, FILE* request, int file_size) {
	if (!write_ans(w_buffer, file_size)) {
		printf("Writing problems: %m\n");
		return 1;
	}
	char c;
	while (fscanf(request, "%c", &c) != EOF) {
		w_buffer.push_back(c);
	}
	w_buffer += "\r\n";
  return 0;
}

int send_all(const char* w_buffer, my_socket tmp, size_t buffer_size) {
	ssize_t tmp_send;
	size_t tmp_shift = 0;
	while ((tmp_send = send(tmp.get_fd(), w_buffer + tmp_shift, 
													buffer_size - tmp_shift, MSG_NOSIGNAL)) > 0) {
		tmp_shift += tmp_send;
	}
	if (tmp_send < 0) {
		return 1;
	}
	return 0;
}

int add_symb(bool text, bool user, std::string &s1, std::string &s2, char symb) {
	std::string tmp;
	if (symb == '"') {
		tmp = "\\\"";
	} else if (symb == '\n') {
		tmp = "\\n";
	} else if (symb == '\\') {
		tmp = "\\\\";
	} else {
		tmp = symb;
	}

	if (text & (text ^ user)) {
		s2 += tmp;
	} else if (user & (text ^ user)) {
		s1 += tmp;
	} else {
		return 1;
	}
	return 0;
}

std::pair<std::string, std::string> parse_message(char* s, int shift) {

	std::function<std::pair<std::string, std::string>()> error_func = []() {
		printf("Wrong message request.\n");
 		return std::make_pair("", "");
	};

	std::pair<std::string, std::string> ans;
	bool text = 0, user = 0;
	while (*(s + shift) != 0) {
		char c = *(s + shift);	
		if (c == '?') {
			if (!strncmp(s + shift + 1, "user=", 5) && user == 0 && text == 0) {
				user = 1;
				text = 0;
				shift += 6;
			} else {
				return error_func();
			}
		} else if (c == '&') {
			if (!strncmp(s + shift + 1, "text=", 5) && user == 1 && text == 0) {
				user = 0;
				text = 1;
				shift += 6;
			} else {
				return error_func();
			}
		}	else if (c == '%') {
			int symb;
			if (sscanf(s + shift + 1, "%2x", &symb) == 0) {
				return error_func();
			}
			shift += 3;
			if ((s + shift - 1) == 0) {
				return error_func();
			}
  		if (add_symb(text, user, ans.first, ans.second, symb)) {
				return error_func();
			}
		} else {
			if (add_symb(text, user, ans.first, ans.second, c)) {
				return error_func();
			}
			shift++;
		}
	}
	return ans;	
}
