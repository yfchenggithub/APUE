#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_key_t new_key;

void* thrd1_run(void* arg)
{
	printf("thrd1\n");
	int specific_data = atoi((char*)arg);
	if (pthread_setspecific(new_key, (const void*)specific_data) < 0)
	{
		printf("pthread_setspecific fail\n");
		return (void*)-1;
	}
	printf("thread id %lu value %d\n", pthread_self(), (int)pthread_getspecific(new_key));
}

int main(int argc, char** argv)
{
	if (pthread_key_create(&new_key, NULL) < 0)
	{
		printf("pthread_key_create fail\n");
		return -1;
	}
	
	if (2 != argc)
	{
		printf("usage: %s specific_data\n", argv[0]);
		return -1;
	}

	pthread_t thrd1;
	if (pthread_create(&thrd1, NULL, thrd1_run, argv[1]) < 0)
	{
		printf("pthread_create fail\n");
		return -1;
	}	
	pthread_join(thrd1, NULL);	
	return 0;
}
