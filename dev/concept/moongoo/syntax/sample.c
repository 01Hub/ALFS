/* Moongoo plugin API example
 *
 * Put your plugin info into the static t_plug structure, including your
 * parser function, which needs to take one xmlNodePtr as argument and
 * return a pointer to a profile struct (see alfs.h). You now can just
 * put your parser code into the parser function using the libalfs 
 * routines. 
 * 
 */

#include <plugin.h>

profile *parse_example (xmlNodePtr node);

static t_plug sample_plugin =
{
	name:	"Example plugin",
	vers:	PLUG_VER,
	parse:	parse_example
};

t_plug *getplug ()
{
	return &sample_plugin;
}

profile *parse_example (xmlNodePtr node)
{
	return NULL;
}
