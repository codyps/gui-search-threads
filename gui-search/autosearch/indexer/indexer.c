#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "sorted-list.h"
#include "tokenizer.h"

typedef struct {
	char *filename;
	unsigned long ct;
} fileent_t;

typedef struct {
	char *word;
	SortedListPtr fileents;
} keyword_t;

static int cmp_fileent(void *p1, void *p2)
{
	fileent_t *f1 = p1, *f2 = p2;
	int ret = strcmp(f2->filename,f1->filename);
	int diff = f1->ct - f2->ct;
	if (!diff) {
		return ret;
	}
	return diff;
}

static int cmp_keyword(void *p1, void *p2)
{
	keyword_t *k1 = p1;
	keyword_t *k2 = p2;

	return strcmp(k2->word, k1->word);
}

static char *memdup(const char *old, size_t len)
{
	char *new = malloc(len);
	if (new)
		memcpy(new, old, len);
	return new;
}

static char *my_strdup(const char *old)
{
	return memdup(old, strlen(old) + 1);
}

char *nice_seps = "\t::\n";
#ifdef DEBUG
#define PRINT_CHECK(str,keywords) { puts(str); puts(" <begin>"); print_index( keywords, '\0', nice_seps,stderr); puts(" <end>"); }
#else
#define PRINT_CHECK(x,y) {}
#endif
/* NIL (\0) is the field seperator. 1 Character immediatly following 
 *  NIL should be ignored by the parser. Keywords are seperated by 2
 *  seperators. All other fields are seperated by one. The first
 *  field in a index file is a keyword.
	 (\t)
keyword\0	file_name\0:ct\0:file_name\0:ct\0:\0
keyword\0	...
 */

static void print_fileents( SortedListPtr fileents, FILE *out)
{
	SortedListIteratorPtr fileent_iter =
	    SLCreateIterator(fileents);

	if (!fileent_iter) {
		perror("Could not print index (fileent_iter failed)");
		return;
	}

	fileent_t *f;
	while ((f = SLNextItem(fileent_iter))) {
		fprintf(out, "%d#%s#%lu|", (int)strlen(f->filename), f->filename, f->ct);
	}
	SLDestroyIterator(fileent_iter);
}

static void print_index(SortedListPtr /* of keyword_t */ keywords, FILE * out)
{
	SortedListIteratorPtr keyword_iter = SLCreateIterator(keywords);
	if (!keyword_iter) {
		perror("Could not print index (keyword_iter failed)");
		return;
	}

	keyword_t *k;
	while ((k = SLNextItem(keyword_iter))) {
		fprintf(out, "%s@", k->word);
		
		print_fileents( k->fileents ,out);
		
		fputc('\n', out);
	}
	SLDestroyIterator(keyword_iter);
}

/* returns new path_len */
static size_t
pathat(char **path_buf, size_t * dest_mem_sz, size_t path_len, char *file)
{
	size_t f_len = strlen(file);

	size_t new_d_sz = path_len + 1 + f_len + 1;
	char *buf = *path_buf;

	if (new_d_sz > *dest_mem_sz) {
		buf = realloc(*path_buf, sizeof(*buf) * new_d_sz);
		if (buf) {
			*dest_mem_sz = new_d_sz;
			*path_buf = buf;
		}
	}

	if (buf) {
		if (buf[path_len-1] != '/') {
			buf[path_len] = '/';
			path_len += 1;
		}
		memcpy(buf + path_len, file, f_len + 1);
	}

	return path_len + f_len;
}

static fileent_t *mk_fileent(const char *filename)
{
/*	printf("mk_fileent:  filename=\"%s\"\n",filename);*/
	fileent_t *f = malloc(sizeof(*f));
	if (f) {
		f->filename = my_strdup(filename);
		if (!f->filename) {
			free(f);
			return 0;
		}
		f->ct = 1;
	}
	return f;
}

