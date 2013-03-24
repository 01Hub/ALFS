/*
 *  su.c - Handler.
 * 
 *  Copyright (C) 2002, 2004
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
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <dlfcn.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME su
#include <nALFS.h>

#include "handlers.h"
#include "parser.h"
#include "nprint.h"
#include "nalfs-core.h"
#include "backend.h"
#include "utility.h"


enum {
	SU_USER,
};

static const struct handler_attribute su_attributes[] = {
	{ .name = "user", .private = SU_USER },
	{ .name = NULL }
};

struct su_data {
	char *user;
};

static int su_setup(element_s * const element)
{
	struct su_data *data;

	if ((data = xmalloc(sizeof(struct su_data))) == NULL)
		return -1;

	data->user = NULL;
	element->handler_data = data;

	return 0;
}

static void su_free(const element_s * const element)
{
	struct su_data *data = (struct su_data *) element->handler_data;

	xfree(data->user);
	xfree(data);
}

static int su_attribute(const element_s * const element,
			const struct handler_attribute * const attr,
			const char * const value)
{
	struct su_data *data = (struct su_data *) element->handler_data;

	switch (attr->private) {
	case SU_USER:
		if (data->user) {
			Nprint_err("<%s>: cannot specify \"user\" more than once.", element->handler->name);
			return 1;
		}
		data->user = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int su_valid_data(const element_s * const element)
{
	struct su_data *data = (struct su_data *) element->handler_data;

	if (!data->user) {
		Nprint_err("<%s>: \"user\" must be specified.", element->handler->name);
		return 0;
	}

	return 1;
}

static int su_valid_child(const element_s * const element, const element_s * const child)
{
	(void) element;

	return child->handler->type & (HTYPE_NORMAL |
				       HTYPE_COMMENT |
				       HTYPE_PACKAGE);
}

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


	setpwent();

	/* getpwnam() is failing in chroot() */
	while ((pw = getpwent())) {
		if (strcmp(pw->pw_name, user) == 0) {
			break;
		}
	}

	endpwent();

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

static int su_main(const element_s * const element)
{
	struct su_data *data = (struct su_data *) element->handler_data;
	int status;
	pid_t su_pid, got_pid;
	
	su_pid = fork();

	if (su_pid == 0) { /* Child. */
		if (change_to_user(data->user) == -1) {
			exit(EXIT_FAILURE);
		}

		status = execute_children(element);

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

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "su",
		.description = "Change user ID",
		.syntax_version = "2.0",
		.type = HTYPE_NORMAL,
		.main = su_main,
		.attributes = su_attributes,
		.setup = su_setup,
		.free = su_free,
		.attribute = su_attribute,
		.valid_data = su_valid_data,
		.valid_child = su_valid_child,
	},
#endif
	{
		.name = NULL
	}
};
