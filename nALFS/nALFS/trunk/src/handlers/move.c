/*
 *  move.c - Handler.
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

#define MODULE_NAME move
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


enum {
	MOVE_BASE,
	MOVE_OPTION,
	MOVE_SOURCES,
	MOVE_SOURCE,
	MOVE_DESTINATION,
};

struct move_data {
	char *base;
	int force;
	char *destination;
	int source_count;
	char **sources;
};

static int move_setup(element_s * const element)
{
	struct move_data *data;

	if ((data = xmalloc(sizeof(struct move_data))) == NULL)
		return 1;

	data->force = 0;
	data->base = NULL;
	data->source_count = 0;
	data->sources = NULL;
	data->destination = NULL;
	element->handler_data = data;

	return 0;
};

static void move_free(const element_s * const element)
{
	struct move_data *data = (struct move_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->destination);
	if (data->source_count > 0) {
		for (i = 0; i < data->source_count; i++)
			xfree(data->sources[i]);
		xfree(data->sources);
	}
	xfree(data);
}

static int move_attribute(const element_s * const element,
			  const struct handler_attribute * const attr,
			  const char * const value)
{
	struct move_data *data = (struct move_data *) element->handler_data;

	switch (attr->private) {
	case MOVE_BASE:
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

static int move_parameter(const element_s * const element,
			  const struct handler_parameter * const param,
			  const char * const value)
{
	struct move_data *data = (struct move_data *) element->handler_data;
	char *tmp;
	char *tok;

	switch (param->private) {
	case MOVE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case MOVE_DESTINATION:
		if (data->destination) {
			Nprint_err("<%s>: cannot specify <destination> more than once.", element->handler->name);
			return 1;
		}
		data->destination = xstrdup(value);
		return 0;
	case MOVE_OPTION:
		if (!strcmp("force", value)) {
			data->force = 1;
			return 0;
		}
		Nprint_err("<%s>: invalid option (%s) ignored", element->handler->name, value);
		return 1;
	case MOVE_SOURCE:
		data->source_count++;
		if ((data->sources = xrealloc(data->sources,
					      sizeof(data->sources[0]) * (data->source_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->sources[(data->source_count - 1)] = xstrdup(value);
		return 0;
	case MOVE_SOURCES:
		tmp = xstrdup(value);
		for (tok = strtok(tmp, WHITE_SPACE); tok; tok = strtok(NULL, WHITE_SPACE)) {
			data->source_count++;
			if ((data->sources = xrealloc(data->sources,
						      sizeof(data->sources[0]) * (data->source_count))) == NULL) {
				Nprint_err("xrealloc() failed: %s", strerror(errno));
				return -1;
			}
			data->sources[(data->source_count - 1)] = xstrdup(value);
		}
		xfree(tmp);
	default:
		return 1;
	}
}

static int move_valid_data(const element_s * const element)
{
	struct move_data *data = (struct move_data *) element->handler_data;

	if (data->source_count == 0) {
		Nprint_err("<%s>: <source> must be specified.", element->handler->name);
		return 0;
	}

	if (!data->destination) {
		Nprint_err("<%s>: <destination> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter move_parameters_v2[] = {
	{ .name = "base", .private = MOVE_BASE },
	{ .name = "options", .private = MOVE_OPTION },
	{ .name = "source", .private = MOVE_SOURCES },
	{ .name = "destination", .private = MOVE_DESTINATION },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter move_parameters_v3[] = {
	{ .name = "option", .private = MOVE_OPTION },
	{ .name = "source", .private = MOVE_SOURCE },
	{ .name = "destination", .private = MOVE_DESTINATION },
	{ .name = NULL }
};

static const struct handler_attribute move_attributes_v3[] = {
	{ .name = "base", .private = MOVE_BASE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

static int move_main(const element_s * const element)
{
	struct move_data *data = (struct move_data *) element->handler_data;
	int status = 0;
	int i;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	for (i = 0; i < data->source_count; i++) {
		Nprint_h("Moving from %s to %s%s",
			 data->sources[i], data->destination, data->force ? " (force)" : "");

		status = execute_command(element, "mv %s %s %s",
					 data->force ? "-f" : "",
					 data->sources[i], data->destination);

		if (status) {
			Nprint_h_err("Moving failed.");
			break;
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
		.name = "move",
		.description = "Move files",
		.syntax_version = "2.0",
		.parameters = move_parameters_v2,
		.main = move_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = move_setup,
		.free = move_free,
		.valid_data = move_valid_data,
		.parameter = move_parameter,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "move",
		.description = "Move files",
		.syntax_version = "3.0",
		.parameters = move_parameters_v3,
		.attributes = move_attributes_v3,
		.main = move_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = move_setup,
		.free = move_free,
		.valid_data = move_valid_data,
		.parameter = move_parameter,
		.attribute = move_attribute,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "move",
		.description = "Move files",
		.syntax_version = "3.1",
		.parameters = move_parameters_v3,
		.attributes = move_attributes_v3,
		.main = move_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = move_setup,
		.free = move_free,
		.valid_data = move_valid_data,
		.attribute = move_attribute,
		.parameter = move_parameter,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "move",
		.description = "Move files",
		.syntax_version = "3.2",
		.parameters = move_parameters_v3,
		.attributes = move_attributes_v3,
		.main = move_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = move_setup,
		.free = move_free,
		.valid_data = move_valid_data,
		.parameter = move_parameter,
		.attribute = move_attribute,
	},
#endif
	{
		.name = NULL
	}
};
