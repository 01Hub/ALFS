/*
 *  logfiles.h - Functions dealing with packages' log files.
 *
 *  Copyright (C) 2003
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


#ifndef H_LOGFILES_
#define H_LOGFILES_


typedef struct logs logs_t;


void logs_add_handler_action(logs_t *logs, const char *string);

void logs_add_installed_files(
	logs_t *logs, const char *find_base, const char *find_prunes);

void logs_add_stopped_time(logs_t *logs, const char *time_str);
void logs_add_end_time(
	logs_t *logs, const char *el_name, const char *time_str, int status);
void logs_add_start_time(
	logs_t *logs, const char *el_name, const char *time_str);

logs_t *logs_init_new_run(const char *pname, const char *pversion);
void logs_dump_to_memory(logs_t *logs, char **ptr, int *size);


void logs_free(logs_t *logs);
int logs_save(logs_t *logs);

/*
 * Multiple log files, initialized from directory.
 */

logs_t *logs_init_from_directory(const char *pdir);

int logs_get_packages_cnt(logs_t *logs);
char *logs_get_plog_filename(logs_t *logs, int i);
char *logs_get_plog_name(logs_t *logs, int i);
char *logs_get_plog_version(logs_t *logs, int i);
char *logs_get_plog_installed(logs_t *logs, int i);

/*
 * A single log file, initialized from package's string.
 */

logs_t *logs_init_from_package_string(const char *pdir, const char *pstr);

int logs_merge_log(logs_t *logs, const char *ptr, size_t size);

/* Checks if the list of installed files exists for this log file. */
int logs_has_flog(logs_t *logs);

/* Creates an empty file for the list of installed files. */
char *logs_create_flog(logs_t *logs);

void logs_update_with_flog(logs_t *logs);


#endif /* H_LOGFILES_ */
