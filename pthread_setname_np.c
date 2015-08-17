#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
pthread_setname_np
set the pthread name; 16 bytes include the null 0
pthread_getname_np
*/

int main(int argc, char** argv)
{
	pthread_t pid = pthread_self(); 
	#define NAME_SIZE 16
	char name[NAME_SIZE];
	memset(name, 0, sizeof(name));
	
	if (0 != pthread_getname_np(pid, name, NAME_SIZE))
	{
		printf("pthread_getname_np fail\n");
		return -1;
	}
	printf("pthread name: %s\n", name);
	
	if (2 != argc)
	{
		printf("usage: %s set_pthread_name\n", argv[0]);
		return -1;
	}
	
	#define MAX_NAME_LEN 15
	if (strlen(argv[1]) > MAX_NAME_LEN)
	{
		printf("MAX_NAME_LEN %d\n", MAX_NAME_LEN);
		return -1;
	}

	if (0 != pthread_setname_np(pid, argv[1]))
	{
		printf("pthread_setname_np fail\n");
		return -1;
	}

	memset(name, 0, sizeof(name));
	if (0 != pthread_getname_np(pid, name, NAME_SIZE))	
	{
		printf("pthread_getname_np  fail\n");
		return -1;
	}
	printf("pthread name: %s\n", name);
	return 0;
}
