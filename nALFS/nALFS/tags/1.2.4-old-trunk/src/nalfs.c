/*
 *  nalfs.c - Main file.
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
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdarg.h>

#include <libxml/uri.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "nALFS.h"
#include "bufsize.h"
#include "utility.h"
#include "parser.h"
#include "nprint.h"
#include "win.h"
#include "handlers.h"
#include "backend.h"
#include "comm.h"
#include "editor.h"
#include "logging.h"
#include "logfiles.h"
#include "options.h"
#include "init.h"

#include "nalfs-core.h"

                     /* r a c v s o w h f B */
#define OPTIONS_SPACE "                     "

#define TIMERS_SPACE  "                "


/* Minimum sizes of main pad when displaying something other than elements. */
static const int min_pad_size_for_elements_info = 128;
static const int min_pad_size_for_help = 64;
static const int min_pad_size_for_options = 32;

static const char state_file_name[] = "state_file";


static char *search_string;		// Last entered string for searching.

static volatile int got_sigchld;	// Signals backend termination.

static element_s *current_running;	// Currently executing element.

/* Timer. */

static struct timer {
	time_t backend_started;
	time_t timer_paused;

	double current_executing;
	double total_executing;
	double current_paused;

} timer;


/* State file. */

static struct state {
	char *filename;
	int exists;
} state;

#define State_changed(s, el) ( \
	s.exists && ! is_corresponding_state(s.filename, el) && \
	el->handler && \
	el->handler->is_action)


/* Currently displayed elements in the main window. */

static struct displayed {
	int top;			// Number of the top line.
	int current;			// Number of the current element.
	int total;			// Total number of elements.
	element_s **elements;		// Elements.
} displayed;

#define Current_element (displayed.elements[displayed.current])


/* Backend information. */

static enum backend_status {
	BACKEND_RUNNING,
	BACKEND_PAUSED,
	BACKEND_STOPPED
} backend_status = BACKEND_STOPPED;

static pid_t backend_pid = 0;

#define Backend_exists (backend_pid > 0)

/* Pointer to current function to use for printing to main window. */
void (*nprint)(msg_id_e mid, const char *format, ...);

/* Counters used by nprint_init to track errors/warnings. */
int init_errors = 0;
int init_warnings = 0;

/*
 * Some smaller, helper functions, used often.
 */

static int find_cursor(element_s *el)
{
	int i;

	for (i = 0; i < displayed.total; ++i) {
		if (displayed.elements[i] == el) {
			return i;
		}
	}

	return -1;
}

static int total_number_of_elements(void)
{
	int num = 0;
	element_s *el;


	for (el = root_element; el; el = get_next_element(el)) {
		++num;
	}

	return num;
}

/*
 * Cursor printing and removing.
 */

static void print_cursor(void)
{
	Xmvwaddstr(windows.main->name, displayed.current, 0, *opt_cursor);

	Xwmove(windows.main->name, displayed.current, strlen(*opt_cursor) + 6);
	wchgat(windows.main->name, -1, A_BOLD, COLP_WHITE, NULL);

}

static void remove_cursor(void)
{
	size_t i, len;


	Xwmove(windows.main->name, displayed.current, 0);

	len = strlen(*opt_cursor);
	for (i = 0; i < len; ++i) {
		Xwaddch(windows.main->name, ' ');
	}

	Xwmove(windows.main->name,
		displayed.current, strlen(*opt_cursor) + 6);
	wchgat(windows.main->name, -1, A_NORMAL, COLP_WHITE, NULL);
}

/*
 *
 */

static void fix_top_and_cursor(int *csr)
{
	if (*csr < 0) {
		*csr = 0;
	}
	if (*csr > displayed.total - 1) {
		*csr = displayed.total - 1;
	}

	while (*csr < displayed.top) {
		--displayed.top;
	}
	while (*csr > displayed.top + windows.main->lines - 1) {
		++displayed.top;
	}

	if (displayed.top < 0) {
		displayed.top = 0;
	}
	if (displayed.top > displayed.total - windows.main->lines) {
		displayed.top = displayed.total - windows.main->lines;
	}
}

/*
 * (Un)Marking elements.
 */

static INLINE void do_mark_element(element_s *el)
{
	if ((el->handler->type & HTYPE_NORMAL) != 0) {
		if (el->marked) {
			unmark_element(el, 1);
		} else {
			mark_element(el, 1);
		}
	}
}

static void clear_done_marks(element_s *el)
{
	element_s *child;

	/*
	 * FIXME: Elements with SOME_DONE should be unmarked too,
	 *        if they don't contain any marked children.
	 */

	if ((el->handler->type & HTYPE_NORMAL) == 0)
		return;

	if (el->run_status == RUN_STATUS_DONE)
		unmark_element(el, 0);

	for (child = el->children; child; child = child->next)
		clear_done_marks(child);
}

/*
 * Run-status marks.
 */

static int status_to_mark(run_status_e status)
{
	int mark = ' ';


	switch (status) {
		case RUN_STATUS_FAILED:
			mark = '!';
			break;
		case RUN_STATUS_NONE:
			mark = ' ';
			break;
		case RUN_STATUS_SOME_DONE:
			mark = '@';
			break;
		case RUN_STATUS_DONE:
			mark = '#';
			break;
		case RUN_STATUS_RUNNING:
			mark = '>';
			break;
	}

	return mark;
}

static void print_status_mark(int csr, run_status_e status)
{
	short pair = COLP_STATUS_NONE;

	switch (status) {
		case RUN_STATUS_FAILED:
			pair = COLP_STATUS_FAILED;
			break;
		case RUN_STATUS_RUNNING:
			pair = COLP_STATUS_RUNNING;
			break;
		case RUN_STATUS_SOME_DONE:
			pair = COLP_STATUS_SOME_DONE;
			break;
		case RUN_STATUS_DONE:
			pair = COLP_STATUS_DONE;
			break;
		case RUN_STATUS_NONE:
			return;
	}

	Xmvwaddch(windows.main->name, csr, strlen(*opt_cursor) + 3,
		status_to_mark(status));

	Xwmove(windows.main->name, csr, strlen(*opt_cursor) + 3);
	wchgat(windows.main->name, 1, A_NORMAL, pair, NULL);
}

static INLINE void change_run_status_mark(element_s *el, run_status_e status)
{
	int csr = find_cursor(el);


	if (Can_do_rewrite && csr != -1) {
		print_status_mark(csr, status);

		windows.main->ref(displayed.top);
	}
}

static void set_run_status(element_s *el, run_status_e status)
{
	if (!*opt_run_interactive)
		return;

	/* Change status. */
	el->run_status = status;

	/* Change run status mark. */
	change_run_status_mark(el, status);
}

run_status_e get_element_status(element_s *el)
{
	element_s *child;
	int child_normal = 0;
	int child_status_none = 0;
	int child_status_some_done = 0;
	int child_status_done = 0;
	run_status_e status = RUN_STATUS_NONE;

	for (child = el->children; child; child = child->next) {
		switch (child->run_status) {
		case RUN_STATUS_RUNNING:
			return child->run_status;
		case RUN_STATUS_FAILED:
			return child->run_status;
		default:
			break;
		}

		if ((child->handler->type & HTYPE_NORMAL) == 0)
			continue;
		child_normal++;

		switch (child->run_status) {
		case RUN_STATUS_NONE:
			child_status_none++;
			break;
		case RUN_STATUS_DONE:
			child_status_done++;
			break;
		case RUN_STATUS_SOME_DONE:
			child_status_some_done++;
			break;
		default:
			break;
		}
	}

	if (child_normal == 0)
		status = RUN_STATUS_DONE;
	else if (child_status_done == child_normal)
		status = RUN_STATUS_DONE;
	else if ((child_status_done + child_status_some_done) != 0)
		status = RUN_STATUS_SOME_DONE;

	return status;
}

static void do_change_run_status_marks(element_s *el, run_status_e status)
{
	element_s *child;

	el->run_status = status;

	for (child = el->children; child; child = child->next) {
		do_change_run_status_marks(child, status);
	}
}

static void change_run_status_marks(element_s *el, run_status_e status)
{
	element_s *parent;


	do_change_run_status_marks(el, status);

	/* Update the marks of element's parents. */
	for (parent = el->parent; parent; parent = parent->parent) {
		parent->run_status = get_element_status(parent);
	}
}

/*
 *
 */

static void draw_profile_name(element_s *profile)
{
	int start, end;
	char *profile_name = NULL;

	profile_name = profile->handler->alloc_data(profile, HDATA_DISPLAY_NAME);
	end = windows.max_cols - strlen(PACKAGE_STRING) - 10;
	start = end - strlen(profile_name) - 3;

	if (start < 0 || end - start < 7) { /* Not enough space. */
		xfree(profile_name);
		return;
	}

	/* Handle huge names. */
	if (start < 3) {
		profile_name[end - 9] = '\0';
		append_str(&profile_name, "...");

		start = 3;
	}

	mvhline(0, 1, ACS_HLINE, start - 1);
	Xmvaddch(0, start, ACS_RTEE);
	mvprintw(0, start + 1, " %s ", profile_name);
	Xmvaddch(0, end, ACS_LTEE);

	refresh();

	xfree(profile_name);
}

/*
 *
 */

static INLINE element_s *get_elements_package(element_s *el)
{
	element_s *p;


	for (p = el; p; p = p->parent) {
		if (p->handler && (p->handler->type & HTYPE_PACKAGE)) {
			break;
		}
	}

	return p;
}

/*
 * Checks if element's <package> corresponds to the state file.
 * Used in State_changed() macro too.
 */
static int is_corresponding_state(const char *file_name, element_s *el)
{
	int is_corresponding = 0;
	int sitsm_len = strlen(STATE_IS_TIME_STAMP_MSG);
	char line[MAX_STATE_FILE_LINE_LEN];
	char *package_str = NULL;
	element_s *current_package;
	FILE *fp;


	if ((current_package = get_elements_package(el)) == NULL) {
		Debug_logging("Element %s is not inside <package>.", el->handler->name);
		return 0;
	}

	if (package_has_name_and_version(current_package)) {
		Debug_logging("Element's <package> has "
			"name and version (%s).", el->handler->name);
		package_str = alloc_package_string(current_package);

	} else {
		Debug_logging("Element's <package> dosn't have "
			"name and version (%s).", el->handler->name);
		return 0;
	}

	/* Read first line of the state file. */
	if ((fp = fopen(file_name, "r")) == NULL) {
		Nprint_warn("Unable to open \"%s\": %s",
			file_name, strerror(errno));
		return 0;
	}
	fgets(line, sizeof line, fp);
	remove_new_line(line);
	fclose(fp);

	/* Get the package string from the state file. */
	if (strncmp(line, STATE_IS_TIME_STAMP_MSG, sitsm_len) == 0) {
		char *state_str = xstrdup(line + sitsm_len + 2);

		if (strcmp(package_str, state_str) == 0) {
			is_corresponding = 1;
		}

		xfree(state_str);
	}


	return is_corresponding;
}

static INLINE void receive_state(void)
{
	if (comm_read_to_file(FRONTEND_CTRL_SOCK, state.filename) == 0) {
		Nprint("State stored in \"%s\".", state.filename);
		state.exists = 1;

	} else {
		Nprint_warn( "Reading state file to \"%s\" failed.",
			state.filename);
	}
}

/*
 * Reads the list of installed files from the backend, stores
 * it in the file, and updates the package's log with its name.
 */
static INLINE void receive_changed_files(logs_t *logs)
{
	ctrl_msg_s *message;

	if ((message = comm_read_ctrl_message(FRONTEND_CTRL_SOCK))) {
		ctrl_msg_type_e type = comm_msg_type(message);

		if (type == CTRL_SENDING_FILES_FILE) {
			char *f = logs_create_flog(logs);

			if (comm_read_to_file(FRONTEND_CTRL_SOCK, f) == 0) {
				if (f) {
					logs_update_with_flog(logs);
					Nprint("List of installed files stored in:");
					Nprint("%s", f);
				}
			}
		}

		comm_free_message(message);
	}
}

