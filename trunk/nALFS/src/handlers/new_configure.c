/*
 *  new-configure.c - Handler.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME new_configure
#include <nALFS.h>
#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"


char HANDLER_SYMBOL(name)[] = "configure";
char HANDLER_SYMBOL(description)[] = "Configure";
char *HANDLER_SYMBOL(syntax_versions)[] = { "3.0", "3.1", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "command", NULL };
char *HANDLER_SYMBOL(parameters)[] = { "param", "prefix", NULL };
int HANDLER_SYMBOL(action) = 1;


int HANDLER_SYMBOL(main)(element_s *el)
{
	int status;
	char *c, *command = NULL;
	char *base;
       

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	command = xstrdup("");

	append_prefix_elements(&command, el);

	if ((c = attr_value("command", el))) {
		append_str(&command, c);
	} else {
		append_str(&command, "./configure");
	}

	append_param_elements(&command, el);

	Nprint_h("Executing in %s:", base);
	Nprint_h("    %s", command);
	status = execute_command("%s", command);

	xfree(base);
	xfree(command);

	return status;
}
