#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <alfs.h>
#include <repl.h>
#include <util.h>
#include <build.h>
#include <plugin.h>

#define	VERSION		"0.0.2"

role default_filter[4] = { NOEXECUTE, INTERACTIVE, TESTSUITE, 0 };
char *paralell_filter[4] = { "configure-host", "clean", "mrproper", NULL };
char *popt_pkg[2] = { "Glibc-20041115", NULL };
char *popt_cmd[2] = { "PARALLELMFLAGS=-j", NULL };

int main (int argc, char **argv)
{
	char c, *syn = NULL, *moo_xml = NULL;
	bool quiet = false;
	int i = 0;
	xmlNodePtr cur;
	plug_info *plugin;
	profile *prof = NULL;
	
	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		return 1;
	}

	plugin = plugscan("syntax");

	if (!plugin)
	{
		fprintf(stderr, "No syntax plugins found.\n");
		return 2;
	}

	while ((c = getopt(argc, argv, "s:qVhc:")) != EOF)
	{
		switch (c)
		{
			case 's':
				if (!strcmp(optarg, "help"))
				{
					printf("Available syntaxes:\n");
					while (plugin[i].path)
					{
						char *tmp = plugarg(plugin[i].path);
						if (strcmp(tmp, "sample"))
							printf("\t%s\t\t%s%s\n", tmp, 
								plugin[i].info->name, ((i==0) 
								? " (default)" : ""));
						i++;
					}
					return 0;
				}
				syn = (char *)malloc(strlen(optarg)+1);
				strcpy(syn, optarg);
				break;
			case 'q':
				quiet = true;
				break;
			case 'V':
				printf("moongoo %s\nWritten by Boris Buegling\n", VERSION);
				return 0;
			case 'h':
				printf("moongoo [OPTIONS] BOOK\n");
				printf("\t-s SYNTAX\tChoose syntax (help shows them)\n");
				printf("\t-c CONF.XML\tXML configuration file.\n");
				printf("\t-q\t\tNo output\n");
				printf("\t-V\t\tVersion information\n");
				printf("\t-h\t\tPrint this fluff\n");
				return 0;
			case 'c':
				moo_xml = (char *)malloc(strlen(optarg)+1);
				strcpy(moo_xml, optarg);
				break;
		}
	}

	xmlSubstituteEntitiesDefault(1);
	doc=xmlParseFile(argv[argc-1]);
	if (!doc)
		return 2;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT);
	cur=xmlDocGetRootElement(doc);
	
	if (syn)
	{
		while (plugin[i].path)
		{
			if (!strcmp(syn, plugarg(plugin[i].path)))
				prof = plugin[i].info->parse(cur);
			i++;
		}
	}
	else
		prof = plugin[0].info->parse(cur);
	
	if (!prof)
	{
		fprintf(stderr, "Document was not parsed correctly.\n");
		xmlFreeDoc(doc);
		return 1;
	}
	
	if (!quiet)
	{
		build_paralell (prof, paralell_filter, popt_pkg, popt_cmd);
		set_filter(default_filter);
		/*print_pkg(*search_pkg(prof, "Glibc-20041115", 
			"chapter-building-system"));*/
		print_profile(*prof);
	}
	
	xmlFreeDoc(doc);
	return 0;
}
