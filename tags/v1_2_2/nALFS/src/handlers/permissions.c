/*
 *  permissions.c - Handler.
 *
 *  Copyright (C) 2001-2003 by Neven Has <haski@sezampro.yu>
 *
 *  Parts Copyright (C) 2002-2003 by Maik Schreiber <bZ@iq-computing.de>
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

#define MODULE_NAME permissions
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#define El_permissions_mode(el) alloc_trimmed_param_value("mode", el)
#define El_permissions_targets(el) alloc_trimmed_param_value("name", el)


#if HANDLER_SYNTAX_2_0

static const char *permissions_parameters_ver2[] =
{ "base", "options", "mode", "name", NULL };

static int permissions_main_ver2(element_s *el)
{
	int status = 0;
	int recursive = option_exists("recursive", el);
	char *tok;
	char *base;
	char *mode;
	char *targets;
	char *command = NULL;
	char *message = NULL;


	if ((mode = El_permissions_mode(el)) == NULL) {
		Nprint_h_err("No permission specified.");
		return -1;
	}

	if ((targets = El_permissions_targets(el)) == NULL) {
		Nprint_h_err("No targets specified.");
		xfree(mode);
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(targets);
		xfree(mode);

		return -1;
	}

	for (tok = strtok(targets, WHITE_SPACE); tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		append_str(&command, "chmod ");

		append_str(&message, "Changing permissions to ");
		append_str(&message, mode);
		append_str(&message, " ");
		if (recursive) {
			append_str(&command, "-R ");
			append_str(&message, "(recursive) ");
		}
		append_str(&message, "in ");
		append_str(&message, base);
		append_str(&message, ": ");
		append_str(&message, tok);

		append_str(&command, mode);
		append_str(&command, " ");
		append_str(&command, tok);

		Nprint_h("%s", message);

		if ((status = execute_command(command))) {
			Nprint_h_err("Changing permissions failed.");
			break;
		}

		xfree(command);
		command = NULL;
		xfree(message);
		message = NULL;
	}

	xfree(command);
	xfree(message);

	xfree(base);
	xfree(targets);
	xfree(mode);
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *permissions_parameters_ver3[] =
{ "option", "name", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "mode", NULL };

static int permissions_main_ver3(element_s *el)
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

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "2.0",
		.parameters = permissions_parameters_ver2,
		.main = permissions_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "3.0",
		.parameters = permissions_parameters_ver3,
		.main = permissions_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "3.1",
		.parameters = permissions_parameters_ver3,
		.main = permissions_main_ver3,
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
