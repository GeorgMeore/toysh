#ifndef PARSE_INCLUDED
#define PARSE_INCLUDED

#include "lex.h"
#include "exec.h"

struct task *parse_words(const struct word_item *head);

#endif /* PARSE_INCLUDED */
