/*
 *  package.c - Handler.
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
#include <string.h>
#include <time.h>

#include "logging.h"
#include "utility.h"
#include "parser.h"
#include "handlers.h"
#include "win.h"
#include "backend.h"


char handler_name[] = "package";
char handler_description[] = "Package";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] = { "name", "version", "base", NULL };
int handler_action = 0;


int handler_main(element_s *el)
{
	int i;

	start_logging_element(el);
	log_start_time(el);

	i = execute_children(el);

	log_end_time(el, i);
	end_logging_element(el, i);

	return i;
}

char *handler_alloc_package_name(element_s *el)
{
	return alloc_trimmed_param_value("name", el);
}

char *handler_alloc_package_version(element_s *el)
{
	return alloc_trimmed_param_value("version", el);
}
