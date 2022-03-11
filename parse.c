#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "parse.h"
#include "sched.h"

#define ARGBUF_STEP 24

struct argbuf {
	int argc;
	char **argv;
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
		int i;
		for (i = 0; buf->argv[i]; i++)
			free(buf->argv[i]);
		free(buf->argv);
	}
}

static int
argbuf_append(struct argbuf *args, const char *arg)
{
	char **tmp;
	if (args->argc > args->cap - 1) {
		args->cap += ARGBUF_STEP;
		tmp = realloc(args->argv, args->cap * sizeof(*args->argv));
		if (!tmp)
			return 0;
		args->argv = tmp;
	}
	tmp = args->argv + args->argc;
	*tmp = malloc(strlen(arg) + 1);
	if (!*tmp)
		return 0;
	strcpy(*tmp, arg);
	*(tmp + 1) = NULL;
	args->argc++;
	return 1;
}

static char **
argbuf_get_argv(struct argbuf *args)
{
	char **tmp;
	tmp = args->argv;
	args->argc = 0;
	args->argv = NULL;
	args->cap = 0;
	return tmp;
}

struct task *
parse(const struct token *head)
{
	struct argbuf args;
	struct task *cmd;
	int bg = 0;
	argbuf_init(&args);
	while (head) {
		switch (head->type) {
		case tok_word:
			if (!argbuf_append(&args, head->word)) {
				argbuf_destroy(&args);
				return NULL;
			}
			break;
		case tok_bg:
			if (head->next) {
				argbuf_destroy(&args);
				fputs("error: & should be last\n", stderr);
				return NULL;
			}
			bg = 1;
		}
		head = head->next;
	}
	if (args.argc < 1)
		cmd = NULL;
	else {
		int argc = args.argc;
		char **argv = argbuf_get_argv(&args);
		cmd = task_new(argc, argv, bg);
	}
	argbuf_destroy(&args);
	return cmd;
}
