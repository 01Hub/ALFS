/*
 *  win.h - Windows handling functions.
 *
 *  Copyright (C) 2001, 2002
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


#ifndef H_WIN_
#define H_WIN_


#include <curses.h>

#include "logging.h"
#include "config.h"


#define MOD_CTRL(x) ((x) & 31)

/*
 * Color.
 */

/* Color pairs. */
enum cpair {
	COLP_CYAN = 1,
	COLP_RED,
	COLP_GREEN,
	COLP_WHITE
};

/* Color pairs for run-status marks. */
#define COLP_STATUS_NONE	COLP_WHITE
#define COLP_STATUS_FAILED	COLP_RED
#define COLP_STATUS_RUNNING	COLP_CYAN
#define COLP_STATUS_SOME_DONE	COLP_GREEN
#define COLP_STATUS_DONE	COLP_GREEN

/* Colors. */
#define COL_CYAN	(COLOR_PAIR(COLP_CYAN))
#define COL_CYAN_BOLD	(COLOR_PAIR(COLP_CYAN) | A_BOLD)
#define COL_RED		(COLOR_PAIR(COLP_RED))
#define COL_RED_BOLD	(COLOR_PAIR(COLP_RED) | A_BOLD)
#define COL_GREEN	(COLOR_PAIR(COLP_GREEN))
#define COL_WHITE	(COLOR_PAIR(COLP_WHITE))


/*
 * Types of messages.  They are also used as message_types[] indexes.
 * First should be 0, last T_ERR, order matters.
 */
typedef enum msg_id_e {
	T_RAW = 0,
	T_INF,
	T_HLP,
	T_SYS,
	T_WAR,
	T_ERR
} msg_id_e;


typedef struct msg_type_t {
	msg_id_e id;		// Unique ID of the message type.
	char character;		// Character to print before the message.
	unsigned long attrs;	// Attributes for printing the above character.
} msg_type_t;

char msg_character(msg_id_e mid);
unsigned long msg_attrs(msg_id_e mid);


/*
 * Windows.
 */

typedef enum active_window_e {
	MAIN_WINDOW,
	STATUS_WINDOW,
	TMP_WINDOW,
	BOTTOM_WINDOW
} active_window_e;

typedef struct window_s {
	WINDOW *name;		/* Actual ncurses window. */
	int lines;		/* Number of lines. */
	int (*ref)(int);	/* Function for refreshing the window. */
} window_s;

#define Can_do_rewrite ( \
	windows.active == MAIN_WINDOW || \
	windows.active == STATUS_WINDOW || \
	windows.active == BOTTOM_WINDOW)

typedef struct windows_s {
	window_s *main;
	window_s *status;

	active_window_e active;

	int max_lines;
	int max_cols;

	int middle_line_offset;
} windows_s;

extern windows_s windows;


/*
 * Printing macros.
 */

void (*nprint)(msg_id_e mid, const char *format,...);

/* Print macros. */
#define Nprint(a, b...)		nprint(T_INF, a, ## b)
#define Nprint_warn(a, b...)	nprint(T_WAR, a, ## b)
#define Nprint_err(a, b...)	nprint(T_ERR, a, ## b)
#define Nprint_sys(a, b...)	nprint(T_SYS, a, ## b)
#define Nprint_raw(a, b...)	nprint(T_RAW, a, ## b)
#define Nprint_help(a, b...)	nprint(T_HLP, a, ## b)

/*
 * Print macros used in handlers.  TODO: Remove them and use the same
 * macros as above, just define them differently when used in handlers.
 */
#define Nprint_h(a, b...)	do { log_handler_action(a, ## b); \
					Nprint(a, ## b); } while (0)
#define Nprint_h_warn(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_warn(a, ## b); } while (0)
#define Nprint_h_err(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_err(a, ## b); } while (0)
#define Nprint_h_sys(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_sys(a, ## b); } while (0)
#define Nprint_h_raw(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_raw(a, ## b); } while (0)
#define Nprint_h_help(a, b...)	do{ log_handler_action(a, ## b); \
					Nprint_help(a, ## b); } while (0)



void start_display(void);
void end_display(void);
void recreate_main_window(int num_of_lines);
void resize_all_windows(void);


/*
 * Wrappers for some ncurses functions.
 * TODO: Use functions, this is terrible.
 */

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xwaddch(win, ch) \
do { \
	if (waddch(win, ch) == ERR) { \
		Fatal_error("waddch() failed"); \
	} \
} while (0)
#else
#define Xwaddch(win, ch) waddch(win, ch)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xwprintw(win, a, b...) \
do { \
	if (wprintw(win, a, ## b) == ERR) { \
		Fatal_error("wprintw() failed"); \
	} \
} while (0)
#else
#define Xwprintw(win, a, b...) wprintw(win, a, ## b)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xwaddstr(win, str) \
do { \
	if (waddstr(win, str) == ERR) { \
		Fatal_error("waddstr() failed"); \
	} \
} while (0)
#else
#define Xwaddstr(win, str) waddstr(win, str)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xmvwaddch(win, y, x, ch) \
do { \
	if (mvwaddch(win, (int)(y), (int)(x), (chtype)(ch)) == ERR) { \
		Fatal_error("mvwaddch() failed"); \
	} \
} while (0)
#else
#define Xmvwaddch(win, y, x, ch) mvwaddch(win, y, x, ch)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xwmove(win, y, x) \
do { \
	if (wmove(win, (int)(y), (int)(x)) == ERR) { \
		Fatal_error("wmove() failed"); \
	} \
} while (0)
#else
#define Xwmove(win, y, x) wmove(win, y, x)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xwerase(win) \
do { \
	if (werase(win) == ERR) { \
		Fatal_error("werase() failed"); \
	} \
} while (0)
#else
#define Xwerase(win) werase(win)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xmvwaddstr(win, y, x, str) \
do { \
	if (mvwaddstr(win, y, x, str) == ERR) { \
		Fatal_error("mvwaddstr() failed"); \
	} \
} while (0)
#else
#define Xmvwaddstr(win, y, x, str) mvwaddstr(win, y, x, str)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xprefresh(win, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol) \
do { \
	if (prefresh(win, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)\
	== ERR) { \
		Fatal_error("prefresh() failed"); \
	} \
} while (0)
#else
#define Xprefresh(win, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol) \
	prefresh(win, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xmvaddch(y, x, ch) \
do { \
	if (mvaddch(y, x, ch) == ERR) { \
		Fatal_error("mvaddch() failed"); \
	} \
} while (0)
#else
#define Xmvaddch(y, x, ch) mvaddch(y, x, ch)
#endif

#ifdef DEBUG_NCURSES_FUNCTIONS
#define Xmvaddstr(y, x, str) \
do { \
	if (mvaddstr(y, x, str) == ERR) { \
		Fatal_error("mvaddstr() failed"); \
	} \
} while (0)
#else
#define Xmvaddstr(y, x, str) mvaddstr(y, x, str)
#endif


#endif
