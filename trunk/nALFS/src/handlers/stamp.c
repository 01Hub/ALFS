/*
 *  stamp.c - Handler.
 * 
 *  Copyright (C) 2002 by Vassili Dzuba <vassilidzuba@nerim.net>
 *
 *  Parts Copyright (C) 2001, 2002 by Neven Has <haski@sezampro.yu>
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

#define MODULE_NAME stamp
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "win.h"
#include "parser.h"
#include "backend.h"


static int stamp_main(element_s *el)
{
	int status = 0;
	char *name;
	char *version;

	if ((name = attr_value("name", el)) == NULL) {
		Nprint_h_err("No name specified.");
		return -1;
	}

	if ((version = attr_value("version", el)) == NULL) {
		Nprint_h_err("No version specified.");
		return -1;
	}

	status = stamp_package_installed(1, name, version);

	return status;
}


/*
 * Handlers' information.
 */

static const char *stamp_parameters[] = { NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "name", "version" };

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "stamp",
		.description = "Produce a stamp",
		.syntax_version = "3.0",
		.parameters = stamp_parameters,
		.main = stamp_main,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 0,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
