/*
 *  move.c - Handler.
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
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME move
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#define El_move_source(el) alloc_trimmed_param_value("source", el)
#define El_move_destination(el) alloc_trimmed_param_value("destination", el)


#if HANDLER_SYNTAX_2_0

static const char *move_parameters_ver2[] =
{ "options", "base", "source", "destination", NULL };

static int move_main_ver2(element_s *el)
{
	int status = 0;
	int force = option_exists("force", el);
	char *tok;
	char *base;
	char *source;
	char *destination;


	if ((source = El_move_source(el))== NULL) {
		Nprint_h_err("No source files specified.");
		return -1;
	}
	
	if ((destination = El_move_destination(el))== NULL) {
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
		Nprint_h("Moving from %s to %s%s: %s",
			base, destination, force ? " (force)" : "", tok);

		if (force) {
			status = execute_command("mv -f %s %s", tok, destination);
		} else {
			status = execute_command("mv %s %s", tok, destination);
		}

		if (status) {
			Nprint_h_err("Moving failed.");
			break;
		}
	}

	xfree(base);
	xfree(source);
	xfree(destination);
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *move_parameters_ver3[] =
{ "option", "source", "destination", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", NULL };

static int move_main_ver3(element_s *el)
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

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "move",
		.description = "Move files",
		.syntax_version = "2.0",
		.parameters = move_parameters_ver2,
		.main = move_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "move",
		.description = "Move files",
		.syntax_version = "3.0",
		.parameters = move_parameters_ver3,
		.main = move_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "move",
		.description = "Move files",
		.syntax_version = "3.1",
		.parameters = move_parameters_ver3,
		.main = move_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
