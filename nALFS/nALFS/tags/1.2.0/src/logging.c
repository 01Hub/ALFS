/*
 *  logging.c - Logging functions.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bufsize.h"
#include "find.h"
#include "utility.h"
#include "parser.h"
#include "backend.h"
#include "win.h"
#include "nalfs-core.h"
#include "comm.h"
#include "handlers.h"
#include "logfiles.h"
#include "options.h"

#include "logging.h"


#define DATE_FORMAT		"%a, %d %b %Y %H:%M:%S %z"
#define DATE_FORMAT_LEN		31

#define MAX_PACKAGE_NAME_AND_VERSION_BUF_SIZE 128

#define LIST_OF_FILES		"/tmp/nALFS.list_of_files.XXXXXX"


typedef enum state_type_e {
	STATE_TYPE_UNKNOWN = 0,
	STATE_IS_TIME_STAMP,
} state_type_e;


typedef struct flist_s {
	int complete;
	char *name;
} flist_s;


static time_t time_stamp = -1;

static flist_s *list_of_files;

static element_s *current_package;

static logs_t *logs;


static INLINE int get_stamp_directory_status(
	const char * const dirname,
	struct stat *buf)
{
	if (stat(dirname, buf)) {
		/* Creation of stamp directory might not really belong here. */

		if (errno != ENOENT) {
			Nprint_warn("Can't stat %s: %s\n",
				dirname,
				strerror(errno));
			return -1;
		}

		/* Directory doesn't exist. */

		if (execute_command("mkdir -p %s", dirname) != 0) {
			Nprint_warn("Creating %s failed.\n", dirname);
			return -1;
		}

		if (stat(dirname, buf)) {
			Nprint_warn("Can't stat %s after mkdir: %s\n",
				dirname,
				strerror(errno));
			return -1;
		}
	}

	if (! S_ISDIR(buf->st_mode)) {
		Nprint_warn("Not a directory: %s", dirname);
		return -1;
	}

	return 0;
}

static char *alloc_stamp_filename(const char * const name)
{
	struct stat buf;
	char *filename = NULL;
	char *dirname = getenv("NALFS_STAMP_DIR");


	if (Empty_string(dirname)) {
		dirname = alloc_real_stamp_directory_name();
	} else {
		dirname = xstrdup(dirname);
	}

	/* FIXME: If <package> is inside some <stage>, the user can be
	 *        changed.  That user doesn't necessarily have the
	 *        permissions for writing to the stamp directory, so
	 *        this can fail.
	 */
	if (get_stamp_directory_status(dirname, &buf) == 0) {
		filename = xmalloc(strlen(dirname) + strlen(name) + 2);
		sprintf(filename, "%s/%s", dirname, name);
	}

	xfree(dirname);

	return filename;
}

int stamp_package_installed(int success, const char *name, const char *version)
{
	char *filename;
	struct stat buf;
	int status = 0;

	if ((name == NULL) || (version == NULL)) {
		Nprint_warn("Package without name and/or version");
		return -1;
	}

	filename = alloc_stamp_filename(name);

	if (filename == NULL) {
		return -1;
	}

	if (stat(filename, &buf) == 0) {
		if (unlink(filename)) {
			Nprint_warn("Unable to remove %s.", filename);
			xfree(filename);
			return -1;
		}
	}

	if (success) {
		FILE *fp;

		if ((fp = fopen(filename, "w")) != NULL) {
			fputs(name, fp);
			fputs("-", fp);
			fputs(version, fp);

			fclose(fp);

		} else {
			Nprint_warn("Unable to open %s for writing.", filename);
			status = -1;
		}
	}

	xfree(filename);

	return status;
}

