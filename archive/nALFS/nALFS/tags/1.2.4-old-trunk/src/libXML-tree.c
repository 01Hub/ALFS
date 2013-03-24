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
#include <stdarg.h>
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
			xmlChar *xml_content = NULL;
			const char *content = NULL;
			xmlNodePtr free_child;

			if (child->children && xmlNodeIsText(child->children))
				xml_content = xmlNodeGetContent(child->children);

			if (xml_content) {
				if (param->untrimmed) {
					content = xstrdup(xml_content);
				} else {
					content = alloc_trimmed_str(xml_content);
				}
				xmlFree(xml_content);
			}

			if (!param->content_optional && (!content ||
							 (strlen(content) == 0))) {
				Nprint_err("<%s>: \"%s\" parameter cannot be empty.", handler->name, param->name);
				result = 1;
			} else {
				result = handler->parameter(element, param, content);
			}

			xfree(content);

			if (result)
				return result;
			/* remove the child node from the DOM tree and free it,
			   it has been handled */
			free_child = child;
			child = child->next;
			xmlUnlinkNode(free_child);
			xmlFreeNode(free_child);
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
	int result = 0;

	/* If the element's parent wants to validate its children */
	if (parent->valid_child) {
		result = parent->valid_child(element->parent, element);
	}
	if (!result) {
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
			xmlChar *content = NULL;

			if (node->type == XML_COMMENT_NODE) {
				content = xmlNodeGetContent(node);
			} else if (node->children &&
				   xmlNodeIsText(node->children)) {
				content = xmlNodeGetContent(node->children);
			}
			if (content) {
				result = handler->content(element,
							  content);
				xmlFree(content);
				if (result)
					return result;
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

static element_s *create_element(xmlNodePtr node,
				 const element_s * const parent,
				 const element_s * const profile)
{
	element_s *el = NULL;

	if (node->type == XML_ELEMENT_NODE) {
		if ((el = create_handler_element(profile, parent,
						 (const char *) node->name))) {
			if (make_handler_element(node, el)) {
				free_element(el);
				el = NULL;
			}
		}
	} else if (node->type == XML_COMMENT_NODE) {
		if ((el = create_comment_element(profile, parent))) {
			if (make_handler_element(node, el)) {
				free_element(el);
				el = NULL;
			}
		}
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

	if ((el = create_element(node, parent, profile))) {
		for (child = node->children; child; child = child->next) {
			if ((c = convert_nodes(child, profile, el))) {
				link_element(c, prev);
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

	if ((realpath((const char *)doc->URL, resolved_path))) {
		profile = create_profile_element(resolved_path);
	} else {
		profile = create_profile_element((const char *)doc->URL);
	}

	for (child = doc->children; child; child = child->next) {
		if ((el = convert_nodes(child, profile, profile))) {
			link_element(el, prev);
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

	if ((doc = xmlParseFile(filename)) == NULL) {
		Nprint_err("Parsing \"%s\" failed.", filename);
		return NULL;
	}

#ifdef HAVE_XINCLUDE
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT);
#endif

	profile = convert_doc(doc);

	xmlFreeDoc(doc);

	return profile;
}

static const char *make_dotted_version(const char * const version)
{
	char *dotted_version = NULL;
	int version_num = atoi(version);
	int major, minor, patch;
	
	patch = version_num % 100;
	minor = (version_num / 100) % 100;
	major = (version_num / 10000) % 100;
	append_str_format(&dotted_version, "%d.%d.%d", major, minor, patch);

	return dotted_version;
}

void init_libXML_tree(void)
{
	xmlSetGenericErrorFunc(NULL, handle_error);

	LIBXML_TEST_VERSION;

	xmlSubstituteEntitiesDefault(1);

	if (strcmp(xmlParserVersion, LIBXML_VERSION_STRING) == 0) {
		Nprint("Using libXML2, version %s.", LIBXML_DOTTED_VERSION);
	} else {
		const char *installed;

		installed = make_dotted_version(xmlParserVersion);
		Nprint_warn("Using libXML2, version %s (compiled against %s).",
			    installed, LIBXML_DOTTED_VERSION);
		xfree(installed);
	}
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
