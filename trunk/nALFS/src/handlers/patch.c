/*
 *  patch.c - Handler.
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

#define MODULE_NAME patch
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#if HANDLER_SYNTAX_2_0

static const char *patch_parameters_ver2[] = { "base", "param", NULL };

static int patch_main_ver2(element_s *el)
{
	int status;
	char *base;
	char *parameters = NULL;


	if (append_param_elements(&parameters, el) == NULL) {
		Nprint_h_err("No patch parameters specified.");
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(parameters);
		return -1;
	}
	
	Nprint_h("Patching in %s", base);
	Nprint_h("    patch %s", parameters);

	if ((status = execute_command("patch %s", parameters))) {
		Nprint_h_err("Patching failed.");
	}
	
	xfree(base);
	xfree(parameters);

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *patch_parameters_ver3[] = { "param", "prefix", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", NULL };

static int patch_main_ver3(element_s *el)
{
	int status;
	char *base;
	char *parameters = NULL;
	char *command;


	if (append_param_elements(&parameters, el) == NULL) {
		Nprint_h_err("No patch parameters specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(parameters);
		return -1;
	}
	
	Nprint_h("Patching in %s", base);
	command = xstrdup("");

	append_prefix_elements(&command, el);

	/* the trailing space on the string below is important, since
	   append_param_elements will not have put a space at the
	   beginning of the parameters string (since it was NULL to
	   begin with)
	*/

	append_str(&command, "patch ");

	append_str(&command, parameters);

	Nprint_h("    %s", command);

	if ((status = execute_command(command))) {
		Nprint_h_err("Patching failed.");
	}
	
	xfree(command);
	xfree(base);
	xfree(parameters);

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "2.0",
		.parameters = patch_parameters_ver2,
		.main = patch_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.0",
		.parameters = patch_parameters_ver3,
		.main = patch_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.1",
		.parameters = patch_parameters_ver3,
		.main = patch_main_ver3,
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
