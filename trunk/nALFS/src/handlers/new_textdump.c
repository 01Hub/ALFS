/*
 *  new-textdump.c - Handler.
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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME new_textdump
#include <nALFS.h>
#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "nalfs-core.h"
#include "backend.h"


#define El_textdump_content(el) alloc_trimmed_param_value("content", el)


static INLINE FILE *open_for_append(const char *file, const char *base)
{
	Nprint_h("Appending text to \"%s\" (%s).", file, base);
	return fopen(file, "a");
}

static INLINE FILE *open_for_overwrite(const char *file, const char *base)
{
	Nprint_h("Creating new file in %s: %s", base, file);
	return fopen(file, "w");
}


char HANDLER_SYMBOL(name)[] = "textdump";
char HANDLER_SYMBOL(description)[] = "Dump text";
char *HANDLER_SYMBOL(syntax_versions)[] = { "3.0", "3.1", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "mode", NULL };
char *HANDLER_SYMBOL(parameters)[] = { "file", "content", NULL };
int HANDLER_SYMBOL(action) = 1;


int HANDLER_SYMBOL(main)(element_s *el)
{
	char *tok;
	char *base = NULL;
	char *file;
	char *content;
	char *mode = attr_value("mode", el);
	FILE *fp;


	if ((file = alloc_textdump_file(el))== NULL) {
		Nprint_h_err("No file for textdump specified.");
		return -1;
	}

	if ((content = El_textdump_content(el)) == NULL) {
		Nprint_h_err("No content for textdump specified.");
		xfree(file);
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(file);
		xfree(content);
		return -1;
	}

	fp = (mode && strcmp(mode, "append") == 0) ?
		open_for_append(file, base) : open_for_overwrite(file, base);

	if (fp == NULL) {
		Nprint_h_err("Unable to open \"%s\": %s",
			file, strerror(errno));
		xfree(base);
		xfree(file);
		xfree(content);
		return -1;
	}

	for (tok = strtok(content, "\n"); tok; tok = strtok(NULL, "\n")) {
		fprintf(fp, "%s\n", ++tok);
	}

	fclose(fp);

	xfree(base);
	xfree(file);
	xfree(content);
	
	return 0;
}

char *HANDLER_SYMBOL(alloc_textdump_file)(element_s *el)
{
	return alloc_trimmed_param_value("file", el);
}
