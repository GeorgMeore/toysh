#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sched.h"

/* args must be malloc-allocated array of malloc allocated strings
 * ownership of args is transferred here */
struct task *
task_new()
{
	struct task *tsk;
	tsk = malloc(sizeof(*tsk));
	if (!tsk)
		return NULL;
	tsk->argc = 0;
	tsk->args = NULL;
	tsk->bg = 0;
	tsk->next = NULL;
	return tsk;
}

static int
argcount(char **args)
{
	int argc;
	for (argc = 0; args[argc]; argc++)
		{}
	return argc;
}

void
task_init(struct task *tsk, char **args, int bg)
{
	tsk->argc = argcount(args);
	tsk->args = args;
	tsk->bg = bg;
}

static void
task_delete(struct task *tsk)
{
	char **tmp;
	for (tmp = tsk->args; *tmp; tmp++)
		free(*tmp);
	free(tsk->args);
	free(tsk);
}

void
task_list_delete(struct task *head)
{
	struct task *tmp;
	while (head) {
		tmp = head->next;
		task_delete(head);
		head = tmp;
	}
}

void
sched(const struct task *tsk)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		{} /* collect zombies */
	for (; tsk; tsk = tsk->next) {
		if (!strcmp(tsk->args[0], "cd")) {
			if (tsk->argc == 1)
				chdir(getenv("HOME"));
			else
				chdir(tsk->args[1]);
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
				execvp(tsk->args[0], tsk->args);
				perror("error: exec");
				exit(1);
			}
			if (!tsk->bg) {
				int status;
				waitpid(pid, &status, 0);
			}
		}
	}
}
