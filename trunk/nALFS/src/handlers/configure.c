/*
 *  configure.c - Handler.
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
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME configure
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "parser.h"
#include "nprint.h"
#include "backend.h"


#if HANDLER_SYNTAX_2_0

static const struct handler_parameter configure_parameters_ver2[] = {
	{ .name = "base" },
	{ .name = "command" },
	{ .name = "param" },
	{ .name = NULL }
};

static int configure_main_ver2(const element_s * const el)
{
	int status;
	char *c, *command = NULL;
	char *base;
       

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	if ((c = alloc_trimmed_param_value("command", el))) {
		append_str(&command, c);
	} else {
		append_str(&command, "./configure");
	}
	xfree(c);

	append_param_elements(&command, el);

	Nprint_h("Executing in %s:", base);
	Nprint_h("    %s", command);
	status = execute_command(el, "%s", command);

	xfree(base);
	xfree(command);

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter configure_parameters_v3[] = {
	{ .name = "prefix" },
	{ .name = "param" },
	{ .name = NULL }
};

static const struct handler_attribute configure_attributes_v3[] = {
	{ .name = "base" },
	{ .name = "command" },
	{ .name = NULL }
};

static int configure_main_ver3(const element_s * const el)
{
	int status;
	char *c, *command = NULL;
       

	if (change_to_base_dir(el, attr_value("base", el), 1))
		return -1;

	command = xstrdup("");

	append_prefix_elements(&command, el);

	if ((c = attr_value("command", el))) {
		append_str(&command, c);
	} else {
		append_str(&command, "./configure");
	}

	append_param_elements(&command, el);

	Nprint_h("Executing:");
	Nprint_h("    %s", command);
	status = execute_command(el, "%s", command);

	xfree(command);

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "2.0",
		.parameters = configure_parameters_ver2,
		.main = configure_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "3.0",
		.parameters = configure_parameters_v3,
		.attributes = configure_attributes_v3,
		.main = configure_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "3.1",
		.parameters = configure_parameters_v3,
		.attributes = configure_attributes_v3,
		.main = configure_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "configure",
		.description = "Configure",
		.syntax_version = "3.2",
		.parameters = configure_parameters_v3,
		.attributes = configure_attributes_v3,
		.main = configure_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
	},
#endif
	{
		.name = NULL
	}
};
