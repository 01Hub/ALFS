/*
 *  backend.c - Backend.
 *
 *  Copyright (C) 2001, 2002, 2004
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

#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <libgen.h>
#include <sys/poll.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bufsize.h"
#include "utility.h"
#include "parser.h"
#include "handlers.h"
#include "win.h"
#include "logging.h"
#include "nalfs-core.h"
#include "comm.h"
#include "options.h"

#include "backend.h"

static volatile int pause_now = 0;
static volatile int got_sigchld = 0;

struct child_output {
	int pipe[2];
	char *buf;
	size_t size;
	size_t used;
};

struct child_output child_stdout;
struct child_output child_stderr;

static void got_SIGCHLD(int sig)
{
	(void)sig;

	/* Nprint("got SIGCHLD"); */

	got_sigchld = 1;

	if (signal(SIGCHLD, got_SIGCHLD) == SIG_ERR) {
		Fatal_error("enabling SIGCHLD failed");
	}
}

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
				if (++*opt_logging_method > LAST_LOGGING_METHOD) {
					*opt_logging_method = 0;
				}
				break;

			case CTRL_LOG_HANDLER_ACTIONS:
				Toggle(*opt_log_handlers);
				break;

			case CTRL_SYSTEM_OUTPUT:
				Toggle(*opt_show_system_output);
				break;

			case CTRL_LOG_BACKEND:
				Toggle(*opt_log_backend);
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


	if (mid == T_SYS && !*opt_show_system_output) {
		return;
	}

	while (pause_now && mid == T_SYS) {
		napms(1);
	}

	va_start(ap, format);
	vsnprintf(raw_msg, sizeof raw_msg, format, ap);
	va_end(ap);

	sprintf(message, "\n%c: %s", msg_character(mid), raw_msg);

	if (write(comm_get_socket(BACKEND_DATA_SOCK), message, strlen(message)) == -1) {
		fatal_backend_error("write() to parent failed: %s",
			strerror(errno));
	}
}

static void send_child_output(const struct child_output * const child,
			      size_t length)
{
	char *chunk;
	char tmp;
	size_t chunk_size;

	if (length <= (MAX_DATA_MSG_LEN - 4)) {
		child->buf[length] = '\0';
		Nprint_sys("%s", &child->buf[0]);
		return;
	}

	chunk = &child->buf[0];

	while (length > 0) {
		chunk_size = (length > MAX_DATA_MSG_LEN - 4) ? MAX_DATA_MSG_LEN - 4 : length;
		length -= chunk_size;
		tmp = chunk[chunk_size];
		chunk[chunk_size] = '\0';
		Nprint_sys("%s", chunk);
		chunk += chunk_size;
		*chunk = tmp;
	}
}

static void handle_child_output(struct child_output * const child)
{
	ssize_t input;
	char *line_end;

	input = read(child->pipe[0], &child->buf[child->used],
		     child->size - child->used);

	if (input < 0)
		fatal_backend_error("read() from child failed: %s",
				    strerror(errno));

	if (input == 0)
		return;

	child->used += input;
	child->buf[child->used] = '\0';
	while ((line_end = strchr(&child->buf[0], '\n')) != NULL) {
		size_t line_length = (line_end - &child->buf[0]);

		send_child_output(child, line_length);
		memmove(&child->buf[0], ++line_end,
			child->used - (line_length + 1));
		child->used -= (line_length + 1);
	}
}

static void wait_for_child_completion(void)
{
	struct pollfd pfd[3];
	int status;
	int poll_timeout = -1;

	pfd[0].fd = child_stdout.pipe[0];
	pfd[0].events = POLLIN | POLLPRI;

	pfd[1].fd = child_stderr.pipe[0];
	pfd[1].events = POLLIN | POLLPRI;

	pfd[2].fd = comm_get_socket(BACKEND_CTRL_SOCK);
	pfd[2].events = POLLIN | POLLPRI;

	while (1) {
		/* If SIGCHLD has arrived, set timeout to zero so any remaining input
		   can be read and flushed
		*/
		if (got_sigchld && poll_timeout)
			poll_timeout = 0;
		pfd[0].revents = 0;
		pfd[1].revents = 0;
		pfd[2].revents = 0;
		status = poll(pfd, 3, poll_timeout);
		if (status == -1) {
			if (errno == EINTR)
				continue;
			Nprint_err("poll error: %s", strerror(errno));
			continue;
		}
		if (status == 0) {
			/* will only get here if timeout is zero, and no input is ready */
			break;
		}
		if (pfd[0].revents & (POLLIN | POLLPRI))
			handle_child_output(&child_stdout);

		if (pfd[1].revents & (POLLIN | POLLPRI))
			handle_child_output(&child_stderr);

		if (pfd[2].revents & (POLLIN | POLLPRI)) {
			handle_ctrl_msg();
		}
	}

	if (child_stdout.used > 0)
		send_child_output(&child_stdout, child_stdout.used);

	if (child_stderr.used > 0)
		send_child_output(&child_stderr, child_stderr.used);

}

