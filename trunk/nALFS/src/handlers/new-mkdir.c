/*
 *  new-mkdir.c - Handler.
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


char handler_name[] = "mkdir";
char handler_description[] = "Make directories";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { "base", NULL };
char *handler_parameters[] = { "option", "name", "permissions", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int options[1], parents;
	int status = 0;
	char *base;
	char *perm;
	element_s *p;


	check_options(1, options, "parents", el);
	parents = options[0];

	if ((first_param("name", el)) == NULL) {
		Nprint_h_err("No directories specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	perm = alloc_trimmed_param_value("permissions", el);

	for (p = first_param("name", el); p; p = next_param(p)) {
		char *dir;
		char *command, *message;


		if ((dir = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_h_warn("Directory name empty, ignoring.");
			continue;
		}

		command = xstrdup("mkdir ");
		message = xstrdup("Creating directory in ");
		append_str(&message, base);

		if (parents) {
			append_str(&command, " -p ");
			append_str(&message, " (parents)");
		}

		append_str(&command, dir);

		append_str(&message, ": ");
		append_str(&message, dir);

		if (perm) {
			append_str(&message, " (");
			append_str(&message, perm);
			append_str(&message, ")");
		}
		
		Nprint_h("%s", message);

		if ((status = execute_command("%s", command))) {
			Nprint_h_err("Creating %s failed.", dir);
			xfree(dir);
			xfree(command);
			xfree(message);
			break;
		}

		if (perm) {
			/* Change permissions. */
			if ((status = execute_command("chmod %s %s",
			perm, dir))) {
				Nprint_h_err("Changing permissions failed.");
				xfree(dir);
				xfree(command);
				xfree(message);
				break;
			}
		}

		xfree(dir);
		xfree(command);
		xfree(message);
	}

	xfree(base);
	xfree(perm);
	
	return status;
}
