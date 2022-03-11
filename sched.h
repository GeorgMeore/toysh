#ifndef EXEC_INCLUDED
#define EXEC_INCLUDED

struct task {
	int argc;
	char **args;
	int bg;
};

struct task *task_new(int argc, char **args, int bg);
void task_delete(struct task *t);

void sched(const struct task *t);

#endif /* EXEC_INCLUDED */
