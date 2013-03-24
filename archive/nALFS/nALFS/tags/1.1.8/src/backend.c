/*
 *  backend.c - Backend.
 *
 *  Copyright (C) 2001, 2002
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
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utility.h"
#include "parser.h"
#include "handlers.h"
#include "win.h"
#include "logging.h"
#include "nalfs.h"
#include "comm.h"
#include "config.h"

#include "backend.h"


static volatile int pause_now = 0;


void fatal_backend_error(const char *format, ...)
{
	va_list ap;
	char msg[MAX_DATA_MSG_LEN-4];
	char message[MAX_DATA_MSG_LEN];

	va_start(ap, format);
	vsnprintf(msg, sizeof msg, format, ap);
	va_end(ap);

	sprintf(message, "\nE: %s", msg);

	write(comm_get_socket(BACKEND_DATA_SOCK), message, strlen(message));
	/* Already in trouble, no need for error checking. */

	raise(SIGKILL);

	while(1) {
		sleep(1);
	}
}

static INLINE void toggle_pause(void)
{
	Toggle(pause_now);

	if (pause_now) {
		Nprint("Pausing...");
	} else {
		Nprint("Continuing...");
	}
}

static INLINE void stop_ourselves(void)
{
	Stop_receiving_sigio();

	log_stopped_execution();

	exit(EXIT_FAILURE); /* TODO: Return something like "-STOPPED". */
}

/*
 * Reads all available messages from the control socket and acts upon them.
 */
static void handle_ctrl_msg(void)
{
	ctrl_msg_s *message;


	while ((message = comm_read_ctrl_message(BACKEND_CTRL_SOCK))) {
		switch (comm_msg_type(message)) {
			case CTRL_LOG_CHANGED_FILES:
				if (++opt_logging_method > LAST_LOGGING_METHOD) {
					opt_logging_method = 0;
				}
				break;

			case CTRL_LOG_HANDLER_ACTIONS:
				Toggle(opt_log_handlers);
				break;

			case CTRL_SYSTEM_OUTPUT:
				Toggle(opt_show_system_output);
				break;

			case CTRL_LOG_BACKEND:
				Toggle(opt_log_backend);
				break;

			case CTRL_PAUSE:
				toggle_pause();
				break;

			case CTRL_STOP:
				stop_ourselves();
				break;

			default:
				Nprint_warn(
					"Unknown control message: \"%s\"",
					comm_msg_content(message));
		}

		comm_free_message(message);
	}
}

static void nprint_backend(msg_id_e mid, const char *format, ...)
{
	va_list ap;
	char message[MAX_DATA_MSG_LEN];
	char raw_msg[MAX_DATA_MSG_LEN - 5];


	while (pause_now && mid == T_SYS) {
		napms(1);
	}

	va_start(ap, format);
	vsnprintf(raw_msg, sizeof raw_msg, format, ap);
	va_end(ap);

	if (mid == T_SYS && !opt_show_system_output) {
		return;
	}

	sprintf(message, "\n%c: %s", msg_character(mid), raw_msg);

	if (write(comm_get_socket(BACKEND_DATA_SOCK), message, strlen(message)) == -1) {
		fatal_backend_error("write() to parent failed: %s",
			strerror(errno));
	}
}

static void signal_io(int sig)
{
	(void)sig;

	/* Nprint("GOT SIGIO!"); */

	handle_ctrl_msg();
}

static INLINE void read_descriptor(int pfd)
{
	char buf[MAX_DATA_MSG_LEN - 4];
	FILE *fp;


	if ((fp = fdopen(pfd, "r")) == NULL) {
		fatal_backend_error("fdopen() failed: %s", strerror(errno));
	}

	/* Start reading. */
	while (fgets(buf, (int) sizeof buf, fp) != NULL) {
		remove_new_line(buf);

		Nprint_sys("%s", buf);
	}

	if (fclose(fp) == EOF) {
		fatal_backend_error("fclose() failed: %s", strerror(errno));
	}
}

