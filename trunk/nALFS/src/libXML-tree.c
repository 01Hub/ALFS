/*
 *  libXML-tree.c - Parsing with libxml2 library (tree).
 *
 *  Copyright (C) 2002-2004
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

static const struct handler_attribute *find_handler_attribute(const handler_info_s *handler,
							      const char *name)
{
	int i;
	const struct handler_attribute *attr;

	if (!handler->attributes)
		return NULL;

	for (i = 0; (handler->attributes[i].name); ++i) {
		attr = &handler->attributes[i];
		if (strcmp(attr->name, name) == 0)
			return attr;
	}

	return NULL;
}

static const struct handler_parameter *find_handler_parameter(const handler_info_s *handler,
							      const char *name)
{
	int i;
	const struct handler_parameter *param;

	if (!handler->parameters)
		return NULL;

	for (i = 0; (handler->parameters[i].name); ++i) {
		param = &handler->parameters[i];
		if (strcmp(param->name, name) == 0)
			return param;
	}

	return NULL;
}

static int parse_node_attributes(xmlNodePtr node, element_s *element)
{
	handler_info_s *handler = element->handler;
	xmlAttrPtr prop;
	const struct handler_attribute *attr;
	int result;

	/* Pass any attributes present in the node to the
	   handler so it can check their values and store
	   any data associated with them.
	*/

	for (prop = node->properties; prop; prop = prop->next) {
		const char *content = NULL;

		attr = find_handler_attribute(handler,
					      (const char *) prop->name);
		if (!attr) {
			Nprint_warn("<%s>: \"%s\" attribute is not supported.", handler->name, (const char *) prop->name);
			continue;
		}
		
		if (prop->children && prop->children->content)
			content = (const char *) prop->children->content;

		if (content && !attr->untrimmed)
			content = alloc_trimmed_str(content);

		if (!attr->content_optional && (!content ||
						(strlen(content) == 0))) {
			Nprint_err("<%s>: \"%s\" attribute cannot be empty.", handler->name, attr->name);
			result = 1;
		} else {
			result = handler->attribute(element, attr, content);
		}

		if (content && !attr->untrimmed)
			xfree(content);

		if (result)
			return result;
	}

	return 0;
}

static int parse_node_parameters(xmlNodePtr node, element_s *element)
{
	handler_info_s *handler = element->handler;
	xmlNodePtr child;
	const struct handler_parameter *param;
	int result;

	/* Check all elements inside the node to see if they are
	   either acceptable parameters for the handler or
	   elements with known handlers.
	*/

	for (child = node->children; child; ) {
		if (!USED_NODE(child)) {
			child = child->next;
			continue;
		}

		param = find_handler_parameter(handler,
					       (const char *) child->name);

		if (param) {
			const char *content = NULL;
			xmlNodePtr free_child;

			if (child->children && child->children->content)
				content = (const char *) child->children->content;

			if (content && !param->untrimmed)
				content = alloc_trimmed_str(content);

			if (!param->content_optional && (!content ||
							 (strlen(content) == 0))) {
				Nprint_err("<%s>: \"%s\" parameter cannot be empty.", handler->name, param->name);
				result = 1;
			} else {
				result = handler->parameter(element, param, content);
			}

			if (content && !param->untrimmed)
				xfree(content);

			if (result)
				return result;
			/* remove the child node from the DOM tree and free it,
			   it has been handled */
			free_child = child;
			child = child->next;
			xmlUnlinkNode(free_child);
			xmlFreeNode(free_child);
		} else if (!find_handler(child->name, syntax_version)) {
			Nprint_warn("<%s>: <%s> not supported here.", 
				    handler->name,
				    (const char *) child->name);
			child = child->next;
		} else {
			child = child->next;
		}
	}

	return 0;
}

static int make_handler_element(xmlNodePtr node, element_s *element)
{
	handler_info_s *handler = element->handler;
	handler_info_s *parent = element->parent->handler;
	int result;

	/* If the element's parent wants to validate its children */
	if (parent->valid_child) {
		result = parent->valid_child(element->parent, element);
		if (!result) {
			Nprint_warn("<%s>: <%s> not valid here.", parent->name, handler->name);
			return -1;
		}
	} else {
		Nprint_warn("<%s>: <%s> not valid here.", parent->name, handler->name);
		return -1;
	}

	if (handler->setup && ((result = handler->setup(element)) == 0)) {
		result = parse_node_attributes(node, element);
		if (result)
			return result;

		result = parse_node_parameters(node, element);
		if (result)
			return result;

		/* If the handler cares about its content, pass it in
		   and check the result. */

		if (handler->content) {
			xmlChar *content;

			if (node->children
			    && node->children->type == XML_TEXT_NODE
			    && node->children->next == NULL) {
				if ((content = xmlNodeGetContent(node))) {
					result = handler->content(element,
								  content);
					xfree(content);
					if (result)
						return result;
				}
			}

		}

		/* If the handler wants to validate its private data */
		if (handler->valid_data) {
			result = handler->valid_data(element);
			if (!result)
				return -1;
		}
	}

	return 0;
}

static INLINE element_s *create_element(xmlNodePtr node, element_s *parent)
{
	xmlChar *c;
	handler_s *handler;
	element_s *el = init_new_element();


	el->id = element_id++;
	el->parent = parent;

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
				el->handler = handler->info;
				if (make_handler_element(node, el)) {
					free_element(el);
					el = NULL;
					break;
				}
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

			break;
		case XML_COMMENT_NODE:
			el->name = xstrdup("comment");
			el->type = TYPE_COMMENT;
			el->handler = find_handler("comment", syntax_version)->info;
			if (make_handler_element(node, el)) {
				free_element(el);
				el = NULL;
				break;
			}

			break;
		default:
			break;
	}

	return el;
}

static element_s *convert_nodes(xmlNodePtr node, element_s *profile,
				element_s *parent)
{
	element_s *el, *c, *prev = NULL;
	xmlNodePtr child;


	if (! USED_NODE(node)) {
		return NULL;
	}

	if ((el = create_element(node, parent))) {
		for (child = node->children; child; child = child->next) {
			if ((c = convert_nodes(child, profile, el))) {
				link_element(c, prev, el, profile);
				prev = c;
			}
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
	profile->profile = profile;
	profile->handler = find_handler("__root", "all")->info;

	for (child = doc->children; child; child = child->next) {
		if ((el = convert_nodes(child, profile, profile))) {
			link_element(el, prev, profile, profile);

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