int execute_direct_command(const char *command, char *const argv[])
{
	int status;
	pid_t got_pid, command_pid = 0;


	got_sigchld = 0;

	command_pid = fork();

	if (command_pid < 0) {
		fatal_backend_error("fork() failed: %s", strerror(errno));

	} else if (command_pid == 0) { /* Child. */
		if (dup2(child_stdout.pipe[1], STDOUT_FILENO) == -1)
			fatal_backend_error("dup2() failed: %s", strerror(errno));

		if (dup2(child_stdout.pipe[1], STDERR_FILENO) == -1)
			fatal_backend_error("dup2() failed: %s", strerror(errno));

		execvp(command, argv);

		Nprint_err("Executing command using %s failed: %s", command,
			strerror(errno));

		/* use a non-standard exit() code to ensure that the parent
		   knows the executing command failed completely */
		exit(-1);
	}

	/* Parent. */
	wait_for_child_completion();

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

int execute_command(const element_s * const element, const char *format, ...)
{
	va_list ap;
	int status = 0;
	char command[MAX_COMMAND_LEN];
	char *args[4];
	const char *shell;
	char *temp;


	va_start(ap, format);
	status = vsnprintf(command, sizeof command, format, ap);
	va_end(ap);

	if (status > -1 && status < (int) sizeof command) {
		if (element && element->handler->alternate_shell) {
			shell = alloc_stage_shell(element);
		} else {
			shell = xstrdup("sh");
		}
		/* make a copy of shell because basename may modify it */
		temp = xstrdup(shell);
		args[0] = xstrdup(basename(temp));
		args[1] = "-c";
		args[2] = command;
		args[3] = NULL;
		status = execute_direct_command(shell, args);
		xfree(args[0]);
		xfree(temp);
		xfree(shell);
	} else {
		Nprint_err("System command is too long.");
		return -1;
	}

	return status;
}

static INLINE void change_to_profiles_dir(element_s *el)
{
	element_s *profile = get_profile_by_element(el);
	char *path = xstrdup(profile->handler->name);
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

	if (!el->should_run && ((el->handler->type & HTYPE_NORMAL) != 0)) {
		return 0;
	}

	comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_STARTED, "%d", el->id);

	if (el->type == TYPE_PROFILE) {
		i = execute_children(el);

	} else {
		/* Find element's handler and then execute it's function. */
		if (el->handler) {
			if (*opt_use_relative_dirs) {
				change_to_profiles_dir(el);
			}

			i = el->handler->main(el);

		} else { /* Never reached.
			  * (Only elements with ->handler have TYPE_ELEMENT
			  * type and only those elements can run.)
			  */
			ASSERT(0);
			Nprint_err("No handler for %s", el->handler->name);
			return -1;
		}
	}

	if (i == 0) {
		// TODO: Is there a point of setting these at all?
		el->run_status = get_element_status(el);
		comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_ENDED, "%d", el->id);
	} else {
		el->run_status = RUN_STATUS_FAILED;
		comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_FAILED, "%d", el->id);
	}

	return i;
}

int do_execute_test_element(element_s *element, int *result)
{
	int i = 0;

	comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_STARTED, "%d", element->id);

	if (*opt_use_relative_dirs)
		change_to_profiles_dir(element);

	i = element->handler->test(element, result);

	if (i == 0) {
		// TODO: Is there a point of setting these at all?
		element->run_status = get_element_status(element);
		comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_ENDED, "%d", element->id);
	} else {
		element->run_status = RUN_STATUS_FAILED;
		comm_send_ctrl_msg(BACKEND_CTRL_SOCK, CTRL_ELEMENT_FAILED, "%d", element->id);
	}

	return i;
}

int execute_children(element_s *element)
{
	return execute_children_filtered(element, HTYPE_NORMAL);
}

int execute_children_filtered(element_s *element, handler_type_e type_filter)
{
	element_s *child;


	for (child = element->children; child; child = child->next) {
		int i;

		if (child->handler &&
		    ((child->handler->type & type_filter) == 0))
			continue;

		if ((i = do_execute_element(child)))
			return i;
	}

	return 0;
}

void start_backend(element_s *first)
{
	int result;

	nprint = nprint_backend;

	if (signal(SIGCHLD, got_SIGCHLD) == SIG_ERR) {
		Fatal_error("enabling SIGCHLD failed");
	}

	if (pipe(&child_stdout.pipe[0])) {
		fatal_backend_error("pipe() for stdout failed: %s", strerror(errno));
	}

	child_stdout.buf = xmalloc(16384);
	/* leave for adding trailing null character */
	child_stdout.size = 16383;
	child_stdout.used = 0;

	if (pipe(&child_stderr.pipe[0])) {
		fatal_backend_error("pipe() for stderr failed: %s", strerror(errno));
	}

	child_stderr.buf = xmalloc(16384);
	/* leave for adding trailing null character */
	child_stderr.size = 16383;
	child_stderr.used = 0;

	result = execute_children(first);

	(void) close(child_stdout.pipe[1]);
	(void) close(child_stdout.pipe[0]);
	(void) close(child_stderr.pipe[1]);
	(void) close(child_stderr.pipe[0]);

	xfree(child_stdout.buf);
	xfree(child_stderr.buf);

	exit(result);
}
