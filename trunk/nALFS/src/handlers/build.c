/*
 *  build.c - Handler.
 *
 *  Copyright (C) 2001, 2002, 2004
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME build
#include <nALFS.h>

#include "handlers.h"
#include "backend.h"
#include "logging.h"

static int build_setup(element_s * const element)
{
	(void) element;

	return 0;
}

static int build_main(const element_s * const el)
{
	int i;

	log_start_time(el);

	i = execute_children(el);

	log_end_time(el, i);

	return i;
}


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "build",
		.description = "Build",
		.syntax_version = "2.0",
		.main = build_main,
		.type = HTYPE_NORMAL,
		.setup = build_setup,
	},
#endif
	{
		.name = NULL
	}
};
