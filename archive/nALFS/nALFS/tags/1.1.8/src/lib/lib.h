/*
 *  lib.h - Functions in the nALFS library.
 * 
 *  Copyright (C) 2003-2003
 *  
 *  Neven Has <haski@sezampro.yu> and
 *  Kevin P. Fleming <kpfleming@backtobasicsmgmt.com>
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

#ifndef H_LIB_
#define H_LIB_

extern int verify_digest(const char *type, char *digest, const char *file);
extern int load_url(const char *output, const char *url);

#endif /* H_LIB_ */
