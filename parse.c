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
		*list = (*list)->next;
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

static void
argbuf_reset(struct argbuf *buf)
{
	buf->argc = 0;
	buf->argv = NULL;
	buf->cap = 0;
}

static int
argbuf_is_empty(const struct argbuf *buf)
{
	return !buf->argc;
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

struct task *
parse(const struct token *tok)
{
	struct task *tasks = NULL, *current;
	struct argbuf args;
	argbuf_init(&args);
	while (tok) {
		current = task_new();
		/* process command arguments */
		while (tok && tok->type == tok_word) {
			argbuf_append(&args, tok->word);
			tok = tok->next;
		}
		if (argbuf_is_empty(&args)) {
			fputs("sh: no command\n", stderr);
			goto fail;
		}
		current->argc = args.argc;
		current->argv = args.argv;
		argbuf_reset(&args);
		/* process redirections */
		while (tok && is_redirection(tok->type)) {
			int flags, which;
			to_flags(tok->type, &flags, &which);
			tok = tok->next;
			if (!tok || tok->type != tok_word) {
				fputs("sh: broken redirection: no filename\n", stderr);
				goto fail;
			}
			if (task_is_redirected(current, which)) {
				fputs("sh: broken redirection: already redirected\n", stderr);
				goto fail;
			}
			task_redirect(current, which, flags, tok->word);
			tok = tok->next;
		}
		/* process command separators */
		if (tok && tok->type != tok_amp) {
			fprintf(stderr, "sh: unexpected token: '%s'\n", tok->word);
			goto fail;
		}
		if (tok && tok->type == tok_amp) {
			current->type = task_bg;
			tok = tok->next;
		}
		task_list_append(&tasks, current);
	}
	argbuf_destroy(&args);
	return tasks;
fail:
	argbuf_destroy(&args);
	task_list_delete(tasks);
	task_delete(current);
	return NULL;
}
