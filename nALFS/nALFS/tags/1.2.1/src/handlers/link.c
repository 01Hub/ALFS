/*
 *  link.c - Handler.
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
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME link
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#define El_link_target(el) alloc_trimmed_param_value("target", el)
#define El_link_name(el) alloc_trimmed_param_value("name", el)


#if HANDLER_SYNTAX_2_0

static const char *link_parameters_ver2[] =
{ "options", "base", "target", "name", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "type", NULL };

static int link_main_ver2(element_s *el)
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

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *link_parameters_ver3[] =
{ "option", "target", "name", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "type", "base", NULL };

static int link_main_ver3(element_s *el)
{
	int options[2], force, no_dereference;
	int status;
	char *type;
	char *base;
	char *targets = NULL;
	char *link_name;
	char *command = NULL;
	char *message = NULL;
	element_s *p;


	/* Read all <option>s. */
	check_options(2, options, "force no-dereference", el);
	force = options[0];
	no_dereference = options[1];

	link_name = alloc_trimmed_param_value("name", el);

	base = alloc_base_dir_new(el);
	if (change_current_dir(base)) {
		xfree(base);
		xfree(link_name);
		return -1;
	}

	type = attr_value("type", el);

	if (type == NULL || strcmp(type, "symbolic") == 0) {
		append_str(&message, "Creating symbolic link in ");
		append_str(&command, "ln -s");

	} else if (strcmp(type, "hard") == 0) {
		append_str(&message, "Creating hard link in ");
		append_str(&command, "ln");

	} else {
		Nprint_h_warn("Unknown link type (%s), using symbolic.", type);
		append_str(&message, "Creating symbolic link in ");
		append_str(&command, "ln -s");
	}

	append_str(&message, base);

	if (force) {
		append_str(&message, " (force)");
		append_str(&command, " -f");
	}

	if (no_dereference) {
		append_str(&message, " (no_dereference)");
		append_str(&command, " -n");
	}

	append_str(&message, ": ");

	if (link_name) {
		append_str(&message, link_name);
	}

	/* Concatenate all targets in "targets". */
	for (p = first_param("target", el); p; p = next_param(p)) {
		char *target;


		if ((target = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_h_warn("No target specified.");
			continue;
		}

		if (targets != NULL) {
			append_str(&targets, " ");
		}

		append_str(&targets, target);

		xfree(target);
	}

	if (targets) {
		append_str(&command, " ");
		append_str(&command, targets);

		if (link_name) {
			append_str(&command, " ");
			append_str(&command, link_name);
		}

		Nprint_h("%s", message);
		Nprint_h("    %s", targets);

		if ((status = execute_command("%s", command))) {
			Nprint_h_err("Executing \"%s\" in %s failed.",
				command, base);
		}

	} else {
		Nprint_h_err("No target for link specified.");
		status = -1;
	}

	xfree(base);
	xfree(link_name);
	xfree(command);
	xfree(message);

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "2.0",
		.parameters = link_parameters_ver2,
		.main = link_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "3.0",
		.parameters = link_parameters_ver3,
		.main = link_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "3.1",
		.parameters = link_parameters_ver3,
		.main = link_main_ver3,
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
