/*
 *  handlers.h - Handlers' functions.
 *
 *  Copyright (C) 2001, 2002, 2004
 *
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


#ifndef H_HANDLER_
#define H_HANDLER_


#include "parser.h"
#include "ltdl/ltdl.h"


typedef enum handler_type_e {
	HTYPE_NORMAL = 1,
	HTYPE_PACKAGE = 2,
	HTYPE_TEXTDUMP = 4,
	HTYPE_TEST = 8,		/* handler provides a test result */
	HTYPE_TRUE_RESULT = 16,	/* handler should be run for a true test */
	HTYPE_FALSE_RESULT = 32,/* handler should be run for a false test */
	HTYPE_EXECUTE = 64,
	HTYPE_STAGE = 128,      /* handler is a stage container */
} handler_type_e;

typedef enum handler_data_e {
	HDATA_COMMAND, HDATA_NAME, HDATA_VERSION, HDATA_FILE
} handler_data_e;

typedef char *(*handler_data_f)(element_s *, handler_data_e data);
typedef int (*handler_f)(element_s *);
typedef int (*handler_test)(element_s *, int *);
typedef int (*handler_parse)(element_s *, const char *, const char *);
typedef int (*handler_setup)(element_s *);
typedef int (*handler_valid)(element_s *);

typedef struct handler_info_s {
	const char *name;		/* Name of the element it handles. */
	const char *description;	/* Short description. */
	const char *syntax_version;	/* Syntax version string. */
	const char **parameters;	/* Parameters allowed. */
	const char **attributes;	/* Attributes allowed. */

	handler_f main;

	handler_type_e type;
	handler_data_f alloc_data;

	int is_action;		/* Whether it's the element that actually
				 * does something, or it's only a "container".
				 */
	int priority;           /* Higher priority handlers "override" lower
				   priority ones (allows user to make custom
				   handlers that replace standard ones) */

	handler_test test;	/* used by HTYPE_TEST */
	int alternate_shell;	/* commands issued by handler should support
				   <shell> element if present in a containing
				   stage
				*/
	/* The following four functions are used during profile parsing, to
	   allow handler to store private data in the element_s structure,
	   and to validate the provided parameters and attributes at
	   profile parsing time.
	*/
	handler_setup setup;	/* Function to setup handler private data. */
	handler_valid valid;	/* Function to validate private data. */
	handler_parse parse_attribute;
	handler_parse parse_parameter;
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
char *alloc_base_dir_force(element_s *el);
char *alloc_stage_shell(element_s *el);
int option_exists(const char *option, element_s *element);
void check_options(int total, int *opts, const char *string_, element_s *el);
char *append_param_elements(char **string, element_s *el);
char *append_prefix_elements(char **string, element_s *el);

#endif /* H_HANDLER_ */
