/*
 *  link.c - Handler.
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

#define MODULE_NAME link
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


enum {
	LINK_BASE,
	LINK_OPTIONS,
	LINK_OPTION,
	LINK_TARGET,
	LINK_NAME,
	LINK_TYPE,
};

struct link_data {
	char *base;
	char *name;
	int target_count;
	char **targets;
	int force;
	int no_dereference;
	int hard_link;
};

static int link_setup(element_s * const element)
{
	struct link_data *data;

	if ((data = xmalloc(sizeof(struct link_data))) == NULL)
		return 1;

	data->hard_link = 0;
	data->force = 0;
	data->no_dereference = 0;
	data->base = NULL;
	data->name = NULL;
	data->target_count = 0;
	data->targets = NULL;
	element->handler_data = data;

	return 0;
};

static void link_free(const element_s * const element)
{
	struct link_data *data = (struct link_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->name);
	if (data->target_count > 0) {
		for (i = 0; i < data->target_count; i++)
			xfree(data->targets[i]);
		xfree(data->targets);
	}
	xfree(data);
}

static int link_attribute(const element_s * const element,
			  const struct handler_attribute * const attr,
			  const char * const value)
{
	struct link_data *data = (struct link_data *) element->handler_data;

	switch (attr->private) {
	case LINK_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case LINK_TYPE:
		if (!strcmp("symbolic", value)) {
			data->hard_link = 0;
		} else if (!strcmp("hard", value)) {
			data->hard_link = 1;
		} else {
			Nprint_err("<%s>: \"type\" must be \"symbolic\" or \"hard\".", element->handler->name);
			return 0;
		}
	default:
		return 1;
	}
}

static int link_parameter(const element_s * const element,
			  const struct handler_parameter * const param,
			  const char * const value)
{
	struct link_data *data = (struct link_data *) element->handler_data;

	switch (param->private) {
	case LINK_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case LINK_OPTIONS:
		if (option_in_string("force", value)) {
			data->force = 1;
			return 0;
		} else {
			Nprint_err("<%s>: invalid options in (%s) ignored", element->handler->name, value);
			return 1;
		}
	case LINK_OPTION:
		if (!strcmp("force", value)) {
			data->force = 1;
			return 0;
		}
		if (!strcmp("no-dereference", value)) {
			data->no_dereference = 1;
			return 0;
		}
		Nprint_err("<%s>: invalid option (%s) ignored", element->handler->name, value);
		return 1;
	case LINK_TARGET:
		data->target_count++;
		if ((data->targets = xrealloc(data->targets,
					      sizeof(data->targets[0]) * (data->target_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->targets[(data->target_count - 1)] = xstrdup(value);
		return 0;
	case LINK_NAME:
		if (data->name) {
			Nprint_err("<%s>: cannot specify <name> more than once.", element->handler->name);
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int link_valid_data(const element_s * const element)
{
	struct link_data *data = (struct link_data *) element->handler_data;

	if (data->target_count == 0) {
		Nprint_err("<%s>: <target> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter link_parameters_v2[] = {
	{ .name = "base", .private = LINK_BASE },
	{ .name = "options", .private = LINK_OPTIONS },
	{ .name = "target", .private = LINK_TARGET },
	{ .name = "name", .private = LINK_NAME },
	{ .name = NULL }
};

static const struct handler_attribute link_attributes_v2[] = {
	{ .name = "type", .private = LINK_TYPE },
	{ .name = NULL }
};

static int link_main_ver2(const element_s * const element)
{
	struct link_data *data = (struct link_data *) element->handler_data;
	int status;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	if (data->name) {
		Nprint_h("Creating %s link %s%s -> %s",
			 data->hard_link ? "hard" : "symbolic",
			 data->force ? "(force) " : "",
			 data->name, data->targets[0]);
		status = execute_command(element, "ln %s %s %s %s",
					 data->hard_link ? "" : "-s",
					 data->force ? "-f" : "",
					 data->targets[0],
					 data->name);
	} else {
		Nprint_h("Creating %s link %s%s",
			 data->hard_link ? "hard" : "symbolic",
			 data->force ? "(force) " : "",
			 data->targets[0]);
		status = execute_command(element, "ln %s %s %s",
					 data->hard_link ? "" : "-s",
					 data->force ? "-f" : "",
					 data->targets[0]);
	}

	if (status)
		Nprint_h_err("Creating link failed.");

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter link_parameters_v3[] = {
	{ .name = "option", .private = LINK_OPTION },
	{ .name = "target", .private = LINK_TARGET },
	{ .name = "name", .private = LINK_NAME },
	{ .name = NULL }
};

static const struct handler_attribute link_attributes_v3[] = {
	{ .name = "type", .private = LINK_TYPE },
	{ .name = "base", .private = LINK_BASE },
	{ .name = NULL }
};

static int link_main_ver3(const element_s * const element)
{
	struct link_data *data = (struct link_data *) element->handler_data;
	int status;
	char *targets = NULL;
	int i;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	for (i = 0; i < data->target_count; i++) {
		append_str_format(&targets, "%s ", data->targets[i]);
	}

	if (data->name) {
		Nprint_h("Creating %s link %s%s%s",
			 data->hard_link ? "hard" : "symbolic",
			 data->force ? "(force) " : "",
			 data->no_dereference ? "(no-dereference) " : "",
			 data->name);
		Nprint_h("    %s", targets);
		status = execute_command(element, "ln %s %s %s %s %s",
					 data->hard_link ? "" : "-s",
					 data->force ? "-f" : "",
					 data->no_dereference ? "-n" : "",
					 targets,
					 data->name);
	} else {
		Nprint_h("Creating %s link %s%s",
			 data->hard_link ? "hard" : "symbolic",
			 data->force ? "(force) " : "",
			 data->no_dereference ? "(no-dereference) " : "");
		Nprint_h("    %s", targets);
		status = execute_command(element, "ln %s %s %s %s",
					 data->hard_link ? "" : "-s",
					 data->force ? "-f" : "",
					 data->no_dereference ? "-n" : "",
					 targets);
	}

	xfree(targets);

	if (status)
		Nprint_h_err("Creating link failed.");

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "2.0",
		.parameters = link_parameters_v2,
		.attributes = link_attributes_v2,
		.main = link_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = link_setup,
		.free = link_free,
		.attribute = link_attribute,
		.parameter = link_parameter,
		.valid_data = link_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "3.0",
		.parameters = link_parameters_v3,
		.attributes = link_attributes_v3,
		.main = link_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = link_setup,
		.free = link_free,
		.attribute = link_attribute,
		.parameter = link_parameter,
		.valid_data = link_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "3.1",
		.parameters = link_parameters_v3,
		.attributes = link_attributes_v3,
		.main = link_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = link_setup,
		.free = link_free,
		.attribute = link_attribute,
		.parameter = link_parameter,
		.valid_data = link_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "link",
		.description = "Link",
		.syntax_version = "3.2",
		.parameters = link_parameters_v3,
		.attributes = link_attributes_v3,
		.main = link_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = link_setup,
		.free = link_free,
		.attribute = link_attribute,
		.parameter = link_parameter,
		.valid_data = link_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
