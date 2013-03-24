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
#include "ltdl/ltdl.h"


typedef enum handler_type_e {
	/* Using 0 for all other handlers, for now */
	HTYPE_EXECUTE = 1,
	HTYPE_PACKAGE,
	HTYPE_TEXTDUMP
} handler_type_e;

typedef enum handler_data_e {
	HDATA_COMMAND, HDATA_NAME, HDATA_VERSION, HDATA_FILE
} handler_data_e;

typedef char *(*handler_data_f)(element_s *, handler_data_e data);
typedef int (*handler_f)(element_s *);

typedef struct handler_info_s {
	const char *name;		/* Name of the element it handles. */
	const char *description;	/* Short description. */
	const char *syntax_version;	/* Syntax version string. */
	const char **parameters;	/* Parameters allowed. */

	handler_f main;

	handler_type_e type;
	handler_data_f alloc_data;

	int is_action;		/* Whether it's the element that actually
				 * does something, or it's only a "container".
				 */
	int priority;           /* Higher priority handlers "override" lower
				   priority ones (allows user to make custom
				   handlers that replace standard ones) */
} handler_info_s;


typedef struct handler_s {
	handler_info_s *info;
	lt_dlhandle handle;
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
char *append_prefix_elements(char **string, element_s *el);


#endif /* H_HANDLER_ */
