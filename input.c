#include <stdio.h>
#include <stdlib.h>
#include "input.h"

struct charbuf *
charbuf_new()
{
	struct charbuf *buf;
	buf = malloc(sizeof(*buf));
	if (!buf)
		return NULL;
	buf->cap = 0;
	buf->size = 0;
	buf->chars = NULL;
	return buf;
}

void
charbuf_delete(struct charbuf *buf)
{
	free(buf->chars);
	free(buf);
}

char *
charbuf_get_str(struct charbuf *buf)
{
	char *tmp;
	tmp = buf->chars;
	buf->chars = NULL;
	buf->cap = 0;
	buf->size = 0;
	return tmp;
}

int
charbuf_add(struct charbuf *buf, char c)
{
	char *tmp;
	/* allocate more memory as needed */
	if (buf->size >= buf->cap - 1) {
		buf->cap += 256;
		tmp = realloc(buf->chars, buf->cap);
		if (!tmp)
			return 0;
		buf->chars = tmp;
	}
	buf->chars[buf->size] = c;
	buf->chars[buf->size+1] = 0;
	buf->size++;
	return 1;
}

int
charbuf_is_empty(const struct charbuf *buf)
{
	return !buf->size;
}

int
read_line(char **lineptr)
{
	struct charbuf *buf;
	buf = charbuf_new();
	if (!buf)
		return 0;
	fputs("> ", stderr);
	for (;;) {
		int c;
		c = getchar();
		switch (c) {
		case EOF:
			if (charbuf_is_empty(buf)) {
				charbuf_delete(buf);
				return 0;
			}
			clearerr(stdin);
			/* fall through */
		case '\n':
			if (charbuf_is_empty(buf)) {
				/* need to return empty, but not NULL */
				if (!charbuf_add(buf, 0)) {
					charbuf_delete(buf);
					return 0;
				}
			}
			*lineptr = charbuf_get_str(buf);
			charbuf_delete(buf);
			return 1;
		}
		if (!charbuf_add(buf, c)) {
			charbuf_delete(buf);
			return 0;
		}
	}
}
