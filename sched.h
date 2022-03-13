#ifndef EXEC_INCLUDED
#define EXEC_INCLUDED

struct task {
	int argc;
	char **args;
	int bg;
	struct task *next;
};

struct task *task_new(void);
void task_init(struct task *tsk, char **args, int bg);
void task_list_delete(struct task *tsk);

void sched(const struct task *tsk);

#endif /* EXEC_INCLUDED */
