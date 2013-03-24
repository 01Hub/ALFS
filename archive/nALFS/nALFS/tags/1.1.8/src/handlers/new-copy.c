/*
 *  new-copy.c - Handler.
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


#define El_copy_destination(el) alloc_trimmed_param_value("destination", el)


char handler_name[] = "copy";
char handler_description[] = "Copy";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { "base", NULL };
char *handler_parameters[] = { "option", "source", "destination", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int options[5];
	int status = 0;
	char *common_command;
	char *base;
	char *destination;
	element_s *p;


	if (first_param("source", el) == NULL) {
		Nprint_h_err("No source files specified.");
		return -1;
	}

	if ((destination = El_copy_destination(el)) == NULL) {
		Nprint_h_err("No destination specified.");
		return -1;
	}

	common_command = xstrdup("cp");

	check_options(5, options,
		"archive force no-dereference preserve recursive", el);
	if (options[0]) {
		append_str(&common_command, " -a");
	}
	if (options[1]) {
		append_str(&common_command, " -f");
	}
	if (options[2]) {
		append_str(&common_command, " -d");
	}
	if (options[3]) {
		append_str(&common_command, " -p");
	}
	if (options[4]) {
		append_str(&common_command, " -R");
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(destination);
		xfree(common_command);
		xfree(base);
		return -1;
	}

	for (p = first_param("source", el); p; p = next_param(p)) {
		char *s, *command;


		if ((s = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_h_warn("Source empty.");
			continue;
		}

		command = xstrdup(common_command);
		
		append_str(&command, " ");
		append_str(&command, s);
		append_str(&command, " ");
		append_str(&command, destination);

		Nprint_h("Executing in %s:", base);
		Nprint_h("    %s", command);

		status = execute_command("%s", command);

		xfree(command);
		xfree(s);

		if (status) {
			Nprint_h_err("Copying failed.");
			break;
		}
	}

	xfree(destination);
	xfree(common_command);
	xfree(base);
	
	return status;
}
