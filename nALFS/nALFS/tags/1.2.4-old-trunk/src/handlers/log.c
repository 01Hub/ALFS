/*
 *  log.c - Handler.
 *
 *  Copyright (C) 2002, 2004
 *
 *  Maik Schreiber <bZ@iq-computing.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME log
#include <nALFS.h>

#include "handlers.h"
#include "nprint.h"
#include "utility.h"

struct log_data {
	char *content;
};

static int log_setup(element_s * const element)
{
	struct log_data *data;

	if ((data = xmalloc(sizeof(struct log_data))) == NULL)
		return 1;

	data->content = NULL;
	element->handler_data = data;

	return 0;
}

static void log_free(const element_s * const element)
{
	struct log_data *data = (struct log_data *) element->handler_data;

	xfree(data->content);
	xfree(data);
}

static int log_content(const element_s * const element,
		       const char * const content)
{
	struct log_data *data = (struct log_data *) element->handler_data;

	if (strlen(content))
		data->content = xstrdup(content);

	return 0;
}

static int log_valid_data(const element_s * const element)
{
	struct log_data *data = (struct log_data *) element->handler_data;

	if (!data->content) {
		Nprint_err("<log>: content cannot be empty.");
		return 0;
	}

	return 1;
}

static int log_main(const element_s * const element)
{
	struct log_data *data = (struct log_data *) element->handler_data;

	Nprint_h("%s", data->content);

	return 0;
}

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "log",
		.description = "Log",
		.syntax_version = "2.0",
		.main = log_main,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 0,
		.priority = 0,
		.setup = log_setup,
		.free = log_free,
		.content = log_content,
		.valid_data = log_valid_data,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "log",
		.description = "Log",
		.syntax_version = "3.0",
		.main = log_main,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 0,
		.priority = 0,
		.setup = log_setup,
		.free = log_free,
		.content = log_content,
		.valid_data = log_valid_data,
	},
#endif
	{
		.name = NULL
	}
};
