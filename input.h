#ifndef READ_INCLUDED
#define READ_INCLUDED

struct charbuf {
	char *chars;
	int cap;
	int size;
};

struct charbuf *charbuf_new();
void charbuf_delete(struct charbuf *b);
char *charbuf_get_str(struct charbuf *b);
int charbuf_add(struct charbuf *b, char c);
int charbuf_is_empty(const struct charbuf *buf);

int read_line(char **lineptr);

#endif /* READ_INCLUDED */
