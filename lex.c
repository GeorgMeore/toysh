#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "input.h"
#include "lex.h"

/* word must be malloc-allocated string
 * ownership of word is transferred here */
static struct token *
token_new(enum token_type type, char *word)
{
	struct token *tok;
	tok = emalloc(sizeof(*tok));
	if (!tok)
		return NULL;
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
	char *chars;
	int cap;
	int size;
};

static void
charbuf_init(struct charbuf *buf)
{
	buf->cap = 0;
	buf->size = 0;
	buf->chars = NULL;
}

static void
charbuf_destroy(struct charbuf *buf)
{
	free(buf->chars);
}

static char *
charbuf_get_str(struct charbuf *buf)
{
	char *tmp;
	tmp = buf->chars;
	buf->chars = NULL;
	buf->cap = 0;
	buf->size = 0;
	return tmp;
}

static int
charbuf_add(struct charbuf *buf, char c)
{
	char *tmp;
	/* allocate more memory as needed */
	if (buf->size >= buf->cap - 1) {
		buf->cap += 256;
		tmp = erealloc(buf->chars, buf->cap);
		if (!tmp)
			return 0;
		buf->chars = tmp;
	}
	buf->chars[buf->size] = c;
	buf->chars[buf->size+1] = 0;
	buf->size++;
	return 1;
}

static int
charbuf_is_empty(const struct charbuf *buf)
{
	return !buf->size;
}

struct lexer {
	enum lexer_state {
		word,
		separator,
		quote,
		empty_quote,
		escape,
		finished,
		error
	} state, prev_state;
	struct charbuf buf;
	struct token *head, *tail;
};

static void
lexer_init(struct lexer *lex)
{
	charbuf_init(&lex->buf);
	lex->state = word;
	lex->prev_state = word;
	lex->head = NULL;
	lex->tail = NULL;
}

static void
lexer_destroy(struct lexer *lex)
{
	charbuf_destroy(&lex->buf);
	token_list_delete(lex->head);
}

static void
lexer_append_token(struct lexer *lex, struct token *tok)
{
	if (lex->tail) {
		lex->tail->next = tok;
		lex->tail = tok;
	}
	else {
		lex->head = tok;
		lex->tail = tok;
	}
}

static void
lexer_handle_word_end(struct lexer *lex)
{
	if (!charbuf_is_empty(&lex->buf)) {
		char *word;
		struct token *tok;
		word = charbuf_get_str(&lex->buf);
		tok = token_new(tok_word, word);
		if (!tok) {
			lex->state = error;
			return;
		}
		lexer_append_token(lex, tok);
	}
}

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

static void
lexer_handle_separator_end(struct lexer *lex)
{
	struct token *tok;
	enum token_type type;
	char *sepstr;
	int valid;
	sepstr = charbuf_get_str(&lex->buf);
	valid = to_tok(sepstr, &type);
	if (!valid) {
		fprintf(stderr, "sh: syntax error near '%s'\n", sepstr);
		free(sepstr);
		lex->state = error;
		return;
	}
	tok = token_new(type, sepstr);
	if (!tok) {
		free(sepstr);
		lex->state = error;
		return;
	}
	lexer_append_token(lex, tok);
}

static int
is_sep_char(char c)
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

static void
lexer_step_word(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
		lex->state = finished;
		/* fall through */
	case ' ':
	case '\t':
		lexer_handle_word_end(lex);
		break;
	case '"':
		lex->state = quote;
		break;
	case '\\':
		lex->prev_state = lex->state;
		lex->state = escape;
		break;
	default:
		if (is_sep_char(c)) {
			lexer_handle_word_end(lex);
			if (lex->state == error)
				return;
			lex->state = separator;
		}
		if (!charbuf_add(&lex->buf, c))
			lex->state = error;
	}
}

static void
lexer_step_separator(struct lexer *lex, char c)
{
	if (is_sep_char(c)) {
		if (!charbuf_add(&lex->buf, c))
			lex->state = error;
	}
	else {
		lexer_handle_separator_end(lex);
		if (lex->state == error)
			return;
		lex->state = word;
		lexer_step_word(lex, c);
	}
}

static void
lexer_step_quote(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
		fputs("sh: unclosed quote\n", stderr);
		lex->state = error;
		break;
	case '"':
		if (charbuf_is_empty(&lex->buf))
			lex->state = empty_quote;
		else
			lex->state = word;
		break;
	case '\\':
		lex->prev_state = lex->state;
		lex->state = escape;
		break;
	default:
		if (!charbuf_add(&lex->buf, c))
			lex->state = error;
	}
}

static void
lexer_step_empty_quote(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
	case ' ':
	case '\t':
		if (!charbuf_add(&lex->buf, 0)) {
			lex->state = error;
			return;
		}
		/* fall through */
	default:
		lex->state = word;
		lexer_step_word(lex, c);
	}
}

static void
lexer_step_escape(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
		fputs("sh: broken escape\n", stderr);
		lex->state = error;
		break;
	default:
		lex->state = lex->prev_state;
		if (!charbuf_add(&lex->buf, c))
			lex->state = error;
	}
}

static struct token *
lexer_get_tokens(struct lexer *lex)
{
	struct token *tok;
	tok = lex->head;
	lex->head = NULL;
	lex->tail = NULL;
	return tok;
}

struct token *
tokenize(const char *line)
{
	struct lexer lex;
	struct token *toks;
	lexer_init(&lex);
	for (;; line++) {
		switch (lex.state) {
		case word:
			lexer_step_word(&lex, *line);
			break;
		case separator:
			lexer_step_separator(&lex, *line);
			break;
		case quote:
			lexer_step_quote(&lex, *line);
			break;
		case empty_quote:
			lexer_step_empty_quote(&lex, *line);
			break;
		case escape:
			lexer_step_escape(&lex, *line);
			break;
		case finished:
			toks = lexer_get_tokens(&lex);
			lexer_destroy(&lex);
			return toks;
		case error:
			lexer_destroy(&lex);
			return NULL;
		}
	}
}
