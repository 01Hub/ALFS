/*
 *  remove.c - Handler.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME remove
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"

static const char *remove_parameters_ver_3_2[] = { "base", "file",  NULL };

#if HANDLER_SYNTAX_2_0

static INLINE void warn_if_doesnt_exist(const char *file)
{
        struct stat file_stat;

        if (lstat(file, &file_stat)) {
		if (errno == ENOENT) {
			Nprint_h_warn("File %s doesn't exist.", file);
		}
	}
}


static int remove_main_ver2(element_s *el)
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

		if ((status = execute_command(el, "rm -fr %s", tok))) {
			Nprint_h_err("Removing failed.");
			break;
		}
	}

	xfree(targets);
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 

static int remove_main_ver3(element_s *el)
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

	if ((status = execute_command(el, "rm -fr %s", name))) {
		Nprint_h_err("Removing failed.");
	}

	xfree(name);
	
	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_3_2

static int remove_main_ver3_2(element_s *el)
{
	int status   = 0;
	char *name   = NULL;
	char *base   = NULL;
	element_s *p = NULL;

	if ((first_param("file", el)) == NULL) {
		Nprint_h_err("No file(s) specified.");
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		return -1;
	}

	xfree(base);

	for (p = first_param("file", el); p; p = next_param(p)) {

    		if ((name = alloc_trimmed_str(p->content)) == NULL) {
            		Nprint_h_err("No file specified.");
            		status = -1;
            		break;
	    	}
        
        	Nprint_h("Removing %s.", name);

        	if ((status = execute_command(el, "rm -fr %s", name))) {
            		Nprint_h_err("Removing failed.");
            		status = -1;
            		break;
	    	}

		xfree(name);
    	}

     	// if we had to exit prematurely from an if test then free name
      
        if (status) {
		xfree(name);
	}

	return status;
}

#endif

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "2.0",
		.main = remove_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "3.0",
		.main = remove_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "3.1",
		.main = remove_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "3.2",
		.parameters = remove_parameters_ver_3_2,
		.main = remove_main_ver3_2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.alternate_shell = 1,
		.priority = 0
	},
#endif
	{
		.name = NULL
	}
};
