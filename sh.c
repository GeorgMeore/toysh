#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "parse.h"
#include "exec.h"

int
main(void)
{
	struct word_item *words;
	char *line;
	while ((line = read_line())) {
		struct task *cmd;
		words = split_line(line);
		cmd = parse_words(words);
		if (cmd) {
			task_exec(cmd);
			task_delete(cmd);
		}
		word_list_delete(words);
		free(line);
	}
}
