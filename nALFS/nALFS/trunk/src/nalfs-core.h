/*
 *  nalfs-core.h - Main header file.
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


#ifndef H_NALFS_CORE_
#define H_NALFS_CORE_


#include <time.h>
#include <curses.h>

#include "parser.h"

#ifdef DEBUG_LOGGING
#define Debug_logging(a, b...) Nprint("D_L: " a, ## b)
#else
#define Debug_logging(a, b...)
#endif


extern int get_key(WINDOW *win);

extern run_status_e get_element_status(element_s *el);

/* void set_should_run_marks(element_s *el); */

#endif /* H_NALFS_CORE_ */
