/*
 *  unpack.c - Handler.
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

#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME unpack
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"

static INLINE char *alloc_basename(const char *archive)
{
	char *base_name;
	char *tmp = strrchr(archive, '/');


	if (tmp == NULL) {
		base_name = xstrdup(archive);
	} else {
		base_name = xstrdup(tmp + 1);
	}

	if ((tmp = strrchr(base_name, '.'))) {
		*tmp = '\0';
		tmp++;
	}

	return base_name;
}

static int unpack_archive(const element_s * const element, const char * const archive)
{
	char *decompressor = NULL;
	char *unpacker = NULL;
	char *command = NULL;
	int status = -1;

	decompressor = alloc_decompress_command(get_compression_type(archive));
	unpacker = alloc_unpack_command(get_archive_format(archive));

	if (unpacker != NULL) {
		if (decompressor != NULL) {
			command = xstrdup(decompressor);
			append_str(&command, " | ");
		} else {
			command = xstrdup("cat %s | ");
		}
		append_str(&command, unpacker);
		Nprint_h("Unpacking %s...", archive);
		status = execute_command(element, command, archive);
	} else if (decompressor != NULL) {
		/* Allocate basename (without the last extension). */
		char *base_name = alloc_basename(archive);

		Nprint_h("Decompressing %s...", archive);
		command = xstrdup(decompressor);
		append_str(&command, " > %s");
		status = execute_command(element, command, archive, base_name);
		xfree(base_name);
	} else {
		Nprint_h_err("Unknown archive (%s).", archive);
	}

	xfree(command);
	xfree(decompressor);
	xfree(unpacker);
	return status;
}

enum {
	UNPACK_ARCHIVE,
	UNPACK_DESTINATION,
	UNPACK_REFERENCE,
	UNPACK_DIGEST,
	UNPACK_BASE,
};

struct unpack_data {
	char *archive;
	char *destination;
	int reference_count;
	char **references;
	char *base;
	const element_s *digest;
};

static int unpack_setup(element_s * const element)
{
	struct unpack_data *data;

	if ((data = xmalloc(sizeof(struct unpack_data))) == NULL)
		return -1;

	data->archive = NULL;
	data->destination = NULL;
	data->reference_count = 0;
	data->references = NULL;
	data->digest = NULL;
	data->base = NULL;
	element->handler_data = data;

	return 0;
}

static void unpack_free(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->archive);
	xfree(data->destination);
	if (data->reference_count > 0) {
		for (i = 0; i < data->reference_count; i++)
			xfree(data->references[i]);
		xfree(data->references);
	}
	xfree(data);
}

