/*
 *  mkdir.c - Handler.
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

#define MODULE_NAME mkdir
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#define El_mkdir_dirs(el) alloc_trimmed_param_value("dir", el)
#define El_mkdir_perm(el) alloc_trimmed_param_value("permissions", el)


#if HANDLER_SYNTAX_2_0

static const char *mkdir_parameters_ver2[] =
{ "options", "base", "dir", "permissions", NULL };

static int mkdir_main_ver2(element_s *el)
{
	int status = 0;
	int parents = option_exists("parents", el);
	char *tok;
	char *base;
	char *directories;
	char *perm;


	if ((directories = El_mkdir_dirs(el))== NULL) {
		Nprint_h_err("No directories specified.");
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(directories);
		return -1;
	}

	perm = El_mkdir_perm(el);

	for (tok = strtok(directories, WHITE_SPACE); tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		if (perm) {
			Nprint_h("Creating directory in %s%s: %s (%s)",
				base, parents ? " (parents)" : "", tok, perm);
		} else {
			Nprint_h("Creating directory in %s%s: %s",
				base, parents ? " (parents)" : "", tok);
		}

		if (parents) {
			status = execute_command("mkdir -p %s", tok);
		} else {
			status = execute_command("mkdir %s", tok);
		}

		if (status) {
			Nprint_h_err("Creating %s failed.", tok);
			break;
		}

		if (! perm) {
			continue;
		}

		/* Change permissions. */
		if ((status = execute_command("chmod %s %s", perm, tok))) {
			Nprint_h_err("Changing permissions failed.");
			break;
		}
	}

	xfree(base);
	xfree(directories);
	xfree(perm);
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *mkdir_parameters_ver3[] =
{ "option", "name", "permissions", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", NULL };

static int mkdir_main_ver3(element_s *el)
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

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "mkdir",
		.description = "Make directories",
		.syntax_version = "2.0",
		.parameters = mkdir_parameters_ver2,
		.main = mkdir_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "mkdir",
		.description = "Make directories",
		.syntax_version = "3.0",
		.parameters = mkdir_parameters_ver3,
		.main = mkdir_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "mkdir",
		.description = "Make directories",
		.syntax_version = "3.1",
		.parameters = mkdir_parameters_ver3,
		.main = mkdir_main_ver3,
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
