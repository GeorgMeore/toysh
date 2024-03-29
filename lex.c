#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "lex.h"

#define CHRBUFSZ 256

#define PEEK(iter) (**iter)
#define NEXT(iter) (*(*iter)++)
#define SKIP(iter) ((void)((*iter)++))

/* word must be malloc-allocated string
 * ownership of word is transferred here */
static struct token *
token_new(enum token_type type, char *word)
{
	struct token *tok;
	tok = emalloc(sizeof(*tok));
	tok->type = type;
	tok->word = word;
	tok->next = NULL;
	return tok;
}

static void
token_delete(struct token *w)
{
	free(w->word);
	free(w);
}

static void
token_list_append(struct token **list, struct token *new)
{
	while (*list != NULL)
		list = &(*list)->next;
	*list = new;
}

void
token_list_delete(struct token *head)
{
	struct token *tmp;
	while (head) {
		tmp = head->next;
		token_delete(head);
		head = tmp;
	}
}

struct charbuf {
	char *buf;
	int size;
};

static void
charbuf_init(struct charbuf *self)
{
	self->buf = emalloc(CHRBUFSZ);
	self->buf[0] = 0;
	self->size = 0;
}

static void
charbuf_destroy(struct charbuf *self)
{
	free(self->buf);
}

static void
charbuf_add(struct charbuf *self, char c)
{
	if (self->size % CHRBUFSZ == CHRBUFSZ - 1) {
		self->buf = erealloc(self->buf, self->size + 1 + CHRBUFSZ);
	}
	self->buf[self->size++] = c;
	self->buf[self->size] = 0;
}

/* check if the charater is a whitespace */
static int
is_ws(char c)
{
	return c == '\n' || c == '\t' || c == ' ';
}

/* check if the charater can be a part of a separator */
static int
is_sep_char(char c)
{
	return c == '>' || c == '&' || c == '<';
}

/* try to convert a string to a token */
enum token_type
to_tok(const char *str)
{
	if (str_equal(str, "&")) {
		return tok_amp;
	} else if (str_equal(str, "<")) {
		return tok_lt;
	} else if (str_equal(str, ">")) {
		return tok_gt;
	} else if (str_equal(str, ">>")) {
		return tok_ggt;
	} else {
		return tok_err;
	}
}

static struct token *
lex_sep(const char **iter)
{
	struct charbuf word;
	enum token_type type;
	charbuf_init(&word);
	while (is_sep_char(PEEK(iter))) {
		charbuf_add(&word, NEXT(iter));
	}
	type = to_tok(word.buf);
	if (type == tok_err) {
		fprintf(stderr, "toysh: syntax error near '%s'\n", word.buf);
		charbuf_destroy(&word);
		return NULL;
	}
	return token_new(type, word.buf);
}

static int
lex_word_quote(struct charbuf *word, const char **iter)
{
	SKIP(iter); /* skip the starting " */
	for (;;) {
		if (!PEEK(iter)) {
			fputs("toysh: unclosed quote\n", stderr);
			return 0;
		} else if (PEEK(iter) == '\\') {
			SKIP(iter);
			if (!PEEK(iter)) {
				fputs("toysh: broken escape\n", stderr);
				return 0;
			}
			charbuf_add(word, NEXT(iter));
		} else if (PEEK(iter) == '"') {
			SKIP(iter); /* skip the closing " */
			return 1;
		} else {
			charbuf_add(word, NEXT(iter));
		}
	}
}

static struct token *
lex_word(const char **iter)
{
	struct token *tokens = NULL;
	struct charbuf word;
	charbuf_init(&word);
	for (;;) {
		if (PEEK(iter) == '\\') {
			SKIP(iter);
			if (!PEEK(iter)) {
				fputs("toysh: broken escape\n", stderr);
				charbuf_destroy(&word);
				return NULL;
			}
			charbuf_add(&word, NEXT(iter));
		} else if (PEEK(iter) == '"') {
			if (!lex_word_quote(&word, iter)) {
				charbuf_destroy(&word);
				return NULL;
			}
		} else if (is_ws(PEEK(iter)) || is_sep_char(PEEK(iter)) || !PEEK(iter)) {
			token_list_append(&tokens, token_new(tok_word, word.buf));
			return tokens;
		} else {
			charbuf_add(&word, NEXT(iter));
		}
	}
}

struct token *
lex(const char *line)
{
	struct token *tokens = NULL;
	const char **iter = &line;
	for (;;) {
		struct token *new_tokens;
		/* skip whitespace characters */
		while (is_ws(PEEK(iter))) {
			SKIP(iter);
		}
		if (!PEEK(iter)) {
			token_list_append(&tokens, token_new(tok_eol, NULL));
			return tokens;
		}
		if (is_sep_char(PEEK(iter))) {
			new_tokens = lex_sep(iter);
		} else {
			new_tokens = lex_word(iter);
		}
		if (!new_tokens) {
			token_list_delete(tokens);
			return NULL;
		}
		token_list_append(&tokens, new_tokens);
	}
}
