/*
 *  options.c - Program's options.
 *
 *  Copyright (C) 2002-2003
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


#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "nalfs-core.h"
#include "logging.h"
#include "utility.h"

/* These macro definitions will cause the options in options.h to actually
   be allocated storage in this module.
*/

#define STRING_OPTION(opt_name, opt_def_value, opt_other...) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_STRING, \
			.val.str = { .def_value = opt_def_value, \
				     ## opt_other } \
		}; \
		STRING * const opt_##opt_name = &real_opt_##opt_name .val.str.value
#define BOOL_OPTION(opt_name, opt_def_value, opt_other...) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_BOOL, \
			.val.bool = { .def_value = opt_def_value, \
				      ## opt_other } \
		}; \
		BOOL * const opt_##opt_name = &real_opt_##opt_name .val.bool.value
#define NUMBER_OPTION(opt_name, opt_def_value, opt_other...) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_NUMBER, \
			.val.num = { .def_value = opt_def_value, \
				     ## opt_other } \
		}; \
		NUMBER * const opt_##opt_name = &real_opt_##opt_name .val.num.value


#include "options.h"
#include "option-list.h"


void set_string_option(STRING * const var, const char *value)
{
	ASSERT(var != NULL);
	ASSERT(value != NULL);

	xfree(*var);
	*var = xstrdup(value);
}

void append_string_option(STRING * const var, const char *value)
{
	char *new_value = NULL;

	ASSERT(var != NULL);
	ASSERT(value != NULL);

	if (*var && strlen(*var) > 0) {
		append_str(&new_value, *var);
		append_str(&new_value, " ");
	}

	append_str(&new_value, value);

	xfree(*var);
	*var = new_value;
}

/*
 * Sets an option to its default value.
 */
static void set_option_to_default(struct option_s *option)
{
	switch (option->type) {
	case O_BOOL:
		option->val.bool.value = option->val.bool.def_value;
		break;
		
	case O_NUMBER:
		option->val.num.value = option->val.num.def_value;
		break;
		
	case O_STRING:
		if (option->val.str.value)
			xfree(option->val.str.value);
		option->val.str.value = xstrdup(option->val.str.def_value);
		break;
	}
}

/*
 * Sets all options to their default values.
 */
void set_options_to_defaults(void)
{
	int i;

	for (i = 0; options[i]; i++) {
		set_option_to_default(options[i]);
	}
}

/*
 *
 */

char *alloc_real_status_logfile_name(void)
{
	char *s;

	if (*opt_status_logfile[0] == '/') {
		return xstrdup(*opt_status_logfile);
	}

	s = xstrdup(*opt_alfs_directory);
	append_str(&s, "/");
	append_str(&s, *opt_status_logfile);

	return s;
}

char *alloc_real_packages_directory_name(void)
{
	char *s;

	if (*opt_packages_directory[0] == '/') {
		return xstrdup(*opt_packages_directory);
	}

	s = xstrdup(*opt_alfs_directory);
	append_str(&s, "/");
	append_str(&s, *opt_packages_directory);

	return s;
}

char *alloc_real_stamp_directory_name(void)
{
	char *s;

	if (*opt_stamp_directory[0] == '/') {
		return xstrdup(*opt_stamp_directory);
	}

	s = xstrdup(*opt_alfs_directory);
	append_str(&s, "/");
	append_str(&s, *opt_stamp_directory);

	return s;
}

static int validate_number_minmax(const struct option_s *option,
				  const NUMBER value)
{
	int valid;

	valid = (value >= option->val.num.min_value &&
		 value <= option->val.num.max_value);

	if (!valid)
		fprintf(stderr,
			"Option %s outside valid range, must be between"
			" %d and %d\n", option->name,
			option->val.num.min_value,
			option->val.num.max_value);

	return valid;
}

static int validate_command(const struct option_s *option, const STRING value)
{
	const char *tmp;
	int string_count = 0;

	for (tmp = value; *tmp; ++tmp) {
		if (*tmp == '%') {
			switch (*(++tmp)) {
			case '%':
				break;
			case 's':
				if (string_count++) {
					fprintf(stderr, "Option %s contains"
						" more than one string"
						" substitution\n",
						option->name);
					return 0;
				}
				break;
			default:
				fprintf(stderr, "Option %s contains an"
					" invalid substitution specifier\n",
					option->name);
				return 0;
			}
		}
	}

	return 1;
}

set_opt_e set_yet_unknown_option(const char *opt, const char *val)
{
	int i;
	long num;
	char *s = NULL;


	for (i = 0; options[i]; i++) {
		if (strcmp(options[i]->name, opt) != 0) {
			continue;
		}

		switch (options[i]->type) {
		case O_BOOL:
			if (strcmp(val, BOOL_TRUE_VALUE) == 0) {
				options[i]->val.bool.value = 1;
				return OPTION_SET;
				
			} else if (strcmp(val, BOOL_FALSE_VALUE) == 0) {
				options[i]->val.bool.value = 0;
				return OPTION_SET;
			}
			
			return OPTION_INVALID_VALUE;
			
		case O_NUMBER:
			num = strtol(val, &s, 10);
			
			if (s != NULL && *s)
				return OPTION_INVALID_VALUE;
			
			if (options[i]->val.num.validate &&
			    !options[i]->val.num.validate(options[i], num))
				return OPTION_INVALID_VALUE;
			
			options[i]->val.num.value = num;
			
			return OPTION_SET;
			
		case O_STRING:
			if (options[i]->val.str.validate &&
			    !options[i]->val.str.validate(options[i], val))
				return OPTION_INVALID_VALUE;
			
			set_string_option(&options[i]->val.str.value, val);
			
			return OPTION_SET;
		}
	}

	return OPTION_UNKNOWN;
}
