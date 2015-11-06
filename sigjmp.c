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
#include <setjmp.h>
#include <sys/inotify.h>

void log_info(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);	
}

void printSigset(FILE* of, const char* prefix, const sigset_t* sigset)
{
	int sig = 0;
	int cnt = 0;
	for (sig=1; sig<NSIG; ++sig)
	{
		if (sigismember(sigset, sig))
		{
			++cnt;
			fprintf(of, "%s %d (%s)\n", prefix, sig, strsignal(sig));
		}
	}	
	
	if (0 == cnt)
	{
		fprintf(of, "%s empty signal set\n", prefix);
	}
}

int printSigMask(FILE* of, const char* msg)
{
	sigset_t currMask;
	if (NULL == msg)
	{
		return -1;
	}		
	else
	{
		fprintf(of, "%s", msg);
	}

	if (sigprocmask(SIG_BLOCK, NULL, &currMask) < 0)
	{
		log_info("sigprocmask fail\n");
		return -1;
	}
	
	printSigset(of, "\t\t", &currMask);
	return 0;
}

int printPendingSigs(FILE* of, const char* msg)
{
	sigset_t pendingSigs;
	if (NULL != msg)
	{
		fprintf(of, "%s", msg);
	}	
	
	if (sigpending(&pendingSigs) < 0)
	{
		log_info("sigpending fail\n");
		return -1;
	}	
	
	printSigset(of, "\t\t", &pendingSigs);
	return 0;
}

int blocksig(int sig)
{
	sigset_t _set;
	sigemptyset(&_set);
	sigaddset(&_set, sig);
	if (sigprocmask(SIG_BLOCK, &_set, NULL) < 0)
	{
		log_info("block %d fail\n");
		return -1;
	}	
	return 0;
}

static volatile sig_atomic_t can_jump = 0;

static sigjmp_buf env;

static void handler(int sig)
{
	log_info("receive signal %d (%s), signal mask is :\n", sig, strsignal(sig));
	printSigMask(stdout, "in the signal handler mask is: \n");	
	if (!can_jump)
	{
		log_info("env buffer not yet set, doing a simple return\n");
		return;
	}
	siglongjmp(env, 1);
}

int main(int argc, char** argv)
{
	struct sigaction _cur_action;
	printSigMask(stdout, "signal mask at begin:\n");
	
	sigemptyset(&_cur_action.sa_mask);
	_cur_action.sa_handler = handler;
	_cur_action.sa_flags = 0;

	if (sigaction(SIGINT, &_cur_action, NULL) < 0)
	{
		log_info("sigaction fail\n");
		return -1;
	}
	
	log_info("calling setjmp()\n");	
	if (0 == sigsetjmp(env, 1))
	{
		can_jump = 1;
	}
	else
	{
		printSigMask(stdout, "after jump from handler; signal mask is: \n");	
	}

	for (;;)
	{
		pause();
	}
	return 0;
}
