/* Copyright (c) 2001, 2002, Jesse Tie-Ten-Quee
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    o Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    o Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *
 *    o Neither the name of "Automated Linux From Scratch" nor the names
 *      of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <libxml/tree.h>
#include "lib.h"
#include "tags.h"


void tag_configure(xmlDoc *doc, xmlNode *cur) {
    char *base = NULL;
    char *command = NULL;
    char *options = NULL;
    char *cmd = NULL;

    base = xmlGetProp(cur, "base");
    command = xmlGetProp(cur, "command");
    options = getMultiTag(doc, cur, "option");

    if (command == NULL) {
	cmd = strconcat("./configure", options, NULL);
        log("Running `configure%s' in %s.\n", options, base);
    } else {
	cmd = strconcat(command, " ", options, NULL);
        log("Running `%s%s' in %s\n", command, options, base);
    }

    execute(base, cmd);

    free(base);
    free(command);
    if (options != "") {
	free(options);
    }
    free(cmd);
}


void tag_copy(xmlDoc *doc, xmlNode *cur) {
    char *base = NULL;
    char *source = NULL;
    char *destination = NULL;
    char *cmd = NULL;

    base = xmlGetProp(cur, "base");

    if (base == NULL) {
	base = strconcat("/", NULL);
    }

    source = getTag(doc, cur, "source");
    destination = getTag(doc, cur, "destination");

    cmd = strconcat("cp ", source, " ", destination, NULL);

    log("Copying %s to %s.\n", source, destination);

    execute(base, cmd);

    free(base);
    free(source);
    free(destination);
    free(cmd);
}


void tag_execute(xmlDoc *doc, xmlNode *cur) {
    char *base = NULL;
    char *command = NULL;
    char *options = NULL;
    char *cmd = NULL;

    base = xmlGetProp(cur, "base");
    command = xmlGetProp(cur, "command");
    options = getMultiTag(doc, cur, "option");

    cmd = strconcat(command, options, NULL);

    log("Executing `%s' in %s.\n", cmd, base);

    execute(base, cmd);

    free(base);
    free(command);
    free(options);
    free(cmd);
}


void tag_info(xmlDoc *doc, xmlNode *cur) {
    char *tmp = NULL;

    log("Entering package info.\n");

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
        if (strcmp(cur->name, "name") == 0) {
            tmp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            log("name: %s\n", tmp);
            free(tmp);
        } else if (strcmp(cur->name, "version") == 0) {
            tmp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            log("version: %s\n", tmp);
            free(tmp);
        }

        cur = cur->next;
    }

    log("Leaving package info.\n");
}


void tag_link(xmlDoc *doc, xmlNode *cur) {
    char *base = NULL;
    char *source = NULL;
    char *destination = NULL;
    char *cmd = NULL;

    base = xmlGetProp(cur, "base");
    source = getTag(doc, cur, "source");
    destination = getTag(doc, cur, "destination");
    cmd = strconcat("ln -sf ", source, " ", destination, NULL);

    log("Creating symbolic link %s to %s in %s.\n", source, destination, base);
    execute(base, cmd);

    free(base);
    free(source);
    free(destination);
    free(cmd);
}

/* FIXME: -C isn't needed if we use base. */
void tag_make(xmlDoc *doc, xmlNode *cur) {
    char *base = NULL;
    char *options = NULL;
    char *cmd = NULL;

    base = xmlGetProp(cur, "base");
    options = getMultiTag(doc, cur, "option");
    cmd = strconcat("make -C ", base, options, " 2>&1", NULL);

    log("Running `make%s' in %s.\n", options, base);
    execute(base, cmd);

    free(base);
    if (options != "") {
	free(options);
    }
    free(cmd);
}


void tag_mkdir(xmlDoc *doc, xmlNode *cur) {
    char *base = NULL;
    char *name = NULL;
    char *cmd = NULL;

    base = xmlGetProp(cur, "base");

    if (base == NULL) {
	base = strconcat("/", NULL);
    }

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
	if (strcmp(cur->name, "name") == 0) {
	    name = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    cmd = strconcat("mkdir ", name, NULL);
	    log("Creating directory %s in %s.\n", name, base);
	    execute(base, cmd);
	    free(name);
	    free(cmd);
	}

	cur = cur->next;
    }

    free(base);
}


void tag_package(xmlDoc *doc, xmlNode *cur) {

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
	if (strcmp(cur->name, "info") == 0) {
	    tag_info(doc, cur);
	} else if (strcmp(cur->name, "unpack") == 0) {
	    tag_unpack(doc, cur);
	} else if (strcmp(cur->name, "configure") == 0) {
	    tag_configure(doc, cur);
	} else if (strcmp(cur->name, "copy") == 0 ) {
	    tag_copy(doc, cur);
	} else if (strcmp(cur->name, "execute") == 0) {
	    tag_execute(doc, cur);
	} else if (strcmp(cur->name, "make") == 0) {
	    tag_make(doc, cur);
	} else if (strcmp(cur->name, "mkdir") == 0) {
	    tag_mkdir(doc, cur);
	} else if (strcmp(cur->name, "link") == 0) {
	    tag_link(doc, cur);
	} else if (strcmp(cur->name, "remove") == 0) {
	    tag_remove(doc, cur);
	} else if (strcmp(cur->name, "setenv") == 0) {
	    tag_setenv(doc, cur);
	}

	cur = cur->next;
    }

}


void tag_remove(xmlDoc *doc, xmlNode *cur) {
    char *file = NULL;
    char *cmd = NULL;

    file = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
    cmd = strconcat("rm -rf ", file, NULL);

    log("Removing %s.\n", file);
    execute("/", cmd);

    free(file);
    free(cmd);
}


void tag_setenv(xmlDoc *doc, xmlNode *cur) {
    char *variable = NULL;
    char *value = NULL;

    variable = getTag(doc, cur, "variable");
    value = getTag(doc, cur, "value");

    if (value == NULL) {
	log("Unsetting %s environmental variable.\n", variable);
	unsetenv(variable);
    } else {
	log("Setting environmental variable %s to %s.\n", variable, value);
	setenv(variable, value, 1);
    }

    free(variable);
    free(value);
}


void tag_unpack(xmlDoc *doc, xmlNode *cur) {
    char *source = NULL;
    char *destination = NULL;

    source = getTag(doc, cur, "source");
    destination = getTag(doc, cur, "destination");

    log("Unpacking %s into %s.\n", source, destination);
    unpack(destination, source);

    free(source);
    free(destination);
}
