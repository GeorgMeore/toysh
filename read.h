#ifndef READ_INCLUDED
#define READ_INCLUDED

struct buffer {
	char *buf;
	int bufcap;
	int bufsz;
};

struct buffer *buffer_new();
void buffer_delete(struct buffer *b);
char *buffer_get_str(struct buffer *b);
int buffer_add(struct buffer *b, char c);

char *read_line();

#endif /* READ_INCLUDED */