static keyword_t *mk_keyword(char *word, const char *filename)
{
/*	printf("mk_keyword: word=\"%s\", filename=\"%s\"\n",word,filename);*/
	keyword_t *k = malloc(sizeof(*k));
	if (k) {
		k->word = word;

		k->fileents = SLCreate(cmp_fileent);
		if (!(k->fileents)) {
			free(k->word);
			free(k);
			return 0;
		}

		fileent_t *fileent = mk_fileent(filename);
		if (!fileent) {
			SLDestroy(k->fileents);
			free(k->word);
			free(k);
			return 0;
		}

		int ret = SLInsert(k->fileents, fileent);
		if (ret != 1) {
			SLDestroy(k->fileents);
			free(k->word);
			free(k);
			return 0;
		}
	}
	return k;
}

static void index_file(SortedListPtr /* of keyword_t */ keywords, char *fname,
		FILE * in)
{
	int ret;
	size_t len_r, len_c;
	long pos_orig, pos_end;
	char *buf;
	char *word;
	TokenizerT tok;

	char *saved_fname = fname;
	while( ( (*saved_fname == '.') || (*saved_fname == '/') ) 
			&& (*saved_fname != '\0') )
		saved_fname++;


	pos_orig = ftell(in);
	ret = fseek(in, 0, SEEK_END);
	if (ret) {
		perror("Could not determine file size");
	}
	pos_end = ftell(in);
	ret = fseek(in, pos_orig, SEEK_SET);

	/* printf("file start: %li, file end: %li\n",pos_orig,pos_end); */
	len_c = pos_end - pos_orig + 1;

	buf = malloc(sizeof(*buf) * len_c);
	if (!buf) {
		fprintf(stderr,
			"Could not allocate %lu bytes for reading \"%s\" :",
			(unsigned long) sizeof(*buf) * len_c, fname);
		perror(0);
		return;
	}

	len_r = fread(buf, 1, len_c, in);
	buf[len_r] = '\0';
	if (len_r != (len_c - 1)) {
		fprintf(stderr, "Error reading file \"%s\": ", fname);
		perror(0);
		return;
	}

	tok = TKCreate(" ~`!@#$%^&*()_+=[]{}|\\:;\"<>,.?/\n\t\r", buf);
	if (!tok) {
		/* strange TK implimentation you got there, mate. */
		return;	
	}
	while ((word = TKGetNextToken(tok))) {
		size_t i;
		for (i = 0; word[i] != '\0'; i++) 
			word[i] = tolower(word[i]);
		SortedListIteratorPtr keyword_iter = SLCreateIterator(keywords);
		/* printf("word : %s \n",word); */
		for (;;) {
			keyword_t *k = SLNextItem(keyword_iter);
			if (!k) {
				/* if not found, add new keyword_t */
				SLDestroyIterator(keyword_iter);
				k = mk_keyword(word, saved_fname);
				if (!k) {
					perror("mk_keyword failed, index may be corrupt");
					continue;
				}
				PRINT_CHECK("new keyword (prior):",keywords);
				ret = SLInsert(keywords, k);
				PRINT_CHECK("new keyword (after):",keywords);
				if (ret != 1) {
					perror("SLInsert [1] failed");
				}
				/* continue 
				 * while( (word = TKGetNextToken(tok)) ) { ... } 
				 */
				break;
			} else if (!strcmp(k->word, word)) {
				/* keyword found, find fileent */
				SortedListIteratorPtr fileent_iter =
				    SLCreateIterator(k->fileents);
				if (!fileent_iter) {
					perror("fileent_iter failed, index may be corrupt");
					continue;
				}

				free(word);
				SLDestroyIterator(keyword_iter);

				for (;;) {
					fileent_t *f = SLNextItem(fileent_iter);
					if (!f) {
						SLDestroyIterator(fileent_iter);
						f = mk_fileent(saved_fname);
						if (!f) {
							perror
							    ("mk_fileent failed, index may be corrupt");
						}


						PRINT_CHECK("new fileent (prior):",keywords);
						ret = SLInsert(k->fileents, f);
						PRINT_CHECK("new fileent (after):",keywords);
						if (ret != 1) {
							perror
							    ("SLInsert [2] failed, index may be corrupt");
						}

						break;
					} else if (!strcmp(f->filename, saved_fname)) {
						/* fileent found. */
						SLDestroyIterator(fileent_iter);
						PRINT_CHECK("Modify fileent (prior):",keywords);
						ret = SLRemove(k->fileents, f);
						PRINT_CHECK("Modify fileent (after):",keywords);
						if (ret != 1) {
							perror
							    ("SLRemove [3] failed, index may be corrupt");
						}

						f->ct++;
						ret = SLInsert(k->fileents, f);
						if (ret != 1) {
							perror
							    ("SLInsert [3] failed, index may be corrupt");
						}

						break;
					}
				}	/* for(;;) fileent */
				break;
			}
		}		/* for(;;) keyword */
	}
	free(buf);
	TKDestroy(tok);
}



