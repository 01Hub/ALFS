/*
 *  new-unpack.c - Handler.
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


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utility.h"
#include "win.h"
#include "handlers.h"
#include "parser.h"
#include "nalfs.h"
#include "backend.h"
#include "config.h"
#include "lib.h"


#define El_unpack_reference(el) alloc_trimmed_param_value("reference", el)
#define El_unpack_archive(el) alloc_trimmed_param_value("archive", el)
#define El_unpack_destination(el) alloc_trimmed_param_value("destination", el)
#define El_unpack_digest(el) alloc_trimmed_param_value("digest", el)


typedef enum extension_e {
	GZ, TAR_GZ, TGZ, BZ2, TAR_BZ2, TAR, ZIP, TAR_Z, Z, UNKNOWN
} extension_e;

static INLINE int get_reference(const char *reference, const char *archive)
{
	int status;
	struct stat file_stat;
	    

	/* TODO: We need to make sure that a directory for archive exists. */

#ifdef HAVE_LIBCURL
	status = load_url(archive, reference);
#else
	status = execute_command("wget --progress=dot -O %s %s", archive, reference);
#endif

	if (status) {
		Nprint_h_err("Getting reference failed:");
		Nprint_h_err("    %s", reference);

		/* TODO: Should we delete the broken archive here? */

		return -1;
	}

	if (stat(archive, &file_stat)) {
		Nprint_h_err("Unable to get %s from reference %s:",
			archive, reference);
		Nprint_h_err("    %s", strerror(errno));
		return -1;
	}

	return 0;
}

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
char *handler_syntax_versions[] = { "3.0", "3.1", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] =
	{ "digest", "reference", "archive", "destination", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status = 0;
	char *base_name;
	char *archive;
	char *destination;
	char *digest;
	struct stat file_stat;
	extension_e extension = UNKNOWN;


	if ((archive = El_unpack_archive(el)) == NULL) {
		Nprint_h_err("Archive name is missing.");
		return -1;
	}

	if ((destination = El_unpack_destination(el)) == NULL) {
		Nprint_h_err("Destination is missing.");
		xfree(archive);
		return -1;
	}
	
	if (change_current_dir(destination)) {
		xfree(archive);
		xfree(destination);
		return -1;
	}

	/* Check if archive exists. */
	if ((stat(archive, &file_stat))) {
		char *reference = El_unpack_reference(el);
	
		if (errno == ENOENT && reference != NULL) {
			Nprint_h_warn("Archive %s not found.", archive);
			Nprint_h("Trying to fetch it from <reference>...");

			if (get_reference(reference, archive)) {
				xfree(archive);
				xfree(destination);
				xfree(reference);
				return -1;
			}

			xfree(reference);

		} else {
			Nprint_h_err("Checking for %s failed:", archive);
			Nprint_h_err("    %s", strerror(errno));
			xfree(archive);
			xfree(destination);
			return -1;
		}
	}

	if ((digest = El_unpack_digest(el)) != NULL) {
		element_s *el2 = first_param("digest", el);
		char *type = attr_value("type", el2);
		char *s;


		if (type != NULL) {
			for (s = type; *s; s++) {
				*s = tolower(*s);
			}
		}
	  
		if ((type == NULL) || (*type == 0)) {
			type = "md5";
		}

		if (verify_digest(type, digest, archive)) {
			Nprint_h_err("Wrong %s digest of archive: %s",
				type, archive);
			xfree(archive);
			xfree(destination);
			return -1;
		}
	}

	/* Watch for the order! */
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
			Nprint_h("Unpacking %s...", archive);
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
	xfree(destination);
	
	return status;
}
