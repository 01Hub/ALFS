/*
 *  mkdir.c - Handler.
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


enum {
	MKDIR_OPTION,
	MKDIR_BASE,
	MKDIR_NAME,
	MKDIR_NAMES,
	MKDIR_PERMISSIONS,
};

struct mkdir_data {
	char *base;
	int parents;
	char *permissions;
	int name_count;
	char **names;
};

static int mkdir_setup(element_s * const element)
{
	struct mkdir_data *data;

	if ((data = xmalloc(sizeof(struct mkdir_data))) == NULL)
		return 1;

	data->parents = 0;
	data->base = NULL;
	data->name_count = 0;
	data->names = NULL;
	data->permissions = NULL;
	element->handler_data = data;

	return 0;
};

static void mkdir_free(const element_s * const element)
{
	struct mkdir_data *data = (struct mkdir_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->permissions);
	if (data->name_count > 0) {
		for (i = 0; i < data->name_count; i++)
			xfree(data->names[i]);
		xfree(data->names);
	}
	xfree(data);
}

static int mkdir_attribute(const element_s * const element,
			   const struct handler_attribute * const attr,
			   const char * const value)
{
	struct mkdir_data *data = (struct mkdir_data *) element->handler_data;

	switch (attr->private) {
	case MKDIR_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int mkdir_parameter(const element_s * const element,
			   const struct handler_parameter * const param,
			   const char * const value)
{
	struct mkdir_data *data = (struct mkdir_data *) element->handler_data;
	char *tmp;
	char *tok;

	switch (param->private) {
	case MKDIR_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case MKDIR_PERMISSIONS:
		if (data->permissions) {
			Nprint_err("<%s>: cannot specify <permissions> more than once.", element->handler->name);
			return 1;
		}
		data->permissions = xstrdup(value);
		return 0;
	case MKDIR_OPTION:
		if (!strcmp("parents", value)) {
			data->parents = 1;
			return 0;
		}
		Nprint_err("<%s>: invalid option (%s) ignored", element->handler->name, value);
		return 1;
	case MKDIR_NAME:
		data->name_count++;
		if ((data->names = xrealloc(data->names,
					    sizeof(data->names[0]) * (data->name_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->names[(data->name_count - 1)] = xstrdup(value);
		return 0;
	case MKDIR_NAMES:
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

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter mkdir_parameters_ver2[] = {
	{ .name = "options", .private = MKDIR_OPTION },
	{ .name = "base", .private = MKDIR_BASE },
	{ .name = "dir", .private = MKDIR_NAMES },
	{ .name = "permissions", .private = MKDIR_PERMISSIONS },
	{ .name = NULL }
};

static int mkdir_valid_data_v2(const element_s * const element)
{
	struct mkdir_data *data = (struct mkdir_data *) element->handler_data;

	if (data->name_count == 0) {
		Nprint_err("<%s>: \"dir\" must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter mkdir_parameters_v3[] = {
	{ .name = "option", .private = MKDIR_OPTION },
	{ .name = "name", .private = MKDIR_NAME },
	{ .name = "permissions", .private = MKDIR_PERMISSIONS },
	{ .name = NULL }
};

static const struct handler_attribute mkdir_attributes_v3[] = {
	{ .name = "base", .private = MKDIR_BASE },
	{ .name = NULL }
};

static int mkdir_valid_data_v3(const element_s * const element)
{
	struct mkdir_data *data = (struct mkdir_data *) element->handler_data;

	if (data->name_count == 0) {
		Nprint_err("<%s>: <name> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

static int mkdir_main(const element_s * const element)
{
	struct mkdir_data *data = (struct mkdir_data *) element->handler_data;
	int status = 0;
	int i;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	for (i = 0; i < data->name_count; i++) {
		if (data->permissions) {
			Nprint_h("Creating directory %s%s: (%s)",
				 data->names[i], data->parents ? " (parents)" : "",
				 data->permissions);
		} else {
			Nprint_h("Creating directory %s%s",
				 data->names[i], data->parents ? " (parents)" : "");
		}

		status = execute_command(element, "mkdir %s %s",
					 data->parents ? "-p" : "", data->names[i]);

		if (status) {
			Nprint_h_err("Creation failed.");
			break;
		}

		if (data->permissions) {
			/* Change permissions. */
			if ((status = execute_command(element, "chmod %s %s",
						      data->permissions,
						      data->names[i]))) {
				Nprint_h_err("Changing permissions failed.");
				break;
			}
		}
	}

	return status;
}

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
		.main = mkdir_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = mkdir_setup,
		.free = mkdir_free,
		.parameter = mkdir_parameter,
		.valid_data = mkdir_valid_data_v2,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "mkdir",
		.description = "Make directories",
		.syntax_version = "3.0",
		.parameters = mkdir_parameters_v3,
		.attributes = mkdir_attributes_v3,
		.main = mkdir_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = mkdir_setup,
		.free = mkdir_free,
		.parameter = mkdir_parameter,
		.attribute = mkdir_attribute,
		.valid_data = mkdir_valid_data_v3,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "mkdir",
		.description = "Make directories",
		.syntax_version = "3.1",
		.parameters = mkdir_parameters_v3,
		.attributes = mkdir_attributes_v3,
		.main = mkdir_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = mkdir_setup,
		.free = mkdir_free,
		.parameter = mkdir_parameter,
		.attribute = mkdir_attribute,
		.valid_data = mkdir_valid_data_v3,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "mkdir",
		.description = "Make directories",
		.syntax_version = "3.2",
		.parameters = mkdir_parameters_v3,
		.attributes = mkdir_attributes_v3,
		.main = mkdir_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = mkdir_setup,
		.free = mkdir_free,
		.parameter = mkdir_parameter,
		.attribute = mkdir_attribute,
		.valid_data = mkdir_valid_data_v3,
	},
#endif
	{
		.name = NULL
	}
};
