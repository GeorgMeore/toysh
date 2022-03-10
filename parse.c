#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "parse.h"
#include "exec.h"

struct argbuf {
	int argc;
	char **argv;
	int cap;
};

struct argbuf *
argbuf_new()
{
	struct argbuf *arg;
	arg = malloc(sizeof(*arg));
	if (!arg)
		return NULL;
	arg->argc = 0;
	arg->argv = NULL;
	arg->cap = 0;
	return arg;
}

void
argbuf_delete(struct argbuf *args)
{
	if (args->argv) {
		int i;
		for (i = 0; args->argv[i]; i++)
			free(args->argv[i]);
		free(args->argv);
	}
	free(args);
}

int
argbuf_append(struct argbuf *args, const char *arg)
{
	char **tmp;
	if (args->argc > args->cap - 1) {
		args->cap += 24;
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

char **
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
	struct argbuf *args;
	int argc;
	char **argv;
	args = argbuf_new();
	if (!args)
		return NULL;
	while (head) {
		switch (head->type) {
		case tok_word:
			if (!argbuf_append(args, head->word)) {
				argbuf_delete(args);
				return NULL;
			}
			break;
		default:
			fputs("error: not implemented!\n", stderr);
			return NULL;
		}
		head = head->next;
	}
	argc = args->argc;
	argv = argbuf_get_argv(args);
	argbuf_delete(args);
	if (argc < 1)
		return NULL;
	return task_new(argc, argv);
}
