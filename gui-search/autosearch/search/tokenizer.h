#ifndef TOKENIZER_H_
#define TOKENIZER_H_

typedef struct TokenizerT_* TokenizerT;
TokenizerT TKCreate(const char *seperators, const char *ts);
void TKDestroy(TokenizerT tk);
char *TKGetNextToken(TokenizerT tk);

#endif
