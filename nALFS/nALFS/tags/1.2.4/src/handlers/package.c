/*
 *  package.c - Handler.
 * 
 *  Copyright (C) 2001-2003
 *  
 *  Neven Has <haski@sezampro.yu>
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
#include <string.h>
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


#if HANDLER_SYNTAX_2_0

static const char *package_parameters_ver2[] =
{ "name", "version", "base", NULL };

static int package_main_ver2(element_s *el)
{
	int i;

	start_logging_element(el);
	log_start_time(el);

	i = execute_children(el);

	log_end_time(el, i);
	end_logging_element(el, i);

	return i;
}

static char *package_data_ver2(element_s *el, handler_data_e data)
{
	if (data == HDATA_NAME) {
		return alloc_trimmed_param_value("name", el);

	} else if (data == HDATA_VERSION) {
		return alloc_trimmed_param_value("version", el);

	}

	return NULL;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static INLINE int check_utilizes(element_s *utilizes)
{
	element_s *el;
	char *package = NULL;


	el = first_param("name", utilizes);

	if (el == NULL) {
		Nprint_h_warn("<utilizes> misses <name>, ignoring.");
		return 1;
	}

	if ((package = alloc_trimmed_str(el->content)) == NULL) {
		Nprint_h_warn("Utilizes name content empty, ignoring.");
		return 1;
	}

	if (check_stamp(package) != 0) {
		Nprint_warn("Utilized package missing: %s", package);
	}

	for (el = first_param("version", utilizes); el; el = next_param(el)) {
		char *version = NULL;
		char *condition = NULL;

		if ((version = alloc_trimmed_str(el->content)) == NULL) {
			Nprint_h_warn("Utilizes version content empty, ignoring.");
			xfree(package);
			return 1;
		}

		condition = attr_value("condition", el);

		if (Empty_string(condition)) {
			Nprint_h_warn("Utilizes condition missing, ignoring.");
			xfree(package);
			xfree(version);
			return 1;
		}
		
		check_stamp_version(package, condition, version);
		/* Ignoring return value, just printing a warning. */			
		xfree(version);
	}

	xfree(package);

	return 0;
}

static INLINE int check_requires(element_s *requires)
{
	element_s *el;
	char *package = NULL;
	int status = 0;


	el = first_param("name", requires);

	if (el == NULL) {
		Nprint_h_warn("<requires> misses <name>, ignoring.");
		return 1;
	}

	if ((package = alloc_trimmed_str(el->content)) == NULL) {
		Nprint_h_warn("Requires name content empty, ignoring.");
		return 1;
	}

	status = check_stamp(package);

	if (status) {
		Nprint_h_err("Some required packages are missing; "
			"build aborted.");
		return -1;
	}

	for (el = first_param("version", requires); el; el = next_param(el)) {
		char *version = NULL;
		char *condition = NULL;

		if ((version = alloc_trimmed_str(el->content)) == NULL) {
			Nprint_h_warn("Requires version content empty, ignoring.");
			xfree(package);
			return 1;
		}

		condition = attr_value("condition", el);

		if (Empty_string(condition)) {
			Nprint_h_warn("Requires condition missing, ignoring.");
			xfree(package);
			xfree(version);
			return 1;
		}

		status = check_stamp_version(package, condition, version);
		/* Ignoring return value, just printing a warning. */			
		xfree(version);

		if (status) {
			Nprint_h_err(
				"Some required packages "
				"don't have the required version; build aborted.");
			xfree(package);
			return -1;
		} else {
			Nprint_h("Required package version OK.");
		}
	}

	xfree(package);

	return 0;
}

static int parse_packageinfo(element_s *packageinfo)
{
	element_s *e;


	for (e = first_param("utilizes", packageinfo); e; e = next_param(e)) {
		if (check_utilizes(e) == -1) {
			return -1;
		}
	}

	for (e = first_param("requires", packageinfo); e; e = next_param(e)) {
		if (check_requires(e) == -1) {
			return -1;
		}
	}

	return 0;
}


static const char *package_parameters_ver3[] = { 
	"packageinfo",
	"description",
	"list",
	"item",
	"para",
	"requires",
	"utilizes",
	"name",
	"version",
	NULL
};
// char *HANDLER_SYMBOL(attributes)[] = { "name", "version", "logfile", NULL };

static int package_main_ver3(element_s *el)
{
	int status = 0;
	element_s *packageinfo;


	start_logging_element(el);
	log_start_time(el);

	packageinfo = first_param("packageinfo", el);
	if (packageinfo != NULL) {
		status = parse_packageinfo(packageinfo);
	}

	if (status == 0) {
		status = execute_children(el);
	}

	log_end_time(el, status);
	end_logging_element(el, status);

	return status;
}

static char *package_data_ver3(element_s *el, handler_data_e data)
{
	char *s;

	if (data == HDATA_NAME) {
		if ((s = attr_value("name", el))) {
			return xstrdup(s);
		}

	} else if (data == HDATA_VERSION) {
		if ((s = attr_value("version", el))) {
			return xstrdup(s);
		}

	}

	return NULL;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "2.0",
		.parameters = package_parameters_ver2,
		.main = package_main_ver2,
		.type = HTYPE_PACKAGE,
		.alloc_data = package_data_ver2,
		.is_action = 0,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "3.0",
		.parameters = package_parameters_ver3,
		.main = package_main_ver3,
		.type = HTYPE_PACKAGE,
		.alloc_data = package_data_ver3,
		.is_action = 0,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "package",
		.description = "Package",
		.syntax_version = "3.1",
		.parameters = package_parameters_ver3,
		.main = package_main_ver3,
		.type = HTYPE_PACKAGE,
		.alloc_data = package_data_ver3,
		.is_action = 0,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
