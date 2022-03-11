#ifndef LEX_INCLUDED
#define LEX_INCLUDED

#include "read.h"

struct token {
	enum token_type {
		tok_word,       /* just word */
		tok_sem  = ';', /* ; */
		tok_lbr  = '(', /* ( */
		tok_rbr  = ')', /* ) */
		tok_gt   = '>', /* > */
		tok_lt   = '<', /* < */
		tok_amp  = '&', /* & */
		tok_pipe = '|', /* | */
	} type;
	char *word;
	struct token *next;
};

void token_list_delete(struct token *head);
int token_list_len(const struct token *head);

struct token *tokenize(const char *line);

#endif /* LEX_INCLUDED */