static INLINE void receive_log_file(const char *package_str)
{
	size_t size;
	char *s;
	char *ptr;
	char *pdir;
	logs_t *logs;

	/* Get new log file. */
	comm_read_to_memory(FRONTEND_CTRL_SOCK, &ptr, &size);

	pdir = alloc_real_packages_directory_name();

	logs = logs_init_from_package_string(pdir, package_str);

	if (logs_merge_log(logs, ptr, size) == -1) {
		Nprint_err("Unable to add new state to the old log file");
	}

	xfree(pdir);
	xfree(ptr);

	if (logs_has_flog(logs)) {
		receive_changed_files(logs);
	}

	s = logs_get_plog_filename(logs, 0);

	if (logs_save(logs) == 0) {
		Nprint("Log file stored in:");
		Nprint("%s", s);
	} else {
		Nprint_err("Unable to save log file to %s.", s);
	}

	logs_free(logs);
}

/*
 * Sends a state (if it exists and is the right one) to the backend
 * and then deletes it.
 */
static INLINE void send_state(void)
{
	if (state.exists) {
		ASSERT(current_running != NULL);

		if (is_corresponding_state(state.filename, current_running)) {
			Nprint("Sending the state to the backend...");
			comm_send_from_file(
				FRONTEND_CTRL_SOCK,
				CTRL_SENDING_STATE,
				state.filename);
		} else {
			Debug_logging("State file for the wrong package.");
			comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_NO_STATE, "");
		}

		Debug_logging("Deleting state file...");
		delete_file(state.filename);
		state.exists = 0;

	} else {
		Debug_logging("State file doesn't exist.");
		comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_NO_STATE, "");
	}
}

/*
 * If there is someting on data socket, it does one read and prints that.
 * Returns non-zero if it founds something, or zero if not.
 */
static int handle_data_msg(void)
{
	int i;
	char buf[MAX_DATA_MSG_LEN];


	i = read(comm_get_socket(FRONTEND_DATA_SOCK), buf, sizeof buf - 1);

	if (i > 0) {
		buf[i] = '\0';
		Nprint_raw("%s", buf);
		return 1;

	} else if (i < 0) {
		/* EAGAIN: No data immediately available for reading.
		 * If not that, we're in trouble.
		 */
		if (errno != EAGAIN) {
			Fatal_error("read() failed: %s", strerror(errno));
		}

	} else if (i == 0) {
		/* End of file. */
	}

	return 0;
}

/* takes string containing element id and gets element */
static element_s *find_element_by_key(const char *str)
{
	unsigned int id = atoi(str);

	return get_element_by_id(id);
}

static INLINE void send_element_status(const char *key)
{
	int status;
	element_s *el;

	el = find_element_by_key(key);
	ASSERT(el != NULL);

	status = get_element_status(el);

	comm_send_ctrl_msg(FRONTEND_CTRL_SOCK,
		CTRL_REQUEST_EL_STATUS, "%d", status);
}

static INLINE void jump_to_current_running(void)
{
	int i = -1;
	element_s *el = current_running;


	/*
	 * If the last running element is not visible,
	 * get the first parent which is.
	 */
	while (el && (i = find_cursor(el)) == -1) {
		el = el->parent;
	}

	if (i != -1) { /* Found the element, jump to it. */
		if (Can_do_rewrite) {
			remove_cursor();
		}

		displayed.current = i;

		if (Can_do_rewrite) {
			fix_top_and_cursor(&displayed.current);

			print_cursor();

			if (*opt_display_profile) {
				draw_profile_name(
				get_profile_by_element(Current_element));
			}

			windows.main->ref(displayed.top);
		}
	}
}

static INLINE void element_started(const char *msg_content)
{
	current_running = find_element_by_key(msg_content);

	// Nprint("Started: %s (%s)", msg_content, current_running->handler->name);

	ASSERT(current_running != NULL);

	set_run_status(current_running, RUN_STATUS_RUNNING);

	/* Jump to this element. */
	if (*opt_follow_running && *opt_run_interactive) {
		jump_to_current_running();
	}
}

/* current_finishing element succeeded. */
static INLINE void element_ended(const char *msg_content)
{
	element_s *current_ended = find_element_by_key(msg_content);

	// Nprint("Ended: %s (%s)", msg_content, current_ended->handler->name);

	ASSERT(current_ended != NULL);

	if (State_changed(state, current_ended)) {
		Debug_logging("Removing state file, state probably changed.");
		delete_file(state.filename);
		state.exists = 0;
	}

	set_run_status(current_ended, get_element_status(current_ended));
}

static INLINE void element_failed(const char *msg_content)
{
	element_s *current_failed = find_element_by_key(msg_content);

	// Nprint("Failed: %s (%s)", msg_content, current_failed->handler->name);

	ASSERT(current_failed != NULL);

	if (State_changed(state, current_failed)) {
		Debug_logging("Removing state file, state probably changed.");
		delete_file(state.filename);
		state.exists = 0;
	}

	set_run_status(current_failed, RUN_STATUS_FAILED);
}

/*
 * Tries to read and handle _one_ contol message.
 * Returns non-zero if it found something for reading.
 */
static int handle_ctrl_msg(void)
{
	ctrl_msg_s *message;


	if ((message = comm_read_ctrl_message(FRONTEND_CTRL_SOCK)) != NULL) {
		char *content;

		content = comm_msg_content(message);

		switch (comm_msg_type(message)) {
			case CTRL_ELEMENT_STARTED:
				element_started(content);
				break;

			case CTRL_ELEMENT_ENDED:
				element_ended(content);
				break;

			case CTRL_ELEMENT_FAILED:
				element_failed(content);
				break;

			case CTRL_SENDING_LOG_FILE:
				receive_log_file(content);
				break;

			case CTRL_SENDING_STATE:
				receive_state();
				break;

			case CTRL_REQUESTING_STATE:
				send_state();
				break;

			case CTRL_REQUEST_EL_STATUS:
				send_element_status(content);
				break;

			default:
				Nprint_warn("Unknown control message: %s",
					content);
		}

		comm_free_message(message);

		return 1;
	}

	return 0;
}

/*
 *
 */

static void clear_should_run_marks(element_s *el)
{
	element_s *child;


	el->should_run = 0;

	for (child = el->children; child; child = child->next) {
		clear_should_run_marks(child);
	}
}

static void clear_done_run_status(element_s *el)
{
	element_s *child;


	if (el->run_status == RUN_STATUS_DONE) {
		el->run_status = RUN_STATUS_NONE;
	}

	for (child = el->children; child; child = child->next) {
		clear_done_run_status(child);
	}
}

/*
 * Starting the editor with specified file name.
 */

static INLINE void run_editor(const char *filename)
{
	int i;
	const char *editor;
	char *command = NULL;


	endwin();

	editor = *opt_editor;

	if (Empty_string(editor)) {
		editor = getenv("EDITOR");
	}

	if (Empty_string(editor)) {
		editor = "vi";
	}

	append_str_format(&command, "%s %s", editor, filename);

	i = system(command);

	refresh();
	keypad(windows.main->name, 1);

	if (i != 0) {
		Nprint_warn("Executing: %s", command);
		Nprint_warn("returned non-zero value.");
	}

	xfree(command);
}

/*
 * Help menu.
 */

static INLINE int write_help(void)
{
	int i, lines_written;


	Xwerase(windows.main->name);
	Xwmove(windows.main->name, 0, 0);

	Xwprintw(windows.main->name,
	"     To exit this help, type 'q'.\n");
	Xwprintw(windows.main->name,
	"\n");
	Xwprintw(windows.main->name,
	" j   move down (down arrow)\n"
	" k   move up (up arrow)\n"
	"^n   move down several lines (PageDown)\n"
	"^p   move up several lines (PageUp)\n"
	"\n"
	" h   go to parent, or close element (left arrow)\n"
	" H   close all elements under the cursor\n"
	" l   open element, or display its info (right arrow)\n"
	" L   open all elements under the cursor\n"
	"\n"
	" J   jump forward to jump-to element (changeable with 'o' -> 'j')\n"
	" K   jump backward to jump-to element\n"
	" ;   go to the last jump-to element\n"
	"\n"
	" s   start running\n"
	" S   stop running\n"
	" p   pause the output\n"
	"\n"
	" *   mark element ('Insert' key can be used too)\n"
	" u   unmark successfully done elements\n"
	" U   unmark all elements\n"
	"\n"
	" i   display short element's info\n"
	" I   display more detailed element's info\n"
//	" x   display XML of elements under the cursor\n"
	"\n"
	" o   quick option change\n"
	" O   enter the options' menu\n"
	"\n"
	" m   change run-status mark of the element\n"
	"\n"
	" e   edit element\n"
	" E   start editor on the current profile\n"
	"\n"
	" t   show timer\n"
	" T   reset timer\n"
	"\n"
	" a   add profile\n"
	" d   delete profile\n"
	" r   reload profile\n"
	" -   move profile up\n"
	" +   move profile down\n"
	"\n"
	" /   search\n"
	" n   repeat search forward\n"
	" N   repeat search backward\n"
	"\n"
	" w   switch between main and status window\n"
	" W   change windows' sizes (using '+' and '-')\n"
	"\n"
	"^l   redraw the screen\n"
	" ?   this help\n"
	" q   quit the program");

	Xwprintw(windows.main->name,
	"\n\n");
	Xwprintw(windows.main->name,
	" " PACKAGE_STRING ", " COPYRIGHT "\n");
	Xwprintw(windows.main->name,
	" nALFS comes with ABSOLUTELY NO WARRANTY.\n");
	Xwprintw(windows.main->name,
	" This is free software, and you are welcome to redistribute it\n");
	Xwprintw(windows.main->name,
	" under certain conditions. See COPYING for more info.");

	getyx(windows.main->name, lines_written, i);

	return lines_written;
}

static INLINE void display_help_page(void)
{
	int lines, top = 0;


	windows.active = TMP_WINDOW;

	/* Recreate main pad if it is too small. */
	if (total_number_of_elements() < min_pad_size_for_help) {
		recreate_main_window(min_pad_size_for_help);
	}

	lines = write_help();

	tmp_window_driver(lines, &top, NULL);

	windows.active = MAIN_WINDOW;
}

/*
 * Packages.
 */

static int pkg_do_remove_package(logs_t *logs, int idx)
{
	char *command = NULL;
	char *flog = logs_get_plog_installed(logs, idx);
	char *plog = logs_get_plog_filename(logs, idx);


	append_str_format(&command, "rm -fr `cat %s`", flog);
	system(command);
	xfree(command);

	Nprint("Done removing files.");

	Nprint("Do you also want to remove the log files? (n/Y)");

	if (get_key(windows.main->name) == 'Y') {
		command = NULL;
		append_str_format(&command, "rm -f %s %f", flog, plog);
		system(command);
		xfree(command);

		Nprint("Log files removed:");
		Nprint("%s", flog);
		Nprint("%s", plog);

		return 0;

	} else {
		Nprint("Log files not removed.");
	}

	return -1;
}

static int pkg_remove_package(logs_t *logs, int idx)
{
	int c;
	char *flog;


	if ((flog = logs_get_plog_installed(logs, idx)) == NULL) {
		Nprint("List of installed files doesn't exist.");
		return -1;
	}

	Nprint("Edit the list of files to remove? (y/N)?");

	c = get_key(windows.main->name);

	if (c == 'y') {
		run_editor(flog);

	} else if (c != 'N') {
		Nprint("Removing package aborted.");
		return -1;
	}

	/* We're going to do "rm -fr", so be REALLY annoying. */

	Nprint("Do you want to continue with removing this package? (n/Y)");

	if ((get_key(windows.main->name)) != 'Y') {
		Nprint("Removing package aborted.");
		return -1;
	}

	Nprint_warn("You're about to remove some files by executing:");
	Nprint_warn("rm -fr `cat %s`", flog);
	Nprint_warn("If you know exactly what this means, type yes.");
	Nprint_warn("If not, press anything else to abort.");

	if (get_key(windows.main->name) != 'y'
	||  get_key(windows.main->name) != 'e'
	||  get_key(windows.main->name) != 's') {
		Nprint("Removing package aborted.");
		return -1;

	}

	return pkg_do_remove_package(logs, idx);
}

