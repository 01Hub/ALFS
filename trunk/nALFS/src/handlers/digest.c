/*
 *  digest.c - Handler.
 *
 *  Copyright (C) 2004
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

#define MODULE_NAME digest
#include <nALFS.h>

#include "handlers.h"
#include "backend.h"
#include "nprint.h"
#include "utility.h"


enum {
	DIGEST_TYPE
};

static const struct handler_attribute digest_attributes[] = {
	{ .name = "type", .private = DIGEST_TYPE },
        { .name = NULL }
};

struct digest_data {
	char *type;
	char *content;
};

static int digest_setup(element_s * const element)
{
	struct digest_data *data;

	if ((data = xmalloc(sizeof(struct digest_data))) == NULL)
		return 1;

	data->type = NULL;
	data->content = NULL;
	element->handler_data = data;

	return 0;
};

static void digest_free(const element_s * const element)
{
	struct digest_data *data = (struct digest_data *) element->handler_data;

	xfree(data->content);
	xfree(data->type);
	xfree(data);
}

static int digest_attribute(const element_s * const element,
			    const struct handler_attribute * const attr,
			    const char * const value)
{
	struct digest_data *data = (struct digest_data *) element->handler_data;
	char *s;

	switch (attr->private) {
	case DIGEST_TYPE:
		if (data->type) {
			Nprint_err("<digest>: cannot specify \"type\" more than once.");
			return 1;
		}
		data->type = xstrdup(value);
	        for (s = data->type; *s; s++)
			*s = tolower(*s);
		return 0;
	default:
		return 1;
	}
}

static int digest_content(const element_s * const element,
			  const char * const content)
{
	struct digest_data *data = (struct digest_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

static int digest_valid_data(const element_s * const element)
{
	struct digest_data *data = (struct digest_data *) element->handler_data;

	if (!data->content) {
		Nprint_err("<digest>: content cannot be empty.");
		return 0;
	}

	return 1;
}

static char *digest_data(const element_s * const element,
			 const handler_data_e data_requested)
{
	struct digest_data *data = (struct digest_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_COMMAND:
		/* We're cheating here, and returning the digest as an HDATA_COMMNAND */
		return xstrdup(data->content);
		break;
	case HDATA_VERSION:
		/* We're cheating here, and returning the digest type as an HDATA_VERSION */
		if (data->type) {
			return xstrdup(data->type);
		} else {
			return xstrdup("md5");
		}
		break;
	default:
		break;
	}

	return NULL;
}

static int digest_main(const element_s * const element)
{
	(void) element;
	/* should never be called */
	return 1;
}


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_0
	{
		.name = "digest",
		.description = "digest",
		.syntax_version = "3.0",
		.main = digest_main,
		.type = HTYPE_DIGEST,
		.setup = digest_setup,
		.free = digest_free,
		.attributes = digest_attributes,
		.attribute = digest_attribute,
		.content = digest_content,
		.data = HDATA_COMMAND | HDATA_VERSION,
		.alloc_data = digest_data,
		.valid_data = digest_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "digest",
		.description = "digest",
		.syntax_version = "3.1",
		.main = digest_main,
		.type = HTYPE_DIGEST,
		.setup = digest_setup,
		.free = digest_free,
		.attributes = digest_attributes,
		.attribute = digest_attribute,
		.content = digest_content,
		.data = HDATA_COMMAND | HDATA_VERSION,
		.alloc_data = digest_data,
		.valid_data = digest_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "digest",
		.description = "digest",
		.syntax_version = "3.2",
		.main = digest_main,
		.type = HTYPE_DIGEST,
		.setup = digest_setup,
		.free = digest_free,
		.attributes = digest_attributes,
		.attribute = digest_attribute,
		.content = digest_content,
		.data = HDATA_COMMAND | HDATA_VERSION,
		.alloc_data = digest_data,
		.valid_data = digest_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
