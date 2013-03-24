/*
 *  textdump.c - Handler.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME textdump
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"


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

static int textdump_main(element_s *el, const char *base_dir)
{
	char *tok;
	char *file;
	char *content;
	char *mode = attr_value("mode", el);
	FILE *fp;


	if ((file = alloc_textdump_file(el)) == NULL) {
		Nprint_h_err("No file for textdump specified.");
		return -1;
	}

	if ((content = El_textdump_content(el)) == NULL) {
		Nprint_h_err("No content for textdump specified.");
		xfree(file);
		return -1;
	}

	if (change_current_dir(base_dir)) {
		xfree(file);
		xfree(content);
		return -1;
	}

	if (mode && strcmp(mode, "append") == 0) {
		fp = open_for_append(file, base_dir);
	} else {
		fp = open_for_overwrite(file, base_dir);
	}

	if (fp == NULL) {
		Nprint_h_err("Unable to open \"%s\":", file);
		Nprint_h_err("    %s", strerror(errno));
		xfree(file);
		xfree(content);
		return -1;
	}

	for (tok = strtok(content, "\n"); tok; tok = strtok(NULL, "\n")) {
		fprintf(fp, "%s\n", ++tok);
	}

	fclose(fp);

	xfree(file);
	xfree(content);
	
	return 0;
}


#if HANDLER_SYNTAX_2_0

static const char *textdump_parameters_ver2[] =
{ "base", "file", "content", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "mode", NULL };

static int textdump_main_ver2(element_s *el)
{
	int i;
	char *base = alloc_base_dir(el);

	i = textdump_main(el, base);

	xfree(base);

	return i;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *textdump_parameters_ver3[] =
{ "file", "content", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "base", "mode", NULL };

static int textdump_main_ver3(element_s *el)
{
	int i;
	char *base = alloc_base_dir_new(el);

	i = textdump_main(el, base);

	xfree(base);

	return i;
}


#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


static char *textdump_data(element_s *el, handler_data_e data)
{
	(void) data;

	return alloc_trimmed_param_value("file", el);
}



/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "2.0",
		.parameters = textdump_parameters_ver2,
		.main = textdump_main_ver2,
		.type = HTYPE_TEXTDUMP,
		.alloc_data = textdump_data,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "3.0",
		.parameters = textdump_parameters_ver3,
		.main = textdump_main_ver3,
		.type = HTYPE_TEXTDUMP,
		.alloc_data = textdump_data,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "textdump",
		.description = "Dump text",
		.syntax_version = "3.1",
		.parameters = textdump_parameters_ver3,
		.main = textdump_main_ver3,
		.type = HTYPE_TEXTDUMP,
		.alloc_data = textdump_data,
		.is_action = 1,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
