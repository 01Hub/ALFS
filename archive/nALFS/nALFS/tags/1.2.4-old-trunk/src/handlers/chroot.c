/*
 *  chroot.c - Handler.
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
#include <sys/wait.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME chroot
#include <nALFS.h>

#include "handlers.h"
#include "parser.h"
#include "nprint.h"
#include "utility.h"
#include "nalfs-core.h"
#include "backend.h"


#if HANDLER_SYNTAX_2_0

enum {
	CHROOT_DIR,
};

static const struct handler_attribute chroot_attributes[] = {
	{ .name = "dir", .private = CHROOT_DIR },
	{ .name = NULL }
};

struct chroot_data {
	char *dir;
};

static int chroot_setup(element_s * const element)
{
	struct chroot_data *data;

	if ((data = xmalloc(sizeof(struct chroot_data))) == NULL)
		return 1;

	data->dir = NULL;
	element->handler_data = data;

	return 0;
};

static void chroot_free(const element_s * const element)
{
	struct chroot_data *data = (struct chroot_data *) element->handler_data;

	xfree(data->dir);
	xfree(data);
}

static int chroot_attribute(const element_s * const element,
			   const struct handler_attribute * const attr,
			   const char * const value)
{
	struct chroot_data *data = (struct chroot_data *) element->handler_data;

	switch (attr->private) {
	case CHROOT_DIR:
		if (data->dir) {
			Nprint_err("<chroot>: cannot specify \"dir\" more than once.");
			return 1;
		}
		data->dir = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int chroot_main(const element_s * const el)
{
	struct chroot_data *data = (struct chroot_data *) el->handler_data;
	int status;
	pid_t chroot_pid, got_pid;

	chroot_pid = fork();

	if (chroot_pid == 0) { /* Child. */
		Nprint_h("Changing root directory to %s.", data->dir);

		if (change_current_dir(data->dir)) {
			exit(EXIT_FAILURE);
		}

		if (chroot(data->dir)) {
			Nprint_h_err("    %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		exit(execute_children(el));

	} else if (chroot_pid == -1) { /* Error. */
		fatal_backend_error("fork() in chroot element failed.");
	}

	if ((got_pid = waitpid(chroot_pid, &status, WUNTRACED)) == -1) {
		fatal_backend_error("waitpid() for %ld in chroot failed.",
				    (long)chroot_pid);
	}

	if (WIFEXITED(status)) { /* Child exited normally. */
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {
		Nprint_h_err("Chroot child (%ld) killed by signal %d%s.",
			     (long)got_pid, WTERMSIG(status),
			     WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_h_err("Chroot child (%ld) stopped by signal %d.",
			     (long)got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_h_err("Chroot child (%ld) exited abnormaly.", (long)got_pid);
		status = -1;
	}
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "chroot",
		.description = "Enter chroot",
		.syntax_version = "2.0",
		.main = chroot_main,
		.type = HTYPE_NORMAL,
		.attributes = chroot_attributes,
		.setup = chroot_setup,
		.free = chroot_free,
		.attribute = chroot_attribute,
	},
#endif
	{
		.name = NULL
	}
};
