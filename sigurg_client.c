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
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

void log_info(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

#define BUF_SIZE 1024

int main(int argc, char** argv)
{
	if (3 != argc)
	{
		log_info("usage: %s ip port\n", argv[0]);
		return -1;
	}	
	
	const char* _ip = argv[1];
	const int _port = atoi(argv[2]);
	log_info("_ip %s _port %d\n", _ip, _port);
	
	struct sockaddr_in _server_addr;
	_server_addr.sin_family = AF_INET;
	_server_addr.sin_port = htons(_port);
	inet_pton(AF_INET, _ip, &_server_addr.sin_addr);
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(-1 != fd);

	if (connect(fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) < 0)
	{
		log_info("connect server fail\n");
	}	
	else
	{
		const char* _oob_data = "abcdefg";
		const char* _normal_data = "123456789";
		send(fd, _normal_data, strlen(_normal_data), 0);		
		send(fd, _oob_data, strlen(_oob_data), MSG_OOB);
		send(fd, _normal_data, strlen(_normal_data), 0);		
	}
	
	close(fd);
	return 0;
}
