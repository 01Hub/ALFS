/*
 *  options.c - Program's options.
 *
 *  Copyright (C) 2002-2003
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


#include <string.h>

#define C_FILE
#include "options.h"

#include "nalfs.h"
#include "logging.h"
#include "utility.h"
#include "config.h"


static option_s options[] = {
	{
		"alfs_directory", O_STRING,
		(option_pointer *)& opt_alfs_directory,
		(option_pointer *)& ""
	},{
		"start_immediately", O_BOOL,
		(option_pointer *)& opt_start_immediately,
		(option_pointer *)  0
	},{
		"packages_directory", O_STRING,
		(option_pointer *)& opt_packages_directory,
		(option_pointer *)& "packages"
	},{
		"default_syntax", O_STRING,
		(option_pointer *)& opt_default_syntax,
		(option_pointer *)& "3.0"
	},{
		"cursor", O_STRING,
		(option_pointer *)& opt_cursor_string,
		(option_pointer *)& "->"
	},{
		"indentation_size", O_NUMBER,
		(option_pointer *)& opt_indentation_size,
		(option_pointer *)  4
	},{
		"sleep_after_stamp", O_NUMBER,
		(option_pointer *)& opt_sleep_after_stamp,
		(option_pointer *)  1
	},{
		"beep_when_done", O_BOOL,
		(option_pointer *)& opt_beep_when_done,
		(option_pointer *)  0
	},{
		"display_profile", O_BOOL,
		(option_pointer *)& opt_display_profile,
		(option_pointer *)  1
	},{
		"display_options_line", O_BOOL,
		(option_pointer *)& opt_display_options_line,
		(option_pointer *)  1
	},{
		"display_timer", O_NUMBER,
		(option_pointer *)& opt_display_timer,
		(option_pointer *)  TIMER_TOTAL
	},{
		"expand_profiles", O_BOOL,
		(option_pointer *)& opt_expand_profiles,
		(option_pointer *)  0
	},{
		"use_relative_dirs", O_BOOL,
		(option_pointer *)& opt_use_relative_dirs,
		(option_pointer *)  0
	},{
		"log_status_window", O_BOOL,
		(option_pointer *)& opt_log_status_window,
		(option_pointer *)  0
	},{
		"status_logfile", O_STRING,
		(option_pointer *)& opt_status_logfile,
		(option_pointer *)& "log_file"
	},{
		"show_system_output", O_BOOL,
		(option_pointer *)& opt_show_system_output,
		(option_pointer *)  1
	},{
		"be_verbose", O_BOOL,
		(option_pointer *)& opt_be_verbose,
		(option_pointer *)  0
	},{
		"display_alfs", O_BOOL,
		(option_pointer *)& opt_display_alfs,
		(option_pointer *)  0
	},{
		"display_doctype", O_BOOL,
		(option_pointer *)& opt_display_doctype,
		(option_pointer *)  0
	},{
		"display_comments", O_BOOL,
		(option_pointer *)& opt_display_comments,
		(option_pointer *)  0
	},{
		"run_interactive", O_BOOL,
		(option_pointer *)& opt_run_interactive,
		(option_pointer *)  1
	},{
		"jumpto_element", O_NUMBER,
		(option_pointer *)& opt_jumpto_element,
		(option_pointer *)  JUMP_TO_RUNNING
	},{
		"logging_method", O_NUMBER,
		(option_pointer *)& opt_logging_method,
		(option_pointer *)  LOG_OFF
	},{
		"log_handlers", O_BOOL,
		(option_pointer *)& opt_log_handlers,
		(option_pointer *)  1
	},{
		"log_backend", O_BOOL,
		(option_pointer *)& opt_log_backend,
		(option_pointer *)  1
	},{
		"stamp_packages", O_BOOL,
		(option_pointer *)& opt_stamp_packages,
		(option_pointer *)  0
	},{
		"stamp_directory", O_STRING,
		(option_pointer *)& opt_stamp_directory,
		(option_pointer *)& "stamps"
	},{
		"display_stage_header", O_BOOL,
		(option_pointer *)& opt_display_stage_header,
		(option_pointer *)  0
	},{
		"find_base", O_STRING,
		(option_pointer *)& opt_find_base,
		(option_pointer *)& "/"
	},{
		"find_prunes", O_STRING,
		(option_pointer *)& opt_find_prunes,
		(option_pointer *)& ""
	},{
		"find_prunes_file", O_STRING,
		(option_pointer *)& opt_find_prunes_file,
		(option_pointer *)& ""
	},{
		"profiles_directory", O_STRING,
		(option_pointer *)& opt_profiles_directory,
		(option_pointer *)& "/" 
	},{
		"warn_if_set", O_BOOL,
		(option_pointer *)& opt_warn_if_set,
		(option_pointer *)  0
	},{
		"follow_running", O_BOOL,
		(option_pointer *)& opt_follow_running,
		(option_pointer *)  0
	},{
		"warn_if_set_variables", O_STRING,
		(option_pointer *)& opt_warn_if_set_variables,
		(option_pointer *)& "CPPFLAGS CXXFLAGS CFLAGS LDFLAGS"
	},{
		"print_startup_help", O_BOOL,
		(option_pointer *)& opt_print_startup_help,
		(option_pointer *)  1
	},{
		"windows_relation", O_NUMBER,
		(option_pointer *)& opt_windows_relation,
		(option_pointer *)  50
	},{
		"status_history", O_NUMBER,
		(option_pointer *)& opt_status_history,
		(option_pointer *)  500
	},{
		"editor", O_STRING,
		(option_pointer *)& opt_editor,
		(option_pointer *)& ""
	},{
		NULL, 0, NULL, NULL
	}
};



static void set_number_option(int idx, NUMBER value)
{
	option_pointer **var_pointer;


	var_pointer = (option_pointer **)(options[idx].var);

	*var_pointer = (option_pointer *)value;
}

static void set_string_option(int idx, const char *value)
{
	option_pointer **var_pointer;


	ASSERT(value != NULL);

	var_pointer = (option_pointer **)(options[idx].var);

	/*
	 * Some options need special treatment when they are set.
	 */

	if (var_pointer == (option_pointer **) &opt_find_prunes) {
		/* Append the value, instead of overwriting the existing. */

		char *new_value = NULL;

		if (opt_find_prunes && strlen(opt_find_prunes) > 0) {
			append_str(&new_value, opt_find_prunes);
			append_str(&new_value, " ");
		}

		append_str(&new_value, value);

		xfree(*var_pointer);

		*var_pointer = (option_pointer *)new_value;

	} else {
		xfree(*var_pointer);

		*var_pointer = (option_pointer *)xstrdup(value);
	}
}

