/*
 *  chroot.c - Handler.
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
#include <sys/types.h>
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

static const char *chroot_parameters[] = { NULL };
// char *HANDLER_SYMBOL(attributes)[] = { "dir", NULL };

static int chroot_main(element_s *el)
{
	int status;
	pid_t chroot_pid, got_pid;
	char *dir = attr_value("dir", el);

	
	if (dir == NULL) {
		Nprint_h_err("No directory specified for chroot.");
		return -1;
	}

	chroot_pid = fork();

	if (chroot_pid == 0) { /* Child. */
		Start_receiving_sigio();

		Nprint_h("Changing root directory to %s.", dir);

		if (change_current_dir(dir)) {
			exit(EXIT_FAILURE);
		}

		if (chroot(dir)) {
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
			(long)got_pid,
			WTERMSIG(status),
			WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_h_err("Chroot child (%ld) stopped by signal %d.",
			(long)got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_h_err("Chroot child (%ld) exited abnormaly.",
			(long)got_pid);
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
		.parameters = chroot_parameters,
		.main = chroot_main,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 0,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
