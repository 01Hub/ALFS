/*
 *  log.c - Handler.
 *
 *  Copyright (C) 2002 by Maik Schreiber <bZ@iq-computing.de>
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


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME log
#include <nALFS.h>
#include "utility.h"
#include "win.h"
#include "parser.h"
#include "backend.h"


char HANDLER_SYMBOL(name)[] = "log";
char HANDLER_SYMBOL(description)[] = "Log";
char *HANDLER_SYMBOL(syntax_versions)[] = { "2.0", "3.0", NULL };
// char *HANDLER_SYMBOL(attributes)[] = { NULL };
char *HANDLER_SYMBOL(parameters)[] = { NULL };
int HANDLER_SYMBOL(action) = 0;


int HANDLER_SYMBOL(main)(element_s *el)
{
	if (el->content != NULL) {
		Nprint_h("%s", el->content);
	} else {
		Nprint_h("");
	}
	
	return 0;
}
