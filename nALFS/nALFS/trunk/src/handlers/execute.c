/*
 *  execute.c - Handler.
 * 
 *  Copyright (C) 2001-2004
 *  
 *  Neven Has <haski@sezampro.yu>
 *  Kevin P. Fleming <kpfleming@linuxfromscratch.org>
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

enum {
	EXECUTE_BASE,
	EXECUTE_COMMAND,
	EXECUTE_PARAM,
	EXECUTE_PREFIX,
	EXECUTE_CONTENT,
};

struct execute_data {
	char *base;
	char *command;
	char *param;
	int param_seen;
	char *prefix;
	int prefix_seen;
	char *content;
};

static int execute_setup(element_s * const element)
{
	struct execute_data *data;

	if ((data = xmalloc(sizeof(struct execute_data))) == NULL)
		return -1;

	data->command = NULL;
	data->param = xstrdup(" ");
	data->param_seen = 0;
	data->prefix = xstrdup("");
	data->prefix_seen = 0;
	data->base = NULL;
	data->content = NULL;
	element->handler_data = data;

	return 0;
}

static void execute_free(const element_s * const element)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	xfree(data->base);
	xfree(data->command);
	xfree(data->prefix);
	xfree(data->param);
	xfree(data->content);
	xfree(data);
}

static int execute_attribute(const element_s * const element,
			     const struct handler_attribute * const attr,
			     const char * const value)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	switch (attr->private) {
	case EXECUTE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case EXECUTE_COMMAND:
		if (data->command) {
			Nprint_err("<%s>: cannot specify \"command\" more than once.", element->handler->name);
			return 1;
		}
		data->command = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int execute_parameter(const element_s * const element,
			     const struct handler_parameter * const param,
			     const char * const value)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	switch (param->private) {
	case EXECUTE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case EXECUTE_COMMAND:
		if (data->command) {
			Nprint_err("<%s>: cannot specify <command> more than once.", element->handler->name);
			return 1;
		}
		data->command = xstrdup(value);
		return 0;
	case EXECUTE_CONTENT:
		if (data->content) {
			Nprint_err("<%s>: cannot specify <content> more than once.", element->handler->name);
			return 1;
		}
		data->content = xstrdup(value);
		return 0;
	case EXECUTE_PREFIX:
		append_str_format(&data->prefix, "%s ", value);
		data->prefix_seen = 1;
		return 0;
	case EXECUTE_PARAM:
		append_str_format(&data->param, "%s ", value);
		data->param_seen = 1;
		return 0;
	default:
		return 1;
	}
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter execute_parameters_v2[] = {
	{ .name = "base", .private = EXECUTE_BASE },
	{ .name = "command", .private = EXECUTE_COMMAND },
	{ .name = "param", .private = EXECUTE_PARAM },
	{ .name = NULL }
};

static int execute_valid_data_v2(const element_s * const element)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	if (!data->command) {
		Nprint_err("<%s>: <command> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const struct handler_parameter execute_parameters_v3[] = {
	{ .name = "prefix", .private = EXECUTE_PREFIX },
	{ .name = "param", .private = EXECUTE_PARAM },
	{ .name = NULL }
};

static const struct handler_attribute execute_attributes_v3[] = {
	{ .name = "command", .private = EXECUTE_COMMAND },
	{ .name = "base", .private = EXECUTE_BASE },
	{ .name = NULL }
};

static int execute_valid_data_v3(const element_s * const element)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	if (!data->command) {
		Nprint_err("<%s>: \"command\" must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_2_0 || HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static int execute_main(const element_s * const element)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;
	int status;

	if (change_to_base_dir(element, data->base, 1))
		return -1;
	
	Nprint_h("Executing system command");
	Nprint_h("    %s%s%s", data->prefix, data->command, data->param);

	status = execute_command(element, "%s%s%s", data->prefix, data->command, data->param);

	return status;
}

static char *execute_data(const element_s * const element,
			  const handler_data_e data_requested)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_DISPLAY_NAME:
	{
		char *display = NULL;

		append_str_format(&display, "Execute %s",
				  data->command ? data->command : "script");

		return display;
	}
	default:
		break;
	}

	return NULL;
}

#endif /* HANDLER_SYNTAX_2_0 || HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#ifdef HANDLER_SYNTAX_3_2

static const struct handler_parameter execute_parameters_v3_2[] = {
	{ .name = "prefix", .private = EXECUTE_PREFIX },
	{ .name = "param", .private = EXECUTE_PARAM },
	{ .name = "content", .private = EXECUTE_CONTENT },
	{ .name = NULL }
};

static const struct handler_attribute execute_attributes_v3_2[] = {
	{ .name = "command", .private = EXECUTE_COMMAND },
	{ .name = "base", .private = EXECUTE_BASE },
	{ .name = NULL }
};

static int execute_valid_data_v3_2(const element_s * const element)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;

	if (!(data->command || data->content)) {
		Nprint_err("<%s>: either \"command\" or <content> must be specified.", element->handler->name);
		return 0;
	}

	if (data->command && data->content) {
		Nprint_err("<%s>: cannot specify both \"command\" and <content>.", element->handler->name);
		return 0;
	}

	if (data->content && data->param_seen) {
		Nprint_err("<%s>: cannot specify both <content> and <param>.", element->handler->name);
		return 0;
	}

	if (data->content && data->prefix_seen) {
		Nprint_err("<%s>: cannot specify both <content> and <prefix>.", element->handler->name);
		return 0;
	}

	return 1;
}

static int execute_main_ver3_2(const element_s * const element)
{
	struct execute_data *data = (struct execute_data *) element->handler_data;
	int status = -1;

	if (change_to_base_dir(element, data->base, 1))
		return -1;
	
	if (data->command) {
		Nprint_h("Executing system command:");
		Nprint_h("    %s%s%s", data->prefix, data->command, data->param);
		status = execute_command(element, "%s%s%s", data->prefix, data->command,
					 data->param);
	} else {
		FILE *temp_script;
		char *tok;
		char *temp_file_name = NULL;
		char *args[2];

		append_str_format(&temp_file_name, "%s/.nALFS.XXXXXX",
				  *opt_alfs_directory);
		if (!create_temp_file(temp_file_name)) {
			if ((temp_script = fopen(temp_file_name, "w"))) {
				char *tmp = xstrdup(data->content);
				for (tok = strtok(tmp, "\n"); tok; tok = strtok(NULL, "\n")) {
					fprintf(temp_script, "%s\n", ++tok);
				}
				fclose(temp_script);
				xfree(tmp);
				if (chmod(temp_file_name, S_IRUSR|S_IXUSR)) {
					Nprint_h_err("Cannot make temporary script executable.");
					Nprint_h_err("    %s (%s)", temp_file_name, strerror(errno));
				} else {
					Nprint_h("Executing script");
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

	return status;
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
		.parameters = execute_parameters_v2,
		.main = execute_main,
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME,
		.alloc_data = execute_data,
		.is_action = 1,
		.setup = execute_setup,
		.free = execute_free,
		.parameter = execute_parameter,
		.valid_data = execute_valid_data_v2,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "3.0",
		.parameters = execute_parameters_v3,
		.attributes = execute_attributes_v3,
		.main = execute_main,
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME,
		.alloc_data = execute_data,
		.is_action = 1,
		.setup = execute_setup,
		.free = execute_free,
		.parameter = execute_parameter,
		.attribute = execute_attribute,
		.valid_data = execute_valid_data_v3,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "3.1",
		.parameters = execute_parameters_v3,
		.attributes = execute_attributes_v3,
		.main = execute_main,
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME,
		.alloc_data = execute_data,
		.is_action = 1,
		.setup = execute_setup,
		.free = execute_free,
		.parameter = execute_parameter,
		.attribute = execute_attribute,
		.valid_data = execute_valid_data_v3,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "execute",
		.description = "Execute",
		.syntax_version = "3.2",
		.parameters = execute_parameters_v3_2,
		.attributes = execute_attributes_v3_2,
		.main = execute_main_ver3_2,
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME,
		.alloc_data = execute_data,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = execute_setup,
		.free = execute_free,
		.parameter = execute_parameter,
		.attribute = execute_attribute,
		.valid_data = execute_valid_data_v3_2,
	},
#endif
	{
		.name = NULL
	}
};
