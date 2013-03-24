/*
 *  textdump.c - Handler.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME textdump
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"

enum {
	TEXTDUMP_BASE,
	TEXTDUMP_FILE,
	TEXTDUMP_CONTENT,
	TEXTDUMP_MODE,
};

struct textdump_data {
	char *base;
	char *file;
	char *content;
	int append_mode;
};

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter textdump_parameters_v2[] = {
	{ .name = "base", .private = TEXTDUMP_BASE },
	{ .name = "file", .private = TEXTDUMP_FILE },
	{ .name = "content", .private = TEXTDUMP_CONTENT },
	{ .name = NULL }
};

static const struct handler_attribute textdump_attributes_v2[] = {
	{ .name = "mode", .private = TEXTDUMP_MODE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter textdump_parameters_v3[] = {
	{ .name = "file", .private = TEXTDUMP_FILE },
	{ .name = "content", .private = TEXTDUMP_CONTENT },
	{ .name = NULL }
};

static const struct handler_attribute textdump_attributes_v3[] = {
	{ .name = "base", .private = TEXTDUMP_BASE },
	{ .name = "mode", .private = TEXTDUMP_MODE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

static int textdump_setup(element_s * const element)
{
	struct textdump_data *data;

	if ((data = xmalloc(sizeof(struct textdump_data))) == NULL)
		return -1;

	data->base = NULL;
	data->file = NULL;
	data->content = NULL;
	data->append_mode = 0;
	element->handler_data = data;

	return 0;
}

static void textdump_free(const element_s * const element)
{
	struct textdump_data *data = (struct textdump_data *) element->handler_data;

	xfree(data->base);
	xfree(data->file);
	xfree(data->content);
	xfree(data);
}

static int textdump_attribute(const element_s * const element,
			      const struct handler_attribute * const attr,
			      const char * const value)
{
	struct textdump_data *data = (struct textdump_data *) element->handler_data;

	switch (attr->private) {
	case TEXTDUMP_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case TEXTDUMP_MODE:
		if (strcmp(value, "append")) {
			Nprint_err("<%s>: the only \"mode\" allowed is \"append\" (%s).", element->handler->name, value);
			return 1;
		}
		data->append_mode = 1;
		return 0;
	default:
		return 1;
	}
}

static int textdump_parameter(const element_s * const element,
			    const struct handler_parameter * const param,
			    const char * const value)
{
	struct textdump_data *data = (struct textdump_data *) element->handler_data;

	switch (param->private) {
	case TEXTDUMP_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case TEXTDUMP_FILE:
		if (data->file) {
			Nprint_err("<%s>: cannot specify <file> more than once.", element->handler->name);
			return 1;
		}
		data->file = xstrdup(value);
		return 0;
	case TEXTDUMP_CONTENT:
		if (data->content) {
			Nprint_err("<%s>: cannot specify <content> more than once.", element->handler->name);
			return 1;
		}
		data->content = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int textdump_valid_data(const element_s * const element)
{
	struct textdump_data *data = (struct textdump_data *) element->handler_data;

	if (!data->file) {
		Nprint_err("<%s>: <file> must be specified.", element->handler->name);
		return 0;
	}

	if (!data->content) {
		Nprint_err("<%s>: <content> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static int textdump_main(const element_s * const element)
{
	struct textdump_data *data = (struct textdump_data *) element->handler_data;
	char *tok;
	FILE *fp;
	char *tmp;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	if (data->append_mode) {
		Nprint_h("Appending text to \"%s\".", data->file);
		fp = fopen(data->file, "a");
	} else {
		Nprint_h("Creating new file: %s", data->file);
		fp = fopen(data->file, "w");
	}

	if (fp == NULL) {
		Nprint_h_err("Unable to open \"%s\":", data->file);
		Nprint_h_err("    %s", strerror(errno));
		return -1;
	}

	tmp = xstrdup(data->content);
	for (tok = strtok(tmp, "\n"); tok; tok = strtok(NULL, "\n")) {
		fprintf(fp, "%s\n", ++tok);
	}
	xfree(tmp);

	fclose(fp);
	
	return 0;
}

static char *textdump_data(const element_s * const element,
			   const handler_data_e data_requested)
{
	struct textdump_data *data = (struct textdump_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_DISPLAY_DETAILS:
	{
		char *display = NULL;

		if (data->base)
			append_str_format(&display, "Base directory: %s\n", data->base);
		append_str_format(&display, "File: %s (%s)\n", data->file,
				  (data->append_mode) ? "append" : "overwrite");

		append_str_format(&display, "Content:\n%s\n", data->content);

		return display;
	}
	case HDATA_DISPLAY_NAME:
	{
		char *display = NULL;

		append_str_format(&display, "Dump text (%s)", data->file);

		return display;
	}
	default:
		break;
	}

	return NULL;
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "2.0",
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME | HDATA_DISPLAY_DETAILS,
		.alloc_data = textdump_data,
		.is_action = 1,
		.main = textdump_main,
		.parameters = textdump_parameters_v2,
		.attributes = textdump_attributes_v2,
		.setup = textdump_setup,
		.free = textdump_free,
		.attribute = textdump_attribute,
		.parameter = textdump_parameter,
		.valid_data = textdump_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME | HDATA_DISPLAY_DETAILS,
		.alloc_data = textdump_data,
		.is_action = 1,
		.main = textdump_main,
		.parameters = textdump_parameters_v3,
		.attributes = textdump_attributes_v3,
		.setup = textdump_setup,
		.free = textdump_free,
		.attribute = textdump_attribute,
		.parameter = textdump_parameter,
		.valid_data = textdump_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME | HDATA_DISPLAY_DETAILS,
		.alloc_data = textdump_data,
		.is_action = 1,
		.main = textdump_main,
		.parameters = textdump_parameters_v3,
		.attributes = textdump_attributes_v3,
		.setup = textdump_setup,
		.free = textdump_free,
		.attribute = textdump_attribute,
		.parameter = textdump_parameter,
		.valid_data = textdump_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.data = HDATA_DISPLAY_NAME | HDATA_DISPLAY_DETAILS,
		.alloc_data = textdump_data,
		.is_action = 1,
		.alternate_shell = 1,
		.main = textdump_main,
		.parameters = textdump_parameters_v3,
		.attributes = textdump_attributes_v3,
		.setup = textdump_setup,
		.free = textdump_free,
		.attribute = textdump_attribute,
		.parameter = textdump_parameter,
		.valid_data = textdump_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
