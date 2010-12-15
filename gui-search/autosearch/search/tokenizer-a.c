/*
 * tokenizer.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

static char *strndup2(const char *old, size_t strlen) {
	char *new = malloc(strlen+1);
	if (new) {
		memcpy(new,old,strlen+1);
		new[strlen] = '\0';
	}
	return new;
}

/*
 * Tokenizer type.  You need to fill in the type as part of your
 * implementation.
 */
struct TokenizerT_ {
	const char *delim;
	const char *cur;
};


/*
 * TKCreate creates a new TokenizerT object for a given set of separator
 * characters (given as a string) and a token stream (given as a string).
 *
 * If the function succeeds, it returns a non-NULL TokenizerT.
 * Else it returns NULL.
 *
 * You need to fill in this function as part of your implementation.
 */

TokenizerT TKCreate(const char *seperators, const char *ts) {
	TokenizerT t = malloc( sizeof(*t) );
	if(t) {
		t->delim = seperators;
		t->cur = ts + strspn(ts,t->delim);
	}	
	return t;
}

/*
 * TKDestroy destroys a TokenizerT object.  It should free all memory
 * dynamically allocated in TKCreate as part of creating a new TokenizerT
 * object.  (It should not free the token stream given as the argument 
 * to TKCreate.
 *
 * You need to fill in this function as part of your implementation.
 */
void TKDestroy(TokenizerT tk) {
	free(tk);
}

/*
 * TKGetNextToken returns the next token from the token stream as a 
 * character string. Space for the returned token should be dynamically 
 * allocated.  Caller is responsible for freeing the space once it is no 
 * longer needed.
 *
 * If the function succeeds, it returns a C string (delimited by '\0')
 * containing the token.  Else it returns 0.
 *
 * You need to fill in this function as part of your implementation.
 */
char *TKGetNextToken(TokenizerT tk) {
	const char *cur_start = tk->cur;
	size_t cur_len;
	size_t junk_len;
	if (!tk->cur) return NULL;
	if (*tk->cur == '\0') {
		tk->cur = NULL;
		return NULL;
	}
	
	/* length of current token (locates first occourance of any
	 * chars in tk->delim in string tk->cur) */
	cur_len = strcspn(tk->cur,tk->delim);
	if (cur_len == 0) {
		tk->cur = NULL;
		return NULL;
	}

	/* length of charaters following a valid token that are in the
	 * delimiter list */
	junk_len = strspn(cur_start+cur_len,tk->delim);
	if(!junk_len) {
		tk->cur = NULL;	
	} else {
		tk->cur = cur_start + cur_len + junk_len;
	}

	return strndup2(cur_start, cur_len);
}

