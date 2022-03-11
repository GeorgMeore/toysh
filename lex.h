#ifndef LEX_INCLUDED
#define LEX_INCLUDED

struct token {
	enum token_type {
		tok_word,         /* just word */
		tok_bg   = '&',   /* execute in background */
		/* TODO: implement other tokens */
		/* tok_sem  = ';', */
		/* tok_lbr  = '(', */
		/* tok_rbr  = ')', */
		/* tok_gt   = '>', */
		/* tok_lt   = '<', */
		/* tok_pipe = '|', */
		/* tok_ggt  = '>'*2, */
		/* tok_llt  = '<'*2, */
		/* tok_and  = '&'*2, */
		/* tok_or  = '|'*2, */
	} type;
	char *word;
	struct token *next;
};

void token_list_delete(struct token *head);
int token_list_len(const struct token *head);

struct token *tokenize(const char *line);

#endif /* LEX_INCLUDED */
