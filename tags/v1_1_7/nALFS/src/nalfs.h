/*
 *  nalfs.h - Main header file.
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


#ifndef H_NALFS_
#define H_NALFS_


#include <time.h>
#include <curses.h>

#include "parser.h"

#include "options.h"

#define Fatal_error(a, b...) do { \
	if (opt_run_interactive) { \
		clear(); refresh(); endwin(); \
	} \
	fprintf(stderr, a, ## b); \
	fprintf(stderr, "\nError in %s, line %d. Program version is %s.\n", \
		__FILE__, __LINE__, VERSION); \
	exit(EXIT_FAILURE); \
} while (0)


#ifdef DEBUG_LOGGING
#define Debug_logging(a, b...) Nprint("D_L: " a, ## b)
#else
#define Debug_logging(a, b...)
#endif


int get_key(WINDOW *win);

run_status_e find_element_status(element_s *el);

/* void set_should_run_marks(element_s *el); */

#endif
