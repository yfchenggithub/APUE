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

static int connfd;

void sig_urg(int sig)
{
	int save_errno = errno;
	char buf[BUF_SIZE];
	bzero(buf, sizeof(buf));
	
	int recv_bytes = recv(connfd, buf, sizeof(buf) -1, MSG_OOB);
	log_info("got %d bytes of oob data %s\n", recv_bytes, buf);
	errno = save_errno;
}

void addsig(int sig, void (*sig_handler) (int))
{
	struct sigaction _new_action;
	_new_action.sa_flags = SA_RESTART;
	_new_action.sa_handler = sig_handler;
	sigemptyset(&_new_action.sa_mask);
	assert(-1 != sigaction(sig, &_new_action, NULL));
}

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
	
	int _lisen_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(-1 != _lisen_fd);

	socklen_t _server_addr_len = sizeof(_server_addr);
	int ret = bind(_lisen_fd, (struct sockaddr*)&_server_addr, _server_addr_len);
	assert(-1 != ret);
	
	ret = listen(_lisen_fd, 5);
	assert(-1 != ret);

	struct sockaddr_in _client_addr;
	socklen_t _client_addr_len = sizeof(_client_addr);
	
	 connfd = accept(_lisen_fd, (struct sockaddr*)&_client_addr, &_client_addr_len);
	assert(-1 != connfd);
	
	addsig(SIGURG, sig_urg);
	
	fcntl(connfd, F_SETOWN, getpid());	
	char buf[BUF_SIZE];
	
	for (;;)
	{
		bzero(buf, sizeof(buf));
		int recv_bytes = recv(connfd, buf, BUF_SIZE-1, 0);
		if (recv_bytes <= 0)
		{
			break;
		}	
		log_info("got %d bytes of normal data %s\n", recv_bytes, buf);	
	}
	close(connfd);
	close(_lisen_fd);
	return 0;
}
