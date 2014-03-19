CC=gcc
CFLAGS=-Wall -lcurses

OBJ=2048_game.c

all:
	$(CC) $(OBJ) $(CFLAGS) -o 2048

clean:
	rm -f 2048

