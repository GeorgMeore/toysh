#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "util.h"
#include "lex.h"
#include "parse.h"

#define ARGBUFSZ 24

#define PEEK(scanner) (*scanner)
#define SKIP(scanner) ((*scanner) = (*scanner)->next)

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
free_arr(char **arr)
{
	char **tmp;
	for (tmp = arr; *tmp; tmp++) {
		free(*tmp);
	}
	free(arr);
}

static void
task_list_append(struct task **list, struct task *new)
{
	while (*list != NULL) {
		list = &(*list)->next;
	}
	*list = new;
}

static void
task_delete(struct task *tsk)
{
	int i;
	for (i = 0; i < 2; i++) {
		free(tsk->rd[i].file);
	}
	if (tsk->argv) {
		free_arr(tsk->argv);
	}
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
};

static void
argbuf_init(struct argbuf *buf)
{
	buf->argc = 0;
	buf->argv = emalloc(ARGBUFSZ*sizeof(*buf->argv));
	buf->argv[0] = NULL;
}

static void
argbuf_destroy(struct argbuf *buf)
{
	free_arr(buf->argv);
}

static void
argbuf_append(struct argbuf *buf, const char *arg)
{
	if (buf->argc % ARGBUFSZ == ARGBUFSZ - 1) {
		buf->argv = erealloc(buf->argv, (buf->argc+1+ARGBUFSZ)*sizeof(*buf->argv));
	}
	buf->argv[buf->argc++] = str_copy(arg);
	buf->argv[buf->argc] = NULL;
}

static int
argbuf_is_empty(const struct argbuf *buf)
{
	return !buf->argc;
}

/* parse_arguments ::= word {word} */
static int
parse_arguments(struct task *tsk, const struct token **scanner)
{
	struct argbuf args;
	argbuf_init(&args);
	while (PEEK(scanner)->type == tok_word) {
		argbuf_append(&args, PEEK(scanner)->word);
		SKIP(scanner);
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
	case tok_gt: case tok_ggt: case tok_lt:
		return 1;
	default:
		return 0;
	}
}

static int
redirection_fd(enum token_type rd_type)
{
	switch (rd_type) {
	case tok_gt: case tok_ggt:
		return 1;
	default:
		return 0;
	}
}

static int
redirection_flags(enum token_type rd_type)
{
	switch (rd_type) {
	case tok_gt:
		return O_WRONLY|O_CREAT;
	case tok_ggt:
		return O_WRONLY|O_CREAT|O_APPEND;
	default:
		return O_RDONLY;
	}
}

/* redirections ::= {('>' | '>>' | '<') word} */
static int
parse_redirections(struct task *tsk, const struct token **scanner)
{
	while (is_redirection(PEEK(scanner)->type)) {
		int flags = redirection_flags(PEEK(scanner)->type);
		int which = redirection_fd(PEEK(scanner)->type);
		SKIP(scanner);
		if (PEEK(scanner)->type != tok_word) {
			fputs("toysh: broken redirection: no filename\n", stderr);
			return 0;
		}
		if (task_is_redirected(tsk, which)) {
			fputs("toysh: broken redirection: already redirected\n", stderr);
			return 0;
		}
		task_redirect(tsk, which, flags, PEEK(scanner)->word);
		SKIP(scanner);
	}
	return 1;
}

/* terminator ::= '&' | eol */
static int
parse_terminator(struct task *tsk, const struct token **scanner)
{
	if (PEEK(scanner)->type != tok_eol && PEEK(scanner)->type != tok_amp) {
		fprintf(stderr, "toysh: unexpected token: '%s'\n", PEEK(scanner)->word);
		return 0;
	}
	if (PEEK(scanner)->type == tok_amp) {
		tsk->type = task_bg;
		SKIP(scanner);
	}
	return 1;
}

/* command ::= args redirections terminator */
struct task *
parse(const struct token *toks)
{
	struct task *tasks = NULL, *current;
	const struct token **scanner = &toks;
	if (!PEEK(scanner)) {
		return NULL;
	}
	while (PEEK(scanner)->type != tok_eol) {
		current = task_new();
		if (
			parse_arguments(current, scanner) &&
			parse_redirections(current, scanner) &&
			parse_terminator(current, scanner)
		) {
			task_list_append(&tasks, current);
		} else {
			task_list_delete(tasks);
			task_delete(current);
			return NULL;
		}
	}
	return tasks;
}
