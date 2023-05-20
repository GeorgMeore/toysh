#ifndef PARSE_INCLUDED
#define PARSE_INCLUDED

#include "lex.h"

struct redirection {
	char *file;
	int flags;
};

enum task_type {
	task_fg,
	task_bg
};

struct task {
	int argc;
	char **argv;
	enum task_type type;
	struct redirection rd[2];
	struct task *next;
};

void task_list_delete(struct task *tsk);

struct task *parse(const struct token *head);

#endif
