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

#define STRING_OPTION(opt_name, opt_def_value) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_STRING, \
			.def_value.str_value = opt_def_value \
		}; \
		STRING *opt_##opt_name = &real_opt_##opt_name .value.str_value;
#define BOOL_OPTION(opt_name, opt_def_value) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_BOOL, \
			.def_value.bool_value = opt_def_value \
		}; \
		BOOL *opt_##opt_name = &real_opt_##opt_name .value.bool_value;
#define NUMBER_OPTION(opt_name, opt_def_value) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_NUMBER, \
			.def_value.num_value = opt_def_value \
		}; \
		NUMBER *opt_##opt_name = &real_opt_##opt_name .value.num_value;
#define COMMAND_OPTION(opt_name, opt_def_value) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_COMMAND, \
			.def_value.str_value = opt_def_value \
		}; \
		STRING *opt_##opt_name = &real_opt_##opt_name .value.str_value;

#include "options.h"
#include "option-list.h"


void set_string_option(STRING *var, const char *value)
{

	ASSERT(var != NULL);
	ASSERT(value != NULL);

	/*
	 * Some options need special treatment when they are set.
	 */

	if (var == opt_find_prunes) {
		/* Append the value, instead of overwriting the existing. */

		char *new_value = NULL;

		if (*var && strlen(*var) > 0) {
			append_str(&new_value, *var);
			append_str(&new_value, " ");
		}

		append_str(&new_value, value);

		xfree(*var);

		*var = new_value;

	} else {
		xfree(*var);

		*var = xstrdup(value);
	}
}

/*
 * Sets an option to its default value.
 */
static void set_option_to_default(struct option_s *option)
{
	switch (option->type) {
	case O_BOOL:
		option->value.bool_value = option->def_value.bool_value;
		break;
		
	case O_NUMBER:
		option->value.num_value = option->def_value.num_value;
		break;
		
	case O_STRING:
	case O_COMMAND:
		if (option->value.str_value)
			xfree(option->value.str_value);
		option->value.str_value = xstrdup(option->def_value.str_value);
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

static INLINE int not_correct_number(const struct option_s *option, NUMBER num)
{
	if (option == &real_opt_display_timer) {
		if (num < TIMER_NONE || num > TIMER_CURRENT) {
			return 1;
		}

	} else if (option == &real_opt_jumpto_element) {
		if (num < JUMP_TO_FAILED || num > JUMP_TO_PACKAGE) {
			return 1;
		}

	} else if (option == &real_opt_logging_method) {
		if (num < 0 || num > LAST_LOGGING_METHOD) {
			return 1;
		}
	}

	return 0;
}

static int not_valid_command(const char *command)
{
	const char *tmp;
	int string_count = 0;

	for (tmp = command; *tmp; ++tmp) {
		if (*tmp == '%') {
			switch (*(++tmp)) {
			case '%':
				break;
			case 's':
				if (string_count++)
					return 1;
				break;
			default:
				return 1;
			}
		}
	}

	return 0;
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
					options[i]->value.bool_value = 1;
					return OPTION_SET;

				} else if (strcmp(val, BOOL_FALSE_VALUE) == 0) {
					options[i]->value.bool_value = 0;
					return OPTION_SET;
				}

				return OPTION_INVALID_VALUE;

			case O_NUMBER:
				num = strtol(val, &s, 10);

				if (s != NULL && *s)
					return OPTION_INVALID_VALUE;

				if (not_correct_number(options[i], num))
					return OPTION_INVALID_VALUE;

				options[i]->value.num_value = num;

				return OPTION_SET;

			case O_STRING:
				set_string_option(&options[i]->value.str_value, val);

				return OPTION_SET;

			case O_COMMAND:
				if (not_valid_command(val))
					return OPTION_INVALID_VALUE;

				set_string_option(&options[i]->value.str_value, val);

				return OPTION_SET;
		}
	}

	return OPTION_UNKNOWN;
}