static INLINE int do_execute_command(const char *cmdstring)
{
	int status, pfd[2];
	pid_t got_pid, command_pid = 0;


	if (pipe(pfd)) {
		fatal_backend_error("pipe() failed: %s", strerror(errno));
	}

	command_pid = fork();

	if (command_pid < 0) {
		fatal_backend_error("fork() failed: %s", strerror(errno));

	} else if (command_pid == 0) { /* Child. */
		if (close(pfd[0])) { /* Child won't read. */
			fatal_backend_error("close() failed: %s",
				strerror(errno));
		}

		/* Make pfd[1] command's stdout. */
		if (pfd[1] != STDOUT_FILENO) {
			if (dup2(pfd[1], STDOUT_FILENO) == -1) {
				fatal_backend_error("dup2() failed: %s",
					strerror(errno));
			}
		}

		/* Make pfd[1] command's stderr. */
		if (pfd[1] != STDERR_FILENO) {
			if (dup2(pfd[1], STDERR_FILENO) == -1) {
				fatal_backend_error("dup2() failed: %s",
					strerror(errno));
			}
		}

		if (close(pfd[1])) {
			fatal_backend_error("close() failed: %s",
				strerror(errno));
		}

		execlp("sh", "sh", "-c", cmdstring, NULL);

		Nprint_err("Executing command using \"sh\" failed: %s",
			strerror(errno));

		exit(EXIT_FAILURE);
	}

	/* Parent. */

	/* Parent won't write. */
	if (close(pfd[1])) {
		fatal_backend_error("close() failed: %s", strerror(errno));
	}

	/* Read command's stdout and stderr and send it to the frontend. */
	read_descriptor(pfd[0]);

	if ((got_pid = waitpid(command_pid, &status, 0)) == -1) {
		fatal_backend_error("waitpid() for %ld failed: %s",
			(long)command_pid, strerror(errno));
	}

	if (WIFEXITED(status)) { /* Child exited normally. */
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {
		Nprint_err("Child (%ld) killed by signal "
			"%d%s.", (long)got_pid, WTERMSIG(status),
			WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_err("Child (%ld) stopped by signal "
			"%d.", (long)got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_err("Command child (%ld) exited "
			"abnormaly.", (long)got_pid);
		status = -1;
	}

	return status;
}

int execute_command(const char *format, ...)
{
	va_list ap;
	int status = 0;
	char command[MAX_COMMAND_LEN];


	va_start(ap, format);
	status = vsnprintf(command, sizeof command, format, ap);
	va_end(ap);

	if (status > -1 && status < (int) sizeof command) {
		status = do_execute_command(command);
	} else {
		Nprint_err("System command is too long.");
		return -1;
	}

	return status;
}

static INLINE void change_to_profiles_dir(element_s *el)
{
	element_s *profile = get_profile_by_element(el);
	char *path = xstrdup(profile->name);
	char *tmp;


	if ((tmp = strrchr(path, '/'))) {
		*tmp = '\0';

		if (chdir(path) == -1) {
			Nprint_warn("Unable to chdir to %s: %s",
				path, strerror(errno));
		}
	}

	xfree(path);
}

static int do_execute_element(element_s *el)
{
	int i = 0;
	element_s *profile = get_profile_by_element(el);


	if (! (Can_run(el) && el->should_run)) {
		return 0;
	}

	comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_STARTED,
		"%s %s %d", profile->name, el->name, el->id);

	if (el->type == TYPE_PROFILE) {
		i = execute_children(el);

	} else {
		/* Find element's handler and then execute it's function. */
		if (el->handler) {
			if (opt_use_relative_dirs) {
				change_to_profiles_dir(el);
			}

			i = el->handler->main_function(el);

		} else { /* Never reached.
			  * (Only elements with ->handler have TYPE_ELEMENT
			  * type and only those elements can run.)
			  */
			ASSERT(0);
			Nprint_err("No handler for %s", el->name);
			return -1;
		}
	}

	if (i == 0) {
		el->run_status = get_element_status(el);
		comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_ENDED,
			"%s %s %d", profile->name, el->name, el->id);
	} else {
		el->run_status = RUN_STATUS_FAILED;
		comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_FAILED,
			"%s %s %d", profile->name, el->name, el->id);
	}

	return i;
}

int execute_children(element_s *el)
{
	element_s *child;


	for (child = el->children; child; child = child->next) {
		int i;

		if ((i = do_execute_element(child))) {
			return i;
		}
	}

	return 0;
}

void set_receive_sigio(int s, int receive_it)
{
	if (receive_it) {
		/* Start handling SIGIO. */
		if (signal(SIGIO, signal_io) == SIG_ERR) {
			fatal_backend_error("signal() failed");
		}

		/* Make child receive SIGIO when control message comes. */
		if (fcntl(s, F_SETFL, O_ASYNC) == -1) {
			Fatal_error("fcntl() failed: %s", strerror(errno));
		}
		if (fcntl(s, F_SETOWN, getpid()) == -1) {
			Fatal_error("fcntl() failed: %s", strerror(errno));
		}

		/* Check if we missed something. */
		handle_ctrl_msg();

	} else {
		/* Stop handling SIGIO. */
		if (signal(SIGIO, SIG_IGN) == SIG_ERR) {
			fatal_backend_error("signal() failed");
		}
	}
}

void start_backend(element_s *first)
{
	nprint = nprint_backend;

	Start_receiving_sigio();

	exit(execute_children(first));
}
