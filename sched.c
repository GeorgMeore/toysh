#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sched.h"

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
			if (tsk->type == task_fg) {
				int status;
				waitpid(pid, &status, 0);
			}
		}
	}
}
