#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "exec.h"

/* args must be malloc-allocated array of malloc allocated strings
 * ownership of args is transferred here */
struct task *
task_new(int argc, char **args)
{
	struct task *t;
	t = malloc(sizeof(*t));
	if (!t) {
		fputs("error: failed to allocate memory\n", stderr);
		return NULL;
	}
	t->argc = argc;
	t->args = args;
	return t;
}

void
task_delete(struct task *t)
{
	char **tmp;
	for (tmp = t->args; *tmp; tmp++)
		free(*tmp);
	free(t->args);
	free(t);
}

void
task_exec(const struct task *t)
{
	if (!strcmp(t->args[0], "cd")) {
		if (t->argc == 1)
			chdir(getenv("HOME"));
		else
			chdir(t->args[1]);
	}
	else {
		int pid;
		fflush(stderr);
		pid = fork();
		if (pid == -1) {
			perror("error: fork");
			return;
		}
		if (pid == 0) {
			execvp(t->args[0], t->args);
			perror("error: exec");
			exit(1);
		}
		wait(NULL);
	}
}
