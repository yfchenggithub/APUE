#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

/*same as printf declaration; return the number of characters printed*/
int info(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int print_bytes = vprintf(format, args);
	va_end(args);
	return print_bytes;
}

int main(int argc, char** argv)
{
	#define MSG "hello vprintf"
	int bytes = info("%s\n", MSG);
	info("the number of characters %d printed\n", bytes);
	exit(EXIT_SUCCESS);
}
