/*
 *  check.c - Handler.
 * 
 *  Copyright (C) 2002, 2004
 *
 *  Vassili Dzuba <vassilidzuba@nerim.net>
 *  Neven Has <haski@sezampro.yu>
 *  Kevin P. Fleming <kpfleming@linuxfromscratch.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME check
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"
#include "nalfs-core.h"


#if 0
/* Finds first package (by name) in the profiles loaded. */
static INLINE element_s *find_package(const char *name)
{
	element_s *el = get_next_element(root_element);


	for (; el; el = get_next_element(el)) {
		if (Is_element_name(el, "package")) {
			char *pname = attr_value("name", el);

			if (pname && ! strcmp(pname, name)) {
				return el;
			}
		}
	}

	return NULL;
}
#endif


#if HANDLER_SYNTAX_3_0

struct check_data {
	char *content;
};

static int check_setup(element_s * const element)
{
	struct check_data *data;

	if ((data = xmalloc(sizeof(struct check_data))) == NULL)
		return 1;

	data->content = NULL;
	element->handler_data = data;

	return 0;
};

static void check_free(const element_s * const element)
{
	struct check_data *data = (struct check_data *) element->handler_data;

	xfree(data->content);
	xfree(data);
}

static int check_content(const element_s * const element, const char * const content)
{
	struct check_data *data = (struct check_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

static char *check_data(const element_s * const element,
			  const handler_data_e data_requested)
{
	struct check_data *data = (struct check_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_DISPLAY_NAME:
	{
		char *display = NULL;

		append_str_format(&display, "Check for installed package%s%s",
				  data->content ? ": " : "",
				  data->content ? data->content : "");

		return display;
	}
	default:
		break;
	}

	return NULL;
}

static int check_main(const element_s * const element)
{
	struct check_data *data = (struct check_data *) element->handler_data;
	int status;

	status = check_stamp(data->content);
#if 0
	if (status && OPTION_BUILD_REQUIRED) {
		/* Stamp file for the required package doesn't exist,
		 * try to build it.
		 */
	  
		element_s *pack;

		element_s *save_current = NULL;
		element_s *save_current_running = NULL;
		element_s *save_current_finishing = NULL;
		element_s *save_root_element = NULL;

		if ((pack = find_package(package))) {
			element_s *save;

			set_should_run_marks(pack);

			save_current = current;
			save_current_running = current_running;
			save_current_finishing = current_finishing;
			save_root_element = root_element;
	    
			save = pack->parent;
			pack->parent = el;
	    
			status = do_execute_element(pack);

			pack->parent = save;

			current = save_current;
			current_running = save_current_running;
			current_finishing = save_current_finishing;
			root_element = save_root_element;

		} else {
			Nprint_h_warn("Required package (%s) not found "
				"in this profile.", package);
			status = -1;
		}
	}
#endif
	
	return status;
}

#endif /* HANDLER_SYNTAX_3_0 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_0
	{
		.name = "check",
		.description = "Check for installed package",
		.syntax_version = "3.0",
		.main = check_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = check_setup,
		.free = check_free,
		.content = check_content,
		.data = HDATA_DISPLAY_NAME,
		.alloc_data = check_data,
	},
#endif
	{
		.name = NULL
	}
};
