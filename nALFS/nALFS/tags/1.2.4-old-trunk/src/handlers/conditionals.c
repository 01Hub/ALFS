/*
 *  conditionals.c - Handlers for conditional logic.
 *
 *  Copyright (C) 2004
 *
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

#define MODULE_NAME conditionals
#include <nALFS.h>

#include "handlers.h"
#include "backend.h"
#include "nprint.h"
#include "utility.h"
#include "parser.h"


static int do_shelltest(const element_s * const element, const char * const test,
			int * const result)
{
	int status = 0;
	long num_value;
	char *temp;

	num_value = strtol(test, &temp, 0);
	if (*temp == '\0') {
		/* strtol found the test string to be completely numeric */
		*result = (num_value == 0) ? 0 : 1;
	} else if (strcasecmp(test, "true") == 0) {
		*result = 1;
	} else if (strcasecmp(test, "yes") == 0) {
		*result = 1;
	} else if (strcasecmp(test, "y") == 0) {
		*result = 1;
	} else if (strcasecmp(test, "t") == 0) {
		*result = 1;
	} else if (strcasecmp(test, "false") == 0) {
		*result = 0;
	} else if (strcasecmp(test, "no") == 0) {
		*result = 0;
	} else if (strcasecmp(test, "n") == 0) {
		*result = 0;
	} else if (strcasecmp(test, "f") == 0) {
		*result = 0;
	} else {
		if (change_to_base_dir(element, NULL, 1))
			return -1;

		Nprint_h("    test %s", temp);
		status = execute_command(element, "test %s", temp);
		if ((status == 0) || (status == 1)) {
			*result = (status == 0) ? 1 : 0;
			status = 0;
		} else
			status = -1;
	}

	return status;
}

/* <if> handler */

enum {
	IF_TEST,
	IF_PACKAGE,
};

struct if_data {
	char *test;
	char *package;
};

static int if_setup(element_s * const element)
{
	struct if_data *data;

	if ((data = xmalloc(sizeof(struct if_data))) == NULL)
		return 1;

	data->test = NULL;
	data->package = NULL;
	element->handler_data = data;

	return 0;
}

static void if_free(const element_s * const element)
{
	struct if_data *data = (struct if_data *) element->handler_data;

	xfree(data->test);
	xfree(data->package);
	xfree(data);
}

static int if_valid_child(const element_s * const element,
			  const element_s * const child)
{
	(void) element;

	return child->handler->type & (HTYPE_TEST |
				       HTYPE_TRUE_RESULT |
				       HTYPE_FALSE_RESULT);
}

#if HANDLER_SYNTAX_3_1

static const struct handler_attribute if_attributes_v3_1[] = {
	{ .name = "test", .private = IF_TEST },
	{ .name = "package", .private = IF_PACKAGE },
	{ .name = NULL }
};