/*
 * Sets the option to the specified value.
 */
void set_option(option_pointer **var_pointer, NUMBER number, char *string)
{
	int i;


	for (i = 0; options[i].name; i++) {
		if (var_pointer != (option_pointer **)options[i].var) {
			continue;
		}

		switch (options[i].type) {
			case O_BOOL:
			case O_NUMBER:
				set_number_option(i, number);
				break;

			case O_STRING:
				set_string_option(i, string);
				break;
		}
	}
}

/*
 * Sets an option to the specified value.
 */
static void set_option_from_index(int idx, option_pointer *value)
{
	switch (options[idx].type) {
		case O_BOOL:
		case O_NUMBER:
			set_number_option(idx, (NUMBER)value);
			break;

		case O_STRING:
			set_string_option(idx, (char *)value);
			break;
	}
}

/*
 * Sets all options to their default values.
 */
void set_options_to_defaults(void)
{
	int i;

	for (i = 0; options[i].name; i++) {
		set_option_from_index(i, options[i].def_value);
	}
}

/*
 * Sets an option to its default value.
 */
void set_option_to_default(option_pointer **var_pointer)
{
	int i;

	for (i = 0; options[i].name; i++) {
		if (var_pointer == (option_pointer **)options[i].var) {
			set_option_from_index(i, options[i].def_value);
		}
	}
}

/*
 *
 */

char *alloc_real_status_logfile_name(void)
{
	char *s;

	if (opt_status_logfile[0] == '/') {
		return xstrdup(opt_status_logfile);
	}

	s = xstrdup(opt_alfs_directory);
	append_str(&s, "/");
	append_str(&s, opt_status_logfile);

	return s;
}

char *alloc_real_packages_directory_name(void)
{
	char *s;

	if (opt_packages_directory[0] == '/') {
		return xstrdup(opt_packages_directory);
	}

	s = xstrdup(opt_alfs_directory);
	append_str(&s, "/");
	append_str(&s, opt_packages_directory);

	return s;
}

char *alloc_real_stamp_directory_name(void)
{
	char *s;

	if (opt_stamp_directory[0] == '/') {
		return xstrdup(opt_stamp_directory);
	}

	s = xstrdup(opt_alfs_directory);
	append_str(&s, "/");
	append_str(&s, opt_stamp_directory);

	return s;
}

static INLINE int not_correct_number(int idx, NUMBER num)
{
	if (options[idx].var == (option_pointer *)&opt_display_timer) {
		if (num < TIMER_NONE || num > TIMER_CURRENT) {
			return 1;
		}

	} else if (options[idx].var == (option_pointer *)&opt_jumpto_element) {
		if (num < JUMP_TO_FAILED || num > JUMP_TO_PACKAGE) {
			return 1;
		}

	} else if (options[idx].var == (option_pointer *)&opt_logging_method) {
		if (num < 0 || num > LAST_LOGGING_METHOD) {
			return 1;
		}
	}

	return 0;
}

set_opt_e set_yet_unknown_option(const char *opt, const char *val)
{
	int i;
	long num;
	char *s = NULL;


	for (i = 0; options[i].name; i++) {
		if (strcmp(options[i].name, opt) != 0) {
			continue;
		}

		switch (options[i].type) {
			case O_BOOL:
				if (strcmp(val, BOOL_TRUE_VALUE) == 0) {
					set_number_option(i, 1);
					return OPTION_SET;

				} else if (strcmp(val, BOOL_FALSE_VALUE) == 0) {
					set_number_option(i, 0);
					return OPTION_SET;
				}

				return OPTION_INVALID_VALUE;

			case O_NUMBER:
				num = strtol(val, &s, 10);

				if (s != NULL && *s) {
					return OPTION_INVALID_VALUE;
				}

				if (not_correct_number(i, num)) {
					return OPTION_INVALID_VALUE;
				}

				set_number_option(i, num);

				return OPTION_SET;

			case O_STRING:
				set_string_option(i, val);

				return OPTION_SET;
		}
	}

	return OPTION_UNKNOWN;
}
