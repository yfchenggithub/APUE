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

static volatile int handlerSleepTime;
static volatile int sigCnt = 0;
static volatile int allDone = 0;

static void siginfoHandler(int sig, siginfo_t* si, void* ucontext)
{
	if ((SIGINT == sig) || (SIGTERM == sig))	
	{
		allDone = 1;
		return;
	}
	
	sigCnt++;
	log_info("caught signal %d\n", sig);
	log_info("si_signo %d\n", si->si_signo);
	log_info("si_code %d\n", si->si_code);
	log_info("si_value %d\n", si->si_value.sival_int);
	log_info("pid %ld uid %ld\n", getpid(), getuid());
	sleep(handlerSleepTime);
}

int main(int argc, char** argv)
{
	struct sigaction sa;
	int sig;
	sigset_t prevMask, blockMask;
	
	if (3 != argc)
	{
		log_info("usage:%s blocktime, handler-sleep-time\n", argv[0]);
		return -1;
	}				
	
	log_info("%s: pid is %ld\n", argv[0], getpid());	
	handlerSleepTime = atoi(argv[2]);
	
	sa.sa_sigaction = siginfoHandler;
	sa.sa_flags = SA_SIGINFO;
	sigfillset(&sa.sa_mask);
	
	for (sig=1; sig<NSIG; ++sig)
	{
		if (sig != SIGTSTP && sig != SIGQUIT)
		{
			sigaction(sig, &sa, NULL);	
		}
	}
	
	sigfillset(&blockMask);
	sigdelset(&blockMask, SIGINT);
	sigdelset(&blockMask, SIGTERM);
	
	if (sigprocmask(SIG_SETMASK, &blockMask, &prevMask) < 0)
	{
		log_info("sigprocmask fail\n");
		perror("sigprocmask");
		return -1;
	}				
	
	log_info("%s: signals blocked -sleeping %s seconds\n", argv[0], argv[1]);
	int blocktime = atoi(argv[2]);
	sleep(blocktime);
	
	log_info("%s: sleep complete\n", argv[0]);
	if (sigprocmask(SIG_SETMASK, &prevMask, NULL) < 0)
	{
		log_info("sigprocmask fail\n");
		return -1;
	}	
	
	while (!allDone)
	{
		pause();
	}
	return 0;
}
