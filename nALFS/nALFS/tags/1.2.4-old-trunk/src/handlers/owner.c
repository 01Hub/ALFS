/*
 *  owner.c - Handler.
 *
 *  Copyright (C) 2002, 2004
 *
 *  Maik Schreiber <bZ@iq-computing.de>
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

#define MODULE_NAME owner
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"

enum {
	OWNER_OPTIONS,
	OWNER_NAMES,
	OWNER_BASE,
	OWNER_USER,
	OWNER_GROUP,
};

struct owner_data {
	char *base;
	char *user;
	char *group;
	int recursive;
	int name_count;
	char **names;
};

static const struct handler_parameter owner_parameters[] = {
	{ .name = "options", .private = OWNER_OPTIONS },
	{ .name = "name", .private = OWNER_NAMES },
	{ .name = "base", .private = OWNER_BASE },
	{ .name = "user", .private = OWNER_USER },
	{ .name = "group", .private = OWNER_GROUP },
	{ .name = NULL }
};

static int owner_setup(element_s * const element)
{
	struct owner_data *data;

	if ((data = xmalloc(sizeof(struct owner_data))) == NULL)
		return 1;

	data->recursive = 0;
	data->base = NULL;
	data->name_count = 0;
	data->names = NULL;
	data->user = NULL;
	data->group = NULL;
	element->handler_data = data;

	return 0;
};

static void owner_free(const element_s * const element)
{
	struct owner_data *data = (struct owner_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->user);
	xfree(data->group);
	if (data->name_count > 0) {
		for (i = 0; i < data->name_count; i++)
			xfree(data->names[i]);
		xfree(data->names);
	}
	xfree(data);
}

static int owner_parameter(const element_s * const element,
			   const struct handler_parameter * const param,
			   const char * const value)
{
	struct owner_data *data = (struct owner_data *) element->handler_data;
	char *tmp;
	char *tok;

	switch (param->private) {
	case OWNER_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case OWNER_USER:
		if (data->user) {
			Nprint_err("<%s>: cannot specify <user> more than once.", element->handler->name);
			return 1;
		}
		data->user = xstrdup(value);
		return 0;
	case OWNER_GROUP:
		if (data->group) {
			Nprint_err("<%s>: cannot specify <group> more than once.", element->handler->name);
			return 1;
		}
		data->group = xstrdup(value);
		return 0;
	case OWNER_OPTIONS:
		if (!strcmp("recursive", value)) {
			data->recursive = 1;
			return 0;
		}
		Nprint_err("<%s>: invalid option (%s) ignored", element->handler->name, value);
		return 1;
	case OWNER_NAMES:
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

static int owner_valid_data(const element_s * const element)
{
	struct owner_data *data = (struct owner_data *) element->handler_data;

	if (data->name_count == 0) {
		Nprint_err("<%s>: <name> must be specified.", element->handler->name);
		return 0;
	}

	if (!(data->user || data->group)) {
		Nprint_err("<%s>: <user> or <group> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static int owner_main(const element_s * const element)
{
	struct owner_data *data = (struct owner_data *) element->handler_data;
	int status = 0;
	int i;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	if (data->user) {
		for (i = 0; i < data->name_count; i++) {
			Nprint_h("Changing user ownership to %s%s: %s", data->user,
				 data->recursive ? "(recursive)" : " ", data->names[i]);
			status = execute_command(element, "chown %s %s %s",
						 data->recursive ? "-R" : "",
						 data->user, data->names[i]);
			if (status) {
				Nprint_h_err("Changing ownership failed.");
				break;
			}
		}
	}

	if ((status == 0) && data->group) {
		for (i = 0; i < data->name_count; i++) {
			Nprint_h("Changing group ownership to %s%s: %s", data->group,
				 data->recursive ? "(recursive)" : " ", data->names[i]);
			status = execute_command(element, "chgrp %s %s %s",
						 data->recursive ? "-R" : "",
						 data->group, data->names[i]);
			if (status) {
				Nprint_h_err("Changing ownership failed.");
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
		.name = "owner",
		.description = "Change ownership",
		.syntax_version = "2.0",
		.parameters = owner_parameters,
		.main = owner_main,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0,
		.setup = owner_setup,
		.free = owner_free,
		.parameter = owner_parameter,
		.valid_data = owner_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
