#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "builtins.h"
#include "util.h"

static int cd(int argc, char **argv)
{
	if (argc == 1) {
		return chdir(getenv("HOME"));
	} else {
		return chdir(argv[1]);
	}
}

static int bfork()
{
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

static int bexit(int argc, char **argv)
{
	if (argc == 1) {
		exit(0);
	} else {
		exit(atoi(argv[1]));
	}
}

struct table_entry {
	char *name;
	builtin fn;
};

static struct table_entry builtin_table[] = {
	{ "cd",   cd    },
	{ "fork", bfork },
	{ "exit", bexit },
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
