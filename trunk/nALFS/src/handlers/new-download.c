/*
 *  new-download.c - Handler.
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

#include "utility.h"
#include "win.h"
#include "handlers.h"
#include "parser.h"
#include "nalfs.h"
#include "backend.h"
#include "config.h"
#include "digest.h"

#define El_download_file(el) alloc_trimmed_param_value("file", el)
#define El_download_destination(el) alloc_trimmed_param_value("destination", el)
#define El_download_digest(el) alloc_trimmed_param_value("digest", el)

#ifdef HAVE_LIBCURL

typedef struct output_file_s {
	const char *filename;
	FILE *stream;
} output_file_s;

#include <curl/curl.h>

int my_fwrite(void *buffer, size_t size, size_t nmemb, void *data)
{
	output_file_s *output = (output_file_s *)data;


	if (output->stream == NULL) { /* Open the output file. */
		if ((output->stream = fopen(output->filename, "wb")) == NULL) {
			Nprint_err("Unable to open %s for writing.",
					output->filename);
			return -1;
		}
	}

	return fwrite(buffer, size, nmemb, output->stream);
}

int load_url(const char *archive, const char *url)
{
	CURL *handle;
	CURLcode err;
	char error_buffer[CURL_ERROR_SIZE];
	output_file_s output = { archive, NULL };
	FILE *error_file = NULL;


	Nprint("Downloading with Curl:");
	Nprint("    %s", url);

	if ((err = curl_global_init(CURL_GLOBAL_ALL)) != 0) {
		Nprint_err("Error initializing Curl (%d).", err);
		return -1;
	}

	if ((handle = curl_easy_init()) == NULL) {
		Nprint_err("Error initializing easy interface.");
		curl_global_cleanup();
		return -1;
	}

	/* Can't ignore CURLOPT_STDERR when CURLOPT_ERRORBUFFER is set? */
	error_file = fopen("/dev/null", "w");
	if (error_file) {
		curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, error_buffer);
		curl_easy_setopt(handle, CURLOPT_STDERR, error_file);
	}
	/* Set URL to download, callback function and the output file. */
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, my_fwrite);
	curl_easy_setopt(handle, CURLOPT_FILE, &output);
	curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);

	/* Start downloading. */
	err = curl_easy_perform(handle);

	/* Cleanup. */
	curl_easy_cleanup(handle);
	curl_global_cleanup();

	if (output.stream) { /* Close output file. */
		fclose(output.stream);
	}
	if (error_file) { /* Close error file. */
		fclose(error_file);
	}

	if (err != CURLE_OK) {
		if (error_file) {
			Nprint_err("Downloading with Curl failed:");
			Nprint_err("    %s", error_buffer);
		} else {
			Nprint_err("Downloading with Curl failed (%d).", err);
		}

		return -1;
	}

	Nprint("Downloading with Curl completed successfully.");

	return 0;
}

#endif


static INLINE int get_url(const char *urldir, const char *file)
{
	int status;
	struct stat file_stat;
	char *url = malloc(strlen(urldir) + strlen(file) + 1);
	
	strcpy(url, urldir);
	strcat(url, file);

	/* TODO: We need to make sure that a directory for archive exists. */

#ifdef HAVE_LIBCURL
	status = load_url(file, url);
#else
	status = execute_command("wget --progress=dot -O %s %s%s", file, url);
#endif

	if (status) {
		Nprint_h_err("Getting url failed:");
		Nprint_h_err("    %s", url);

		/* TODO: Should we delete the broken archive here? */

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

char handler_name[] = "download";
char handler_description[] = "Download";
char *handler_syntax_versions[] = { "3.1", NULL };
// char *handler_attributes[] = { NULL };
char *handler_parameters[] =
	{ "digest", "file", "url", "destination", NULL };
int handler_action = 1;


int handler_main(element_s *el)
{
	int status = 0;
	char *file;
	char *destination;
	char *digest;
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
				break;
			      }

			      xfree(s);
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
	}

	if ((digest = El_download_digest(el)) != NULL) {
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

		if (verify_digest(type, digest, file)) {
			Nprint_h_err("Wrong %s digest of file: %s",
				type, file);
			xfree(file);
			xfree(destination);
			return -1;
		}
	}

	xfree(file);
	xfree(destination);
	
	return status;
}
