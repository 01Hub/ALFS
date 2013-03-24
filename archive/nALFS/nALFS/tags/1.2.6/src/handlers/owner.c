/*
 *  owner.c - Handler.
 *
 *  Copyright (C) 2002 by Maik Schreiber <bZ@iq-computing.de>
 *
 *  Parts Copyright (C) 2001, 2002 by Neven Has <haski@sezampro.yu>
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

#define MODULE_NAME owner
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#define El_owner_user(el) alloc_trimmed_param_value("user", el)
#define El_owner_group(el) alloc_trimmed_param_value("group", el)
#define El_owner_targets(el) alloc_trimmed_param_value("name", el)


#if HANDLER_SYNTAX_2_0

static const char *owner_parameters[] =
{ "options", "base", "user", "group", "name", NULL };

static int owner_main(element_s *el)
{
	int status = 0;
	int recursive = option_exists("recursive", el);
	char *tok;
	char *base;
	char *user;
	char *group;
	char *targets;
	char *command = NULL;
	char *message = NULL;


	user = El_owner_user(el);
	group = El_owner_group(el);

	if ((user == NULL) && (group == NULL)) {
		Nprint_h_err("No user and/or group specified.");
		xfree(user);
		xfree(group);
		return -1;
	}

	if ((targets = El_owner_targets(el)) == NULL) {
		Nprint_h_err("No targets specified.");
		xfree(user);
		xfree(group);
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(targets);
		xfree(user);
		xfree(group);

		return -1;
	}

	for (tok = strtok(targets, WHITE_SPACE); tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		if ((user != NULL) && (group != NULL)) {
			append_str(&command, "chown ");

			append_str(&message, "Changing ownership to ");
			append_str(&message, user);
			append_str(&message, ":");
			append_str(&message, group);
			append_str(&message, " ");
			if (recursive) {
				append_str(&command, "-R ");
				append_str(&message, "(recursive) ");
			}
			append_str(&message, "in ");
			append_str(&message, base);
			append_str(&message, ": ");
			append_str(&message, tok);

			append_str(&command, user);
			append_str(&command, ":");
			append_str(&command, group);
		} else if (user != NULL) {
			append_str(&command, "chown ");

			append_str(&message, "Changing user ownership to ");
			append_str(&message, user);
			append_str(&message, " ");
			if (recursive) {
				append_str(&command, "-R ");
				append_str(&message, "(recursive) ");
			}
			append_str(&message, "in ");
			append_str(&message, base);
			append_str(&message, ": ");
			append_str(&message, tok);

			append_str(&command, user);
		} else {   /* group != NULL */
			append_str(&command, "chgrp ");

			append_str(&message, "Changing group ownership to ");
			append_str(&message, group);
			append_str(&message, " ");
			if (recursive) {
				append_str(&command, "-R ");
				append_str(&message, "(recursive) ");
			}
			append_str(&message, "in ");
			append_str(&message, base);
			append_str(&message, ": ");
			append_str(&message, tok);

			append_str(&command, group);
		}

		append_str(&command, " ");
		append_str(&command, tok);

		Nprint_h("%s", message);

		if ((status = execute_command(command))) {
			Nprint_h_err("Changing ownership failed.");
			break;
		}

		xfree(command);
		command = NULL;
		xfree(message);
		message = NULL;
	}

	xfree(command);
	xfree(message);

	xfree(base);
	xfree(targets);
	xfree(user);
	xfree(group);
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "owner",
		.description = "Change ownership",
		.syntax_version = "2.0",
		.parameters = owner_parameters,
		.main = owner_main,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
