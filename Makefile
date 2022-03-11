CC = gcc
CFLAGS = -Wall -Wextra -g

OBJ = input.o lex.o parse.o sched.o sh.o

default: sh

input.o: input.h
lex.o: lex.h
sched.o: sched.h
parse.o: lex.h parse.h sched.h
sh.o: lex.h parse.h sched.h input.h

sh: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf sh $(OBJ)

.PHONY: default clean
