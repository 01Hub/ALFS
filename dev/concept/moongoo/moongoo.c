#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <build.h>
#include <libalfs.h>
#include <main.h>
#include <plugin.h>
#include <url.h>

int main (int argc, char **argv)
{
	char c, *syn = DEF_SYN, *moo_xml = MOO_XML, *plug_dir = PLUG_DIR, *f;
	char *out = NULL;
	bool quiet = false, build = false;
	int i = 0, ret = 0;
	xmlDocPtr doc = NULL;
	xmlNodePtr cur;
	plug_info *parsers, *writers;
	profile *prof = NULL;
	
	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		ret=1;
		goto cleanup;
	}

	if (getenv("NALFS_PLUGIN_DIR"))
		plug_dir = getenv("NALFS_PLUGIN_DIR");
	parsers = plugscan(strdog(plug_dir, "syntax/"));
	writers = plugscan(strdog(plug_dir, "output/"));

	if (!parsers)
	{
		fprintf(stderr, "No syntax plugins found.\n");
		ret=2;
		goto cleanup;
	}

	while ((c = getopt(argc, argv, "s:qVhc:bCo:")) != EOF)
	{
		switch (c)
		{
			case 's':
				if (!strcmp(optarg, "help"))
				{
					printf("Available input syntaxes:\n");
					print_plugs(parsers, DEF_SYN);
					goto cleanup;
				}
				syn = (char *)malloc(strlen(optarg)+1);
				strcpy(syn, optarg);
				break;
			case 'q':
				quiet = true;
				break;
			case 'V':
				printf("%s %s\nWritten by Boris Buegling\n", NAME, VERSION);
				goto cleanup;
			case 'h':
				printf("%s [OPTIONS] BOOK\n", NAME);
				printf("\t-b\t\tBuild\n");
				printf("\t-C\t\tNo colors in output\n");
				printf("\t-c CONF.XML\tXML configuration file\n");
				printf("\t-h\t\tPrint this fluff\n");
				printf("\t-q\t\tNo output\n");
				printf("\t-s SYNTAX\tChoose syntax (help shows them)\n");
				printf("\t-o SYNTAX\tOutput the input in another %s",
						"syntax (help shows them)\n");
				printf("\t-V\t\tVersion information\n");
				goto cleanup;
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
			case 'o':
				if (!strcmp(optarg, "help"))
				{
					printf("Available output syntaxes:\n");
					print_plugs(writers, NULL);
					goto cleanup;
				}
				out = (char *)malloc(strlen(optarg)+1);
				strcpy(out, optarg);
				break;
		}
	}

	xmlSubstituteEntitiesDefault(1);
	doc = xmlParseFile(argv[argc-1]);
	if (!doc)
	{
		ret=2;
		goto cleanup;
	}
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

	while (parsers[i].path)
	{
		if (!strcmp(syn, plugarg(parsers[i].path)))
			prof = parsers[i].info->parse(cur, r);
		i++;
	}
	
	if (!prof)
	{
		fprintf(stderr, "Document was not parsed correctly.\n");
		ret=1;
		goto cleanup;
	}

	if (out)
	{
		if (!writers)
		{
			fprintf(stderr, "No output plugins available.\n");
			ret=2;
			goto cleanup;
		}
		
		i=0;
		while (writers[i].path)
		{
			if (!strcmp(out, plugarg(writers[i].path)))
				writers[i].info->write_prof(prof, NULL);
			i++;
		}
		
		goto cleanup;
	}
	
	if (build)
		build_pkg(prof->ch[0].pkg[0]);

	if (!quiet)
	{
		//package *glibc = search_pkg(prof, "glibc", "chapter-building-system");
		//package *gtk = search_pkg(prof, "gtk+", "x-lib");
		//package *glib = search_pkg(prof, "glib", "general-genlib");
		//package *gcc = search_pkg(prof, "gcc", "phase 2");
		
		sed_paralell (prof, paralell_filter, popt_pkg, popt_cmd);
		set_filter(default_filter);
	
		/*if (gcc)
			print_urls(*gcc);*/
		/*if (glib)
			//print_pkg(*glib);
			print_urls(*glib);*/
		/*if (gtk)
			//print_pkg(*gtk);
			print_deptree(*prof, *gtk);*/
		/*if (glibc)
			print_pkg(*glibc);*/
		
		print_profile(*prof);
		//print_links(*prof);
	}

	// Package URLs
	//find_urls(PKG_XML, prof);


cleanup:
	if (parsers)
		plugunload(parsers);
	if (writers)
		plugunload(writers);
	if (doc)
		xmlFreeDoc(doc);
	return ret;
}
