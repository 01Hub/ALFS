/*
 *  new-patch.c - Handler.
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

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"
#include "config.h"


char handler_name[] = "patch";
char handler_description[] = "Patch";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { "base", NULL };
char *handler_parameters[] = { "param", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status;
	char *base;
	char *parameters = NULL;


	if (append_param_elements(&parameters, el) == NULL) {
		Nprint_h_err("No patch parameters specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(parameters);
		return -1;
	}
	
	Nprint_h("Patching in %s", base);
	Nprint_h("    patch %s", parameters);

	if ((status = execute_command("patch %s", parameters))) {
		Nprint_h_err("Patching failed.");
	}
	
	xfree(base);
	xfree(parameters);

	return status;
}
