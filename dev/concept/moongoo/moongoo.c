#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <build.h>
#include <libalfs.h>
#include <moongoo.h>
#include <plugin.h>
#include <url.h>

int main (int argc, char **argv)
{
	char c, *syn = DEF_SYN, *moo_xml = MOO_XML, *plug_dir = PLUG_DIR, *f;
	bool quiet = false, build = false;
	int i = 0;
	xmlDocPtr doc;
	xmlNodePtr cur;
	plug_info *plugin;
	profile *prof = NULL;
	
	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		return 1;
	}

	if (getenv("NALFS_PLUGIN_DIR"))
		plug_dir = getenv("NALFS_PLUGIN_DIR");
	plugin = plugscan(plug_dir);

	if (!plugin)
	{
		fprintf(stderr, "No syntax plugins found.\n");
		return 2;
	}

	while ((c = getopt(argc, argv, "s:qVhc:bC")) != EOF)
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
								plugin[i].info->name, ((!strcmp(tmp, DEF_SYN)) 
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
				printf("\t-b\t\tBuild\n");
				printf("\t-C\t\tNo colors in output\n");
				printf("\t-c CONF.XML\tXML configuration file\n");
				printf("\t-h\t\tPrint this fluff\n");
				printf("\t-q\t\tNo output\n");
				printf("\t-s SYNTAX\tChoose syntax (help shows them)\n");
				printf("\t-V\t\tVersion information\n");
				return 0;
			case 'c':
				moo_xml = (char *)malloc(strlen(optarg)+1);
				strcpy(moo_xml, optarg);
				break;
			case 'b':
				build = true;
				quiet = true;
				break;
			case 'C':
				colors = false;
				break;
		}
	}

	xmlSubstituteEntitiesDefault(1);
	doc = xmlParseFile(argv[argc-1]);
	if (!doc)
		return 2;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT);
	cur = xmlDocGetRootElement(doc);

	//print_subtree(cur);

	f = strdog(getenv("HOME"), moo_xml);
	if (access(f, R_OK))
	{
		if (access("moo.xml", R_OK))
			fprintf(stderr, "Configuration file could not be opened.\n");
		else
			r = init_repl("moo.xml");
	}
	else
		r = init_repl(f);
	free(f);

	while (plugin[i].path)
	{
		if (!strcmp(syn, plugarg(plugin[i].path)))
			prof = plugin[i].info->parse(cur, r);
		i++;
	}
	
	if (!prof)
	{
		fprintf(stderr, "Document was not parsed correctly.\n");
		plugunload(plugin);
		xmlFreeDoc(doc);
		return 1;
	}

	if (build)
		build_pkg(prof->ch[0].pkg[0]);

	if (!quiet)
	{
		/*package *glibc = search_pkg(prof, "glibc", 
			"chapter-building-system");*/
		package *gtk2 = search_pkg(prof, "gtk+",
			"x-lib");
		
		sed_paralell (prof, paralell_filter, popt_pkg, popt_cmd);
		set_filter(default_filter);
		
		if (gtk2)
		{
			//print_pkg(*gtk2);
			print_deptree(*prof, *gtk2);
		}
		/*if (glibc)
			print_pkg(*glibc);*/
		
		//print_profile(*prof);
		//print_links(*prof);
	}

	// Package URLs
	//find_urls(PKG_XML, prof);

	plugunload(plugin);
	xmlFreeDoc(doc);
	return 0;
}
