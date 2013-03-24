/*
 *  win.c - Windows handling functions.
 *
 *  Copyright (C) 2001-2003
 *
 *  Neven Has <haski@sezampro.yu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <curses.h>

#include "utility.h"
#include "nalfs.h"
#include "options.h"
#include "config.h"

#include "win.h"


windows_s windows = {
	.main 			= NULL,
	.status 		= NULL,
	.active 		= MAIN_WINDOW,
	.max_lines 		= 0,
	.max_cols 		= 0,
	.middle_line_offset 	= 0
};


static const msg_type_t message_types[] = {
	{ T_RAW, ' ', COL_WHITE },
	{ T_INF, 'I', COL_CYAN },
	{ T_HLP, 'H', COL_CYAN },
	{ T_SYS, '-', COL_CYAN },
	{ T_WAR, 'W', COL_CYAN_BOLD },
	{ T_ERR, 'E', COL_RED_BOLD }
};



char msg_character(msg_id_e mid)
{
	ASSERT(mid <= T_ERR);

	if (mid <= T_ERR) {
		return message_types[mid].character;
	}

	return 'x';
}

unsigned long msg_attrs(msg_id_e mid)
{
	ASSERT(mid <= T_ERR);

	if (mid <= T_ERR) {
		return message_types[mid].attrs;
	}

	return COL_WHITE;
}

static INLINE void init_colors(void)
{
	if (has_colors()) {
		start_color();

		init_pair(COLP_CYAN, COLOR_CYAN, COLOR_BLACK);
		init_pair(COLP_RED, COLOR_RED, COLOR_BLACK);
		init_pair(COLP_GREEN, COLOR_GREEN, COLOR_BLACK);
		init_pair(COLP_WHITE, COLOR_WHITE, COLOR_BLACK);
	}
}

static void set_default_window_options(window_s *w)
{
	leaveok(w->name, TRUE);
	scrollok(w->name, TRUE);
	idlok(w->name, TRUE);
	keypad(w->name, TRUE);
	nodelay(w->name, TRUE);
}



static int main_window_lines(void)
{
	int lines;
	int lines_with_offset;


	lines = (opt_windows_relation * (windows.max_lines - 3)) / 100;

	lines_with_offset = lines + windows.middle_line_offset;

	if (lines_with_offset < 2) {
		lines_with_offset = 2;

	} else if (lines_with_offset > windows.max_lines - 3 - 2) {
		lines_with_offset = windows.max_lines - 3 - 2;
	}

	windows.middle_line_offset = lines_with_offset - lines;

	return lines_with_offset;
}



static int refresh_main(int top_line)
{
	Xprefresh(windows.main->name, top_line, 0,
		1, 1, windows.main->lines, windows.max_cols - 2);

	return 0;
}

static window_s *create_main_window(int num_of_lines)
{
	window_s *w = xmalloc(sizeof *w);


	w->ref = refresh_main;
	w->lines = main_window_lines();
	/* We're adding 1 just in case. */
	w->name = newpad(num_of_lines + 1, windows.max_cols - 2);

	if (w->name == NULL) {
		Fatal_error("newpad() failed");
	}

	set_default_window_options(w);

	return w;
}



static int refresh_status(int offset)
{
	int x, starting_top;
	int top;


	/* Get starting top line number. */
	getmaxyx(windows.status->name, starting_top, x);
	starting_top -= windows.status->lines;

	top = starting_top - offset;

	if (top < 0) {
		top = 0;

	} else if (top > starting_top) {
		top = starting_top;
	}

	Xprefresh(windows.status->name, top, 0,
		windows.max_lines - 1 - windows.status->lines, 1,
		windows.max_lines - 2, windows.max_cols - 2);

	return (starting_top - top); /* offset */
}

static INLINE window_s *create_status_window(void)
{
	int main_lines = main_window_lines();
	window_s *w = xmalloc(sizeof *w);


	w->ref = refresh_status;
	w->lines = windows.max_lines - 3 - main_lines;
	w->name = newpad(opt_status_history + w->lines, windows.max_cols - 2);

	if (w->name == NULL) {
		Fatal_error("newpad() failed");
	}

	set_default_window_options(w);

	Xwmove(w->name, opt_status_history - 1, 0);

	return w;
}



static INLINE void create_windows(int num_of_lines)
{
	getmaxyx(stdscr, windows.max_lines, windows.max_cols);

	/* windows.active is defined in browse() */

	windows.main = create_main_window(num_of_lines);
	windows.status = create_status_window();
}

void start_display(void)
{
	initscr();
	cbreak();
	noecho();

	scrollok(stdscr, FALSE);

	init_colors();

	create_windows(0);
}

static INLINE void delete_all_windows(void)
{
	delwin(windows.main->name);
	xfree(windows.main);

	delwin(windows.status->name);
	xfree(windows.status);

	clear();
	refresh();
}

void end_display(void)
{
	delete_all_windows();

	endwin();
}

void recreate_main_window(int num_of_lines)
{
	/* Clear the window in case the new one is smaller. */
	werase(windows.main->name);
	refresh_main(0);

	delwin(windows.main->name);
	xfree(windows.main);

	windows.main = create_main_window(num_of_lines);
}

void resize_all_windows(void)
{
	int main_lines = main_window_lines();
	int status_lines = windows.max_lines - 3 - main_lines;


	windows.main->lines = main_lines;
	windows.status->lines = status_lines;
}
