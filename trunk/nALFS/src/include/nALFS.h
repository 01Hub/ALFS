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

extern int verify_digest(const char *type, const char *digest, const char *file);
extern int load_url(const char *output, const char *url);

#ifdef MODULE_NAME
#define __HANDLER_SYMBOL(handler_mod, name) handler_mod ## _LTX_ ## handler_ ## name
#define _HANDLER_SYMBOL(handler_mod, name) __HANDLER_SYMBOL(handler_mod,name)
#define HANDLER_SYMBOL(name) _HANDLER_SYMBOL(MODULE_NAME,name)
#else
#define HANDLER_SYMBOL(name) MODULE_NAME_was_not_defined
#endif

#endif /* H_NALFS_ */
