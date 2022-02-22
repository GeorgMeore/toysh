#ifndef LEX_INCLUDED
#define LEX_INCLUDED

struct word_item {
	char *word;
	struct word_item *next;
};

struct word_item * word_item_new(char *word);
void word_item_delete(struct word_item *w);
void word_list_delete(struct word_item *head);

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
		memory_err,
		quote_err,
		escape_err,
	} state, prev_state;
	struct buffer *buf;
	struct word_item *head, *tail;
};

struct lexer * lexer_new();
void lexer_delete(struct lexer *l);
void lexer_cut(struct lexer *l);
void lexer_step_normal(struct lexer *l, char c);
void lexer_step_quote(struct lexer *l, char c);
void lexer_step_empty(struct lexer *l, char c);
void lexer_step_escape(struct lexer *l, char c);
struct word_item *lexer_get_list(struct lexer *l);

char * read_line();
struct word_item * split_line(const char *line);

#endif // LEX_INCLUDED
