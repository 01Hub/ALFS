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
#include <getopt.h>
#include <libxml/tree.h>
#include <libxml/parser.h>


static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {0, 0, NULL, 0}
};


void print_usage(void) {
    printf("Usage: halfling [options] profile...\n");
}


void print_help(void) {
    print_usage();
    printf("\
Options:\n\
  -h,  --help       display this help and exit.\n\
  -v,  --version    print the version information and exit.\
\n");
}


void print_version(void) {
    printf("halfling version CVS, by Jesse Tie-Ten-Quee.\n");

}


int parse_profile(const char *filename) {
    xmlDoc *doc = NULL;
    xmlNode *cur = NULL;

    doc = xmlParseFile(filename);

    if (doc == NULL) {
	fprintf(stderr, "Document is not well-formed.\n");
	return (-1);
    }

    cur = xmlDocGetRootElement(doc);

    if (cur = NULL) {
	fprintf(stderr, "Empty root element.\n");
	xmlFreeDoc(doc);
	return (-1);
    }

    printf("So far so good.\n");

    xmlFreeDoc(doc);

    return (0);
}


int main(int argc, char **argv) {
    int c;


    while ((c = getopt_long(argc, argv, "hv", long_options, (int *)0)) != EOF) {
	switch (c) {
	    case 'h':
		print_help();
		exit(1);
		break;
	    case 'v':
		print_version();
		exit(1);
		break;
	}
    }

    if (optind < argc) {
	while (optind < argc) {
	    parse_profile(argv[optind++]);
	}
    } else {
	print_usage();
	printf("\nTry 'halfing --help' for more information.\n");
    }

    return (0);
}
