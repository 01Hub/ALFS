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
	HTYPE_NORMAL = (1 << 1),
	HTYPE_PACKAGE = (1 << 2),
	HTYPE_TEXTDUMP = (1 << 3),
	HTYPE_TEST = (1 << 4),		/* handler provides a test result */
	HTYPE_TRUE_RESULT = (1 << 5),	/* handler should be run for a true test */
	HTYPE_FALSE_RESULT = (1 << 6),	/* handler should be run for a false test */
	HTYPE_EXECUTE = (1 << 7),
	HTYPE_STAGE = (1 << 8),      /* handler is a stage container */
	HTYPE_STAGEINFO = (1 << 9),  /* handler provides stage information */
} handler_type_e;

typedef enum handler_data_e {
	HDATA_COMMAND = (1 << 1),
	HDATA_NAME = (1 << 2),
	HDATA_VERSION = (1 << 3),
	HDATA_FILE = (1 << 4),
	HDATA_BASE = (1 << 5),
	HDATA_SHELL = (1 << 6),
} handler_data_e;

struct handler_attribute {
	const char * const name;
	const int private;		/* Internal to handler. */
	const int content_optional;
};

typedef char *(*handler_data_f)(const element_s * const element,
				const handler_data_e data_requested);
typedef int (*handler_f)(element_s * const element);
typedef int (*handler_test)(element_s * const element, int * const result);
typedef int (*handler_setup)(element_s * const element);
typedef void (*handler_free)(const element_s * const element);
typedef int (*handler_attribute)(const element_s * const element,
				 const struct handler_attribute * const attr,
				 const char * const value);
typedef int (*handler_parse)(const element_s * const element,
			     const char * const name,
			     const char * const value);
typedef int (*handler_parse_content)(const element_s * const element,
				     const char * const content);
typedef int (*handler_invalid_data)(const element_s * const element);
typedef int (*handler_invalid_child)(const element_s * const element,
				     const element_s * const child);

typedef struct handler_info_s {
	const char *name;		/* Name of the element it handles. */
	const char *description;	/* Short description. */
	const char *syntax_version;	/* Syntax version string. */
	const char **parameters;	/* Parameters allowed. */
	const struct handler_attribute *attributes; /* Attributes allowed. */

	handler_type_e type;
	handler_data_e data;
	handler_data_f alloc_data;

	handler_f main;
	handler_test test;		/* used by HTYPE_TEST handlers */

	int is_action;		/* If the element actually performs something
				   or is only a container */
	int priority;           /* Higher priority handlers "override" lower
				   priority ones (allows user to make custom
				   handlers that replace standard ones) */
	int alternate_shell;	/* commands issued by handler should support
				   <shell> element if present in a containing
				   element */

	/* The following functions are used during profile parsing, to
	   allow handler to store private data in the element_s structure,
	   and to validate the provided parameters, attributes and content.
	   The handler_free function is called when an element is freed,
	   so the handler can free any private data it may have stored.
	*/
	handler_setup setup;	/* Function to setup handler private data. */
	handler_free free;
	handler_invalid_data invalid_data; /* Validate private data. */
	handler_invalid_child invalid_child; /* Validate potential child. */
	handler_attribute attribute;
	handler_parse parse_parameter;
	handler_parse_content parse_content;
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
char *alloc_base_dir_new(const element_s * const el, const int default_root);
char *alloc_stage_shell(const element_s * const el);
int option_exists(const char *option, element_s *element);
void check_options(int total, int *opts, const char *string_, element_s *el);
char *append_param_elements(char **string, element_s *el);
char *append_prefix_elements(char **string, element_s *el);

char *parse_string_parameter(const char * const value,
			     const char * const message);
char *parse_string_content(const char * const value,
			   const char * const message);

#endif /* H_HANDLER_ */
