/*
 *  libXML-tree.c - Parsing with libxml2 library (tree).
 *
 *  Copyright (C) 2002-2003
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


/* TODO:
 * This is WRONG. Constructing libxml2's tree, reading it and creating
 * our own structures, and then freeing it is plain stupid.
 * But since libxml2's SAX won't play nice, this will have to be used
 * for a while.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxml/tree.h>
#include <libxml/parser.h>
#ifdef HAVE_XINCLUDE
#include <libxml/xinclude.h>
#endif
#include <libxml/xmlerror.h>

#include "bufsize.h"
#include "win.h"
#include "handlers.h"
#include "utility.h"
#include "parser.h"
#include "nalfs-core.h"
#include "options.h"

#include "libXML-tree.h"


#define USED_NODE(node) ( \
	node->type == XML_ELEMENT_NODE || \
	node->type == XML_COMMENT_NODE \
)

#define LIBXML_ERROR_PREFIX "libXML: "


/*
 * XML_ELEMENT_NODE=           1
 * XML_ATTRIBUTE_NODE=         2
 * XML_TEXT_NODE=              3
 * XML_CDATA_SECTION_NODE=     4
 * XML_ENTITY_REF_NODE=        5
 * XML_ENTITY_NODE=            6
 * XML_PI_NODE=                7
 * XML_COMMENT_NODE=           8
 * XML_DOCUMENT_NODE=          9
 * XML_DOCUMENT_TYPE_NODE=     10
 * XML_DOCUMENT_FRAG_NODE=     11
 * XML_NOTATION_NODE=          12
 * XML_HTML_DOCUMENT_NODE=     13
 * XML_DTD_NODE=               14
 * XML_ELEMENT_DECL=           15
 * XML_ATTRIBUTE_DECL=         16
 * XML_ENTITY_DECL=            17
 * XML_NAMESPACE_DECL=         18
 * XML_XINCLUDE_START=         19
 * XML_XINCLUDE_END=           20
 * XML_DOCB_DOCUMENT_NODE=     21
 */


static char *syntax_version = NULL;

static unsigned int element_id;


static INLINE void set_attributes(element_s *el, xmlNodePtr node)
{
	int i = 0;
	xmlAttrPtr attr;


	for (attr = node->properties; attr; attr = attr->next) {
		el->attr = xrealloc(el->attr, (i + 3) * sizeof *el->attr);

		el->attr[i] = xstrdup((const char *)attr->name);
		el->attr[i+1] = xstrdup((const char *)attr->children->content);
		el->attr[i+2] = NULL;

		i += 2;
	}
}

static INLINE element_s *create_element(xmlNodePtr node)
{
	xmlChar *c;
	handler_s *handler;
	element_s *el = init_new_element();


	el->id = element_id++;

	switch (node->type) {
		case XML_ELEMENT_NODE:
			el->name = xstrdup((const char *)node->name);

			/* Change syntax version, if specified. */
			if (strcmp(node->name, "alfs") == 0) {
				char *version = (char *) xmlGetProp(node,
					(const xmlChar *)"version");

				if (version && strlen(version)) {
					xfree(syntax_version);
					syntax_version = version;
				}
			}

			if ((handler = find_handler(el->name, syntax_version))) {
				el->type = TYPE_ELEMENT;
				el->handler = handler;

			} else if (parameter_exists(el->name)) {
				el->type = TYPE_PARAMETER;

			}

			/* Add content, if any. */
			if (node->children
			&& node->children->type == XML_TEXT_NODE
			&& node->children->next == NULL) {
				if ((c = xmlNodeGetContent(node))) {
					el->content = xstrdup((const char *)c);
					xfree(c);
				}
			}

			set_attributes(el, node);

			break;

		case XML_COMMENT_NODE:
			el->name = xstrdup("comment");
			el->type = TYPE_COMMENT;

			if ((c = xmlNodeGetContent(node))) {
				el->content = xstrdup((const char *)c);
				xfree(c);
			}

			break;

		default:
			break;
	}

	return el;
}

static element_s *convert_nodes(xmlNodePtr node)
{
	element_s *el, *c, *prev = NULL;
	xmlNodePtr child;


	if (! USED_NODE(node)) {
		return NULL;
	}

	el = create_element(node);

	for (child = node->children; child; child = child->next) {
		if ((c = convert_nodes(child))) {
			link_element(c, prev, el);

			prev = c;
		}
	}

	return el;
}

static INLINE element_s *convert_doc(xmlDocPtr doc)
{
	char resolved_path[PATH_MAX];
	element_s *el, *profile, *prev = NULL;
	xmlNodePtr child;


	element_id = 0;

	profile = init_new_element();

	if ((realpath((const char *)doc->URL, resolved_path))) {
		profile->name = xstrdup(resolved_path);
	} else {
		profile->name = xstrdup((const char *)doc->URL);
	}

	profile->type = TYPE_PROFILE;
	profile->id = element_id++;

	for (child = doc->children; child; child = child->next) {
		if ((el = convert_nodes(child))) {
			link_element(el, prev, profile);

			prev = el;
		}
	}

	return profile;
}

static void handle_error(void *ctx, const char *msg, ...)
{
	va_list args;
	char *tmp, buffer[MAX_XML_ERROR_MSG_LEN];


	(void)ctx;

	va_start(args, msg);
	vsnprintf(buffer, MAX_XML_ERROR_MSG_LEN, msg, args);
	va_end(args);

	if ((tmp = strrchr(buffer, '\n'))) {
		*tmp = '\0';
	}

	Nprint_err(LIBXML_ERROR_PREFIX "%s", buffer);
}

element_s *parse_with_libxml2_tree(const char *filename)
{
	xmlDocPtr doc;
	element_s *profile = NULL;


	/* Set the default syntax version. It will be changed while
	 * parsing, if there is version="" inside <alfs>.
	 */
	syntax_version = xstrdup(*opt_default_syntax);

	xmlSubstituteEntitiesDefault(1);
	xmlSetGenericErrorFunc(NULL, handle_error);

	if ((doc = xmlParseFile(filename)) == NULL) {
		Nprint_err("Parsing \"%s\" failed.", filename);
		xfree(syntax_version);
		syntax_version = NULL;
		return NULL;
	}

#ifdef HAVE_XINCLUDE
	xmlXIncludeProcess(doc);
#endif

	profile = convert_doc(doc);

	xmlFreeDoc(doc);

	xfree(syntax_version);
	syntax_version = NULL;

	return profile;
}

/*
 * Helper functions for accessing nodes in libXML.
 */

xmlNodePtr n_xmlGetLastElementByName(xmlNodePtr p, const xmlChar *name)
{
	xmlNodePtr n, c;
	xmlNodePtr found = NULL;

	for (n = p->children; n; n = n->next) {
		if (n->type == XML_ELEMENT_NODE) {
			if (xmlStrcmp(n->name, name) == 0) {
				found = n;
			}
		}

		if ((c = n_xmlGetLastElementByName(n, name))) {
			found = c;
		}
	}

	return found;
}
