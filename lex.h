#ifndef LEX_INCLUDED
#define LEX_INCLUDED

enum token_type {
	tok_word, /* just word */
	tok_amp,  /* & execute in background */
	tok_gt,   /* > redirect stdout */
	tok_lt,   /* < redirect stdin */
	tok_ggt,  /* >> redirect stdout (append) */
	tok_err,  /* lexical error */
};

/* TODO: implement other tokens */
struct token {
	char *word;
	enum token_type type;
	struct token *next;
};

void token_list_delete(struct token *head);

struct token *lex(const char *line);

#endif
