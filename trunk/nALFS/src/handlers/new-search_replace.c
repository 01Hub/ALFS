/*
 *  new-search_replace.c - Handler.
 * 
 *  Copyright (C) 2001-2003
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
#include <sys/stat.h>

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "handlers.h"
#include "backend.h"
#include "config.h"
#include "nalfs.h"


#define El_search_replace_file(el) alloc_trimmed_param_value("file", el)
#define El_search_replace_find(el) alloc_trimmed_param_value("find", el)
#define El_search_replace_replace(el) alloc_trimmed_param_value("replace", el)

#define TMP_SEARCH_REPLACE_FILE	"/tmp/nALFS-XXXXXX"


char handler_name[] = "search_replace";
char handler_description[] = "Search and replace";
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { "base", NULL };
char *handler_parameters[] = { "find", "replace", "file", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int i, c, fdw, offset, num_found = 0;
	char *buf = NULL;
	char *file;
	char *base;
	char *find;
	char *replace = El_search_replace_replace(el);
	char tmp_file[] = TMP_SEARCH_REPLACE_FILE;
	FILE *fp, *fpw;
	struct stat file_stat;


	if ((find = El_search_replace_find(el)) == NULL) {
		Nprint_h_err("No find string specified.");
		return -1;
	}

	if ((file = El_search_replace_file(el)) == NULL) {
		Nprint_h_err("No file specified.");
		xfree(find);
		return -1;
	}

	base = alloc_base_dir_new(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(find);
		xfree(file);
		return -1;
	}

	Nprint_h("Searching %s (%s)",  file, base);
	Nprint_h("    for \"%s\"", find);
	Nprint_h("    and replacing with \"%s\".",
		replace ? replace : "");

	xfree(base);


	if ((fp = fopen(file, "r")) == NULL) {
		Nprint_h_err("Opening %s failed: %s",
			file, strerror(errno));
		xfree(find);
		xfree(file);
		return -1;

	}

	if (fstat(fileno(fp), &file_stat)) {
		Nprint_h_err("Error fstat()ing %s: %s",
			file, strerror(errno));
		xfree(find);
		xfree(file);
		return -1;
	}

	if ((fdw = mkstemp(tmp_file)) == -1) {
		Nprint_h_err("Error opening temporary file: %s",
			strerror(errno));
		xfree(find);
		xfree(file);
		return -1;
	}
	

	if ((fpw = fdopen(fdw, "w")) == NULL) {
		Nprint_h_err("Opening %s failed: %s",
			tmp_file, strerror(errno));
		xfree(find);
		xfree(file);
		return -1;
	}

	offset = (int)strlen(find) - 1;

	buf = xmalloc(strlen(find) + 1);

	for (i = 0; i < (int)(strlen(find)) + 1; i++) {
		buf[i] = '\0';
	}

	while ((c = getc(fp)) != EOF) {
		/* Shift to left, adding c at the end. */
		i = offset;
		while ((buf[i+1])) {
			buf[i] = buf[i+1];
			i++;
		}
		buf[i] = c;

		if (strcmp(buf + offset, find) == 0) {
			num_found++;
			fseek(fpw, (long)-strlen(find) + 1, SEEK_CUR);

			if (replace) {
				fprintf(fpw, "%s", replace);
			}

		} else {
			putc(c, fpw);
		}

		if (offset > 0)
			offset--;
	}

	xfree(buf);

	fclose(fpw);
	fclose(fp);

	Nprint_h("Made %d change%s.",
		num_found, num_found != 1 ? "s" : ""); /* :-) */

	if (execute_command("mv -f %s %s", tmp_file, file)) {
		Nprint_h_err("System command for moving %s to %s failed.",
			tmp_file, file);
		xfree(find);
		xfree(file);
		return -1;
	}

	/* Changing the file mode. */
	if (chmod(file, file_stat.st_mode)) {
		Nprint_h_err("chmod(%s) failed: %s",
			file, strerror(errno));
		xfree(find);
		xfree(file);
		return -1;
	}

	xfree(find);
	xfree(file);

	return 0;
}
