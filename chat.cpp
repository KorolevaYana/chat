#include "chat.h"

int get_file_size(FILE *input) {
	fseek(input, 0, SEEK_END);
	int bytes = ftell(input);
	fseek(input, 0, SEEK_SET);
	return bytes;
}

int write_ans(char* w_buffer, int content_length) {
	return sprintf(w_buffer,
	  			"HTTP/1.1 200 OK \r\n"
					"Content-Length: %d \r\n"
					"Connection: close \r\n"
					"Cache-Control: no-cache,no-store \r\n"
					"\r\n", content_length);
}

int write_file(char* w_buffer, FILE* request, int file_size) {
	int shift = write_ans(w_buffer, file_size);
	char c;
	while (fscanf(request, "%c", &c) != EOF) {
		sprintf(w_buffer + shift++, "%c", c);
	}
  return 0;
}

int send_all(char* w_buffer, my_socket tmp, int buffer_size) {
	int tmp_send;
	int tmp_shift = 0;
	while ((tmp_send = send(tmp.get_fd(), w_buffer + tmp_shift, 
													buffer_size - tmp_shift, MSG_NOSIGNAL)) > 0) {
		tmp_shift += tmp_send;
	}
	if (tmp_send < 0) {
		printf("Sending problems.\n");
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
//		printf("%c\n", c);
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
				printf("kek");
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
			}, [](){printf("Problems with accepting!\n");});

	server reading(meow, 10, [this](my_socket tmp) {
				char buffer[1024];
				int tmp_read;
				if ((tmp_read = recv(tmp.get_fd(), buffer, 1024, 0)) > 0) {
				  char file_name[1000];
					if (sscanf(buffer, "GET %s HTTP/1.1\r\n", file_name) < 0) {
					 	printf("Wrong http request.\n");
						return 1;
					}
					
					char* w_buffer;
					int buffer_size = 200;
					if (!strcmp(file_name, "/")) {
					  strcpy(file_name, "lal.html");

						FILE *request = fopen(file_name, "r");
						if (request == NULL) {
						  printf("problems with opening file %s", file_name);
							return 1;
						}
						int file_size = get_file_size(request);
						buffer_size += file_size;
  					w_buffer = new char[buffer_size];
	
						if (write_file(w_buffer, request, file_size)) {
							printf("Write problems(if)\n");
							fclose(request);
							return 1;
						}
						fclose(request);
					} else if (!strncmp(file_name, "/send", 5)) {
			//			printf("%s\n", file_name);
						messages.push_back(parse_message(file_name, 5));
						
						w_buffer = new char[buffer_size];
						if (!write_ans(w_buffer, 0)) {
						  printf("Write problems(else)\n");
						  return 1;
						}
					}	else if (!strcmp(file_name, "/chat")) {
						std::string json;
						json += "{messages:[";
						for (int i = std::max((int)messages.size() - 80, 0); i < (int)messages.size() - 1; i++) {
							json += "{user:\"" + messages[i].first + "\",text:\"" + messages[i].second + "\"},";
						}
						if (messages.size() > 0) {
							int i = messages.size() - 1;
							json += "{user:\"" + messages[i].first + "\",text:\"" + messages[i].second + "\"}";
						}
						json+="]}";
						buffer_size += json.length();
						w_buffer = new char[buffer_size];
						int shift;

						if ((shift = write_ans(w_buffer, json.length())) == 0) {
							printf("Write problems(else if)\n");
							return 1;
						}
						memcpy(w_buffer + shift, json.c_str(), json.length());
						shift += json.length();
						w_buffer[shift + 1] = 0;

					} else {
					  printf("Wrong file requested.\n");
						return 1;
					}

//					printf("%s\n", w_buffer);
					if (send_all(w_buffer, tmp, buffer_size)) {
						printf("Sending problems\n");
						delete[] w_buffer;
						return 1;
					}
					delete[] w_buffer;
					
				} else if (tmp_read < 0) {
				  tmp.get_error_func()();
					return 1;
				} 
				return 0;
			}, [](){printf("Problems with reading: %m\n");});

  if(reading.bind_heart()) {
		return 1;
	}

  new_epoll.add_server(reading);
	return 0;
}
