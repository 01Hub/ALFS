/*
 *  remove.c - Handler.
 * 
 *  Copyright (C) 2001-2004
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

#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME remove
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"

enum {
	REMOVE_BASE,
	REMOVE_FILE,
};

struct remove_data {
	char *base;
	int file_count;
	char **files;
	char *content;
};

#if HANDLER_SYNTAX_3_2

static const struct handler_parameter remove_parameters_v3_2[] = {
	{ .name = "file", .private = REMOVE_FILE },
	{ .name = NULL }
};

static const struct handler_attribute remove_attributes_v3_2[] = {
	{ .name = "base", .private = REMOVE_BASE },
	{ .name = NULL }
};

#endif

static int remove_setup(element_s * const element)
{
	struct remove_data *data;

	if ((data = xmalloc(sizeof(struct remove_data))) == NULL)
		return -1;

	data->file_count = 0;
	data->files = NULL;
	data->base = NULL;
	data->content = NULL;
	element->handler_data = data;

	return 0;
}

static void remove_free(const element_s * const element)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;
	int i;

	xfree(data->base);
	xfree(data->content);
	if (data->file_count > 0) {
		for (i = 0; i < data->file_count; i++)
			xfree(data->files[i]);
		xfree(data->files);
	}
	xfree(data);
}

static int remove_attribute(const element_s * const element,
			    const struct handler_attribute * const attr,
			    const char * const value)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;

	switch (attr->private) {
	case REMOVE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify \"base\" more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int remove_parameter(const element_s * const element,
			    const struct handler_parameter * const param,
			    const char * const value)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;

	switch (param->private) {
	case REMOVE_FILE:
		data->file_count++;
		if ((data->files = xrealloc(data->files,
					    sizeof(data->files[0]) * (data->file_count))) == NULL) {
			Nprint_err("xrealloc() failed: %s", strerror(errno));
			return -1;
		}
		data->files[(data->file_count - 1)] = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int remove_content(const element_s * const element,
			  const char * const content)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

#if HANDLER_SYNTAX_2_0 || HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static int remove_valid_data(const element_s * const element)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;

	if (!data->content) {
		Nprint_err("<%s>: content must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#endif /* HANDLER_SYNTAX_2_0 || HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_2_0

static INLINE void warn_if_doesnt_exist(const char * const file)
{
        struct stat file_stat;

        if (lstat(file, &file_stat)) {
		if (errno == ENOENT) {
			Nprint_h_warn("File %s doesn't exist.", file);
		}
	}
}

static int remove_main_ver2(const element_s * const element)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;
	int status = 0;
	char *tok;
	char *tmp;
       
	if (change_current_dir("/"))
		return -1;

	tmp = xstrdup(data->content);
	for (tok = strtok(tmp, WHITE_SPACE); tok; tok = strtok(NULL, WHITE_SPACE)) {
		Nprint_h("Removing %s.", tok);
		warn_if_doesnt_exist(tok);
		if ((status = execute_command(element, "rm -fr %s", tok))) {
			Nprint_h_err("Removing failed.");
			break;
		}
	}
	xfree(tmp);

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 

static int remove_main_ver3(const element_s * const element)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;
	int status = 0;
       
	if (change_current_dir("/"))
		return -1;

	Nprint_h("Removing %s.", data->content);

	if ((status = execute_command(element, "rm -fr %s", data->content)))
		Nprint_h_err("Removing failed.");
	
	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */

#if HANDLER_SYNTAX_3_2

static int remove_main_ver3_2(const element_s * const element)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;
	int status = 0;
	int i;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	for (i = 0; i < data->file_count; i++) {
        	Nprint_h("Removing %s.", data->files[i]);

        	if ((status = execute_command(element, "rm -fr %s", data->files[i]))) {
            		Nprint_h_err("Removing failed.");
            		status = -1;
            		break;
	    	}
    	}

	return status;
}

static int remove_valid_data_v3_2(const element_s * const element)
{
	struct remove_data *data = (struct remove_data *) element->handler_data;

	if (!data->file_count) {
		Nprint_err("<%s>: <file> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

#endif

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "2.0",
		.main = remove_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = remove_setup,
		.free = remove_free,
		.content = remove_content,
		.valid_data = remove_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "3.0",
		.main = remove_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = remove_setup,
		.free = remove_free,
		.content = remove_content,
		.valid_data = remove_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "3.1",
		.main = remove_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = remove_setup,
		.free = remove_free,
		.content = remove_content,
		.valid_data = remove_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "remove",
		.description = "Remove files",
		.syntax_version = "3.2",
		.parameters = remove_parameters_v3_2,
		.attributes = remove_attributes_v3_2,
		.main = remove_main_ver3_2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = remove_setup,
		.free = remove_free,
		.valid_data = remove_valid_data_v3_2,
		.attribute = remove_attribute,
		.parameter = remove_parameter,
	},
#endif
	{
		.name = NULL
	}
};
