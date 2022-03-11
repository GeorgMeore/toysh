#ifndef READ_INCLUDED
#define READ_INCLUDED

struct charbuf {
	char *chars;
	int cap;
	int size;
};

void charbuf_init(struct charbuf *buf);
void charbuf_destroy(struct charbuf *buf);
char *charbuf_get_str(struct charbuf *buf);
int charbuf_add(struct charbuf *buf, char c);
int charbuf_is_empty(const struct charbuf *buf);

int read_line(char **lineptr);

#endif /* READ_INCLUDED */
