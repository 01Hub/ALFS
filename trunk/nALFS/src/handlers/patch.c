/*
 *  patch.c - Handler.
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
#include <sys/stat.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME patch
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "backend.h"


#if HANDLER_SYNTAX_2_0

static const struct handler_parameter patch_parameters_v2[] = {
	{ .name = "base" },
	{ .name = "param" },
	{ .name = NULL }
};

static int patch_main_ver2(const element_s * const el)
{
	int status;
	char *base;
	char *parameters = NULL;


	if (append_param_elements(&parameters, el) == NULL) {
		Nprint_h_err("No patch parameters specified.");
		return -1;
	}

	base = alloc_base_dir(el);

	if (change_current_dir(base)) {
		xfree(base);
		xfree(parameters);
		return -1;
	}
	
	Nprint_h("Patching in %s", base);
	Nprint_h("    patch %s", parameters);

	if ((status = execute_command(el, "patch %s", parameters))) {
		Nprint_h_err("Patching failed.");
	}
	
	xfree(base);
	xfree(parameters);

	return status;
}

#endif /* HANDLER_SYNTAX_2_0 */


#if HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1

static const struct handler_parameter patch_parameters_v3[] = {
	{ .name = "prefix" },
	{ .name = "param" },
	{ .name = NULL }
};

static const struct handler_attribute patch_attributes_v3[] = {
	{ .name = "base" },
	{ .name = NULL }
};

static int patch_main_ver3(const element_s * const el)
{
	int status;
	char *parameters = NULL;
	char *command;


	if (change_to_base_dir(el, attr_value("base", el), 1))
		return -1;

	if (append_param_elements(&parameters, el) == NULL) {
		Nprint_h_err("No patch parameters specified.");
		return -1;
	}

	Nprint_h("Patching");
	command = xstrdup("");

	append_prefix_elements(&command, el);

	/* the trailing space on the string below is important, since
	   append_param_elements will not have put a space at the
	   beginning of the parameters string (since it was NULL to
	   begin with)
	*/

	append_str(&command, "patch ");

	append_str(&command, parameters);

	Nprint_h("    %s", command);

	if ((status = execute_command(el, command))) {
		Nprint_h_err("Patching failed.");
	}
	
	xfree(command);
	xfree(parameters);

	return status;
}

#endif /* HANDLER_SYNTAX_3_0 || HANDLER_SYNTAX_3_1 */


#if HANDLER_SYNTAX_3_2

static const struct handler_parameter patch_parameters_v3_2[] = {
	{ .name = "prefix" },
	{ .name = "param" },
	{ .name = "digest" },
	{ .name = "reference" },
	{ .name = "file" },
	{ .name = NULL }
};

static const struct handler_attribute patch_attributes_v3_2[] = {
	{ .name = "base" },
	{ .name = "mode" },
	{ .name = "path_strip" },
	{ .name = NULL }
};

static int patch_main_ver3_2(const element_s * const el)
{
        int status = -1;
	char *file = NULL;
	char *command = NULL;
	char *digest = NULL;
	char *digest_type = NULL;
	char *decompressor = NULL;
	char *mode;
	char *path_strip;
	struct stat file_stat;


	if (change_to_base_dir(el, attr_value("base", el), 1))
		return -1;

	if ((file = alloc_trimmed_param_value("file", el)) == NULL) {
		Nprint_h_err("File name is missing.");
		goto free_all_and_return;
	}

	alloc_element_digest(el, &digest, &digest_type);

	/* Check if file exists and if not attempt to download it. */
	if ((stat(file, &file_stat))) {
		if (errno == ENOENT && first_param("reference", el) != NULL) {
			int found = 0;
			element_s *p;

			Nprint_h_warn("File %s not found.", file);
			Nprint_h("Trying to fetch it from <reference>...");

			for (p = first_param("reference", el); p; p = next_param(p)) {
				char *s;

				if ((s = alloc_trimmed_str(p->content)) == NULL) {
					Nprint_h_warn("Source empty.");
					continue;
				}

				if (! get_url(s, file, digest, digest_type)) {
					found = 1;
				}
				xfree(s);
				if (found)
					break;
			}

			if (!found) {
				Nprint_h_err("Unable to download file %s.",
					     file);
				goto free_all_and_return;
			}
		} else {
			Nprint_h_err("Unable to download file %s.", file);
			goto free_all_and_return;
		}
	} else if (digest && verify_digest(digest_type, digest, file)) {
		Nprint_h_err("Wrong %s digest of file: %s", digest_type,
			     file);
		goto free_all_and_return;
	}

	decompressor = alloc_decompress_command(get_compression_type(file));

	if (decompressor == NULL)
		decompressor = xstrdup("cat %s");
	append_str(&decompressor, " | %s");

	command = xstrdup("");

	append_prefix_elements(&command, el);

	append_str(&command, "patch -p");

	path_strip = attr_value("path_strip", el);
	if (path_strip)
		append_str(&command, path_strip);
	else
		append_str(&command, "1");
	
	mode = attr_value("mode", el);
	if (mode) {
		if (strcmp(mode, "forward") == 0)
			append_str(&command, " -N");
		else if (strcmp(mode, "reverse") == 0)
			append_str(&command, " -R");
		else {
			Nprint_h_err("Unknown mode specified: %s", mode);
			goto free_all_and_return;
		}
	} else
		append_str(&command, " -N");

	append_param_elements(&command, el);

	Nprint_h("    %s", command);

	if ((status = execute_command(el, decompressor, file,
				      command))) {
		Nprint_h_err("Patching failed.");
	}
	
free_all_and_return:
	xfree(decompressor);
	xfree(digest_type);
	xfree(digest);
	xfree(file);
	xfree(command);

	return status;
}

#endif /* HANDLER_SYNTAX_3_2 */

/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_2_0
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "2.0",
		.parameters = patch_parameters_v2,
		.main = patch_main_ver2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_0
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.0",
		.parameters = patch_parameters_v3,
		.attributes = patch_attributes_v3,
		.main = patch_main_ver3,
		.type = HTYPE_NORMAL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.1",
		.parameters = patch_parameters_v3,
		.attributes = patch_attributes_v3,
		.main = patch_main_ver3,
		.type = HTYPE_NORMAL,
		.alloc_data = NULL,
		.is_action = 1,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "patch",
		.description = "Patch",
		.syntax_version = "3.2",
		.parameters = patch_parameters_v3_2,
		.attributes = patch_attributes_v3_2,
		.main = patch_main_ver3_2,
		.type = HTYPE_NORMAL,
		.is_action = 1,
		.alternate_shell = 1,
	},
#endif
	{
		.name = NULL
	}
};
