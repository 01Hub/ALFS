/*
 *  parser.c - General profile parsing functions and utilities.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include "utility.h"
#include "win.h"
#include "handlers.h"
#include "nalfs.h"
#include "config.h"
#ifdef PARSE_WITH_LIBXML_SAX
#include "libXML-SAX.h"
#else
#include "libXML-tree.h"
#endif
#include "parser.h"


element_s *init_new_element(void)
{
	element_s *new = xmalloc(sizeof *new);


	new->type = TYPE_UNKNOWN;

	new->id = 0;

	new->name = NULL;
	new->attr = NULL;
	new->content = NULL;

	new->should_run = 0;
	new->run_status = RUN_STATUS_NONE;
	new->marked = 0;
	new->hide_children = 1;

	new->handler = NULL;

	new->parent = NULL;
	new->children = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

void free_element(element_s *el)
{
	int i;
	element_s *tmp;
	element_s *child = el->children;


	while (child) {
		tmp = child->next;

		free_element(child);

		child = tmp;
	}

	xfree(el->name);
	xfree(el->content);

	if (el->attr) {
		for (i = 0; el->attr[i]; i += 2) {
			xfree(el->attr[i]);
			xfree(el->attr[i+1]);
		}
		xfree(el->attr[i]);

		xfree(el->attr);
		el->attr = NULL;
	}

	xfree(el);
}

/* Link new element. */
void link_element(element_s *el, element_s *prev, element_s *parent)
{
	/* Parent<->Child */
	if (parent) {
		el->parent = parent;

		if (parent->children == NULL) {
			parent->children = el;
		}
	}

	/* Previous<->Next */
	if (prev) {
		el->prev = prev;

		prev->next = el;
	}
}

static void append_el_parents(char **el_path, element_s *el)
{
	if (el->parent && el->parent->type != TYPE_PROFILE) {
		append_el_parents(el_path, el->parent);
		append_str(el_path, "->");
	}

	append_str(el_path, el->name);
}

static INLINE void print_unknown_elements(element_s *profile)
{
	element_s *el;


	for (el = profile; el; el = get_next_element(el)) {
		if (el->type == TYPE_UNKNOWN) {
			/* Print the names of all element's parents and
			 * the element itself.
			 */
			char *el_path = NULL;

			append_el_parents(&el_path, el);

			Nprint_warn("Unknown element: %s", el_path);

			xfree(el_path);
		}
	}
}

element_s *parse_profile(const char *filename)
{
	element_s *profile;


	if (opt_be_verbose) {
		Nprint("Parsing %s...", filename);
	}

#ifdef PARSE_WITH_LIBXML_SAX
	profile = parse_with_libxml2_SAX(filename);
#else
	profile = parse_with_libxml2_tree(filename);
#endif

	print_unknown_elements(profile);

	return profile;
}



/*
 * Different utility funtions.
 */

char *attr_value(const char *name, element_s *element)
{
	if (element->attr) {
		int i;
		char *att_name = NULL;

		for (i = 0; (att_name = element->attr[i]); i += 2) {
			if (strcmp(att_name, name) == 0) {
				return element->attr[i+1];
			}
		}
	}

	return NULL;
}

char *raw_param_value(const char *name, element_s *element)
{
	element_s *tmp;


	for (tmp = element->children; tmp; tmp = tmp->next) {
		if (strcmp(tmp->name, name) == 0) {
			return tmp->content;
		}
	}

	return NULL;
}

char *alloc_trimmed_param_value(const char *name, element_s *element)
{
	char *tmp;


	if ((tmp = raw_param_value(name, element))) {
		return alloc_trimmed_str(tmp);
	}

	return tmp;
}

element_s *next_param(element_s *param)
{
	element_s *tmp;


	for (tmp = param->next; tmp; tmp = tmp->next) {
		if (tmp->type == TYPE_PARAMETER
		&& strcmp(tmp->name, param->name) == 0) {
			return tmp;
		}
	}

	return NULL;
}

element_s *first_param(const char *name, element_s *element)
{
	element_s *tmp;


	for (tmp = element->children; tmp; tmp = tmp->next) {
		if (tmp->type == TYPE_PARAMETER
		&& strcmp(tmp->name, name) == 0) {
			return tmp;
		}
	}

	return NULL;
}

element_s *get_profile_by_element(element_s *el)
{
	for (; el; el = el->parent) {
		if (el->type == TYPE_PROFILE && !el->parent->parent) {
			return el;
		}
	}

	ASSERT(0); /* Shouldn't be reached. */

	return NULL;
}

element_s *get_profile_by_name(element_s *root, const char *name)
{
	element_s *el;

	for (el = root->children; el; el = el->next) {
		if (strcmp(el->name, name) == 0) {
			return el;
		}
	}

	return NULL;
}

element_s *get_next_element(element_s *el)
{
	if (el->children) {
		return el->children;
	}

	if (el->next) {
		return el->next;
	}

	for (el = el->parent; el; el = el->parent) {
		if (el->next) {
			return el->next;
		}
	}

	return NULL;
}

element_s *get_prev_element(element_s *el)
{
	element_s *tmp, *previous;


	if (el->prev) {
		previous = el->prev;

		for (tmp = get_next_element(previous);
		     tmp && tmp != el;
		     tmp = get_next_element(tmp)) {
			previous = tmp;
		}

		return previous;
	}

	return el->parent;
}
