/*
 *  remove.c - Handler.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME remove
#include <nALFS.h>
#include "utility.h"
#include "win.h"
#include "parser.h"
#include "backend.h"


static INLINE void warn_if_doesnt_exist(const char *file)
{
        struct stat file_stat;

        if (stat(file, &file_stat)) {
		if (errno == ENOENT) {
			Nprint_h_warn("File %s doesn't exist.", file);
		}
	}
}


char HANDLER_SYMBOL(name)[] = "remove";
char HANDLER_SYMBOL(description)[] = "Remove files";
char *HANDLER_SYMBOL(syntax_versions)[] = { "2.0", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { NULL };
char *HANDLER_SYMBOL(parameters)[] = { NULL };
int HANDLER_SYMBOL(action) = 1;


int HANDLER_SYMBOL(main)(element_s *el)
{
	int status = 0;
	char *tok;
	char *targets;
       

	if (change_current_dir("/")) {
		return -1;
	}

	if ((targets = alloc_trimmed_str(el->content)) == NULL) {
		Nprint_h_err("No targets specified.");
		return -1;
	}

	for (tok = strtok(targets, WHITE_SPACE);
	     tok;
	     tok = strtok(NULL, WHITE_SPACE)) {
		Nprint_h("Removing %s.", tok);

		warn_if_doesnt_exist(tok);

		if ((status = execute_command("rm -fr %s", tok))) {
			Nprint_h_err("Removing failed.");
			break;
		}
	}

	xfree(targets);
	
	return status;
}
