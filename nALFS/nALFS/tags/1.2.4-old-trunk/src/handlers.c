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

#include <sys/stat.h>

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

/* Embedded "handlers" for the profile and comment elements. */

enum {
	PROFILE_PATH
};

struct profile_data {
	char *path;
};

static const struct handler_attribute profile_attributes[] = {
	{ .name = "path", .private = PROFILE_PATH },
        { .name = NULL }
};

static int profile_setup(element_s * const element)
{
	struct profile_data *data;

	if ((data = xmalloc(sizeof(struct profile_data))) == NULL)
		return 1;

	data->path = NULL;
	element->handler_data = data;

	return 0;
}

static void profile_free(const element_s * const element)
{
	struct profile_data *data = (struct profile_data *) element->handler_data;

	xfree(data->path);
	xfree(data);
}

static int profile_attribute(const element_s * const element,
			     const struct handler_attribute * const attr,
			     const char * const value)
{
	struct profile_data *data = (struct profile_data *) element->handler_data;

	switch (attr->private) {
	case PROFILE_PATH:
		/* will never be called by parser more than once */
		data->path = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int profile_valid_child(const element_s * const element,
			       const element_s * const child)
{
	(void) element;
	(void) child;
	return 1;
}

static char *profile_data(const element_s * const element,
			  const handler_data_e data_requested)
{
	struct profile_data *data = (struct profile_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_SYNTAX_VERSION:
		return xstrdup(*opt_default_syntax);
	case HDATA_DISPLAY_NAME:
	{
		char *tmp;

		/* display only the file name, not the whole path */
		if ((tmp = strrchr(data->path, '/'))) {
			return xstrdup(++tmp);
		} else {
			return xstrdup(data->path);
		}
	}
	default:
		break;
	}

	return NULL;
}

static int profile_main(const element_s * const el)
{
	(void) el;

	return 0;
}

struct comment_data {
	char *content;
};

static int comment_setup(element_s * const element)
{
	struct comment_data *data;

	if ((data = xmalloc(sizeof(struct comment_data))) == NULL)
		return 1;

	data->content = NULL;
	element->handler_data = data;

	return 0;
}

static void comment_free(const element_s * const element)
{
	struct comment_data *data = (struct comment_data *) element->handler_data;

	xfree(data->content);
	xfree(data);
}

static int comment_content(const element_s * const element,
		       const char * const content)
{
	struct comment_data *data = (struct comment_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

static char *comment_data(const element_s * const element,
			  const handler_data_e data_requested)
{
	struct comment_data *data = (struct comment_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_DISPLAY_NAME:
		if (strchr(data->content, '\n')) {
			return xstrdup("comment block");
		} else {
			return xstrdup(data->content);
		};
	default:
		break;
	}

	return NULL;
}

static int comment_main(const element_s * const el)
{
	(void) el;

	return 0;
}

static handler_info_s embedded_handlers_info[] = {
	{
		.name = "__profile",
		.description = "profile",
		.syntax_version = "all",
		.main = profile_main,
		.type = HTYPE_NORMAL,
		.valid_child = profile_valid_child,
		.data = HDATA_SYNTAX_VERSION | HDATA_DISPLAY_NAME,
		.alloc_data = profile_data,
		.attributes = profile_attributes,
		.attribute = profile_attribute,
		.setup = profile_setup,
		.free = profile_free,
	},
	{
		.name = "__comment",
		.description = "comment",
		.syntax_version = "all",
		.main = comment_main,
		.type = HTYPE_COMMENT,
		.setup = comment_setup,
		.free = comment_free,
		.content = comment_content,
		.data = HDATA_DISPLAY_NAME,
		.alloc_data = comment_data,
	},
	{
		.name = NULL
	}
};

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

static int parse_handler_info(handler_info_s * handler_info,
			      lt_dlhandle handle)
{
	int i;

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

static int load_handler(lt_dlhandle handle, lt_ptr data)
{
	handler_info_s *handler_info;

	(void) data;

	handler_info = (handler_info_s *) lookup_symbol(handle, "handler_info");

	if (handler_info == NULL) {
		return 0;
	}

	return parse_handler_info(handler_info, handle);
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

	/* Load the embedded handlers first, then the loaded ones. */
	(void) parse_handler_info(embedded_handlers_info, NULL);

	lt_dlforeach(&load_handler, NULL);

	return 0;
}

unsigned int get_handler_count(void)
{
	return handlers.cnt;
}

/*
 * Utility functions tied to handlers in some way.
 */

char *alloc_package_name(element_s *el)
{
	return el->handler->alloc_data(el, HDATA_NAME);
}

char *alloc_package_version(element_s *el)
{
	return el->handler->alloc_data(el, HDATA_VERSION);
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

const char *find_handler_data(const element_s * const element,
			      const handler_data_e data_requested)
{
	const element_s *s;
	char *data;

	for (s = element; s; s = s->parent) {
		if (!s->handler)
			continue;
		if ((s->handler->data & data_requested) == 0)
			continue;

		data = s->handler->alloc_data(s, data_requested);
		if (data)
			return data;
	}

	return NULL;
}

const char *alloc_base_dir(const element_s * const element)
{
	return find_handler_data(element->parent, HDATA_BASE);
}

int change_to_base_dir(const element_s * const element,
		       const char * const local_base,
		       const int default_root)
{
	const char *dir;
	int result;

	if (local_base)
		return change_current_dir(local_base);

	if ((dir = alloc_base_dir(element)) != NULL) {
		result = change_current_dir(dir);
		xfree(dir);
		return result;
	}

	if (default_root) {
		return change_current_dir("/");
	} else {
		return -1;
	}
}

const char *alloc_stage_shell(const element_s * const el)
{
	const char *dir;

	dir = find_handler_data(el->parent, HDATA_SHELL);

	if (dir)
		return dir;

	return xstrdup("sh");
}

int option_in_string(const char * const option, const char * const string)
{
	char *tmp;
	const char *tok;

	tmp = xstrdup(string);
	for (tok = strtok(tmp, WHITE_SPACE); tok; tok = strtok(NULL, WHITE_SPACE)) {
		if (strcmp(option, tok) == 0)
			break;
	}
	xfree(tmp);
	return tok ? 1 : 0;
}

const struct handler_attribute *find_handler_attribute(const handler_info_s *handler,
						       const char *name)
{
	int i;
	const struct handler_attribute *attr;

	if (!handler->attributes)
		return NULL;

	for (i = 0; (handler->attributes[i].name); ++i) {
		attr = &handler->attributes[i];
		if (strcmp(attr->name, name) == 0)
			return attr;
	}

	return NULL;
}

const struct handler_parameter *find_handler_parameter(const handler_info_s *handler,
						       const char *name)
{
	int i;
	const struct handler_parameter *param;

	if (!handler->parameters)
		return NULL;

	for (i = 0; (handler->parameters[i].name); ++i) {
		param = &handler->parameters[i];
		if (strcmp(param->name, name) == 0)
			return param;
	}

	return NULL;
}
