/*
 *  download.c - Handler.
 * 
 *  Copyright (C) 2003
 *  
 *  Neven Has <haski@sezampro.yu>
 *  Vassili Dzuba <vdzuba@nerim.net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME download
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "nalfs-core.h"
#include "backend.h"


#define El_download_file(el) alloc_trimmed_param_value("file", el)
#define El_download_destination(el) alloc_trimmed_param_value("destination", el)


#if HANDLER_SYNTAX_3_1

static const char *download_parameters[] =
{ "digest", "file", "url", "destination", NULL };

static int download_main(element_s *el)
{
	/* status assumes failure until set otherwise */
	int status = -1;
	char *file = NULL;
	char *destination = NULL;
	char *digest = NULL;
	char *digest_type = NULL;
	struct stat file_stat;

	/* <file> is mandatory */
	if ((file = El_download_file(el)) == NULL) {
		Nprint_h_err("File name is missing.");
		goto free_all_and_return;
	}

	/* <destination> is mandatory */
	if ((destination = El_download_destination(el)) == NULL) {
		Nprint_h_err("Destination is missing.");
		goto free_all_and_return;
	}
	
	/* changing to <destination> directory */
	if (change_current_dir(destination))
		goto free_all_and_return;

	alloc_element_digest(el, &digest, &digest_type);

	/* Check if file exists. */
	if ((stat(file, &file_stat))) {
	        if (errno == ENOENT && first_param("url", el) != NULL) {
		        int found = 0;
			element_s *p;

			Nprint_h_warn("File %s not found.", file);
			Nprint_h("Trying to fetch it from <url>...");

	                for (p = first_param("url", el); p; p = next_param(p)) {
				char *s;

				if ((s = alloc_trimmed_str(p->content)) == NULL) {
					Nprint_h_warn("Source empty.");
					continue;
				}
				
				append_str(&s, file);
				if (! get_url(s, file, digest, digest_type))
					found = 1;
				xfree(s);
				if (found)
					break;
			}
			
			if (! found) {
				Nprint_h_err("Unable to download file %s.", file);
				goto free_all_and_return;
			}

		} else {
			Nprint_h_err("Checking for %s failed:", file);
			Nprint_h_err("    %s", strerror(errno));
			goto free_all_and_return;
		}
	} else if (digest != NULL) {
		if (verify_digest(digest_type, digest, file)) {
			Nprint_h_err("Wrong %s digest of file: %s",
				     digest_type, file);
			goto free_all_and_return;
		}
	}

	/* operation was successful, set status */
	status = 0;

 free_all_and_return:
	xfree(digest_type);
	xfree(digest);
	xfree(file);
	xfree(destination);

	return status;
}

#endif /* HANDLER_SYNTAX_3_1 */


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_1
	{
		.name = "download",
		.description = "Download",
		.syntax_version = "3.1",
		.parameters = download_parameters,
		.main = download_main,
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
