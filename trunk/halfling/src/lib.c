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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <libxml/tree.h>
#include "halfling.h"
#include "lib.h"


void log(const char *format, ...) {
    va_list args;
    char msg[256];
    char *tmp = NULL;

    va_start(args, format);
    vsnprintf(msg, sizeof msg, format, args);
    va_end(args);

    if (showtime) {
	tmp = display_time("[%H:%M:%S]");
	printf("%s %s", tmp, msg);
	free(tmp);
    } else {
	printf("%s", msg);
    }
}


char *display_time(const char *format) {
    char str[256];
    struct tm *tm;
    time_t now;

    now = time(NULL);
    tm = localtime(&now);

    if (strftime(str, sizeof(str), format, tm) < 0) {
	return NULL;
    }

    return strdup(str);
}


char *getTag(xmlDoc *doc, xmlNode *cur, const char *name) {
    char *tag = NULL;

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
	if (strcmp(cur->name, name) == 0) {
	    tag = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	    break;
	}

	cur = cur->next;
    }

    return(tag);
}


char *getMultiTag(xmlDoc *doc, xmlNode *cur, const char *name) {
    char *tag = NULL;

    cur = cur->xmlChildrenNode;

    tag = strconcat("", NULL);

    while (cur != NULL) {
	if (strcmp(cur->name, name) == 0) {
	    tag = strconcat(tag, " ",  xmlNodeListGetString(doc, cur->xmlChildrenNode, 1), NULL);
	}

	cur = cur->next;
    }

    if (strcmp(tag, "") == 0) {
	free(tag);
	return("");
    } else {
	return(tag);
    }
}


/* This function is a hack.  Har-har, but it does work for what I want todo.
   A friend suggested I try something similar as he likes to use it all the
   time while coding with glib. */
char *strconcat(const char *str, ...) {
    va_list args;
    char *retval = NULL;
    char *tmp = NULL;
    int len;

    len = strlen(str) + 1;
    retval = malloc(len);
    strcpy(retval, str);

    va_start(args, str);
    tmp = va_arg(args, char *);

    while (tmp) {
        len += strlen(tmp);
        retval = realloc(retval, len);
        strcat(retval, tmp);
        tmp = va_arg(args, char *);
    }

    va_end(args);

    return(retval);
}

/* Should add an else, a-cactch all, considering this will bork,
   if it can't find any of the ones below */
int unpack(const char *destination, const char *source) {
    char *bname = NULL;
    char *tmp = NULL;
    char *cmd = NULL;

    bname = (char *)basename(source);

    if (strcmp(source + strlen(source) - 8, ".tar.bz2") == 0) {
	cmd = strconcat("bzip2 -dc ", source, " | tar -x -C ", destination, NULL);
    } else if (strcmp(source + strlen(source) - 7, ".tar.gz") == 0) {
	cmd = strconcat("tar zxf ", source, " -C ", destination, NULL);
    } else if (strcmp(source + strlen(source) - 4, ".tar") == 0) {
	cmd = strconcat("tar xf ", source, " -C ", destination, NULL);
    } else if (strcmp(source + strlen(source) - 4, ".bz2") == 0) {
	tmp = malloc(strlen(bname) - 3);
	tmp = memset(tmp, '\0', strlen(bname) - 3);
	tmp = strncpy(tmp, bname, strlen(bname) - 4);
	cmd = strconcat("bzip2 -dc ", source, " > ",  destination, "/", tmp, NULL);
	free(tmp);
    } else if (strcmp(source + strlen(source) - 3, ".gz") == 0) {
	tmp = malloc(strlen(bname) - 2);
	tmp = memset(tmp, '\0', strlen(bname) - 2);
	tmp = strncpy(tmp, bname, strlen(bname) - 3);
	cmd = strconcat("gzip -dc ", source, " > ", destination, "/", tmp, NULL);
	free(tmp);
    }

    execute(destination, cmd);

    free(cmd);

    return(0);
}

int execute(const char *base, const char *cmd) {
    FILE *output = NULL;
    char line[1024];

    log("base: %s\n", base);
    log("cmd: %s\n", cmd);

/*
    if (chdir(base)) {
	perror("chdir");
	return(1);
    }

    output = popen(cmd, "r");

    if (output == NULL) {
	perror("popen");
	return(1);
    }

    while (fgets(line, sizeof line, output) != NULL) {
	log("%s", line);
    }

    pclose(output);
*/

    return(0);
}
