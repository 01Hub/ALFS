/*
 *  utility.c - Various useful utilities.
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
#include <pwd.h>
#include <sys/stat.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utility.h"

/*
 * Program-specific part.
 */

#include "win.h"
#include "nprint.h"

/* Fatal error handlers */

void fatal_error_normal(const char * const file, unsigned int line, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\nError in %s, line %d. Program version is %s.\n",
		file, line, VERSION);
	exit(EXIT_FAILURE);
}

/* TODO: set fatal_error global to point here when needed */

void fatal_error_interactive(const char * const file, unsigned int line, const char *format, ...)
{
	va_list ap;

	clear();
	refresh();
	endwin();

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\nError in %s, line %d. Program version is %s.\n",
		file, line, VERSION);
	exit(EXIT_FAILURE);
}

void (*fatal_error)(const char * const file, unsigned int line,
		    const char *format, ...) = fatal_error_normal;

/*
 * Strings' utilities.
 */

char *xstrdup(const char *s)
{
	char *new = strdup(s);


	if (new == NULL) {
		Fatal_error("strdup() failed");
	}
	return new;
}

char *xstrcasestr(const char *haystack_, const char *needle_)
{
	char *tmp;
	char *haystack = xstrdup(haystack_);
	char *needle = xstrdup(needle_);


	tmp = haystack;
	while (*tmp) {
		*tmp = tolower(*tmp);
		++tmp;
	}

	tmp = needle;
	while (*tmp) {
		*tmp = tolower(*tmp);
		++tmp;
	}

	tmp = strstr(haystack, needle);

	free(haystack);
	free(needle);

	return tmp;
}

char *alloc_trimmed_str(const char *s)
{
	int i;
	char *tmp, *source;


	if (Empty_string(s)) {
		return NULL;
	}

	tmp = source = xstrdup(s);

	i = (int) strspn(tmp, WHITE_SPACE);
	memmove(tmp, &tmp[i], strlen(tmp) - i + 1);
	while (*tmp) {
		if (*tmp == '\n') {
			if (! *++tmp) {
				break;
			}

			while (*tmp == '\n') {
				++tmp;
			}

			i = (int) strspn(tmp, WHITE_SPACE);
			memmove(tmp, &tmp[i], strlen(tmp) - i + 1);
		}

		++tmp;
	}

	/* Trim trailing space. */
	while (strlen(source) && isspace(source[strlen(source) - 1])) {
		source[strlen(source) - 1]='\0';
	}

	if (strlen(source) == 0) {
		xfree(source);
		return NULL;

	} else {
		source = xrealloc(source, strlen(source) + 1);
	}

	return source;
}

int remove_new_line(char *buf)
{
	char *tmp;


	if ((tmp = strchr(buf, '\n'))) {
		*tmp = '\0';
		return 1;
	}

	return 0;
}

void append_str_format(char **ptr, const char *format, ...)
{
	va_list ap;
	va_list ap2;
	int old_length = 0;
	int new_length;
	char *value;

	if (Empty_string(format)) {
		return;
	}

	if (*ptr)
		old_length = strlen(*ptr);

	va_start(ap, format);
	va_copy(ap2, ap);
	new_length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

      	value = xrealloc(*ptr, new_length + old_length + 1);
	vsnprintf(value + old_length, new_length + 1, format, ap2);
	va_end(ap2);

	*ptr = value;
}


void append_str(char **ptr, const char *str)
{
	char *value;
	int old_length = 0;
	int new_length;

	if (Empty_string(str)) {
		return;
	}

	if (*ptr)
		old_length = strlen(*ptr);

	new_length = strlen(str);

      	value = xrealloc(*ptr, new_length + old_length + 1);
	memcpy(value + old_length, str, new_length + 1);

	*ptr = value;
}

/*
 * Files' utilities.
 */

int file_exists(const char *file)
{
	FILE *fp;
	struct stat file_stat;


	if (stat(file, &file_stat)) {
		return 0;
	}

	if (! S_ISREG(file_stat.st_mode)) {
		return 0;
	}

	if ((fp = fopen(file, "r")) == NULL) {
		return 0;
	}

	fclose(fp);

	return 1;
}

int delete_file(const char *file)
{
	if (unlink(file)) {
		if (errno != ENOENT) {
			Nprint_err("Deleting file (%s) failed:", file);
			Nprint_err("%s", strerror(errno));
			return -1;

		} else { // File doesn't exist.
			return 0;
		}
	}

	return 1;
}

int create_temp_file(char *templ)
{
	int fd;


	if ((fd = mkstemp(templ)) == -1) {
		Nprint_err("Creating temporary file");
		Nprint_err("%s", templ);
		Nprint_err("failed: %s", strerror(errno));
		return -1;

	} else {
		close(fd);
	}

	return 0;
}

struct dirent *xreaddir(DIR *dir, const char *dir_name, const char *suffix)
{
	struct dirent *next;

	while ((next = readdir(dir)) != NULL) {
		char *fullname = NULL;
		struct stat file_stat;


		if (! Empty_string(suffix)) { /* Check the suffix. */
			char *s;
			size_t name_len = strlen(next->d_name);
			size_t suffix_len = strlen(suffix);

			if (name_len < suffix_len)
				continue;
			
			s = next->d_name + (name_len - suffix_len);
			if (strcmp(s, suffix) != 0) {
				continue;
			}
		}

		append_str(&fullname, dir_name);
		append_str(&fullname, "/");
		append_str(&fullname, next->d_name);

		if (stat(fullname, &file_stat) == 0) {
			if (! S_ISDIR(file_stat.st_mode)) {
				break;
			}
		}

		xfree(fullname);
	}

	return next;
}

/*
 * Misc.
 */

int change_current_dir(const char * const dir)
{
	if (chdir(dir)) {
		Nprint_err("Changing current directory to %s failed:", dir);
		Nprint_err("    %s", strerror(errno));
		return -1;
	}
	return 0;
}

char *get_home_directory()
{
	char *dir;

	dir = getenv("HOME");

	if (Empty_string(dir)) {
		/* No HOME variable, try getting it from passwd. */
		struct passwd *pw = getpwuid(getuid());

		if (pw == NULL) {
			perror("getpwuid()");
			exit(EXIT_FAILURE);
		}

		dir = pw->pw_dir;
	}

	return dir;
}
