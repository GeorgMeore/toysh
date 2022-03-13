#ifndef PARSE_INCLUDED
#define PARSE_INCLUDED

#include "lex.h"

struct task {
	int argc;
	char **args;
	int bg;
	struct task *next;
};

void task_list_delete(struct task *tsk);

struct task *parse(const struct token *head);

#endif /* PARSE_INCLUDED */
