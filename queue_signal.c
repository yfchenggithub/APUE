#define _GNU_SOURCE
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
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <setjmp.h>
#include <assert.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <ftw.h>
#include <limits.h>
#include <libgen.h>
#include <signal.h>
#include <sys/inotify.h>

void log_info(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);	
}

#define MAX 1000
pthread_mutex_t the_mutex;
pthread_cond_t condc, condp;
int buffer = 0;

void* producer(void* ptr)
{
	int i;
	for (i=0; i<=MAX; ++i)
	{
		pthread_mutex_lock(&the_mutex);
		while (0 != buffer)
		{
			pthread_cond_wait(&condp, &the_mutex);
		}
		buffer = i;
		log_info("%ld buffer %d\n", pthread_self(), buffer);
		pthread_cond_signal(&condc);
		pthread_mutex_unlock(&the_mutex);
	}	
	pthread_exit(0);	
}

void* consumer(void* ptr)
{
	int i;
	for (i=0; i<MAX; ++i)
	{
		pthread_mutex_lock(&the_mutex);
		while (0 == buffer)
		{
			pthread_cond_wait(&condc, &the_mutex);
		}
		buffer = 0;
		log_info("%ld buffer %d\n", pthread_self(), buffer);
		pthread_cond_signal(&condp);
		pthread_mutex_unlock(&the_mutex);			
	}
	pthread_exit(0);
}

int main(int argc, char** argv)
{
	int sig;
	int numSigs;
	int j;
	int sigData;
	
	union sigval sv;
	if (5 != argc)
	{
		log_info("usage:%s pid sig-num data [num-sigs]\n", argv[0]);
		return -1;
	}
	
	log_info("%s: pid is %ld, uid is %ld\n", argv[0], getpid(), getuid());
	sig = atoi(argv[2]);
	sigData = atoi(argv[3]);
	numSigs = atoi(argv[4]);
	pid_t pid = atoi(argv[1]);
	for (j=0; j<numSigs; ++j)
	{
		sv.sival_int = sigData + 1;
		if (sigqueue(pid, sig, sv) < 0)
		{
			log_info("sigqueue fail\n");
			return -1;
		}	
	}	
	return 0;
}
