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
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

#define NROWS 4
#define NCOLS NROWS

typedef unsigned int tile_t;

struct game_t {
	int turns, score, highscore;
  char *highscorefile;
	tile_t board[NROWS][NCOLS];
};

const char HIGHSCORE_FILE[] = ".2048score";

int place_tile(struct game_t *game)
{
	tile_t *lboard = (tile_t *)game->board;
	int i, num_zeros = 0;
	for (i = 0; i < NROWS * NCOLS; i++) {
		num_zeros += lboard[i] ? 0 : 1;
	}

	if (!num_zeros) {
		return -1;
	}

	int loc = random() % num_zeros;

	for (i = 0; i < NROWS * NCOLS; i++) {
		if (!lboard[i] && !(loc--)) {
			lboard[i] = random() % 10 ? 1 : 2;
			return 0;
		}
	}
	return -1;
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

void print_game(struct game_t *game)
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

int combine_left(struct game_t *game, tile_t row[NCOLS])
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

int deflate_left(tile_t row[NCOLS])
{
	tile_t buf[NCOLS] = {0};
	tile_t *out = buf;
	int did_compress = 0;

	int in;
	for (in = 0; in < NCOLS; in++) {
		if (row[in] != 0) {
			*(out++) = row[in];
			did_compress |= buf[in] != row[in];
		}
	}

	memcpy(row, buf, sizeof(buf));
	return did_compress;
}

void rotate_clockwise(struct game_t *game)
{
	tile_t buf[NROWS][NCOLS];
	memcpy(buf, game->board, sizeof(game->board));

	int r, c;
	for (r = 0; r < NROWS; r++) {
		for (c = 0; c < NCOLS; c++) {
			game->board[r][c] = buf[NCOLS - c - 1][r];
		}
	}
}

void move_left(struct game_t *game)
{
	int r, ret = 0;
	for (r = 0; r < NROWS; r++) {
		tile_t *row = &game->board[r][0];
		ret |= deflate_left(row);
		ret |= combine_left(game, row);
		ret |= deflate_left(row);
	}

	game->turns += ret;
}

void move_right(struct game_t *game)
{
	rotate_clockwise(game);
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
}

void move_up(struct game_t *game)
{
	rotate_clockwise(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
}

void move_down(struct game_t *game)
{
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
}

int lose_game(struct game_t test_game)
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

int max_tile(tile_t *lboard)
{
	int i, ret = 0;
	for (i = 0; i < NROWS * NCOLS; i++) {
		ret = max(ret, lboard[i]);
	}
	return ret;
}

void get_highscore_filepath(struct game_t *g)
{
  uid_t uid;
  struct passwd *pw;

  uid = getuid();
  if((pw = getpwuid(uid)) == NULL)
  {
    g->highscorefile = malloc(sizeof(HIGHSCORE_FILE));
    memcpy(g->highscorefile, HIGHSCORE_FILE, sizeof(HIGHSCORE_FILE));
    return;
  }

  g->highscorefile = calloc(sizeof(HIGHSCORE_FILE)+strlen(pw->pw_dir)+2, 1);
  sprintf(g->highscorefile, "%s/%s", pw->pw_dir, HIGHSCORE_FILE);
}

void load_highscore(struct game_t *g)
{
  FILE *f = NULL;
  int hs = 0;
  if(access(g->highscorefile, F_OK) != 0)
  {
    if((f = fopen(g->highscorefile, "w")) == NULL)
    {
      printf("Failed to create the highscore file %s.\n", g->highscorefile);
      return;
    }
    fprintf(f, "%d", 0);
    fclose(f);
    g->highscore = 0;
    return;
  }
  if(access(g->highscorefile, F_OK | R_OK) != 0 ||
      (f = fopen(g->highscorefile, "r")) == NULL)
  {
    printf("Failed to read the highscore file %s.\n", g->highscorefile);
    return;
  }
  if(fscanf(f, "%d", &hs) != 1 || hs<0)
  {
    g->highscore = 0;
    printf("The contents of the highscore file %s are corrupted. "
        "Please remove the file and run the game again.\n", g->highscorefile);
    fclose(f);
    return;
  }
  g->highscore = hs;
  fclose(f);
  return;
}

int main()
{
	init_curses();

	const char *exit_msg = "";
	srandom(time(NULL));

	struct game_t game = {0};
	int last_turn = game.turns;

  get_highscore_filepath(&game);
  load_highscore(&game);

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
		1 << max_tile((tile_t *)game.board));
	return 0;
}

