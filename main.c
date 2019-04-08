#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <ncurses.h>

#include "entry.h"
#include "file_handling.h"
#include "dir_info.h"
#include "input.h"
#include "command_handling.h"

#define ctrl(x)		((x) & 0x1f)
#define KEY_SPACE	' '

/*
TODO:
	- 	Find file in dir with '/', move to index if exists
		Also (later) allow find from command input prompt
	- 	Storing index/position from prev/forw dirs:
	- 	If text printed is longer than columns-1,
		put ... at the end (or columns-4 i guess)
	- 	When ':' is pressed, start accepting input
	-	Add directory with mkdir
	-	Check permissions before chdir or opening file
		https://linux.die.net/man/2/access
		https://linux.die.net/man/2/stat
	-	Scrolling:
		upward scrolling
*/


int main()
{
	int c, x, y;
	WINDOW* win = NULL;
	ENTRY* entry_arr;
	U_DIR* dir_arr;
	int dirs_stored;

	char* cwd;
	char *input;
	int current_index;
	int num_entries;
	int show_dots;

	if (!init_ncurses(win)) {
		endwin();
		fprintf(stderr, "Error: failed to init ncurses window\n");
		exit(EXIT_FAILURE);
	}

	cwd = malloc(sizeof(char) * PATH_MAX);
	cwd = getcwd(NULL,0);
	if (!cwd) {
		endwin();
		perror("getcwd");
		exit(EXIT_FAILURE);
	}
	int run = 1;

	dirs_stored = 0;
	dir_arr = malloc(sizeof(U_DIR) * 32);
	show_dots = 0;
	current_index = 0;
	num_entries = 0;
	getyx(stdscr, y, x);
	get_entries(cwd, &entry_arr, &num_entries, show_dots);
	display_entries(entry_arr, num_entries, current_index,LINES);
	//move(0,0);
	display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
	//mvprintw(LINES-1,0,"%s/%s",cwd, entry_arr[current_index].name);
	//mvprintw(LINES-2,0,"index:%d // total entries:%d(-1) // LINES: %d // ncurses cursor pos: y=%d",
	//	current_index,num_entries,LINES,y);

	//move(y,x);
	refresh(); /* wrefresh(stdscr); */

	while(run)
	{
		flushinp();
		getyx(stdscr, y, x);
		c = getch();
		switch(c) {
		case KEY_DOWN:
			c = 'j';
		case ('j'):
			if (current_index < num_entries -1 ) {
				update_curr_index(DOWN, &current_index, &num_entries);
				display_entries(entry_arr, num_entries, current_index,LINES);
				display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
				refresh();
			}

			break;
		case KEY_UP:
			c = 'k';
		case 'k':
			if (current_index > 0 ) {
				update_curr_index(UP, &current_index, &num_entries);
				display_entries(entry_arr, num_entries, current_index,LINES);
				display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
				refresh();
			}
			break;
		case KEY_LEFT:
			c = 'h';
		case 'h':
			if (strcmp(cwd, "/")) {
				prev_dir(&cwd);
				clear_entries(entry_arr, &num_entries, &current_index);
				get_entries(cwd, &entry_arr, &num_entries, show_dots);
				display_entries(entry_arr, num_entries, current_index,LINES);
				display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
				refresh();
			}
			break;

		case KEY_RIGHT:
			c = 'l';
		case 'l':
			if (entry_arr[current_index].type == 'd') {
				next_dir(&cwd, entry_arr[current_index].name);
				clear_entries(entry_arr, &num_entries, &current_index);
				get_entries(cwd, &entry_arr, &num_entries, show_dots);
			}
			else if (entry_arr[current_index].type != 'd') {
				open_file(cwd, entry_arr[current_index].name);
			}
			else {
				/* damn */
			}
			erase();
			refresh();
			display_entries(entry_arr, num_entries, current_index,LINES);
			display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
			break;

		case ':':
			input = malloc(sizeof(char)*128);
			input = get_input();
			handle_cmd(input,&cwd);
			free(input);
			erase();
			clear_entries(entry_arr, &num_entries, &current_index);
			get_entries(cwd, &entry_arr, &num_entries, show_dots);
			display_entries(entry_arr, num_entries, current_index,LINES);
			display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
			refresh();
			break;

		case 'g':
			if ( (c = getch()) == 'g'){
				current_index = 0;
				erase();
				display_entries(entry_arr, num_entries, current_index,LINES);
				display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
				refresh();
			}
			break;

		case 'G':
			current_index = num_entries -1;
			erase();
			display_entries(entry_arr, num_entries, current_index,LINES);
			display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
			refresh();
			break;

		case 'q':
			run = 0;
			break;
		
		case 'S':
			open_shell(cwd);
			break;

		case ctrl('h'):
			(!show_dots) ?  (show_dots = 1) : (show_dots = 0);
			clear_entries(entry_arr, &num_entries, &current_index);
			get_entries(cwd, &entry_arr, &num_entries, show_dots);
			display_entries(entry_arr, num_entries, current_index,LINES);
			display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
			break;
		case KEY_SPACE:
			if (!entry_arr[current_index].marked)
				mark_file(&entry_arr[current_index]);
			else
				unmark_file(&entry_arr[current_index]);
			if (current_index < num_entries -1) 
				update_curr_index(DOWN, &current_index, &num_entries);
			else 
				erase();
			display_entries(entry_arr, num_entries, current_index,LINES);
			display_file_info(cwd, entry_arr[current_index],current_index, num_entries);
			refresh();
			break;
		default:
			break;
		}
	}

	free(entry_arr);
	move(0,0);
	erase();
	refresh();
	endwin();
	exit(EXIT_SUCCESS);
}