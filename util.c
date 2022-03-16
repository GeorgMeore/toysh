#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void *
emalloc(int size)
{
	void *ptr;
	ptr = malloc(size);
	if (!ptr)
		perror("sh: malloc");
	return ptr;
}

void *
erealloc(void *ptr, int size)
{
	void *new;
	new = realloc(ptr, size);
	if (!new)
		perror("sh: realloc");
	return new;
}

char *
str_copy(const char *str)
{
	char *copy;
	copy = emalloc(strlen(str) + 1);
	if (!copy)
		return NULL;
	strcpy(copy, str);
	return copy;
}

int
str_equal(const char *s1, const char *s2)
{
	for (; *s1 && (*s1 == *s2); s1++, s2++)
		{}
	return *s1 == *s2;
}
