/*
 *  mkdir.c - Handler.
 * 
 *  Copyright (C) 2001, 2002
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

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"
#include "config.h"


#define El_mkdir_dirs(el) alloc_trimmed_param_value("dir", el)
#define El_mkdir_perm(el) alloc_trimmed_param_value("permissions", el)


char handler_name[] = "mkdir";
char handler_description[] = "Make directories";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] = { "options", "base", "dir", "permissions", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status = 0;
	int parents = option_exists("parents", el);
	char *tok;
	char *base;
	char *directories;
	char *perm;


	if ((directories = El_mkdir_dirs(el))== NULL) {
		Nprint_h_err("No directories specified.");
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(directories);
		return -1;
	}

	perm = El_mkdir_perm(el);

	for (tok = strtok(directories, WHITE_SPACE); tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		if (perm) {
			Nprint_h("Creating directory in %s%s: %s (%s)",
				base, parents ? " (parents)" : "", tok, perm);
		} else {
			Nprint_h("Creating directory in %s%s: %s",
				base, parents ? " (parents)" : "", tok);
		}

		if (parents) {
			status = execute_command("mkdir -p %s", tok);
		} else {
			status = execute_command("mkdir %s", tok);
		}

		if (status) {
			Nprint_h_err("Creating %s failed.", tok);
			break;
		}

		if (! perm) {
			continue;
		}

		/* Change permissions. */
		if ((status = execute_command("chmod %s %s", perm, tok))) {
			Nprint_h_err("Changing permissions failed.");
			break;
		}
	}

	xfree(base);
	xfree(directories);
	xfree(perm);
	
	return status;
}
