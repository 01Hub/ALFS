/*
 *  load_url.c - Use libcurl to download from a URL.
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
#include <nALFS.h>

#include "win.h"

#ifdef HAVE_LIBCURL

typedef struct output_file_s {
	const char *filename;
	FILE *stream;
} output_file_s;

#include <curl/curl.h>

static int my_fwrite(void *buffer, size_t size, size_t nmemb, void *data)
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
