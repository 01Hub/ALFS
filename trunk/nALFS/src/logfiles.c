/*
 *  logfiles.c - Functions dealing with the format of XML log files
 *               of the packages.
 *
 *  Copyright (C) 2003
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


#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "logfiles.h"


/* Names of the elements used in packages' log files. They are ugly.
 * But this needs some standardization anyway, so it doesn't matter yet.
 */
#define EL_NAME_FOR_ROOT		"package_log"
#define EL_NAME_FOR_A_RUN		"single_try"
#define EL_NAME_FOR_ACTION		"handler_action"
#define EL_NAME_FOR_FILES_ROOT		"changed_files"
#define EL_NAME_FOR_FILES_FIND_ROOT	"find_root"
#define EL_NAME_FOR_FILES_FIND_PRUNES	"find_prunes"
#define EL_NAME_FOR_FILES_NAME		"filename"


void logf_add_handler_action(xmlDocPtr xml_doc, const char *string)
{
	xmlNewTextChild(xml_doc->children, NULL,
		EL_NAME_FOR_ACTION, (const xmlChar *)string);
}

void logf_add_installed_files_one_find(xmlDocPtr xml_doc)
{
	xmlNodePtr node;

	node = xmlNewTextChild(
		xml_doc->children, NULL, EL_NAME_FOR_FILES_ROOT, NULL);

	xmlSetProp(node, "method", "time stamp");
}

void logf_add_installed_files_two_finds(
	xmlDocPtr xml_doc,
	const char *find_base,
	const char *find_prunes)
{
	xmlNodePtr node;


	node = xmlNewTextChild(
		xml_doc->children, NULL, EL_NAME_FOR_FILES_ROOT, NULL);

	xmlSetProp(node, "method", "two finds");

	xmlNewTextChild(node, NULL,
		EL_NAME_FOR_FILES_FIND_ROOT, (const xmlChar *)find_base);

	xmlNewTextChild(node, NULL,
		EL_NAME_FOR_FILES_FIND_PRUNES, (const xmlChar *)find_prunes);
}

void logf_add_stopped_time(xmlDocPtr xml_doc, const char *time_str)
{
	xmlNewTextChild(xml_doc->children, NULL,
		"stopped", (const xmlChar *)time_str);
}

void logf_add_end_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str, int status)
{
	xmlNodePtr node;

	node = xmlNewTextChild(xml_doc->children, NULL,
		(const xmlChar *)name,
		(const xmlChar *)time_str);

	xmlSetProp(node, "mode", "end");
	xmlSetProp(node, "status", status ? "failed" : "done");
}

void logf_add_start_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str)
{
	xmlNodePtr node;

	node = xmlNewTextChild(xml_doc->children, NULL,
		(const xmlChar *)name,
		(const xmlChar *)time_str);

	xmlSetProp(node, "mode", "start");
}

xmlDocPtr logf_new_run(const char *name, const char *version)
{
	xmlDocPtr doc = xmlNewDoc("1.0");

	doc->children = xmlNewDocNode(doc, NULL, EL_NAME_FOR_A_RUN, NULL);

	xmlNewTextChild(doc->children, NULL, "name", name);
	xmlNewTextChild(doc->children, NULL, "version", version);

	return doc;
}



xmlDocPtr logf_parse_logfile(const char *file)
{
	xmlDocPtr doc = NULL;

	if ((doc = xmlParseFile(file)) != NULL) {
		doc->children = xmlDocGetRootElement(doc);
	}

	return doc;
}

xmlDocPtr logf_new_logfile(void)
{
	xmlDocPtr doc = xmlNewDoc("1.0");

	doc->children = xmlNewDocNode(doc, NULL, EL_NAME_FOR_ROOT, NULL);

	return doc;
}


int logf_has_installed_files(xmlNodePtr node)
{
	xmlNodePtr n;

	/* Check if the list will be send. */
	for (n = node->children; n; n = n->next) {
		if (n->type == XML_ELEMENT_NODE
		&& strcmp((const char *)n->name, EL_NAME_FOR_FILES_ROOT) == 0) {
			return 1;;
		}
	}

	return 0;
}

void logf_add_installed_files(xmlNodePtr node, const char *f)
{
	xmlNewTextChild(node, NULL, EL_NAME_FOR_FILES_NAME, f);
}
