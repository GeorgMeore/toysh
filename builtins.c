#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "builtins.h"
#include "util.h"

#define BUILTIN_DECLARE(NAME) static int NAME(int argc, char **argv)

BUILTIN_DECLARE(cd)
{
	if (argc == 1) {
		return chdir(getenv("HOME"));
	} else {
		return chdir(argv[1]);
	}
}

BUILTIN_DECLARE(bfork)
{
	(void)argc, (void)argv;
	int pid = fork();
	if (pid == -1) {
		perror("error: fork");
		return -1;
	}
	if (pid == 0) {
		return 0;
	}
	printf("%d", pid);
	return 0;
}

struct table_entry {
	char *name;
	builtin fn;
};

static struct table_entry builtin_table[] = {
	{ "cd",   cd },
	{ "fork", bfork }
};

#define TABLESIZE (sizeof(builtin_table) / sizeof(*builtin_table))

builtin
find_builtin(const char *name)
{
	unsigned i;
	for (i = 0; i < TABLESIZE; i++) {
		if (str_equal(builtin_table[i].name, name)) {
			return builtin_table[i].fn;
		}
	}
	return NULL;
}
