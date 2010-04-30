#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include "tokenizer.h"
#include "sorted-list.h"
#include "index.h"
#include "util.h"

#if DEBUG
static void print_fileents( SortedListPtr fileents, char sep, char *pretty_seps,
                 FILE *out)
{
        SortedListIteratorPtr fileent_iter =
            SLCreateIterator(fileents);

        if (!fileent_iter) {
                perror("Could not print index (fileent_iter failed)");
                return;
        }

        fileent_t *f;
        while ((f = SLNextItem(fileent_iter))) {
                fprintf(out, "%s%c%c%lu%c%c", f->filename, sep,
                        pretty_seps[1], f->ct, sep, pretty_seps[2]);
        }
        SLDestroyIterator(fileent_iter);
}
#endif

static void free_keywords(size_t n, keyword_t **keywords) {
        keyword_t *k;
	size_t i;
        for (i = 0; i < n; i++) {
		k = keywords[i];
                free(k->word);

                SortedListIteratorPtr f_i = SLCreateIterator(k->fileents);
                if (!f_i) {
                        perror("f_iter failed, index may be corrupt");
                        continue;
                }
                fileent_t *f;
                while ((f = SLNextItem(f_i))) {
                        free(f->filename);
                        free(f);
                }
                SLDestroyIterator(f_i);
                SLDestroy(k->fileents);
                free(k);
        }
	free(keywords);
}

void error_at_line(int status, int errnum, const char *filename, unsigned int linenum,
	const char *format, ...) {

	va_list args;

	fflush(stdout);

	fprintf(stderr,"search: ");
	if (filename)
		fprintf(stderr,"%s:%d: ", filename, linenum);
	else
		fputs(" ",stderr);


	va_start(args, format);
	vfprintf(stderr,format,args);

	fputc('\n',stderr);

	if (status != 0)
		exit(status);
}

static int look(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords) {
	printf("argc: %d n_keywords: %d\n",argc, n_keywords);
	size_t i,j;
	for ( i = 0; i < argc; i++) {
		printf(" %d : %s\n",(int)i,argv[i]);

		for ( j = 0; j < n_keywords; j++) {
			printf(" keyword \"%s\" %d \n",keywords[j]->word,j);
			fileent_t *f = mk_fileent(argv[i],0);
			fileent_t *r = SLLookup(keywords[j]->fileents,f);
			if (r) {
				printf("  found %s\n",r->filename);
			} else {
				printf("  could not find %s\n",argv[i]);
			}
			free(f);
		}
	}

	return 0;
}


int cmp_str(void *p1, void *p2) {
	return strcmp(p1,p2);
}

void sl_ins_p(SortedListPtr s, char *str) {
	printf("SLInsert( sl = %p, str = \"%s\" ) -> ",s,str);
	printf("%d\n",SLInsert(s,str));
}

void sl_look_p(SortedListPtr s, char *str) {
	printf("SLLookup( sl = %p, str = \"%s\" ) -> ",s,str);
	char *r = SLLookup(s,str);
	if (r)
		puts(r);
	else
		puts("(null)");
}

static int testlookup(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords) {
	SortedListPtr s = SLCreate(cmp_str);
	sl_ins_p(s,"hola");
	sl_ins_p(s,"goodbye");
	sl_look_p(s,"hola");
	sl_look_p(s,"not");
	return 0;
}

static int cmp_lookup_keyword( const void *key, const void *elem ) {
	const char *str = key;
	const keyword_t *const *keyword = elem;

	return strcmp(str,(*keyword)->word);
}

static int sa(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords) {
	SortedListPtr fileents;
	keyword_t **k;
	size_t i = 0;

	if (argc < 1)
		return -1;
	k = bsearch( argv[0], keywords, n_keywords, sizeof(*keywords), cmp_lookup_keyword); 
		
	if (k) {
		/* extract fileents */
		fileents = SLDup((*k)->fileents);
		for(; i < argc; i++) {
			k = bsearch( argv[i], keywords, n_keywords, sizeof(*keywords),cmp_lookup_keyword);
			
			/* AND */
			/* for each element in the original fileents,
			 *  lookup that element in the current keyword's list
 			 *  if not found, remove from the original fileents.
	 		 */
			if (!k)	
				goto cleanup;
			SLIntersect(fileents,(*k)->fileents);
		}
		/* print out the extracted list */
		SortedListIteratorPtr iter = SLCreateIterator(fileents);
		fileent_t *f;
		f = SLNextItem(iter);
		if (f) {
			fputs(f->filename,stdout);
			while((f = SLNextItem(iter))) {
				fprintf(stdout, ", %s",f->filename);
			}
			fputs(".\n",stdout);
		}
		SLDestroyIterator(iter);
		cleanup:
		SLDestroy(fileents);
		return 1;
	}
	return -1;
}

