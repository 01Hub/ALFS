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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME execute
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"
#include "options.h"


#if HANDLER_SYNTAX_2_0

static const char *execute_parameters_ver2[] =
{ "base", "command", "param", NULL };

static int execute_main_ver2(element_s * const el)
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

	status = execute_command(el, "%s", command);

	xfree(base);
	xfree(command);

	return status;
	
}

static char *execute_data_ver2(const element_s * const el,
			       const handler_data_e data)
{
	(void) data;

	return alloc_trimmed_param_value("command", el);
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *execute_parameters_ver3[] =
{ "param", "prefix", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "command", NULL };

static int execute_main_ver3(element_s * const el)
{
	int status;
	char *base;
	char *c, *command;


	if ((c = attr_value("command", el)) == NULL) {
		Nprint_h_err("No command specified.");
		return -1;
	}

	base = alloc_base_dir_new(el, 1);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	command = xstrdup("");

	append_prefix_elements(&command, el);

	append_str(&command, c);

	append_param_elements(&command, el);

	Nprint_h("Executing system command in %s:", base);
	Nprint_h("    %s", command);

	status = execute_command(el, "%s", command);

	xfree(base);
	xfree(command);

	return status;
}

static char *execute_data_ver3(const element_s * const el,
			       const handler_data_e data)
{
	char *command;

	(void) data;

	if ((command = attr_value("command", el))) {
		return xstrdup(command);
	}

	return NULL;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


#ifdef HANDLER_SYNTAX_3_2

static const char *execute_parameters_ver3_2[] =
{ "param", "prefix", "content", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "command", NULL };

static int execute_main_ver3_2(element_s * const el)
{
	int status = -1;
	char *base;
	char *c;
	char *content = NULL;

	c = attr_value("command", el);
	content = alloc_trimmed_param_value("content", el);

	if (!(c || content)) {
		Nprint_h_err("Either command or <content> must be specified.");
		return -1;
	}
	
	if (c && content) {
		Nprint_h_err("Cannot specify both command and <content>.");
		return -1;
	}

	if (content &&
	    (first_param("prefix", el) || first_param("param", el))) {
		Nprint_h_err("Cannot specify both <content> and <param>/<prefix>.");
		return -1;
	}

	base = alloc_base_dir_new(el, 1);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	if (c) {
		char *command;

		command = xstrdup("");
		append_prefix_elements(&command, el);
		append_str(&command, c);
		append_param_elements(&command, el);
		Nprint_h("Executing system command in %s:", base);
		Nprint_h("    %s", command);
		status = execute_command(el, "%s", command);
		xfree(command);
	} else {
		FILE *temp_script;
		char *tok;
		char *temp_file_name;
		char *args[2];

		temp_file_name = xstrdup(*opt_alfs_directory);
		append_str(&temp_file_name, "/");
		append_str(&temp_file_name, ".nALFS.XXXXXX");
		if (!create_temp_file(temp_file_name)) {
			if ((temp_script = fopen(temp_file_name, "w"))) {
				for (tok = strtok(content, "\n");
				     tok;
				     tok = strtok(NULL, "\n")) {
					fprintf(temp_script, "%s\n", ++tok);
				}
				fclose(temp_script);
				if (chmod(temp_file_name, S_IRUSR|S_IXUSR)) {
					Nprint_h_err("Cannot make temporary script executable.");
					Nprint_h_err("    %s (%s)", temp_file_name, strerror(errno));
				} else {
					Nprint_h("Executing script in %s:", base);
					args[0] = "nALFS_temp_script";
					args[1] = NULL;
					status = execute_direct_command(temp_file_name, args);
				}
				unlink(temp_file_name);
			} else {
				Nprint_h_err("Cannot open %s for writing.", temp_file_name);
			}
		}
		xfree(temp_file_name);
	}

	xfree(base);
	xfree(content);

	return status;
}

static char *execute_data_ver3_2(const element_s * const el,
				 const handler_data_e data)
{
	char *command;
	element_s *content_param;

	(void) data;

	if ((command = attr_value("command", el))) {
		return xstrdup(command);
	} else if ((content_param = first_param("content", el))) {
		return alloc_trimmed_str(content_param->content);
	}

	return NULL;
}

#endif


/*
 * Handlers' information.
 */

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
#if HANDLER_SYNTAX_3_2
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "3.2",
		.parameters = execute_parameters_ver3_2,
		.main = execute_main_ver3_2,
		.type = HTYPE_EXECUTE,
		.alloc_data = execute_data_ver3_2,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
#endif
	{
		.name = NULL
	}
};