/*
 * TODO: Pressing enter should display package's information first.
 *       From that window, other commands for that package can be given.
 */
static int pkg_process_command(logs_t *logs, int idx, int input)
{
	switch (input) {
		case 'R':
			return pkg_remove_package(logs, idx);

		/*
		 * A lot of useful commands can be added here.  Not
		 * only related to _installed_ packages, but to ones
		 * in loaded profiles too (or packages described in
		 * profiles located in some specified directory).
		 */

	      default:
			Nprint_warn("Unknown command.");
			break;
	}

	return -1;
}

static void pkg_write_main_line(logs_t *logs, int idx)
{
	size_t i;
	char *line = NULL;
	char *name = logs_get_plog_name(logs, idx);
	char *version = logs_get_plog_version(logs, idx);


	/* Space for cursor. */
	for (i = 0; i < strlen(*opt_cursor) + 1; ++i) {
		append_str(&line, " ");
	}

	append_str_format(&line, "%s - %s", name, version);

	/* Print the line. */
	if (strlen(line) + 4 > (unsigned int)windows.max_cols) {
		waddnstr(windows.main->name, line, windows.max_cols - 7);
		Xwaddstr(windows.main->name, "...\n");
	} else {
		Xwaddstr(windows.main->name, line);
		Xwaddstr(windows.main->name, "\n");
	}
}

#define EXTRA_TOP_LINES   2
#define EXTRA_TOTAL_LINES 3

static INLINE int pkg_print_installed_packages(logs_t *logs)
{
	size_t j;
	int i, lines_written;
	int packages_cnt = logs_get_packages_cnt(logs);


	Xwerase(windows.main->name);
	Xwmove(windows.main->name, 0, 0);

	/* Space for cursor. */
	for (j = 0; j < strlen(*opt_cursor) + 1; ++j) {
		Xwaddstr(windows.main->name, " ");
	}

	Xwaddstr(windows.main->name, "R - remove package\n\n");

	for (i = 0; i < packages_cnt ; ++i) {
		pkg_write_main_line(logs, i);
	}

	getyx(windows.main->name, lines_written, i);

	return lines_written + 1;
}

/* TODO: pkg_main(Current_element)
 *       So that we can immediately point the cursor
 *       to the package in selected profile.
 */
static INLINE void pkg_main(void)
{
	int pcnt;
	int input;
	int lines, top = 0, curr = 0;
	char *pdir;
	logs_t *logs;
	

	pdir = alloc_real_packages_directory_name();

	Nprint("Reading log files from %s...", pdir);

	logs = logs_init_from_directory(pdir);
	pcnt = logs_get_packages_cnt(logs);

	if (pcnt > 0) {
		Nprint("Found %d packages' logs.", pcnt);

		windows.active = TMP_WINDOW;

		/* Recreate main pad if it is too small. */
		if (total_number_of_elements() < pcnt + EXTRA_TOTAL_LINES) {
			recreate_main_window(pcnt + EXTRA_TOTAL_LINES);
		}

		lines = pkg_print_installed_packages(logs);

		while ((input = tmp_window_driver(lines, &top, &curr)) != -1) {
			if (pkg_process_command(
				logs, curr-EXTRA_TOP_LINES, input) == 0) {
				break;
			}
		}

		windows.active = MAIN_WINDOW;

	} else {
		Nprint("No packages' logs found in %s", pdir);
	}

	logs_free(logs);

	xfree(pdir);
}

/*
 * Writing to main window.
 */

static int should_skip_element(element_s *el, int *depth)
{
	if (el->type == TYPE_COMMENT && !*opt_display_comments) {
		return 1;
	}

/* TODO: is skipping of <alfs> elements really useful? */
#if 0
	if (!*opt_display_alfs && (el, "alfs")) {
		if (depth) {
			--(*depth);
			el->hide_children = 0;
		}
		return 1;
	}
#endif

	if (!*opt_display_doctype && el->type == TYPE_DOCTYPE) {
		if (depth) {
			el->hide_children = 1;
		}
		return 1;
	}
	if (!*opt_display_doctype
	&& el->parent && el->parent->type == TYPE_DOCTYPE) {
		return 1;
	}

	if (el == root_element) {
		return 1;
	}

	return 0;
}

static void write_main_line(element_s *el, int *depth)
{
	int i, j;
	char *line = NULL;

	if (should_skip_element(el, depth)) {
		return;
	}

	++(displayed.total);
	displayed.elements = xrealloc(displayed.elements,
		(displayed.total) * sizeof *displayed.elements);
	displayed.elements[displayed.total - 1] = el;

	append_str_format(&line, "%*s %s %c%*s%s ",
			  strlen(*opt_cursor), "",
			  el->marked ? "*" : " ",
			  status_to_mark(el->run_status),
			  *depth * *opt_indentation_size, "",
			  el->children ? (el->hide_children ? "+" : "-") : " ");

	if (el->handler->data & HDATA_DISPLAY_NAME) {
		char *display = el->handler->alloc_data(el, HDATA_DISPLAY_NAME);
		append_str(&line, display);
		xfree(display);
	} else {
		append_str(&line, el->handler->description);
	}

	/* Print the line. */
	if ((int)strlen(line) > windows.max_cols - 4) {
		waddnstr(windows.main->name, line, windows.max_cols - 7);
		Xwaddstr(windows.main->name, "...\n");
	} else {
		Xwaddstr(windows.main->name, line);
		Xwaddstr(windows.main->name, "\n");
	}

	/* Print run-status mark. */
	getyx(windows.main->name, j, i);
	print_status_mark(j - 1, el->run_status);
	Xwmove(windows.main->name, j, i);

	xfree(line);
}

static void write_element_and_its_children(element_s *el, int depth)
{
	element_s *child;


	write_main_line(el, &depth);

	if (el->hide_children ) {
		return;
	}

	++depth;

	for (child = el->children; child; child = child->next) {
		write_element_and_its_children(child, depth);
	}
}

/* Also updates displayed structure. */
static void rewrite_main(void)
{
	element_s *child;


	if (displayed.total > 0) {
		displayed.elements = xrealloc(displayed.elements,
			sizeof *displayed.elements);
	} else {
		displayed.elements = xmalloc(sizeof *displayed.elements);
	}
	displayed.total = 0;

	Xwerase(windows.main->name);

	for (child = root_element->children; child; child = child->next) {
		write_element_and_its_children(child, 0);
	}
}



/*
 * Jumping to elements.
 */

static int found_searched(element_s *el)
{
	element_s *tmp = el;


	/* Open all parents of this element. */
	while ((tmp = tmp->parent)) {
		tmp->hide_children = 0;
	}

	rewrite_main();

	return find_cursor(el);
}

static INLINE int jump_match(element_s *el)
{
	switch (*opt_jumpto_element) {
		case JUMP_TO_FAILED:
			return el->run_status == RUN_STATUS_FAILED;

		case JUMP_TO_RUNNING:
			return el->run_status == RUN_STATUS_RUNNING;

		case JUMP_TO_DONE:
			return (el->run_status == RUN_STATUS_DONE
				|| el->run_status == RUN_STATUS_SOME_DONE);

		case JUMP_TO_PACKAGE:
			return el->handler->type & HTYPE_PACKAGE;
	}

	/* Never reached. */

	return 0;
}

static INLINE int jump_forward(element_s *el)
{
	element_s *curr = get_next_element(el);


	if (curr == NULL) {
		Nprint("Jumping wrapped to top.");
		curr = root_element;
	}

	while (curr != el) {
		/* Check if this is the element we need. */
		if (jump_match(curr)) {
			int i;

			// Open all elements to the package.
			if (*opt_jumpto_element == JUMP_TO_PACKAGE) {
				i = found_searched(curr);
			} else {
				i = find_cursor(curr);
			}

			if (i != -1) {
				displayed.current = i;
				return 1;
			}
		}

		/* It's not, keep going. */
		if ((curr = get_next_element(curr)) == NULL) {
			Nprint("Jumping wrapped to top.");
			curr = root_element;
		}
	}

	return 0;
}

static INLINE int jump_backward(element_s *el)
{
	element_s *curr = NULL;
	element_s *last = NULL;


	/* Set last element. */
	for (curr = root_element; curr; curr = get_next_element(curr)) {
		last = curr;
	}

	curr = get_prev_element(el);

	if (curr == NULL || curr == root_element) {
		Nprint("Jumping wrapped to bottom.");
		curr = last;
	}

	while (curr != el) {
		/* Check if this is the element we need. */
		if (jump_match(curr)) {
			int i;

			// Open all elements to the package.
			if (*opt_jumpto_element == JUMP_TO_PACKAGE) {
				i = found_searched(curr);
			} else {
				i = find_cursor(curr);
			}

			if (i != -1) {
				displayed.current = i;
				return 1;
			}
		}

		/* It's not, keep going. */
		curr = get_prev_element(curr);

		if (curr == root_element) {
			Nprint("Jumping wrapped to bottom.");
			curr = last;

		}
	}

	return 0;
}

static void print_did_not_jump_message(void)
{
	switch (*opt_jumpto_element) {
		case JUMP_TO_FAILED:
			Nprint("No more failed elements.");
			break;

		case JUMP_TO_RUNNING:
			Nprint("No more running elements.");
			break;

		case JUMP_TO_DONE:
			Nprint("No more done elements.");
			break;

		case JUMP_TO_PACKAGE:
			Nprint("No more packages.");
			break;
	}
}

enum jump_direction {
	JUMP_FORWARD,
	JUMP_BACKWARD
};

static void jump_to_element(enum jump_direction direction)
{
	int did_jump = 0;


	switch (direction) {
		case JUMP_FORWARD:
			did_jump = jump_forward(Current_element);
			break;

		case JUMP_BACKWARD:
			did_jump = jump_backward(Current_element);
			break;
	}

	if (! did_jump) { /* Print the error message. */
		print_did_not_jump_message();
	}
}


static void jump_to_last_element(void)
{
	element_s *el;
	element_s *match = NULL;


	for (el = root_element; el; el = get_next_element(el)) {
		/* Check if this is the element we need. */
		if (jump_match(el)) {
			match = el;
		}
	}

	if (match) {
		int i = found_searched(match);

		ASSERT(i != -1);

		if (i != -1) {
			displayed.current = i;
		}
	} else {
		print_did_not_jump_message();
	}
}

static INLINE void toggle_jump_to_element(void)
{
	Nprint("Enter jump-to element.");

	switch (get_key(windows.main->name)) {
		case 'r':
			*opt_jumpto_element = JUMP_TO_RUNNING;
			Nprint("Jumping to running elements from now on.");
			break;

		case 'f':
			*opt_jumpto_element = JUMP_TO_FAILED;
			Nprint("Jumping to failed elements from now on.");
			break;

		case 'd':
			*opt_jumpto_element = JUMP_TO_DONE;
			Nprint("Jumping to done elements from now on.");
			break;

		case 'p':
			*opt_jumpto_element = JUMP_TO_PACKAGE;
			Nprint("Jumping to packages from now on.");
			break;

		default:
			Nprint_warn("No such option.");
	}
}


/*
 *
 */

static INLINE void toggle_verbosity(void)
{
	Toggle(*opt_be_verbose);

	if (*opt_be_verbose) {
		Nprint("Verbosity now on.");
	} else {
		Nprint("Verbosity now off.");
	}
}

static INLINE void toggle_backend_logging(void)
{
	if (Backend_exists) {
		comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_LOG_BACKEND, "");
	}

	Toggle(*opt_log_backend);

	if (*opt_log_backend) {
		Nprint("Backend logging now on.");
	} else {
		Nprint("Backend logging now off.");
	}
}

static INLINE void toggle_logging(void)
{
	char *file = alloc_real_status_logfile_name();
	FILE *fp;


	if (*opt_log_status_window) {
		Nprint("Logging to \"%s\" now off.", file);
		*opt_log_status_window = 0;

	} else {
		if ((fp = fopen(file, "a")) != NULL) {
			*opt_log_status_window = 1;
			Nprint("Logging to \"%s\" now on.", file);
			fclose(fp);

		} else {
			Nprint_warn("Unable to open \"%s\" for logging: %s.",
				file, strerror(errno));
		}
	}

	xfree(file);
}

