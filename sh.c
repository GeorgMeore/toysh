#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "read.h"
#include "lex.h"
#include "parse.h"
#include "exec.h"

int
main(void)
{
	struct token *tokens;
	char *line;
	while ((line = read_line())) {
		struct task *cmd;
		tokens = tokenize(line);
		cmd = parse(tokens);
		if (cmd) {
			task_exec(cmd);
			task_delete(cmd);
		}
		token_list_delete(tokens);
		free(line);
	}
}
