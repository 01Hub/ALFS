/*
 *  new-permissions.c - Handler.
 *
 *  Copyright (C) 2001-2003 by Neven Has <haski@sezampro.yu3
 *
 *  Parts Copyright (C) 2002 by Maik Schreiber <bZ@iq-computing.de>
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


char handler_name[] = "permissions";
char handler_description[] = "Change permissions";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { "base", "mode", NULL };
char *handler_parameters[] = { "option", "name", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int options[1], recursive;
	int status = 0;
	char *base;
	char *mode;
	element_s *p;


	check_options(1, options, "recursive", el);
	recursive = options[0];

	if ((mode = attr_value("mode", el)) == NULL) {
		Nprint_h_err("No permission specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);
	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	for (p = first_param("name", el); p; p = next_param(p)) {
		char *name;
		char *command = NULL;
		char *message = NULL;


		if ((name = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_h_warn("Name empty.");
			continue;
		}

		append_str(&command, "chmod ");

		append_str(&message, "Changing permissions to ");
		append_str(&message, mode);

		if (recursive) {
			append_str(&command, "-R ");
			append_str(&message, " (recursive)");

		}
		append_str(&message, " in ");
		append_str(&message, base);
		append_str(&message, ": ");
		append_str(&message, name);

		append_str(&command, mode);
		append_str(&command, " ");
		append_str(&command, name);

		Nprint_h("%s", message);

		if ((status = execute_command(command))) {
			Nprint_h_err("Changing permissions failed.");
		}

		xfree(name);
		xfree(command);
		xfree(message);

		if (status)
			break;
	}

	xfree(base);
	
	return status;
}