static INLINE void toggle_logging_files(void)
{
	if (Backend_exists) {
		comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_LOG_CHANGED_FILES, "");
	}

	if (++*opt_logging_method > LAST_LOGGING_METHOD) {
		*opt_logging_method = 0;
	}

	switch (*opt_logging_method) {
		case LOG_USING_ONE_FIND:
			Nprint("Logging changed files using time stamps.");
			break;
		case LOG_OFF:
			Nprint("Logging changed files now off.");
			break;
	}
}

static INLINE void toggle_logging_actions(void)
{
	if (Backend_exists) {
		comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_LOG_HANDLER_ACTIONS, "");
	}

	if (*opt_log_handlers) {
		Nprint("Logging handler actions now off.");
		*opt_log_handlers = 0;
	} else {
		Nprint("Logging handler actions now on.");
		*opt_log_handlers = 1;
	}
}

#if 0
static INLINE void toggle_generate_stamp(void)
{
	if (Backend_exists) {
		comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_GENERATE_STAMP, "");
	}

	if (*opt_stamp_packages) {
		Nprint("Generating stamp now off.");
		*opt_stamp_packages = 0;
	} else {
		Nprint("Generating stamp now on.");
		*opt_stamp_packages = 1;
	}
}
#endif

static INLINE void toggle_system_output(void)
{
	if (Backend_exists) {
		comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_SYSTEM_OUTPUT, "");
	}

	if (*opt_show_system_output) {
		Nprint("Displaying system output turned off.");
		*opt_show_system_output = 0;
	} else {
		Nprint("Displaying system output turned on.");
		*opt_show_system_output = 1;
	}
}

static INLINE void toggle_following(void)
{
	if (*opt_follow_running) {
		Nprint("Following running elements now off.");
		*opt_follow_running = 0;
	} else {
		Nprint("Following running elements now on.");
		*opt_follow_running = 1;
	}
}

/*
 * Toggling comments displaying.
 */

static INLINE element_s *find_nearest_non_comment_element(void)
{
	int i;
	element_s *el = NULL;


	for (i = 0; ; ++i) {
		if (displayed.current + i < displayed.total) {
			el = displayed.elements[displayed.current + i];
			if (el && el->type != TYPE_COMMENT) {
				break;
			}
		}

		if (displayed.current - i >= 0) {
			el = displayed.elements[displayed.current - i];
			if (el && el->type != TYPE_COMMENT) {
				break;
			}
		}
	}

	return el;
}

static INLINE void toggle_comments(void)
{
	int i;
	element_s *old_el = Current_element;


	Toggle(*opt_display_comments);

	if (! *opt_display_comments) {
		Nprint("Comments hidden.");
	} else {
		Nprint("Comments will be displayed again.");
	}

	/* We're on the element that's going to be hidden, find another. */
	if (Current_element->type == TYPE_COMMENT && !*opt_display_comments) {
		old_el = find_nearest_non_comment_element();
	}

	rewrite_main();

	/* Get new cursor position. */
	i = find_cursor(old_el);

	displayed.top += i - displayed.current;
	displayed.current = i;
}

/*
 * Toggling <alfs> displaying.
 */

static INLINE element_s *find_nearest_non_alfs_element(void)
{
	int i;
	element_s *el = NULL;


	for (i = 0; ; ++i) {
		if (displayed.current + i < displayed.total) {
			el = displayed.elements[displayed.current + i];
			if (el && ! Is_element_name(el, "alfs")) {
				break;
			}
		}
		if (displayed.current - i >= 0) {
			el = displayed.elements[displayed.current - i];
			if (el && ! Is_element_name(el, "alfs")) {
				break;
			}
		}
	}

	return el;
}

static INLINE void toggle_alfs(void)
{
	int i;
	element_s *old_el = Current_element;


	Toggle(*opt_display_alfs);

	if (! *opt_display_alfs) {
		Nprint("Alfs element hidden.");
	} else {
		Nprint("Alfs element will be displayed again.");
	}

	/* We're on the element that's going to be hidden, find another. */
	if (Is_element_name(Current_element, "alfs") && !*opt_display_alfs) {
		old_el = find_nearest_non_alfs_element();
	}

	rewrite_main();

	/* Get new cursor position. */
	i = find_cursor(old_el);

	displayed.top += i - displayed.current;
	displayed.current = i;
}

static INLINE void change_find_options(void)
{
	char *tmp;

	if (Backend_exists) {
		Nprint_warn("Can't change find options while running.");
		return;
	}

	tmp = xstrdup(*opt_find_base);
	get_string_from_bottom("Root directory:", &tmp);
	set_string_option(opt_find_base, tmp);
	xfree(tmp);
	Nprint("Root directory: %s", *opt_find_base ? *opt_find_base : "/");

	tmp = xstrdup(*opt_find_prunes);
	get_string_from_bottom("Prune directories:", &tmp);
	set_string_option(opt_find_prunes, tmp);
	xfree(tmp);
	if (*opt_find_prunes) {
		Nprint("Prune directories: %s", *opt_find_prunes);
	} else {
		Nprint("Not ignoring any directory.");
	}
}

static void draw_options_indicators(void)
{
	int c = '?';
	int row = 1 + windows.main->lines;
	int col = windows.max_cols - 2 - strlen(OPTIONS_SPACE);
	short pair = COLP_STATUS_NONE;

	if (col < 4) { /* Not enough space. */
		return;
	}

	/* Jump to option. */

	switch (*opt_jumpto_element) {
		case JUMP_TO_FAILED:
			c = 'f';
			pair = COLP_STATUS_FAILED;
			break;

		case JUMP_TO_RUNNING:
			c = 'r';
			pair = COLP_STATUS_RUNNING;
			break;

		case JUMP_TO_DONE:
			c = 'd';
			pair = COLP_STATUS_DONE;
			break;

		case JUMP_TO_PACKAGE:
			c = 'p';

			break;
	}

	Xmvaddch(row, col++, c);
	Xwmove(stdscr, row, col - 1);
	wchgat(stdscr, 1, A_NORMAL, pair, NULL);

	++col;
	Xmvaddch(row, col++, *opt_display_alfs		? 'a' : ' ');
	Xmvaddch(row, col++, *opt_display_doctype	? 'd' : ' ');
	Xmvaddch(row, col++, *opt_display_comments	? 'c' : ' ');
	++col;
	Xmvaddch(row, col++, *opt_be_verbose		? 'v' : ' ');
	++col;
	Xmvaddch(row, col++, *opt_show_system_output 	? 's' : ' ');
	++col;
	Xmvaddch(row, col++, *opt_follow_running		? 'o' : ' ');
	++col;
	Xmvaddch(row, col++, *opt_log_status_window	? 'w' : ' ');
	++col;
	Xmvaddch(row, col++, *opt_log_handlers		? 'h' : ' ');
	++col;
	switch (*opt_logging_method) {
		case LOG_USING_ONE_FIND:
			Xmvaddch(row, col++, 'f');
			break;
		case LOG_OFF:
			Xmvaddch(row, col++, ' ');
			break;
	}
	++col;
	Xmvaddch(row, col++, *opt_log_backend ? 'B' : ' ');

	refresh();
}

#define Po(a, b...) Xwprintw(windows.main->name, a, ## b)

static int print_options(void)
{
	int lines, x;
	char jump[8];
	char find[31];
	char tdisplay[8];

	Xwerase(windows.main->name);
	Xwmove(windows.main->name, 0, 0);

	/*
	 * Set them.
	 */

	switch (*opt_jumpto_element) {
		case JUMP_TO_FAILED:
			strcpy(jump, "Failed");
			break;

		case JUMP_TO_RUNNING:
			strcpy(jump, "Running");
			break;

		case JUMP_TO_DONE:
			strcpy(jump, "Done");
			break;

		case JUMP_TO_PACKAGE:
			strcpy(jump, "Package");
			break;

		default:
			ASSERT(0);
			strcpy(jump, "Unknown");
			
	}


	switch (*opt_logging_method) {
		case LOG_USING_ONE_FIND:
			strcpy(find, "Using one find()-like search");
			break;

		case LOG_OFF:
			strcpy(find, "No");
			break;

		default:
			ASSERT(0);
			strcpy(find, "Unknown");
	}

	switch (*opt_display_timer) {
		case TIMER_NONE:
			strcpy(tdisplay, "Nothing");
			break;

		case TIMER_CURRENT:
			strcpy(tdisplay, "Current");
			break;

		case TIMER_TOTAL:
			strcpy(tdisplay, "Total");
			break;

		default:
			ASSERT(0);
			strcpy(tdisplay, "Unknown");
	}

	/*
	 * Print them.
	 */

	Po("To exit the option menu, type 'q'.\n");
	Po("\n");
	Po("(B)ackend logging: %s\n", Onoff(*opt_log_backend));
	Po("    (h) Log handler actions: %s\n", Yesno(*opt_log_handlers));
	Po("    (f) Log changed files: %s\n", find);
	Po("    (F) Find search options\n");
	Po("        Base directory: %s\n", *opt_find_base ? *opt_find_base : "");
	Po("        Prune directories:\n");
	Po("        %s", *opt_find_prunes ? *opt_find_prunes : "");
	Po("\n\n");
	Po("Jump-to element: %s\n", jump);
	Po("    (jr) Jump to next running element\n");
	Po("    (jf) Jump to next failed element\n");
	Po("    (jd) Jump to next successfully done element\n");
	Po("    (jp) Jump to next package\n");
	Po("\n");
	Po("(w) Log status window: %s\n", Yesno(*opt_log_status_window));
	Po("\n");
	Po("(o) Follow running element: %s\n", Yesno(*opt_follow_running));
	Po("\n");
	Po("(a) Show <alfs> element: %s\n", Yesno(*opt_display_alfs));
//	Po("(d) Show doctype: %s\n", Yesno(*opt_display_doctype));
	Po("(c) Show comments: %s\n", Yesno(*opt_display_comments));
	Po("\n");
	Po("(v) Verbose program output: %s\n", Yesno(*opt_be_verbose));
	Po("(s) Display system output: %s\n", Yesno(*opt_show_system_output));
	Po("\n");
	Po("(t) Timer displaying : %s", tdisplay);

	getyx(windows.main->name, lines, x);

	return lines;
}

/*
 * Draws the main program's border and prints the program's name.
 */
static void draw_static_border(void)
{
	int i;
	int middle_line_y = 1 + windows.main->lines;


	erase();

	/* Surrounding box. */
	box(stdscr, ACS_VLINE, ACS_HLINE);

	/* Middle horizontal line. */
	mvhline(middle_line_y, 1, ACS_HLINE, windows.max_cols - 2);
	Xmvaddch(middle_line_y, 0, ACS_LTEE);
	Xmvaddch(middle_line_y, windows.max_cols - 1, ACS_RTEE);

	/* Program's border and its name. */
	if ((i = windows.max_cols - strlen(PACKAGE_STRING) - 6) > 1) {
		Xmvaddch(0, i, ACS_RTEE);
		mvprintw(0, i + 1, " " PACKAGE_STRING " ");
		Xmvaddch(0, i + 1 + 1 + strlen(PACKAGE_STRING) + 1, ACS_LTEE);
	}

	if (*opt_display_options_line) { /* Border of option indicators. */
		int begin = strlen(OPTIONS_SPACE) + 4;
		if ((i = windows.max_cols - begin) > 1) {
			Xmvaddch(middle_line_y, i, ACS_RTEE);
			Xmvaddstr(middle_line_y, i + 1, OPTIONS_SPACE);
			Xmvaddch(middle_line_y, i + begin - 3 , ACS_LTEE);
		}
	}

	if (*opt_display_timer) { /* Timer's and backend status border. */
		int ts = strlen(TIMERS_SPACE);
		if ((i = windows.max_cols - ( ts + 3 )) > 1) {
			Xmvaddch(windows.max_lines - 1, i, ACS_RTEE);
			Xmvaddstr(windows.max_lines - 1, i + 1, TIMERS_SPACE);
			Xmvaddch(windows.max_lines - 1, i + ts , ACS_LTEE);
		}
	}

	refresh();
}

