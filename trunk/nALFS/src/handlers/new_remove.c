/*
 *  new-remove.c - Handler.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME new_remove
#include <nALFS.h>
#include "utility.h"
#include "win.h"
#include "backend.h"


char HANDLER_SYMBOL(name)[] = "remove";
char HANDLER_SYMBOL(description)[] = "Remove files";
char *HANDLER_SYMBOL(syntax_versions)[] = { "3.0", "3.1", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { NULL };
char *HANDLER_SYMBOL(parameters)[] = { NULL };
int HANDLER_SYMBOL(action) = 1;


int HANDLER_SYMBOL(main)(element_s *el)
{
	int status = 0;
	char *name;
       

	if (change_current_dir("/")) {
		return -1;
	}

	if ((name = alloc_trimmed_str(el->content)) == NULL) {
		Nprint_h_err("No name specified.");
		return -1;
	}

	Nprint_h("Removing %s.", name);

	if ((status = execute_command("rm -fr %s", name))) {
		Nprint_h_err("Removing failed.");
	}

	xfree(name);
	
	return status;
}
