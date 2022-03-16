#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "util.h"
#include "sched.h"

void
sched(const struct task *tsk)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		{} /* collect zombies */
	for (; tsk; tsk = tsk->next) {
		if (str_equal(tsk->argv[0], "cd")) {
			if (tsk->argc == 1)
				chdir(getenv("HOME"));
			else
				chdir(tsk->argv[1]);
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
				execvp(tsk->argv[0], tsk->argv);
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
