#include <stdio.h>
#include <stdlib.h>
#include "read.h"

struct buffer *
buffer_new()
{
	struct buffer *b;
	b = malloc(sizeof(*b));
	if (!b)
		return NULL;
	b->bufcap = 0;
	b->bufsz = 0;
	b->buf = NULL;
	return b;
}

void
buffer_delete(struct buffer *b)
{
	free(b->buf);
	free(b);
}

char *
buffer_get_str(struct buffer *b)
{
	char *buf;
	buf = b->buf;
	b->buf = NULL;
	b->bufcap = 0;
	b->bufsz = 0;
	return buf;
}

int
buffer_add(struct buffer *b, char c)
{
	char *tmp;
	/* allocate more memory as needed */
	if (b->bufsz >= b->bufcap - 1) {
		b->bufcap += 256;
		tmp = realloc(b->buf, b->bufcap);
		if (!tmp)
			return 0;
		b->buf = tmp;
	}
	b->buf[b->bufsz] = c;
	b->buf[b->bufsz+1] = 0;
	b->bufsz++;
	return 1;
}

char *
read_line()
{
	struct buffer *b;
	char *line;
	b = buffer_new();
	if (!b)
		return NULL;
	fputs("> ", stderr);
	for (;;) {
		int c, res;
		c = getchar();
		switch (c) {
		case EOF:
			if (!b->bufsz) {
				buffer_delete(b);
				return NULL;
			}
			clearerr(stdin);
			/* fall through */
		case '\n':
			line = buffer_get_str(b);
			buffer_delete(b);
			return line;
		}
		res = buffer_add(b, c);
		if (!res) {
			buffer_delete(b);
			return NULL;
		}
	}
}
