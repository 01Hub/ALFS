/*
 *  configure.c - Handler.
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

#define MODULE_NAME configure
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"
#include "options.h"

enum {
	CONFIGURE_BASE,
	CONFIGURE_PARAM,
	CONFIGURE_PREFIX,
	CONFIGURE_COMMAND,
};

struct configure_data {
	char *base;
	char *param;
	int param_seen;
	char *prefix;
	int prefix_seen;
	char *command;
};

static int configure_setup(element_s * const element)
{
	struct configure_data *data;

	if ((data = xmalloc(sizeof(struct configure_data))) == NULL)
		return -1;

	data->param = xstrdup(" ");
	data->param_seen = 0;
	data->prefix = xstrdup("");
	data->prefix_seen = 0;
	data->base = NULL;
	data->command = NULL;
	element->handler_data = data;

	return 0;
}

static void configure_free(const element_s * const element)
{
	struct configure_data *data = (struct configure_data *) element->handler_data;

	xfree(data->base);
	xfree(data->prefix);
	xfree(data->param);
	xfree(data->command);
	xfree(data);
}

static int configure_attribute(const element_s * const element,
			       const struct handler_attribute * const attr,
			       const char * const value)
{
	struct configure_data *data = (struct configure_data *) element->handler_data;

	switch (attr->private) {
	case CONFIGURE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case CONFIGURE_COMMAND:
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

static int configure_parameter(const element_s * const element,
			       const struct handler_parameter * const param,
			       const char * const value)
{
	struct configure_data *data = (struct configure_data *) element->handler_data;

	switch (param->private) {
	case CONFIGURE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case CONFIGURE_PREFIX:
		append_str_format(&data->prefix, "%s ", value);
		data->prefix_seen = 1;
		return 0;
	case CONFIGURE_PARAM:
		append_str_format(&data->param, "%s ", value);
		data->param_seen = 1;
		return 0;
	case CONFIGURE_COMMAND:
		if (data->command) {
			Nprint_err("<%s>: cannot specify <command> more than once.", element->handler->name);
			return 1;
		}
		data->command = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter configure_parameters_v2[] = {
	{ .name = "base", .private = CONFIGURE_BASE },
	{ .name = "param", .private = CONFIGURE_PARAM },
	{ .name = "command", .private = CONFIGURE_COMMAND },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter configure_parameters_v3[] = {
	{ .name = "prefix", .private = CONFIGURE_PREFIX },
	{ .name = "param", .private = CONFIGURE_PARAM },
	{ .name = NULL }
};

static const struct handler_attribute configure_attributes_v3[] = {
	{ .name = "base", .private = CONFIGURE_BASE },
	{ .name = "command", .private = CONFIGURE_COMMAND },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

static int configure_main(const element_s * const element)
{
	struct configure_data *data = (struct configure_data *) element->handler_data;
	int status;

	if (change_to_base_dir(element, data->base, 1))
		return -1;
	
	Nprint_h("Executing system command");
	Nprint_h("    %s %s %s", data->prefix,
		 (data->command) ? data->command : "./configure",
		 data->param);

	status = execute_command(element, "%s %s %s", data->prefix,
				 (data->command) ? data->command : "./configure",
				 data->param);

	return status;
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "2.0",
		.parameters = configure_parameters_v2,
		.main = configure_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = configure_setup,
		.free = configure_free,
		.parameter = configure_parameter,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "3.0",
		.parameters = configure_parameters_v3,
		.attributes = configure_attributes_v3,
		.main = configure_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = configure_setup,
		.free = configure_free,
		.parameter = configure_parameter,
		.attribute = configure_attribute,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "3.1",
		.parameters = configure_parameters_v3,
		.attributes = configure_attributes_v3,
		.main = configure_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = configure_setup,
		.free = configure_free,
		.parameter = configure_parameter,
		.attribute = configure_attribute,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "3.2",
		.parameters = configure_parameters_v3,
		.attributes = configure_attributes_v3,
		.main = configure_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = configure_setup,
		.free = configure_free,
		.parameter = configure_parameter,
		.attribute = configure_attribute,
	},
#endif
	{
		.name = NULL
	}
};
