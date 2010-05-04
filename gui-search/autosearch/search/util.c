#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

#define DEFAULT_SZ 120
ssize_t pgetdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
        size_t ct = 0;

        if (lineptr == NULL || n == NULL) {
                return -1;
        }

        if ( *lineptr == NULL ) {
                *lineptr = malloc( sizeof( **lineptr) * DEFAULT_SZ );
                if (*lineptr == NULL) {
                        return -1;
                }
                *n = DEFAULT_SZ;
        }

        for(;;) {
                int in;

                if (ferror(stream))
                        break;
                if (feof(stream))
                        break;
                in = fgetc(stream);

                if (ct+2 > *n) {
                        size_t needed = *n * 2;
                        char *tmp = realloc(*lineptr,
                                            sizeof(**lineptr) * needed);
                        if (tmp == NULL)
                                return -1;
                        *lineptr = tmp;
                        *n = needed;
                }

                if ( in == EOF ) return -1;

                (*lineptr)[ct] = in;
                ct++;

                if ( in == delim )
                        break;
        }

        (*lineptr)[ct] = '\0';
        return ct;
}

int cmp_fileent_by_filename(void *p1, void *p2)
{
	if (!p1 || !p2) {
		fprintf(stderr,"cmp_fileent_by_filename %p %p\n",p1,p2);
		return 0;
	}

        fileent_t *f1 = p1, *f2 = p2;
        int ret = strcoll(f2->filename,f1->filename);
        return ret;
}

void merge_fileent(void *dv, void *sv)
{
	fileent_t *d = dv, *s = sv;

	d->ct += s->ct;
	

}

int cmp_keyword(void *p1, void *p2)
{
        keyword_t *k1 = p1;
        keyword_t *k2 = p2;

        return strcoll(k2->word, k1->word);
}

fileent_t *mk_fileent(char *filename, unsigned long ct)
{
/*      printf("mk_fileent:  filename=\"%s\"\n",filename);*/
        fileent_t *f = malloc(sizeof(*f));
        if (f) {
		f->filename = filename;
                f->ct = ct;
        }
        return f;
}

keyword_t *mk_keyword(char *word)
{
/*      printf("mk_keyword: word=\"%s\", filename=\"%s\"\n",word,filename);*/
        keyword_t *k = malloc(sizeof(*k));
        if (k) {
                k->word = word;

                k->fileents = SLCreate(cmp_fileent_by_filename);
                if (!(k->fileents)) {
                        free(k->word);
                        free(k);
                        return 0;
                }
	}
        return k;
}

char *memdup(const char *old, size_t len) {
        char *new = malloc(len);
        if (new)
                memcpy(new,old,len);
        return new;
}

