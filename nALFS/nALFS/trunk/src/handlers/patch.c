/*
 *  patch.c - Handler.
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

#define MODULE_NAME patch
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


enum {
	PATCH_BASE,
	PATCH_PREFIX,
	PATCH_PARAM,
	PATCH_REFERENCE,
	PATCH_FILE,
	PATCH_MODE,
	PATCH_PATH_STRIP,
};

struct patch_data {
	char *base;
	char *prefix;
	char *param;
	int param_seen;
	int reference_count;
	char **references;
	char *file;
	const element_s *digest;
	int forward_mode;
	int path_strip;
};

static int patch_setup(element_s * const element)
{
	struct patch_data *data;

	if ((data = xmalloc(sizeof(struct patch_data))) == NULL)
		return -1;

	data->file = NULL;
	data->param = xstrdup(" ");
	data->param_seen = 0;
	data->prefix = xstrdup("");
	data->reference_count = 0;
	data->references = NULL;
	data->digest = NULL;
	data->base = NULL;
	data->path_strip = 1;
	data->forward_mode = 1;
	element->handler_data = data;

	return 0;
}

static void patch_free(const element_s * const element)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->file);
	xfree(data->prefix);
	xfree(data->param);
	if (data->reference_count > 0) {
		for (i = 0; i < data->reference_count; i++)
			xfree(data->references[i]);
		xfree(data->references);
	}
	xfree(data);
}

static int patch_attribute(const element_s * const element,
			   const struct handler_attribute * const attr,
			   const char * const value)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;

	switch (attr->private) {
	case PATCH_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case PATCH_MODE:
		if (!strcmp(value, "forward")) {
			data->forward_mode = 1;
			return 0;
		} else if (!strcmp(value, "reverse")) {
			data->forward_mode = 0;
			return 0;
		} else {
			Nprint_err("<%s>: \"mode\" must be \"forward\" or \"reverse\".", element->handler->name);
			return 1;
		}
	case PATCH_PATH_STRIP:
		data->path_strip = atoi(value);
		return 0;
	default:
		return 1;
	}
}

static int patch_parameter(const element_s * const element,
			   const struct handler_parameter * const param,
			   const char * const value)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;

	switch (param->private) {
	case PATCH_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case PATCH_PREFIX:
		append_str_format(&data->prefix, "%s ", value);
		return 0;
	case PATCH_PARAM:
		append_str_format(&data->param, "%s ", value);
		data->param_seen = 1;
		return 0;
	case PATCH_FILE:
		if (data->file) {
			Nprint_err("<%s>: cannot specify <file> more than once.", element->handler->name);
			return 1;
		}
		data->file = xstrdup(value);
		return 0;
	case PATCH_REFERENCE:
		data->reference_count++;
		if ((data->references = xrealloc(data->references,
						 sizeof(data->references[0]) * (data->reference_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->references[(data->reference_count - 1)] = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int patch_valid_data(const element_s * const element)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;

	if (!data->param_seen) {
		Nprint_err("<%s>: at least one <param> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter patch_parameters_v2[] = {
	{ .name = "base", .private = PATCH_BASE },
	{ .name = "param", .private = PATCH_PARAM },
	{ .name = NULL }
};

static int patch_main_ver2(const element_s * const element)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;
	int status;

	if (change_to_base_dir(element, data->base, 1))
		return -1;
	
	Nprint_h("Patching:");
	Nprint_h("    patch%s", data->param);

	if ((status = execute_command(element, "patch%s", data->param)))
		Nprint_h_err("Patching failed.");

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const struct handler_parameter patch_parameters_v3[] = {
	{ .name = "prefix", .private = PATCH_PREFIX },
	{ .name = "param", .private = PATCH_PARAM },
	{ .name = NULL }
};

static const struct handler_attribute patch_attributes_v3[] = {
	{ .name = "base", .private = PATCH_BASE },
	{ .name = NULL }
};

static int patch_main_ver3(const element_s * const element)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;
	int status;
	char *command = NULL;

	if (change_to_base_dir(element, data->base, 1))
		return -1;
	
	append_str_format(&command, "%s patch %s", data->prefix, data->param);

	Nprint_h("Patching");
	Nprint_h("    %s", command);

	if ((status = execute_command(element, command)))
		Nprint_h_err("Patching failed.");
	
	xfree(command);

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_3_2

static const struct handler_parameter patch_parameters_v3_2[] = {
	{ .name = "prefix", .private = PATCH_PREFIX },
	{ .name = "param", .private = PATCH_PARAM },
	{ .name = "reference", .private = PATCH_REFERENCE },
	{ .name = "file", .private = PATCH_FILE },
	{ .name = NULL }
};

static const struct handler_attribute patch_attributes_v3_2[] = {
	{ .name = "base", .private = PATCH_BASE },
	{ .name = "mode", .private = PATCH_MODE },
	{ .name = "path_strip", .private = PATCH_PATH_STRIP },
	{ .name = NULL }
};

static int patch_valid_data_v3_2(const element_s * const element)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;

	if (!data->file) {
		Nprint_err("<%s>: <file> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static int patch_valid_child_v3_2(const element_s * const element,
				  const element_s * const child)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;

	if (child->handler->type & HTYPE_DIGEST) {
		if (data->digest) {
			Nprint_err("<%s>: only one <digest> allowed.", element->handler->name);
			return 0;
		}

		data->digest = child;
		return 1;
	}

	return 0;
}

static int patch_main_ver3_2(const element_s * const element)
{
	struct patch_data *data = (struct patch_data *) element->handler_data;
        int status = -1;
	char *digest = NULL;
	char *digest_type = NULL;
	char *decompressor = NULL;
	struct stat file_stat;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	if (data->digest) {
		digest = data->digest->handler->alloc_data(data->digest, HDATA_COMMAND);
		digest_type = data->digest->handler->alloc_data(data->digest, HDATA_VERSION);
	}

	/* Check if file exists. */
	if ((stat(data->file, &file_stat))) {
	        if ((errno == ENOENT) && (data->reference_count > 0)) {
			int i;

			Nprint_h_warn("File %s not found.", data->file);
			Nprint_h("Trying to fetch it from <reference>...");

			for (i = 0; i < data->reference_count; i++) {
				if (!get_url(data->references[i],
					     data->file,
					     digest,
					     digest_type))
					break;
			}

			if (i < data->reference_count) {
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

	if (status == 0) {
		decompressor = alloc_decompress_command(get_compression_type(data->file));
		if (decompressor == NULL)
			decompressor = xstrdup("cat %s");
		append_str(&decompressor, " | %spatch -p%d %s%s");

		Nprint_h("    %spatch -p%d %s%s", data->prefix, data->path_strip,
			 data->forward_mode ? "-N" : "-R", data->param);

		if ((status = execute_command(element, decompressor, data->file,
					      data->prefix, data->path_strip,
					      data->forward_mode ? "-N" : "-R", data->param)))
			Nprint_h_err("Patching failed.");

		xfree(decompressor);
	}
	
	xfree(digest_type);
	xfree(digest);

	return status;
}

#endif /* HANDLER_SYNTAX_3_2 */

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "2.0",
		.parameters = patch_parameters_v2,
		.main = patch_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = patch_setup,
		.free = patch_free,
		.valid_data = patch_valid_data,
		.parameter = patch_parameter,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.0",
		.parameters = patch_parameters_v3,
		.attributes = patch_attributes_v3,
		.main = patch_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = patch_setup,
		.free = patch_free,
		.parameter = patch_parameter,
		.valid_data = patch_valid_data,
		.attribute = patch_attribute,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.1",
		.parameters = patch_parameters_v3,
		.attributes = patch_attributes_v3,
		.main = patch_main_ver3,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
		.setup = patch_setup,
		.free = patch_free,
		.valid_data = patch_valid_data,
		.parameter = patch_parameter,
		.attribute = patch_attribute,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.2",
		.parameters = patch_parameters_v3_2,
		.attributes = patch_attributes_v3_2,
		.main = patch_main_ver3_2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = patch_setup,
		.free = patch_free,
		.valid_data = patch_valid_data_v3_2,
		.parameter = patch_parameter,
		.attribute = patch_attribute,
		.valid_child = patch_valid_child_v3_2,
	},
#endif
	{
		.name = NULL
	}
};
