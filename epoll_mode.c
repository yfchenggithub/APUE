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

/*level trigger and edge trigger 水平触发 和 垂直触发*/

void log_info(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

int set_non_blocking(int fd)
{
	int old_optiton = fcntl(fd, F_GETFL, 0);
	assert(-1 != old_optiton);
	int new_option = old_optiton | O_NONBLOCK;
	int ret = 0;
	ret = fcntl(fd, F_SETFL, new_option);
	assert(-1 != ret);
}

typedef int bool;

/*将文件描述符fd 注册到epollfd指示的epoll内核事件表中， enable_edge_trigger 指示是否启动edge trigger模式*/
void addfd(int epollfd, int fd, bool enable_edge_trigger)
{
	struct epoll_event _event;	
	_event.data.fd = fd;
	_event.events = EPOLLIN;
	if (enable_edge_trigger)
	{
		_event.events |= EPOLLET;
	}	
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &_event);
	set_non_blocking(fd);
}

#define true 1
#define false 0

/*水平触发*/
void lt(struct epoll_event* events, int number, int epollfd, int listenfd)
{
	char buf[BUFFER_SIZE];
	bzero(buf, sizeof(buf));
	
	int i;
	for (i=0; i<number; ++i)
	{
		int _sockfd = events[i].data.fd;
		if (listenfd == _sockfd)
		{
			log_info("new client comming\n");
			struct sockaddr_in _client_addr;
			socklen_t _client_addr_len = sizeof(_client_addr);
			int _connfd = accept(listenfd, (struct sockaddr*)&_client_addr, &_client_addr_len);
			addfd(epollfd, _connfd, false);
		}
		else
		{
			if (events[i].events & EPOLLIN)
			{
				log_info("level trigger\n");
				int ret = recv(_sockfd, buf, sizeof(buf)-1, 0);
				if (ret <= 0)
				{
					close(_sockfd);
					continue;
				}
				log_info("get %d bytes content %s\n", ret, buf);
				send(_sockfd, buf, strlen(buf), 0);
			}
			else
			{
				log_info("something else happen\n");
			}
		}
	}	
}

void et(struct epoll_event* events, int number, int epollfd, int listenfd)
{
	char buf[BUFFER_SIZE];
	bzero(buf, sizeof(buf));	
	
	int i;
	for (i=0; i<number; ++i)
	{
		int sockfd = events[i].data.fd;
		if (listenfd == sockfd)
		{
			struct sockaddr_in _client_addr;
			socklen_t _client_addr_len = sizeof(_client_addr);
			int connfd = accept(listenfd, (struct sockaddr*)&_client_addr, &_client_addr_len);
			addfd(epollfd, connfd, true);
		}
		else
		{
			if (events[i].events & EPOLLIN)
			{
				log_info("edge trigger happen\n");
			
				while (1)
				{
					bzero(buf, sizeof(buf));
					int read_bytes = recv(sockfd, buf, sizeof(BUFFER_SIZE) -1, 0);
					if (read_bytes < 0)
					{
						if ((EWOULDBLOCK == errno) || (EAGAIN == errno))
						{
							break;
						}
						close(sockfd);
						break;
					}
					else
					{
						if (0 == read_bytes)
						{
							close(sockfd);
						}
						else
						{
							
							log_info("get %d bytes content %s\n", read_bytes, buf);
							send(sockfd, buf, strlen(buf), 0);
						}
					}
				}
			}	
		}
	}
}

int main(int argc, char** argv)
{
	log_info("hello epoll\n");
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
	
	struct epoll_event _events[MAX_EVENT_NUMBER];
	int _epollfd = epoll_create(5);
	assert(-1 != _epollfd);
	
	addfd(_epollfd, _lisen_fd, true);	
	
	for (;;)
	{
		int fd_ready_num = epoll_wait(_epollfd, _events, MAX_EVENT_NUMBER, -1);
		if (fd_ready_num < 0)
		{
			log_info("epoll_wait fail\n");	
			perror("epoll_wait");
			break;
		}
		et(_events, fd_ready_num, _epollfd, _lisen_fd);	
	}

	close(_lisen_fd);
	return 0;
}


