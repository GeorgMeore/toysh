#ifndef PARSE_INCLUDED
#define PARSE_INCLUDED

#include "lex.h"

struct task {
	int argc;
	char **argv;
	enum exec_type {
		task_fg,
		task_bg,
	} type;
	struct task_rd {
		char *file;
		int flags;
	} rd[2];
	struct task *next;
};

void task_list_delete(struct task *tsk);

struct task *parse(const struct token *head);

#endif
