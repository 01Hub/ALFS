#include <string.h>

#include <libalfs.h>
#include <plugin.h>

profile *prof;

profile *syn_profile (xmlNodePtr node, replaceable *r);

static t_plug syn_plugin =
{
	name:	"Hive profile syntax",
	vers:	PLUG_VER,
	parse:	syn_profile,
	write_prof: NULL
};

t_plug *getplug ()
{
	return &syn_plugin;
}

static void t_shell (xmlNodePtr node, void *data)
{
	parse_cmdblock(prof, node);
}

static void t_dl (xmlNodePtr node, void *data)
{
	download *dl = (download *)data;
	
	if (!strcmp(node->name, "http"))
		dl->proto = HTTP;
	else
	if (!strcmp(node->name, "ftp"))
		dl->proto = FTP;
	else
		fprintf(stderr, "Unknown protocol '%s'.\n", node->name);

	dl->url = xmlNodeGetContent(node);

	if (is_unpackable(dl->url))
		parse_unpck(prof, dl->url, node);
}

static void t_item (xmlNodePtr node, void *data)
{
	download *dl = dl_append(prof);

	dl->algo = ALGO_SHA1;
	dl->sum = xmlGetProp(node, "sha");
	
	foreach(node->children, "ftp", (xml_handler_t)t_dl, dl);
	if (!dl->url)
		foreach(node->children, "http", (xml_handler_t)t_dl, dl);
}

static void t_download (xmlNodePtr node, void *data)
{
	foreach(node->children, "item", (xml_handler_t)t_item, NULL);
}

static void t_dir (xmlNodePtr node, void *data)
{
	char *cwd = xmlGetProp(node, "cwd");

	if ((cwd) && (!strcmp(cwd, "false")))
		return;

	parse_cmd(prof, strdog("__cd  ", xmlGetProp(node, "name")), NULL);
}

static void t_pkg (xmlNodePtr node, void *data)
{
	char *tmp = find_value(node->children, "title");
	package *pkg = pkg_append(prof);

	pkg->name = strcut(tmp, 0, whereis(tmp, ' '));
	pkg->vers = find_value(node, "version");

	foreach(node->children, "download", (xml_handler_t)t_download, NULL);
	foreach(node->children, "directory", (xml_handler_t)t_dir, NULL);
	foreach(node->children, "shell", (xml_handler_t)t_shell, NULL);

	if (!g_list_length(pkg->build))
		last_chpt(prof)->pkg = g_list_remove(last_chpt(prof)->pkg, pkg);
}

static void t_section (xmlNodePtr node)
{	
	chapter *ch = chpt_append(prof);
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

	prof = prof_alloc();
	prof->name = find_value(node, "title");
	prof->vers = find_value(node, "version");
	foreach(node->children, "section", (xml_handler_t)t_section, NULL);
	return prof;
}
