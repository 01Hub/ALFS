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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utility.h"
#include "backend.h"
#include "nalfs.h"
#include "win.h"
#include "logging.h"
#include "config.h"

#include "find.h"


static char **prunes;
static FILE *fp;
static time_t time_stamp = -1;


static INLINE struct passwd *xgetpwuid(uid_t uid)
{
	struct passwd *pw;
	FILE *f;


	if ((f = fopen("/etc/passwd", "r")) == NULL) {
		Nprint_err("Unable to open /etc/passwd: %s",
			strerror(errno));
		return NULL;
	}

	while ((pw = fgetpwent(f))) {
		if (pw->pw_uid == uid) {
			break;
		}
	}

	fclose(f);

	return pw;
}

static INLINE struct group *xgetgrgid(gid_t gid)
{
	struct group *gr;
	FILE *f;


	if ((f = fopen("/etc/group", "r")) == NULL) {
		Nprint_err("Unable to open /etc/group: %s",
			strerror(errno));
		return NULL;
	}

	while ((gr = fgetgrent(f))) {
		if (gr->gr_gid == gid) {
			break;
		}
	}

	fclose(f);

	return gr;
}

static INLINE void print_file(const char *filename, struct stat *statbuf)
{
	int i;
	char time_string[13];
	char linkname[PATH_MAX];
	struct tm *tm;
	struct passwd *p;
	struct group *g;


	tm = localtime(&statbuf->st_ctime);

	if ((i = strftime(time_string, 13, "%b %e %H:%M", tm)) != 12) {
		fatal_backend_error("strftime() failed");
	}

	/* Start printing */

	fprintf(fp, "%7o", statbuf->st_mode);

	/* getpwuid() is failing in chroot() */
	if ((p = xgetpwuid(statbuf->st_uid)) != NULL) {
		fprintf(fp, "%8s", p->pw_name);
	} else {
		fprintf(fp, "%8d", statbuf->st_uid);
	}

	/* getgrgid() is failing in chroot() */
	if ((g = xgetgrgid(statbuf->st_gid)) != NULL) {
		fprintf(fp, "%8s", g->gr_name);
	} else {
		fprintf(fp, "%8d", statbuf->st_gid);
	}

	fprintf(fp, "%12d %s %s", (int)statbuf->st_size, time_string, filename);

	if (S_ISLNK(statbuf->st_mode)) {
		if ((i = readlink(filename, linkname, PATH_MAX-1)) == -1) {
			Nprint_warn("%s: %s", filename, strerror(errno));
		} else {
			linkname[i] = '\0';
			fprintf(fp, " -> %s", linkname);
		}
	}
}

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
		print_file(filename, &statbuf);
		putc('\n', fp);
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

	Start_receiving_sigio(); /* So we can stop the backend
				  * while he's collecting files.
				  */
	status = recursive_action(root);

	Stop_receiving_sigio();

	if (prunes) { /* Free prune list. */
		for (i = 0; prunes[i]; ++i) {
			xfree(prunes[i]);
		}
		xfree(prunes);
		prunes = NULL;
	}

	return status;
}
