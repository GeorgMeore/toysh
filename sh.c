#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "lex.h"
#include "parse.h"
#include "sched.h"

int
main(void)
{
	struct token *tokens;
	char *line;
	while (read_line(&line)) {
		struct task *cmd;
		tokens = tokenize(line);
		cmd = parse(tokens);
		if (cmd) {
			sched(cmd);
			task_delete(cmd);
		}
		token_list_delete(tokens);
		free(line);
	}
}
