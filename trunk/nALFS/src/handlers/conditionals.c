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


#include <stdlib.h>

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


static const char *null_parameters[] = { NULL };

#if HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2
static int do_shelltest(element_s *element, const char *test, int *result)
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
#endif

#if HANDLER_SYNTAX_3_1
static const char *if_parameters_3_1[] = { "then", "else", NULL };

static int if_main_3_1(element_s *element)
{
	char *shelltest;
	char *packagetest;
	int test_result = 1;
	handler_type_e result_handler_type = HTYPE_TRUE_RESULT;
	int status;

	shelltest = attr_value("test", element);
	packagetest = attr_value("package", element);
	if (shelltest && packagetest) {
		Nprint_h_err("Cannot specify both \"test\" and \"package\".");
		return -1;
	}
	if (!shelltest && !packagetest) {
		Nprint_h_err("Must specify either \"test\" or \"package\".");
		return -1;
	}
	if (shelltest) {
		if ((status = do_shelltest(element, shelltest, &test_result)))
			return status;
	} else {
	}
	if (!test_result)
		result_handler_type = HTYPE_FALSE_RESULT;

	return execute_children_filtered(element, result_handler_type);
}
#endif

#if HANDLER_SYNTAX_3_2
static const char *if_parameters_3_2[] = { "test", "package-version", "package-built", "and", "or", "not", "then", "else", NULL };
static const char *boolean_parameters_3_2[] = { "test", "package-version", "package-built", "and", "or", "not", NULL };

static int if_main_3_2(element_s *element)
{
	int i;
	element_s *child;
	int test_result = 1;
	handler_type_e result_handler_type = HTYPE_TRUE_RESULT;

	for (child = element->children; test_result && child; child = child->next) {
		if (!child->handler) {
			Nprint_h_err("<%s> element not allowed inside <if>.", child->name);
			return -1;
		}
		if ((child->handler->info->type & HTYPE_TEST) != 0) {
			i = do_execute_test_element(child, &test_result);
			if (i != 0)
				return i;
		}
	}

	if (!test_result)
		result_handler_type = HTYPE_FALSE_RESULT;

	return execute_children_filtered(element, result_handler_type);
}

static int and_test_3_2(element_s *element, int *result)
{
	int i;
	element_s *child;

	*result = 1;
	for (child = element->children; *result && child; child = child->next) {
		if (child->handler &&
		    ((child->handler->info->type & HTYPE_TEST) != 0)) {
			i = do_execute_test_element(child, result);
			if (i != 0)
				return i;
		} else {
			Nprint_h_err("<%s> element not allowed inside <and>.", child->name);
			return -1;
		}
	}

	return 0;
}

static int or_test_3_2(element_s *element, int *result)
{
	int i;
	element_s *child;

	*result = 0;
	for (child = element->children; !*result && child; child = child->next) {
		if (child->handler &&
		    ((child->handler->info->type & HTYPE_TEST) != 0)) {
			i = do_execute_test_element(child, result);
			if (i != 0)
				return i;
		} else {
			Nprint_h_err("<%s> element not allowed inside <or>.", child->name);
			return -1;
		}
	}

	return 0;
}

static int not_test_3_2(element_s *element, int *result)
{
	int i;
	element_s *child;

	child = element->children;
	if (child->next) {
		Nprint_h_err("multiple elements not allowed inside <not>.");
		return -1;
	}

	if (child->handler &&
	    ((child->handler->info->type & HTYPE_TEST) != 0)) {
		i = do_execute_test_element(child, result);
		if (i != 0)
			return i;
		*result = !*result;
	} else {
		Nprint_h_err("<%s> element not allowed inside <not>.", child->name);
		return -1;
	}

	return 0;
}

static int shelltest_test_3_2(element_s *element, int *result)
{
	char *test;
	int status = -1;

	if ((test = alloc_trimmed_str(element->content)) == NULL) {
		Nprint_h_err("No test content specified.");
	} else {
		status = do_shelltest(element, test, result);
		xfree(test);
	}

	return status;
}

static int package_version_test_3_2(element_s *element, int *result)
{
	(void) element;
	(void) result;
	return 0;
}

static int package_built_test_3_2(element_s *element, int *result)
{
	(void) element;
	(void) result;
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
		.parameters = if_parameters_3_1,
		.main = if_main_3_1,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "if",
		.description = "if",
		.syntax_version = "3.2",
		.parameters = if_parameters_3_2,
		.main = if_main_3_2,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
	{
		.name = "and",
		.description = "and",
		.syntax_version = "3.2",
		.parameters = boolean_parameters_3_2,
		.test = and_test_3_2,
		.type = HTYPE_TEST,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
	{
		.name = "or",
		.description = "or",
		.syntax_version = "3.2",
		.parameters = boolean_parameters_3_2,
		.test = or_test_3_2,
		.type = HTYPE_TEST,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
	{
		.name = "not",
		.description = "not",
		.syntax_version = "3.2",
		.parameters = boolean_parameters_3_2,
		.test = not_test_3_2,
		.type = HTYPE_TEST,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
	{
		.name = "test",
		.description = "Shell Test",
		.syntax_version = "3.2",
		.parameters = null_parameters,
		.test = shelltest_test_3_2,
		.type = HTYPE_TEST,
		.alloc_data = NULL,
		.alternate_shell = 1,
		.is_action = 1,
		.priority = 0
	},
	{
		.name = "package-built",
		.description = "Package Built",
		.syntax_version = "3.2",
		.parameters = null_parameters,
		.test = package_built_test_3_2,
		.type = HTYPE_TEST,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
	{
		.name = "package-version",
		.description = "Package Version",
		.syntax_version = "3.2",
		.parameters = null_parameters,
		.test = package_version_test_3_2,
		.type = HTYPE_TEST,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
#endif
	{
		.name = NULL
	}
};
