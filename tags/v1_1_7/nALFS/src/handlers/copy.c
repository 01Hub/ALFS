/*
 *  copy.c - Handler.
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
#include <unistd.h>
#include <errno.h>

#include "utility.h"
#include "win.h"
#include "config.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"


#define El_copy_source(el) alloc_trimmed_param_value("source", el)
#define El_copy_destination(el) alloc_trimmed_param_value("destination", el)


char handler_name[] = "copy";
char handler_description[] = "Copy";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] =
{ "options", "base", "source", "destination", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status = 0;
	int archive = option_exists("archive",el);
	int force = option_exists("force", el);
	int no_dereference = option_exists("no-dereference", el);
	int preserve = option_exists("preserve", el);
	int recursive = option_exists("recursive", el);
	char *tok;
	char *base = NULL;
	char *source = NULL;
	char *destination = NULL;


	if ((source = El_copy_source(el)) == NULL) {
		Nprint_h_err("No source files specified.");
		return -1;
	}

	if ((destination = El_copy_destination(el)) == NULL) {
		Nprint_h_err("No destination specified.");
		xfree(source);
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(source);
		xfree(destination);
		return -1;
	}

	for (tok = strtok(source, WHITE_SPACE); tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		char *command = xstrdup("cp");

		/* Options. */
		if (archive)
			append_str(&command, " -a");
		if (force)
			append_str(&command, " -f");
		if (no_dereference)
			append_str(&command, " -d");
		if (preserve)
			append_str(&command, " -p");
		if (recursive)
			append_str(&command, " -R");

		append_str(&command, " ");
		append_str(&command, tok);
		append_str(&command, " ");
		append_str(&command, destination);

		Nprint_h("Executing in %s:", base);
		Nprint_h("    %s", command);

		status = execute_command("%s", command);

		xfree(command);

		if (status) {
			Nprint_h_err("Copying failed.");
			break;
		}
	}

	xfree(base);
	xfree(source);
	xfree(destination);
	
	return status;
}
