#include <alfsd.h>
#include <libcomm.h>
#include <plugin.h>
#include <remote.h>
#include <stdbool.h>
#include <stdio.h>

plug_info *parsers = NULL;
xmlDocPtr doc = NULL;

static void cleanup ()
{
	if (parsers)
		plugunload(parsers);
	if (doc)
		xmlFreeDoc(doc);
}

static profile *parse (char *syn, char *fname)
{
	int i=0;
	profile *prof = NULL;
	xmlNodePtr cur;
	
	xmlSubstituteEntitiesDefault(1);
	doc = xmlParseFile(fname);
	if (!doc)
		return NULL;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT|XML_PARSE_NONET);
	cur = xmlDocGetRootElement(doc);
	
	/*f = strdog(getenv("HOME"), moo_xml);
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

	add_option(r, "arch", arch);*/
	
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
	
	if (comm_srv_init(argc, argv))
		return 1;

	atexit(cleanup);
	
	if (getenv("NALFS_PLUGIN_DIR"))
		plug_dir = getenv("NALFS_PLUGIN_DIR");
	parsers = plugscan(strdog(plug_dir, "syntax/"));
	
	while (running)
	{
		int call = comm_rdint();

		switch (call)
		{
			case 0: // Quit
				running=false;
				break;
			
			case 1: // List of parser plugins
				{
					int i, len=0;
					while (parsers[len].info) len++;
					comm_wrint(len);
					for (i=0;i<len;i++)
					{
						plug mango;
						strcpy(mango.path, plugarg(parsers[i].path));
						strcpy(mango.name, parsers[i].info->name);
						comm_wrchunk(&mango, sizeof(plug));
					}
				}
				break;

			case 2: // Parse a profile
				profile_writer(parse("book", comm_rdstr()));
				comm_wrstr("WIP.");
				break;

			default:
				printf("FID %i is not implemented.\n", call);
				break;
		}
	}
	
	return comm_srv_deinit();
}
