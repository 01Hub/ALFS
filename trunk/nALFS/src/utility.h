/*
 *  utility.h - Various useful utilities.
 *
 *  Copyright (C) 2001-2004
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


#ifndef H_UTILITY_
#define H_UTILITY_

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#ifdef DO_ASSERT
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#define WHITE_SPACE	" \n\r\t\v"

#define SKIPWS(c)	while (*(c) && isspace ((int)(*(c)))) ++c

#define Empty_string(s)	((s) == NULL || *s == '\0')

#define Toggle(a)	((a) = (a) ? 0 : 1);
#define Yesno(a)	((a) ? "Yes" : "No")
#define Onoff(a)	((a) ? "On" : "Off")
#define Min(a,b)	(((a) < (b)) ? (a) : (b))

#ifndef WCOREDUMP
#define WCOREDUMP(a) 0
#endif

extern void (*fatal_error)(const char * const file, unsigned int line,
			   const char * const format, ...);

#define Fatal_error(a, b...) do { \
	fatal_error(__FILE__, __LINE__, a, ## b); \
} while (0)

/*
 * Memory allocation utilities.
 */

static inline void *xmalloc(size_t size)
{
	void *value = malloc(size);

	if (value == NULL) {
		Fatal_error("malloc() failed (size %zd)", size);
	}
	return value;
}

static inline void *xrealloc(void *ptr, size_t size)
{
	void *value;

	if ((value = realloc(ptr, size)) == NULL) {
		Fatal_error("realloc() failed (ptr: %p, size %zd)", ptr, size);
	}
	return value;
}

#define xfree free

/*
 * Strings' utilities.
 */

char *xstrdup(const char *str);
char *xstrcasestr(const char *haystack, const char *needle);
char *alloc_trimmed_str(const char *s);
int remove_new_line(char *buf);
void append_str_format(char **ptr, const char *format, ...);
void append_str(char **ptr, const char *str);

/*
 * Files' utilities.
 */

int file_exists(const char *file);
int delete_file(const char *file);
int create_temp_file(char *templ);

/* Returns non-directories with specified suffix, unless suffix is NULL. */
struct dirent *xreaddir(DIR *dir, const char *dir_name, const char *suffix);

/*
 * Misc.
 */

int change_current_dir(const char *dir);
char *get_home_directory(void);

#endif /* H_UTILITY_ */
