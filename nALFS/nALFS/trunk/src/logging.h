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


#define STATE_IS_TIME_STAMP_MSG		"Using time stamp"


int stamp_package_installed(
	int installed, const char* name, const char* version);

int check_stamp(const char* name);
int check_stamp_version(const char *name, char *condition, char *version);

void log_handler_action(const char *format, ...);

void log_stopped_execution(void);

void log_end_time(const element_s * const el, const int status);
void log_start_time(const element_s * const el);

void start_logging_element(const element_s * const el);
void end_logging_element(const element_s * const el, const int status);


#endif /* H_LOGGING_ */
