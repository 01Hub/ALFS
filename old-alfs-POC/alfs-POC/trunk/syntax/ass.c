#include <libalfs.h>
#include <plugin.h>

profile *prof;

profile *ass_profile (xmlNodePtr node, replaceable *r);

static t_plug ass_plugin =
{
	name:	"ALFS simple syntax",
	vers:	PLUG_VER,
	parse:	ass_profile,
	write_prof: NULL
};

t_plug *getplug ()
{
	return &ass_plugin;
}

// TODO: Implement the new ASS syntax
static void t_shell (xmlNodePtr node, void *data)
{
	parse_cmdblock(prof, node);
}

static void t_page (xmlNodePtr node, void *data)
{
	package *pkg = pkg_append(prof);
	pkg->name = find_value(node, "title");
	pkg->vers = find_value(node, "version");

	//foreach(node->children, "download", (xml_handler_t)t_down, NULL);
	/*parse_unpck(prof, pkg->dl[0].url, node);
	parse_cmd(prof, strdog("__cd ", xmlNodeGetContent(dir)), node);*/
	foreach(node->children, "shell", (xml_handler_t)t_shell, NULL);
}

static  void t_chapter (xmlNodePtr node, void *data)
{
	chapter *ch = chpt_append(prof);
	ch->name = xmlGetProp(node, "name");
	ch->ref = xmlGetProp(node, "ref");
	
	foreach(node->children, "page", (xml_handler_t)t_page, NULL);
}

profile *ass_profile (xmlNodePtr node, replaceable *r)
{
	node = find_node(node, "ass");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid ASS profile.\n");
		return NULL;
	}
	
	prof = prof_alloc();
	prof->name = find_value(node, "title");
	prof->vers = find_value(node, "version");
	
	foreach(node->children, "chapter", (xml_handler_t)t_chapter, NULL);
	return prof;
}
