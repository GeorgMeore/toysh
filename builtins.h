#ifndef BUILTINS_INCLUDED
#define BUILTINS_INCLUDED

typedef int (*builtin)(int, char **);

builtin find_builtin(const char *name);

#endif
