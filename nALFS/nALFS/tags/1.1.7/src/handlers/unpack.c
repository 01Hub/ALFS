/*
 *  unpack.c - Handler.
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


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utility.h"
#include "win.h"
#include "parser.h"
#include "backend.h"
#include "config.h"


#define El_unpack_archive(el) alloc_trimmed_param_value("archive", el)
#define El_unpack_destination(el) alloc_trimmed_param_value("destination", el)


typedef enum extension_e {
	GZ, TAR_GZ, TGZ, BZ2, TAR_BZ2, TAR, ZIP, TAR_Z, Z, UNKNOWN
} extension_e;


static INLINE char *alloc_basename(const char *archive)
{
	char *base_name;
	char *tmp = strrchr(archive, '/');


	if (tmp == NULL) {
		base_name = xstrdup(archive);
	} else {
		base_name = xstrdup(tmp + 1);
	}

	if ((tmp = strrchr(base_name, '.'))) {
		*tmp = '\0';
		tmp++;
	}

	return base_name;
}


char handler_name[] = "unpack";
char handler_description[] = "Unpack";
char *handler_syntax_versions[] = { "2.0", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] = { "archive", "destination", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status = 0;
	char *base_name;
	char *archive, *destination;
	struct stat file_stat;
	extension_e extension = UNKNOWN;


	if ((archive = El_unpack_archive(el)) == NULL) {
		Nprint_h_err("Archive name missing.");
		return -1;
	}

	if ((destination = El_unpack_destination(el)) == NULL) {
		Nprint_h_err("Destination missing.");
		xfree(archive);
		return -1;
	}
	
	if (change_current_dir(destination)) {
		xfree(archive);
		xfree(destination);
		return -1;
	}
	xfree(destination);

	/* Check if archive exists. */
	if (stat(archive, &file_stat)) {
		Nprint_h_err("Checking for %s failed:", archive);
		Nprint_h_err("    %s", strerror(errno));
		xfree(archive);
		return -1;
	}

	/* Watch for the order ! */
	if (! (strncmp(archive + strlen(archive) - 7, ".tar.gz", 7)))
		extension = TAR_GZ;
	else if (! (strncmp(archive + strlen(archive) - 8, ".tar.bz2", 8)))
		extension = TAR_BZ2;
	else if (! (strncmp(archive + strlen(archive) - 6, ".tar.Z", 6)))
		extension = TAR_Z;
	else if (! (strncmp(archive + strlen(archive) - 2, ".Z", 2)))
		extension = Z;
	else if (! (strncmp(archive + strlen(archive) - 4, ".tgz", 4)))
		extension = TGZ;
	else if (! (strncmp(archive + strlen(archive) - 3, ".gz", 3)))
		extension = GZ;
	else if (! (strncmp(archive + strlen(archive) - 4, ".bz2", 4)))
		extension = BZ2;
	else if (! (strncmp(archive + strlen(archive) - 4, ".tar", 4)))
		extension = TAR;
	else if (! (strncmp(archive + strlen(archive) - 4, ".zip", 4)))
		extension = ZIP;
	else
		extension = UNKNOWN;
	
	/* Allocate basename (without the last extension). */
	base_name = alloc_basename(archive);

	switch (extension) {
		case GZ: 
		case Z: 
			Nprint_h("Unpacking %s...", archive);
			status = execute_command("zcat %s > %s",
				archive, base_name);
			break;

		case TGZ:
		case TAR_GZ:
		case TAR_Z:
			Nprint_h("Unpacking %s...", archive);
			status = execute_command("tar xzvf %s", archive);
			break;

		case BZ2:
			Nprint_h("Unpacking %s...", archive);
			status = execute_command("bzip2 -dc %s > %s",
				archive, base_name);
			break;

		case TAR_BZ2:
			Nprint_h("Unpacking %s...",
				archive);
			status = execute_command(
				"tar --use-compress-prog=bunzip2 -xvf %s",
				archive);
			break;

		case TAR:
			Nprint_h("Unpacking %s...", archive);
			status = execute_command("tar xvf %s", archive);
			break;

		case ZIP:
			Nprint_h("Unpacking %s...", archive);
			status = execute_command("unzip %s", archive);
			break;

		case UNKNOWN:
			Nprint_h_err("Unknown archive (%s).", archive);
			status = -1;
	}

	if (status) {
		Nprint_h_err("Unpacking %s failed.", archive);
	} else {
		Nprint_h("Done unpacking %s.", archive);
	}

	xfree(base_name);
	xfree(archive);
	
	return status;
}
