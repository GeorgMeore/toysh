#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "util.h"
#include "input.h"

int
read_line(char **lineptr)
{
	int read = 0, size = 0;
	int tty = isatty(1);
	if (tty) {
		fputs("> ", stderr);
	}
	for (*lineptr = NULL; ;read++) {
		int c = getchar();
		if (read >= size - 1) {
			size += 256;
			*lineptr = erealloc(*lineptr, size);
		}
		switch (c) {
		case EOF:
			if (read == 0) {
				free(*lineptr);
				return 0;
			}
			if (tty) {
				clearerr(stdin);
			}
			/* fall through */
		case '\n':
			(*lineptr)[read] = 0;
			return 1;
		default:
			(*lineptr)[read] = c;
		}
	}
}
