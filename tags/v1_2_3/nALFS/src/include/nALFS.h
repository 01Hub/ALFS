/*
 *  nALFS.h - Definition of interfaces to nALFS library.
 * 
 *  Copyright (C) 2003-2003
 *  
 *  Neven Has <haski@sezampro.yu> and
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

#ifndef H_NALFS_
#define H_NALFS_

/* this is the equivalent of an "extern" definition, meaning the actual
   structure definition will be found later during the compile. this is
   needed because we don't want to bring all of parser.h and handlers.h
   into this file at this time.
*/

struct element_s;


/* functions for downloading and verifying file contents */
extern int verify_digest(const char *type, const char *digest, const char *file);
extern void alloc_element_digest(const struct element_s *el, char **digest, char **type);
extern int load_url(const char *output, const char *url);
extern int get_url(const char *url, const char *destination,
		   const char *digest, const char *digest_type);


/* functions for determining and acting upon file compression types and
   archive formats
*/

typedef enum compression_type_e {
	COMPRESS_GZ, COMPRESS_BZ2, COMPRESS_Z, COMPRESS_UNKNOWN
} compression_type_e;

typedef enum archive_format_e {
	ARCHIVE_TAR, ARCHIVE_ZIP, ARCHIVE_PAX, ARCHIVE_CPIO, ARCHIVE_UNKNOWN
} archive_format_e;

extern compression_type_e get_compression_type(const char *filename);
extern archive_format_e get_archive_format(const char *filename);
extern char *alloc_decompress_command(compression_type_e type);
extern char *alloc_unpack_command(archive_format_e format);


/* macro to prefix handler symbols to match libtool expectations for when
   handlers are linked into the main binary (in a static build)
*/

#ifdef MODULE_NAME
#define __HANDLER_SYMBOL(handler_mod, name) handler_mod ## _LTX_ ## handler_ ## name
#define _HANDLER_SYMBOL(handler_mod, name) __HANDLER_SYMBOL(handler_mod,name)
#define HANDLER_SYMBOL(name) _HANDLER_SYMBOL(MODULE_NAME,name)
#else
#define HANDLER_SYMBOL(name) MODULE_NAME_was_not_defined
#endif

#endif /* H_NALFS_ */
