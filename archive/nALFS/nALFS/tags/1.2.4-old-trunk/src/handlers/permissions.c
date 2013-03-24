/*
 *  permissions.c - Handler.
 *
 *  Copyright (C) 2001-2004
 *
 *  Neven Has <haski@sezampro.yu>
 *  Maik Schreiber <bZ@iq-computing.de>
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

enum {
	PERMISSIONS_MODE,
	PERMISSIONS_NAME,
	PERMISSIONS_NAMES,
	PERMISSIONS_OPTION,
	PERMISSIONS_BASE,
};

struct permissions_data {
	char *base;
	int name_count;
	char **names;
	int recursive;
	char *mode;
};

static int permissions_setup(element_s * const element)
{
	struct permissions_data *data;

	if ((data = xmalloc(sizeof(struct permissions_data))) == NULL)
		return 1;

	data->recursive = 0;
	data->base = NULL;
	data->name_count = 0;
	data->names = NULL;
	data->mode = NULL;
	element->handler_data = data;

	return 0;
};

static void permissions_free(const element_s * const element)
{
	struct permissions_data *data = (struct permissions_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->mode);
	if (data->name_count > 0) {
		for (i = 0; i < data->name_count; i++)
			xfree(data->names[i]);
		xfree(data->names);
	}
	xfree(data);
}

static int permissions_attribute(const element_s * const element,
				 const struct handler_attribute * const attr,
				 const char * const value)
{
	struct permissions_data *data = (struct permissions_data *) element->handler_data;

	switch (attr->private) {
	case PERMISSIONS_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case PERMISSIONS_MODE:
		if (data->mode) {
			Nprint_err("<%s>: cannot specify \"mode\" more than once.", element->handler->name);
			return 1;
		}
		data->mode = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int permissions_parameter(const element_s * const element,
				 const struct handler_parameter * const param,
				 const char * const value)
{
	struct permissions_data *data = (struct permissions_data *) element->handler_data;
	char *tmp;
	char *tok;

	switch (param->private) {
	case PERMISSIONS_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case PERMISSIONS_OPTION:
		if (!strcmp("recursive", value)) {
			data->recursive = 1;
			return 0;
		}
		Nprint_err("<%s>: invalid option (%s) ignored", element->handler->name, value);
		return 1;
	case PERMISSIONS_NAME:
		data->name_count++;
		if ((data->names = xrealloc(data->names,
					    sizeof(data->names[0]) * (data->name_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->names[(data->name_count - 1)] = xstrdup(value);
		return 0;
	case PERMISSIONS_NAMES:
		tmp = xstrdup(value);
		for (tok = strtok(tmp, WHITE_SPACE); tok; tok = strtok(NULL, WHITE_SPACE)) {
			data->name_count++;
			if ((data->names = xrealloc(data->names,
						    sizeof(data->names[0]) * (data->name_count))) == NULL) {
				Nprint_err("xrealloc() failed: %s", strerror(errno));
				return -1;
			}
			data->names[(data->name_count - 1)] = xstrdup(value);
		}
		xfree(tmp);
	default:
		return 1;
	}
}

static int permissions_valid_data(const element_s * const element)
{
	struct permissions_data *data = (struct permissions_data *) element->handler_data;

	if (data->name_count == 0) {
		Nprint_err("<%s>: <name> must be specified.", element->handler->name);
		return 0;
	}

	if (!data->mode) {
		Nprint_err("<%s>: \"mode\" must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter permissions_parameters_v2[] = {
	{ .name = "base", .private = PERMISSIONS_BASE },
	{ .name = "options", .private = PERMISSIONS_OPTION },
	{ .name = "mode", .private = PERMISSIONS_MODE },
	{ .name = "name", .private = PERMISSIONS_NAMES },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter permissions_parameters_v3[] = {
	{ .name = "option", .private = PERMISSIONS_OPTION },
	{ .name = "name", .private = PERMISSIONS_NAME },
	{ .name = NULL }
};

static const struct handler_attribute permissions_attributes_v3[] = {
	{ .name = "base", .private = PERMISSIONS_BASE },
	{ .name = "mode", .private = PERMISSIONS_MODE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

static int permissions_main(const element_s * const element)
{
	struct permissions_data *data = (struct permissions_data *) element->handler_data;
	int status = 0;
	char *command = NULL;
	char *message = NULL;
	int i;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	for (i = 0; i < data->name_count; i++) {
		append_str_format(&command, "chmod %s %s %s",
				  data->recursive ? "-R" : "",
				  data->mode,
				  data->names[i]);
		append_str_format(&message, "Changing permissions to %s%s: %s",
				  data->mode,
				  data->recursive ? " (recursive) " : " ",
				  data->names[i]);

		Nprint_h("%s", message);

		if ((status = execute_command(element, command))) {
			Nprint_h_err("Changing permissions failed.");
			break;
		}

		xfree(command);
		command = NULL;
		xfree(message);
		message = NULL;
	}

	return status;
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "2.0",
		.parameters = permissions_parameters_v2,
		.main = permissions_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = permissions_setup,
		.free = permissions_free,
		.parameter = permissions_parameter,
		.valid_data = permissions_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "3.0",
		.parameters = permissions_parameters_v3,
		.attributes = permissions_attributes_v3,
		.main = permissions_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = permissions_setup,
		.free = permissions_free,
		.parameter = permissions_parameter,
		.attribute = permissions_attribute,
		.valid_data = permissions_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "3.1",
		.parameters = permissions_parameters_v3,
		.attributes = permissions_attributes_v3,
		.main = permissions_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = permissions_setup,
		.free = permissions_free,
		.parameter = permissions_parameter,
		.attribute = permissions_attribute,
		.valid_data = permissions_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "permissions",
		.description = "Change permissions",
		.syntax_version = "3.2",
		.parameters = permissions_parameters_v3,
		.attributes = permissions_attributes_v3,
		.main = permissions_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = permissions_setup,
		.free = permissions_free,
		.parameter = permissions_parameter,
		.attribute = permissions_attribute,
		.valid_data = permissions_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
