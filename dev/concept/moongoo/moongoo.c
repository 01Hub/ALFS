#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <alfs.h>
#include <book.h>
#include <syn.h>
#include <repl.h>
#include <nalfs.h>
#include <ass.h>
#include <util.h>
#include <build.h>

role default_filter[4] = { NOEXECUTE, INTERACTIVE, TESTSUITE, 0 };
char *paralell_filter[4] = { "configure-host", "clean", "mrproper", NULL };
char *popt_pkg[2] = { "Glibc-20041115", NULL };
char *popt_cmd[2] = { "PARALLELMFLAGS=-j", NULL };

int main (int argc, char **argv)
{
	char c, *syn = NULL;
	bool quiet = false;
	xmlNodePtr cur;
	profile *prof;
	
	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		return 1;
	}

	while ((c = getopt(argc, argv, "s:qVh")) != EOF)
	{
		switch (c)
		{
			case 's':
				syn = (char *)malloc(strlen(optarg)+1);
				strcpy(syn, optarg);
				break;
			case 'q':
				quiet = true;
				break;
			case 'V':
				printf("moongoo 0.0.1\nWritten by Boris Buegling\n");
				return 0;
			case 'h':
				printf("moongoo [OPTIONS] BOOK\n");
				printf("\t-s SYNTAX\tChoose syntax\n");
				printf("\t-q\t\tNo output\n");
				printf("\t-V\t\tVersion information\n");
				printf("\t-h\t\tPrint this fluff\n");
				return 0;
		}
	}

	xmlSubstituteEntitiesDefault(1);
	doc=xmlParseFile(argv[argc-1]);
	if (!doc)
		return 2;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT);
	cur=xmlDocGetRootElement(doc);
	
	if ((!syn)||(!strcmp(syn, "book")))
		prof=bookasprofile(cur);
	else
	if (!strcmp(syn, "syn"))
		prof=syn_profile(cur);
	else
	if (!strcmp(syn, "nalfs"))
		prof=nalfs_profile(cur);
	else
	if (!strcmp(syn, "ass"))
		prof=ass_profile(cur);
	else
	{
		fprintf(stderr, "Syntax '%s' is not valid.\n", syn);
		return 1;
	}

	if ((prof) && (!quiet))
	{
		build_paralell (prof, paralell_filter, popt_pkg, popt_cmd);
		set_filter(default_filter);
		print_pkg(*search_pkg(prof, "Glibc-20041115", 
			"chapter-building-system"));
		//print_profile(*prof);
	}
	
	xmlFreeDoc(doc);
	return 0;
}
