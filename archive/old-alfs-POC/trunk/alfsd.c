#include "alfs.h"
#include <build.h>
#include <libcomm.h>
#include <plugin.h>
#include <remote.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

plug_info *parsers = NULL, *writers = NULL;
xmlDocPtr doc = NULL;

static void cleanup ()
{
	if (parsers)
		plugunload(parsers);
	if (writers)
		plugunload(writers);
	if (doc)
		xmlFreeDoc(doc);
}

static profile *parse (char *syn, char *arch, char *moo_xml, char *fname)
{
	char *f;
	int i=0;
	profile *prof = NULL;
	xmlNodePtr cur;
	
	xmlSubstituteEntitiesDefault(1);
	doc = xmlParseFile(fname);
	if (!doc)
		return NULL;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT|XML_PARSE_NONET);
	cur = xmlDocGetRootElement(doc);
	
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

	add_option(r, "arch", arch);
	
	while (parsers[i].path)
	{
		if (!strcmp(syn, plugarg(parsers[i].path)))
			prof = parsers[i].info->parse(cur, NULL); //, r);
		i++;
	}

	return prof;
}	

int main (int argc, char **argv)
{
	char *plug_dir = PLUG_DIR;
	bool running = true;
	int i, len;
	profile *prof = NULL;
	
	if (comm_srv_init(argc, argv))
		return 1;

	atexit(cleanup);
	
	if (getenv("NALFS_PLUGIN_DIR"))
		plug_dir = getenv("NALFS_PLUGIN_DIR");
	parsers = plugscan(strdog(plug_dir, "syntax/"));
	writers = plugscan(strdog(plug_dir, "output/"));

	if (!parsers)
		fprintf(stderr, "No syntax plugins found.\n");
	
	while (running)
	{
		int call = comm_rdint();

		switch (call)
		{
			case REM_QUIT:
				running=false;
				break;
			
			case REM_LIST_P:
				len=0;
				if (!parsers)
					comm_wrint(1);
				while (parsers[len].info) len++;
				comm_wrint(0);
				comm_wrint(len);
				for (i=0;i<len;i++)
				{
					plug mango;
					strcpy(mango.path, plugarg(parsers[i].path));
					strcpy(mango.name, parsers[i].info->name);
					comm_wrchunk(&mango, sizeof(plug));
				}
				break;

			case REM_PARSE:
				if (!parsers)
					comm_wrint(1);
				prof = parse(comm_rdstr(), comm_rdstr(), comm_rdstr(),
					comm_rdstr());
				if (prof)
				{
					comm_wrint(0);
					profile_writer(prof);
				}
				else
				{
					fprintf(stderr, "Document was not parsed correctly.\n");
					comm_wrint(1);
				}
				break;

			case REM_OUTPUT:
				if (!writers)
				{
					fprintf(stderr, "No output plugins available.\n");
					comm_wrint(1);
				}

				if (!prof)
				{
					fprintf(stderr, "No profile was parsed, yet.\n");
					comm_wrint(1);
				}

				i=0;
				while (writers[i].path)
				{
					if (!strcmp(comm_rdstr(), plugarg(writers[i].path)))
						writers[i].info->write_prof(prof, NULL);
					i++;
				}
				break;

			default:
				printf("FID %i is not implemented.\n", call);
				break;
		}
	}
	
	return comm_srv_deinit();
}
