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
#include <limits.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utility.h"
#include "win.h"
#include "handlers.h"
#include "nalfs-core.h"
#include "options.h"
#include "libXML-tree.h"
#include "parser.h"

static unsigned int element_id = 0;
element_s *root_element = NULL;

element_s *init_new_element(void)
{
	element_s *new = xmalloc(sizeof *new);

	new->type = TYPE_UNKNOWN;

	new->id = element_id++;

	new->should_run = 0;
	new->run_status = RUN_STATUS_NONE;
	new->marked = 0;
	new->hide_children = 1;

	new->handler = NULL;
	new->handler_data = NULL;

	new->parent = NULL;
	new->children = NULL;
	new->prev = NULL;
	new->next = NULL;
	new->profile = NULL;

	return new;
}

static void create_root_element(void)
{
	root_element = init_new_element();
	root_element->type = TYPE_ROOT;
}

void add_profile(element_s * const profile)
{
	ASSERT(profile->type == TYPE_PROFILE);

	if (root_element->children == NULL) {
		root_element->children = profile;
	} else {
		element_s *last_profile = root_element->children;
		while (last_profile->next) {
			last_profile = last_profile->next;
		}
		last_profile->next = profile;
		profile->prev = last_profile;
	}
}

void remove_profile(element_s * const profile)
{
	ASSERT(profile->type == TYPE_PROFILE);

	if (profile->prev == NULL) {
		/* It's the top profile. */
		profile->parent->children = profile->next;
	} else {
		profile->prev->next = profile->next;
	}

	if (profile->next) {
		profile->next->prev = profile->prev;
	}

	free_element(profile);
}

element_s *create_profile_element(const char * const profile_path)
{
	element_s *profile = init_new_element();

	profile->type = TYPE_PROFILE;
	profile->parent = root_element;
	profile->profile = profile;
	profile->handler = find_handler("__profile", "all")->info;
	(void) profile->handler->setup(profile);
	(void) profile->handler->attribute(profile,
					   find_handler_attribute(profile->handler,
								  "path"),
					   profile_path);

	return profile;
}

element_s *create_comment_element(const element_s * const profile,
				  const element_s * const parent)
{
	element_s *el = init_new_element();

	el->type = TYPE_COMMENT;
	el->parent = parent;
	el->profile = profile;
	el->handler = find_handler("__comment", "all")->info;

	return el;
}

element_s *create_handler_element(const element_s * const profile,
				  const element_s * const parent,
				  const char * const handler_name)
{
	element_s *el = NULL;
	const char *syntax_version;
	handler_s *handler;

	syntax_version = find_handler_data(parent, HDATA_SYNTAX_VERSION);
	if ((handler = find_handler(handler_name, syntax_version)) != NULL) {
		el = init_new_element();
		el->type = TYPE_ELEMENT;
		el->parent = parent;
		el->profile = profile;
		el->handler = handler->info;
	} else {
		Nprint_err("No handler found for %s (syntax version %s).",
			   handler_name, syntax_version);
		free_element(el);
		el = NULL;
	}

	xfree(syntax_version);
	return el;
}

void free_element(element_s *el)
{
	element_s *tmp;
	element_s *child = el->children;

	while (child) {
		tmp = child->next;
		free_element(child);
		child = tmp;
	}

	if (el->handler_data)
		el->handler->free(el);

	xfree(el);
}

/* Link new element. */
void link_element(element_s *el, element_s *prev)
{
	/* Parent<->Child */
	if (el->parent->children == NULL) {
		el->parent->children = el;
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

	append_str(el_path, el->handler->name);
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

	profile = parse_with_libxml2_tree(filename);

	print_unknown_elements(profile);

	return profile;
}

void init_parser(void)
{
	init_libXML_tree();
	create_root_element();
}

/*
 * Different utility funtions.
 */

element_s *get_element_by_id(unsigned int id)
{
	element_s *el;

	for (el = root_element; el; el = get_next_element(el)) {
		if (el->id == id)
			return el;
	}

	return NULL;
}

element_s *get_profile_by_element(const element_s * const el)
{
	return el->profile;
}

element_s *get_profile_by_name(const char * const name)
{
	element_s *el;

	for (el = root_element->children; el; el = el->next) {
/* TODO: is this correct, or even necessary? */
		if (strcmp(el->handler->name, name) == 0) {
			return el;
		}
	}

	return NULL;
}

element_s *get_next_element(const element_s * const el)
{
	element_s *tmp;

	if (el->children) {
		return el->children;
	}

	if (el->next) {
		return el->next;
	}

	for (tmp = el->parent; tmp; tmp = tmp->parent) {
		if (tmp->next) {
			return tmp->next;
		}
	}

	return NULL;
}

element_s *get_prev_element(const element_s * const el)
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

static int has_child_with_mark(const element_s * const el)
{
	element_s *child;

	for (child = el->children; child; child = child->next)
		if (child->marked &&
		    ((child->handler->type & HTYPE_NORMAL) != 0))
			return 1;

	return 0;
}

void mark_element(element_s * const el, int recursive)
{
	element_s *child;

	el->marked = 1;
	if (el->parent && !el->parent->marked)
		mark_element(el->parent, 0);

	for (child = el->children; child; child = child->next) {
		if (child->marked)
			continue;
		if (recursive ||
		    ((child->handler->type & HTYPE_NORMAL) == 0))
			mark_element(child, 1);
	}
}

void unmark_element(element_s * const el, int recursive)
{
	element_s *child;

	el->marked = 0;
	if (el->parent && el->parent->marked &&
	    !has_child_with_mark(el->parent))
		unmark_element(el->parent, 0);

	for (child = el->children; child; child = child->next) {
		if (!child->marked)
			continue;
		if (recursive ||
		    ((child->handler->type & HTYPE_NORMAL) == 0))
			unmark_element(child, 1);
	}
}
