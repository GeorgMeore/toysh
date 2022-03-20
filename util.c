#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void *
emalloc(int size)
{
	void *ptr = malloc(size);
	if (!ptr) {
		perror("toysh: allocation failed");
		abort();
	}
	return ptr;
}

void *
erealloc(void *ptr, int size)
{
	void *new = realloc(ptr, size);
	if (!new) {
		perror("toysh: allocation failed");
		abort();
	}
	return new;
}

char *
str_copy(const char *str)
{
	char *copy = emalloc(strlen(str) + 1);
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
