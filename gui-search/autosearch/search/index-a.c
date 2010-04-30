#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "index.h"
#include "util.h"

enum fields {
	KEYWORD,
	FILENAME,
	FREQ
};

char * dupfield(char *buf, size_t word_len) {
	char *word;
	if ( word_len == 1 && *buf == '\0' )
		return 0;

	if ( buf[word_len-1] == '\0' ) {
		word = malloc( word_len );
		if (!word) {
			error_at_line(
				0,errno,
				__FILE__,__LINE__,
				"bad malloc");
			return 0;
		}
	} else  {
		word = malloc( word_len + 1 );
		if (!word) {
			error_at_line(
				0,errno,
				__FILE__,__LINE__,
				"bad malloc");
			return 0;
		}
		word[word_len] = '\0';
	}
	memcpy(word,buf,word_len);
	return word; 
}

keyword_t **index_read( size_t *keywords_len, FILE *in ) {
	keyword_t **keywords = 0;
	*keywords_len = 0;

	char *buf = 0;
	size_t buf_mem = 0;

	ssize_t line_len;


	while( (line_len = pgetdelim(&buf, &buf_mem, '\n', in))) {
		char *lineptr = 0;
		char *keyword = strtok_r(buf,"@", &lineptr);
		if (!keyword) {
			error_at_line( 0,errno,__FILE__,__LINE__,
				"keyword not found");
			continue;
		}
		/*printf("got keyword \"%s\"\n",keyword); */

		(*keywords_len)++;
		keywords = realloc(keywords,sizeof(*keywords) * (*keywords_len));

		char *keyword_cpy = memdup(keyword,strlen(keyword)+1);
		keywords[*keywords_len - 1] = mk_keyword(keyword_cpy);

		char *fileent;
		while( (fileent = strtok_r(0,"|",&lineptr) ) ) {
			/* printf("got fileent \"%s\"\n",fileent); */
			char *fileentptr = 0;
			/*char *fname_len = */
			strtok_r(fileent,"#", &fileentptr);
			char *fname = strtok_r(0,"#", &fileentptr);
			if (!fname) {
				break;
			}
			size_t fname_len_i = strlen(fname) + 1;
			char *freq = strtok_r(0,"#"     , &fileentptr);
			char *filename = memdup(fname,fname_len_i);

			unsigned long ct;
			/* printf("got freq \"%s\"\n",freq); */
			int ret = sscanf(freq,"%lu",&ct);
			if (ret != 1) {
				error_at_line( 1,errno,__FILE__,__LINE__,
					"sscanf failed");
			
			}

			fileent_t * fileent_obj = mk_fileent(filename,ct);
			SLInsert(keywords[(*keywords_len)-1]->fileents,fileent_obj);
			
		}
	}
	free(buf);
	return keywords;
}

#if 0
keyword_t **index_read( size_t *keywords_len, FILE *in ) {
	keyword_t **keywords = 0;
	*keywords_len = 0;

	char *buf = 0;
	size_t buf_mem = 0;

	int new_keyword = 1;	
	enum fields field = KEYWORD;
	char *filename;
	
	for(;;) {
		ssize_t word_len;
		char *word;

		word_len = pgetdelim(&buf, &buf_mem, '\n', in);
		if (word_len < 0)
			break;
		else if (new_keyword) {
			(*keywords_len)++;
			keyword_t **tmp = realloc(keywords,(*keywords_len) * (sizeof(*tmp)));
			if (!tmp) {
				error_at_line( 0,errno,__FILE__,__LINE__,
					"bad realloc");
				break;
			}
			keywords = tmp;
			new_keyword = 0;
			field = KEYWORD;
		}
	
		getc(in); /* discard the character following the null */

		if (word_len > 1) {
			switch(field) {
			case KEYWORD:	
				word = dupfield(buf,word_len);		
				keywords[(*keywords_len)-1] = mk_keyword(word);
				field = FILENAME;
				break;
			case FILENAME:
				word = dupfield(buf,word_len);		
				filename = word;
				field = FREQ;
				break;		
			case FREQ: {
					unsigned long ct;
					int ret = sscanf(buf,"%lu",&ct);
					if (ret != 1) {
						error_at_line(
							0,errno,
							__FILE__,__LINE__,
							"invalid frequency");
					}
					fileent_t * fileent = mk_fileent(filename,ct);
					SLInsert(keywords[(*keywords_len)-1]->fileents,fileent);
					field = FILENAME;
				}
				break;
			default:
				error_at_line(0,errno,__FILE__,__LINE__,
				"this case should never occour");
			}

		} else {
			new_keyword = 1;
		}
	}
	free(buf);
	return keywords;
}

#endif

