/*
 *  package.c - Handler.
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

#include <time.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME package
#include <nALFS.h>

#include "handlers.h"
#include "logging.h"
#include "parser.h"
#include "backend.h"
#include "utility.h"
#include "nprint.h"

/* <package> handler */

enum {
	PKG_NAME,
	PKG_VERSION,
	PKG_BASE,
};

struct package_data {
	char *name;
	char *version;
	char *base;
	const element_s *packageinfo;
};

static int package_setup(element_s * const element)
{
	struct package_data *data;

	if ((data = xmalloc(sizeof(struct package_data))) == NULL)
		return 1;

	data->name = NULL;
	data->version = NULL;
	data->base = NULL;
	data->packageinfo = NULL;
	element->handler_data = data;

	return 0;
}

static void package_free(const element_s * const element)
{
	struct package_data *data = (struct package_data *) element->handler_data;
	
	xfree(data->name);
	xfree(data->version);
	xfree(data->base);
	xfree(element->handler_data);
}

static int package_attribute(const element_s * const element,
			     const struct handler_attribute * const attr,
			     const char * const value)
{
	struct package_data *data = (struct package_data *) element->handler_data;

	switch (attr->private) {
	case PKG_NAME:
		if (data->name) {
			Nprint_err("<%s>: cannot specify \"name\" more than once.", element->handler->name);
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	case PKG_VERSION:
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

static int package_parameter(const element_s * const element,
			     const struct handler_parameter * const param,
			     const char * const value)
{
	struct package_data *data = (struct package_data *) element->handler_data;

	switch (param->private) {
	case PKG_NAME:
		if (data->name) {
			Nprint_err("<%s>: cannot specify <name> more than once.", element->handler->name);
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	case PKG_VERSION:
		if (data->version) {
			Nprint_err("<%s>: cannot specify <version> more than once.", element->handler->name);
			return 1;
		}
		data->version = xstrdup(value);
		return 0;
	case PKG_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int package_valid_child(const element_s * const element,
			       const element_s * const child)
{
	struct package_data *data = (struct package_data *) element->handler_data;

	if (child->handler->type & HTYPE_PACKAGEINFO) {
		if (data->packageinfo) {
			Nprint_err("<%s>: only one <packageinfo> allowed.", element->handler->name);
			return 0;
		}

		data->packageinfo = child;
		return 1;
	}

	return child->handler->type & (HTYPE_NORMAL |
				       HTYPE_COMMENT |
				       HTYPE_PACKAGE);
}

static int package_valid_data(const element_s * const element)
{
	struct package_data *data = (struct package_data *) element->handler_data;

	if (!data->name) {
		Nprint_err("<%s>: \"name\" must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static char *package_data(const element_s * const element,
			  const handler_data_e data_requested)
{
	struct package_data *data = (struct package_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_BASE:
		if (data->base)
			return xstrdup(data->base);
		break;
	case HDATA_NAME:
		if (data->name)
			return xstrdup(data->name);
		break;
	case HDATA_VERSION:
		if (data->version)
			return xstrdup(data->version);
		break;
	case HDATA_DISPLAY_NAME:
	{
		char *display = NULL;

		append_str_format(&display, "Package %s %s",
				  data->name ? data->name : "",
				  data->version ? data->version : "");

		return display;
	}
	default:
		break;
	}

	return NULL;
}

static int package_main(const element_s * const element)
{
	struct package_data *data = (struct package_data *) element->handler_data;
	int status = 0;

	start_logging_element(element);
	log_start_time(element);

	if (data->packageinfo) {
		status = execute_children_filtered(element, HTYPE_PACKAGEINFO);
	}

	if (status == 0)
		status = execute_children_filtered(element, ~HTYPE_PACKAGEINFO);

	log_end_time(element, status);
	timer_pause();
	end_logging_element(element, status);
	timer_resume();

	return status;
}

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter package_parameters_v2[] = {
	{ .name = "name", .private = PKG_NAME },
	{ .name = "version", .private = PKG_VERSION },
	{ .name = "base", .private = PKG_BASE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_attribute package_attributes_v3[] = {
	{ .name = "name", .private = PKG_NAME },
	{ .name = "version", .private = PKG_VERSION },
	{ .name = NULL }
};

/* <version> handler */

enum {
	VERSION_CONDITION,
};

static const struct handler_attribute version_attributes[] = {
	{ .name = "condition", .private = VERSION_CONDITION },
	{ .name = NULL }
};

struct version_data {
	char *condition;
	char *content;
};

static int version_setup(element_s *element)
{
	struct version_data *data;

	if ((data = xmalloc(sizeof(struct version_data))) == NULL)
		return 1;

	data->condition = NULL;
	data->content = NULL;
	element->handler_data = data;

	return 0;
}

static void version_free(const element_s * const element)
{
	struct version_data *data = (struct version_data *) element->handler_data;
	
	xfree(data->condition);
	xfree(data->content);
	xfree(element->handler_data);
}

static int version_attribute(const element_s * const element,
			     const struct handler_attribute * const attr,
			     const char * const value)
{
	struct version_data *data = (struct version_data *) element->handler_data;

	switch (attr->private) {
	case VERSION_CONDITION:
		if (data->condition) {
			Nprint_err("<%s>: cannot specify \"condition\" more than once.", element->handler->name);
			return 1;
		}
		data->condition = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int version_content(const element_s * const element,
			   const char * const content)
{
	struct version_data *data = (struct version_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

static int version_valid_data(const element_s * const element)
{
	struct version_data *data = (struct version_data *) element->handler_data;

	if (!data->content) {
		Nprint_err("<%s>: content cannot be empty.", element->handler->name);
		return 0;
	}

	return 1;
}

static int version_test(const element_s * const element, int * const result)
{
	struct version_data *data = (struct version_data *) element->handler_data;

/* TODO: check_stamp_version(package, condition, version); */
/* needs to get package name from parent element in some fashion */

	(void) element;
	(void) result;
	(void) data;
	return 0;
}

/* <utilizes> amd <requires> handlers */

enum {
	UTREQ_NAME,
};

struct utreq_data {
	char *name;
};

static const struct handler_parameter utreq_parameters[] = {
	{ .name = "name", .private = UTREQ_NAME },
	{ .name = NULL }
};

static int utreq_setup(element_s * const element)
{
	struct utreq_data *data;

	if ((data = xmalloc(sizeof(struct utreq_data))) == NULL)
		return -1;

	data->name = NULL;
	element->handler_data = data;

	return 0;
}

static void utreq_free(const element_s * const element)
{
	struct utreq_data *data = (struct utreq_data *) element->handler_data;

	xfree(data->name);
	xfree(data);
}

static int utreq_parameter(const element_s * const element,
			    const struct handler_parameter * const param,
			    const char * const value)
{
	struct utreq_data *data = (struct utreq_data *) element->handler_data;

	switch (param->private) {
	case UTREQ_NAME:
		if (data->name) {
			Nprint_err("<%s>: cannot specify <name> more than once.", element->handler->name);
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int utreq_valid_data(const element_s * const element)
{
	struct utreq_data *data = (struct utreq_data *) element->handler_data;

	if (!data->name) {
		Nprint_err("<%s>: <name> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static int utreq_valid_child(const element_s * const element,
			     const element_s * const child)
{
	(void) element;

	if (child->handler->test != version_test)
		return 0;

	return 1;
}

static int process_utreq(const element_s * const element, int optional)
{
	struct utreq_data *data = (struct utreq_data *) element->handler_data;
	int i;
	element_s *child;
	int result;

	if (check_stamp(data->name) != 0) {
		if (optional) {
			Nprint_h_warn("Utilized package missing: %s", data->name);
		} else {
			Nprint_h_warn("Required package missing: %s", data->name);
			return 1;
		}
	}

	result = 0;
	for (child = element->children; !result && child; child = child->next) {
		i = do_execute_test_element(child, result);
		if (i != 0)
			return i;
	}

	if (!result && !optional) {
		Nprint_h_warn("Required package version missing: %s", data->name);
		return 1;
	}
	return 0;
}

static int utilizes_main(const element_s * const element)
{
	return process_utreq(element, 1);
}

static int requires_main(const element_s * const element)
{
	return process_utreq(element, 0);
}

/* <packageinfo> handler */

static int packageinfo_valid_child(const element_s * const element,
				   const element_s * const child)
{
	(void) element;

	if (!(child->handler->main == requires_main) ||
	     (child->handler->main == utilizes_main))
		return 0;

	return 1;
}

static int packageinfo_main(const element_s * const element)
{
	return execute_children(element);
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "2.0",
		.parameters = package_parameters_v2,
		.main = package_main,
		.type = HTYPE_PACKAGE | HTYPE_NORMAL,
		.data = HDATA_NAME | HDATA_VERSION | HDATA_BASE | HDATA_DISPLAY_NAME,
		.alloc_data = package_data,
		.setup = package_setup,
		.free = package_free,
		.main = package_main,
		.parameter = package_parameter,
		.valid_child = package_valid_child,
		.valid_data = package_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "3.0",
		.attributes = package_attributes_v3,
		.main = package_main,
		.type = HTYPE_PACKAGE | HTYPE_NORMAL,
		.data = HDATA_NAME | HDATA_VERSION | HDATA_DISPLAY_NAME,
		.alloc_data = package_data,
		.setup = package_setup,
		.free = package_free,
		.main = package_main,
		.attribute = package_attribute,
		.valid_child = package_valid_child,
		.valid_data = package_valid_data,
	},
	{
		.name = "packageinfo",
		.description = "packageinfo",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL,
		.main = packageinfo_main,
		.valid_child = packageinfo_valid_child,
	},
	{
		.name = "utilizes",
		.description = "utilizes",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL,
		.setup = utreq_setup,
		.free = utreq_free,
		.parameters = utreq_parameters,
		.parameter = utreq_parameter,
		.valid_data = utreq_valid_data,
		.valid_child = utreq_valid_child,
		.main = utilizes_main,
	},
	{
		.name = "requires",
		.description = "requires",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL,
		.setup = utreq_setup,
		.free = utreq_free,
		.parameters = utreq_parameters,
		.parameter = utreq_parameter,
		.valid_data = utreq_valid_data,
		.valid_child = utreq_valid_child,
		.main = requires_main,
	},
	{
		.name = "version",
		.description = "version",
		.syntax_version = "3.0",
		.type = HTYPE_TEST,
		.setup = version_setup,
		.free = version_free,
		.attributes = version_attributes,
		.attribute = version_attribute,
		.content = version_content,
		.valid_data = version_valid_data,
		.test = version_test,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "3.1",
		.attributes = package_attributes_v3,
		.main = package_main,
		.type = HTYPE_PACKAGE | HTYPE_NORMAL,
		.data = HDATA_NAME | HDATA_VERSION | HDATA_DISPLAY_NAME,
		.alloc_data = package_data,
		.setup = package_setup,
		.free = package_free,
		.main = package_main,
		.attribute = package_attribute,
		.valid_child = package_valid_child,
		.valid_data = package_valid_data,
	},
	{
		.name = "packageinfo",
		.description = "packageinfo",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.main = packageinfo_main,
		.valid_child = packageinfo_valid_child,
	},
	{
		.name = "utilizes",
		.description = "utilizes",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.setup = utreq_setup,
		.free = utreq_free,
		.parameters = utreq_parameters,
		.parameter = utreq_parameter,
		.valid_data = utreq_valid_data,
		.valid_child = utreq_valid_child,
		.main = utilizes_main,
	},
	{
		.name = "requires",
		.description = "requires",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.setup = utreq_setup,
		.free = utreq_free,
		.parameters = utreq_parameters,
		.parameter = utreq_parameter,
		.valid_data = utreq_valid_data,
		.valid_child = utreq_valid_child,
		.main = requires_main,
	},
	{
		.name = "version",
		.description = "version",
		.syntax_version = "3.1",
		.type = HTYPE_TEST,
		.setup = version_setup,
		.free = version_free,
		.attributes = version_attributes,
		.attribute = version_attribute,
		.content = version_content,
		.valid_data = version_valid_data,
		.test = version_test,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "3.2",
		.attributes = package_attributes_v3,
		.main = package_main,
		.type = HTYPE_PACKAGE | HTYPE_NORMAL,
		.data = HDATA_NAME | HDATA_VERSION | HDATA_DISPLAY_NAME,
		.alloc_data = package_data,
		.setup = package_setup,
		.free = package_free,
		.main = package_main,
		.attribute = package_attribute,
		.valid_child = package_valid_child,
		.valid_data = package_valid_data,
	},
	{
		.name = "packageinfo",
		.description = "packageinfo",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.main = packageinfo_main,
		.valid_child = packageinfo_valid_child,
	},
	{
		.name = "utilizes",
		.description = "utilizes",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.setup = utreq_setup,
		.free = utreq_free,
		.parameters = utreq_parameters,
		.parameter = utreq_parameter,
		.valid_data = utreq_valid_data,
		.valid_child = utreq_valid_child,
		.main = utilizes_main,
	},
	{
		.name = "requires",
		.description = "requires",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.setup = utreq_setup,
		.free = utreq_free,
		.parameters = utreq_parameters,
		.parameter = utreq_parameter,
		.valid_data = utreq_valid_data,
		.valid_child = utreq_valid_child,
		.main = requires_main,
	},
	{
		.name = "version",
		.description = "version",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.setup = version_setup,
		.free = version_free,
		.attributes = version_attributes,
		.attribute = version_attribute,
		.content = version_content,
		.valid_data = version_valid_data,
		.test = version_test,
	},
#endif
	{
		.name = NULL
	}
};
