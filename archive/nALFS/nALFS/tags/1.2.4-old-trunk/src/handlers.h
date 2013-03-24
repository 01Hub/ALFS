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
	HTYPE_COMMENT = 1,
	HTYPE_NORMAL = 2,
	HTYPE_PACKAGE = 4,
	HTYPE_TEST = 16,		/* handler provides a test result */
	HTYPE_TRUE_RESULT = 32,	/* handler should be run for a true test */
	HTYPE_FALSE_RESULT = 64,	/* handler should be run for a false test */
	HTYPE_STAGE = 256,      /* handler is a stage container */
	HTYPE_STAGEINFO = 512,  /* handler provides stage information */
	HTYPE_DIGEST = 1024,    /* handler provides file digest information */
	HTYPE_PACKAGEINFO = 2048,  /* handler provides package information */
} handler_type_e;

typedef enum handler_data_e {
	HDATA_COMMAND = 1,
	HDATA_VERSION = 4,
	HDATA_NAME = 2,
	HDATA_BASE = 16,
	HDATA_SHELL = 32,
	HDATA_SYNTAX_VERSION = 64,
	HDATA_DISPLAY_NAME = 128,
	HDATA_DISPLAY_DETAILS = 256,
} handler_data_e;

struct handler_attribute {
	const char * const name;
	const int private;		/* Internal to handler. */
	const int content_optional;
	const int untrimmed;		/* handler wants raw content */
};

struct handler_parameter {
	const char * const name;
	const int private;		/* Internal to handler. */
	const int content_optional;
	const int untrimmed;		/* handler wants raw content */
};

typedef char *(*handler_data_f)(const element_s * const element,
				const handler_data_e data_requested);
typedef int (*handler_f)(const element_s * const element);
typedef int (*handler_test)(const element_s * const element, int * const result);
typedef int (*handler_setup)(element_s * const element);
typedef void (*handler_free)(const element_s * const element);
typedef int (*handler_attribute)(const element_s * const element,
				 const struct handler_attribute * const attr,
				 const char * const value);
typedef int (*handler_parameter)(const element_s * const element,
				 const struct handler_parameter * const param,
				 const char * const value);
typedef int (*handler_content)(const element_s * const element,
			       const char * const content);
typedef int (*handler_valid_data)(const element_s * const element);
typedef int (*handler_valid_child)(const element_s * const element,
				   const element_s * const child);

typedef struct handler_info_s {
	const char *name;		/* Name of the element it handles. */
	const char *description;	/* Short description. */
	const char *syntax_version;	/* Syntax version string. */
	const struct handler_parameter *parameters; /* Parameters allowed. */
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
	handler_valid_data valid_data; /* Validate private data. */
	handler_valid_child valid_child; /* Validate potential child. */
	handler_attribute attribute;
	handler_parameter parameter;
	handler_content content;
} handler_info_s;


typedef struct handler_s {
	handler_info_s *info;
	lt_dlhandle handle;
} handler_s;


handler_s *find_handler(const char *name, const char *version);
int parameter_exists(const char *name);

int load_all_handlers(void);
unsigned int get_handler_count(void);

/*
 * Handlers' utility functions.
 */

char *alloc_package_name(element_s *el);
char *alloc_package_version(element_s *el);
char *alloc_package_string(element_s *el);
int package_has_name_and_version(element_s *el);

const char *find_handler_data(const element_s * const element,
			      const handler_data_e data_requested);

const char *alloc_base_dir(const element_s * const element);

int change_to_base_dir(const element_s * const element, const char * const local_base, 
		       const int default_root);

const char *alloc_stage_shell(const element_s * const el);
int option_in_string(const char * const option, const char * const string);

const struct handler_attribute *find_handler_attribute(const handler_info_s *handler,
						       const char *name);
const struct handler_parameter *find_handler_parameter(const handler_info_s *handler,
						       const char *name);

#endif /* H_HANDLER_ */
