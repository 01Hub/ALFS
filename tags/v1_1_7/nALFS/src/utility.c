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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utility.h"

/*
 * Program-specific part.
 */

#include "win.h" /* For Nprint_err.*/
#include "nalfs.h" /* For Fatal_error. */

#define PRINT_ERROR Nprint_err
#define FATAL_ERROR Fatal_error

/*
 * End of program-specific part.
 */


static const char pfile_location[] = "/tmp/pfile.output";


/*
 * Memory allocation utilities.
 */

void *xmalloc(size_t size)
{
	void *value = malloc(size);


	if (value == NULL) {
		FATAL_ERROR("malloc() failed");
	}

#ifdef DEBUG_MEMORY
	pfile("more");
#endif

	return value;
}

void *xrealloc(void *ptr, size_t size)
{
	void *value;

#ifdef DEBUG_MEMORY
	if (ptr == NULL) {
		pfile("more");
	}
#endif

	if ((value = realloc(ptr, size)) == NULL) {
		FATAL_ERROR("relloc() failed");
	}

	return value;
}

#ifdef DEBUG_MEMORY
void xfree(void *ptr)
{
	if (ptr) {
		pfile("less");
	}
	free(ptr);
}
#endif

/*
 * Functions operating on strings in some way.
 */

char *xstrdup(const char *s)
{
	char *new = strdup(s);


	if (new == NULL) {
		FATAL_ERROR("strdup() failed");
	}
#ifdef DEBUG_MEMORY
	pfile("more");
#endif

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

void append_str(char **ptr, const char *str)
{
	char *value;


	if (Empty_string(str)) {
		return;
	}

	if (*ptr) {
		value = xrealloc(*ptr, strlen(*ptr) + strlen(str) + 1);
		strcat(value, str);
	} else {
		value = xmalloc(strlen(str) + 1);
		strcpy(value, str);
	}

	*ptr = value;
}

/*
 * Misc.
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
		PRINT_ERROR("Creating temporary file (%s) failed:", templ);
		PRINT_ERROR("    %s", strerror(errno));
		return -1;

	} else {
		close(fd);
	}

	return 0;
}

int change_current_dir(const char *dir)
{
	if (chdir(dir)) {
		PRINT_ERROR("Changing current directory to %s failed:", dir);
		PRINT_ERROR("    %s", strerror(errno));

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

/* Useful when debugging. */
void pfile(const char *format, ...)
{
	va_list ap;
	FILE *fp;


	if ((fp = fopen(pfile_location, "a")) == NULL) {
		return;
	}

	va_start(ap, format);
	vfprintf(fp, format, ap);
	va_end(ap);

	fputc('\n', fp);

	fclose(fp);
}
