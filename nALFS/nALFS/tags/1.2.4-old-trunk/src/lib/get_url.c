/*
 *  get_url.c - Download a file from a URL, optionally checking a digest.
 * 
 *  Copyright (C) 2001-2003
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

#include <sys/stat.h>
#include <libgen.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <nALFS.h>

#include "utility.h"
#include "win.h"
#include "backend.h"


int get_url(const char *url, const char *destination, const char *digest,
	    const char *digest_type)
{
	/* status defaults to failure */
	int status = -1;
	int command_status;
	struct stat stat_buf;
	char *dirname_buf;
	char *temp_file_name = NULL;
	    

	/* Construct a temporary filename */

	/* have to make a writable copy of the destination path in case
	   the dirname() function wants to modify it
	*/
	dirname_buf = xstrdup(destination);
	append_str_format(&temp_file_name, "%s/.nALFS.XXXXXX", dirname(dirname_buf));
	xfree(dirname_buf);
	if (create_temp_file(temp_file_name))
		goto free_all_and_return;
	/* There is a small risk that another user could create a symlink
	   at the path we just generated before the download starts,
	   which would then put the download results in a different place
	   than intended. While this could be considered a security risk,
	   it is so minor as to be negligible for this application.
	*/
	    
	/* Perform the download into the temporary file, to avoid damaging
	   any existing file during a failed download attempt.
	*/

#ifdef HAVE_LIBCURL
	command_status = load_url(temp_file_name, url);
#else
	command_status = execute_command(NULL,
					 "wget --progress=dot -O %s %s",
					 temp_file_name, url);
#endif

	if (command_status) {
		Nprint_h_err("Getting via URL failed:");
		Nprint_h_err("    %s", url);
		unlink(temp_file_name);
		goto free_all_and_return;
	}

	if (stat(temp_file_name, &stat_buf)) {
		Nprint_h_err("Unable to get %s from url %s:",
			     destination, url);
		Nprint_h_err("    %s", strerror(errno));
		goto free_all_and_return;
	}

	if (rename(temp_file_name, destination)) {
		Nprint_h_err("Unable to move file to %s:", destination);
		Nprint_h_err("    %s", strerror(errno));
		unlink(temp_file_name);
		goto free_all_and_return;
	}

	if (digest && verify_digest(digest_type, digest, destination)) {
		Nprint_h_err("Wrong %s digest of %s", digest_type, url);
		goto free_all_and_return;
	}

        /* all operations successful, set return code */
	status = 0;

 free_all_and_return:
	xfree(temp_file_name);

	return status;
}
