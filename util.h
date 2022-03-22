#ifndef UTIL_INLUDED
#define UTIL_INLUDED

/* malloc-wrappers which complain on error */
void *emalloc(int size);
void *erealloc(void *ptr, int size);

/* more convenient string manipulation routines */
char *str_copy(const char *str);
int str_equal(const char *s1, const char *s2);

#endif