/*
 * Draws a small backend's status.
 */
static void draw_backend_status(void)
{
	int row = windows.max_lines - 1;
	int col = windows.max_cols - 6;


	if (*opt_display_timer == TIMER_NONE) {
		return;
	}

	if (col < 15) { /* Not enough space. */
		return;
	}

	switch (backend_status) {
		case BACKEND_RUNNING:
			Xmvaddstr(row, col, "|>");
			break;
		case BACKEND_PAUSED:
			Xmvaddstr(row, col, "||");
			break;
		case BACKEND_STOPPED:
			Xmvaddstr(row, col, "[]");
			break;
	}

	refresh();
}

/*
 * Timer functions.
 */

static INLINE char *timer_convert(double sec)
{
	int h, m, s;
	char *str = xmalloc(9);

	h = sec / 3600;
	m = (sec -= h * 3600) / 60;
	s = sec - m * 60;

	snprintf(str, 9, "%02d:%02d:%02d", h, m, s);

	return str;
}

static double timer_display(int backend_running)
{
	double curr = (difftime(time(NULL), timer.backend_started) - timer.current_paused);


	if (! backend_running) {
		char *t = timer_convert(timer.total_executing);

		Nprint("Total time executing: %s", t);

		xfree(t);

	} else {
		char *c = timer_convert(curr);
		char *t = timer_convert(curr + timer.total_executing);

		Nprint("Executing for %s, total %s.", c, t);

		xfree(c);
		xfree(t);
	}

	return curr;
}

static INLINE void timer_end(int backend_running)
{
	double last = timer_display(backend_running);

	timer.timer_paused = 0;

	timer.current_executing = last;
	timer.total_executing += last;
	timer.current_paused = 0;
}

static INLINE void timer_begin(void)
{
	timer.backend_started = time(NULL);
}

static INLINE void timer_reset(void)
{
	timer.backend_started = time(NULL);
	timer.timer_paused = 0;

	timer.current_executing = 0;
	timer.total_executing = 0;
	timer.current_paused = 0;
}

void timer_pause(void)
{
	timer.timer_paused = time(NULL);
}

void timer_resume(void)
{
	timer.current_paused += difftime(time(NULL), timer.timer_paused);
	timer.timer_paused = 0;
}

static void draw_timer(void)
{
	int row = windows.max_lines - 1;
	int col = windows.max_cols - 15;


	if (*opt_display_timer != TIMER_NONE && col >= 6) {
		char *str;
		double curr, total;


		if (Backend_exists) {
			curr = difftime(time(NULL), timer.backend_started) - timer.current_paused;
			total = timer.total_executing + curr;
		} else {
			curr = timer.current_executing;
			total = timer.total_executing;
		}

		switch (*opt_display_timer) {
			case TIMER_CURRENT:
				mvaddch(row, col - 2, 'C');
				str = timer_convert(curr);
				break;

			case TIMER_TOTAL:
				mvaddch(row, col - 2, 'T');
				str = timer_convert(total);
				break;

			default:
				ASSERT(0);
				mvaddch(row, col - 2, '?');
				str = timer_convert(0);
		}

		mvprintw(row, col, "%s", str);

		xfree(str);

		move(0,0); /* Move the cursor out of the way,
			    * as the timer is constantly being updated.
			    */
		refresh();
	}
}

/*
 *
 */

static void redisplay_all(void)
{
	draw_static_border();

	draw_backend_status();
	draw_timer();

	if (*opt_display_profile) {
		draw_profile_name(get_profile_by_element(Current_element));
	}
	if (*opt_display_options_line) {
		draw_options_indicators();
	}

	windows.main->ref(displayed.top);
	windows.status->ref(0);
}

static INLINE void toggle_timer(void)
{
	switch (*opt_display_timer) {
		case TIMER_NONE:
			*opt_display_timer = TIMER_TOTAL;
			Nprint("Displaying total time of execution.");
			break;

		case TIMER_TOTAL:
			*opt_display_timer = TIMER_CURRENT;
			Nprint("Displaying current time of execution.");
			break;

		case TIMER_CURRENT:
			*opt_display_timer = TIMER_NONE;
			Nprint("Not displaying timer.");
			break;

	}

	/* We need this for changing timer's border. */
	redisplay_all();
}

static int do_change_option(int input)
{
	int ok = 0;


	switch (input) {
		case 'c':
			toggle_comments();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;
		case 'a':
			toggle_alfs();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;
		case 'd':
			/* TODO: toggle_doctype(); */
			Nprint("No doctype available yet.");
			break;

		case 'v':
			toggle_verbosity();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 'w':
			toggle_logging();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 'o':
			toggle_following();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 'f':
			toggle_logging_files();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 'F':
			change_find_options();
			break;

		case 'h':
			toggle_logging_actions();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 's':
			toggle_system_output();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 'j':
			toggle_jump_to_element();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 'B':
			toggle_backend_logging();
			if (*opt_display_options_line) {
				draw_options_indicators();
			}
			break;

		case 't':
			toggle_timer();
			draw_timer();
			break;

		default:
			ok = -1;
	}

	return ok;
}

static INLINE void change_options(void)
{
	int input;
	int lines, top = 0;


	windows.active = TMP_WINDOW;

	/* Recreate main pad if it is too small. */
	if (total_number_of_elements() < min_pad_size_for_options) {
		recreate_main_window(min_pad_size_for_options);
	}

	lines = print_options();

	while ((input = tmp_window_driver(lines, &top, NULL)) != -1) {
		if (do_change_option(input) != 0) {
			Nprint_warn("No such option.");
		} else {
			/* Option changed, print them again. */
			lines = print_options();
		}
	}

	windows.active = MAIN_WINDOW;
}



/* Reads all data and control messages already sent by backend. */
static void read_all_messages(void)
{
	int i = 1;
	int j = 1;


	while (i || j) {
		j = handle_ctrl_msg();
		i = handle_data_msg();
	}
}

static inline void change_run_status_to_failed(element_s *el)
{
	element_s *parent;


	set_run_status(el, RUN_STATUS_FAILED);

	for (parent = el->parent; parent; parent = parent->parent) {
		set_run_status(parent, RUN_STATUS_FAILED);
	}
}

