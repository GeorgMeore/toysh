#include <stdio.h>
#include <stdlib.h>
#include "lex.h"

void word_list_print(const struct word_item *head)
{
	while (head) {
		printf("[%s]\n", head->word);
		head = head->next;
	}
}

int main()
{
	struct word_item *words;
	char *line;
	while ((line = read_line())) {
		words = split_line(line);
		word_list_print(words);
		word_list_delete(words);
		free(line);
	}
}
