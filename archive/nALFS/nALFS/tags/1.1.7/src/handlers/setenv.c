/*
 *  setenv.c - Handler.
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
#include <unistd.h>
#include <errno.h>

#include "win.h"
#include "parser.h"
#include "utility.h"
#include "config.h"


#define El_setenv_value(el) raw_param_value("value", el)
#define El_setenv_variable(el) alloc_trimmed_param_value("variable", el)
#define El_setenv_mode(el) attr_value("mode", el)


static INLINE int set_variable(const char *variable, const char *value)
{
	Nprint_h("Setting environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		return -1;
	}

	return 0;
}

static INLINE int append_to_variable(const char *variable, const char *value)
{
	int status = 0;
	char *old_value, *full_value = NULL;
       

	if ((old_value = getenv(variable))) {
		full_value = xstrdup(old_value);
	}
	append_str(&full_value, value);

	Nprint_h("Appending to environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, full_value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		status = -1;
	}

	xfree(full_value);

	return status;
}

static INLINE void unset_variable(const char *variable)
{
	Nprint_h("Unsetting environment variable %s.", variable);

	unsetenv(variable);
}

static INLINE int do_setenv(
	const char *variable, const char *value, const char *mode)
{
	if (value == NULL) {
		unset_variable(variable);

	} else {
		if (mode && strcmp(mode, "append") == 0) {
			return append_to_variable(variable, value);
		} else {
			return set_variable(variable, value);
		}
	}

	return 0;
}


char handler_name[] = "setenv";
char handler_description[] = "Set environment";
char *handler_syntax_versions[] = { "2.0", "3.0", NULL };
// char *handler_attributes[] = { "mode", NULL };
char *handler_parameters[] = { "variable", "value", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int i;
	char *variable;
	char *value;
	char *mode;


	if ((variable = El_setenv_variable(el)) == NULL) {
		Nprint_h_err("No variable specified.");
		return -1;
	}

	value = El_setenv_value(el);
	mode = El_setenv_mode(el);

	i = do_setenv(variable, value, mode);

	xfree(variable);

	return i;
}
