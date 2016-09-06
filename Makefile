CC=gcc
CFLAGS=-Wall -Wextra -Werror
LIBS=-lcurses

OBJ=2048_game.c

all: 2048 test

2048: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

test: run-tests 2048
	bash -x ./$<

demo: 2048
	./$< -p "test-data/1/in"

clean:
	rm -f 2048

