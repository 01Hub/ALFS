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

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "libXML-tree.h"
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

#define SUFFIX_FOR_LOGF			".xml"
#define SUFFIX_FOR_FLOG			".files"


struct plogf {
	char *filename;		/* Log's file name. */

	char *name;		/* Name of the package. */
	char *version;		/* Version of the package. */

	char *installed;	/* File containing the list
				 * of installed packages. */

	xmlDocPtr doc;		/* We're using libxml2 internally. */
};

struct logs {
	int cnt;
	struct plogf **list;
};


void logs_add_handler_action(struct logs *logs, const char *string)
{
	xmlNewTextChild(logs->list[0]->doc->children, NULL,
		EL_NAME_FOR_ACTION, (const xmlChar *)string);
}

void logs_add_installed_files(
	struct logs *logs,
	const char *find_base,
	const char *find_prunes)
{
	xmlDocPtr doc;
	xmlNodePtr node;


	doc = logs->list[0]->doc;

	node = xmlNewTextChild(
		doc->children, NULL, EL_NAME_FOR_FILES_ROOT, NULL);

	xmlSetProp(node, "method", "time stamp");

	xmlNewTextChild(node, NULL,
		EL_NAME_FOR_FILES_FIND_ROOT, (const xmlChar *)find_base);

	xmlNewTextChild(node, NULL,
		EL_NAME_FOR_FILES_FIND_PRUNES, (const xmlChar *)find_prunes);
}

void logs_add_stopped_time(struct logs *logs, const char *time_str)
{
	xmlDocPtr doc = logs->list[0]->doc;

	xmlNewTextChild(doc->children, NULL,
		"stopped", (const xmlChar *)time_str);
}

void logs_add_end_time(
	struct logs *logs, const char *name, const char *time_str, int status)
{
	xmlDocPtr doc = logs->list[0]->doc;
	xmlNodePtr node;

	node = xmlNewTextChild(doc->children, NULL,
		(const xmlChar *)name,
		(const xmlChar *)time_str);

	xmlSetProp(node, "mode", "end");
	xmlSetProp(node, "status", status ? "failed" : "done");
}

void logs_add_start_time(
	struct logs *logs, const char *name, const char *time_str)
{
	xmlDocPtr doc = logs->list[0]->doc;
	xmlNodePtr node;

	node = xmlNewTextChild(doc->children, NULL,
		(const xmlChar *)name,
		(const xmlChar *)time_str);

	xmlSetProp(node, "mode", "start");
}

struct logs *logs_init_new_run(const char *name, const char *version)
{
	struct logs *logs;
	struct plogf *plogf;
	xmlDocPtr doc;
       

	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocNode(doc, NULL, EL_NAME_FOR_A_RUN, NULL);

	xmlNewTextChild(doc->children, NULL, "name", name);
	xmlNewTextChild(doc->children, NULL, "version", version);


	plogf = xmalloc(sizeof *plogf);

	plogf->filename = NULL;
	plogf->name = NULL;
	plogf->version = NULL;
	plogf->installed = NULL;
	plogf->doc = doc;


	logs = xmalloc(sizeof *logs);

	logs->cnt = 1;
	logs->list = xmalloc(sizeof *logs->list);
	logs->list[0] = plogf;

	return logs;
}

void logs_dump_to_memory(struct logs *logs, char **ptr, int *size)
{
	xmlDocPtr doc = logs->list[0]->doc;

	xmlDocDumpFormatMemory(doc,  (xmlChar **)ptr, size, 1);
}



void logs_free(struct logs *logs)
{
	int i;

	for (i = 0; i < logs->cnt; ++i) {
		struct plogf *plogf = logs->list[i];

		xfree(plogf->filename);
		xfree(plogf->name);
		xfree(plogf->version);
		xfree(plogf->installed);
		if (plogf->doc != NULL) {
			xmlFreeDoc(plogf->doc);
		}

		xfree(plogf);
	}

	xfree(logs);
}

/* TODO: Saves only the first log file (all we need, for now). */
int logs_save(struct logs *logs)
{
	if (logs->cnt > 0) {
		char *filename = logs->list[0]->filename;
		xmlDocPtr doc = logs->list[0]->doc;
		
		xmlSetDocCompressMode(doc, 0);

		if (xmlSaveFormatFile(filename, doc, 1) == -1) {
			return -1;
		} else {
			return 0;
		}
	}

	return -1;
}

/*
 * Multiple log files, initialized from directory.
 */


static void update_plogf_after_parsing(struct plogf *plogf)
{
	xmlNodePtr p, node;
       

	p = plogf->doc->children;

	node = n_xmlGetLastElementByName(p, "name");
	if (node) {
		plogf->name = (char *) xmlNodeGetContent(node);
	}

	node = n_xmlGetLastElementByName(p, "version");
	if (node) {
		plogf->version = (char *) xmlNodeGetContent(node);
	}

	node = n_xmlGetLastElementByName(p, EL_NAME_FOR_FILES_NAME);
	if (node) {
		plogf->installed = (char *) xmlNodeGetContent(node);
	}
}

