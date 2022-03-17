#include <stdlib.h>
#include "input.h"
#include "lex.h"
#include "parse.h"
#include "sched.h"

int
main(void)
{
	char *line;
	while (read_line(&line)) {
		struct token *tokens;
		struct task *tasks;
		tokens = tokenize(line);
		tasks = parse(tokens);
		sched(tasks);
		task_list_delete(tasks);
		token_list_delete(tokens);
		free(line);
	}
	return 0;
}
