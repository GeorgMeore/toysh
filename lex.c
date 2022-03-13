#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "input.h"
#include "lex.h"

struct lexer {
	enum lexer_state {
		normal,
		quote,
		empty,
		escape,
		finished,
		error
	} state, prev_state;
	enum lexer_error {
		memory_err,
		quote_err,
		escape_err
	} err_type;
	struct charbuf buf;
	struct token *head, *tail;
};

/* word must be malloc-allocated string
 * ownership of word is transferred here */
static struct token *
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

static void
lexer_init(struct lexer *lex)
{
	charbuf_init(&lex->buf);
	lex->state = normal;
	lex->prev_state = normal;
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
lexer_set_err(struct lexer *lex, enum lexer_error type)
{
	lex->state = error;
	lex->err_type = type;
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
			lexer_set_err(lex, memory_err);
			return;
		}
		lexer_append_token(lex, tok);
	}
}

static void
lexer_handle_separator(struct lexer *lex, char sep)
{
	struct token *tok;
	tok = token_new(sep, NULL);
	if (!tok) {
		lexer_set_err(lex, memory_err);
		return;
	}
	lexer_append_token(lex, tok);
}

static void
lexer_step_normal(struct lexer *lex, char c)
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
	case '&':
		lexer_handle_word_end(lex);
		lexer_handle_separator(lex, c);
		break;
	default:
		if (!charbuf_add(&lex->buf, c))
			lexer_set_err(lex, memory_err);
	}
}

static void
lexer_step_quote(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
		lexer_set_err(lex, quote_err);
		break;
	case '"':
		lex->state = charbuf_is_empty(&lex->buf) ? normal : empty;
		break;
	case '\\':
		lex->prev_state = lex->state;
		lex->state = escape;
		break;
	default:
		if (!charbuf_add(&lex->buf, c))
			lexer_set_err(lex, memory_err);
	}
}

static void
lexer_step_empty(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
	case ' ':
	case '\t':
		if (!charbuf_add(&lex->buf, 0)) {
			lexer_set_err(lex, memory_err);
			return;
		}
		/* fall through */
	default:
		lex->state = normal;
		lexer_step_normal(lex, c);
	}
}

static void
lexer_step_escape(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
		lexer_set_err(lex, escape_err);
		break;
	default:
		lex->state = lex->prev_state;
		if (!charbuf_add(&lex->buf, c))
			lexer_set_err(lex, memory_err);
	}
}

static void
lexer_step_error(struct lexer *lex)
{
	switch (lex->err_type) {
	case memory_err:
		fputs("error: failed to allocate memory\n", stderr);
		return;
	case quote_err:
		fputs("error: unclosed quote\n", stderr);
		return;
	case escape_err:
		fputs("error: broken escape\n", stderr);
		return;
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
		case normal:
			lexer_step_normal(&lex, *line);
			break;
		case quote:
			lexer_step_quote(&lex, *line);
			break;
		case empty:
			lexer_step_empty(&lex, *line);
			break;
		case escape:
			lexer_step_escape(&lex, *line);
			break;
		case finished:
			toks = lexer_get_tokens(&lex);
			lexer_destroy(&lex);
			return toks;
		case error:
			lexer_step_error(&lex);
			lexer_destroy(&lex);
			return NULL;
		}
	}
}
