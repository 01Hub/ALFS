/*
 *  libXML-tree.h - Parsing with libxml2 library (tree).
 *
 *  Copyright (C) 2002
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


#ifndef H_LIBXML_
#define H_LIBXML_


#include "parser.h"

void init_libXML_tree(void);

element_s *parse_with_libxml2_tree(const char *filename);

/*
 * Helper functions for accessing nodes in libXML.
 */

#include <libxml/tree.h>

xmlNodePtr n_xmlGetLastElementByName(xmlNodePtr p, const xmlChar *name);


#endif /* H_LIBXML_ */
