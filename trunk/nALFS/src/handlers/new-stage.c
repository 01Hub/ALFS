/*
 *  new-stage.c - Handler.
 * 
 *  Copyright (C) 2002-2003
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
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "nalfs.h"
#include "config.h"
#include "backend.h"


static INLINE int set_supplementary_groups(const char *user, gid_t gid)
{
	size_t size;
	struct group *gr;
	FILE *fp;
	gid_t *list;


	/* Add user's group ID first. */
	list = xmalloc(sizeof *list);
	list[0] = gid;
	size = 1;

	/* Now check other groups if they contain user as a member. */
	if ((fp = fopen("/etc/group", "r"))) {
		/* getgrnam() is failing in chroot() */
		while ((gr = fgetgrent(fp))) {
			int i;

			for (i = 0; gr->gr_mem[i]; i++) {
				if (strcmp(gr->gr_mem[i], user) == 0) {
					size++;
					list = xrealloc(list, size * sizeof *list);
					list[size-1] = gr->gr_gid;
	
					break;
				}
			}
		}

		fclose(fp);

	} else {
		Nprint_h_warn("Unable to open /etc/group: %s",
			strerror(errno));
	}

	if (setgroups(size, list)) {
		Nprint_h_err("Unable to set supplementary group IDs: %s",
			strerror(errno));
		return -1;
	}

	xfree(list);

	return 0;
}

static INLINE int change_to_user(const char *user)
{
	struct passwd *pw;
	FILE *fp;


	if ((fp = fopen("/etc/passwd", "r")) == NULL) {
		Nprint_h_err("Unable to open /etc/passwd: %s",
			strerror(errno));
		return -1;
	}

	/* getpwnam() is failing in chroot() */
	while ((pw = fgetpwent(fp))) {
		if (strcmp(pw->pw_name, user) == 0) {
			break;
		}
	}

	fclose(fp);

	if (pw == NULL) {
		Nprint_h_err("User %s doesn't exist.", user);
		return -1;
	}

	if (set_supplementary_groups(user, pw->pw_gid)) {
		return -1;
	}

	if (setgid(pw->pw_gid)) {
		Nprint_h_err("Unable to set group ID: %s",
			strerror(errno));
		return -1;
	}

	if (setuid(pw->pw_uid)) {
		Nprint_h_err("Unable to set user ID: %s",
			strerror(errno));
		return -1;
	}

	return 0;
}



static INLINE int unset_variable(const char *variable)
{
	Nprint_h("Unsetting environment variable %s.", variable);
	unsetenv(variable);

	return 0;
}

static INLINE int append_to_variable(const char *variable, const char *value)
{
	int status = 0;
	char *old_value, *full_value = NULL;
       

	if ((old_value = getenv(variable))) {
		full_value = xstrdup(old_value);
	}
	append_str(&full_value, value);

	Nprint_h("Appending to environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, full_value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		status = -1;
	}

	xfree(full_value);

	return status;
}

static INLINE int prepend_to_variable(const char *variable, const char *value)
{
	int status = 0;
	char *old_value, *full_value = NULL;
       

	append_str(&full_value, value);

	if ((old_value = getenv(variable))) {
	        append_str(&full_value, old_value);
	}

	Nprint_h("Prepending to environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, full_value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		status = -1;
	}

	xfree(full_value);

	return status;
}

static INLINE int set_variable(const char *variable, const char *value)
{
	Nprint_h("Setting environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		return -1;
	}

	return 0;
}

static INLINE int set_environment(element_s *environment)
{
	element_s *v;


	for (v = first_param("variable", environment); v; v = next_param(v)) {
		char *name, *value;
				
		name = attr_value("name", v);
		value = alloc_trimmed_str(v->content);

		if (value) {
			char *mode = attr_value("mode", v);

			if (mode && strcmp(mode, "append") == 0) {
				append_to_variable(name, value);
			} else if (mode && strcmp(mode, "prepend") == 0) {
				prepend_to_variable(name, value);
			} else {
				set_variable(name, value);
			}

			xfree(value);

		} else {
			unset_variable(name);
		}
	}
	
	return 0;
}



