#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <libalfs.h>
#include <plugin.h>

profile *prof;
replaceable *r;
bool is_rng = false;

profile *bookasprofile (xmlNodePtr node, replaceable *r);

static t_plug book_plugin =
{
	name:	"LFS book as profile",
	vers:	PLUG_VER,
	parse:	bookasprofile,
	write_prof: NULL
};

t_plug *getplug ()
{
	return &book_plugin;
}

static void t_userinput (xmlNodePtr node, void *data)
{
	// TODO: <replaceable> problems w/ the RNG book
	foreach(node->children, "replaceable", (xml_handler_t)t_repl, r);
	parse_cmdblock(prof, node); 
}

static void t_sect1 (xmlNodePtr node, void *data)
{
	package *pkg = pkg_append_title(node, prof);

	foreach(node->children, "userinput", (xml_handler_t)t_userinput, NULL);

	if (!g_list_length(pkg->build))
		last_chpt(prof)->pkg = g_list_remove(last_chpt(prof)->pkg, pkg);
}

static void t_chapter (xmlNodePtr node, void *data)
{
	chapter *ch = chpt_append(prof);
	ch->name = xmlGetProp(node, "xreflabel");
	ch->ref = xmlGetProp(node, "id");

	foreach(node->children, is_rng ? "section" : "sect1", 
		(xml_handler_t)t_sect1, NULL);
}

static void t_part (xmlNodePtr node, void *data)
{
	foreach(node->children, "chapter", (xml_handler_t)t_chapter, NULL);
}

static bool handle_arch (xmlNodePtr node, void *data)
{
	char *arch = get_option(r, "arch");
	char *attr = xmlGetProp(node, "arch");

	//printf("%s\n", arch);
	
	/* TODO: Check this w/o having a default arch set
	if (!arch)
	{
		fprintf(stderr, "You're using a multi-arch book w/o having arch set.\n");
		return false;
	}*/
	
	if (!attr)
		return false;
	if (!strcmp(arch, attr))
		return false;

	return true;
}

profile *bookasprofile (xmlNodePtr node, replaceable *_r)
{
	xmlNodePtr info = find_node(node, "bookinfo");
	if (!info)
	{
		info = find_node(node, "info");
		is_rng = true;
	}

	if (!info)
	{
		fprintf(stderr, "XML document is not a valid LFS book.\n");
		return NULL;
	}

	remove_nodes(node, (xml_opt_t)handle_arch, _r);

	r = _r;
	prof = prof_alloc();
	prof->name = find_value(info, "title");
	prof->vers = find_value(info, "subtitle");
	prof->vers = strcut(prof->vers, strlen("Version"), strlen(prof->vers));
	
	foreach(node->children, "part", (xml_handler_t)t_part, NULL);
	return prof;
}
