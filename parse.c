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
	tsk->rd[0].file = NULL;
	tsk->rd[1].file = NULL;
	tsk->next = NULL;
	return tsk;
}

static int
task_is_redirected(struct task *tsk, int which)
{
	return tsk->rd[which].file != NULL;
}

static void
task_redirect(struct task *tsk, int which, int flags, const char *file)
{
	tsk->rd[which].file = str_copy(file);
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
task_list_append(struct task **list, struct task *new)
{
	while (*list != NULL)
		list = &(*list)->next;
	*list = new;
}

static void
task_delete(struct task *tsk)
{
	int i;
	for (i = 0; i < 2; i++)
		free(tsk->rd[i].file);
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

static int
argbuf_is_empty(const struct argbuf *buf)
{
	return !buf->argc;
}

/* parse command and arguments */
static int
parse_cmd(struct task *tsk, const struct token **tok)
{
	struct argbuf args;
	argbuf_init(&args);
	while (*tok && (*tok)->type == tok_word) {
		argbuf_append(&args, (*tok)->word);
		*tok = (*tok)->next;
	}
	if (argbuf_is_empty(&args)) {
		fputs("toysh: no command\n", stderr);
		argbuf_destroy(&args);
		return 0;
	}
	tsk->argc = args.argc;
	tsk->argv = args.argv;
	return 1;
}

static int
is_redirection(enum token_type type)
{
	switch (type) {
	case tok_gt:
	case tok_ggt:
	case tok_lt:
		return 1;
	default:
		return 0;
	}
}

static void
to_flags(enum token_type rd_type, int *flags, int *which)
{
	switch (rd_type) {
	case tok_gt:
		*flags = O_WRONLY|O_CREAT;
		*which = 1;
		return;
	case tok_ggt:
		*flags = O_WRONLY|O_CREAT|O_APPEND;
		*which = 1;
		return;
	case tok_lt:
		*flags = O_RDONLY;
		*which = 0;
		return;
	default: /* this should never happen */
	}
}

/* parse output rediretions */
static int
parse_rd(struct task *tsk, const struct token **tok)
{
	while ((*tok) && is_redirection((*tok)->type)) {
		int flags, which;
		to_flags((*tok)->type, &flags, &which);
		(*tok) = (*tok)->next;
		if (!(*tok) || (*tok)->type != tok_word) {
			fputs("toysh: broken redirection: no filename\n", stderr);
			return 0;
		}
		if (task_is_redirected(tsk, which)) {
			fputs("toysh: broken redirection: already redirected\n", stderr);
			return 0;
		}
		task_redirect(tsk, which, flags, (*tok)->word);
		(*tok) = (*tok)->next;
	}
	return 1;
}

/* parse terminator tokens (there is only '&' for now) */
static int
parse_term(struct task *tsk, const struct token **tok)
{
	if ((*tok) && (*tok)->type != tok_amp) {
		fprintf(stderr, "toysh: unexpected token: '%s'\n", (*tok)->word);
		return 0;
	}
	if ((*tok) && (*tok)->type == tok_amp) {
		tsk->type = task_bg;
		(*tok) = (*tok)->next;
	}
	return 1;
}

struct task *
parse(const struct token *tok)
{
	struct task *tasks = NULL, *current;
	while (tok) {
		current = task_new();
		if (!parse_cmd(current, &tok))
			goto fail;
		if (!parse_rd(current, &tok))
			goto fail;
		if (!parse_term(current, &tok))
			goto fail;
		task_list_append(&tasks, current);
	}
	return tasks;
fail:
	task_list_delete(tasks);
	task_delete(current);
	return NULL;
}
