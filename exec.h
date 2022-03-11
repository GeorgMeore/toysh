#ifndef EXEC_INCLUDED
#define EXEC_INCLUDED

struct task {
	int argc;
	char **args;
};

struct task * task_new(int argc, char **args);
void task_delete(struct task *t);
void task_exec(const struct task *t);

#endif /* EXEC_INCLUDED */
