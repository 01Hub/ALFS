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


#define Is_element_name(el,str) \
	(el->type == TYPE_ELEMENT && strcmp(el->handler->name, str) == 0)

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
	TYPE_COMMENT,
	TYPE_DOCTYPE,
	TYPE_ENTITY,
	TYPE_UNKNOWN
} el_type_e;

typedef struct element_s {
	el_type_e type;
	run_status_e run_status;

	unsigned int id;

	int marked;
	int should_run;
	int hide_children;

	struct handler_info_s *handler;
	void *handler_data;

	struct element_s *parent;
	struct element_s *children;
	struct element_s *prev;
	struct element_s *next;
	struct element_s *profile;
} element_s;

element_s *root_element;

void init_parser(void);

void add_profile(element_s * const profile);
void remove_profile(element_s * const profile);

element_s *create_profile_element(const char * const profile_path);
element_s *create_comment_element(const element_s * const profile,
				  const element_s * const parent);
element_s *create_handler_element(const element_s * const profile,
				  const element_s * const parent,
				  const char * const handler_name);

void free_element(element_s * const el);
void link_element(element_s * const el, element_s * const prev);

element_s *parse_profile(const char *filename);

element_s *get_element_by_id(unsigned int id);

element_s *get_profile_by_element(const element_s * const el);
element_s *get_profile_by_name(const char * const name);

element_s *get_next_element(const element_s * const el);
element_s *get_prev_element(const element_s * const el);

void mark_element(element_s * const el, int recursive);
void unmark_element(element_s * const el, int recursive);

#endif /* H_PARSER_ */
