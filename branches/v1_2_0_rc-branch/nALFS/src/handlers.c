/*
 *  handlers.c - Handlers' functions.
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


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "parser.h"
#include "utility.h"
#include "win.h"
#include "nalfs-core.h"
#include "options.h"

#include "handlers.h"
#include "ltdl/ltdl.h"


static struct handlers {
	int cnt;
	handler_s **list;
} handlers = { 0, NULL };

static char **parameters;


/*
 * Returns a pointer to handler_s if element (name/version) has a handler,
 * or NULL if it doesn't.
 */
handler_s *find_handler(const char *name, const char *syntax_version)
{
	int i;

	for (i = 0; i < handlers.cnt; ++i) {
		handler_s *h = handlers.list[i];

		if (strcmp(name, h->info->name) == 0
		&& strcmp(syntax_version, h->info->syntax_version) == 0) {
			return h;
		}
	}

	return NULL;
}

/*
 * Checks if name is a "parameter element".
 * TODO: Check the syntax version too.
 */
int parameter_exists(const char *name)
{
	int i;


	for (i = 0; parameters[i]; ++i) {
		if (strcmp(parameters[i], name) == 0) {
			return 1;
		}
	}

	return 0;
}



static INLINE int add_new_parameters(const char **params)
{
	int i, total = 0;
	const char *param;


	for (i = 0; (param = params[i]); ++i) {
		int j, param_already_exists = 0;

		/* Check if tok already exists in parameters. */
		for (j = 0; parameters[j]; ++j) {
			if (strcmp(param, parameters[j]) == 0) {
				++param_already_exists;
			}
		}

		if (! param_already_exists) {
			++total;

			parameters = xrealloc(
				parameters,
				(j+2) * sizeof *parameters);
			parameters[j] = xstrdup(param);
			parameters[j+1] = NULL;
		}
	}

	return total;
}

static INLINE int number_of_parameters(void)
{
	int i;

	for (i = 0; parameters[i]; ++i)
		/* Count the number of parameters. */;

	return i;
}

static INLINE lt_ptr lookup_symbol(lt_dlhandle handle, const char *symbol_name)
{
	lt_ptr result;

	if ((result = lt_dlsym(handle, symbol_name)) == NULL)
		Nprint_err("%s: %s", symbol_name, lt_dlerror());
	return result;
}

/* Creates new handler and puts it in handlers. */
static INLINE void create_new_handler(handler_info_s *hi, lt_dlhandle handle)
{
	handler_s *handler = xmalloc(sizeof *handler);

	handler->info = hi;
	handler->handle = handle;

	/* Add to handlers. */

	++handlers.cnt;

	handlers.list = xrealloc(handlers.list,
		(handlers.cnt) * sizeof *handlers.list);

	handlers.list[handlers.cnt-1] = handler;
}

/* Replaces an existing handler with a new one. */
static INLINE void replace_handler(handler_s *existing, handler_info_s *hi,
				   lt_dlhandle handle)
{
	existing->info = hi;
	existing->handle = handle;
}

static int load_handler(lt_dlhandle handle, lt_ptr data)
{
	int i;
	handler_info_s *handler_info;

	(void) data;

	handler_info = (handler_info_s *)lookup_symbol(handle, "handler_info");

	if (handler_info == NULL) {
		return 0;
	}

	/* Go through handler_info[].  */
	for (i = 0; (handler_info[i].name); ++i) {
		handler_info_s *hi = &handler_info[i];
		handler_s *found;

		if ((found = find_handler(hi->name, hi->syntax_version))) {
			if (found->info->priority >= hi->priority) {
				Nprint_warn("The handler already exists, "
					    "skipping it: %s (%s)",
					    hi->name, hi->syntax_version);
			} else {
				Nprint("Replacing handler %s (%s) "
				       "priority %d",
				       found->info->name,
				       found->info->syntax_version,
				       found->info->priority);
				Nprint("with %s (%s) priority %d",
				       hi->name,
				       hi->syntax_version,
				       hi->priority);
				replace_handler(found, hi, handle);
			}
		}
		else
			create_new_handler(hi, handle);
	}

	return 0;
}

static INLINE void add_all_parameters(void)
{
	int i;

	for (i = 0; i < handlers.cnt; ++i) {
		add_new_parameters(handlers.list[i]->info->parameters);
	}
}

#ifndef STATIC_BUILD
static int foreachfile_callback(const char *filename, lt_ptr data)
{
	lt_dlhandle handle;

	(void) data;

	if ((handle = lt_dlopenext(filename)) == NULL) {
		Nprint_err("%s", lt_dlerror());
	}
	return 0;
}
#endif