static int so(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords) {
	SortedListPtr fileents;
	keyword_t **k;
	size_t i = 0;
	for( i = 0; i < argc; i++) {
		k = bsearch( argv[i], keywords, n_keywords, sizeof(*keywords), cmp_lookup_keyword); 
		
		if (k) {
			/* extract fileents */
			fileents = SLDup((*k)->fileents);
			for(; i < argc; i++) {
				k = bsearch( argv[i], keywords, n_keywords, sizeof(*keywords),cmp_lookup_keyword);

				/* OR */
				/* add each fileent to the extracted fileents from argv[0] */
				if (k)	
					SLUnion(fileents,(*k)->fileents);


				/* AND */
				/* for each element in the original fileents,
 				 *  lookup that element in the current keyword's list
	 			 *  if not found, remove from the original fileents.
		 		 */
			}
			/* print out the extracted list */
			SortedListIteratorPtr iter = SLCreateIterator(fileents);
			if (iter) {
				fileent_t *f;

				f = SLNextItem(iter);
				if (f) {
					fputs(f->filename,stdout);
					while((f = SLNextItem(iter))) {
						fprintf(stdout, ", %s",f->filename);
					}
					fputs(".\n",stdout);
				}
				SLDestroyIterator(iter);
			}
			SLDestroy(fileents);

			return 1;
		}
	}
	return 0;
}

static void free_args(size_t n, char **args) {
	size_t i;
	for(i = 0; i < n; i++) {
		free(args[i]);
	}
	free(args);
}

static char **tok2arg(size_t *n, TokenizerT tk) {
	char **args = 0;
	char *token;
	*n = 0;
	while((token = TKGetNextToken(tk))) {
		(*n)++;
		args = realloc(args,(*n) * sizeof(*args));
		args[(*n)-1] = token;
	}
	return args;
}

int q(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords) {
	exit(EXIT_SUCCESS);
}

typedef int (*cmd_func)(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords);

struct cmd {
	const char *name;
	cmd_func func;
} cmds [] = {
	{"q",q},
	{"so",so},
	{"sa",sa},
	{"look",look},
	{"tl",testlookup},
	{}
};

int main(int argc, char **argv) {	
	if (argc != 2) {
		fprintf(stderr,"usage: %s <index file>\n",argv[0]);
		return 1;
	}

	FILE *fp = fopen(argv[1],"rb");
	if (!fp || ferror(fp)) {
		error_at_line(-1,errno,__FILE__,__LINE__,"could not open \"%s\"",argv[1]);
	}

	size_t n_keywords;
	keyword_t **keywords = index_read(&n_keywords,fp);
	fclose(fp);
	if (!keywords) {
		error_at_line(-1,errno,__FILE__,__LINE__,"keywords is null");
	}

	#if DEBUG
	for(i = 0; i < n_keywords; i++) {
		size_t i;
		printf("%s%c%c",keywords[i]->word,'\0','\t');
		print_fileents(keywords[i]->fileents, '\0', "\t::\n",stdout);
		putchar('\n');	

	}
	#endif

	size_t n = 0;
	char *line = NULL;

	for(;;) {
		char **args;
		char *cmd;		

		ssize_t ret;
		
		size_t n_args;

		/* putchar('\0'); */
		fflush(stdout);
		ret = pgetdelim(&line,&n,'\n',stdin);
		if (ret < 0) {
			putchar('\n');
			break;
		}
	
		if (ret > 0) {		
			TokenizerT toker = TKCreate(" \n", line);
			cmd = TKGetNextToken(toker);
			if (cmd) {
				size_t i;
				for( i = 0;; i++ ) {
					if (cmds[i].name) {
						if (!strcmp(cmds[i].name,cmd)) {
							args = tok2arg(&n_args,toker);
							cmds[i].func(n_args,args,n_keywords,keywords);
							free_args(n_args,args);
							break;
						}
					} else {
						fputs("valid commands: ",stderr);
						for( i = 0; cmds[i].name; i++) {
							fprintf(stderr,
								"%s, ", cmds[i].name);
						}
						putc('\n',stderr);
						break;
					}
				}	
			}							
			free(cmd);		
			TKDestroy(toker);
			exit(16);
		}
	}

	
	/* cleanup */
	free_keywords(n_keywords,keywords);
	free(line);	
	
	return 0;
}
