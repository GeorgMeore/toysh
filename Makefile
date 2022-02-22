CC = gcc
CFLAGS = -Wall -Wextra -g

OBJ = lex.o sh.o

default: sh

lex.o: lex.c lex.h
sh.o: sh.c lex.h

sh: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf sh $(OBJ)

.PHONY: default clean
