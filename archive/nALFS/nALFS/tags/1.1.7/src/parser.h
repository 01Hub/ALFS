/*
 *  parser.h - General profile parsing functions and utilities.
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


#ifndef H_PARSER_
#define H_PARSER_


#define Can_run(el) ( \
	(el)->type == TYPE_ELEMENT || \
	(el)->type == TYPE_PROFILE || \
	(el)->type == TYPE_ROOT )

#define Is_element_name(el,str) \
	(el->type == TYPE_ELEMENT && strcmp(el->name, str) == 0)
#define Is_parameter_name(el,str) \
	(el->type == TYPE_PARAMETER && strcmp(el->name, str) == 0)


typedef enum run_status_e {
	RUN_STATUS_FAILED,
	RUN_STATUS_RUNNING,
	RUN_STATUS_SOME_DONE,
	RUN_STATUS_DONE,
	RUN_STATUS_NONE
} run_status_e;

typedef enum el_type_e {
	TYPE_ROOT,
	TYPE_PROFILE,
	TYPE_ELEMENT,
	TYPE_PARAMETER,
	TYPE_COMMENT,
	TYPE_DOCTYPE,
	TYPE_ENTITY,
	TYPE_UNKNOWN
} el_type_e;

typedef struct element_s {
	el_type_e type;
	run_status_e run_status;

	int marked;
	int should_run;
	int hide_children;

	struct handler_s *handler;

	char *name;
	char **attr;
	char *content;

	struct element_s *parent;
	struct element_s *children;
	struct element_s *prev;
	struct element_s *next;
} element_s;


element_s *init_new_element(void);
void free_element(element_s *el);
void link_element(element_s *el, element_s *prev, element_s *parent);
element_s *parse_profile(const char *filename);

char *attr_value(const char *name, element_s *element);
char *raw_param_value(const char *name, element_s *element);
char *alloc_trimmed_param_value(const char *name, element_s *element);
element_s *next_param(element_s *param);
element_s *first_param(const char *name, element_s *element);
element_s *get_main_profile(element_s *el);
element_s *get_next_element(element_s *el);
element_s *get_prev_element(element_s *el);


#endif
