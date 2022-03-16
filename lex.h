#ifndef LEX_INCLUDED
#define LEX_INCLUDED

/* TODO: implement other tokens */
struct token {
	enum token_type {
		tok_word, /* just word */
		tok_amp,  /* & execute in background */
		tok_gt,   /* > redirect stdout */
		tok_lt,   /* < redirect stdin */
		tok_ggt,  /* >> redirect stdout (append) */
	} type;
	char *word;
	struct token *next;
};

void token_list_delete(struct token *head);

struct token *tokenize(const char *line);

#endif /* LEX_INCLUDED */