struct logs *logs_init_from_directory(const char *dir_name)
{
	DIR *dir;
	struct logs *logs;


	logs = xmalloc(sizeof *logs);

	logs->cnt = 0;
	logs->list = NULL;

	if ((dir = opendir(dir_name))) {
		struct dirent *next;

		while ((next = xreaddir(dir, dir_name, SUFFIX_FOR_LOGF))) {
			struct plogf *plogf;

			plogf = xmalloc(sizeof *plogf);

			plogf->filename = NULL;
			append_str_format(&plogf->filename, "%s/%s",
					  dir_name, next->d_name);
			plogf->name = NULL;
			plogf->version = NULL;
			plogf->installed = NULL;
			plogf->doc = xmlParseFile(plogf->filename);
			if (plogf->doc) {
				plogf->doc->children =
				xmlDocGetRootElement(plogf->doc);

				update_plogf_after_parsing(plogf);
			}

			++logs->cnt;

			logs->list = xrealloc(
				logs->list, logs->cnt * sizeof *logs->list);

			logs->list[logs->cnt - 1] = plogf;
		}

		closedir(dir);
	}
	
	return logs;
}

int logs_get_packages_cnt(struct logs *logs)
{
	return logs->cnt;
}

char *logs_get_plog_filename(struct logs *logs, int i)
{
	if (0 <= i && i < logs->cnt) {
		return logs->list[i]->filename;
	}

	return NULL;
}

char *logs_get_plog_name(struct logs *logs, int i)
{
	if (0 <= i && i < logs->cnt) {
		return logs->list[i]->name;
	}

	return NULL;
}

char *logs_get_plog_version(struct logs *logs, int i)
{
	if (0 <= i && i < logs->cnt) {
		return logs->list[i]->version;
	}

	return NULL;
}

char *logs_get_plog_installed(struct logs *logs, int i)
{
	if (0 <= i && i < logs->cnt) {
		return logs->list[i]->installed;
	}

	return NULL;
}

/*
 * A single log file, initialized from package's string.
 */

static xmlDocPtr logs_parse_or_create_plogf_doc(struct plogf *plogf)
{
	xmlDocPtr doc = NULL;

	if (file_exists(plogf->filename)) {
		if ((doc = xmlParseFile(plogf->filename)) != NULL) {
			doc->children = xmlDocGetRootElement(doc);
		}
	}

 	if (doc == NULL) {
		doc = xmlNewDoc("1.0");
		doc->children = xmlNewDocNode(doc, NULL, EL_NAME_FOR_ROOT, NULL);
	}

	return doc;
}

/* Parses xml file too. */
struct logs *logs_init_from_package_string(
	const char *pdir, const char *package_str)
{
	struct logs *logs;
	struct plogf *plogf;


	plogf = xmalloc(sizeof *plogf);

	plogf->filename = NULL;
	append_str_format(&plogf->filename, "%s/%s%s", pdir,
			  package_str, SUFFIX_FOR_LOGF);
	plogf->name = NULL;
	plogf->version = NULL;
	plogf->installed = NULL;
	plogf->doc = logs_parse_or_create_plogf_doc(plogf);


	logs = xmalloc(sizeof *logs);

	logs->cnt = 1;
	logs->list = xmalloc(sizeof *logs->list);
	logs->list[0] = plogf;

	return logs;
}

int logs_merge_log(struct logs *logs, const char *ptr, size_t size)
{
	xmlNodePtr new_node;
	xmlDocPtr doc = logs->list[0]->doc;

	/* Parse it and unlink its root element (new_node). */
	doc = xmlParseMemory(ptr, size);
	new_node = xmlDocGetRootElement(doc);
	xmlUnlinkNode(new_node);
	xmlFreeDoc(doc);

	/* Do merge. */

	if (xmlAddChild(logs->list[0]->doc->children, new_node) == NULL) {
		return -1;
	}

	return 0;
}

static xmlNodePtr logs_get_flog_element(struct logs *logs)
{
	xmlNodePtr run = n_xmlGetLastElementByName(
		logs->list[0]->doc->children, EL_NAME_FOR_A_RUN);

	return n_xmlGetLastElementByName(run, EL_NAME_FOR_FILES_ROOT);
}

int logs_has_flog(struct logs *logs)
{
	return logs_get_flog_element(logs) ? 1 : 0;
}

char *logs_create_flog(struct logs *logs)
{
	char *tmp;
	char *filename;


	/* Just in case. */
	xfree(logs->list[0]->installed);

	filename = xstrdup(logs->list[0]->filename);

	tmp = strrchr(filename, '.');
	*tmp = '\0';
	
	append_str(&filename, SUFFIX_FOR_FLOG ".XXXXXX");

	if (create_temp_file(filename)) {
		xfree(filename);
		return NULL;
	}

	logs->list[0]->installed = filename;

	return filename;
}

void logs_update_with_flog(struct logs *logs)
{
	xmlNodePtr c;

	if ((c = logs_get_flog_element(logs))) {
		xmlNewTextChild(
		c, NULL, EL_NAME_FOR_FILES_NAME, logs->list[0]->installed);
	}
}
