/* Plugin API example
 *
 * Put your plugin info into the static t_plug structure, including your
 * parser function, which needs to take one xmlNodePtr as argument and
 * return a pointer to a profile struct (see alfs.h). You now can just
 * put your parser code into the parser function using the libalfs 
 * routines. Use the routines from libalfs/parse.h for rapid parser
 * development.
 * 
 */

#include <plugin.h>

profile *parse_example (xmlNodePtr node, replaceable *r);
void write_example (profile *prof, char *fname);

static t_plug sample_plugin =
{
	name:	"Example plugin",
	vers:	PLUG_VER,
	parse:	parse_example,
	write_prof: write_example
};

t_plug *getplug ()
{
	return &sample_plugin;
}

profile *parse_example (xmlNodePtr node, replaceable *r)
{
	return NULL;
}

void write_example (profile *prof, char *fname)
{
	printf("Moongoo!\n");
}
