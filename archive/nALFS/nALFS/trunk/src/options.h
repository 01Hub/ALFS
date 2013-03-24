/*
 *  options.h - Program's options.
 *
 *  Copyright (C) 2002, 2005
 *
 *  Neven Has <haski@sezampro.yu>
 *  Kevin P. Fleming <kpfleming@linuxfromscratch.org>
 *  Jamie Bennett <jamie@linuxuk.org>
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


#include "option-struct.h"

#ifndef H_OPTIONS_
#define H_OPTIONS_


STRING_OPTION(alfs_directory, "", NULL, NULL);
BOOL_OPTION(start_immediately, 0, NULL, NULL);
STRING_OPTION(packages_directory, "packages", NULL, NULL);
STRING_OPTION(default_syntax, "3.0", NULL, NULL);
STRING_OPTION(cursor, "->", NULL, NULL);
NUMBER_OPTION(indentation_size, 4, 0, 0, NULL, NULL);
NUMBER_OPTION(sleep_after_stamp, 1, 0, 0, NULL, NULL);
BOOL_OPTION(beep_when_done, 0, NULL, NULL);
BOOL_OPTION(display_profile, 1, NULL, NULL);
BOOL_OPTION(display_options_line, 1, NULL, NULL);
NUMBER_OPTION(display_timer, TIMER_TOTAL, TIMER_NONE, TIMER_CURRENT,
	      validate_number_minmax, NULL);
BOOL_OPTION(expand_profiles, 0, NULL, NULL);
BOOL_OPTION(use_relative_dirs, 0, NULL, NULL);
BOOL_OPTION(log_status_window, 0, NULL, NULL);
STRING_OPTION(status_logfile, "log_file", NULL, NULL);
BOOL_OPTION(show_system_output, 1, NULL, NULL);
BOOL_OPTION(be_verbose, 0, NULL, NULL);
BOOL_OPTION(display_alfs, 0, NULL, NULL);
BOOL_OPTION(display_doctype, 0, NULL, NULL);
BOOL_OPTION(display_comments, 0, NULL, NULL);
BOOL_OPTION(run_interactive, 1, NULL, NULL);
NUMBER_OPTION(jumpto_element, JUMP_TO_RUNNING, JUMP_TO_FAILED, JUMP_TO_PACKAGE,
	      validate_number_minmax, NULL);
NUMBER_OPTION(logging_method, LOG_OFF, LOG_OFF, LAST_LOGGING_METHOD,
	      validate_number_minmax, NULL);
BOOL_OPTION(log_handlers, 1, NULL, NULL);
BOOL_OPTION(log_backend, 1, NULL, NULL);
BOOL_OPTION(stamp_packages, 0, NULL, NULL);
STRING_OPTION(stamp_directory, "stamps", NULL, NULL);
BOOL_OPTION(disable_digest, 0, NULL, NULL);
BOOL_OPTION(download_check, 0, NULL, NULL);
BOOL_OPTION(display_stage_header, 0, NULL, NULL);
STRING_OPTION(find_base, "/", NULL, NULL);
STRING_OPTION(find_prunes, "", NULL, NULL);
STRING_OPTION(find_prunes_file, "", NULL, NULL);
STRING_OPTION(profiles_directory, "/", NULL, NULL);
BOOL_OPTION(warn_if_set, 0, NULL, NULL);
BOOL_OPTION(follow_running, 0, NULL, NULL);
STRING_OPTION(warn_if_set_variables, "CPPFLAGS CXXFLAGS CFLAGS LDFLAGS", NULL, NULL);
BOOL_OPTION(print_startup_help, 1, NULL, NULL);
NUMBER_OPTION(windows_relation, 50, 10, 90, validate_number_minmax, NULL);
NUMBER_OPTION(status_history, 500, 20, 5000, validate_number_minmax, NULL);
STRING_OPTION(editor, "", NULL, NULL);
STRING_OPTION(bunzip2_command, "bunzip2 -dc %s", validate_command, NULL);
STRING_OPTION(gunzip_command, "zcat %s", validate_command, NULL);
STRING_OPTION(uncompress_command, "zcat %s", validate_command, NULL);
STRING_OPTION(untar_command, "tar xv", validate_command, NULL);
STRING_OPTION(unpax_command, "pax -rv", validate_command, NULL);
STRING_OPTION(uncpio_command, "cpio -idv", validate_command, NULL);
STRING_OPTION(unzip_command, "unzip %s", validate_command, NULL);


#endif /* H_OPTIONS_ */
