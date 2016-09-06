CC=gcc
CFLAGS=-Wall -Wextra -Werror
LIBS=-lcurses

OBJ=2048_game.c

all: 2048 test

2048: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

test: run-tests
	bash -x ./$<

clean:
	rm -f 2048

