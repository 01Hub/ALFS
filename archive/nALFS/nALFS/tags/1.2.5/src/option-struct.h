/*
 *  option-struct.h - Structures and enums used to process program options.
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


#ifndef H_OPTION_STRUCT_
#define H_OPTION_STRUCT_


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

enum option_type {
	O_BOOL,
	O_NUMBER,
	O_STRING
};

struct option_s {
	char *name;

	enum option_type type;

	union {
		struct {
			STRING value;
			STRING const def_value;
			int (*validate)(const struct option_s *option,
					const STRING value);
			void (*post_validate)(const struct option_s *option);
		} u_str;
		struct {
			BOOL value;
			BOOL const def_value;
			int (*validate)(const struct option_s *option,
					const BOOL value);
			void (*post_validate)(const struct option_s *option);
		} u_bool;
		struct {
			NUMBER value;
			NUMBER const def_value;
			NUMBER const min_value;
			NUMBER const max_value;
			int (*validate)(const struct option_s *option,
					const NUMBER value);
			void (*post_validate)(const struct option_s *option);
		} u_num;
	} val;
};

#ifndef STRING_OPTION
#define STRING_OPTION(opt_name, opt_def_value, opt_other...) \
		extern STRING * const opt_##opt_name
#endif /* STRING_OPTION */

#ifndef BOOL_OPTION
#define BOOL_OPTION(opt_name, opt_def_value, opt_other...) \
		extern BOOL * const opt_##opt_name
#endif /* BOOL_OPTION */

#ifndef NUMBER_OPTION
#define NUMBER_OPTION(opt_name, opt_def_value, opt_other...) \
		extern NUMBER * const opt_##opt_name
#endif /* NUMBER_OPTION */

void set_string_option(STRING * const var, const STRING value);
void append_string_option(STRING * const var, const STRING value);
void set_options_to_defaults(void);
void post_validate_options(void);

char *alloc_real_status_logfile_name(void);
char *alloc_real_packages_directory_name(void);
char *alloc_real_stamp_directory_name(void);

/*
 * Setting option from RC file parser.
 */

typedef enum set_opt_e {
	OPTION_SET,
	OPTION_UNKNOWN,
	OPTION_INVALID_VALUE
} set_opt_e;

set_opt_e set_yet_unknown_option(const char *opt, const char *val);


#endif /* H_OPTION_STRUCT_ */
