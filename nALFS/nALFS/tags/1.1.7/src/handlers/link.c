/*
 *  link.c - Handler.
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


#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"
#include "config.h"


#define El_link_target(el) alloc_trimmed_param_value("target", el)
#define El_link_name(el) alloc_trimmed_param_value("name", el)


char handler_name[] = "link";
char handler_description[] = "Link";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { "type", NULL };
char *handler_parameters[] = { "options", "base", "target", "name", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status;
	int force = option_exists("force", el);
	char *type = attr_value("type", el);
	char *base;
	char *target;
	char *link_name;
	char *command = NULL;
	char *message = NULL;


	if ((target = El_link_target(el)) == NULL) {
		Nprint_h_err("No source files specified.");
		return -1;
	}

	link_name = El_link_name(el);

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(target);
		xfree(link_name);
		return -1;
	}

	if (type == NULL || strcmp(type, "symbolic") == 0) {
		append_str(&command, "ln -s");
		append_str(&message, "Creating symbolic link in ");

	} else if (strcmp(type, "hard") == 0) {
		append_str(&command, "ln");
		append_str(&message, "Creating hard link in ");

	} else {
		Nprint_h_warn("Unknown link type (%s), using symbolic.", type);
		append_str(&command, "ln -s");
		append_str(&message, "Creating symbolic link in ");
	}

	append_str(&message, base);

	if (force) {
		append_str(&command, " -f");
		append_str(&message, " (force):");

	} else {
		append_str(&message, ":");
	}

	Nprint_h("%s", message);

	if (link_name) {
		Nprint_h("    %s -> %s", link_name, target);
	} else {
		Nprint_h("    %s", target);
	}

	append_str(&command, " ");
	append_str(&command, target);

	if (link_name) {
		append_str(&command, " ");
		append_str(&command, link_name);
	}

	if ((status = execute_command("%s", command))) {
		Nprint_h_err("Executing \"%s\" in \"%s\" failed.",
			command, base);
	}

	xfree(base);
	xfree(target);
	xfree(link_name);
	xfree(command);
	xfree(message);

	return status;
}
