#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "util.h"
#include "sched.h"

static int
cd(int argc, char **argv)
{
	if (argc == 1)
		return chdir(getenv("HOME"));
	else
		return chdir(argv[1]);
}

static void
exec_fg(int argc, char **argv)
{
	if (str_equal(argv[0], "cd"))
		cd(argc, argv);
	else {
		int pid, status;
		fflush(stderr);
		pid = fork();
		if (pid == -1) {
			perror("error: fork");
			return;
		}
		if (pid == 0) {
			execvp(argv[0], argv);
			perror("error: exec");
			exit(1);
		}
		waitpid(pid, &status, 0);
	}
}

static void
exec_bg(int argc, char **argv)
{
	int pid;
	fflush(stderr);
	pid = fork();
	if (pid == -1) {
		perror("error: fork");
		return;
	}
	if (pid == 0) {
		if (str_equal(argv[0], "cd"))
			exit(cd(argc, argv));
		else {
			execvp(argv[0], argv);
			perror("error: exec");
			exit(1);
		}
	}
}

static int
redirect(char *file, int flags, int which)
{
	int old, new;
	old = dup(which);
	if (file) {
		new = open(file, flags, 0666);
		if (new == -1)
			perror("toysh: open");
		close(which);
		dup2(new, which);
	}
	return old;
}

void
sched(const struct task *tsk)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		{} /* collect zombies */
	while (tsk) {
		int i, oldfd[2];
		for (i = 0; i < 2; i++)
			oldfd[i] = redirect(tsk->rd[i].file, tsk->rd[i].flags, i);
		if (tsk->type == task_fg)
			exec_fg(tsk->argc, tsk->argv);
		else
			exec_bg(tsk->argc, tsk->argv);
		for (i = 0; i < 2; i++)
			dup2(oldfd[i], i);
		tsk = tsk->next;
	}
}
