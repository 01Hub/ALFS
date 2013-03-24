/*
 *  unpack.c - Handler.
 * 
 *  Copyright (C) 2001-2003, 2005
 *  
 *  Neven Has <haski@sezampro.yu>
 *  Jamie Bennett <jamie@linuxuk.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME unpack
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"
#include "options.h"

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

static int unpack_archive(const char *archive)
{
	char *decompressor = NULL;
	char *unpacker = NULL;
	char *command = NULL;
	int status = -1;

	decompressor = alloc_decompress_command(get_compression_type(archive));
	unpacker = alloc_unpack_command(get_archive_format(archive));

	if (unpacker != NULL) {
		if (decompressor != NULL) {
			command = xstrdup(decompressor);
			append_str(&command, " | ");
		} else {
			command = xstrdup("cat %s | ");
		}
		append_str(&command, unpacker);
		Nprint_h("Unpacking %s...", archive);
		status = execute_command(command, archive);
	} else if (decompressor != NULL) {
		/* Allocate basename (without the last extension). */
		char *base_name = alloc_basename(archive);

		Nprint_h("Decompressing %s...", archive);
		command = xstrdup(decompressor);
		append_str(&command, " > %s");
		status = execute_command(command, archive, base_name);
		xfree(base_name);
	} else {
		Nprint_h_err("Unknown archive (%s).", archive);
	}

	xfree(command);
	xfree(decompressor);
	xfree(unpacker);
	return status;
}


#if HANDLER_SYNTAX_2_0

static const char *unpack_parameters_ver2[] =
{ "archive", "destination", NULL };

static int unpack_main_ver2(element_s *el)
{
	int status = 0;
	char *archive, *destination;
	struct stat file_stat;


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

	if (unpack_archive(archive)) {
		Nprint_h_err("Unpacking %s failed.", archive);
	} else {
		Nprint_h("Done unpacking %s.", archive);
	}

	xfree(archive);
	
	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const char *unpack_parameters_ver3[] =
{ "digest", "reference", "archive", "destination", NULL };

static int unpack_main_ver3(element_s *el)
{
	int status = -1;
	char *archive = NULL;
	char *destination = NULL;
	char *digest = NULL;
	char *digest_type = NULL;
	struct stat file_stat;


	if ((archive = El_unpack_archive(el)) == NULL) {
		Nprint_h_err("Archive name is missing.");
		goto free_all_and_return;
	}

	if ((destination = El_unpack_destination(el)) == NULL) {
		Nprint_h_err("Destination is missing.");
		goto free_all_and_return;
	}
	
	if (change_current_dir(destination))
		goto free_all_and_return;

	if (!*opt_disable_digest)
		alloc_element_digest(el, &digest, &digest_type);

	/* Check if archive exists. */
	if ((stat(archive, &file_stat))) {
	        if (errno == ENOENT && first_param("reference", el) != NULL) {
		        int found = 0;
			element_s *p;

			Nprint_h_warn("Archive %s not found.", archive);
			Nprint_h("Trying to fetch it from <reference>...");

	                for (p = first_param("reference", el); p; p = next_param(p)) {
				char *s;

				if ((s = alloc_trimmed_str(p->content)) == NULL) {
					Nprint_h_warn("Source empty.");
					continue;
				}
				
				if (! get_url(s, archive, digest, digest_type))
					found = 1;
				xfree(s);
				if (found)
					break;
			}
			
			if (! found) {
				Nprint_h_err("Unable to download file %s.", archive);
				goto free_all_and_return;
			}
		} else {
			Nprint_h_err("Checking for %s failed:", archive);
			Nprint_h_err("    %s", strerror(errno));
			goto free_all_and_return;
		}
	} else if (digest && verify_digest(digest_type, digest, archive)) {
		Nprint_h_err("Wrong %s digest of archive: %s",
			     digest_type, archive);
		goto free_all_and_return;
	}

	if (*opt_download_check) {
		Nprint_h("File %s available", archive);
		status = 0;
	}
	else {
		if (unpack_archive(archive)) {
			Nprint_h_err("Unpacking %s failed.", archive);
		} else {
			Nprint_h("Done unpacking %s.", archive);
			status = 0;
		}
	}

 free_all_and_return:
	xfree(digest_type);
	xfree(digest);
	xfree(archive);
	xfree(destination);
	
	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "2.0",
		.parameters = unpack_parameters_ver2,
		.main = unpack_main_ver2,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "3.0",
		.parameters = unpack_parameters_ver3,
		.main = unpack_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "unpack",
		.description = "Unpack",
		.syntax_version = "3.1",
		.parameters = unpack_parameters_ver3,
		.main = unpack_main_ver3,
		.type = 0,
		.alloc_data = NULL,
		.is_action = 1,
		.priority = 0
	},
#endif
	{
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
