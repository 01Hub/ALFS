/*
 *  prebuild.c - Handler.
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

#define MODULE_NAME prebuild
#include <nALFS.h>
#include "backend.h"
#include "logging.h"


char HANDLER_SYMBOL(name)[] = "prebuild";
char HANDLER_SYMBOL(description)[] = "Pre building";
char *HANDLER_SYMBOL(syntax_versions)[] = { "2.0", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { NULL };
char *HANDLER_SYMBOL(parameters)[] = { NULL };
int HANDLER_SYMBOL(action) = 0;


int HANDLER_SYMBOL(main)(element_s *el)
{
	int i;

	log_start_time(el);

	i = execute_children(el);

	log_end_time(el, i);

	return i;
}
