/*
 *  config.h - Configuration header file.
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


#ifndef H_CONFIG_
#define H_CONFIG_


/* Use element's editor. */
#define USE_EDITOR

/*
 * Environment variable whose value can contain a directory
 * for nALFS to use.
 */
#define ALFS_VAR	"ALFS_DIR"


/* Name of directory (in $HOME) used when alfs_directory is not specified. */
#define ALFS_DIRECTORY_NAME	".nALFS"


/* Permissions to use for created directories. Modified by mask value. */
#define DIRS_MODE	0700


/* Name of the configuration file, located inside user's home directory. */
#define RC_FILE_NAME	".nALFSrc"


/*
 * Maximum lengths of some strings.
 */
#define MAX_DATA_MSG_LEN		1024
#define MAX_CTRL_MSG_LEN		1024
#define MAX_COMMAND_LEN			1024
#define MAX_ACTION_MSG_LEN		1024
#define MAX_XML_ERROR_MSG_LEN		1024
#define MAX_STATE_FILE_LINE_LEN		1024
#define MAX_RC_LINE_LEN			256

/*
 * Some program's info.
 */
#define VERSION            "1.1.8"
#define PROGRAM_NAME       "nALFS"
#define PROGRAM_FULL_NAME  PROGRAM_NAME " v" VERSION
#define EMAIL              "haski@sezampro.yu"
#define SITE               "http://www.beotel.yu/~has/projects/alfs/index.html"
#define COPYRIGHT          "Copyright (C) 2001-2003 Neven Has"


/* Not included in the package, still testing it. */
// #define PARSE_WITH_LIBXML_SAX


#ifdef DO_NOT_USE_INLINE
#define INLINE
#else
#define INLINE inline
#endif


#endif /* H_CONFIG_ */
