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


#include "option_struct.h"

#ifndef H_OPTIONS_
#define H_OPTIONS_


STRING_OPTION(alfs_directory, "");
BOOL_OPTION(start_immediately, 0);
STRING_OPTION(packages_directory, "packages");
STRING_OPTION(default_syntax, "3.0");
STRING_OPTION(cursor, "->");
NUMBER_OPTION(indentation_size, 4);
NUMBER_OPTION(sleep_after_stamp, 1);
BOOL_OPTION(beep_when_done, 0);
BOOL_OPTION(display_profile, 1);
BOOL_OPTION(display_options_line, 1);
NUMBER_OPTION(display_timer, TIMER_TOTAL, .validate=validate_number_minmax,
	      .min_value=TIMER_NONE, .max_value=TIMER_CURRENT);
BOOL_OPTION(expand_profiles, 0);
BOOL_OPTION(use_relative_dirs, 0);
BOOL_OPTION(log_status_window, 0);
STRING_OPTION(status_logfile, "log_file");
BOOL_OPTION(show_system_output, 1);
BOOL_OPTION(be_verbose, 0);
BOOL_OPTION(display_alfs, 0);
BOOL_OPTION(display_doctype, 0);
BOOL_OPTION(display_comments, 0);
BOOL_OPTION(run_interactive, 1);
NUMBER_OPTION(jumpto_element, JUMP_TO_RUNNING,
	      .validate=validate_number_minmax,
	      .min_value=JUMP_TO_FAILED, .max_value=JUMP_TO_PACKAGE);
NUMBER_OPTION(logging_method,LOG_OFF, .validate=validate_number_minmax,
	      .min_value=LOG_OFF, .max_value=LAST_LOGGING_METHOD);
BOOL_OPTION(log_handlers, 1);
BOOL_OPTION(log_backend, 1);
BOOL_OPTION(stamp_packages, 0);
STRING_OPTION(stamp_directory, "stamps");
BOOL_OPTION(display_stage_header, 0);
STRING_OPTION(find_base, "/");
STRING_OPTION(find_prunes, "");
STRING_OPTION(find_prunes_file, "");
STRING_OPTION(profiles_directory, "/");
BOOL_OPTION(warn_if_set, 0);
BOOL_OPTION(follow_running, 0);
STRING_OPTION(warn_if_set_variables, "CPPFLAGS CXXFLAGS CFLAGS LDFLAGS");
BOOL_OPTION(print_startup_help, 1);
NUMBER_OPTION(windows_relation, 50, .validate=validate_number_minmax,
	      .min_value=10, .max_value=90);
NUMBER_OPTION(status_history, 500, .validate=validate_number_minmax,
	      .min_value=20, .max_value=5000);
STRING_OPTION(editor, "");
STRING_OPTION(bunzip2_command, "bunzip2 -dc %s", validate_command);
STRING_OPTION(gunzip_command, "zcat %s", validate_command);
STRING_OPTION(uncompress_command, "zcat %s", validate_command);
STRING_OPTION(untar_command, "tar xv", validate_command);
STRING_OPTION(unpax_command, "pax -rv", validate_command);
STRING_OPTION(uncpio_command, "cpio -idv", validate_command);
STRING_OPTION(unzip_command, "unzip %s", validate_command);


#endif /* H_OPTIONS_ */