static int wait_for_the_backend(void)
{
	int status;
	pid_t got_pid;


	if ((got_pid = waitpid(backend_pid, &status, 0)) == -1) {
		Fatal_error("waitpid() for %ld failed: %s",
			(long)backend_pid, strerror(errno));
	}

	if (WIFEXITED(status)) { /* Backend exited normally. */
		if (WEXITSTATUS(status)) {
			Nprint_err("Execution failed (%d).",
				WEXITSTATUS(status));
			status = -1;
		} else {
			Nprint("Execution successfully done.");
			status = 0;
		}

	} else if (WIFSIGNALED(status)) {
		Nprint_err("Backend (%ld) killed by signal "
			"%d%s.", (long)got_pid, WTERMSIG(status),
			WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_err("Backend (%ld) stopped by signal "
			"%d.", (long)got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_err("Backend exited abnormally "
			"(crashed or got fatal error).");
		status = -1;
	}

	backend_pid = 0;

	timer_end(Backend_exists);

	if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
		Fatal_error("signal() failed");
	}

	/*
	 * When execution is stopped before ending normally,
	 * "element failed" message is not received.  That's why
	 * marking of the current running element (and its parents)
	 * has to be performed here.
	 */
	if (status != 0) {
		change_run_status_to_failed(current_running);
	}

	if (*opt_beep_when_done) {
		if (*opt_run_interactive) {
			beep();

		} else {
			printf("\a");
			fflush(stdout);
		}
	}

	return status;
}



int get_key(WINDOW *win)
{
	int input;


	while ((input = wgetch(win)) == ERR) {
		if (Backend_exists) {
			draw_timer();

			handle_ctrl_msg();
			handle_data_msg();
		}

		if (got_sigchld) {
			got_sigchld = 0;

			read_all_messages();

			wait_for_the_backend();

			clear_should_run_marks(root_element);

			if (Can_do_rewrite) {
				rewrite_main();
			}

			backend_status = BACKEND_STOPPED;

			draw_backend_status();
			draw_timer();

			return ERR;
		}

		napms(1);
	}

	return input;
}

/*
 * Status window's main loop.
 */

static void status_window(void)
{
	int input;
	int offset = 0;


	windows.active = STATUS_WINDOW;

	while (1) {
		input = get_key(windows.status->name);

		switch (input) {
			case 'k':
			case KEY_UP:
				++offset;
				offset = windows.status->ref(offset);
				break;

			case 'j':
			case '\n':
			case KEY_DOWN:
				--offset;
				offset = windows.status->ref(offset);
				break;

			case 'b':
			case MOD_CTRL('p'):
			case KEY_PPAGE:
				offset += windows.status->lines / 2;
				offset = windows.status->ref(offset);
				break;

			case ' ':
			case MOD_CTRL('n'):
			case KEY_NPAGE:
				offset -= windows.status->lines / 2;
				offset = windows.status->ref(offset);
				break;

			case KEY_END:
				offset = windows.status->ref(0);
				break;

			case 'q':
			case 'w':
			case KEY_F(10):
				windows.active = MAIN_WINDOW;
				return;
		}
	}
}

/*
 * Writing element's info.
 */

static void write_extra_element_info(element_s *el)
{
	Xwaddstr(windows.main->name, "Type          : ");
	switch (el->type) {
		case TYPE_ROOT:
			Xwaddstr(windows.main->name, "Root element\n");
			break;
		case TYPE_PROFILE:
			Xwaddstr(windows.main->name, "Profile\n");
			break;
		case TYPE_ELEMENT:
			Xwaddstr(windows.main->name, "Element\n");
			break;
		case TYPE_COMMENT:
			Xwaddstr(windows.main->name, "Comment\n");
			break;
		case TYPE_DOCTYPE:
			Xwaddstr(windows.main->name, "Doctype\n");
			break;
		case TYPE_ENTITY:
			Xwaddstr(windows.main->name, "Entity\n");
			break;
		case TYPE_UNKNOWN:
			Xwaddstr(windows.main->name, "Unknown\n");
			break;
	}

	Xwprintw(windows.main->name, "Marked        : %s\n",
		Yesno(el->marked));

	Xwaddstr(windows.main->name, "Run status    : ");
	switch (el->run_status) {
		case RUN_STATUS_FAILED:
			Xwaddstr(windows.main->name, "Failed\n");
			break;
		case RUN_STATUS_NONE:
			Xwaddstr(windows.main->name, "None\n");
			break;
		case RUN_STATUS_SOME_DONE:
			Xwaddstr(windows.main->name,
				"Some are done, but not all\n");
			break;
		case RUN_STATUS_DONE:
			Xwaddstr(windows.main->name, "Done\n");
			break;
		case RUN_STATUS_RUNNING:
			Xwaddstr(windows.main->name, "Running\n");
			break;
	}

	Xwprintw(windows.main->name, "Should run    : %s\n",
		Yesno(el->should_run));

	Xwprintw(windows.main->name, "Hide children : %s\n",
		Yesno(el->hide_children));
}

static void build_description_aux(char **pcontent, element_s *node, int indent)
{
#if 0
	static const char *spaces = "            ";

	if (indent > 12) {
		indent = 12;
	}

	if (strcmp(node->handler->name, "para") == 0) {
		char *s = xstrdup(node->content);

		if (! Empty_string(s)) {
			char *str = strtok(s, "\n\r");
			while (str) {
				SKIPWS(str);

				if (*str) {
					if (indent > 0) {
						append_str(pcontent, &spaces[12 - indent]);
					}
					append_str(pcontent,(const char *)str);
					append_str(pcontent,"\n");
				}

				str = strtok(NULL, "\n\r");
			}
		}

		xfree(s);
	}

	if (strcmp(node->handler->name, "list") == 0) {
		element_s *child;
		for (child = node->children; child; child = child->next) {
			build_description_aux(pcontent, child, indent + 4);
		}
	}

	if (strcmp(node->handler->name, "item") == 0) {
		char *s = xstrdup(node->content);

		if (! Empty_string(s)) {
			int first = 1;
			char *str = strtok(s, "\n\r");

			while (str) {
				SKIPWS(str);

				if (*str) {
					if (indent > 0) {
						if (first) {
							append_str(pcontent, "  - ");
							first = 0;
							append_str(pcontent, &spaces[12 - indent + 4]);
						}
						else {
							append_str(pcontent, &spaces[12 - indent]);
						}
					}
					append_str(pcontent,(const char *)str);
					append_str(pcontent,"\n");
				}

				str = strtok(NULL, "\n\r");
			}
		}

		xfree(s);
	}
#endif
}

enum print_info_amount {
	PRINT_MORE_INFO,
	PRINT_LESS_INFO
};

static int write_element_info(element_s *el, enum print_info_amount print_extra_info)
{
	int lines, i;

	Xwerase(windows.main->name);
	Xwmove(windows.main->name, 0, 0);

	if (print_extra_info == PRINT_MORE_INFO) {
		write_extra_element_info(el);
		Xwaddch(windows.main->name, '\n');
	}
	
	Xwprintw(windows.main->name, "Element name: %s (syntax version %s)\n\n",
		 el->handler->name, el->handler->syntax_version);

	if (el->handler->data && HDATA_DISPLAY_DETAILS) {
		char *details;

		details = el->handler->alloc_data(el, HDATA_DISPLAY_DETAILS);
		Xwaddstr(windows.main->name, details);
		xfree(details);
	}

#if 0
	if (Is_parameter_name(el, "description")
	&& el->parent && Is_parameter_name(el->parent, "packageinfo")) {
		char *s = build_description(el);

		Xwprintw(windows.main->name,
			"Package description\n-------------------\n%s",
			Empty_string(s) ? el->content : s);

		xfree(s);
	}
#endif

	getyx(windows.main->name, lines, i);

	return lines;
}

static void display_element_info(element_s *el, enum print_info_amount print_extra_info)
{
	int lines, top = 0;

	windows.active = TMP_WINDOW;

	/* Recreate main pad if it is too small. */
	if (total_number_of_elements() < min_pad_size_for_elements_info) {
		recreate_main_window(min_pad_size_for_elements_info);
	}

	lines = write_element_info(el, print_extra_info);
	
	tmp_window_driver(lines, &top, NULL);

	windows.active = MAIN_WINDOW;
}

/*
 *
 */

static void signal_chld(int sig)
{
	(void)sig;

	got_sigchld = 1;
}

static void start_executing(void)
{
	current_running = root_element;

	comm_create_socket_pairs();

	/* Start handling SIGCHLD. */
	if (signal(SIGCHLD, signal_chld) == SIG_ERR) {
		Fatal_error("signal() failed");
	}

	timer_begin();

	backend_pid = fork();

	if (backend_pid == -1) {
		Fatal_error("fork() failed: %s", strerror(errno));

	} else if (backend_pid == 0) { /* Child. */
		start_backend(root_element);
		Nprint_warn("start_backend() returned.");
	}

	backend_status = BACKEND_RUNNING;

	if (*opt_run_interactive) {
		draw_backend_status();
	}
}

static void do_mark_for_running(element_s *el)
{
	element_s *child;


	if (el->run_status == RUN_STATUS_DONE)
		return;

	if ((el->handler->type & HTYPE_NORMAL) == 0)
		return;

	el->should_run = 1;

	for (child = el->children; child; child = child->next) {
		do_mark_for_running(child);
	}
}

static INLINE void start_executing_marked(void)
{
	element_s *el;
	int num_marked = 0;


	for (el = root_element; el; el = get_next_element(el)) {
		if (el->marked) {
			el->should_run = 1;
			el->run_status = RUN_STATUS_NONE;
			++num_marked;
		}
	}

	if (num_marked == 0) {
		Nprint_warn("You have to mark some elements first.");
		return;
	}
	rewrite_main();

	Nprint("");
	Nprint("Starting execution of marked elements...");
	Nprint("");

	start_executing();
}

static void start_executing_children(element_s *el)
{
	element_s *parent;

	if (el->run_status == RUN_STATUS_DONE) {
		Nprint_warn("Nothing to run. "
		"Use 's' -> 'f' to force running of already run elements.");
		return;
	}
	if ((el->handler->type & HTYPE_NORMAL) == 0) {
		Nprint_warn("This is not an element that can run.");
		return;
	}

	/* Mark all element parents. */
	for (parent = el->parent; parent; parent = parent->parent) {
		parent->should_run = 1;
	}

	/* Mark element and all its children. */
	do_mark_for_running(el);


	Nprint("");
	Nprint("Executing %s...", el->handler->name);
	Nprint("");

	start_executing();
}

static INLINE void start_executing_next(element_s *el)
{
	element_s *tmp_el;


	if (el->run_status == RUN_STATUS_DONE) {
		Nprint_warn("Nothing to run. "
		"Use 's' -> 'f' to force running of already run elements.");
		return;
	}
	if ((el->handler->type & HTYPE_NORMAL) == 0) {
		Nprint_warn("This is not an element that can run.");
		return;
	}


	/* Mark all element parents. */
	for (tmp_el = el->parent; tmp_el; tmp_el = tmp_el->parent) {
		tmp_el->should_run = 1;
	}

	/* Mark element, all its children and all elements after it. */
	tmp_el = el;
	do {
		do_mark_for_running(tmp_el);
		tmp_el = tmp_el->next;
	} while (tmp_el);


	Nprint("");
	Nprint("Executing from %s...", el->handler->name);
	Nprint("");

	start_executing();
}

/*
 * Profile's functions (adding, removing, reloading, moving).
 */

static INLINE int reload_profile(element_s *old_profile)
{
	element_s *new_profile;


	if ((new_profile = parse_profile(old_profile->handler->name)) == NULL) {
		Nprint_err("Parsing %s failed.", old_profile->handler->name);
		return -1;
	}

	new_profile->parent = old_profile->parent;
	new_profile->prev = old_profile->prev;
	new_profile->next = old_profile->next;

	if (old_profile->prev == NULL) {
		/* It's a first profile. */
		old_profile->parent->children = new_profile;
	} else {
		old_profile->prev->next = new_profile;
	}

	if (old_profile->next) {
		old_profile->next->prev = new_profile;
	}

	free_element(old_profile);

	return 0;
}

static element_s *do_add_profile(const char *profile)
{
	struct stat file_stat;
	element_s *new_profile;


	if (stat(profile, &file_stat) == 0) {
		if (S_ISDIR(file_stat.st_mode)) {
			Nprint("%s is a directory, ignoring it.", profile);
			return NULL;
		}
	}

	if ((new_profile = parse_profile(profile)) == NULL) {
		return NULL;
	}

	add_profile(new_profile);

	Nprint("Profile added: %s", profile);

	return new_profile;
}

/*
 * If name is a file, adds that single profile.  If it's
 * a directory, reads all .xml files in it, non-recursively.
 * Returns number of added profiles.
 */
static int add_profile_from_path(const char *name)
{
	int num = 0;
	struct stat file_stat;

	if (stat(name, &file_stat) == 0) {
		if (S_ISDIR(file_stat.st_mode)) {
			struct dirent *next;
			DIR *dir = opendir(name);

			if (dir != NULL) {
				while ((next = xreaddir(dir, name, ".xml"))) {
					char *f = xstrdup(name);
					append_str(&f, "/");
					append_str(&f, next->d_name);

					if (do_add_profile(f)) {
						++num;
					}

					xfree(f);
				}

				closedir(dir);
			}

		} else {
			do_add_profile(name);
		}

	} else {
		Nprint_err("Can't get information about %s:", name);
		Nprint_err("%s", strerror(errno));
	}

	return num;
}

static INLINE int add_new_profile(void)
{
	int num = 0;
	char *filename = NULL;
	

	get_string_from_bottom("Profile to add:", &filename);

	if (! Empty_string(filename)) {
		char *fullname = NULL;
		char *base = xstrdup(*opt_profiles_directory);
	
		/* Lying and cheating RFC 2396, telling xmlBuildURI() that
		 * the _whole_ string should be used as a base directory.
		 */
		if (base[strlen(base)-1] != '/') {
			append_str(&base, "/");
		}

		fullname = (char *) xmlBuildURI(
			(const xmlChar *)filename,
			(const xmlChar *)base);

		num = add_profile_from_path(fullname);

		xfree(fullname);
		xfree(base);
	}

	xfree(filename);

	return num;
}

static INLINE int move_profile_up(element_s *profile)
{
	element_s *pp, *p, *n;


	/* First profile, can't move it up. */
	if (profile->prev == NULL) {
		return -1;
	}

	/* Second profile. */
	if (profile->prev->prev == NULL) {
		/* Now it's first, update root element. */
		profile->parent->children = profile;
	}


	pp = profile->prev->prev;
	p = profile->prev;
	n = profile->next;

	profile->next = p;
	profile->prev = p->prev;

	if (pp) {
		pp->next = profile;
	}

	if (p) {
		p->next = n;
		p->prev = profile;
	}

	if (n) {
		n->prev = p;
	}

	return 0;
}

static INLINE int move_profile_down(element_s *profile)
{
	element_s *p, *n, *nn;


	/* Last profile, can't move it down. */
	if (profile->next == NULL) {
		return -1;
	}

	/* First profile. */
	if (profile->prev == NULL) {
		/* Now it's second, update root element. */
		profile->parent->children = profile->next;
	}


	p = profile->prev;
	n = profile->next;
	nn = profile->next->next;

	profile->next = nn;
	profile->prev = n;

	if (p) {
		p->next = n;
	}

	if (n) {
		n->next = profile;
		n->prev = p;
	}

	if (nn) {
		nn->prev = profile;
	}

	return 0;
}

/*
 * Searching.
 */

enum search_type {
	SEARCH_NOTHING = 0,
	SEARCH_ELEMENT_NAME,
	SEARCH_ATTRIBUTES,
	SEARCH_CONTENT = 4,
	SEARCH_FOR_PACKAGE = 8,
	SEARCH_FOR_FULL_PACKAGE = 16
};

static int search_matches(element_s *el, const char *string)
{
	int offset = 0;
	enum search_type what = SEARCH_NOTHING;


	if (string[0] == '~') {
		switch (string[1]) {
			case 'e':
				what |= SEARCH_ELEMENT_NAME;
				break;
			case 'a':
				what |= SEARCH_ATTRIBUTES;
				break;
			case 'c':
				what |= SEARCH_CONTENT;
				break;
			case 'p':
				what |= SEARCH_FOR_PACKAGE;
				break;
			case 'P':
				what |= SEARCH_FOR_FULL_PACKAGE;
				break;
			default:
				Nprint_err("No such search operation.");
				return -1;
		}

		if (string[2] != ' ' || ! string[3]) {
			Nprint_err("Search for what?");
			return -1;
		}

		offset = 3; // Use the string after "~x ".

	} else {
		what |= SEARCH_ELEMENT_NAME;
		what |= SEARCH_ATTRIBUTES;
		what |= SEARCH_CONTENT;

		offset = 0; // Use the whole string.
	}

	if (what & SEARCH_ELEMENT_NAME) {
		if (xstrcasestr(el->handler->name, string + offset)) {
			return 1;
		}
	}

/* TODO: handle content from handler
	if ((what & SEARCH_ATTRIBUTES) && el->attr) {
		int i;

		for (i = 0; el->attr[i]; ++i) {
			if (xstrcasestr(el->attr[i], string + offset)) {
				return 1;
			}
		}
	}
*/

#if 0
	if ((what & SEARCH_CONTENT) && el->content) {
		if (xstrcasestr(el->content, string + offset)) {
			return 1;
		}
	}
#endif

	if ((what & SEARCH_FOR_PACKAGE) && (el->handler->type & HTYPE_PACKAGE)) {
		int match;
		char *name = alloc_package_name(el);

		match = xstrcasestr(name, string + offset) != NULL;

		xfree(name);

		if (match) {
			return 1;
		}
	}

	if ((what & SEARCH_FOR_FULL_PACKAGE) && (el->handler->type & HTYPE_PACKAGE)) {
		int match;
		char *name = alloc_package_name(el);

		match = strcasecmp(name, string + offset) == 0;

		xfree(name);

		if (match) {
			return 1;
		}
	}

	return 0;
}

static element_s *search_forwards(const char *string, element_s *el)
{
	int match = 0;
	element_s *start, *tmp;


	start = tmp = get_next_element(el);

	do {
		if (tmp == NULL) { /* Past the last one. */
			Nprint("Search wrapped to top.");
			tmp = root_element;
		}

		if (! should_skip_element(tmp, NULL)) {
			match = search_matches(tmp, string);

			if (match == 1) {
				break;
			} else if (match == -1) {
				return NULL;
			}
		}

		tmp = get_next_element(tmp);

	} while (tmp != start);

	if (! match) {
		Nprint("String not found.");
		tmp = NULL;
	}

	return tmp;
}

static element_s *search_backwards(const char *string, element_s *el)
{
	int match = 0;
	element_s *start, *tmp;
	element_s *last = NULL;


	/* Get the last element. */
	for (tmp = root_element; tmp; tmp = get_next_element(tmp)) {
		last = tmp;
	}

	start = tmp = get_prev_element(el);

	do {
		if (tmp == NULL) { /* Past the first one. */
			Nprint("Search wrapped to bottom.");
			tmp = last;
		}

		if (! should_skip_element(tmp, NULL)) {
			match = search_matches(tmp, string);

			if (match == 1) {
				break;
			} else if (match == -1) {
				return NULL;
			}
		}

		tmp = get_prev_element(tmp);

	} while (tmp != start);

	if (! match) {
		tmp = NULL;
		Nprint("String not found.");
	}

	return tmp;
}

/*
 * Opening and closing all elements.
 */

static void close_all_elements(element_s *el)
{
	element_s *child;

	el->hide_children = 1;

	for (child = el->children; child; child = child->next) {
		close_all_elements(child);
	}
		
}

static void open_all_elements(element_s *el)
{
	element_s *child;

	el->hide_children = 0;

	for (child = el->children; child; child = child->next) {
		open_all_elements(child);
	}
}



/*
 * Main input loop.
 */
static int browse(void)
{
	int input;


	windows.active = MAIN_WINDOW;

	recreate_main_window(total_number_of_elements());

	draw_backend_status();
	draw_timer();

	if (*opt_display_options_line) {
		draw_options_indicators();
	}

	if (*opt_expand_profiles) {
		element_s *profile;

		for (profile = root_element->children;
		     profile;
		     profile = profile->next) {
			profile->hide_children = 0;
		}
	}

	rewrite_main();

	if (*opt_print_startup_help) {
		Nprint("For help, type '?'.");
	}

	if (*opt_start_immediately) {
		start_executing_children(root_element);
	}

	while (1) {
		fix_top_and_cursor(&displayed.current);

		print_cursor();

		if (*opt_display_profile) {
			draw_profile_name(get_profile_by_element(Current_element));
		}

		windows.main->ref(displayed.top);


		input = get_key(windows.main->name);

		remove_cursor();


		switch (input) {
		case ERR: /* Do nothing. */
			break;

		case 'k':
		case KEY_UP:
			--displayed.current;
			break;

		case 'j':
		case KEY_DOWN:
			++displayed.current;
			break;

		case MOD_CTRL('p'):
		case KEY_PPAGE:
			if (displayed.current != displayed.top) {
				displayed.current = displayed.top;
			} else {
				displayed.current -= windows.main->lines - 1;
			}
			break;

		case MOD_CTRL('n'):
		case KEY_NPAGE:
			if (displayed.current != displayed.top + windows.main->lines - 1) {
				displayed.current = displayed.top + windows.main->lines - 1;
			} else {
				displayed.current += windows.main->lines - 1;
			}
			break;

		case KEY_HOME:
			displayed.current = 0;
			break;

		case KEY_END:
			displayed.current = displayed.total - 1;
			break;

		/* Toggle tree collapsing or display element info */
		case ' ':
		case '\n':
			/* Doesn't have children - display its info. */
			if (Current_element->children == NULL) {
				display_element_info(Current_element, PRINT_LESS_INFO);
				rewrite_main();
				break;
			}

			Toggle(Current_element->hide_children);

			rewrite_main();

			break;

		case 'l':
		case KEY_RIGHT:
			if (Current_element->children == NULL) {
				display_element_info(Current_element, PRINT_LESS_INFO);
				rewrite_main();
				break;
			}

			Current_element->hide_children = 0;

			rewrite_main();

			break;

		case 'h':
		case KEY_LEFT:
			if (Current_element->children != NULL
			&& ! Current_element->hide_children) {
				/* It's an open element, close it. */
				Current_element->hide_children = 1;

			} else {
				element_s *current = Current_element;

				if (current->type == TYPE_PROFILE) {
					/* It's the (closed) profile. */
					break;
				}

				do {
					current = current->parent;
				} while (find_cursor(current) == -1);

				current->hide_children = 1;

				displayed.current = find_cursor(current);
			}

			rewrite_main();

			break;

		case 'J': /* Jump to next jump_to element. */
			jump_to_element(JUMP_FORWARD);

			break;

		case 'K': /* Jump to previous jump_to element. */
			jump_to_element(JUMP_BACKWARD);

			break;

		case 'H':
			close_all_elements(Current_element);

			rewrite_main();

			break;

		case 'L':
			open_all_elements(Current_element);

			rewrite_main();

			break;

		case ';': /* Jump to last jump-to element. */
			jump_to_last_element();

			break;

		case 's': /* Start running. */
			if (Backend_exists) {
				Nprint_warn("Already running.");
				break;
			}

			Nprint("Run (m)arked elements, element and "
				"it's (c)hildren, element's children");
			Nprint("and (n)ext elements, or (f)orce "
				"running of current element? ");

			input = get_key(windows.main->name);

			if (input == 'm') {
				start_executing_marked();

			} else if (input == 'c') {
				start_executing_children(Current_element);

			} else if (input == 'n') {
				start_executing_next(Current_element);

			} else if (input == 'f') {
				clear_done_run_status(Current_element);
				rewrite_main();

				start_executing_children(Current_element);

			} else {
				Nprint_warn("No such option.");
			}

			break;

		case 'S': /* Stop running. */
			if (! Backend_exists) {
				Nprint_warn("Not running.");
				break;
			}

			/* Send the stop request to the backend. */
			comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_STOP, "");

			break;

		case 'p': /* Pause backend's output. */
			if (! Backend_exists) {
				Nprint_warn("Not running.");
				break;
			}

			comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_PAUSE, "");

			if (backend_status == BACKEND_PAUSED) {
				backend_status = BACKEND_RUNNING;
			} else if (backend_status == BACKEND_RUNNING) {
				backend_status = BACKEND_PAUSED;

			}

			draw_backend_status();

			break;

		case '*': /* Mark current element. */
		case KEY_IC:
			do_mark_element(Current_element);
			++displayed.current;

			rewrite_main();

			break;

		case 'u': /* Unmark successfully ended elements. */
			Nprint_warn("Unmark successfully ended elements "
				"('y' to confirm) ?");

			input = get_key(windows.status->name);

			if (input != 'y' && input != 'Y') {
				Nprint("Unmarking aborted.");
				break;
			}

			clear_done_marks(root_element);
			rewrite_main();

			Nprint("Successfully ended elements unmarked.");

			break;

		case 'U': /* Unmark all elements. */
			Nprint_warn("Unmark all elements "
				"('y' to confirm) ?");

			input = get_key(windows.status->name);

			if (input != 'y' && input != 'Y') {
				Nprint("Unmarking aborted.");
				break;
			}

			unmark_element(root_element, 1); /* All. */
			rewrite_main();

			Nprint("All elements unmarked.");

			break;

		case 'e': /* Edit current element. */
#ifdef USE_EDITOR
			if (Backend_exists
			&& get_profile_by_element(current_running)
			== get_profile_by_element(Current_element)) {
				Nprint_warn("Can't edit running profile.");
				break;
			}

			if (Current_element->type == TYPE_ENTITY) {
				Nprint_warn(
				"This won't do what you want.");
				Nprint_warn(
				"Changing this value won't affect "
				"elements that use it.");
			}

			edit_element(Current_element);

			rewrite_main();
#else
			Nprint_warn(
			"nALFS was built with --disable-element-editor.");
#endif
			break;

		case 'E': /* Start editor on element's profile. */
			{
				element_s *profile =
				get_profile_by_element(Current_element);

				run_editor(profile->handler->name);
			}


			break;

		case 'i': // Display info about the current element.
			display_element_info(Current_element, PRINT_LESS_INFO);

			rewrite_main();

			break;

		case 'I': // Display info about the current element.
			display_element_info(Current_element, PRINT_MORE_INFO);

			rewrite_main();

			break;
#if 0
		case 'x': // Display XML for the elements under the cursor.
			display_element_in_xml(Current_element);

			rewrite_main();

			break;
#endif
		case 'm': /* Change run-status marks. */
			if (Current_element->should_run) {
				Nprint_warn("Can't change status mark "
				"of this element now - it's marked for "
				"running.");
				break;
			}

			Nprint("Set run-status marks to "
				"(d)one, (f)ailed or (n)one ? ");

			switch (get_key(windows.main->name)) {
				case 'd':
					change_run_status_marks(
						Current_element,
						RUN_STATUS_DONE);
					break;

				case 'f':
					change_run_status_marks(
						Current_element,
						RUN_STATUS_FAILED);
					break;

				case 'n':
					change_run_status_marks(
						Current_element,
						RUN_STATUS_NONE);
					break;

				default:
					Nprint_warn("No such option.");
			}

			rewrite_main();

			break;

		case 'o': /* Quickly change the option. */
			Nprint("Quick option change - enter the option. ");

			input = get_key(windows.status->name);

			if (do_change_option(input) != 0) {
				Nprint_warn("No such option.");
			}

			break;

		case 'O': /* Enter options' menu. */
			change_options();

			rewrite_main();

			break;

		case 't':
			timer_display(Backend_exists);

			break;

		case 'T':
			timer_reset();

			draw_timer();

			Nprint("Timer reset.");

			break;

		case '-': /* Move current profile up. */
		{
			element_s *current = Current_element;

			if (move_profile_up(get_profile_by_element(current))) {
				Nprint_warn("Already on top.");
			} else {
				rewrite_main();
				displayed.current = find_cursor(current);
			}

			break;
		}

		case '+': /* Move current profile down. */
		case '=':
		{
			element_s *current = Current_element;

			if (move_profile_down(get_profile_by_element(current))) {
				Nprint_warn("Already at the bottom.");
			} else {
				rewrite_main();
				displayed.current = find_cursor(current);
			}
			break;
		}

		case 'a': /* Add a profile. */
			if (Backend_exists) {
				Nprint_warn(
				"Can't add profiles while running.");
				break;
			}

			if (add_new_profile() <= 0) {
				Nprint("No profiles added.");
				break;
			}

			recreate_main_window(total_number_of_elements());
			rewrite_main();

			break;

		case 'd': /* Remove the profile from the list. */
		case KEY_DC:
		{
			element_s *el = get_profile_by_element(Current_element);

			if (el->should_run) {
				Nprint_warn(
				"Can't remove profile scheduled to run.");
				break;
			}

			/* Single profile? */
			if (root_element->children
			&& ! root_element->children->next) {
				Nprint_warn("Can't remove last profile.");
				break;
			}

			Nprint_warn("Delete current profile "
				"('y' to confirm) ?");

			input = get_key(windows.status->name);

			if (input != 'y' && input != 'Y') {
				Nprint("Deleting aborted.");
				break;
			}

			remove_profile(el);

			/*
			 * No more profiles.
			 * TODO: Allow empty profiles' list.
			 */
/*			if (root_element->children == NULL) {
				i_need_a_profile();
			}
*/
			recreate_main_window(total_number_of_elements());

			rewrite_main();

			Nprint("Profile removed.");

			break;
		}

		case 'r': /* Reload the profile. */
		{
			element_s *el = get_profile_by_element(Current_element);

			if (Backend_exists
			&& get_profile_by_element(current_running) == el) {
				Nprint_warn("Can't reload while running.");
				break;
			}

			displayed.current = find_cursor(el);

			if (reload_profile(el)) {
				Nprint_err("Reloading %s failed.", el->handler->name);
				break;
			}

			recreate_main_window(total_number_of_elements());

			rewrite_main();

			Nprint("Profile %s reloaded.", Current_element->handler->name);

			break;
		}

		case '/': /* Search forwards. */
			get_string_from_bottom("Search:", &search_string);

			if (search_string != NULL) {
				element_s *el =
				search_forwards(search_string, Current_element);

				if (el) {
					displayed.current = found_searched(el);
				}
			}

			break;

		case 'n': /* Search next forwards. */
		{
			element_s *el;

			if (search_string == NULL) {
				get_string_from_bottom(
					"Search:", &search_string);

				if (search_string == NULL) {
					break;
				}
			}

			el = search_forwards(search_string, Current_element);

			if (el) {
				displayed.current = found_searched(el);
			}

			break;
		}

		case 'N': /* Search next backwards. */
		{
			element_s *el;

			if (search_string == NULL) {
				get_string_from_bottom(
					"Search:", &search_string);

				if (search_string == NULL) {
					break;
				}
			}

			el = search_backwards(search_string, Current_element);

			if (el) {
				displayed.current = found_searched(el);
			}

			break;
		}

		case 'w': /* Switch windows */
			windows.main->ref(displayed.top); /* For removed cursor. */

			Nprint("Status window is now active.");
			status_window();
			Nprint("Main window is now active again.");

			break;

		case 'W': /* Change windows sizes. */
			input = get_key(windows.main->name);

			if (input == '+') {
				++(windows.middle_line_offset);
			} else if (input == '-' || input == '_') {
				--(windows.middle_line_offset);
			} else if (input == '=') {
				windows.middle_line_offset = 0;
			} else {
				Nprint_warn(
				"Use '+', '-' or '=' for changing the sizes.");
			}

			resize_all_windows();
			redisplay_all();

			break;

		case 'P': /* 'P'ackages. */
			if (Backend_exists) {
				Nprint_warn(
				"Can't manage packages while running.");
			} else {
				pkg_main();
				rewrite_main();
			}

			break;

		case '?': /* Show help in the main window. */
		case KEY_F(1):
			display_help_page();

			rewrite_main();

			break;


		case 'q': /* Quit program. */
		case KEY_F(10):
			if (Backend_exists) {
				Nprint_warn("Can't quit while running.");
				break;
			}

			Nprint_warn("Really quit ('y' to confirm) ?");

			while (1) {
				input = get_key(windows.main->name);

				if (input == 'y' || input == 'Y') {
					return 0;

				} else if (input != 'q' && input != KEY_F(10)) {
					break;
				}
			}

			Nprint("Good.");

			break;

		case MOD_CTRL('l'): /* Refresh screen. */
			endwin();
			refresh();

			keypad(windows.main->name, 1);

			break;

		default:
			Nprint_help("Huh? Press '?' for help.");
			break;
		}
	}

	return 0; /* Never reached. */
}

