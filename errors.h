#ifndef ERRORS_H_
#define ERRORS_H_

#define INFO(...) do {                                                     \
		if (unlikely(debug)) {                                     \
			fprintf(stderr,"INFO: ");                          \
			perror_at_line(0,0,__func__,__LINE__,__VA_ARGS__); \
		}                                                          \
	} while(0)

#define WARN(_exitnum_,_errnum_,...) do {               \
		fprintf(stderr,"WARN: ");               \
		perror_at_line(_exitnum_,_errnum_,      \
			__func__,__LINE__,__VA_ARGS__); \
	} while(0)

extern int debug;

void __attribute__((format(printf,5,6)))
perror_at_line(int status, int errnum, const char *fname,
	unsigned int linenum, const char *format, ...);

#endif
