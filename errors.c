#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

int debug;/* = 0;*/
#define unlikely(x)     __builtin_expect((x),0)

void __attribute__((format(printf,5,6)))
perror_at_line(int status, int errnum, const char *fname,
	unsigned int linenum, const char *format, ...)
{
	va_list vl;
	va_start(vl,format);

	fflush(stdout);
	fprintf(stderr,"%s:%d ",fname,linenum);
	if (errnum)
		fprintf(stderr,"[%s] : ",strerror(errnum));
	else
		fprintf(stderr," : ");
	vfprintf(stderr,format,vl);
	fputc('\n',stderr);
	fflush(stderr);
	if (status)
		exit(status);
}
