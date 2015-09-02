#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>

int g_interrupt_flag = 0;

void sig_int_handle(int sig)
{
	printf("recv sig %d\n", sig);
	g_interrupt_flag = 1;	
}

int main(int argc, char** argv)
{
	struct sigaction new_action;
	struct sigaction old_action;
	sigset_t zero_mask;
	sigemptyset(&zero_mask);
	sigemptyset(&new_action.sa_mask);
	sigaddset(&new_action.sa_mask, SIGINT);
	new_action.sa_handler = sig_int_handle;
	new_action.sa_flags = 0;
	sigaction(SIGINT, &new_action, &old_action);
	int lis_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (lis_fd < 0)
	{
		perror("socket create fail");
		return -1;
	}
	
	/*监听localhost:5555*/	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(5555);
	socklen_t server_addr_len = sizeof(struct sockaddr_in);
	
	if (bind(lis_fd, (struct sockaddr*)&server_addr, server_addr_len) < 0)
	{
		perror("bind fail");
		close(lis_fd);
		return -1;
	}
	
	/*允许有5个客户同时连接*/
	if (listen(lis_fd, 5) < 0)
	{	
		perror("listen fail");
		close(lis_fd);
		return -1;
	}
	
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(lis_fd, &read_set);	
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(struct sockaddr_in);
	int maxfd = lis_fd + 1;
	
	int client_set[FD_SETSIZE];
	int i;
	/*已经连接的文件描述符，需要保存， -1表示这个位置可用*/
	for (i=0; i<FD_SETSIZE; ++i)
	{
		client_set[i] = -1;
	}
	
	for(;;)
	{
		struct timeval select_timeout;
		select_timeout.tv_sec = 3;
		select_timeout.tv_usec = 30;
		struct timespec pselect_timeout;
		pselect_timeout.tv_sec = 10;
		pselect_timeout.tv_nsec = 30;
		/*等待有文件描述符可以进行IO操作*/
		int num_fd = pselect(maxfd, &read_set, NULL, NULL, &pselect_timeout, &new_action.sa_mask);
		if (0 == num_fd)
		{
			printf("timeout, continue\n");
			continue;
		}

		if (-1 == num_fd)
		{
			if (errno == EINTR)
			{
				printf("select EINTR happen\n");
			}
			continue;
		}
		
		if (FD_ISSET(lis_fd, &read_set))
		{
			printf("new client connect\n");
			int new_client = accept(lis_fd, (struct sockaddr*)&client_addr, &client_addr_len);
			if (new_client < 0)
			{
				continue;
			}	

			FD_SET(new_client, &read_set);
			if (new_client > maxfd)
			{
				maxfd = new_client;
			}
			/*找一个空位置 把已连接描述符进行纪录*/
			int j = 0;
			while (j < FD_SETSIZE)
			{
				if (-1 == client_set[j])
				{
					client_set[j] = new_client;
					break;
				}
				++j;
			}

			if (j == FD_SETSIZE)
			{
				printf("can not accept more client\n");
				close(new_client);
			}
		}
		
		int count = 0;
		for (; count<maxfd; ++count)
		{
			if (-1 == client_set[count])
			{
				continue;
			}
		
			if (FD_ISSET(client_set[count], &read_set))
			{
				#define MAX_SIZE 1024
				char buf[MAX_SIZE];
				memset(buf, 0, sizeof(buf));
				int read_bytes = read(client_set[count], buf, MAX_SIZE);
				if (0 == read_bytes)
				{
					close(client_set[count]);
					FD_CLR(client_set[count], &read_set);
					client_set[count] = -1;
				}
				
				if (0 < read_bytes)
				{
					printf("buf %s\n", buf);
					write(client_set[count], buf, strlen(buf));
				}
				else
				{
					perror("read fail");
				}
				
			}
		}
	}	

	return 0;
}