static int unpack_attribute(const element_s * const element,
			    const struct handler_attribute * const attr,
			    const char * const value)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	switch (attr->private) {
	case UNPACK_BASE:
		if (data->base) {
			Nprint_err("<unpack>: cannot specify \"base\" more than once.");
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int unpack_parameter(const element_s * const element,
			    const struct handler_parameter * const param,
			    const char * const value)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	switch (param->private) {
	case UNPACK_ARCHIVE:
		if (data->archive) {
			Nprint_err("<unpack>: cannot specify <archive> more than once.");
			return 1;
		}
		data->archive = xstrdup(value);
		return 0;
	case UNPACK_REFERENCE:
		data->reference_count++;
		if ((data->references = xrealloc(data->references,
						 sizeof(data->references[0]) * (data->reference_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->references[(data->reference_count - 1)] = xstrdup(value);
		return 0;
	case UNPACK_DESTINATION:
		if (data->destination) {
			Nprint_err("<unpack>: cannot specify <destination> more than once.");
			return 1;
		}
		data->destination = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int unpack_valid_data(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	if (!data->archive) {
		Nprint_err("<unpack>: \"archive\" must be specified.");
		return 0;
	}

	if (!data->destination) {
		Nprint_err("<unpack>: \"destination\" must be specified.");
		return 0;
	}

	return 1;
}

static int unpack_valid_child(const element_s * const element,
			      const element_s * const child)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	if (child->handler->type & HTYPE_DIGEST) {
		if (data->digest) {
			Nprint_err("<unpack>: only one <digest> allowed.");
			return 0;
		}

		data->digest = child;
		return 1;
	}

	return 0;
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter unpack_parameters_v2[] = {
	{ .name = "archive", .private = UNPACK_ARCHIVE },
	{ .name = "destination", .private = UNPACK_DESTINATION },
	{ .name = NULL }
};

static int unpack_main_ver2(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;
	int status;
	struct stat file_stat;

	if (change_current_dir(data->destination))
		return -1;

	/* Check if archive exists. */
	if (stat(data->archive, &file_stat)) {
		Nprint_h_err("Checking for %s failed:", data->archive);
		Nprint_h_err("    %s", strerror(errno));
		return -1;
	}

	status = unpack_archive(element, data->archive);

	if (status) {
		Nprint_h_err("Unpacking %s failed.", data->archive);
	} else {
		Nprint_h("Done unpacking %s.", data->archive);
	}

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANLER_SYNTAX_3_2

static int unpack_common_v3(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;
	int status = -1;
	struct stat file_stat;
	char *digest = NULL;
	char *digest_type = NULL;

	if (data->digest) {
		digest = data->digest->handler->alloc_data(data->digest, HDATA_COMMAND);
		digest_type = data->digest->handler->alloc_data(data->digest, HDATA_VERSION);
	}

	/* Check if archive exists. */
	if ((stat(data->archive, &file_stat))) {
	        if ((errno == ENOENT) && (data->reference_count > 0)) {
			int i;

			Nprint_h_warn("Archive %s not found.", data->archive);
			Nprint_h("Trying to fetch it from <reference>...");

			for (i = 0; i < data->reference_count; i++) {
				if (!get_url(data->references[i],
					     data->archive,
					     digest,
					     digest_type))
					break;
			}

			if (i < data->reference_count) {
				status = 0;
			} else {
				Nprint_h_err("Unable to download file %s.", data->archive);
			}
		} else {
			Nprint_h_err("Checking for %s failed:", data->archive);
			Nprint_h_err("    %s", strerror(errno));
		}
	} else if (digest && verify_digest(digest_type, digest, data->archive)) {
		Nprint_h_err("Wrong %s digest of archive: %s", digest_type, data->archive);
	} else {
		status = 0;
	}

	if (status == 0) {
		status = unpack_archive(element, data->archive);
		if (status) {
			Nprint_h_err("Unpacking %s failed.", data->archive);
		} else {
			Nprint_h("Done unpacking %s.", data->archive);
		}
	}

	xfree(digest_type);
	xfree(digest);
	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANLER_SYNTAX_3_2 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const struct handler_parameter unpack_parameters_v3[] = {
	{ .name = "archive", .private = UNPACK_ARCHIVE },
	{ .name = "destination", .private = UNPACK_DESTINATION },
	{ .name = "reference", .private = UNPACK_REFERENCE },
	{ .name = NULL }
};

static int unpack_main_ver3(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	if (change_current_dir(data->destination))
		return -1;

	return unpack_common_v3(element);
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_3_2

static const struct handler_parameter unpack_parameters_v3_2[] = {
	{ .name = "archive", .private = UNPACK_ARCHIVE },
	{ .name = "reference", .private = UNPACK_REFERENCE },
	{ .name = NULL }
};

static const struct handler_attribute unpack_attributes_v3_2[] = {
	{ .name = "base", .private = UNPACK_BASE },
	{ .name = NULL }
};

static int unpack_valid_data_v3_2(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	if (!data->archive) {
		Nprint_err("<unpack>: \"archive\" must be specified.");
		return 0;
	}

	if (!data->base) {
		const char *base = alloc_base_dir(element);

		if (!base) {
			Nprint_err("<unpack>: \"base\" must be specified at or above this element.");
			return 0;
		} else {
			xfree(base);
		}
	}

	return 1;
}

static int unpack_main_ver3_2(const element_s * const element)
{
	struct unpack_data *data = (struct unpack_data *) element->handler_data;

	if (change_to_base_dir(element, data->base, 0))
		return -1;

	return unpack_common_v3(element);
}

#endif /* HANDLER_SYNTAX_3_2 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "2.0",
		.parameters = unpack_parameters_v2,
		.main = unpack_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = unpack_setup,
		.free = unpack_free,
		.parameter = unpack_parameter,
		.valid_data = unpack_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "3.0",
		.parameters = unpack_parameters_v3,
		.main = unpack_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = unpack_setup,
		.free = unpack_free,
		.parameter = unpack_parameter,
		.valid_data = unpack_valid_data,
		.valid_child = unpack_valid_child,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "3.1",
		.parameters = unpack_parameters_v3,
		.main = unpack_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = unpack_setup,
		.free = unpack_free,
		.parameter = unpack_parameter,
		.valid_data = unpack_valid_data,
		.valid_child = unpack_valid_child,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "3.2",
		.parameters = unpack_parameters_v3_2,
		.attributes = unpack_attributes_v3_2,
		.main = unpack_main_ver3_2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = unpack_setup,
		.free = unpack_free,
		.parameter = unpack_parameter,
		.attribute = unpack_attribute,
		.alternate_shell = 1,
		.valid_data = unpack_valid_data_v3_2,
		.valid_child = unpack_valid_child,
	},
#endif
	{
		.name = NULL
	}
};
