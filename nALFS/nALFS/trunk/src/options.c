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

#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "nalfs-core.h"
#include "logging.h"
#include "utility.h"
#include "nprint.h"
#include "bufsize.h"

/* These macro definitions will cause the options in options.h to actually
   be allocated storage in this module.
*/

#define STRING_OPTION(opt_name, opt_def_value, opt_validate, \
		      opt_post_validate) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_STRING, \
			.val = { \
				.u_str = { .value = NULL, \
					   .def_value = opt_def_value, \
					   .validate = opt_validate, \
					   .post_validate = opt_post_validate } \
				} \
		}; \
		STRING * const opt_##opt_name = &real_opt_##opt_name .val.u_str.value
#define BOOL_OPTION(opt_name, opt_def_value, opt_validate, \
		    opt_post_validate) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_BOOL, \
			.val = { \
				.u_bool = { .value = 0, \
					    .def_value = opt_def_value, \
					    .validate = opt_validate, \
					    .post_validate = opt_post_validate } \
				} \
		}; \
		BOOL * const opt_##opt_name = &real_opt_##opt_name .val.u_bool.value
#define NUMBER_OPTION(opt_name, opt_def_value, opt_min_value, opt_max_value, \
		      opt_validate, opt_post_validate) \
		static struct option_s real_opt_##opt_name = { \
			.name = #opt_name, \
			.type = O_NUMBER, \
			.val = { \
				.u_num = { .value = 0, \
					   .def_value = opt_def_value, \
					   .min_value = opt_min_value, \
					   .max_value = opt_max_value, \
					   .validate = opt_validate, \
					   .post_validate = opt_post_validate } \
				} \
		}; \
		NUMBER * const opt_##opt_name = &real_opt_##opt_name .val.u_num.value

#include "option-struct.h"

static int validate_command(const struct option_s *option, const STRING value);
static int validate_number_minmax(const struct option_s *option, const NUMBER value);

#include "options.h"
#include "option-array.h"


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
		option->val.u_bool.value = option->val.u_bool.def_value;
		break;
		
	case O_NUMBER:
		option->val.u_num.value = option->val.u_num.def_value;
		break;
		
	case O_STRING:
		if (option->val.u_str.value)
			xfree(option->val.u_str.value);
		option->val.u_str.value = xstrdup(option->val.u_str.def_value);
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

static void option_invalid(const struct option_s *option,
			   const char *format, ...)
{
	va_list ap;
	char buffer[MAX_ERROR_MSG_LEN];

	va_start(ap, format);
	if (vsnprintf(buffer, sizeof buffer, format, ap) > 0)
		Nprint_err("Option \"%s\" invalid value: %s",
			   option->name, buffer);
	else
		Nprint_err("Option \"%s\" invalid value.", option->name);
	va_end(ap);
}

static int validate_number_minmax(const struct option_s *option,
				  const NUMBER value)
{
	int valid;

	valid = (value >= option->val.u_num.min_value &&
		 value <= option->val.u_num.max_value);

	if (!valid)
		option_invalid(option, "%d\n\tmust be between %d and %d",
			       value,
			       option->val.u_num.min_value,
			       option->val.u_num.max_value);

	return valid;
}

static int validate_command(const struct option_s *option, const STRING value)
{
	const char *tmp;
	int string_count = 0;

	for (tmp = value; *tmp; ++tmp) {
		if (*tmp != '%')
			continue;
		switch (*(++tmp)) {
		case '%':
			break;
		case 's':
			if (string_count++) {
				option_invalid(option,
					       "%s\n\tonly one string substitution allowed", value);
				return 0;
			}
			break;
		default:
			option_invalid(option,
				       "%s\n\tinvalid substitution specified",
				       value);
			return 0;
		}
	}

	return 1;
}

static int validate_boolean_input(const struct option_s *option,
				  const char *input, BOOL *val)
{
	char *p;
	char *tmp;
	int status = 1;

	p = tmp = xstrdup(input);
	while (*p) {
		*p = tolower(*p);
		++p;
	}

	if (!strcmp(tmp, "yes"))
		*val = 1;
	else if (!strcmp(tmp, "y"))
		*val = 1;
	else if (!strcmp(tmp, "true"))
		*val = 1;
	else if (!strcmp(tmp, "no"))
		*val = 0;
	else if (!strcmp(tmp, "n"))
		*val = 0;
	else if (!strcmp(tmp, "false"))
		*val = 0;
	else {
		option_invalid(option,
			       "%s\n\tvalid choices are yes/y/true or no/n/false.",
			       input);
		status = 0;
	}

	xfree(tmp);
	return status;
}

static int validate_number_input(const struct option_s *option,
				 const char *input, NUMBER *val)
{
	char *tmp;
	int status = 1;

	errno = 0;
	*val = (NUMBER) strtol(input, &tmp, 10);
			
	if (tmp != NULL && *tmp) {
		option_invalid(option,
			       "%s\n\textraneous characters found (%s).",
			       input, tmp);
		status = 0;
	} else if (errno) {
		option_invalid(option,
			       "%s\n\tnumeric conversion failure.",
			       input);
		status = 0;
	}

	return status;
			
}

set_opt_e set_yet_unknown_option(const char *opt, const char *val)
{
	int i;
	NUMBER num;

	for (i = 0; options[i]; i++) {
		if (strcmp(options[i]->name, opt) != 0) {
			continue;
		}

		switch (options[i]->type) {
		case O_BOOL:
			if (!validate_boolean_input(options[i], val,
						   &options[i]->val.u_bool.value))
				return OPTION_INVALID_VALUE;
			
			return OPTION_SET;
			
		case O_NUMBER:
			if (!validate_number_input(options[i], val, &num))
				return OPTION_INVALID_VALUE;
			
			if (options[i]->val.u_num.validate &&
			    !options[i]->val.u_num.validate(options[i], num))
				return OPTION_INVALID_VALUE;
			
			options[i]->val.u_num.value = num;
			
			return OPTION_SET;
			
		case O_STRING:
			if (options[i]->val.u_str.validate &&
			    !options[i]->val.u_str.validate(options[i], val))
				return OPTION_INVALID_VALUE;
			
			set_string_option(&options[i]->val.u_str.value, val);
			
			return OPTION_SET;
		}
	}

	return OPTION_UNKNOWN;
}

/*
 * Executes all "post-validation" functions for options.
 */
void post_validate_options(void)
{
	int i;

	for (i = 0; options[i]; i++) {
		switch (options[i]->type) {
		case O_BOOL:
			if (options[i]->val.u_bool.post_validate)
				options[i]->val.u_bool.post_validate(options[i]);
			break;
		case O_NUMBER:
			if (options[i]->val.u_num.post_validate)
				options[i]->val.u_num.post_validate(options[i]);
			break;
		case O_STRING:
			if (options[i]->val.u_str.post_validate)
				options[i]->val.u_str.post_validate(options[i]);
			break;
		}
	}
}

