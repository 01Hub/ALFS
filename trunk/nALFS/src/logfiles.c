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

#define SUFFIX_FOR_LOGF			".log"
#define SUFFIX_FOR_FLOG			".files"


void log_f_add_handler_action(xmlDocPtr xml_doc, const char *string)
{
	xmlNewTextChild(xml_doc->children, NULL,
		EL_NAME_FOR_ACTION, (const xmlChar *)string);
}

void log_f_add_installed_files_one_find(xmlDocPtr xml_doc)
{
	xmlNodePtr node;

	node = xmlNewTextChild(
		xml_doc->children, NULL, EL_NAME_FOR_FILES_ROOT, NULL);

	xmlSetProp(node, "method", "time stamp");
}

void log_f_add_installed_files_two_finds(
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

void log_f_add_stopped_time(xmlDocPtr xml_doc, const char *time_str)
{
	xmlNewTextChild(xml_doc->children, NULL,
		"stopped", (const xmlChar *)time_str);
}

void log_f_add_end_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str, int status)
{
	xmlNodePtr node;

	node = xmlNewTextChild(xml_doc->children, NULL,
		(const xmlChar *)name,
		(const xmlChar *)time_str);

	xmlSetProp(node, "mode", "end");
	xmlSetProp(node, "status", status ? "failed" : "done");
}

void log_f_add_start_time(
	xmlDocPtr xml_doc, const char *name, const char *time_str)
{
	xmlNodePtr node;

	node = xmlNewTextChild(xml_doc->children, NULL,
		(const xmlChar *)name,
		(const xmlChar *)time_str);

	xmlSetProp(node, "mode", "start");
}

xmlDocPtr log_f_new_run(const char *name, const char *version)
{
	xmlDocPtr doc = xmlNewDoc("1.0");

	doc->children = xmlNewDocNode(doc, NULL, EL_NAME_FOR_A_RUN, NULL);

	xmlNewTextChild(doc->children, NULL, "name", name);
	xmlNewTextChild(doc->children, NULL, "version", version);

	return doc;
}

/*
 * Implementation of log_f interface.
 */

struct plogf {
	char *dir;		/* Log's directory. */
	char *name;		/* Log's filename. */
	char *fullname;		/* Both -- makes life easier. */

	char *installed;	/* File containing the list
				 * of installed packages. */

	xmlDocPtr doc;		/* We're using libxml2 internally. */
};

struct log_f {
	int cnt;
	struct plogf **list;
};


void log_f_free(struct log_f *log_f)
{
	int i;

	for (i = 0; i < log_f->cnt; ++i) {
		struct plogf *plogf = log_f->list[i];

		xfree(plogf->dir);
		xfree(plogf->name);
		xfree(plogf->fullname);
		xfree(plogf->installed);
		if (plogf->doc != NULL) {
			xmlFreeDoc(plogf->doc);
		}

		xfree(plogf);
	}

	xfree(log_f);
}

/* TODO: Saves only the first log file (all we need, for now). */
int log_f_save(struct log_f *log_f)
{
	if (log_f->cnt > 0) {
		char *filename = log_f->list[0]->fullname;
		xmlDocPtr doc = log_f->list[0]->doc;
		
		xmlSetDocCompressMode(doc, 0);

		if (xmlSaveFormatFile(filename, doc, 1) == -1) {
			return -1;
		} else {
			return 0;
		}
	}

	return -1;
}

char *log_f_get_package_fullname(struct log_f *log_f, int i)
{
	return log_f->list[i]->fullname;
}

/*
 * Multiple log files, initialized from directory.
 */

struct log_f *log_f_init_from_directory(const char *dir_name)
{
	DIR *dir;
	struct dirent *next;
	struct log_f *log_f;


	log_f = xmalloc(sizeof *log_f);

	log_f->cnt = 0;
	log_f->list = NULL;

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
			plogf->doc = xmlParseFile(plogf->fullname);
			if (plogf->doc) {
				plogf->doc->children =
				xmlDocGetRootElement(plogf->doc);
			}

