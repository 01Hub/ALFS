/*
 *  options.h - Program's options.
 *
 *  Copyright (C) 2002
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


#ifndef H_OPTIONS_
#define H_OPTIONS_


/*
 * Types of some options.
 */

typedef enum timer_type {
	TIMER_NONE = 0,
	TIMER_TOTAL,
	TIMER_CURRENT
} timer_type_t;

typedef enum jump_location {
	JUMP_TO_FAILED = 0,
	JUMP_TO_RUNNING,
	JUMP_TO_DONE,
	JUMP_TO_PACKAGE
} jump_location_t;

/* We're looping through this enum (toggling) - 0 markes the first value. */
typedef enum logging_method {
	LOG_OFF = 0,
	LOG_USING_ONE_FIND,
} logging_method_t;

#define LAST_LOGGING_METHOD LOG_USING_ONE_FIND


/*
 * Options themselves.
 */

#define BOOL 	int
#define NUMBER 	int
#define STRING 	char *

union option_value {
	STRING str_value;
	BOOL bool_value;
	NUMBER num_value;
};

enum option_type {
	O_BOOL,
	O_NUMBER,
	O_STRING,
	O_COMMAND
};

struct option_s {
	char *name;

	enum option_type type;

	union option_value value;
	union option_value def_value;
};

#ifndef STRING_OPTION
#define STRING_OPTION(opt_name, opt_def_value) \
		extern STRING * const opt_##opt_name;
#endif /* STRING_OPTION */

#ifndef BOOL_OPTION
#define BOOL_OPTION(opt_name, opt_def_value) \
		extern BOOL * const opt_##opt_name;
#endif /* BOOL_OPTION */

#ifndef NUMBER_OPTION
#define NUMBER_OPTION(opt_name, opt_def_value) \
		extern NUMBER * const opt_##opt_name;
#endif /* NUMBER_OPTION */

#ifndef COMMAND_OPTION
#define COMMAND_OPTION(opt_name, opt_def_value) \
		extern STRING * const opt_##opt_name;
#endif /* COMMAND_OPTION */


STRING_OPTION(alfs_directory,"")
BOOL_OPTION(start_immediately,0)
STRING_OPTION(packages_directory,"packages")
STRING_OPTION(default_syntax,"3.0")
STRING_OPTION(cursor,"->")
NUMBER_OPTION(indentation_size,4)
NUMBER_OPTION(sleep_after_stamp,1)
BOOL_OPTION(beep_when_done,0)
BOOL_OPTION(display_profile,1)
BOOL_OPTION(display_options_line,1)
NUMBER_OPTION(display_timer,TIMER_TOTAL)
BOOL_OPTION(expand_profiles,0)
BOOL_OPTION(use_relative_dirs,0)
BOOL_OPTION(log_status_window,0)
STRING_OPTION(status_logfile,"log_file")
BOOL_OPTION(show_system_output,1)
BOOL_OPTION(be_verbose,0)
BOOL_OPTION(display_alfs,0)
BOOL_OPTION(display_doctype,0)
BOOL_OPTION(display_comments,0)
BOOL_OPTION(run_interactive,1)
NUMBER_OPTION(jumpto_element,JUMP_TO_RUNNING)
NUMBER_OPTION(logging_method,LOG_OFF)
BOOL_OPTION(log_handlers,1)
BOOL_OPTION(log_backend,1)
BOOL_OPTION(stamp_packages,0)
STRING_OPTION(stamp_directory,"stamps")
BOOL_OPTION(display_stage_header,0)
STRING_OPTION(find_base,"/")
STRING_OPTION(find_prunes,"")
STRING_OPTION(find_prunes_file,"")
STRING_OPTION(profiles_directory,"/")
BOOL_OPTION(warn_if_set,0)
BOOL_OPTION(follow_running,0)
STRING_OPTION(warn_if_set_variables,"CPPFLAGS CXXFLAGS CFLAGS LDFLAGS")
BOOL_OPTION(print_startup_help,1)
NUMBER_OPTION(windows_relation,50)
NUMBER_OPTION(status_history,500)
STRING_OPTION(editor,"")
COMMAND_OPTION(bunzip2_command,"bunzip2 -dc %s")
COMMAND_OPTION(gunzip_command,"zcat %s")
COMMAND_OPTION(uncompress_command,"zcat %s")
COMMAND_OPTION(untar_command,"tar xv")
COMMAND_OPTION(unpax_command,"pax -rv")
COMMAND_OPTION(uncpio_command,"cpio -idv")
COMMAND_OPTION(unzip_command,"unzip %s")


void set_string_option(STRING * const var, const STRING value);
void append_string_option(STRING * const var, const STRING value);
void set_options_to_defaults(void);

char *alloc_real_status_logfile_name(void);
char *alloc_real_packages_directory_name(void);
char *alloc_real_stamp_directory_name(void);

/*
 * Setting option from RC file parser.
 */

#define BOOL_TRUE_VALUE "yes"
#define BOOL_FALSE_VALUE "no"

typedef enum set_opt_e {
	OPTION_SET,
	OPTION_UNKNOWN,
	OPTION_INVALID_VALUE
} set_opt_e;

set_opt_e set_yet_unknown_option(const char *opt, const char *val);


#endif /* H_OPTIONS_ */