static int
process_dir(SortedListPtr keywords, char **d_name, size_t d_name_len,
	    size_t * d_mem_sz)
{
	DIR *dir;
	struct dirent *de;
	struct stat s;
	int ret;

	ret = stat(*d_name, &s);
	if (ret) {
		fprintf(stderr, "Could not stat file \"%s\": ", *d_name);
		perror(0);
		return -1;
	}
	if (S_ISREG(s.st_mode)) {
		FILE *fp = fopen(*d_name, "r");
		if (fp) {
			index_file(keywords, *d_name, fp);
			fclose(fp);
			return 0;
		} else {
			fprintf(stderr, "Could not open file \"%s\": ",
				*d_name);
			perror(0);
			return -1;
		}
	} else if (!S_ISDIR(s.st_mode)) {
		return -2;
	}

	dir = opendir(*d_name);
	if (!dir) {
		fprintf(stderr, "Could not open dir \"%s\": ", *d_name);
		perror(0);
		return -1;
	}

	while ((de = readdir(dir))) {
		/* printf("dirent = %s\n",de->d_name); */

		/* if (strcmp(".", de->d_name) && strcmp("..", de->d_name)) { */
		if (de->d_name[0] != '.') {
			size_t p_len = pathat(d_name, d_mem_sz, d_name_len,
					      de->d_name);
			process_dir(keywords, d_name, p_len, d_mem_sz);
		}
	}

	closedir(dir);

	return 0;
}

static void free_keyword_sl(SortedListPtr keywords) {
	SortedListIteratorPtr k_i = SLCreateIterator(keywords);
	if (!k_i) {
		perror("k_iter failed, index may be corrupt");
		return;
	}
	keyword_t *k;
	while ((k = SLNextItem(k_i))) {
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
	SLDestroyIterator(k_i);
	SLDestroy(keywords);
}

int main(int argc, char **argv)
{
	FILE *fp_o;
	SortedListPtr /* of keyword_t */ keywords = SLCreate(cmp_keyword);

	if (argc != 3) {
		fprintf(stderr, "usage: %s <inverted-index file name>"
			" <directory or file name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	size_t p_len = strlen(argv[2]);
	size_t p_mem_sz = p_len + 1;
	char *path = malloc(sizeof(*path) * (p_mem_sz));
	if (!path) {
		perror(0);
		exit(EXIT_FAILURE);
	}

	memcpy(path, argv[2], p_len + 1);

	#if 0
	size_t p_len = 1;
	size_t p_mem_sz = 2;
	char *path = malloc(2);
	if (!path) {
		perror(0);
		exit(EXIT_FAILURE);
	}
	memcpy(path,".",p_len+1);

	char *cwd = getcwd(0,0); /* glibc allocates as much as needed. */
	int ret = chdir(argv[2]);
	if (ret) {
		perror("Could not change to indexing dir");
		exit(EXIT_FAILURE);
	}
	#endif

	int ret_important = process_dir(keywords, &path, p_len, &p_mem_sz);
	free(path);

	/*
	ret = chdir(cwd);
	if (ret) {
		perror("Change back failed");
		exit(EXIT_FAILURE);
	}
	*/

	if (ret_important >= 0) {
		fp_o = fopen(argv[1], "wb");
		if (!fp_o) {
			fprintf(stderr,"indexer: failed to open output file %s : ",argv[1]);
			perror(0);
			exit(EXIT_FAILURE);
		}
		print_index(keywords, fp_o);
		fclose(fp_o);
	}

	free_keyword_sl(keywords);
	return 0;
}
