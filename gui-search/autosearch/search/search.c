#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "tokenizer.h"
#include "sorted-list.h"
#include "index.h"
#include "util.h"
#include "threadpool.h"
#include "errors.h"

static int debug = 0;

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

static int look(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords, int thread_count) {
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

static int testlookup(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords, int thread_count) {
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
/* Contains the final score data */
struct score {
	char *filename;
	long double score;
};

int cmp_score_by_filename(void *a_, void *b_)
{
	struct score *a = a_, *b = b_;

	return strcoll(a->filename,b->filename);
}

int cmp_score_by_score(void *a_, void *b_)
{
	struct score *a = a_, *b = b_;

	long double x = a->score - b->score;
	if (x != 0) {
		return x;
	} else {
		return cmp_score_by_filename(a_,b_);
	}
}

/* Data specific to a given query from the user */
struct query_data {
	SortedListPtr /* of score */ scores; /* sorted by filename */
	SortedListPtr /* of fileents */ fileents; /* ALL fileents */
	
	keyword_t **words;	
	size_t words_ct; /* the n */

};

/* Data given to a specific thread */
struct thread_data {
	size_t word_i; // the one it is responsible for.

	struct query_data *q_data;
};


/* we can get N by unioning all the keyword's fileent trees together,
 * and then using SLGetCt on the result
 */

/*
 *
 */

/* TODO: 
 * this function: 
 * 	score(Q,F) = sum( log( 1 + N / Nt ) * ( 1 + log(Ft) ) ) / sqrt(|F|);
 *	Ft = fileent's frequency (for term t)
	 *	Nt = number of fileents in a keyword_t
 *	N = total number of files
 *	|F| = total number of terms in the file.
 *	n = number of terms
 *
 */

void worker_thread(void *data_v)
{
	struct thread_data *data = data_v;
	size_t our_word_i = data->word_i;
	keyword_t *our_word = data->q_data->words[our_word_i];
	SortedListPtr scores = data->q_data->scores;

	SortedListIteratorPtr our_files = SLCreateIterator(our_word->fileents);
	fileent_t *our_curr_file;

	// for each of our word's files,
	while ( (our_curr_file = SLNextItem(our_files)) ) {
		// create a skeleton struct for score with the value set to 0.
		struct score *curr_file_score = malloc(sizeof(*curr_file_score));
		if (!curr_file_score) {
			/* XXX: error */
			WARN(1,errno,"could not allocate curr_file_score");
		}
		curr_file_score->filename = our_curr_file->filename;
		curr_file_score->score = 0;

		/* XXX: possible issue */
		if (!SLInsert(scores,curr_file_score)) {
			/* Someone is already at work. */
			INFO("Someone already is working on %s",our_curr_file->filename);
			continue;
		}
	
		/* We are now responsible for this file */	
		long double Nt = SLGetCt(our_word->fileents);
			
		// for each term in the query.
		size_t i;
		for(i = 0; i < data->q_data->words_ct; i++) {
				keyword_t *curr_term = data->q_data->words[i];
				fileent_t *our_file_in_term;
				our_file_in_term = SLLookup(curr_term->fileents,our_curr_file);

				// if our_file is not in this term,
				if (our_file_in_term == NULL) {
					INFO("our_file %s is not in term %s",our_curr_file->filename,curr_term->word);
					continue;
				}

				long double Ft = our_file_in_term->ct;
				long double N = SLGetCt(data->q_data->fileents);
				long double score = log( 1 + N / Nt) * ( 1 + log( Ft ) );
				
				curr_file_score->score += score;
		}

		/* Need |F| */
		fileent_t *file_abs_f = SLLookup(data->q_data->fileents,our_curr_file);
		if (!file_abs_f) {
			/*XXX: error */
			WARN(1,errno,"could not locate the desired fileent %s in the global list",our_curr_file->filename);
		}

		long double abs_F = file_abs_f->ct;
		
		curr_file_score->score /= sqrt(abs_F);
		INFO("File %s has a score %Lf",curr_file_score->filename,curr_file_score->score);

		// score(Q,F) should be calculated. yay.
	}
}

static int so(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords, int thread_count) {
	SortedListPtr fileents;
	keyword_t **k;
	keyword_t **words = 0;
	size_t words_ct = 0;
	size_t i = 0;

	fileents = SLCreate(cmp_fileent_by_filename);
	for( i = 0; i < argc; i++) {
		k = bsearch( argv[i], keywords, n_keywords, sizeof(*keywords), cmp_lookup_keyword); 
		
		if (k) {
			/* Place the terms into the word array, used for dispatching the query later */
			words_ct++;
			words = realloc(words, words_ct * sizeof(*words));

			words[words_ct-1] = *k;

			/* Create the SortedList of all fileents */
			SLUnionSmart(fileents,(*k)->fileents,merge_fileent);
			
		}
	}

	if (words_ct > 0) {
		// create the query_data.	
		struct query_data q_data;
		q_data.words = words;
		q_data.words_ct = words_ct;
		q_data.scores = SLCreate(cmp_score_by_filename);
		q_data.fileents = fileents;

		//create the threadpool
		threadpool tp = create_threadpool(thread_count);

		// allocate thread data.
		struct thread_data *t_datas = malloc(sizeof(*t_datas) * words_ct);
		for(i = 0; i < words_ct; i++) {
			// populate thread data.
			t_datas[i].q_data = &q_data;
			t_datas[i].word_i = i;

			INFO("dispatching thread %d on %s",i,words[i]->word);

			// dispatch work to thread.
			dispatch(tp, worker_thread, &t_datas[i]);
		}

		// wait for all threads to complete.
		destroy_threadpool(tp);

		// stop leaking t_data.
		free(t_datas);


		/* print out the extracted list */

		// resort the list of (score,filename) by score.
		SortedListIteratorPtr sort_iter = SLCreateIterator(q_data.scores);
		if (sort_iter) {
			SortedListPtr scores_sorted = SLCreate(cmp_score_by_score);
			struct score *s;
			while(( s = SLNextItem(sort_iter) )) {
				SLInsert(scores_sorted,s);
			}
			SLDestroyIterator(sort_iter);
			SLDestroy(q_data.scores);

			// print them in order of scoring.
			SortedListIteratorPtr iter = SLCreateIterator(scores_sorted);
			if (iter) {
				struct score *s2;
				s2 = SLNextItem(iter);
				if (s2) {
					// print the first term.
					fprintf(stdout,"%s (%Lf)",s2->filename,s2->score);
					free(s2);
					while((s2 = SLNextItem(iter))) {
						// print following terms preceded by a comma.
						fprintf(stdout, ", %s (%Lf)",s2->filename,s2->score);
						free(s2);
					}
					fputs(".\n",stdout);
				}
				SLDestroyIterator(iter);
			}
			SLDestroy(scores_sorted);
		
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

int q(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords, int thread_count) {
	exit(EXIT_SUCCESS);
}

typedef int (*cmd_func)(size_t argc, char **argv, size_t n_keywords, keyword_t **keywords, int thread_count);

struct cmd {
	const char *name;
	cmd_func func;
} cmds [] = {
	{"q",q},
	{"so",so},
	{"look",look},
	{"tl",testlookup},
	{}
};


int main(int argc, char **argv)
{
	size_t n_keywords;
	keyword_t **keywords;
	int thread_count;

	if (argc != 3) {
		fprintf(stderr,"usage: %s <index file> <thread count>\n",argv[0]);
		return 1;
	}

	FILE *fp = fopen(argv[1],"rb");
	if (!fp || ferror(fp)) {
		error_at_line(-1,errno,__FILE__,__LINE__,"could not open \"%s\"",argv[1]);
	}
	
	int ret = sscanf(argv[2], "%d", &thread_count);
	if(ret != 1 || thread_count < 1){
		error_at_line(-1,errno,__FILE__,__LINE__,"Invalid thread count \"%s\"",argv[2]);
	}
	
	keywords = index_read(&n_keywords,fp);
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
							cmds[i].func(n_args,args,n_keywords,keywords, thread_count);
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