int check_stamp(const char *name)
{
	int status = 0;
	char *filename;
	FILE *fp = NULL;


	filename = alloc_stamp_filename(name);

	if (filename == NULL) {
		return 1;
	}

	if ((fp = fopen(filename, "r")) != NULL) {
		char *s;
		char fullpname[MAX_PACKAGE_NAME_AND_VERSION_BUF_SIZE] = {0};

		s = fgets(fullpname, MAX_PACKAGE_NAME_AND_VERSION_BUF_SIZE, fp);

		fclose(fp);

		/* TODO: Check whether fullpname corresponds to name. */

		if (s) {
			remove_new_line(fullpname);
			Nprint("Checking package OK: %s (%s)", name, fullpname);
		} else {
			Nprint_warn("Stamp file empty.");
		}
		status = 0;

	} else {
		status = 1;
	}

	xfree(filename);

	return status;
}

/* return  1 if s1 is greater than s2
          -1 if s1 is smaller than s2
           0 if s1 and s2 are equals
         999 if s1 and s2 are not comparable
	     (with a digit being compared with a nondigit)

	 sequence of digits are compared numerically
	 other characters are compared according to their character code
*/
static int compare_version_aux(char *s1, char *s2)
{
	char *ps1 = s1;
	char *ps2 = s2;

	for (;;) {
		Nprint("scan '%s' - '%s'", ps1, ps2);

		if (isdigit(*ps1)) {
			if (isdigit(*ps2)) {
				/* Two numbers, comparing them numerically. */

				unsigned long val1 = strtoul(ps1, &ps1, 10);
				unsigned long val2 = strtoul(ps2, &ps2, 10);

				Nprint("Comparing %lu with %lu.", val1, val2);

				if (val1 < val2) {
					return -1;

				} else if (val1 > val2) {
					return 1;
				}

				/* Else, numbers are equal. */

			} else if (*ps2) {
				return 999;

			} else { /* arg1 is longer than arg2 */
				return 1;
			}

		} else {
			if (isdigit(*ps2)) {
				if (*ps1) {
					return 999;

				} else { /* arg1 is shorter than arg2 */
					return -1;
				}

			} else {
				if (*ps1 < *ps2) {
					return -1;
	
				} else if (*ps1 > *ps2) {
					return 1;
				}

				if (*ps1 == 0) {
					return 0;
				}

				ps1++;
				ps2++;
			}
		}
	}
}

static int compare_version(char *arg1, char *arg2)
{
	char *s1 = xstrdup(arg1);
	char *s2 = xstrdup(arg2);
	int ret;

	ret = compare_version_aux(s1, s2);

	xfree(s1);
	xfree(s2);

	return ret;
}

int check_stamp_version(const char *name, char *condition, char *version)
{
	int status = -1;
	char *filename;
	FILE *fp = NULL;


	filename = alloc_stamp_filename(name);

	if (filename == NULL) {
		return 1;
	}

	if ((fp = fopen(filename, "r")) != NULL) {
		char *s;
		char fullpname[MAX_PACKAGE_NAME_AND_VERSION_BUF_SIZE] = {0};
		char *pversion;
		int len;
		int cmpstatus;


		s = fgets(fullpname, MAX_PACKAGE_NAME_AND_VERSION_BUF_SIZE, fp);

		fclose(fp);

		if (s == NULL) {
			Nprint_warn("Stamp file empty.");
			return 1;
		}
		
		remove_new_line(fullpname);

		/* check that stamp file content is correct */
		len = strlen(name);
		if (len >=  MAX_PACKAGE_NAME_AND_VERSION_BUF_SIZE) {
			Nprint("Package name is too long", name);
			return 1;
		}
		pversion = &fullpname[len];
		*pversion = 0;
		pversion++;

		if (strcmp(&fullpname[0], name)) {
			Nprint("Stamp package names don't match (%s, %s).",
				fullpname,
				name);

			xfree(filename);
			return 1;
		}

		cmpstatus = compare_version(pversion, version);

		if (cmpstatus == 999) {
			Nprint(
				"Required and stamp file (%s) versions not "
				"comparable.", name);
			xfree(filename);
			return 1;
		}

		if (! strcmp(condition, "eq")) {
			status = cmpstatus == 0;
		}
		else if (! strcmp(condition, "ne")) {
			status = cmpstatus != 0;
		}
		else if (! strcmp(condition, "gt")) {
			status = cmpstatus > 0;
		}
		else if (! strcmp(condition, "lt")) {
			status = cmpstatus < 0;
		}
		else if (! strcmp(condition, "ge")) {
			status = cmpstatus >= 0;
		}
		else if (! strcmp(condition, "gt")) {
			status = cmpstatus <= 0;
		}
		else {
			Nprint("Unknown condition specified (%s) for %s.",
				condition,
				name);

			xfree(filename);
			return 1;
		}

		if (status) {
			Nprint("Checking package version OK: %s(%s) %s %s", 
				name,
				pversion, 
				condition,
				version);

			status = 0;

		} else {
			Nprint("Checking package version failed: %s(%s) %s %s", 
				name,
				pversion,
				condition,
				version);

			status = -1;
		}
	}

	xfree(filename);

	return status;
}


