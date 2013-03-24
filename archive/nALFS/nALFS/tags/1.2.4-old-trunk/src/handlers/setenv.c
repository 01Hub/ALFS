/*
 *  setenv.c - Handler.
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

#define MODULE_NAME setenv
#include <nALFS.h>

#include "handlers.h"
#include "nprint.h"
#include "parser.h"
#include "utility.h"

enum {
	SETENV_VALUE,
	SETENV_VARIABLE,
	SETENV_MODE,
};

struct setenv_data {
	char *variable;
	char *value;
	int append_mode;
};

static const struct handler_parameter setenv_parameters[] = {
	{ .name = "variable", .private = SETENV_VARIABLE },
	{ .name = "value", .private = SETENV_VALUE, .untrimmed = 1 },
	{ .name = NULL }
};

static const struct handler_attribute setenv_attributes[] = {
	{ .name = "mode", .private = SETENV_MODE },
	{ .name = NULL }
};

static int setenv_setup(element_s * const element)
{
	struct setenv_data *data;

	if ((data = xmalloc(sizeof(struct setenv_data))) == NULL)
		return -1;

	data->variable = NULL;
	data->value = NULL;
	data->append_mode = 0;
	element->handler_data = data;

	return 0;
}

static void setenv_free(const element_s * const element)
{
	struct setenv_data *data = (struct setenv_data *) element->handler_data;

	xfree(data->variable);
	xfree(data->value);
	xfree(data);
}

static int setenv_attribute(const element_s * const element,
			    const struct handler_attribute * const attr,
			    const char * const value)
{
	struct setenv_data *data = (struct setenv_data *) element->handler_data;

	switch (attr->private) {
	case SETENV_MODE:
		if (strcmp(value, "append")) {
			Nprint_err("<%s>: the only \"mode\" allowed is \"append\". (%s)", element->handler->name, value);
			return 1;
		}
		data->append_mode = 1;
		return 0;
	default:
		return 1;
	}
}

static int setenv_parameter(const element_s * const element,
			    const struct handler_parameter * const param,
			    const char * const value)
{
	struct setenv_data *data = (struct setenv_data *) element->handler_data;

	switch (param->private) {
	case SETENV_VARIABLE:
		if (data->variable) {
			Nprint_err("<%s>: cannot specify <variable> more than once.", element->handler->name);
			return 1;
		}
		data->variable = xstrdup(value);
		return 0;
	case SETENV_VALUE:
		if (data->value) {
			Nprint_err("<%s>: cannot specify <value> more than once.", element->handler->name);
			return 1;
		}
		data->value = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int setenv_valid_data(const element_s * const element)
{
	struct setenv_data *data = (struct setenv_data *) element->handler_data;

	if (!data->variable) {
		Nprint_err("<%s>: <variable> must be specified.", element->handler->name);
		return 0;
	}

	if (!data->value && data->append_mode) {
		Nprint_err("<%s>: <value> must be specified in \"append\" mode.", element->handler->name);
		return 0;
	}

	return 1;
}

static INLINE int set_variable(const char * const variable, const char * const value)
{
	Nprint_h("Setting environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		return -1;
	}

	return 0;
}

static INLINE int append_to_variable(const char * const variable, const char * const value)
{
	int status = 0;
	char *old_value, *full_value = NULL;
       

	if ((old_value = getenv(variable))) {
		full_value = xstrdup(old_value);
	}
	append_str(&full_value, value);

	Nprint_h("Appending to environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, full_value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		status = -1;
	}

	xfree(full_value);

	return status;
}

static INLINE int unset_variable(const char * const variable)
{
	Nprint_h("Unsetting environment variable %s.", variable);

	unsetenv(variable);

	return 0;
}

static int setenv_main(const element_s * const element)
{
	struct setenv_data *data = (struct setenv_data *) element->handler_data;

	if (!data->value) {
		return unset_variable(data->variable);
	} else {
		if (data->append_mode) {
			return append_to_variable(data->variable, data->value);
		} else {
			return set_variable(data->variable, data->value);
		}
	}
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "setenv",
		.description = "Set environment",
		.syntax_version = "2.0",
		.main = setenv_main,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0,
		.setup = setenv_setup,
		.free = setenv_free,
		.parameters = setenv_parameters,
		.attributes = setenv_attributes,
		.parameter = setenv_parameter,
		.attribute = setenv_attribute,
		.valid_data = setenv_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "setenv",
		.description = "Set environment",
		.syntax_version = "3.0",
		.main = setenv_main,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0,
		.setup = setenv_setup,
		.free = setenv_free,
		.parameters = setenv_parameters,
		.attributes = setenv_attributes,
		.parameter = setenv_parameter,
		.attribute = setenv_attribute,
		.valid_data = setenv_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
