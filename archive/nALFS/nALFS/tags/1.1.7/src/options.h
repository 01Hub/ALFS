/*
 *  options.h - Program's options.
 *
 *  Copyright (C) 2002
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
	LOG_USING_TWO_FINDS
} logging_method_t;

#define LAST_LOGGING_METHOD LOG_USING_TWO_FINDS


/*
 * Options themselves.
 */

typedef unsigned char option_pointer;

enum option_type {
	O_BOOL,
	O_NUMBER,
	O_STRING
};

typedef struct option_s {
	char *name;

	enum option_type type;

	option_pointer *var;

	option_pointer *def_value;
} option_s;




#ifdef C_FILE_
#define EXTERN
#else
#define EXTERN extern
#endif


#undef INIT
#undef EXTERN

#ifdef C_FILE
# define INIT(x) x
# define EXTERN
#else
# define INIT(x)
# define EXTERN extern
#endif


#define BOOL 	int
#define NUMBER 	int
#define STRING 	char *

EXTERN BOOL opt_start_immediately INIT(= 0);
EXTERN STRING opt_alfs_directory INIT(= NULL);
EXTERN STRING opt_packages_directory INIT(= NULL);
EXTERN STRING opt_default_syntax INIT(= NULL);
EXTERN STRING opt_cursor_string INIT(= NULL);
EXTERN NUMBER opt_indentation_size INIT(= 0);
EXTERN NUMBER opt_sleep_after_stamp INIT(= 0);
EXTERN BOOL opt_beep_when_done INIT(= 0);
EXTERN BOOL opt_display_profile INIT(= 0);
EXTERN BOOL opt_display_options_line INIT(= 0);
EXTERN BOOL opt_display_timer INIT(= 0);
EXTERN BOOL opt_expand_profiles INIT(= 0);
EXTERN BOOL opt_use_relative_dirs INIT(= 0);
EXTERN BOOL opt_log_status_window INIT(= 0);
EXTERN STRING opt_status_logfile INIT(= NULL);
EXTERN BOOL opt_show_system_output INIT(= 0);
EXTERN BOOL opt_be_verbose INIT(= 0);
EXTERN BOOL opt_display_alfs INIT(= 0);
EXTERN BOOL opt_display_doctype INIT(= 0);
EXTERN BOOL opt_display_comments INIT(= 0);
EXTERN BOOL opt_run_interactive INIT(= 0);
EXTERN NUMBER opt_jumpto_element INIT(= 0);
EXTERN NUMBER opt_logging_method INIT(= 0);
EXTERN BOOL opt_log_handlers INIT(= 0);
EXTERN BOOL opt_log_backend INIT(= 0);
EXTERN BOOL opt_stamp_packages INIT(= 0);
EXTERN STRING opt_stamp_directory INIT(= NULL);
EXTERN BOOL opt_display_stage_header INIT(= 0);
EXTERN STRING opt_find_base INIT(= NULL);
EXTERN STRING opt_find_prunes INIT(= NULL);
EXTERN STRING opt_find_prunes_file INIT(= NULL);
EXTERN STRING opt_profiles_directory INIT(= NULL);
EXTERN BOOL opt_warn_if_set INIT(= 0);
EXTERN NUMBER opt_follow_running INIT(= 0);
EXTERN STRING opt_warn_if_set_variables INIT(= NULL);
EXTERN BOOL opt_print_startup_help INIT(= 0);
EXTERN NUMBER opt_windows_relation INIT(= 0);
EXTERN NUMBER opt_status_history INIT(= 0);
EXTERN STRING opt_editor INIT(= NULL);


void set_option(option_pointer **var_pointer, NUMBER number, char *string);
#define Set_string_option(a,b) set_option((option_pointer **)&a, 0, b);

void set_options_to_defaults(void);
void set_option_to_default(option_pointer **var_pointer);

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


#endif
