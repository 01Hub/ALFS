/*
 *  alfs.c - Handler.
 *
 *  Copyright (C) 2001, 2002, 2004
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

#define MODULE_NAME alfs
#include <nALFS.h>

#include "handlers.h"
#include "backend.h"
#include "nprint.h"
#include "utility.h"


enum {
	ALFS_VERSION
};

static const struct handler_attribute alfs_attributes[] = {
	{ .name = "version", .private = ALFS_VERSION },
        { .name = NULL }
};

struct alfs_data {
	char *version;
};

static int alfs_setup(element_s * const element)
{
	struct alfs_data *data;

	if ((data = xmalloc(sizeof(struct alfs_data))) == NULL)
		return 1;

	data->version = NULL;
	element->handler_data = data;

	return 0;
};

static void alfs_free(const element_s * const element)
{
	struct alfs_data *data = (struct alfs_data *) element->handler_data;

	xfree(data->version);
	xfree(data);
}

static int alfs_attribute(const element_s * const element,
			  const struct handler_attribute * const attr,
			  const char * const value)
{
	struct alfs_data *data = (struct alfs_data *) element->handler_data;

	switch (attr->private) {
	case ALFS_VERSION:
		if (data->version) {
			Nprint_err("<alfs>: cannot specify \"version\" more than once.");
			return 1;
		}
		data->version = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int alfs_valid_child(const element_s * const element,
			    const element_s * const child)
{
	(void) element;

	return child->handler->type & (HTYPE_NORMAL |
				       HTYPE_COMMENT |
				       HTYPE_PACKAGE);
}

static char *alfs_data(const element_s * const element,
		       const handler_data_e data_requested)
{
	struct alfs_data *data = (struct alfs_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_SYNTAX_VERSION:
		if (data->version)
			return xstrdup(data->version);
		break;
	default:
		break;
	}

	return NULL;
}

static int alfs_main(const element_s * const el)
{
	int status;

	status = execute_children(el);

	return status;
}


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "2.0",
		.main = alfs_main,
		.type = HTYPE_NORMAL,
		.setup = alfs_setup,
		.free = alfs_free,
		.attributes = alfs_attributes,
		.attribute = alfs_attribute,
		.valid_child = alfs_valid_child,
		.data = HDATA_SYNTAX_VERSION,
		.alloc_data = alfs_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "3.0",
		.main = alfs_main,
		.type = HTYPE_NORMAL,
		.setup = alfs_setup,
		.free = alfs_free,
		.attributes = alfs_attributes,
		.attribute = alfs_attribute,
		.valid_child = alfs_valid_child,
		.data = HDATA_SYNTAX_VERSION,
		.alloc_data = alfs_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "3.1",
		.main = alfs_main,
		.type = HTYPE_NORMAL,
		.setup = alfs_setup,
		.free = alfs_free,
		.attributes = alfs_attributes,
		.attribute = alfs_attribute,
		.valid_child = alfs_valid_child,
		.data = HDATA_SYNTAX_VERSION,
		.alloc_data = alfs_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "3.2",
		.main = alfs_main,
		.type = HTYPE_NORMAL,
		.setup = alfs_setup,
		.free = alfs_free,
		.attributes = alfs_attributes,
		.attribute = alfs_attribute,
		.valid_child = alfs_valid_child,
		.data = HDATA_SYNTAX_VERSION,
		.alloc_data = alfs_data,
	},
#endif
	{
		.name = NULL
	}
};
