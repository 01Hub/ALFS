/*
 *  permissions.c - Handler.
 *
 *  Copyright (C) 2001, 2002 by Neven Has <haski@sezampro.yu>
 *
 *  Parts Copyright (C) 2002 by Maik Schreiber <bZ@iq-computing.de>
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

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"
#include "config.h"


#define El_permissions_mode(el) alloc_trimmed_param_value("mode", el)
#define El_permissions_targets(el) alloc_trimmed_param_value("name", el)


char handler_name[] = "permissions";
char handler_description[] = "Change permissions";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] = { "base", "options", "mode", "name", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status = 0;
	int recursive = option_exists("recursive", el);
	char *tok;
	char *base;
	char *mode;
	char *targets;
	char *command = NULL;
	char *message = NULL;


	if ((mode = El_permissions_mode(el)) == NULL) {
		Nprint_h_err("No permission specified.");
		return -1;
	}

	if ((targets = El_permissions_targets(el)) == NULL) {
		Nprint_h_err("No targets specified.");
		xfree(mode);
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(targets);
		xfree(mode);

		return -1;
	}

	for (tok = strtok(targets, WHITE_SPACE); tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		append_str(&command, "chmod ");

		append_str(&message, "Changing permissions to ");
		append_str(&message, mode);
		append_str(&message, " ");
		if (recursive) {
			append_str(&command, "-R ");
			append_str(&message, "(recursive) ");
		}
		append_str(&message, "in ");
		append_str(&message, base);
		append_str(&message, ": ");
		append_str(&message, tok);

		append_str(&command, mode);
		append_str(&command, " ");
		append_str(&command, tok);

		Nprint_h("%s", message);

		if ((status = execute_command(command))) {
			Nprint_h_err("Changing permissions failed.");
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
	xfree(mode);
	
	return status;
}
