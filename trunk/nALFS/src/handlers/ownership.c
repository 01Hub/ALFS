/*
 *  ownership.c - Handler.
 *
 *  Copyright (C) 2002, 2004
 *
 *  Maik Schreiber <bZ@iq-computing.de>
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


#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME ownership
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter ownership_parameters[] = {
	{ .name = "option" },
	{ .name = "name" },
	{ .name = NULL }
};

static const struct handler_attribute ownership_attributes[] = {
	{ .name = "base" },
	{ .name = "user" },
	{ .name = "group" },
	{ .name = NULL }
};

static int ownership_main(const element_s *const el)
{
	int status = 0;
	int options[1], recursive;
	char *user, *group;
	element_s *p;


	if (change_to_base_dir(el, attr_value("base", el), 1))
		return -1;

	check_options(1, options, "recursive", el);
	recursive = options[0];

	user = attr_value("user", el);
	group = attr_value("group", el);

	if (user == NULL && group == NULL) {
		Nprint_h_err("No user and/or group specified.");
		return -1;
	}

	for (p = first_param("name", el); p; p = next_param(p)) {
		char *s;


		if ((s = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_h_warn("Name empty.");
			continue;
		}

		if (user) {
			char *command = NULL;
			char *message = NULL;


			append_str(&command, "chown ");

			append_str(&message, "Changing user ownership to ");
			append_str(&message, user);

			if (recursive) {
				append_str(&command, "-R ");
				append_str(&message, " (recursive)");
			}

			append_str(&message, ": ");
			append_str(&message, s);

			append_str(&command, user);
			append_str(&command, " ");
			append_str(&command, s);

			Nprint_h("%s", message);

			if ((status = execute_command(el, command))) {
				Nprint_h_err("Changing ownership failed.");
				xfree(s);
				xfree(command);
				xfree(message);
				break;
			}
			
			xfree(command);
			xfree(message);
		}

		if (group) {
			char *command = NULL;
			char *message = NULL;


			append_str(&command, "chgrp ");

			append_str(&message, "Changing group ownership to ");
			append_str(&message, group);
			append_str(&message, " ");

			if (recursive) {
				append_str(&command, "-R ");
				append_str(&message, "(recursive) ");
			}

			append_str(&message, ": ");
			append_str(&message, s);

			append_str(&command, group);
			append_str(&command, " ");
			append_str(&command, s);

			Nprint_h("%s", message);

			if ((status = execute_command(el, command))) {
				Nprint_h_err("Changing ownership failed.");
				xfree(s);
				xfree(command);
				xfree(message);
				break;
			}
			
			xfree(command);
			xfree(message);
		}

		xfree(s);
	}

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_0
	{
		.name = "ownership",
		.description = "Change ownership",
		.syntax_version = "3.0",
		.parameters = ownership_parameters,
		.attributes = ownership_attributes,
		.main = ownership_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "ownership",
		.description = "Change ownership",
		.syntax_version = "3.1",
		.parameters = ownership_parameters,
		.attributes = ownership_attributes,
		.main = ownership_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "ownership",
		.description = "Change ownership",
		.syntax_version = "3.2",
		.parameters = ownership_parameters,	
		.attributes = ownership_attributes,
		.main = ownership_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
	},
#endif
	{
		.name = NULL
	}
};
