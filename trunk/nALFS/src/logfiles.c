/*
 *  logfiles.c - Functions dealing with packages' log files.
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
#include <dirent.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "utility.h"
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

#define SUFFIX_FOR_LOGF			".log"
#define SUFFIX_FOR_FLOG			".files"


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

/*
 * Implementation of logf interface.
 */

struct plogf {
	char *dir;		/* Log's directory. */
	char *name;		/* Log's filename. */
	char *fullname;		/* Both -- makes life easier. */

	char *installed;	/* File containing the list
				 * of installed packages. */

	xmlDocPtr doc;		/* We're using libxml2 internally. */
};

struct logf {
	int cnt;
	struct plogf **list;
};


void logf_free(struct logf *logf)
{
	int i;

	for (i = 0; i < logf->cnt; ++i) {
		struct plogf *plogf = logf->list[i];

		xfree(plogf->dir);
		xfree(plogf->name);
		xfree(plogf->fullname);
		xfree(plogf->installed);
		if (plogf->doc != NULL) {
			xmlFreeDoc(plogf->doc);
		}

		xfree(plogf);
	}

	xfree(logf);
}

/* TODO: Saves only the first log file (all we need, for now). */
int logf_save(struct logf *logf)
{
	if (logf->cnt > 0) {
		char *filename = logf->list[0]->fullname;
		xmlDocPtr doc = logf->list[0]->doc;
		
		xmlSetDocCompressMode(doc, 0);

		if (xmlSaveFormatFile(filename, doc, 1) == -1) {
			return -1;
		} else {
			return 0;
		}
	}

	return -1;
}

char *logf_get_package_fullname(struct logf *logf, int i)
{
	return logf->list[i]->fullname;
}

/*
 * Multiple log files, initialized from directory.
 */

struct logf *logf_init_from_directory(const char *dir_name)
{
	DIR *dir;
	struct dirent *next;
	struct logf *logf;


	logf = xmalloc(sizeof *logf);

	logf->cnt = 0;
	logf->list = NULL;

	if ((dir = opendir(dir_name))) {
		while ((next = readdir(dir)) != NULL) {
			char *s;
			struct plogf *plogf;

			/* Check for the right suffix. */
			s = strrchr(next->d_name, '.');
			if (s == NULL || (strcmp(s, SUFFIX_FOR_LOGF) != 0)) {
				continue;
			}

			plogf = xmalloc(sizeof *plogf);

			plogf->dir = xstrdup(dir_name);
			plogf->name = xstrdup(next->d_name);
			plogf->fullname = xstrdup(plogf->dir);
			append_str(&plogf->fullname, "/");
			append_str(&plogf->fullname, plogf->name);
			plogf->installed = NULL;
			plogf->doc = NULL;

			++logf->cnt;

			logf->list = xrealloc(
				logf->list, logf->cnt * sizeof *logf->list);

			logf->list[logf->cnt - 1] = plogf;
		}

		closedir(dir);
	}
	
	return logf;
}

int logf_get_packages_cnt(struct logf *logf)
{
	return logf->cnt;
}

char *logf_get_package_name(struct logf *logf, int i)
{
	return logf->list[i]->name;
}

/*
 * A single log file, initialized from package's string.
 */

static xmlDocPtr logf_parse_or_create_plogf_doc(struct plogf *plogf)
{
	xmlDocPtr doc;

	if (file_exists(plogf->fullname)) {
		if ((doc = xmlParseFile(plogf->fullname)) != NULL) {
			doc->children = xmlDocGetRootElement(doc);
		}
	} else {
		doc = xmlNewDoc("1.0");
		doc->children = xmlNewDocNode(doc, NULL, EL_NAME_FOR_ROOT, NULL);
	}

	return doc;
}

/* Parses xml file too. */
struct logf *logf_init_from_package_string(
	const char *pdir, const char *package_str)
{
	struct logf *logf;
	struct plogf *plogf;


	plogf = xmalloc(sizeof *plogf);

	plogf->dir = xstrdup(pdir);
	plogf->name = xstrdup(package_str);
	append_str(&plogf->name, SUFFIX_FOR_LOGF);
	plogf->fullname = xstrdup(plogf->dir);
	append_str(&plogf->fullname, "/");
	append_str(&plogf->fullname, plogf->name);
	plogf->installed = NULL;
	plogf->doc = logf_parse_or_create_plogf_doc(plogf);


	logf = xmalloc(sizeof *logf);

	logf->cnt = 1;
	logf->list = xmalloc(sizeof *logf->list);
	logf->list[0] = plogf;

	return logf;
}

int logf_merge_log(struct logf *logf, const char *ptr, size_t size)
{
	xmlNodePtr new_node;
	xmlDocPtr doc = logf->list[0]->doc;


	/* Parse it and unlink its root element (new_node). */
	doc = xmlParseMemory(ptr, size);
	new_node = xmlDocGetRootElement(doc);
	xmlUnlinkNode(new_node);
	xmlFreeDoc(doc);

	/* Do merge. */

	if (xmlAddChild(logf->list[0]->doc->children, new_node) == NULL) {
		return -1;
	}

	return 0;
}

static xmlNodePtr logf_get_flog_element(struct logf *logf)
{
	xmlNodePtr c;
	xmlNodePtr n = xmlGetLastChild(logf->list[0]->doc->children);
	/* n should be EL_NAME_FOR_A_RUN */

	/* Find the last non-text child of n. */
	for (c = n->children; c; c = c->next) {
		if (c->type == XML_ELEMENT_NODE
		&& strcmp((const char *)c->name, EL_NAME_FOR_FILES_ROOT) == 0) {
			return c;
		}
	}

	return NULL;
}

int logf_has_flog(struct logf *logf)
{
	return logf_get_flog_element(logf) ? 1 : 0;
}

char *logf_create_flog(struct logf *logf)
{
	char *tmp;
	char *filename;


	/* Just in case. */
	xfree(logf->list[0]->installed);

	filename = xstrdup(logf->list[0]->fullname);

	tmp = strrchr(filename, '.');
	*tmp = '\0';
	
	append_str(&filename, SUFFIX_FOR_FLOG);
	append_str(&filename, ".XXXXXX");

	if (create_temp_file(filename)) {
		xfree(filename);
		return NULL;
	}

	logf->list[0]->installed = filename;

	return filename;
}

void logf_update_with_flog(struct logf *logf)
{
	xmlNodePtr c;

	if ((c = logf_get_flog_element(logf))) {
		xmlNewTextChild(
		c, NULL, EL_NAME_FOR_FILES_NAME, logf->list[0]->installed);
	}
}
