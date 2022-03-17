CC = gcc
CFLAGS = -ansi -Wall -Wextra -g

SRC = util.c input.c lex.c parse.c sched.c sh.c
OBJ = $(SRC:.c=.o)

default: sh

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRC)
	$(CC) -MM $^ > $@

sh: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf sh $(OBJ) deps.mk

.PHONY: default clean