/* Used by Phandler() macro. */
void log_handler_action(const char *format, ...)
{
	char string[MAX_ACTION_MSG_LEN];
	va_list ap;


	if (! *opt_log_handlers) {
		return;
	}

	if (logs == NULL) { // No open file for logging.
		Debug_logging("log_handler_action: logs is NULL");
		return;
	}

	va_start(ap, format);
	vsnprintf(string, MAX_ACTION_MSG_LEN, format, ap);
	va_end(ap);

	logs_add_handler_action(logs, string);
}

static void free_flist(flist_s **f)
{
	if (*f == NULL) {
		return;
	}

	Debug_logging("Deleting the state file (%s).", (*f)->name);
	delete_file((*f)->name);

	xfree((*f)->name);
	xfree(*f);
	*f = NULL;
}



static int collect_files(const char *f, flist_s **flist)
{
	int i;
	char filename[strlen(f) + 1];
	FILE *fp;


	strcpy(filename, f);

	if (create_temp_file(filename)) {
		return -1;
	}

	if ((fp = fopen(filename, "w")) == NULL) {
		Nprint_err("Unable to open \"%s\" for writing: %s",
			filename, strerror(errno));
		return -1;
	}

	/* Doing allocation here, in case collecting is stopped
	 * (so that this temporary file can be deleted).
	 */
	*flist = xmalloc(sizeof *flist);
	(*flist)->name = xstrdup(filename);
	(*flist)->complete = 0;


	Nprint("Collecting files from %s...",
		*opt_find_base ? *opt_find_base : "/");

	i = do_collect_files(fp, *opt_find_base, *opt_find_prunes, time_stamp);

	fclose(fp);

	if (i) {
		Nprint_warn("Collecting files failed or interrupted.");
		free_flist(flist);
		return -1;
	}

	Debug_logging("Collecting files successful.");
	(*flist)->complete = 1;

	return 0;
}

static INLINE void send_state(void)
{
	/*
	 * Send a time stamp (if any) to the frontend.
	 */
	if (time_stamp != -1) {
		size_t data_len;
		char *data;
		char *package_string;
		char time_string[DATE_FORMAT_LEN + 1];


		package_string = alloc_package_string(current_package);

		strftime(time_string, sizeof time_string,
			DATE_FORMAT, localtime(&time_stamp));

		data_len = strlen(STATE_IS_TIME_STAMP_MSG) +
			strlen(package_string) + DATE_FORMAT_LEN + 5;

		data = xmalloc(data_len);

		sprintf(data, "%s: %s\n%s\n",
			STATE_IS_TIME_STAMP_MSG, package_string, time_string);

		xfree(package_string);

		Debug_logging("Sending the time stamp to the frontend...");

		comm_send_ctrl_msg(BACKEND_CTRL_SOCK,
			CTRL_SENDING_STATE, "Sending data...");
		comm_send_from_memory(BACKEND_CTRL_SOCK, data, data_len);

		xfree(data);
	}
}

