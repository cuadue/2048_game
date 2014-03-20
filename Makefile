CC=gcc
CFLAGS=-Wall -lcurses

OBJ=2048_game.c

all: 2048

2048: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f 2048

