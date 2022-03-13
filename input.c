#include <stdio.h>
#include <stdlib.h>
#include "input.h"

int
read_line(char **lineptr)
{
	int read = 0;
	int size = 0;
	int c;
	fputs("> ", stderr);
	for (*lineptr = NULL; ;read++) {
		if (read >= size - 1) {
			char *tmp;
			size += 256;
			tmp = realloc(*lineptr, size);
			if (!tmp) {
				free(*lineptr);
				return 0;
			}
			*lineptr = tmp;
		}
		c = getchar();
		switch (c) {
		case EOF:
			if (read == 0) {
				free(*lineptr);
				return 0;
			}
			clearerr(stdin);
			/* fall through */
		case '\n':
			(*lineptr)[read] = 0;
			return 1;
		default:
			(*lineptr)[read] = c;
		}
	}
}
