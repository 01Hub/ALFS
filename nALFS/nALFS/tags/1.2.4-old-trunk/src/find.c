/*
 *  find.c - find(1)-like search for collecting files.
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
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utility.h"
#include "backend.h"
#include "nalfs-core.h"
#include "win.h"
#include "logging.h"
#include "options.h"

#include "find.h"


static char **prunes;
static FILE *fp;
static time_t time_stamp = -1;


/* Returns:
 *  -1  Search interrupted (option turened off)
 *   1  File action failed.
 *   0  File action succeded.
 */
static int recursive_action(const char *filename)
{
	int i;
	int status = 0;
	DIR *dir;
	struct stat statbuf;
	struct dirent *next;


	if (opt_logging_method == LOG_OFF) {
		return -1;
	}

	if (lstat(filename, &statbuf) == -1) {
		Nprint_warn("%s: %s", filename, strerror(errno));
		return 1;
	}

	if (prunes) { /* Check if a file is in the prune list. */
		for (i = 0; prunes[i]; ++i) {
			if (strcmp(filename, prunes[i]) == 0) {
				Debug_logging("In prune list: %s", filename);
				return 0;
			}
		}
	}

	/* Not in the prune list, print it. */
	if (time_stamp == -1 || statbuf.st_ctime > time_stamp) {
		fprintf(fp, "%s\n",  filename);
	}

	if (! S_ISDIR(statbuf.st_mode)) {
		/* It's not a directory, we're done. */
		return 0;
	}

	/* It's a directory, read it. */

	if (! (dir = opendir(filename))) {
		Nprint_warn("%s: %s", filename, strerror(errno));
		return 1;
	}

	while ((next = readdir(dir)) != NULL) {
		char *next_f;

		if ((strcmp(next->d_name, "..") == 0)
		|| (strcmp(next->d_name, ".") == 0)) {
			continue;
		}

		next_f = xmalloc(strlen(filename) + strlen(next->d_name) + 2);

		sprintf(next_f, "%s%s%s",
			filename,
			filename[strlen(filename)-1] == '/' ? "" : "/",
			next->d_name);

		status = recursive_action(next_f);

		xfree(next_f);
	}

	closedir(dir);

	return status;
}

static void init_prune_list(const char *root, const char *string_)
{
	int i = 0;
	char *file;
	char *string = xstrdup(string_);
	char resolved_path[PATH_MAX];


	for (file = strtok(string, WHITE_SPACE); file;
	file = strtok(NULL, WHITE_SPACE)) {
		char tmp[strlen(root) + strlen(file) + 2];

		sprintf(tmp, "%s/%s", root, file);

		/* TODO: File is added in the prune list,
		 *       only if it already exists.
		 */
		if (realpath(tmp, resolved_path) == NULL) {
			continue;
		}

		++i;

		prunes = xrealloc(prunes, (i + 1) * sizeof *prunes);

		prunes[i-1] = xstrdup(resolved_path);
		prunes[i] = NULL;
	}

	xfree(string);
}

int do_collect_files(
	FILE *fp_,
	const char *find_root_,
	const char *find_prunes_,
	time_t time_stamp_)
{
	int i, status;
	char root[PATH_MAX];


	fp = fp_;
	time_stamp = time_stamp_;

	if (find_root_) {
		if (realpath(find_root_, root) == NULL) {
			Nprint_warn("Can't use %s as a search path: %s",
				find_root_, strerror(errno));

			Nprint_warn("Using /.");
			strcpy(root, "/");
		}
	} else {
		strcpy(root, "/");
	}

	if (find_prunes_ && strlen(find_prunes_)) {
		init_prune_list(root, find_prunes_);

		if (prunes) { /* Print prune list. */
			Nprint("Ignoring these files:");

			for (i = 0; prunes[i]; ++i) {
				Nprint("    %s", prunes[i]);
			}
		}

	} else {
		Debug_logging("Prune list is empty.");
	}

	/* TODO: code used to stop backend control message processing here */
	status = recursive_action(root);

	if (prunes) { /* Free prune list. */
		for (i = 0; prunes[i]; ++i) {
			xfree(prunes[i]);
		}
		xfree(prunes);
		prunes = NULL;
	}

	return status;
}
