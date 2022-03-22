#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "lex.h"

#define CHRBUFSZ 256

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
	cbuf->buf = emalloc(CHRBUFSZ);
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
	if (cbuf->size % CHRBUFSZ == CHRBUFSZ - 1)
		cbuf->buf = erealloc(cbuf->buf, cbuf->size + 1 + CHRBUFSZ);
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

static int
is_name(char c)
{
	if (c > 'a' && c < 'z')
		return 1;
	if (c > 'A' && c < 'Z')
		return 1;
	if (c > '0' && c < '9')
		return 1;
	if (c == '_')
		return 1;
	return 0;
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

struct token *
lex_sep(const char **lineptr)
{
	struct charbuf word;
	enum token_type type;
	charbuf_init(&word);
	while (**lineptr && is_sep(**lineptr)) {
		charbuf_add(&word, **lineptr);
		NEXT(lineptr);
	}
	if (!to_tok(word.buf, &type)) {
		fprintf(stderr, "toysh: syntax error near '%s'\n", word.buf);
		charbuf_destroy(&word);
		return NULL;
	}
	return token_new(type, word.buf);
}

static int
lex_var(const char **lineptr, char **valptr)
{
	struct charbuf varname;
	NEXT(lineptr); /* skip [ */
	charbuf_init(&varname);
	for (;;) {
		if (**lineptr == ']') {
			*valptr = getenv(varname.buf);
			NEXT(lineptr); /* skip ] */
			charbuf_destroy(&varname);
			return 1;
		}
		if (!is_name(**lineptr)) {
			fputs("toysh: broken variable expansion\n", stderr);
			charbuf_destroy(&varname);
			return 0;
		} else {
			charbuf_add(&varname, **lineptr);
			NEXT(lineptr);
		}
	}
}

static int
lex_word_quote(struct charbuf *word, const char **lineptr)
{
	NEXT(lineptr); /* skip starting " */
	for (;;) {
		if (!**lineptr) {
			fputs("toysh: unclosed quote\n", stderr);
			return 0;
		} else if (**lineptr == '\\') {
			NEXT(lineptr);
			if (!**lineptr) {
				fputs("toysh: broken escape\n", stderr);
				return 0;
			}
			charbuf_add(word, **lineptr);
			NEXT(lineptr);
		} else if (**lineptr == '"') {
			NEXT(lineptr); /* skip closing " */
			return 1;
		} else if (**lineptr == '[') {
			char *value;
			if (!lex_var(lineptr, &value))
				return 0;
			for (; value && *value; value++)
				charbuf_add(word, *value);
		} else {
			charbuf_add(word, **lineptr);
			NEXT(lineptr);
		}
	}
}

static struct token *
lex_word(const char **lineptr)
{
	struct token *tokens = NULL;
	struct charbuf word;
	charbuf_init(&word);
	for (;;) {
		if (**lineptr == '\\') {
			NEXT(lineptr);
			if (!**lineptr) {
				fputs("toysh: broken escape\n", stderr);
				goto fail;
			}
			charbuf_add(&word, **lineptr);
			NEXT(lineptr);
		} else if (**lineptr == '"') {
			if (!lex_word_quote(&word, lineptr))
				goto fail;
		} else if (**lineptr == '[') {
			char *value;
			if (!lex_var(lineptr, &value))
				goto fail;
			for (; value && *value; value++)
				if (is_ws(*value) && word.size)
					token_list_append(&tokens, token_new(tok_word, word.buf));
				else
					charbuf_add(&word, *value);
		} else if (is_ws(**lineptr) || is_sep(**lineptr) || !**lineptr) {
			token_list_append(&tokens, token_new(tok_word, word.buf));
			return tokens;
		} else {
			charbuf_add(&word, **lineptr);
			NEXT(lineptr);
		}
	}
fail:
	charbuf_destroy(&word);
	return NULL;
}

struct token *
lex(const char *line)
{
	struct token *tokens = NULL;
	for (;;) {
		struct token *new_tokens;
		/* skip whitespace characters */
		while (*line && is_ws(*line))
			line++;
		if (*line == 0)
			return tokens;
		if (is_sep(*line))
			new_tokens = lex_sep(&line);
		else
			new_tokens = lex_word(&line);
		if (!new_tokens) {
			token_list_delete(tokens);
			return NULL;
		}
		token_list_append(&tokens, new_tokens);
	}
}
