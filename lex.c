#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "input.h"
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
	char *buf;
	int bufsize;
	int bufcap;
	struct token *head, *tail;
};

static void
lexer_init(struct lexer *lex)
{
	lex->state = word;
	lex->prev_state = word;
	lex->buf = emalloc(BUFCAP);
	lex->buf[0] = 0;
	lex->bufsize = 0;
	lex->bufcap = BUFCAP;
	lex->head = NULL;
	lex->tail = NULL;
}

static void
lexer_destroy(struct lexer *lex)
{
	free(lex->buf);
	token_list_delete(lex->head);
}

static void
lexer_buf_add(struct lexer *lex, char c)
{
	/* allocate more memory as needed */
	if (lex->bufsize >= lex->bufcap - 1) {
		char *tmp;
		lex->bufcap += BUFCAP;
		tmp = erealloc(lex->buf, lex->bufcap);
		lex->buf = tmp;
	}
	lex->buf[lex->bufsize++] = c;
	lex->buf[lex->bufsize] = 0; /* always keep valid string in buffer */
}

static void
lexer_buf_reset(struct lexer *lex)
{
	lex->bufsize = 0;
	lex->buf[0] = 0; /* always keep valid string in buffer */
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
lexer_form_word(struct lexer *lex)
{
	char *word;
	struct token *tok;
	word = str_copy(lex->buf);
	lexer_buf_reset(lex);
	tok = token_new(tok_word, word);
	return lexer_append_token(lex, tok);
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

static void
lexer_form_separator(struct lexer *lex)
{
	struct token *tok;
	enum token_type type;
	char *sepstr;
	sepstr = str_copy(lex->buf);
	lexer_buf_reset(lex);
	if (!to_tok(sepstr, &type)) {
		fprintf(stderr, "sh: syntax error near '%s'\n", sepstr);
		free(sepstr);
		lex->state = error;
		return;
	}
	tok = token_new(type, sepstr);
	return lexer_append_token(lex, tok);
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
		if (lex->bufsize)
			lexer_form_word(lex);
		return;
	case '"':
		lex->state = quote;
		break;
	case '\\':
		lex->prev_state = lex->state;
		lex->state = escape;
		break;
	default:
		if (is_sep_char(c)) {
			if (lex->bufsize)
				lexer_form_word(lex);
			lex->state = separator;
		}
		return lexer_buf_add(lex, c);
	}
}

static void
lexer_step_separator(struct lexer *lex, char c)
{
	if (is_sep_char(c))
		return lexer_buf_add(lex, c);
	else {
		lexer_form_separator(lex);
		if (lex->state == error)
			return;
		lex->state = word;
		return lexer_step_word(lex, c);
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
		lex->state = lex->bufsize ? word : empty_quote;
		break;
	case '\\':
		lex->prev_state = lex->state;
		lex->state = escape;
		break;
	default:
		return lexer_buf_add(lex, c);
	}
}

static void
lexer_step_empty_quote(struct lexer *lex, char c)
{
	switch (c) {
	case 0:
	case ' ':
	case '\t':
		lexer_form_word(lex);
		/* fall through */
	default:
		lex->state = word;
		return lexer_step_word(lex, c);
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
		return lexer_buf_add(lex, c);
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
