#ifndef PARSE_INCLUDED
#define PARSE_INCLUDED

#include "lex.h"
#include "sched.h"

struct task *parse(const struct token *head);

#endif /* PARSE_INCLUDED */
