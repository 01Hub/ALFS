#include "alfs.h"
#include <build.h>
#include <getopt.h>
#include <libcomm.h>
#include <plugin.h>
#include <remote.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void __print_plug (gpointer data, gpointer user_data)
{
	plug *tst = (plug *)data;
	printf("\t%s\t\t%s %s\n", tst->path, tst->name, !strcmp(tst->path, DEF_SYN)
		? "(default)" : "");
}

int main (int argc, char **argv)
{
	char c, *syn = DEF_SYN, *moo_xml = MOO_XML, *arch = NULL, *out = NULL;
	bool quiet = false, build = false;
	profile *prof;

	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		return 1;
	}
	
	if (comm_init("127.0.0.1", getenv("PWD"))<0)
		return 1;

	while ((c = getopt(argc, argv, "s:qVhc:bCo:a:")) != EOF)
	{
		switch (c)
		{
			case 's':
				if (!strcmp(optarg, "help"))
				{
					GList *plugs;
					comm_wrint(REM_LIST_P);
					if (comm_rdint()!=0)
						return 3;
					plugs = comm_rdlist(comm_rdchunk);
					printf("Available input syntaxes:\n");
					g_list_foreach(plugs, __print_plug, NULL);
					return 0;
				}
				syn = (char *)malloc(strlen(optarg)+1);
				strcpy(syn, optarg);
				break;
			case 'q':
				quiet = true;
				break;
			case 'V':
				printf("alfscl %s\nWritten by Boris Buegling\n", VERSION);
				return 0;
			case 'h':
				printf("alfscl [OPTIONS] BOOK\n");
				printf("\t-b\t\tBuild\n");
				printf("\t-C\t\tNo colors in output\n");
				printf("\t-c CONF.XML\tXML configuration file\n");
				printf("\t-h\t\tPrint this fluff\n");
				printf("\t-q\t\tNo output\n");
				printf("\t-s SYNTAX\tChoose syntax (help shows them)\n");
				printf("\t-o SYNTAX\tOutput the input in another"
					"syntax (help shows them)\n");
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
			case 'o':
				if (!strcmp(optarg, "help"))
				{
					GList *plugs;
					comm_wrint(REM_LIST_O);
					if (comm_rdint()!=0)
						return 3;
					plugs = comm_rdlist(comm_rdchunk);
					printf("Available output syntaxes:\n");
					g_list_foreach(plugs, __print_plug, NULL);
					return 0;
				}
				out = (char *)malloc(strlen(optarg)+1);
				strcpy(out, optarg);
				break;
			case 'a':
				arch = (char *)malloc(strlen(optarg)+1);
				strcpy(arch, optarg);
				break;
			case ':':
			case '?':
				return 1;
		}
	}
	
	comm_wrint(REM_PARSE);
	comm_wrstr(argv[argc-1]);
	comm_wrstr(moo_xml);
	comm_wrstr("");
	comm_wrstr(syn);
	if (comm_rdint()==0)
		prof = profile_reader();
	else
		return 3;
	
	// Package URLs
	/*comm_wrint(REM_PKG_URLS);
	comm_wrstr(PKG_XML);
	if (comm_rdint()!=0)
		return 3;*/
	
	comm_wrint(REM_QUIT);
	comm_deinit();

	if (build)
	{
		comm_wrint(REM_BUILD);
		comm_wrstr(first_pkg(prof)->name);
		if (comm_rdint()!=0)
			return 3;
	}

	if (out)
	{
		comm_wrint(REM_OUTPUT);
		comm_wrstr("ass");
		if (comm_rdint()!=0)
			return 1;
	}
	
	if (!quiet)
	{
		//package *glibc = search_pkg(prof, "glibc", "chapter-building-system");
		//package *gtk = search_pkg(prof, "gtk+", "x-lib");
		//package *glib = search_pkg(prof, "glib", "general-genlib");
		//package *gcc = search_pkg(prof, "gcc", "phase 2");
		
		sed_paralell (prof, paralell_filter, popt_pkg, popt_cmd);
		set_filter(default_filter);
		
		print_profile(*prof);
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
		//print_links(*prof);
	}

	return 0;
}
