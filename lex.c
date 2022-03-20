#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "lex.h"

#define BUFCAP 256

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
charbuf_init(struct charbuf *cbuf)
{
	cbuf->buf = emalloc(BUFCAP);
	cbuf->buf[0] = 0;
	cbuf->size = 0;
}

static void
charbuf_destroy(struct charbuf *cbuf)
{
	free(cbuf->buf);
}

static void
charbuf_add(struct charbuf *cbuf, char c)
{
	if (cbuf->size % BUFCAP == BUFCAP - 1)
		cbuf->buf = erealloc(cbuf->buf, cbuf->size + 1 + BUFCAP);
	cbuf->buf[cbuf->size++] = c;
	cbuf->buf[cbuf->size] = 0;
}

static int
is_ws(char c)
{
	switch (c) {
	case '\n':
	case '\t':
	case ' ':
		return 1;
	default:
		return 0;
	}
}

static int
is_sep(char c)
{
	switch (c) {
	case '>':
	case '&':
	case '<':
		return 1;
	default:
		return 0;
	}
}

/* try to convert a string to a token */
static int
to_tok(const char *str, enum token_type *type)
{
	if (str_equal(str, "&"))
		*type = tok_amp;
	else if (str_equal(str, "<"))
		*type = tok_lt;
	else if (str_equal(str, ">"))
		*type = tok_gt;
	else if (str_equal(str, ">>"))
		*type = tok_ggt;
	else
		return 0;
	return 1;
}

#define NEXT(x) ((*(x))++)

static char *
lex_sep(const char **lineptr, enum token_type *type)
{
	struct charbuf word;
	charbuf_init(&word);
	while (**lineptr && is_sep(**lineptr)) {
		charbuf_add(&word, **lineptr);
		NEXT(lineptr);
	}
	if (!to_tok(word.buf, type)) {
		fprintf(stderr, "sh: syntax error near '%s'\n", word.buf);
		charbuf_destroy(&word);
		return NULL;
	}
	return word.buf;
}

static int
lex_word_quote(struct charbuf *word, const char **lineptr)
{
	for (;;) {
		switch (**lineptr) {
		case 0:
			fputs("sh: unclosed quote\n", stderr);
			return 0;
		case '\\':
			NEXT(lineptr);
			if (!**lineptr) {
				fputs("sh: broken escape\n", stderr);
				return 0;
			}
			charbuf_add(word, **lineptr);
			NEXT(lineptr);
			break;
		case '"':
			NEXT(lineptr);
			return 1;
		default:
			charbuf_add(word, **lineptr);
			NEXT(lineptr);
		}
	}
}

static char *
lex_word(const char **lineptr)
{
	struct charbuf word;
	charbuf_init(&word);
	for (;;) {
		switch (**lineptr) {
		case '\\':
			NEXT(lineptr);
			if (!**lineptr) {
				fputs("sh: broken escape\n", stderr);
				charbuf_destroy(&word);
				return NULL;
			}
			charbuf_add(&word, **lineptr);
			NEXT(lineptr);
			break;
		case '"':
			NEXT(lineptr);
			if (!lex_word_quote(&word, lineptr)) {
				charbuf_destroy(&word);
				return NULL;
			}
			break;
		default:
			if (is_ws(**lineptr) || is_sep(**lineptr) || !**lineptr)
				return word.buf;
			charbuf_add(&word, **lineptr);
			NEXT(lineptr);
		}
	}
}

struct token *
lex(const char *line)
{
	struct token *tokens = NULL;
	for (;;) {
		enum token_type type;
		char *word;
		struct token *current;
		/* skip whitespace characters */
		while (*line && is_ws(*line))
			line++;
		if (*line == 0)
			return tokens;
		if (is_sep(*line))
			word = lex_sep(&line, &type);
		else {
			type = tok_word;
			word = lex_word(&line);
		}
		if (!word) {
			token_list_delete(tokens);
			return NULL;
		}
		current = token_new(type, word);
		token_list_append(&tokens, current);
	}
}
