#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "parse.h"
#include "exec.h"

struct task *
parse_words(const struct word_item *head)
{
	char **args;
	int i, argc;
	argc = word_list_len(head);
	if (argc < 1)
		return NULL;
	args = malloc((argc + 1) * sizeof(*args));
	if (!args) {
		fputs("error: failed to allocate memory\n", stderr);
		return NULL;
	}
	for (i = 0; i < argc; head = head->next, i++) {
		args[i] = malloc(strlen(head->word) + 1);
		if (!args[i]) {
			int j;
			fputs("error: failed to allocate memory\n", stderr);
			for (j = 0; j < i; j++)
				free(args[j]);
			free(args);
			return NULL;
		}
		strcpy(args[i], head->word);
	}
	args[argc] = NULL;
	return task_new(argc, args);
}
