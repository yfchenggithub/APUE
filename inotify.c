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

#define NAME_LEN 100
static void displayInotifyEvent(struct inotify_event* i)
{
	log_info("	wd=%2d; ", i->wd);
	if (i->cookie > 0)
	{
		log_info("cookie %d\n", i->cookie);
	}		

	log_info("mask = ");
	if (i->mask & IN_ACCESS) log_info("IN_ACCESS  \n");
	if (i->mask & IN_ATTRIB) log_info("IN_ATTRIB  \n");
	if (i->mask & IN_CLOSE_NOWRITE) log_info("IN_CLOSE_NOWRITE \n");
	if (i->mask & IN_CLOSE_WRITE) log_info("IN_CLOSE_WRITE \n");
	if (i->mask & IN_CREATE) log_info("IN_CREATE \n");
	if (i->mask & IN_DELETE) log_info("IN_DELETE \n");
	if (i->mask & IN_DELETE_SELF) log_info("IN_DELETE_SELF \n");
	if (i->mask & IN_MODIFY) log_info("IN_MODIFY \n");
	if (i->mask & IN_MOVED_TO) log_info("IN_MOVED_TO \n");
	if (i->mask & IN_MOVED_FROM) log_info("IN_MOVED_FROM \n");
	if (i->mask & IN_MOVE_SELF) log_info("IN_MOVE_SELF \n");

	if (i->len > 0)
	{
		log_info("	name = %s\n", i->name);
	}	
}

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_LEN + 1))

int main(int argc, char** argv)
{
	int inotifyfd = inotify_init();
	assert(inotifyfd > 0);
	
	abort();
	int j;
	int wd;
	for (j=1; j<argc; ++j)
	{
		wd = inotify_add_watch(inotifyfd, argv[j], IN_ALL_EVENTS);		
		assert(wd > 0);
		log_info("watching %s using wd %d\n", argv[j], wd);
	}	
	
	ssize_t num_read;
	char buf[BUF_LEN];
	bzero(buf, sizeof(buf));
	for (;;)
	{
		num_read = read(inotifyfd, buf, BUF_LEN);		
		if (0 == num_read)
		{
			log_info("read from inotify fd return 0\n");
		}	
	
		if (-1 == num_read)
		{
			log_info("read error\n");
			return -1;
		}
		
		char* p;
		struct inotify_event* event;
		for (p=buf; p<buf+num_read;)
		{
			event = (struct inotify_event*)p;
			displayInotifyEvent(event);
			p += sizeof(struct inotify_event) + event->len;
		}
	}		
	return 0;
}
