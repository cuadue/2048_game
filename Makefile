CFLAGS=-Wall -Wextra -Werror
LDFLAGS=-lcurses
OBJ=2048_game.o
APP=2048

all: $(APP) test

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(APP): $(OBJ)
	$(CC) -o $@ $< $(LDFLAGS)

test: run-tests $(APP)
	bash -x ./$<

demo: $(APP)
	./$< -p "test-data/1/in"

clean:
	rm -f $(APP) $(OBJ)