static int parse_stageinfo_and_execute_children(
	element_s *el,
	element_s *stageinfo)
{
	int status = 0;
	pid_t pid, got_pid;
	char *user = NULL, *root = NULL;
	element_s *environment_el, *user_el, *root_el;
	
	/* <base> is read by elements under <stage> themselves. */
	environment_el = first_param("environment", stageinfo);
	user_el = first_param("user", stageinfo);
	root_el = first_param("root", stageinfo);

	if (user_el) {
		if ((user = alloc_trimmed_str(user_el->content)) == NULL) {
	                Nprint_h_err("User not specified.");
	                return -1;
		}
	}

	if (root_el) {
		if ((root = alloc_trimmed_str(root_el->content)) == NULL) {
			Nprint_h_err("Root directory not specified.");
			xfree(user);
			return -1;
		}
	}

	/* Nothing interesting found, no need for forking. */
	if (!environment_el && !user && !root) {
		xfree(user);
		xfree(root);

		return execute_children(el);
	}

	pid = fork();

	if (pid == 0) { /* Child. */
		Start_receiving_sigio();

		if (environment_el) {
			set_environment(environment_el);
		}

		if (user) {
			Nprint_h("Changing to user \"%s\".", user);

			if (change_to_user(user) == -1) {
				xfree(user);
				xfree(root);
				exit(EXIT_FAILURE);
			}

			xfree(user);
		}

		if (root) {
			Nprint_h("Changing root directory to \"%s\".", root);

			if (change_current_dir(root)) {
				xfree(root);
				exit(EXIT_FAILURE);
			}
			if (chroot(root)) {
				Nprint_h_err("    %s", strerror(errno));
				xfree(root);
				exit(EXIT_FAILURE);
			}

			xfree(root);
		}

		exit(execute_children(el));

	} else if (pid == -1) { /* Error. */
		fatal_backend_error("fork() in su/chroot element failed.");
	}

	/* Parent. */

	if ((got_pid = waitpid(pid, &status, WUNTRACED)) == -1) {
		fatal_backend_error("waitpid() for %ld in /chroot failed.",
			(long)pid);
	}

	if (WIFEXITED(status)) { /* Child exited normally. */
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {
		Nprint_h_err("Su/chroot child (%ld) killed by signal %d%s.",
			(long)got_pid, WTERMSIG(status),
			WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_h_err("Su/chroot child (%ld) stopped by signal %d.",
			(long)got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_h_err("Su/chroot child (%ld) exited abnormaly.",
			(long)got_pid);
		status = -1;
	}

	xfree(user);
	xfree(root);

	return status;
}



char handler_name[] = "stage";
char handler_description[] = "Enter stage: ";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = {
// "name", "description", "logfile", "mode", NULL };
char *handler_parameters[] =
{ "stageinfo", "base", "root", "user", "environment", "variable", NULL };
int handler_action = 0;


int handler_main(element_s *el)
{
	int status;
	char *stage_name = attr_value("name", el);
	element_s *stageinfo;
       

	if (opt_be_verbose) {
		if (stage_name) {
			Nprint_h("Entering new stage: %s", stage_name);
		} else {
			Nprint_h("Entering new stage.");
		}
	}

	log_start_time(el);

	if ((stageinfo = first_param("stageinfo", el))) {
		status = parse_stageinfo_and_execute_children(el, stageinfo);
	} else {
		status = execute_children(el);
	}

	log_end_time(el, status);

	if (opt_be_verbose) {
		if (stage_name) {
			Nprint_h("Exiting stage: %s", stage_name);
		} else {
			Nprint_h("Exiting stage.");
		}
	}

	return status;
}
