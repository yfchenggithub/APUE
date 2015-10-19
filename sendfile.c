#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <pthread.h>
#include <fcntl.h>


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
		exit(0);
	}
	
	const char* _ip = argv[1];
	const int _port = atoi(argv[2]);
	const char* _file_name = argv[3];
	int _filefd = open(_file_name, O_RDONLY);
	struct stat _file_stat;
	if (fstat(_filefd, &_file_stat) < 0)
	{
		log_info("fstat fail\n");
		perror("fstat");
	}

	struct sockaddr_in _server;
	_server.sin_family = AF_INET;
	_server.sin_port = htons(_port);
	inet_pton(AF_INET, _ip, &_server.sin_addr);
	
	int lis_fd = socket(AF_INET, SOCK_STREAM, 0);
	socklen_t _server_addr_len = sizeof(_server);
	bind(lis_fd, (struct sockaddr*)&_server, _server_addr_len);
	listen(lis_fd, 5);
	
	int conn_fd = accept(lis_fd, NULL, NULL);
	log_info("conn_fd %d\n", conn_fd);
	if (conn_fd < 0)
	{
		log_info("accept fail\n");	
	}
	else
	{
		int send_bytes = sendfile(conn_fd, _filefd, 0, _file_stat.st_size);
		log_info("send_bytes %d\n", send_bytes);
		close(conn_fd);			
	}
	
	close(lis_fd);
	return 0;
}
