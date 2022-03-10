#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lex.h"

/* word must be malloc-allocated string
 * ownership of word is transferred here */
struct token *
token_new(enum token_type type, char *word)
{
	struct token *tok;
	tok = malloc(sizeof(*tok));
	if (!tok)
		return NULL;
	tok->type = type;
	tok->word = word;
	tok->next = NULL;
	return tok;
}

void
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

int
token_list_len(const struct token *head)
{
	int len;
	for (len = 0; head; len++)
		head = head->next;
	return len;
}

struct lexer *
lexer_new()
{
	struct lexer *l;
	struct buffer *b;
	l = malloc(sizeof(*l));
	if (!l)
		return NULL;
	b = buffer_new();
	if (!b) {
		free(l);
		return NULL;
	}
	l->state = normal;
	l->prev_state = normal;
	l->buf = b;
	l->head = NULL;
	l->tail = NULL;
	return l;
}

void
lexer_delete(struct lexer *l)
{
	buffer_delete(l->buf);
	token_list_delete(l->head);
	free(l);
}

void
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

void
lexer_cut(struct lexer *lex)
{
	if (lex->buf->bufsz) {
		char *word;
		struct token *tok;
		word = buffer_get_str(lex->buf);
		tok = token_new(tok_word, word);
		if (!tok) {
			lex->state = memory_err;
			return;
		}
		lexer_append_token(lex, tok);
	}
}

void
lexer_handle_separator(struct lexer *lex, char sep)
{
	struct token *tok;
	tok = token_new(sep, NULL);
	if (!tok) {
		lex->state = memory_err;
		return;
	}
	lexer_append_token(lex, tok);
}

void
lexer_step_normal(struct lexer *l, char c)
{
	switch (c) {
	case 0:
		l->state = finished;
		/* fall through */
	case ' ':
	case '\t':
		lexer_cut(l);
		break;
	case '"':
		l->state = quote;
		break;
	case '\\':
		l->prev_state = l->state;
		l->state = escape;
		break;
	case ';':
	case '(':
	case ')':
	case '&':
	case '|':
	case '>':
	case '<':
		lexer_cut(l);
		lexer_handle_separator(l, c);
		break;
	default:
		if (!buffer_add(l->buf, c))
			l->state = memory_err;
	}
}

void
lexer_step_quote(struct lexer *l, char c)
{
	switch (c) {
	case 0:
		l->state = quote_err;
		break;
	case '"':
		l->state = l->buf->bufsz ? normal : empty;
		break;
	case '\\':
		l->prev_state = l->state;
		l->state = escape;
		break;
	default:
		if (!buffer_add(l->buf, c))
			l->state = memory_err;
	}
}

void
lexer_step_empty(struct lexer *l, char c)
{
	switch (c) {
	case 0:
	case ' ':
	case '\t':
		if (!buffer_add(l->buf, 0)) {
			l->state = memory_err;
			return;
		}
		/* fall through */
	default:
		l->state = normal;
		lexer_step_normal(l, c);
	}
}

void
lexer_step_escape(struct lexer *l, char c)
{
	switch (c) {
	case 0:
		l->state = escape_err;
		break;
	default:
		l->state = l->prev_state;
		if (!buffer_add(l->buf, c))
			l->state = memory_err;
	}
}

struct token *
lexer_get_list(struct lexer *l)
{
	struct token *w;
	w = l->head;
	l->head = NULL;
	l->tail = NULL;
	return w;
}


struct token *
tokenize(const char *line)
{
	struct lexer *l;
	struct token *w;
	l = lexer_new();
	if (!l)
		return NULL;
	for (;; line++) {
		switch (l->state) {
		case normal:
			lexer_step_normal(l, *line);
			break;
		case quote:
			lexer_step_quote(l, *line);
			break;
		case empty:
			lexer_step_empty(l, *line);
			break;
		case escape:
			lexer_step_escape(l, *line);
			break;
		case finished:
			w = lexer_get_list(l);
			lexer_delete(l);
			return w;
		case memory_err:
			fputs("error: failed to allocate memory\n", stderr);
			lexer_delete(l);
			return NULL;
		case quote_err:
			fputs("error: unclosed quote\n", stderr);
			lexer_delete(l);
			return NULL;
		case escape_err:
			fputs("error: broken escape\n", stderr);
			lexer_delete(l);
			return NULL;
		}
	}
}

struct buffer *
buffer_new()
{
	struct buffer *b;
	b = malloc(sizeof(*b));
	if (!b)
		return NULL;
	b->bufcap = 0;
	b->bufsz = 0;
	b->buf = NULL;
	return b;
}

void
buffer_delete(struct buffer *b)
{
	free(b->buf);
	free(b);
}

char *
buffer_get_str(struct buffer *b)
{
	char *buf;
	buf = b->buf;
	b->buf = NULL;
	b->bufcap = 0;
	b->bufsz = 0;
	return buf;
}

int
buffer_add(struct buffer *b, char c)
{
	char *tmp;
	/* allocate more memory as needed */
	if (b->bufsz >= b->bufcap - 1) {
		b->bufcap += 256;
		tmp = realloc(b->buf, b->bufcap);
		if (!tmp)
			return 0;
		b->buf = tmp;
	}
	b->buf[b->bufsz] = c;
	b->buf[b->bufsz+1] = 0;
	b->bufsz++;
	return 1;
}

char *
read_line()
{
	struct buffer *b;
	char *line;
	b = buffer_new();
	if (!b)
		return NULL;
	fputs("> ", stderr);
	for (;;) {
		int c, res;
		c = getchar();
		switch (c) {
		case EOF:
			if (!b->bufsz) {
				buffer_delete(b);
				return NULL;
			}
			clearerr(stdin);
			/* fall through */
		case '\n':
			line = buffer_get_str(b);
			buffer_delete(b);
			return line;
		}
		res = buffer_add(b, c);
		if (!res) {
			buffer_delete(b);
			return NULL;
		}
	}
}
