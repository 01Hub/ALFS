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
#include "win.h"
#include "parser.h"
#include "nalfs-core.h"
#include "backend.h"


#define El_download_file(el) alloc_trimmed_param_value("file", el)
#define El_download_destination(el) alloc_trimmed_param_value("destination", el)
#define El_download_digest(el) alloc_trimmed_param_value("digest", el)


static INLINE int get_url(const char *urldir, const char *file)
{
	int status;
	struct stat file_stat;
	char *url = malloc(strlen(urldir) + strlen(file) + 1);
	
	strcpy(url, urldir);
	strcat(url, file);

	/* TODO: We need to make sure that a directory for archive exists. */

	/* check if the file exists and remove it, we can't be sure the
	   chosen downloader will overwrite an existing file
	*/
	if (stat(file, &file_stat))
		unlink(file);

#ifdef HAVE_LIBCURL
	status = load_url(file, url);
#else
	status = execute_command("wget --progress=dot -O %s %s", file, url);
#endif

	if (status) {
		Nprint_h_err("Getting url failed:");
		Nprint_h_err("    %s", url);

		unlink(file);

		xfree(url);
		return -1;
	}

	if (stat(file, &file_stat)) {
		Nprint_h_err("Unable to get %s from url %s:",
			file, url);
		Nprint_h_err("    %s", strerror(errno));

		xfree(url);
		return -1;
	}

	xfree(url);	
	return 0;
}

int download_main(element_s *el)
{
	int status = 0;
	char *file;
	char *destination;
	char *digest;
	char *digest_type;
	struct stat file_stat;

	/* <file> is mandatory */
	if ((file = El_download_file(el)) == NULL) {
		Nprint_h_err("File name is missing.");
		return -1;
	}

	/* <destination> is mandatory */
	if ((destination = El_download_destination(el)) == NULL) {
		Nprint_h_err("Destination is missing.");
		xfree(file);
		return -1;
	}
	
	/* changing to <destination> directory */
	if (change_current_dir(destination)) {
		xfree(file);
		xfree(destination);
		return -1;
	}

	if ((digest = El_download_digest(el)) != NULL) {
		element_s *el2 = first_param("digest", el);

		digest_type = attr_value("type", el2);

		if (digest_type != NULL) {
			char *s;

			for (s = digest_type; *s; s++) {
				*s = tolower(*s);
			}
		}
	  
		if ((digest_type == NULL) || (*digest_type == 0)) {
			digest_type = "md5";
		}
	}

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

  			      if (! get_url(s, file)) {
				found = 1;
			      }

			      if (found && (digest != NULL)) {
				      if (verify_digest(digest_type, digest, file)) {
					      Nprint_h_err("Wrong %s digest of file: %s",
							   digest_type, file);
					      found = 0;
				      }
			      }

			      xfree(s);

			      if (found)
				      break;
			}

			if (! found) {
			  Nprint_h_err("Unable to download file %s.", file);
			  xfree(file);
			  xfree(destination);
			  return -1;
			}

		} else {
			Nprint_h_err("Checking for %s failed:", file);
			Nprint_h_err("    %s", strerror(errno));
			xfree(file);
			xfree(destination);
			return -1;
		}
	} else if (digest != NULL) {
		if (verify_digest(digest_type, digest, file)) {
			Nprint_h_err("Wrong %s digest of file: %s",
				digest_type, file);
			xfree(file);
			xfree(destination);
			return -1;
		}
	}

	xfree(file);
	xfree(destination);
	
	return status;
}


/*
 * Handlers' information.
 */

char *download_parameters[] = { "digest", "file", "url", "destination", NULL };

handler_info_s HANDLER_SYMBOL(info)[] = {
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
	}, {
		NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, 0
	}
};
