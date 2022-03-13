#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "parse.h"
#include "sched.h"

#define ARGBUF_STEP 24

struct argbuf {
	char **argv;
	int argc;
	int cap;
};

static void
argbuf_init(struct argbuf *buf)
{
	buf->argc = 0;
	buf->argv = NULL;
	buf->cap = 0;
}

static void
argbuf_destroy(struct argbuf *buf)
{
	if (buf->argv) {
		char **tmp;
		for (tmp = buf->argv; *tmp; tmp++)
			free(*tmp);
		free(buf->argv);
	}
}

static int
argbuf_append(struct argbuf *buf, const char *arg)
{
	char **tmp;
	if (buf->argc > buf->cap - 1) {
		buf->cap += ARGBUF_STEP;
		tmp = realloc(buf->argv, buf->cap * sizeof(*buf->argv));
		if (!tmp)
			return 0;
		buf->argv = tmp;
	}
	tmp = buf->argv + buf->argc;
	*tmp = malloc(strlen(arg) + 1);
	if (!*tmp)
		return 0;
	strcpy(*tmp, arg);
	*(tmp + 1) = NULL;
	buf->argc++;
	return 1;
}

static char **
argbuf_get_argv(struct argbuf *buf)
{
	char **tmp;
	tmp = buf->argv;
	buf->argc = 0;
	buf->argv = NULL;
	buf->cap = 0;
	return tmp;
}

static int
argbuf_is_empty(const struct argbuf *buf)
{
	return !buf->argc;
}

struct parser {
	enum parser_state {
		normal,
		finished,
		error
	} state;
	enum parser_error {
		memory_err,
		parsing_err
	} err_type;
	struct argbuf args;
	int bg;
	struct task *head, *tail;
};

static void
parser_init(struct parser *par)
{
	argbuf_init(&par->args);
	par->state = normal;
	par->bg = 0;
	par->head = NULL;
	par->tail = NULL;
}

static void
parser_destroy(struct parser *par)
{
	argbuf_destroy(&par->args);
	task_list_delete(par->head);
}

static void
parser_set_error(struct parser *par, enum parser_error err)
{
	par->state = error;
	par->err_type = err;
}

static void
parser_append_task(struct parser *par, struct task *tsk)
{
	if (par->tail) {
		par->tail->next = tsk;
		par->tail = tsk;
	}
	else {
		par->head = tsk;
		par->tail = tsk;
	}
}

static void
parser_form_task(struct parser *par)
{
	struct task *tsk;
	char **argv;
	if (argbuf_is_empty(&par->args)) {
		parser_set_error(par, parsing_err);
		return;
	}
	tsk = task_new();
	if (!tsk) {
		parser_set_error(par, memory_err);
		return;
	}
	argv = argbuf_get_argv(&par->args);
	task_init(tsk, argv, par->bg);
	parser_append_task(par, tsk);
	par->bg = 0;
}

static struct task *
parser_get_tasks(struct parser *par)
{
	struct task *tasks;
	tasks = par->head;
	par->head = NULL;
	par->tail = NULL;
	return tasks;
}

static void
parser_step_normal(struct parser *par, const struct token *tok)
{
	if (!tok) {
		if (!argbuf_is_empty(&par->args)) {
			parser_form_task(par);
			par->state = finished;
		}
		else if (par->bg)
			parser_set_error(par, parsing_err);
		else
			par->state = finished;
		return;
	}
	switch (tok->type){
	case tok_word:
		if (!argbuf_append(&par->args, tok->word))
			parser_set_error(par, memory_err);
		break;
	case tok_bg:
		par->bg = 1;
		parser_form_task(par);
	}
}

static void
parser_step_error(struct parser *par)
{
	switch (par->err_type) {
	case parsing_err:
		fputs("error: failed to parse\n", stderr);
		break;
	case memory_err:
		fputs("error: failed to allocate memory\n", stderr);
		break;
	}
}

struct task *
parse(const struct token *tok)
{
	struct parser par;
	struct task *tasks;
	parser_init(&par);
	for(;;) {
		switch (par.state) {
		case normal:
			parser_step_normal(&par, tok);
			break;
		case error:
			parser_step_error(&par);
			parser_destroy(&par);
			return NULL;
		case finished:
			tasks = parser_get_tasks(&par);
			parser_destroy(&par);
			return tasks;
		}
		if (tok)
			tok = tok->next;
	}
}
