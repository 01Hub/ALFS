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


#include <libxml/tree.h>
#include <libxml/parser.h>


/*
 * Functions dealing with the format of XML log files.
 * TODO: Hide libxml2 stuff completely, by using only log_f_t pointer.
 *       (Like the functions at the bottom do.)
 */

void log_f_add_handler_action(xmlDocPtr xml_doc, const char *string);
void log_f_add_installed_files_one_find(xmlDocPtr xml_doc);
void log_f_add_installed_files_two_finds(
	xmlDocPtr xml_doc, const char *find_base, const char *find_prunes);
void log_f_add_stopped_time(xmlDocPtr xml_doc, const char *time_str);
void log_f_add_end_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str, int status);
void log_f_add_start_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str);
xmlDocPtr log_f_new_run(const char *name, const char *version);


/*
 * log_f interface.
 */

typedef struct log_f log_f_t;


void log_f_free(log_f_t *log_f);
int log_f_save(log_f_t *log_f);

char *log_f_get_package_fullname(log_f_t *log_f, int i);

/*
 * Multiple log files, initialized from directory.
 */

log_f_t *log_f_init_from_directory(const char *pdir);

int log_f_get_packages_cnt(log_f_t *log_f);

char *log_f_get_plog_filename(log_f_t *log_f, int i);
char *log_f_get_flog_filename(log_f_t *log_f, int i);

/*
 * A single log file, initialized from package's string.
 */

log_f_t *log_f_init_from_package_string(const char *pdir, const char *pstr);

int log_f_merge_log(log_f_t *log_f, const char *ptr, size_t size);

/* Checks if the list of installed files exists for this log file. */
int log_f_has_flog(log_f_t *log_f);

/* Creates an empty file for the list of installed files. */
char *log_f_create_flog(log_f_t *log_f);

void log_f_update_with_flog(log_f_t *log_f);


#endif /* H_LOGFILES_ */
