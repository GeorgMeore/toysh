#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "util.h"
#include "lex.h"
#include "parse.h"

static struct task *
task_new(void)
{
	struct task *tsk;
	tsk = emalloc(sizeof(*tsk));
	tsk->argc = 0;
	tsk->argv = NULL;
	tsk->type = task_fg;
	tsk->rd[0].name = NULL;
	tsk->rd[1].name = NULL;
	tsk->next = NULL;
	return tsk;
}

static int
task_is_redirected(struct task *tsk, int which)
{
	return tsk->rd[which].name != NULL;
}

static void
task_redirect(struct task *tsk, int which, int flags, const char *name)
{
	tsk->rd[which].name = str_copy(name);
	tsk->rd[which].flags = flags;
}

static void
argdelete(char **args)
{
	char **tmp;
	for (tmp = args; *tmp; tmp++)
		free(*tmp);
	free(args);
}

static void
task_delete(struct task *tsk)
{
	int i;
	for (i = 0; i < 2; i++)
		free(tsk->rd[i].name);
	if (tsk->argv)
		argdelete(tsk->argv);
	free(tsk);
}

void
task_list_delete(struct task *head)
{
	struct task *tmp;
	while (head) {
		tmp = head->next;
		task_delete(head);
		head = tmp;
	}
}

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
	if (buf->argv)
		argdelete(buf->argv);
}

static void
argbuf_append(struct argbuf *buf, const char *arg)
{
	char **tmp;
	if (buf->argc > buf->cap - 1) {
		buf->cap += 24;
		buf->argv = erealloc(buf->argv, buf->cap * sizeof(*buf->argv));
	}
	tmp = buf->argv + buf->argc;
	tmp[0] = str_copy(arg);
	tmp[1] = NULL;
	buf->argc++;
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
		command,
		redirection,
		finished,
		error
	} state;
	struct argbuf args;
	int rd_which, rd_flags;
	struct task *current; /* currently formed task */
	struct task *head, *tail;
};

static void
parser_init(struct parser *par)
{
	argbuf_init(&par->args);
	par->state = command;
	par->head = NULL;
	par->tail = NULL;
	par->current = task_new();
}

static void
parser_destroy(struct parser *par)
{
	argbuf_destroy(&par->args);
	task_delete(par->current);
	task_list_delete(par->head);
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
	char **argv;
	argv = argbuf_get_argv(&par->args);
	par->current->argc = par->args.argc;
	par->current->argv = argv;
	parser_append_task(par, par->current);
	par->current = task_new();
}

static void
parser_step_redirection(struct parser *par, const struct token *tok)
{
	if (!tok) {
		fputs("sh: broken redirection: unexpected newline\n", stderr);
		par->state = error;
		return;
	}
	switch (tok->type) {
		case tok_word:
			if (task_is_redirected(par->current, par->rd_which)) {
				fputs("sh: broken redirection: already redirected\n", stderr);
				par->state = error;
				return;
			}
			task_redirect(par->current, par->rd_which, par->rd_flags, tok->word);
			par->state = command;
			break;
		default:
			fputs("sh: broken redirection: no filename\n", stderr);
			par->state = error;
	}
}

static void
parser_step_command(struct parser *par, const struct token *tok)
{
	if (!tok) {
		par->state = finished;
		if (!argbuf_is_empty(&par->args))
			parser_form_task(par);
		return;
	}
	switch (tok->type) {
	case tok_word:
		argbuf_append(&par->args, tok->word);
		break;
	case tok_amp:
		if (argbuf_is_empty(&par->args)) {
			fprintf(stderr, "sh: parsing error near '%s'\n", tok->word);
			par->state = error;
			return;
		}
		par->current->type = task_bg;
		parser_form_task(par);
		break;
	case tok_gt:
		par->state = redirection;
		par->rd_which = 1;
		par->rd_flags = O_WRONLY|O_CREAT;
		break;
	case tok_ggt:
		par->state = redirection;
		par->rd_which = 1;
		par->rd_flags = O_WRONLY|O_CREAT|O_APPEND;
		break;
	case tok_lt:
		par->state = redirection;
		par->rd_which = 0;
		par->rd_flags = O_RDONLY;
		break;
	}
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

struct task *
parse(const struct token *tok)
{
	struct parser par;
	struct task *tasks;
	parser_init(&par);
	for(;;) {
		switch (par.state) {
		case command:
			parser_step_command(&par, tok);
			break;
		case redirection:
			parser_step_redirection(&par, tok);
			break;
		case finished:
			tasks = parser_get_tasks(&par);
			parser_destroy(&par);
			return tasks;
		case error:
			parser_destroy(&par);
			return NULL;
		}
		if (tok)
			tok = tok->next;
	}
}
