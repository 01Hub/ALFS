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
 * TODO: Hide libxml2 stuff completely, by using only logf_t pointer.
 *       (Like the functions at the bottom do.)
 */

void logf_add_handler_action(xmlDocPtr xml_doc, const char *string);
void logf_add_installed_files_one_find(xmlDocPtr xml_doc);
void logf_add_installed_files_two_finds(
	xmlDocPtr xml_doc, const char *find_base, const char *find_prunes);
void logf_add_stopped_time(xmlDocPtr xml_doc, const char *time_str);
void logf_add_end_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str, int status);
void logf_add_start_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str);
xmlDocPtr logf_new_run(const char *name, const char *version);

xmlDocPtr logf_parse_logfile(const char *file);
xmlDocPtr logf_new_logfile(void);
int logf_has_installed_files(xmlNodePtr node);
void logf_add_installed_files(xmlNodePtr node, const char *f);

/*
 * logf interface.
 */

#define LOG_FILE_SUFFIX ".log"

typedef struct logf logf_t;

logf_t *logf_init(const char *pdir);
void logf_free(logf_t *logf);

int logf_get_packages_cnt(logf_t *logf);
char *logf_get_package_name(logf_t *logf, int i);


#endif /* H_LOGFILES_ */
