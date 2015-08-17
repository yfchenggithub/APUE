#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


/*
getrlimit process can open 1024 descriptor
*/
int main(int argc, char** argv)
{
	int max_fd = getdtablesize();
	printf("max_fd %d\n", max_fd);
	return 0;
}
