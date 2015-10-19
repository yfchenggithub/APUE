#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#define BUFFER_SIZE 1024
static const char* status_line[2] = {"200 ok", "500 internal server error"};
typedef int bool;

void log_info(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

int main(int argc, char** argv)
{
	if (4 != argc)
	{
		log_info("usage: %s ip port file\n", argv[0]);
		return -1;
	}
	
	const char* _ip = argv[1];
	const int _port = atoi(argv[2])	;
	const char* _filename = argv[3];
	
	struct sockaddr_in _server;
	_server.sin_family = AF_INET;
	_server.sin_port = htons(_port);
	inet_pton(AF_INET, _ip, &_server.sin_addr);
	
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	socklen_t _addrlen = sizeof(_server);
	bind(listen_fd, (struct sockaddr*)&_server, _addrlen);
	listen(listen_fd, 5);

	struct sockaddr_in _client_addr;
	socklen_t _clinet_addr_len = sizeof(_client_addr);	
	
	int conn_fd = accept(listen_fd, (struct sockaddr*)&_client_addr, &_clinet_addr_len);
	if (conn_fd < 0)
	{
		log_info("accept fail\n");
	}
	else
	{
		char header_buf[BUFFER_SIZE];
		bzero(header_buf, sizeof(header_buf));
		char* file_buf;
		struct stat _file_stat;
		bool valid = 1;
		if (stat(_filename, &_file_stat) < 0)
		{
			log_info("_filename %s not exist\n", _filename);
			valid = 0;
		}	
		else
		{
			if (S_ISDIR(_file_stat.st_mode))
			{
				valid = 0;
			}
			else
			{
				if (_file_stat.st_mode & S_IROTH)
				{
					int fd = open(_filename, O_RDONLY);
					file_buf = (char*)malloc(_file_stat.st_size + 1);
					bzero(file_buf, _file_stat.st_size + 1);
					if (read(fd, file_buf, _file_stat.st_size) < 0)
					{
						valid = 0;
					}
				}
				else
				{
					valid = 0;
				}
			}
		}
		
		if (valid)
		{
			int len = 0;
			int ret = snprintf(header_buf, BUFFER_SIZE-1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
			len += ret;
			ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "Content-Length: %d\r\n", _file_stat.st_size);
			len += ret;
			ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "%s", "\r\n");
			struct iovec _iv[2];
			_iv[0].iov_base = header_buf;
			_iv[0].iov_len = strlen(header_buf);
			_iv[1].iov_base = file_buf;
			_iv[1].iov_len = _file_stat.st_size;
			ret = writev(conn_fd, _iv, 2);
		}
		else
		{
			int ret = snprintf(header_buf, BUFFER_SIZE-1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
			int len = 0;
			len += ret;
			ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "%s", "\r\n");
			send(conn_fd, header_buf, strlen(header_buf), 0);
		}
		
		close(conn_fd);
		free(file_buf);
	}
	
	close(listen_fd);
	return 0;
}
