/*
 *  stamp.c - Handler.
 * 
 *  Copyright (C) 2001, 2002, 2004
 *
 *  Vassili Dzuba <vassilidzuba@nerim.net>
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

#define MODULE_NAME stamp
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"

enum {
	STAMP_NAME,
	STAMP_VERSION,
};

struct stamp_data {
	char *name;
	char *version;
};

static const struct handler_attribute stamp_attributes[] = {
	{ .name = "name", .private = STAMP_NAME },
	{ .name = "version", .private = STAMP_VERSION },
	{ .name = NULL }
};

static int stamp_setup(element_s * const element)
{
	struct stamp_data *data;

	if ((data = xmalloc(sizeof(struct stamp_data))) == NULL)
		return 1;

	data->name = NULL;
	data->version = NULL;
	element->handler_data = data;

	return 0;
}

static void stamp_free(const element_s * const element)
{
	struct stamp_data *data = (struct stamp_data *) element->handler_data;

	xfree(data->name);
	xfree(data->version);
	xfree(data);
}

static int stamp_attribute(const element_s * const element,
			   const struct handler_attribute * const attr,
			   const char * const value)
{
	struct stamp_data *data = (struct stamp_data *) element->handler_data;

	switch (attr->private) {
	case STAMP_NAME:
		if (data->name) {
			Nprint_err("<%s>: cannot specify \"name\" more than once.", element->handler->name);
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	case STAMP_VERSION:
		if (data->version) {
			Nprint_err("<%s>: cannot specify \"version\" more than once.", element->handler->name);
			return 1;
		}
		data->version = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int stamp_valid_data(const element_s * const element)
{
	struct stamp_data *data = (struct stamp_data *) element->handler_data;

	if (!data->name) {
		Nprint_err("<%s>: \"name\" cannot be empty.", element->handler->name);
		return 0;
	}

	if (!data->version) {
		Nprint_err("<%s>: \"version\" cannot be empty.", element->handler->name);
		return 0;
	}

	return 1;
}

static int stamp_main(const element_s * const element)
{
	struct stamp_data *data = (struct stamp_data *) element->handler_data;

	return stamp_package_installed(1, data->name, data->version);
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "stamp",
		.description = "Produce a stamp",
		.syntax_version = "2.0",
		.main = stamp_main,
		.type = HTYPE_NORMAL,
		.setup = stamp_setup,
		.free = stamp_free,
		.attributes = stamp_attributes,
		.attribute = stamp_attribute,
		.valid_data = stamp_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