/*
 * Runs program in non-interactive, non-curses mode.
 */
static INLINE int run_non_interactively(void)
{
	if (root_element == NULL) {
		Nprint("No profiles specified.");
		return -1;
	}

	start_executing_children(root_element);

	while (1) {
		handle_ctrl_msg();
		handle_data_msg();

		if (got_sigchld) {
			break;
		}

		napms(1);
	}

	/* If there is something left to read, read it. */
	read_all_messages();

	return wait_for_the_backend();
}

/*
 * Prints a warning for each specified environment variable set.
 */
static INLINE void warn_if_variables_set(void)
{
	char *var, *variables;


	if (!*opt_warn_if_set_variables || !strlen(*opt_warn_if_set_variables)) {
		return;
	}

	variables = xstrdup(*opt_warn_if_set_variables);

	for (var = strtok(variables, WHITE_SPACE);
	     var;
	     var = strtok(NULL, WHITE_SPACE)) {
		char *value;

		if ((value = getenv(var)) && strlen(value)) {
			Nprint_warn("Environment variable \"%s\" "
				"is set to \"%s\".", var, value);
		}
	}

	xfree(variables);
}

/*
 * Some signal handlers.
 */

static void signal_winch(int sig)
{
	(void)sig;

	endwin();
	refresh();

	getmaxyx(stdscr, windows.max_lines, windows.max_cols);

	resize_all_windows();
	redisplay_all();

	/* TODO: endwin() sends garbage for some reason. */
	get_key(windows.main->name);

	keypad(windows.main->name, 1);
}