static void finish_logging(char *installed_files)
{
	int size;
	char *ptr;
	char *package_string;


	Debug_logging("Finishing logging...");

	/*
	 * Send package log file to the frontend.
	 */

	logs_dump_to_memory(logs, &ptr, &size);

	logs_free(logs);
	logs = NULL;

	package_string = alloc_package_string(current_package);

	if (*opt_be_verbose) {
		Nprint("Sending the package log to the frontend... ");
	}

	comm_send_ctrl_msg(BACKEND_CTRL_SOCK,
		CTRL_SENDING_LOG_FILE, "%s", package_string);
	comm_send_from_memory(BACKEND_CTRL_SOCK, ptr, (size_t)size);

	xfree(package_string);

	xfree(ptr);

	/*
	 * Send the list of installed files, or state, to the frontend.
	 */

	if (installed_files) {
		Debug_logging("Sending a list of installed files (%s)... ",
			installed_files);

		comm_send_from_file(
			BACKEND_CTRL_SOCK,
			CTRL_SENDING_FILES_FILE,
			installed_files);
	} else {
		send_state();
	}

	time_stamp = -1;
	free_flist(&list_of_files);
	current_package = NULL;
}



static INLINE void log_stopped_time(void)
{
	char time_str[DATE_FORMAT_LEN + 1];
	time_t t;


	t = time(NULL);
	strftime(time_str, sizeof time_str, DATE_FORMAT, localtime(&t));

	logs_add_stopped_time(logs, time_str);
}

void log_stopped_execution(void)
{
	if (logs == NULL) { // No open file for logging.
		Debug_logging("log_stopped_execution: logs is NULL");
		return;
	}

	log_stopped_time();

	finish_logging(NULL);
}


static INLINE int ask_about_element_status(element_s *el)
{
	ctrl_msg_s *message;
	element_s *profile = get_profile_by_element(el);

	comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_REQUEST_EL_STATUS,
		"%s %s %d", profile->name, el->name, el->id);

	while ((message = comm_read_ctrl_message(BACKEND_CTRL_SOCK)) == NULL)
		/* Wait for the first control message. */ ;

	/* RUN_STATUS_DONE is not 0 anyway, so atoi() will do. */
	return atoi(message->content);
	
}

static INLINE char *stage_two_of_logging_changed_files(void)
{
	/* We have to ask the frontend about element's run status,
	 * as backend can't keep it with all the forking that goes around.
	 */
	if (ask_about_element_status(current_package) != RUN_STATUS_DONE) {
		Debug_logging("Not doing stage two - "
			"package status is not DONE.");
		return NULL;
	}

	if (*opt_logging_method == LOG_USING_ONE_FIND) {
		Debug_logging("Logging using one find.");

		if (time_stamp == -1) {
			Debug_logging("Not doing stage two - "
				"no time stamp created.");
			return NULL;
		}
	}

	collect_files(LIST_OF_FILES, &list_of_files);

	if (list_of_files) {
		logs_add_installed_files(logs, *opt_find_base, *opt_find_prunes);
		return list_of_files->name;
	}

	return NULL;
}

static INLINE void end_package_logging(int status)
{
	char *installed_files = NULL;


	if (*opt_logging_method != LOG_OFF) {
		installed_files = stage_two_of_logging_changed_files();
	} else {
		Debug_logging("Logging files is off, not entering stage two.");
	}

	if (*opt_stamp_packages) {
		char *name = alloc_package_name(current_package);
		char *version = alloc_package_version(current_package);

		/* FIXME:
		 * When element inside the package is run alone,
		 * the whole package won't be installed, but the status
		 * can be 0. nalfs.c::get_element_status() has to be used.
		 * Better when stamping is integrated with existing logging.
		 */
		stamp_package_installed(status == 0, name, version);

		xfree(name);
		xfree(version);
	}

	finish_logging(installed_files);
}



