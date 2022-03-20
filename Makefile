CC = gcc
CFLAGS = -ansi -Wall -Wextra -g

SRC = util.c input.c lex.c parse.c sched.c toysh.c
OBJ = $(SRC:.c=.o)

toysh: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRC)
	$(CC) -MM $^ > $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf toysh $(OBJ) deps.mk

.PHONY: clean
