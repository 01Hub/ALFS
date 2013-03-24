/*
 *  file_handling.c - Functions for determining and acting on file
 *                    compression and archiving formats.
 * 
 *  Copyright (C) 2003
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nALFS.h>

#include "utility.h"
#include "options.h"


compression_type_e get_compression_type(const char *filename)
{
	if (! (strncmp(filename + strlen(filename) - 2, ".Z", 2)))
	        return COMPRESS_Z;
	else if (! (strncmp(filename + strlen(filename) - 4, ".tgz", 4)))
		return COMPRESS_GZ;
	else if (! (strncmp(filename + strlen(filename) - 3, ".gz", 3)))
		return COMPRESS_GZ;
	else if (! (strncmp(filename + strlen(filename) - 4, ".bz2", 4)))
		return COMPRESS_BZ2;
	else
		return COMPRESS_UNKNOWN;
}

archive_format_e get_archive_format(const char *filename)
{
	if (! (strncmp(filename + strlen(filename) - 7, ".tar.gz", 7)))
		return ARCHIVE_TAR;
	else if (! (strncmp(filename + strlen(filename) - 8, ".tar.bz2", 8)))
		return ARCHIVE_TAR;
	else if (! (strncmp(filename + strlen(filename) - 6, ".tar.Z", 6)))
		return ARCHIVE_TAR;
	else if (! (strncmp(filename + strlen(filename) - 4, ".tgz", 4)))
		return ARCHIVE_TAR;
	else if (! (strncmp(filename + strlen(filename) - 4, ".tar", 4)))
		return ARCHIVE_TAR;
	else if (! (strncmp(filename + strlen(filename) - 4, ".zip", 4)))
		return ARCHIVE_ZIP;
	else if (! (strncmp(filename + strlen(filename) - 4, ".pax", 4)))
		return ARCHIVE_PAX;
	else if (! (strncmp(filename + strlen(filename) - 5, ".cpio", 5)))
		return ARCHIVE_CPIO;
	else
		return ARCHIVE_UNKNOWN;
}


/* This function returns the appropriate command to perform a decompression
   of the specified type to stdout. The default commands are in src/options.c.
   The commands will be passed the filename to decompress as a string
   parameter, so '%s' should be used to substitute the path into the
   command string.
*/
char *alloc_decompress_command(compression_type_e type)
{
	switch (type) {
	case COMPRESS_GZ:
		return xstrdup(*opt_gunzip_command);
	case COMPRESS_BZ2:
		return xstrdup(*opt_bunzip2_command);
	case COMPRESS_Z:
		return xstrdup(*opt_uncompress_command);
	default:
		return NULL;
	}
}


/* This function returns the appropriate command to unpack a file of the
   specified type. The default commands are in src/options.c. The commands
   _should_ be written to expect their input on stdin, but if a particular
   command cannot accept input in that fashion (unzip, for example), '%s'
   can be used in the string to substitute the path of the file to unpack.
   If this is done, however, the unpack command will not be usable in a
   "chain" with a decompressor ahead of it.
*/
char *alloc_unpack_command(archive_format_e format)
{
	switch (format) {
	case ARCHIVE_TAR:
		return xstrdup(*opt_untar_command);
	case ARCHIVE_ZIP:
		return xstrdup(*opt_unzip_command);
	case ARCHIVE_PAX:
		return xstrdup(*opt_unpax_command);
	case ARCHIVE_CPIO:
		return xstrdup(*opt_uncpio_command);
	default:
		return NULL;
	}
}
