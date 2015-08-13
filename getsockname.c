#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>

/*nc -l 5555 create a server which listen port 5555*/

int main(int argc, char** argv)
{
	if (3 != argc)
	{
		printf("usage:%s server_ip server_port\n", argv[0]);
		return -1;
	}

        int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		printf("socket fail\n");
		return -1;
	}
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));
	socklen_t addr_len = sizeof(struct sockaddr_in);
	
	if (connect(fd, (struct sockaddr*)&server_addr, addr_len) < 0)
	{
		printf("connect fail\n");
		return -1;
	}
	printf("connect succ\n");
		
	struct sockaddr_in bind_addr;
	socklen_t bind_len = sizeof(bind_addr);
	if (getsockname(fd, (struct sockaddr*)&bind_addr, &bind_len) < 0)
	{
		printf("getsockname fail\n");
		return -1;
	}
	
	char bind_ip[INET_ADDRSTRLEN];
	memset(bind_ip, 0, sizeof(bind_ip));
	if (NULL == inet_ntop(AF_INET, (struct sockaddr*)&bind_addr.sin_addr, bind_ip, INET_ADDRSTRLEN))
	{
		printf("inet_ntop fail\n");
		return -1;
	}	
	printf("bind ip %s bind port %d\n", bind_ip, ntohs(bind_addr.sin_port));
	close(fd);
        return 0;
}
