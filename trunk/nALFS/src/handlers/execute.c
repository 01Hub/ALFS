/*
 *  execute.c - Handler.
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

#define MODULE_NAME execute
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "win.h"
#include "parser.h"
#include "backend.h"


static int execute_main_ver2(element_s *el)
{
	int status;
	char *base;
	char *command;


	if ((command = alloc_trimmed_param_value("command", el)) == NULL) {
		Nprint_h_err("No command specified.");
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(command);
		return -1;
	}

	append_param_elements(&command, el);

	Nprint_h("Executing system command in %s:", base);
	Nprint_h("    %s", command);

	status = execute_command("%s", command);

	xfree(base);
	xfree(command);

	return status;
	
}

static char *execute_data_ver2(element_s *el, handler_data_e data)
{
	(void) data;

	return alloc_trimmed_param_value("command", el);
}


static int execute_main_ver3(element_s *el)
{
	int status;
	char *base;
	char *c, *command;


	if ((c = attr_value("command", el)) == NULL) {
		Nprint_h_err("No command specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	command = xstrdup(c);
	append_param_elements(&command, el);

	Nprint_h("Executing system command in %s:", base);
	Nprint_h("    %s", command);

	status = execute_command("%s", command);

	xfree(base);
	xfree(command);

	return status;
}

static char *execute_data_ver3(element_s *el, handler_data_e data)
{
	char *command;

	(void) data;

	if ((command = attr_value("command", el))) {
		return xstrdup(command);
	}

	return NULL;
}


/*
 * Handlers' information.
 */

const char *execute_parameters_ver2[] = { "base", "command", "param", NULL };

const char *execute_parameters_ver3[] = { "base", "command", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "command", NULL };

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "2.0",
		.parameters = execute_parameters_ver2,
		.main = execute_main_ver2,
		.type = HTYPE_EXECUTE,
		.alloc_data = execute_data_ver2,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "3.0",
		.parameters = execute_parameters_ver3,
		.main = execute_main_ver3,
		.type = HTYPE_EXECUTE,
		.alloc_data = execute_data_ver3,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "3.1",
		.parameters = execute_parameters_ver3,
		.main = execute_main_ver3,
		.type = HTYPE_EXECUTE,
		.alloc_data = execute_data_ver3,
		.is_action = 1,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