static int if_attribute_v3_1(const element_s * const element,
			     const struct handler_attribute * const attr,
			     const char * const value)
{
	struct if_data *data = (struct if_data *) element->handler_data;

	switch (attr->private) {
	case IF_TEST:
		if (data->test) {
			Nprint_err("<if>: cannot specify \"test\" more than once.");
			return 1;
		}
		data->test = xstrdup(value);
		return 0;
	case IF_PACKAGE:
		if (data->package) {
			Nprint_err("<if>: cannot specify \"package\" more than once.");
			return 1;
		}
		data->package = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int if_valid_data_v3_1(const element_s * const element)
{
	struct if_data *data = (struct if_data *) element->handler_data;

	if (data->test && data->package) {
		Nprint_err("<if>: cannot specify both \"test\" and \"package\".");
		return 0;
	}

	if (!(data->test || data->package)) {
		Nprint_err("<if>: must specify either \"test\" or \"package\".");
		return 0;
	}

	return 1;
}

static int if_main_v3_1(const element_s * const element)
{
	struct if_data *data = (struct if_data *) element->handler_data;
	int test_result = 1;
	handler_type_e result_handler_type = HTYPE_TRUE_RESULT;
	int status;

	if (data->test) {
		if ((status = do_shelltest(element, data->test, &test_result)))
			return status;
	} else {
		/* TODO: implement package test */
	}
	if (!test_result)
		result_handler_type = HTYPE_FALSE_RESULT;

	return execute_children_filtered(element, result_handler_type);
}

#endif

#if HANDLER_SYNTAX_3_2

static int if_main_v3_2(const element_s * const element)
{
	int i;
	element_s *child;
	int test_result = 1;
	handler_type_e result_handler_type = HTYPE_TRUE_RESULT;

	for (child = element->children; test_result && child; child = child->next) {
		if ((child->handler->type & HTYPE_TEST) != 0) {
			i = do_execute_test_element(child, &test_result);
			if (i != 0)
				return i;
		}
	}

	if (!test_result)
		result_handler_type = HTYPE_FALSE_RESULT;

	return execute_children_filtered(element, result_handler_type);
}

#endif

#if HANDLER_SYNTAX_3_2

/* handlers for <and>, <or> and <not> (booleans) */

static int boolean_setup(element_s * const element)
{
	(void) element;

	return 0;
}

static int boolean_valid_child(const element_s * const element,
			       const element_s * const child)
{
	(void) element;

	return child->handler->type & HTYPE_TEST;
}

static int not_valid_child(const element_s * const element,
			   const element_s * const child)
{
	if (element->children) {
		Nprint_err("<not>: cannot specify more than one test.");
		return 0;
	}

	if (!(child->handler->type & HTYPE_TEST))
		return 0;

	return 1;
}

static int and_test(const element_s * const element, int * const result)
{
	int i;
	element_s *child;

	*result = 1;
	for (child = element->children; *result && child; child = child->next) {
		i = do_execute_test_element(child, result);
		if (i != 0)
			return i;
	}

	return 0;
}

static int or_test(const element_s * const element, int * const result)
{
	int i;
	element_s *child;

	*result = 0;
	for (child = element->children; !*result && child; child = child->next) {
		i = do_execute_test_element(child, result);
		if (i != 0)
			return i;
	}

	return 0;
}

static int not_test(const element_s * const element, int * const result)
{
	int i;
	element_s *child;

	child = element->children;
	i = do_execute_test_element(child, result);
	if (i != 0)
		return i;
	*result = !*result;

	return 0;
}

/* <test>, <package-built> and <package-version> handlers */

struct test_data {
	char *content;
};

static int test_setup(element_s * const element)
{
	struct test_data *data;

	if ((data = xmalloc(sizeof(struct test_data))) == NULL)
		return 1;

	data->content = NULL;
	element->handler_data = data;

	return 0;
}

static void test_free(const element_s * const element)
{
	struct test_data *data = (struct test_data *) element->handler_data;

	xfree(data->content);
	xfree(data);
}

static int test_content(const element_s * const element,
			const char * const content)
{
	struct test_data *data = (struct test_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

static int test_valid_data(const element_s * const element)
{
	struct test_data *data = (struct test_data *) element->handler_data;

	if (!data->content) {
		Nprint_err("<%s>: content cannot be blank.", element->handler->name);
		return 0;
	}

	return 1;
}

static int shelltest_test(const element_s * const element, int * const result)
{
	struct test_data *data = (struct test_data *) element->handler_data;

	return do_shelltest(element, data->content, result);
}

static int package_version_test(const element_s * const element, int * const result)
{
	struct test_data *data = (struct test_data *) element->handler_data;

	(void) element;
	(void) result;
	(void) data;
	return 0;
}

static int package_built_test(const element_s * const element, int * const result)
{
	struct test_data *data = (struct test_data *) element->handler_data;

	(void) element;
	(void) result;
	(void) data;
	return 0;
}
#endif

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_1
	{
		.name = "if",
		.description = "if",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.main = if_main_v3_1,
		.setup = if_setup,
		.free = if_free,
		.attributes = if_attributes_v3_1,
		.attribute = if_attribute_v3_1,
		.valid_data = if_valid_data_v3_1,
		.valid_child = if_valid_child,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "if",
		.description = "if",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.main = if_main_v3_2,
		.setup = if_setup,
		.free = if_free,
		.valid_child = if_valid_child,
	},
	{
		.name = "and",
		.description = "and",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.is_action = 1,
		.test = and_test,
		.setup = boolean_setup,
		.valid_child = boolean_valid_child,
	},
	{
		.name = "or",
		.description = "or",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.is_action = 1,
		.test = or_test,
		.setup = boolean_setup,
		.valid_child = boolean_valid_child,
	},
	{
		.name = "not",
		.description = "not",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.is_action = 1,
		.test = not_test,
		.setup = boolean_setup,
		.valid_child = not_valid_child,
	},
	{
		.name = "test",
		.description = "Shell Test",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.alternate_shell = 1,
		.is_action = 1,
		.test = shelltest_test,
		.setup = test_setup,
		.free = test_free,
		.content = test_content,
		.valid_data = test_valid_data,
	},
	{
		.name = "package-built",
		.description = "Package Built",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.is_action = 1,
		.alternate_shell = 1,
		.test = package_built_test,
		.setup = test_setup,
		.free = test_free,
		.content = test_content,
		.valid_data = test_valid_data,
	},
	{
		.name = "package-version",
		.description = "Package Version",
		.syntax_version = "3.2",
		.type = HTYPE_TEST,
		.is_action = 1,
		.alternate_shell = 1,
		.test = package_version_test,
		.setup = test_setup,
		.free = test_free,
		.content = test_content,
		.valid_data = test_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
