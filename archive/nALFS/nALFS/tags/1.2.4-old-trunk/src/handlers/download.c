/*
 *  download.c - Handler.
 * 
 *  Copyright (C) 2003, 2004
 *  
 *  Neven Has <haski@sezampro.yu>
 *  Vassili Dzuba <vdzuba@nerim.net>
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

#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME download
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "nalfs-core.h"
#include "backend.h"


enum {
	DOWNLOAD_FILE,
	DOWNLOAD_URL,
	DOWNLOAD_DESTINATION,
	DOWNLOAD_BASE,
};

struct download_data {
	char *file;
	char *destination;
	int url_count;
	char **urls;
	char *base;
	const element_s *digest;
};

static int download_setup(element_s * const element)
{
	struct download_data *data;

	if ((data = xmalloc(sizeof(struct download_data))) == NULL)
		return -1;

	data->file = NULL;
	data->destination = NULL;
	data->url_count = 0;
	data->urls = NULL;
	data->digest = NULL;
	data->base = NULL;
	element->handler_data = data;

	return 0;
}

static void download_free(const element_s * const element)
{
	struct download_data *data = (struct download_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->file);
	xfree(data->destination);
	if (data->url_count > 0) {
		for (i = 0; i < data->url_count; i++)
			xfree(data->urls[i]);
		xfree(data->urls);
	}
	xfree(data);
}

static int download_parameter(const element_s * const element,
			      const struct handler_parameter * const param,
			      const char * const value)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	switch (param->private) {
	case DOWNLOAD_FILE:
		if (data->file) {
			Nprint_err("<download>: cannot specify <file> more than once.");
			return 1;
		}
		data->file = xstrdup(value);
		return 0;
	case DOWNLOAD_URL:
		data->url_count++;
		if ((data->urls = xrealloc(data->urls,
					   sizeof(data->urls[0]) * (data->url_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->urls[(data->url_count - 1)] = xstrdup(value);
		return 0;
	case DOWNLOAD_DESTINATION:
		if (data->destination) {
			Nprint_err("<download>: cannot specify <destination> more than once.");
			return 1;
		}
		data->destination = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int download_valid_data(const element_s * const element)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	if (!data->file) {
		Nprint_err("<download>: \"file\" must be specified.");
		return 0;
	}

	if (!data->destination) {
		Nprint_err("<download>: \"destination\" must be specified.");
		return 0;
	}

	return 1;
}

static int download_valid_child(const element_s * const element,
				const element_s * const child)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	if (child->handler->type & HTYPE_DIGEST) {
		if (data->digest) {
			Nprint_err("<download>: only one <digest> allowed.");
			return 0;
		}

		data->digest = child;
		return 1;
	}

	return 0;
}

static int download_common(const element_s * const element)
{
	struct download_data *data = (struct download_data *) element->handler_data;
	int status = -1;
	struct stat file_stat;
	char *digest = NULL;
	char *digest_type = NULL;

	if (data->digest) {
		digest = data->digest->handler->alloc_data(data->digest, HDATA_COMMAND);
		digest_type = data->digest->handler->alloc_data(data->digest, HDATA_VERSION);
	}

	/* Check if file exists. */
	if ((stat(data->file, &file_stat))) {
	        if ((errno == ENOENT) && (data->url_count > 0)) {
			int i, found = 0;

			Nprint_h_warn("File %s not found.", data->file);
			Nprint_h("Trying to fetch it from <url>...");

			for (i = 0; i < data->url_count; i++) {
				char *tmp = NULL;

				append_str_format(&tmp, "%s%s",
						  data->urls[i],
						  data->file);
				if (!get_url(tmp, data->file, digest, digest_type))
					found = 1;
				xfree(tmp);
				if (found)
					break;
			}

			if (i < data->url_count) {
				status = 0;
			} else {
				Nprint_h_err("Unable to download file %s.", data->file);
			}
		} else {
			Nprint_h_err("Checking for %s failed:", data->file);
			Nprint_h_err("    %s", strerror(errno));
		}
	} else if (digest && verify_digest(digest_type, digest, data->file)) {
		Nprint_h_err("Wrong %s digest of file: %s", digest_type, data->file);
	} else {
		status = 0;
	}

	xfree(digest_type);
	xfree(digest);
	return status;
}

#if HANDLER_SYNTAX_3_1 

static const struct handler_parameter download_parameters[] = {
	{ .name = "file", .private = DOWNLOAD_FILE },
	{ .name = "url", .private = DOWNLOAD_URL },
	{ .name = "destination", .private = DOWNLOAD_DESTINATION },
	{ .name = NULL }
};

static int download_main_v2(const element_s * const element)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	if (change_current_dir(data->destination))
		return -1;

	return download_common(element);
}

#endif /* HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_3_2

static const struct handler_parameter download_parameters_v3_2[] = {
	{ .name = "file", .private = DOWNLOAD_FILE },
	{ .name = "url", .private = DOWNLOAD_URL },
	{ .name = NULL }
};

static const struct handler_attribute download_attributes_v3_2[] = {
	{ .name = "base", .private = DOWNLOAD_BASE },
	{ .name = NULL }
};

static int download_attribute(const element_s * const element,
			      const struct handler_attribute * const attr,
			      const char * const value)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	switch (attr->private) {
	case DOWNLOAD_BASE:
		if (data->base) {
			Nprint_err("<download>: cannot specify \"base\" more than once.");
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int download_valid_data_v3_2(const element_s * const element)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	if (!data->file) {
		Nprint_err("<download>: \"file\" must be specified.");
		return 0;
	}

	if (!data->base) {
		const char *base = alloc_base_dir(element);

		if (!base) {
			Nprint_err("<download>: \"base\" must be specified at or above this element.");
			return 0;
		} else {
			xfree(base);
		}
	}

	return 1;
}

static int download_main_v3_2(const element_s * const element)
{
	struct download_data *data = (struct download_data *) element->handler_data;

	if (change_to_base_dir(element, data->base, 0))
		return -1;

	return download_common(element);
}

#endif /* HANDLER_SYNTAX_3_2 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_1
	{
		.name = "download",
		.description = "Download",
		.syntax_version = "3.1",
		.parameters = download_parameters,
		.main = download_main_v2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = download_setup,
		.free = download_free,
		.parameter = download_parameter,
		.valid_child = download_valid_child,
		.valid_data = download_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "download",
		.description = "Download",
		.syntax_version = "3.2",
		.parameters = download_parameters_v3_2,
		.attributes = download_attributes_v3_2,
		.main = download_main_v3_2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = download_setup,
		.free = download_free,
		.parameter = download_parameter,
		.valid_child = download_valid_child,
		.valid_data = download_valid_data_v3_2,
		.attribute = download_attribute,
	},
#endif
	{
		.name = NULL
	}
};