int load_all_handlers(void)
{
	parameters = xmalloc(sizeof *parameters);
	parameters[0] = NULL;

#ifdef STATIC_BUILD
	LTDL_SET_PRELOADED_SYMBOLS();
#endif

	if (lt_dlinit() != 0) {
		Nprint_err("%s", lt_dlerror());
		return -1;
	}

#ifdef STATIC_BUILD
	if (lt_dlopen_preloaded() != 0)
		Nprint_err("unable to open all preloaded handlers");
#else
	Nprint("Loading handlers from %s.", HANDLERS_DIRECTORY);
	lt_dlforeachfile(HANDLERS_DIRECTORY, &foreachfile_callback, NULL);
#endif

	lt_dlforeach(&load_handler, NULL);
	add_all_parameters();
	Nprint("Total %d handlers loaded.", handlers.cnt);
	Nprint("Total %d parameters found.", number_of_parameters());

	return 0;
}



/*
 * Utility functions tied to handlers in some way.
 */

char *alloc_package_name(element_s *el)
{
	return el->handler->info->alloc_data(el, HDATA_NAME);
}

char *alloc_package_version(element_s *el)
{
	return el->handler->info->alloc_data(el, HDATA_VERSION);
}

char *alloc_package_string(element_s *el)
{
	char *str = NULL;
	char *name = alloc_package_name(el);
	char *version = alloc_package_version(el);


	if (name && version) {
		str = xmalloc(strlen(name) + strlen(version) + 2);
		sprintf(str, "%s-%s", name, version);
	}

	xfree(name);
	xfree(version);

	return str;
}

int package_has_name_and_version(element_s *el)
{
	int s = 0;
	char *name = alloc_package_name(el);
	char *version = alloc_package_version(el);


	if (name && version) {
		s = 1;
	}

	xfree(name);
	xfree(version);

	return s;
}

char *alloc_textdump_file(element_s *el)
{
	return el->handler->info->alloc_data(el, HDATA_FILE);
}

char *alloc_execute_command(element_s *el)
{
	return el->handler->info->alloc_data(el, HDATA_COMMAND);
}

/* Used by old syntax (2.0). */
char *alloc_base_dir(element_s *el)
{
	element_s *p;
	char *dir;


	if ((dir = alloc_trimmed_param_value("base", el))) {
		return dir;
	}

	for (p = el->parent; p; p = p->parent) {
		if (Is_element_name(p, "package")) {
			if ((dir = alloc_trimmed_param_value("base", p))) {
				return dir;
			}

			break; /* One <package> is enough. */
		}
	}

	return xstrdup("/");
}

/* Used by new syntax (3.0). */
char *alloc_base_dir_new(element_s *el)
{
	element_s *s;
	char *dir;


	if ((dir = attr_value("base", el)) && strlen(dir)) {
		return xstrdup(dir);
	}

	for (s = el->parent; s; s = s->parent) {
		if (Is_element_name(s, "stage")) {
			element_s *sinfo;

			if ((sinfo = first_param("stageinfo", s)) == NULL) {
				continue;
			}

			if ((dir = alloc_trimmed_param_value("base", sinfo))) {
				return dir;
			}
		}
	}

	return xstrdup("/");
}

/* Used by the old syntax (2.0). */
int option_exists(const char *option, element_s *element)
{
	int exists = 0;
	char *option_string = alloc_trimmed_param_value("options", element);


	if (option_string && strstr(option_string, option)) {
		exists = 1;
	}

	xfree(option_string);

	return exists;
}

/* Used by new syntax (3.0). */
void check_options(int total, int *opts, const char *string_, element_s *el)
{
	int i;
	char *str, *string;
	element_s *p;


	for (i = 0; i < total; ++i) {
		opts[i] = 0;
	}

	for (p = first_param("option", el); p; p = next_param(p)) {
		int unknown_option = 1;
		char *o;


		if ((o = alloc_trimmed_str(p->content)) == NULL) {
			Nprint_warn("Skipping empty option.");
			continue;
		}

		string = xstrdup(string_);
		for (str = strtok(string, WHITE_SPACE), i = 0;
		     str;
		     str = strtok(NULL, WHITE_SPACE), ++i) {
			if (strcmp(o, str) == 0) {
				unknown_option = 0;
				opts[i] = 1;
				break;
			}
		}
		xfree(string);

		if (unknown_option) {
			Nprint_warn("Ignoring unknown option (%s).", o);
		}

		xfree(o);
	}
}

/* Used by both syntaxes. */
char *append_param_elements(char **string, element_s *el)
{
	element_s *tmp;


	for (tmp = first_param("param", el); tmp; tmp = next_param(tmp)) {
		char *content;

		if ((content = alloc_trimmed_str(tmp->content))) {
			if (*string) {
				append_str(string, " ");
			}
			append_str(string, content);

			xfree(content);

		} else if (opt_be_verbose) {
			Nprint_warn("Skipping empty parameter.");
		}
	}

	return *string;
}

char *append_prefix_elements(char **string, element_s *el)
{
	element_s *tmp;


	for (tmp = first_param("prefix", el); tmp; tmp = next_param(tmp)) {
		char *content;

		if ((content = alloc_trimmed_str(tmp->content))) {
			append_str(string, content);
			append_str(string, " ");

			xfree(content);

		} else if (opt_be_verbose) {
			Nprint_warn("Skipping empty prefix.");
		}
	}

	return *string;
}
