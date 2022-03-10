CC = gcc
CFLAGS = -Wall -Wextra -g

OBJ = lex.o parse.o exec.o sh.o

default: sh

lex.o: lex.h
exec.o: exec.h
parse.o: lex.h parse.h exec.h
sh.o: lex.h parse.h exec.h

sh: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf sh $(OBJ)

.PHONY: default clean
