/*
 *  search_replace.c - Handler.
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

#include <stdio.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME search_replace
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"

#define TMP_SEARCH_REPLACE_FILE	"/tmp/nALFS-XXXXXX"

enum {
	SEARCH_REPLACE_FIND,
	SEARCH_REPLACE_REPLACE,
	SEARCH_REPLACE_FILE,
	SEARCH_REPLACE_BASE,
};

struct search_replace_data {
	char *find;
	char *replace;
	char *file;
	char *base;
};

#if HANDLER_SYNTAX_2_0

static const struct handler_parameter search_replace_parameters_v2[] = {
	{ .name = "base", .private = SEARCH_REPLACE_BASE },
	{ .name = "find", .private = SEARCH_REPLACE_FIND, .untrimmed = 1 },
	{ .name = "replace", .private = SEARCH_REPLACE_REPLACE, .untrimmed = 1 },
	{ .name = "file", .private = SEARCH_REPLACE_FILE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_2_0 */

#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static const struct handler_parameter search_replace_parameters_v3[] = {
	{ .name = "find", .private = SEARCH_REPLACE_FIND, .untrimmed = 1 },
	{ .name = "replace", .private = SEARCH_REPLACE_REPLACE, .untrimmed = 1 },
	{ .name = "file", .private = SEARCH_REPLACE_FILE },
	{ .name = NULL }
};

static const struct handler_attribute search_replace_attributes[] = {
	{ .name = "base", .private = SEARCH_REPLACE_BASE },
	{ .name = NULL }
};

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

static int search_replace_setup(element_s * const element)
{
	struct search_replace_data *data;

	if ((data = xmalloc(sizeof(struct search_replace_data))) == NULL)
		return -1;

	data->base = NULL;
	data->file = NULL;
	data->find = NULL;
	data->replace = NULL;
	element->handler_data = data;

	return 0;
}

static void search_replace_free(const element_s * const element)
{
	struct search_replace_data *data = (struct search_replace_data *) element->handler_data;

	xfree(data->base);
	xfree(data->file);
	xfree(data->find);
	xfree(data->replace);
	xfree(data);
}

