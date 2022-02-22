#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lex.h"

struct word_item *
word_item_new(char *word)
{
	struct word_item *w;
	w = malloc(sizeof(*w));
	if (!w)
		return NULL;
	w->word = word;
	w->next = NULL;
	return w;
}

void
word_item_delete(struct word_item *w)
{
	free(w->word);
	free(w);
}

void
word_list_delete(struct word_item *head)
{
	struct word_item *tmp;
	while (head) {
		tmp = head->next;
		word_item_delete(head);
		head = tmp;
	}
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
	word_list_delete(l->head);
	free(l);
}

void
lexer_cut(struct lexer *l)
{
	if (l->buf->bufsz) {
		char *word;
		struct word_item *w;
		word = buffer_get_str(l->buf);
		w = word_item_new(word);
		if (!w) {
			l->state = memory_err;
			return;
		}
		if (l->tail) {
			l->tail->next = w;
			l->tail = w;
		}
		else {
			l->head = w;
			l->tail = w;
		}
	}
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

struct word_item *
lexer_get_list(struct lexer *l)
{
	struct word_item *w;
	w = l->head;
	l->head = NULL;
	l->tail = NULL;
	return w;
}


struct word_item *
split_line(const char *line)
{
	struct lexer *l;
	struct word_item *w;
	l = lexer_new();
	if (!l)
		return NULL;
	for (;; line++) {
		switch (l->state) {
		case finished:
			w = lexer_get_list(l);
			lexer_delete(l);
			return w;
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
