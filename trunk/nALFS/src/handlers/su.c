/*
 *  su.c - Handler.
 * 
 *  Copyright (C) 2002
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
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <dlfcn.h>

#include "parser.h"
#include "win.h"
#include "nalfs.h"
#include "backend.h"
#include "utility.h"
#include "config.h"


static INLINE int set_supplementary_groups(const char *user)
{
	int i;
	size_t size = 0;
	gid_t *list = NULL;
	struct group *gr;
	FILE *fp;


	if ((fp = fopen("/etc/group", "r")) == NULL) {
		Nprint_h_err("Unable to open /etc/group: %s",
			strerror(errno));
		return -1;
	}

	/* getgrnam() is failing in chroot() */
	while ((gr = fgetgrent(fp))) {
		for (i = 0; gr->gr_mem[i]; i++) {
			if (strcmp(gr->gr_mem[i], user) == 0) {
				size++;
				list = xrealloc(list, size);
				list[size-1] = gr->gr_gid;

				break;
			}
		}
	}

	fclose(fp);

	if (size == 0) {
		return 0;
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

	Nprint_h("Changing to user \"%s\".", pw->pw_name);

	if (set_supplementary_groups(user)) {
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


char handler_name[] = "su";
char handler_description[] = "Change user ID";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { "user", NULL };
char *handler_parameters[] = { NULL };
int handler_action = 0;


int handler_main(element_s *el)
{
	int status;
	pid_t su_pid, got_pid;
        char *user = attr_value("user", el);

	
        if (user == NULL) {
                Nprint_h_err("User not specified for su.");
                return -1;
	}

	su_pid = fork();

	if (su_pid == 0) { /* Child. */
		Start_receiving_sigio();

		if (change_to_user(user) == -1) {
			exit(EXIT_FAILURE);
		}

		status = execute_children(el);

		/* Nprint_h("Exited from su (%s).", pw->pw_name); */
		
		exit(status);

	} else if (su_pid == -1) { /* Error. */
		fatal_backend_error("fork() in su element failed.");
	}

	if ((got_pid = waitpid(su_pid, &status, WUNTRACED)) == -1) {
		fatal_backend_error("waitpid() for %ld in su failed.",
			(long)su_pid);
	}

	if (WIFEXITED(status)) { /* Child exited normally. */
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {
		Nprint_h_err("Su child (%ld) killed by signal %d%s.",
			(long)got_pid, WTERMSIG(status),
			WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_h_err("Su child (%ld) stopped by signal %d.",
			(long)got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_h_err("Su child (%ld) exited abnormaly.",
			(long)got_pid);
		status = -1;
	}
	
	return status;
}
