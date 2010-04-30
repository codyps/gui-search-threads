#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <sys/types.h>
#include "sorted-list.h"

typedef struct { 
        char *filename; 
        unsigned long ct; 
} fileent_t; 
 
typedef struct { 
        char *word; 
        SortedListPtr fileents; 
} keyword_t;

keyword_t *mk_keyword(char *word);
fileent_t *mk_fileent(char *filename, unsigned long ct);

int cmp_keyword(void *p1, void *p2);
int cmp_fileent_by_filename(void *p1, void *p2);
char *memdup(const char *old, size_t len);

ssize_t pgetdelim(char **lineptr, size_t *n, int delim, FILE *stream);
void error_at_line(int status, int errnum, const char *filename,
                          unsigned int linenum, const char *format, ...);
#endif
