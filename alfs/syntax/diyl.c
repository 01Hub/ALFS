#include <plugin.h>

profile *prof;

profile *parse_diyl (xmlNodePtr node, replaceable *r);

static t_plug diyl_plugin =
{
	name:	"DIY Linux",
	vers:	PLUG_VER,
	parse:	parse_diyl,
	write_prof: NULL
};

t_plug *getplug ()
{
	return &diyl_plugin;
}

static void t_userinput (xmlNodePtr node, void *data)
{
	parse_cmdblock(prof, node);
}

static void t_sect2 (xmlNodePtr node, void *data)
{
	package *pkg = next_pkg_title(prof, node);
	foreach(node->children, "userinput", (xml_handler_t)t_userinput, NULL);

	if (!pkg->n)
		prof->ch[prof->n-1].n--;
}

static void t_sect1 (xmlNodePtr node, void *data)
{
	chapter *ch = next_chpt(prof);
	ch->name = find_value(node->children, "title");
	ch->ref = xmlGetProp(node, "id");
	foreach(node->children, "sect2", (xml_handler_t)t_sect2, NULL);
}

profile *parse_diyl (xmlNodePtr node, replaceable *r)
{
	xmlNodePtr info = find_node(node, "articleinfo");
	
	if (!info)
	{
		fprintf(stderr, "XML document is not a valid DIY Linux refbuild.\n");
		return NULL;
	}

	prof = new_prof();
	prof->name = find_value(info, "title");
	prof->vers = find_value(info, "pubdate");

	foreach(node->children, "sect1", (xml_handler_t)t_sect1, NULL);
	return prof;
}