static void signal_hup(int sig)
{
	(void)sig;

	Nprint_warn("SIGHUP received.");
}

static void signal_quit(int sig)
{
	(void)sig;

	Nprint_warn("SIGQUIT received.");
}

static void signal_term(int sig)
{
	(void)sig;

	/* First, stop the backend. */
	comm_send_ctrl_msg(FRONTEND_CTRL_SOCK, CTRL_STOP, "");

	if (*opt_run_interactive) {
		end_display();
	}

	fprintf(stderr, "Program terminated.\n");

	exit(EXIT_FAILURE);
}

static void signal_int(int sig)
{
	(void)sig;

	if (*opt_run_interactive) {
		end_display();
	}

	fprintf(stderr, "Interrupt from keyboard received.\n");

	exit(EXIT_FAILURE);
}

static INLINE void set_main_signals(void)
{
	if (signal(SIGINT, signal_int) == SIG_ERR) {
		Fatal_error("signal() failed");
	}
	if (signal(SIGTERM, signal_term) == SIG_ERR) {
		Fatal_error("signal() failed");
	}
	if (signal(SIGQUIT, signal_quit) == SIG_ERR) {
		Fatal_error("signal() failed");
	}
	if (signal(SIGHUP, signal_hup) == SIG_ERR) {
		Fatal_error("signal() failed");
	}

	if (*opt_run_interactive) {
		if (signal(SIGWINCH, signal_winch) == SIG_ERR) {
			Fatal_error("signal() failed");
		}
	}
}

/*
 * Appends directories to opt_find_prunes.
 */
static INLINE void append_prune_dirs_from_file(void)
{
	FILE *fp;
	char *file = NULL;


	/* Get real file name. */
	if (*opt_find_prunes_file[0] == '/') {
		file = xstrdup(*opt_find_prunes_file);
	} else {
		append_str_format(&file, "%s/%s", *opt_alfs_directory, *opt_find_prunes_file);
	}

	/* Read directories from a file. */
	if ((fp = fopen(file, "r"))) {
		char *new_dirs = NULL;
		char line[PATH_MAX];

		while (fgets(line, sizeof line, fp)) {
			char *s;

			if ((s = alloc_trimmed_str(line))) {
				append_str(&new_dirs, s);
				append_str(&new_dirs, " ");
				xfree(s);
			}
		}
		
		fclose(fp);

		append_string_option(opt_find_prunes, new_dirs);

		xfree(new_dirs);
	}

	xfree(file);
}

/*
 * Initialize a state file by constructing its name and checking for its
 * existence.  Uses already set opt_alfs_directory.
 */
static INLINE void init_state_file(void)
{
	state.filename = NULL;
	append_str_format(&state.filename, "%s/%s",  *opt_alfs_directory, state_file_name);
	state.exists = file_exists(state.filename) ? 1 : 0;
}

/*
 * Printing functions.
 */

static void nprint_curses(msg_id_e mid, const char *format,...)
{
	char *file = alloc_real_status_logfile_name();
	FILE *fp = NULL;
	va_list ap;
        va_list ap2;

	if (*opt_log_status_window && (fp = fopen(file, "a")) == NULL) {
		*opt_log_status_window = 0;
		nprint_curses(T_WAR,
			"Unable to open \"%s\" for logging (%s).",
			file, strerror(errno));
		nprint_curses(T_WAR, "Logging will be disabled.");
	}

	va_start(ap, format);
        __va_copy(ap2, ap);

	wattrset(windows.status->name, msg_attrs(mid));

	if (mid != T_RAW) {
		Xwprintw(windows.status->name, "\n%c: ", msg_character(mid));

		if (*opt_log_status_window && fp) {
			fprintf(fp, "\n%c: ", msg_character(mid));
		}
	}

	wstandend(windows.status->name);

	vwprintw(windows.status->name, format, ap);

	if (*opt_log_status_window && fp) {
		vfprintf(fp, format, ap2);
	}

	windows.status->ref(0);

	va_end(ap);
        va_end(ap2);

	if (*opt_log_status_window && fp) {
		fclose(fp);
	}

	xfree(file);
}

static void nprint_text(msg_id_e mid, const char *format,...)
{
	char *file = alloc_real_status_logfile_name();
	FILE *fp = NULL;
	FILE *console;
	va_list ap;
        va_list ap2;

	if (*opt_log_status_window && (fp = fopen(file, "a")) == NULL) {
		*opt_log_status_window = 0;
		nprint_text(T_WAR,
			"Unable to open \"%s\" for logging.", file);
		nprint_text(T_WAR, "Logging will be disabled.");
	}

	va_start(ap, format);
        va_copy(ap2, ap);

	console = (mid == T_ERR) ? stderr : stdout;

	if (mid != T_RAW) {
		fprintf(console, "\n%c: ", msg_character(mid));

		if (*opt_log_status_window && fp) {
			fprintf(fp, "\n%c: ", msg_character(mid));
		}
	}

	vfprintf(console, format, ap);
	fflush(console);

	if (*opt_log_status_window && fp) {
		vfprintf(fp, format, ap2);
	}

	va_end(ap);
        va_end(ap2);
        
	if (*opt_log_status_window && fp) {
		fclose(fp);
	}

	xfree(file);
}

static void nprint_init(msg_id_e mid, const char *format,...)
{
	FILE *console;
	va_list ap;

	va_start(ap, format);

	switch (mid) {
	case T_ERR:
		console = stderr;
		++init_errors;
		break;
	case T_WAR:
		console = stdout;
		++init_warnings;
		break;
	default:
		console = stdout;
		break;
	}

	vfprintf(console, format, ap);

	fputc('\n', console);

	va_end(ap);
}

/*
 * Main.
 */

int main(int argc, char **argv)
{
	int i;

	nprint = nprint_init;

	/* Load all handlers (before options, so handlers can provide options). */
	load_all_handlers();

	set_options_to_defaults();

	/*
	 * Init options.
	 */

	read_env_variables();

	read_system_rc_file();

	read_user_rc_file();

	read_command_line_options(&argc, &argv);

	post_validate_options();

	if (init_errors)
		return EXIT_FAILURE;

	nprint = nprint_text;

	init_needed_directories();

	init_state_file();

	if (! Empty_string(*opt_find_prunes_file)) {
		append_prune_dirs_from_file();
	}

	set_main_signals();

	if (*opt_run_interactive) { /* Start ncurses. */
		start_display();
		draw_static_border();
		nprint = nprint_curses;
	}

	/* Print some useful information. */
	Nprint("Using \"%s\" directory.", *opt_alfs_directory);
	Nprint("Total %d handlers loaded.", get_handler_count());

	init_parser();

	/* Add profiles from command line. */
	for (i = optind; i < argc; ++i) {
		add_profile_from_path(argv[i]);
	}

	if (*opt_log_status_window) {
		char *file = alloc_real_status_logfile_name();
		Nprint("Using \"%s\" for status logging.", file);
		xfree(file);
	}

	if (*opt_warn_if_set) {
		warn_if_variables_set();
	}

	if (!*opt_run_interactive) {
		i = run_non_interactively();

		printf("\n\n");

		return i;
	}

	if (root_element == NULL) {
		Nprint_err("There are no sane profiles. "
			"Scroll up to see why, or (q)uit.");
		status_window();

	} else {
		browse();
	}

	end_display();

	printf("Have a nice day.\n");

	return 0;
}