static int search_replace_attribute(const element_s * const element,
				    const struct handler_attribute * const attr,
				    const char * const value)
{
	struct search_replace_data *data = (struct search_replace_data *) element->handler_data;

	switch (attr->private) {
	case SEARCH_REPLACE_BASE:
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

static int search_replace_parameter(const element_s * const element,
			    const struct handler_parameter * const param,
			    const char * const value)
{
	struct search_replace_data *data = (struct search_replace_data *) element->handler_data;

	switch (param->private) {
	case SEARCH_REPLACE_BASE:
		if (data->base) {
			Nprint_err("<%s>: cannot specify <base> more than once.", element->handler->name);
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case SEARCH_REPLACE_FILE:
		if (data->file) {
			Nprint_err("<%s>: cannot specify <file> more than once.", element->handler->name);
			return 1;
		}
		data->file = xstrdup(value);
		return 0;
	case SEARCH_REPLACE_FIND:
		if (data->find) {
			Nprint_err("<%s>: cannot specify <find> more than once.", element->handler->name);
			return 1;
		}
		data->find = xstrdup(value);
		return 0;
	case SEARCH_REPLACE_REPLACE:
		if (data->replace) {
			Nprint_err("<%s>: cannot specify <replace> more than once.", element->handler->name);
			return 1;
		}
		data->replace = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int search_replace_valid_data(const element_s * const element)
{
	struct search_replace_data *data = (struct search_replace_data *) element->handler_data;

	if (!data->file) {
		Nprint_err("<%s>: <file> must be specified.", element->handler->name);
		return 0;
	}

	if (!data->find) {
		Nprint_err("<%s>: <find> must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static int search_replace_main(const element_s * const element)
{
	struct search_replace_data *data = (struct search_replace_data *) element->handler_data;
	int i, c, fdw, offset, num_found = 0;
	char *buf = NULL;
	char tmp_file[] = TMP_SEARCH_REPLACE_FILE;
	FILE *fp, *fpw;
	struct stat file_stat;

	if (change_to_base_dir(element, data->base, 1))
		return -1;

	Nprint_h("Searching %s", data->file);
	Nprint_h("    for \"%s\"", data->find);
	Nprint_h("    and replacing with \"%s\".", data->replace ? data->replace : "");

	if ((fp = fopen(data->file, "r")) == NULL) {
		Nprint_h_err("Opening %s failed: %s", data->file, strerror(errno));
		return -1;
	}

	if (fstat(fileno(fp), &file_stat)) {
		Nprint_h_err("Error fstat()ing %s: %s", data->file, strerror(errno));
		return -1;
	}

	if ((fdw = mkstemp(tmp_file)) == -1) {
		Nprint_h_err("Error opening temporary file: %s", strerror(errno));
		return -1;
	}
	
	if ((fpw = fdopen(fdw, "w")) == NULL) {
		Nprint_h_err("Opening %s failed: %s", tmp_file, strerror(errno));
		return -1;
	}

	offset = (int)strlen(data->find) - 1;
	buf = xmalloc(strlen(data->find) + 1);
	for (i = 0; i < (int)(strlen(data->find)) + 1; i++) {
		buf[i] = '\0';
	}
	while ((c = getc(fp)) != EOF) {
		/* Shift to left, adding c at the end. */
		i = offset;
		while ((buf[i+1])) {
			buf[i] = buf[i+1];
			i++;
		}
		buf[i] = c;

		if (strcmp(buf + offset, data->find) == 0) {
			num_found++;
			fseek(fpw, (long)-strlen(data->find) + 1, SEEK_CUR);

			if (data->replace) {
				fprintf(fpw, "%s", data->replace);
			}

		} else {
			putc(c, fpw);
		}

		if (offset > 0)
			offset--;
	}

	xfree(buf);

	fclose(fpw);
	fclose(fp);

	Nprint_h("Made %d change%s.", num_found, num_found != 1 ? "s" : ""); /* :-) */

	if (execute_command(element, "mv -f %s %s", tmp_file, data->file)) {
		Nprint_h_err("System command for moving %s to %s failed.", tmp_file, data->file);
		return -1;
	}

	/* Changing the file mode. */
	if (chmod(data->file, file_stat.st_mode)) {
		Nprint_h_err("chmod(%s) failed: %s", data->file, strerror(errno));
		return -1;
	}

	return 0;
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "search_replace",
		.description = "Search and replace",
		.syntax_version = "2.0",
		.parameters = search_replace_parameters_v2,
		.main = search_replace_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = search_replace_setup,
		.free = search_replace_free,
		.parameter = search_replace_parameter,
		.valid_data = search_replace_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "search_replace",
		.description = "Search and replace",
		.syntax_version = "3.0",
		.parameters = search_replace_parameters_v3,
		.attributes = search_replace_attributes,
		.main = search_replace_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = search_replace_setup,
		.free = search_replace_free,
		.attribute = search_replace_attribute,
		.parameter = search_replace_parameter,
		.valid_data = search_replace_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "search_replace",
		.description = "Search and replace",
		.syntax_version = "3.1",
		.parameters = search_replace_parameters_v3,
		.attributes = search_replace_attributes,
		.main = search_replace_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.setup = search_replace_setup,
		.free = search_replace_free,
		.attribute = search_replace_attribute,
		.parameter = search_replace_parameter,
		.valid_data = search_replace_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "search_replace",
		.description = "Search and replace",
		.syntax_version = "3.2",
		.parameters = search_replace_parameters_v3,
		.attributes = search_replace_attributes,
		.main = search_replace_main,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
		.setup = search_replace_setup,
		.free = search_replace_free,
		.attribute = search_replace_attribute,
		.parameter = search_replace_parameter,
		.valid_data = search_replace_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
