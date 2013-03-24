/*
 *  copy.c - Handler.
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

#define MODULE_NAME copy
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#define El_copy_source(el) alloc_trimmed_param_value("source", el)
#define El_copy_destination(el) alloc_trimmed_param_value("destination", el)


#if HANDLER_SYNTAX_2_0

static const char *copy_parameters_ver2[] =
{ "options", "base", "source", "destination", NULL };

static int copy_main_ver2(element_s *el)
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

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *copy_parameters_ver3[] =
{ "option", "source", "destination", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", NULL };

static int copy_main_ver3(element_s *el)
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

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "2.0",
		.parameters = copy_parameters_ver2,
		.main = copy_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "3.0",
		.parameters = copy_parameters_ver3,
		.main = copy_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "3.1",
		.parameters = copy_parameters_ver3,
		.main = copy_main_ver3,
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
