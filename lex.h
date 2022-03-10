#ifndef LEX_INCLUDED
#define LEX_INCLUDED

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

struct token * token_new(enum token_type type, char *word);
void token_delete(struct token *w);
void token_list_delete(struct token *head);
int token_list_len(const struct token *head);

struct buffer {
	char *buf;
	int bufcap;
	int bufsz;
};

struct buffer * buffer_new();
void buffer_delete(struct buffer *b);
char * buffer_get_str(struct buffer *b);
int buffer_add(struct buffer *b, char c);

struct lexer {
	enum lexer_state {
		normal,
		quote,
		empty,
		escape,
		finished,
		error,
	} state, prev_state;
	enum errtype {
		memory_err,
		quote_err,
		escape_err
	} err;
	struct buffer *buf;
	struct token *head, *tail;
};

struct lexer * lexer_new();
void lexer_delete(struct lexer *lex);
void lexer_set_err(struct lexer *lex, enum errtype type);
void lexer_append_token(struct lexer *lex, struct token *tok);
void lexer_cut(struct lexer *lex);
void lexer_handle_separator(struct lexer *lex, char sep);
struct token *lexer_get_tokens(struct lexer *lex);
void lexer_step_normal(struct lexer *lex, char c);
void lexer_step_quote(struct lexer *lex, char c);
void lexer_step_empty(struct lexer *lex, char c);
void lexer_step_escape(struct lexer *lex, char c);
void lexer_step_error(struct lexer *lex);

char * read_line();
struct token *tokenize(const char *line);

#endif /* LEX_INCLUDED */
