#include <libalfs.h>
#include <plugin.h>

profile *prof;

profile *syn_profile (xmlNodePtr node, replaceable *r);

static t_plug syn_plugin =
{
	name:	"Hive profile syntax",
	vers:	PLUG_VER,
	parse:	syn_profile
};

t_plug *getplug ()
{
	return &syn_plugin;
}

// TODO: Complete syn support
void t_shell (xmlNodePtr node, void *data)
{
	parse_cmdblock(prof, node);
}

static void t_pkg (xmlNodePtr node, void *data)
{
	char *tmp = find_value(node, "title");
	package *pkg = next_pkg(prof);

	pkg->name = strcut(tmp, 0, whereis(tmp, ' '));
	pkg->vers = find_value(node, "version");
	foreach(node->children, "shell", (xml_handler_t)t_shell, NULL);

	if (!pkg->n)
		prof->ch[prof->n-1].n--;
}

static void t_section (xmlNodePtr node)
{	
	chapter *ch = next_chpt(prof);
	ch->name = xmlGetProp(node, "title");
	ch->ref = ch->name;
	foreach(node->children, "page", (xml_handler_t)t_pkg, NULL);
}

profile *syn_profile (xmlNodePtr node, replaceable *r)
{
	node = find_node(node, "hive");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid Hive profile.\n");
		return NULL;
	}

	prof = new_prof();
	prof->name = find_value(node, "title");
	prof->vers = find_value(node, "version");
	foreach(node->children, "section", (xml_handler_t)t_section, NULL);
	return prof;
}
