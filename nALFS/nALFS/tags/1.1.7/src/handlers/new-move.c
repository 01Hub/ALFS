/*
 *  new-move.c - Handler.
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


#include <stdlib.h>

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"
#include "config.h"


char handler_name[] = "move";
char handler_description[] = "Move files";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { "base", NULL };
char *handler_parameters[] = { "option", "source", "destination", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int options[1], force;
	int status = 0;
	char *base;
	char *destination;
	element_s *p;


	check_options(1, options, "force", el);
	force = options[0];

	if (first_param("source", el) == NULL) {
		Nprint_h_err("No source files specified.");
		return -1;
	}

	destination = alloc_trimmed_param_value("destination", el);
	if (destination == NULL) {
		Nprint_h_err("No destination specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(destination);
		return -1;
	}

	for (p = first_param("source", el); p; p = next_param(p)) {
		char *s;

		if ((s = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_h_warn("Source empty.");
			continue;
		}

		Nprint_h("Moving from %s to %s%s: %s",
			base, destination, force ? " (force)" : "", s);

		if (force) {
			status = execute_command("mv -f %s %s", s, destination);
		} else {
			status = execute_command("mv %s %s", s, destination);
		}

		xfree(s);

		if (status) {
			Nprint_h_err("Moving failed.");
			break;
		}
	}

	xfree(base);
	xfree(destination);
	
	return status;
}
