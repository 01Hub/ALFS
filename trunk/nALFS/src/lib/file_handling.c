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


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nALFS.h>

#include "utility.h"


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
	else if (! (strncmp(filename + strlen(filename) - 5, ".cpio", 4)))
		return ARCHIVE_CPIO;
	else
		return ARCHIVE_UNKNOWN;
}


char *alloc_decompress_command(compression_type_e type)
{
	switch (type) {
	case COMPRESS_GZ:
		return xstrdup("zcat %s");
		break;
	case COMPRESS_BZ2:
		return xstrdup("bunzip2 -dc %s");
		break;
	case COMPRESS_Z:
		return xstrdup("zcat %s");
		break;
	default:
		return NULL;
	}
}


char *alloc_unpack_command(archive_format_e format)
{
	switch (format) {
	case ARCHIVE_TAR:
		return xstrdup("tar xv");
		break;
	case ARCHIVE_ZIP:
		/* since the "unzip" command does not support reading
		   from standard input, and since it also will not be
		   used in combination with another compression type,
		   this unpack command gets the filename parameter
		*/
		return xstrdup("unzip %s");
		break;
	case ARCHIVE_PAX:
	case ARCHIVE_CPIO:
	default:
		return NULL;
	}
}
