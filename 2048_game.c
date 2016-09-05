/* Clone of the 2048 sliding tile puzzle game. (C) Wes Waugh 2014
 *
 * This program only works on Unix-like systems with curses. Link against
 * the curses library. You can pass the -lcurses flag to gcc at compile
 * time.
 *
 * This program is free software, licensed under the GPLv3. Check the
 * LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

#define NROWS 4
#define NCOLS NROWS

typedef int tile;

struct game {
	int turns, score;
	tile board[NROWS][NCOLS];
};

// place_tile() returns 0 if it did place a tile and -1 if there is no open
// space.
int place_tile(struct game *game)
{
	// lboard is the "linear board" -- no need to distinguish rows/cols
	tile *lboard = (tile *)game->board;
	int i, num_zeros = 0;

	// Walk the board and count the number of empty tiles
	for (i = 0; i < NROWS * NCOLS; i++) {
		num_zeros += lboard[i] ? 0 : 1;
	}

	if (!num_zeros) {
		return -1;
	}

	// Choose the insertion point
	int loc = random() % num_zeros;

	// Find the insertion point and place the new tile
	for (i = 0; i < NROWS * NCOLS; i++) {
		if (!lboard[i] && !(loc--)) {
			lboard[i] = random() % 10 ? 1 : 2;
			return 0;
		}
	}
	assert(0);
}

void print_tile(int tile)
{
	if (tile) {
		if (tile < 6)
			attron(A_BOLD);
		int pair = COLOR_PAIR(1 + (tile % 6));
		attron(pair);
		printw("%4d", 1 << tile);
		attroff(pair);
		attroff(A_BOLD);
	}
	else {
		printw("   .");
	}
}

void print_game(const struct game *game)
{
	int r, c;
	move(0, 0);
	printw("Score: %6d  Turns: %4d", game->score, game->turns);
	for (r = 0; r < NROWS; r++) {
		for (c = 0; c < NCOLS; c++) {
			move(r + 2, 5 * c);
			print_tile(game->board[r][c]);
		}
	}

	refresh();
}

int combine_left(struct game *game, tile row[NCOLS])
{
	int c, did_combine = 0;
	for (c = 1; c < NCOLS; c++) {
		if (row[c] && row[c-1] == row[c]) {
			row[c-1]++;
			row[c] = 0;
			game->score += 1 << (row[c-1] - 1);
			did_combine = 1;
		}
	}
	return did_combine;
}

// deflate_left() returns nonzero if it did deflate, and 0 otherwise
int deflate_left(tile row[NCOLS])
{
	tile buf[NCOLS] = {0};
	tile *out = buf;
	int did_deflate = 0;
	int in;

	for (in = 0; in < NCOLS; in++) {
		if (row[in] != 0) {
			*out++ = row[in];
			did_deflate |= buf[in] != row[in];
		}
	}

	memcpy(row, buf, sizeof(buf));
	return did_deflate;
}

void rotate_clockwise(struct game *game)
{
	tile buf[NROWS][NCOLS];
	memcpy(buf, game->board, sizeof(game->board));

	int r, c;
	for (r = 0; r < NROWS; r++) {
		for (c = 0; c < NCOLS; c++) {
			game->board[r][c] = buf[NCOLS - c - 1][r];
		}
	}
}

void move_left(struct game *game)
{
	int r, ret = 0;
	for (r = 0; r < NROWS; r++) {
		tile *row = &game->board[r][0];
		ret |= deflate_left(row);
		ret |= combine_left(game, row);
		ret |= deflate_left(row);
	}

	game->turns += ret;
}

void move_right(struct game *game)
{
	rotate_clockwise(game);
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
}

void move_up(struct game *game)
{
	rotate_clockwise(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
}

void move_down(struct game *game)
{
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
}

// Pass by value because this function mutates the game
int lose_game(struct game test_game)
{
	int start_turns = test_game.turns;
	move_left(&test_game);
	move_up(&test_game);
	move_down(&test_game);
	move_right(&test_game);
	return test_game.turns == start_turns;
}

void init_curses()
{
	int bg = 0;
	initscr(); // curses init
	start_color();
	cbreak(); // curses don't wait for enter key
	noecho(); // curses don't echo the pressed key
	keypad(stdscr,TRUE);
	clear(); // curses clear screen and send cursor to (0,0)
	refresh();
	curs_set(0);

	bg = use_default_colors() == OK ? -1 : 0;
	init_pair(1, COLOR_RED, bg);
	init_pair(2, COLOR_GREEN, bg);
	init_pair(3, COLOR_YELLOW, bg);
	init_pair(4, COLOR_BLUE, bg);
	init_pair(5, COLOR_MAGENTA, bg);
	init_pair(6, COLOR_CYAN, bg);
}

int max_tile(const tile *lboard)
{
	int i, ret = 0;
	for (i = 0; i < NROWS * NCOLS; i++) {
		ret = max(ret, lboard[i]);
	}
	return ret;
}

int main()
{
	init_curses();

	const char *exit_msg = "";
	srandom(time(NULL));

	struct game game = {0};
	int last_turn = game.turns;

	place_tile(&game);
	place_tile(&game);

	while (1) {
		print_game(&game);

		if (lose_game(game)) {
			exit_msg = "lost";
			goto lose;
		}

		last_turn = game.turns;

		switch (getch()) {
		case 'h': case KEY_LEFT: move_left(&game); break;
		case 'j': case KEY_DOWN: move_down(&game); break;
		case 'k': case KEY_UP: move_up(&game); break;
		case 'l': case KEY_RIGHT: move_right(&game); break;
		case 'q':
			exit_msg = "quit";
			goto end;
		}

		if (last_turn != game.turns)
			place_tile(&game);
	}

lose:
	move(7, 0);
	printw("You lose! Press q to quit.");
	while (getch() != 'q');
end:
	endwin();
	printf("You %s after scoring %d points in %d turns, "
		"with largest tile %d\n",
		exit_msg, game.score, game.turns,
		1 << max_tile((tile *)game.board));
	return 0;
}