			++log_f->cnt;

			log_f->list = xrealloc(
				log_f->list, log_f->cnt * sizeof *log_f->list);

			log_f->list[log_f->cnt - 1] = plogf;
		}

		closedir(dir);
	}
	
	return log_f;
}

int log_f_get_packages_cnt(struct log_f *log_f)
{
	return log_f->cnt;
}

char *log_f_get_plog_filename(struct log_f *log_f, int i)
{
	if (0 <= i && i < log_f->cnt) {
		return log_f->list[i]->name;
	}

	return NULL;
}

char *log_f_get_flog_filename(struct log_f *log_f, int i)
{
	char *filename = NULL;
	xmlDocPtr doc = NULL;

	if (0 <= i && i < log_f->cnt) {
		doc = log_f->list[i]->doc;
	}

	if (doc != NULL) {
		xmlNodePtr n = n_xmlGetLastElementByName(
			doc->children, EL_NAME_FOR_FILES_NAME);

		if (n) {
			filename = xmlNodeGetContent(n);
		}
	}

	return filename;
}

/*
 * A single log file, initialized from package's string.
 */

static xmlDocPtr log_f_parse_or_create_plogf_doc(struct plogf *plogf)
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
struct log_f *log_f_init_from_package_string(
	const char *pdir, const char *package_str)
{
	struct log_f *log_f;
	struct plogf *plogf;


	plogf = xmalloc(sizeof *plogf);

	plogf->dir = xstrdup(pdir);
	plogf->name = xstrdup(package_str);
	append_str(&plogf->name, SUFFIX_FOR_LOGF);
	plogf->fullname = xstrdup(plogf->dir);
	append_str(&plogf->fullname, "/");
	append_str(&plogf->fullname, plogf->name);
	plogf->installed = NULL;
	plogf->doc = log_f_parse_or_create_plogf_doc(plogf);


	log_f = xmalloc(sizeof *log_f);

	log_f->cnt = 1;
	log_f->list = xmalloc(sizeof *log_f->list);
	log_f->list[0] = plogf;

	return log_f;
}

int log_f_merge_log(struct log_f *log_f, const char *ptr, size_t size)
{
	xmlNodePtr new_node;
	xmlDocPtr doc = log_f->list[0]->doc;

	/* Parse it and unlink its root element (new_node). */
	doc = xmlParseMemory(ptr, size);
	new_node = xmlDocGetRootElement(doc);
	xmlUnlinkNode(new_node);
	xmlFreeDoc(doc);

	/* Do merge. */

	if (xmlAddChild(log_f->list[0]->doc->children, new_node) == NULL) {
		return -1;
	}

	return 0;
}

static xmlNodePtr log_f_get_flog_element(struct log_f *log_f)
{
	return n_xmlGetLastElementByName(
		log_f->list[0]->doc->children, EL_NAME_FOR_FILES_ROOT);
}

int log_f_has_flog(struct log_f *log_f)
{
	return log_f_get_flog_element(log_f) ? 1 : 0;
}

char *log_f_create_flog(struct log_f *log_f)
{
	char *tmp;
	char *filename;


	/* Just in case. */
	xfree(log_f->list[0]->installed);

	filename = xstrdup(log_f->list[0]->fullname);

	tmp = strrchr(filename, '.');
	*tmp = '\0';
	
	append_str(&filename, SUFFIX_FOR_FLOG);
	append_str(&filename, ".XXXXXX");

	if (create_temp_file(filename)) {
		xfree(filename);
		return NULL;
	}

	log_f->list[0]->installed = filename;

	return filename;
}

void log_f_update_with_flog(struct log_f *log_f)
{
	xmlNodePtr c;

	if ((c = log_f_get_flog_element(log_f))) {
		xmlNewTextChild(
		c, NULL, EL_NAME_FOR_FILES_NAME, log_f->list[0]->installed);
	}
}
