/*
 *  copy.c - Handler.
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

#define MODULE_NAME copy
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


enum {
	COPY_OPTIONS,
	COPY_OPTION,
	COPY_BASE,
	COPY_SOURCE,
	COPY_DESTINATION,
};

struct copy_data {
        int archive;
	int force;
	int recursive;
	int no_dereference;
	int preserve;
	char *base;
	int source_count;
	char **sources;
	char *destination;
};

static int copy_setup(element_s * const element)
{
	struct copy_data *data;

	if ((data = xmalloc(sizeof(struct copy_data))) == NULL)
		return 1;

	data->archive = 0;
	data->force = 0;
	data->recursive = 0;
	data->no_dereference = 0;
	data->preserve = 0;
	data->base = NULL;
	data->destination = NULL;
	data->source_count = 0;
	data->sources = NULL;
	element->handler_data = data;

	return 0;
};

static void copy_free(const element_s * const element)
{
	struct copy_data *data = (struct copy_data *) element->handler_data;
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

static int copy_attribute(const element_s * const element,
			  const struct handler_attribute * const attr,
			  const char * const value)
{
	struct copy_data *data = (struct copy_data *) element->handler_data;

	switch (attr->private) {
	case COPY_BASE:
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

static int copy_parameter(const element_s * const element,
			  const struct handler_parameter * const param,
			  const char * const value)
{
	struct copy_data *data = (struct copy_data *) element->handler_data;
	int option_found = 0;

	switch (param->private) {
	case COPY_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case COPY_OPTIONS:
		if (option_in_string("force", value)) {
			data->force = 1;
			option_found++;
		}
		if (option_in_string("archive", value)) {
			data->archive = 1;
			option_found++;
		}
		if (option_in_string("preserve", value)) {
			data->preserve = 1;
			option_found++;
		}
		if (option_in_string("recursive", value)) {
			data->recursive = 1;
			option_found++;
		}
		if (option_in_string("no-dereference", value)) {
			data->no_dereference = 1;
			option_found++;
		}
		if (!option_found) {
			Nprint_err("<%s>: invalid options in (%s) ignored", element->handler->name, value);
			return 1;
		} else {
			return 0;
		}
	case COPY_OPTION:
		if (!strcmp("force", value)) {
			data->force = 1;
			return 0;
		}
		if (!strcmp("archive", value)) {
			data->archive = 1;
			return 0;
		}
		if (!strcmp("preserve", value)) {
			data->preserve = 1;
			return 0;
		}
		if (!strcmp("recursive", value)) {
			data->recursive = 1;
			return 0;
		}
		if (!strcmp("no-dereference", value)) {
			data->no_dereference = 1;
			return 0;
		}
		Nprint_err("<%s>: invalid option (%s) ignored", element->handler->name, value);
		return 1;
	case COPY_SOURCE:
		data->source_count++;
		if ((data->sources = xrealloc(data->sources,
					      sizeof(data->sources[0]) * (data->source_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->sources[(data->source_count - 1)] = xstrdup(value);
		return 0;
	case COPY_DESTINATION:
		if (data->destination) {
			Nprint_err("<%s>: cannot specify <destination> more than once.", element->handler->name);
			return 1;
		}
		data->destination = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int copy_valid_data(const element_s * const element)
{
	struct copy_data *data = (struct copy_data *) element->handler_data;

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

static const struct handler_parameter copy_parameters_ver2[] = {
	{ .name = "options", .private = COPY_OPTIONS },
	{ .name = "base", .private = COPY_BASE },
	{ .name = "source", .private = COPY_SOURCE },
	{ .name = "destination", .private = COPY_DESTINATION },
	{ .name = NULL }
};

static int copy_main_ver2(const element_s * const el)
{
	struct copy_data *data = (struct copy_data *) el->handler_data;
	int status = 0;
	char *tok;
	char *tmp;
	char *common_command = NULL;

	if (change_to_base_dir(el, data->base, 1))
		return -1;

	append_str_format(&common_command, "cp %s%s%s%s%s",
			  data->archive ? " -a" : "",
			  data->force ? " -f" : "",
			  data->no_dereference ? " -d" : "",
			  data->preserve ? " -p" : "",
			  data->recursive ? " -R" : "");

	tmp = xstrdup(data->sources[0]);
	for (tok = strtok(tmp, WHITE_SPACE); tok; tok = strtok(NULL, WHITE_SPACE)) {
		char *command = NULL;

		append_str_format(&command, "%s %s %s",
				  common_command,
				  tok,
				  data->destination);

		Nprint_h("Executing:");
		Nprint_h("    %s", command);

		status = execute_command(el, "%s", command);

		xfree(command);

		if (status) {
			Nprint_h_err("Copying failed.");
			break;
		}
	}

	xfree(tmp);
	xfree(common_command);
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter copy_parameters_v3[] = {
	{ .name = "option", .private = COPY_OPTION },
	{ .name = "source", .private = COPY_SOURCE },
	{ .name = "destination", .private = COPY_DESTINATION },
	{ .name = NULL }
};

static const struct handler_attribute copy_attributes_v3[] = {
	{ .name = "base", .private = COPY_BASE },
	{ .name = NULL }
};

static int copy_main_ver3(const element_s * const el)
{
	struct copy_data *data = (struct copy_data *) el->handler_data;
	int status = 0;
	int i;
	char *common_command = NULL;

	if (change_to_base_dir(el, data->base, 1))
		return -1;

	append_str_format(&common_command, "cp %s%s%s%s%s",
			  data->archive ? " -a" : "",
			  data->force ? " -f" : "",
			  data->no_dereference ? " -d" : "",
			  data->preserve ? " -p" : "",
			  data->recursive ? " -R" : "");

	for (i = 0; i < data->source_count; i++) {
		char *command = NULL;

		append_str_format(&command, "%s %s %s",
				  common_command,
				  data->sources[i],
				  data->destination);

		Nprint_h("Executing:");
		Nprint_h("    %s", command);

		status = execute_command(el, "%s", command);

		xfree(command);

		if (status) {
			Nprint_h_err("Copying failed.");
			break;
		}
	}

	xfree(common_command);
	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "2.0",
		.main = copy_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.parameters = copy_parameters_ver2,
		.setup = copy_setup,
		.free = copy_free,
		.parameter = copy_parameter,
		.valid_data = copy_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "3.0",
		.parameters = copy_parameters_v3,
		.attributes = copy_attributes_v3,
		.main = copy_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = copy_setup,
		.free = copy_free,
		.parameter = copy_parameter,
		.attribute = copy_attribute,
		.valid_data = copy_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "3.1",
		.parameters = copy_parameters_v3,
		.attributes = copy_attributes_v3,
		.main = copy_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = copy_setup,
		.free = copy_free,
		.parameter = copy_parameter,
		.attribute = copy_attribute,
		.valid_data = copy_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "copy",
		.description = "Copy",
		.syntax_version = "3.2",
		.parameters = copy_parameters_v3,
		.attributes = copy_attributes_v3,
		.main = copy_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = copy_setup,
		.free = copy_free,
		.parameter = copy_parameter,
		.attribute = copy_attribute,
		.valid_data = copy_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
