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

#define MAX_EVENT_NUMBER 1024
static pipefd[2];

int set_non_blocking(int fd)
{
	int old_optiton = fcntl(fd, F_GETFL, 0);
	assert(-1 != old_optiton);
	int new_option = old_optiton | O_NONBLOCK;
	int ret = 0;
	ret = fcntl(fd, F_SETFL, new_option);
	assert(-1 != ret);
	log_info("%d set_non_blocking succ\n", fd);
}

void addfd(int epollfd, int fd)
{
	struct epoll_event _event;
	_event.data.fd = fd;
	_event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &_event);
	set_non_blocking(fd);
	log_info("%d add to epoll fd %d\n", fd, epollfd);
}

void sig_handler(int sig)
{
	log_info("sig %s caught\n", strsignal(sig));
	int _save_errno = errno;
	int _msg = sig;
	send(pipefd[1], (char*)&_msg, 1, 0);
	errno = _save_errno;
}

void add_sig(int sig)
{
	struct sigaction _action;
	bzero(&_action, sizeof(_action));
	_action.sa_handler = sig_handler;
	_action.sa_flags |= SA_RESTART;
	sigemptyset(&_action.sa_mask);
	assert(-1 != sigaction(sig, &_action, NULL));
	log_info("add %s succ\n", strsignal(sig));
}

int main(int argc, char** argv)
{
	if (3 != argc)
	{
		log_info("usage: %s ip port \n", argv[0]);
		return -1;
	}
	
	const char* _ip = argv[1];
	const int _port = atoi(argv[2]);
	
	struct sockaddr_in _server;
	_server.sin_family = AF_INET;
	_server.sin_port = htons(_port);	
	inet_pton(AF_INET, _ip, &_server.sin_addr);
	
	int _lisfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(-1 != _lisfd);
	
	socklen_t _server_len = sizeof(_server);
	int ret = bind(_lisfd, (struct sockaddr*)&_server, _server_len);
	assert(-1 != ret);
	
	ret = listen(_lisfd, 5);
	assert(-1 != ret);
	
	struct epoll_event _events[MAX_EVENT_NUMBER];
	int _epofd = epoll_create(5);
	assert(-1 != _epofd);
	
	addfd(_epofd, _lisfd);
	
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);
	assert(-1 != ret);
	
	set_non_blocking(pipefd[1]);
	addfd(_epofd, pipefd[0]);
	
	add_sig(SIGHUP);
	add_sig(SIGCHLD);
	add_sig(SIGTERM);
	add_sig(SIGINT);
	
	typedef int bool;
	#define false 0
	#define true 1
	bool stop_server = false;
	
	while (!stop_server)
	{
		int _number = epoll_wait(_epofd, _events, MAX_EVENT_NUMBER, -1);
		if ((_number < 0) && (EINTR != errno))
		{
			log_info("epoll_wait fail, errno %s\n", strerror(errno));
			break;
		}
		
		int i;
		for (i=0; i<_number; ++i)
		{
			int _sockfd = _events[i].data.fd;
			if (_lisfd == _sockfd)
			{
				struct sockaddr_in _client_addr;
				socklen_t _client_addr_len = sizeof(_client_addr);
				int _connfd = accept(_lisfd, (struct sockaddr*)&_client_addr, &_client_addr_len);
				addfd(_epofd, _connfd);	
			}
			
			if ((pipefd[0] == _sockfd) && (_events[i].events & EPOLLIN))
			{
				int sig;
				char _signals[1024];
				int ret = recv(pipefd[0], _signals, sizeof(_signals), 0);
				if (ret <= 0)
				{
					continue;
				}
				else
				{
					int i;
					for (i=0; i<ret; ++i)
					{
						switch (_signals[i])
						{
							case SIGCHLD:
							case SIGHUP:continue;
							case SIGTERM:
							case SIGINT: stop_server = true;
									
						}
					}
				}
			}
		}
	}	
	log_info("close fds\n");
	close(_lisfd);
	close(pipefd[0]);
	close(pipefd[1]);
	return 0;
}