static void stage_one_of_logging_changed_files(void)
{
	if (*opt_logging_method == LOG_USING_ONE_FIND) {
		Debug_logging("Logging using one find.");

		/* Check if the frontend sent us a time stamp. */
		if (time_stamp == -1) {
			Nprint("Creating new time stamp.");
			time_stamp = time(NULL);

			if (*opt_sleep_after_stamp > 0) {
				sleep(*opt_sleep_after_stamp);
			}

		} else {
			Nprint("Using the existing time stamp.");
		}
	}
}

/* Used in the function below. */
char *strptime(const char *s, const char *format, struct tm *tm);

static INLINE void receive_state(void)
{
	char *ptr;
	size_t size;
	char *time_str;
	struct tm broken_down_time;


	comm_read_to_memory(BACKEND_CTRL_SOCK, &ptr, &size);

	time_str = strchr(ptr, '\n') + 1;

	strptime(time_str, DATE_FORMAT, &broken_down_time);

	time_stamp = mktime(&broken_down_time);

	Debug_logging("Time stamp stored (%ld).", time_stamp);
}

static INLINE void request_state(void)
{
	ctrl_msg_s *message;
	ctrl_msg_type_e type;


	comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_REQUESTING_STATE, "");

	while ((message = comm_read_ctrl_message(BACKEND_CTRL_SOCK)) == NULL)
		/* Wait for the first control message. */ ;

	type = comm_msg_type(message);

	if (type == CTRL_NO_STATE) {
		Debug_logging("Frontend doesn't have a state file.");

	} else if (type == CTRL_SENDING_STATE) {
		receive_state();

	} else {
		Nprint_warn("Unexpected control message (%d) received.", type);
	}
}

static INLINE logs_t *start_package_logging(element_s *el)
{
	logs_t *l;

	char *name = alloc_package_name(el);
	char *version = alloc_package_version(el);


	current_package = el;

	l = logs_init_new_run(name, version);

	xfree(name);
	xfree(version);

	// Check if frontend has a state file to send us.
	request_state();

	if (*opt_logging_method != LOG_OFF) {
		stage_one_of_logging_changed_files();
	} else {
		Debug_logging("Logging files is off, not entering stage one.");
	}

	return l;
}



void log_end_time(element_s *el, int status)
{
	char time_str[DATE_FORMAT_LEN + 1];
	time_t t;


	if (logs == NULL) { // No open file for logging.
		Debug_logging("log_end_time: logs is NULL");
		return;
	}

	t = time(NULL);
	strftime(time_str, sizeof time_str, DATE_FORMAT, localtime(&t));

	logs_add_end_time(logs, el->name, time_str, status);
}

void log_start_time(element_s *el)
{
	char time_str[DATE_FORMAT_LEN + 1];
	time_t t;


	if (logs == NULL) { // No open file for logging.
		Debug_logging("log_start_time: logs is NULL");
		return;
	}

	t = time(NULL);
	strftime(time_str, sizeof time_str, DATE_FORMAT, localtime(&t));

	logs_add_start_time(logs, el->name, time_str);
}



void start_logging_element(element_s *el)
{
	if (!*opt_log_backend) { // Logging off.
		Debug_logging("start_logging_element: log_backend is off");
		return;
	}

	Stop_receiving_sigio();

	if (Is_element_name(el, "package")) {
		if (package_has_name_and_version(el)) {
			logs = start_package_logging(el);

		} else {
			Debug_logging("start_logging_element: no name/version");
		}
	}

	Start_receiving_sigio();
}

void end_logging_element(element_s *el, int status)
{
	if (logs == NULL) { // No open file for logging.
		Debug_logging("end_logging_element: logs is NULL");
		return;
	}

	Stop_receiving_sigio();

	if (Is_element_name(el, "package")) {
		end_package_logging(status);

	}

	Start_receiving_sigio();
}
