/*
 *  logging.h - Logging functions.
 *
 *  Copyright (C) 2001-2003
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


#ifndef H_LOGGING_
#define H_LOGGING_


#include <time.h>

#include "parser.h"


/*
 * Names of the elements used in packages' log files. They are ugly.
 * But this needs some standardization anyway, so it doesn't matter yet.
 */
#define EL_NAME_FOR_ROOT		"package_log"
#define EL_NAME_FOR_A_RUN		"single_try"
#define EL_NAME_FOR_ACTION		"handler_action"
#define EL_NAME_FOR_FILES_ROOT		"changed_files"
#define EL_NAME_FOR_FILES_FIND_ROOT	"find_root"
#define EL_NAME_FOR_FILES_FIND_PRUNES	"find_prunes"
#define EL_NAME_FOR_FILES_FILENAME	"filename"


#define STATE_IS_TIME_STAMP_MSG		"Using time stamp"
#define STATE_IS_LIST_OF_FILES_MSG	"Using two finds"


int stamp_package_installed(
	int installed, const char* name, const char* version);

int check_stamp(const char* name);
int check_stamp_version(const char *name, char *condition, char *version);

void log_handler_action(const char *format, ...);

void log_stopped_execution(void);

void log_end_time(element_s *el, int status);
void log_start_time(element_s *el);

void start_logging_element(element_s *el);
void end_logging_element(element_s *el, int i);


#endif
