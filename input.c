#include <stdio.h>
#include <stdlib.h>
#include "input.h"

#define CHARBUF_STEP 256

void
charbuf_init(struct charbuf *buf)
{
	buf->cap = 0;
	buf->size = 0;
	buf->chars = NULL;
}

void
charbuf_destroy(struct charbuf *buf)
{
	free(buf->chars);
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
		buf->cap += CHARBUF_STEP;
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
	struct charbuf buf;
	charbuf_init(&buf);
	fputs("> ", stderr);
	for (;;) {
		int c;
		c = getchar();
		switch (c) {
		case EOF:
			if (charbuf_is_empty(&buf)) {
				charbuf_destroy(&buf);
				return 0;
			}
			clearerr(stdin);
			/* fall through */
		case '\n':
			if (charbuf_is_empty(&buf))
				/* need to return empty, but not NULL */
				if (!charbuf_add(&buf, 0)) {
					charbuf_destroy(&buf);
					return 0;
				}
			*lineptr = charbuf_get_str(&buf);
			charbuf_destroy(&buf);
			return 1;
		}
		if (!charbuf_add(&buf, c)) {
			charbuf_destroy(&buf);
			return 0;
		}
	}
}
