/*
 *  handlers.h - Handlers' functions.
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


#ifndef H_HANDLER_
#define H_HANDLER_


#include "parser.h"


typedef int (*main_handler_function_f)(struct element_s *);

typedef struct handler_s {
	char *name;		/* Name of the element it handles. */
	char *version;		/* Syntax version string. */

	char *description;	/* Short description. */

	int action;		/* Whether it's the element that actually
				 * does something, or it's only a "container".
				 */

	/* Main handler function. */
	main_handler_function_f main_function;

	void *handle;		/* Handle for the dynamic library. */
} handler_s;


handler_s *find_handler(const char *name, const char *version);
int parameter_exists(const char *name);

int load_all_handlers(void);

/*
 * Handlers' utility functions.
 */

char *alloc_package_name(element_s *el);
char *alloc_package_version(element_s *el);
char *alloc_package_string(element_s *el);
int package_has_name_and_version(element_s *el);
char *alloc_textdump_file(element_s *el);
char *alloc_execute_command(element_s *el);

char *alloc_base_dir(element_s *el);
char *alloc_base_dir_new(element_s *el);
int option_exists(const char *option, element_s *element);
void check_options(int total, int *opts, const char *string_, element_s *el);
char *append_param_elements(char **string, element_s *el);



#endif
