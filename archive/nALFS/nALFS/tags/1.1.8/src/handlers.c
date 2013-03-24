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
#include <dlfcn.h>

#include "parser.h"
#include "utility.h"
#include "win.h"
#include "nalfs.h"
#include "config.h"

#include "handlers.h"


typedef char *(*alloc_handler_function_f)(element_s *);


static handler_s **handlers;
static char **parameters;


/*
 * Returns a pointer to handler_s if element (name/version) has a handler,
 * or NULL if it doesn't.
 */
handler_s *find_handler(const char *name, const char *version)
{
	int i;
	handler_s *h;


	for (i = 0; (h = handlers[i]); ++i) {
		if (strcmp(name, h->name) == 0
		&& strcmp(version, h->version) == 0) {
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



static INLINE int add_to_parameters(char **params)
{
	int i, total = 0;
	char *param;


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

/* Check if this handler already exists (has the same name and version). */
static INLINE int does_already_exist(const char *name, const char *version)
{
	int i = 0;
	handler_s *h;


	for (i = 0; (h = handlers[i]); ++i) {
		if (strcmp(name, h->name) == 0
		&& strcmp(version, h->version) == 0) {
			return 1;
		}
	}

	return 0;
}

static INLINE int number_of_parameters(void)
{
	int i;

	for (i = 0; parameters[i]; ++i)
		/* Count the number of parameters. */;

	return i;
}

static INLINE int number_of_handlers(void)
{
	int i;

	for (i = 0; handlers[i]; ++i)
		/* Count the number of handlers. */;

	return i;
}

static INLINE int load_handler(const char *filename)
{
	int i, all_symbols_found = 1;
	char *error, *version;

	char *name, **versions, *description;
	int *action;
	main_handler_function_f main_function;
	char **params;

	void *handle;
	handler_s *handler;


	if ((handle = dlopen(filename, RTLD_NOW)) == NULL) {
		Nprint_err("%s", dlerror());
		return -1;
	}

	name = dlsym(handle, "handler_name");
	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		all_symbols_found = 0;
	}

	versions = dlsym(handle, "handler_syntax_versions");
	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		all_symbols_found = 0;
	}

	description = dlsym(handle, "handler_description");
	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		all_symbols_found = 0;
	}

	action = dlsym(handle, "handler_action");
	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		all_symbols_found = 0;
	}

	main_function = (main_handler_function_f)dlsym(handle, "handler_main");
	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		all_symbols_found = 0;
	}

	params = dlsym(handle, "handler_parameters");
	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		all_symbols_found = 0;
	}

	/* Some symbols are missing. */
	if (! all_symbols_found) {
		dlclose(handle);
		return -1;
	}

	/* Create a handler for each syntax version. */
	for (i = 0; (version = versions[i]); ++i) {
		int n;


		/* Check if this handler already exists in "handlers". */
		if (does_already_exist(name, version)) {
			Nprint_warn("The handler already exists, "
				"skipping it: %s (%s)", name, version);
			dlclose(handle);
			continue;
		}

		handler = xmalloc(sizeof *handler);

		handler->name = name;
		handler->version = version;
		handler->description = description;
		handler->action = *action;
		handler->main_function = main_function;
		handler->handle = handle;

		n = number_of_handlers();

		/* Add to handlers. */
		handlers = xrealloc(handlers, (n + 2) * sizeof *handlers);
		handlers[n] = handler;
		handlers[n+1] = NULL;
	}

	/* Update parameters. */
	add_to_parameters(params);

	return 0;
}

int load_all_handlers(void)
{
	DIR *dir;
	struct dirent *next;


	handlers = xmalloc(sizeof *handlers);
	handlers[0] = NULL;

	parameters = xmalloc(sizeof *parameters);
	parameters[0] = NULL;

	if ((dir = opendir(HANDLERS_DIRECTORY)) == NULL) {
		Nprint_err("Can't open handlers directory (%s):",
			HANDLERS_DIRECTORY);
		Nprint_err("%s", strerror(errno));
		return -1;
	}

	while ((next = readdir(dir)) != NULL) {
		char *filename = NULL;
		struct stat file_stat;


		append_str(&filename, HANDLERS_DIRECTORY);
		append_str(&filename, next->d_name);

		if (stat(filename, &file_stat) == 0) {
			/* Only load it if it's not a directory. */
			if (! S_ISDIR(file_stat.st_mode)) {
				char *tmp = strrchr(filename, '.');
				/* Only load .so files. */
				if (tmp && strncmp(tmp, ".so", 3) == 0) {
					load_handler(filename);
				}
			}
		}

		xfree(filename);
	}

	closedir(dir);

	Nprint("Loading handlers from %s.", HANDLERS_DIRECTORY);
	Nprint("Total %d handlers loaded.", number_of_handlers());
	Nprint("Total %d parameters found.", number_of_parameters());

	return 0;
}



/*
 * Utility functions tied to handlers in some way.
 */

static int get_symbol(void **symbol, const char *string, element_s *el)
{
	char *error;


	if (el == NULL) {
		Nprint_err("Element is NULL.");
		return -1;
	}

	if (el->handler == NULL) {
		Nprint_err("Handler is NULL.");
		return -1;
	}

	if (el->handler->handle == NULL) {
		Nprint_err("Handle is NULL.");
		return -1;
	}

	if (string == NULL) {
		Nprint_err("String is NULL.");
		return -1;
	}

	*symbol = dlsym(el->handler->handle, string);

	if ((error = dlerror()) != NULL)  {
		Nprint_err("%s", error);
		return -1;
	}

	return 0;
}

char *alloc_package_name(element_s *el)
{
	alloc_handler_function_f s;


	if (get_symbol((void **)&s, "handler_alloc_package_name", el)) {
		return NULL;
	}

	return s(el);
}

char *alloc_package_version(element_s *el)
{
	alloc_handler_function_f s;


	if (get_symbol((void **)&s, "handler_alloc_package_version", el)) {
		return NULL;
	}

	return s(el);
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
	alloc_handler_function_f s;


	if (get_symbol((void **)&s, "handler_alloc_textdump_file", el)) {
		return NULL;
	}

	return s(el);
}

char *alloc_execute_command(element_s *el)
{
	alloc_handler_function_f s;


	if (get_symbol((void **)&s, "handler_alloc_execute_command", el)) {
		return NULL;
	}

	return s(el);
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
