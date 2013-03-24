/*
 *  alfs.c - Handler.
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


#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME alfs
#include <nALFS.h>

#include "handlers.h"
#include "backend.h"


static const char *alfs_parameters[] = { NULL };

static int alfs_main(element_s *el)
{
	return execute_children(el);
}


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "2.0",
		.parameters = alfs_parameters,
		.main = alfs_main,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 0,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "3.0",
		.parameters = alfs_parameters,
		.main = alfs_main,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 0,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "alfs",
		.description = "ALFS profile",
		.syntax_version = "3.1",
		.parameters = alfs_parameters,
		.main = alfs_main,
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
