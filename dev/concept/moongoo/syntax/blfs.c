#include <ctype.h>
#include <string.h>

#include <plugin.h>

profile *prof;

// TODO: Removed a <command> block because of parsing problems (postlfs/config/bootdisk.xml)
profile *parse_blfs (xmlNodePtr node, replaceable *r);

static t_plug sample_plugin =
{
	name:	"BLFS book as profile",
	vers:	PLUG_VER,
	parse:	parse_blfs
};

t_plug *getplug ()
{
	return &sample_plugin;
}

static void t_command (xmlNodePtr node, void *data)
{
	parse_cmdblock(prof, node);
}

static void t_userinput (xmlNodePtr node, void *data)
{
	// TODO: Add all possible replaceables to moo.xml
	/*replaceable *r = (replaceable *)data;
	foreach(node->children, "replaceable", (xml_handler_t)t_repl, r);*/
	foreach(node->children, "command", (xml_handler_t)t_command, NULL);
}

static void t_xref (xmlNodePtr node, void *data)
{
	dtype *type = (dtype *)data;
	char *role = xmlGetProp(node, "role");
	dep *dep;

	if ((role)&&(!strcmp(role, "no")))
		return;

	dep = next_dep(prof);
	dep->name = xmlGetProp(node, "linkend");
	dep->type = *type;
}

static void t_sect4 (xmlNodePtr node, void *data)
{
	char *title = find_value(node->children, "title");
	dtype type;

	if (!strncmp(title, "Optional", 8))
		type = OPT;
	else
	if (!strcmp(title, "Required"))
		type = REQ;
	else
	if (!strcmp(title, "Recommended"))
		type = RECOM;
	else
	{
		fprintf(stderr, "Unknown dependency type '%s'\n", title);
		type = DEP_NONE;
	}
	
	foreach(node->children, "xref", (xml_handler_t)t_xref, &type);
}

static void t_sect3 (xmlNodePtr node, void *data)
{
	char *title = strstr(find_value(node->children, "title"), " ");
	
	// TODO: Dependency parsing is broken
	return;
	
	if ((title)&&(!strncmp(lower_case(title), " dependencies", 13)))
		foreach(node->children, "sect4", (xml_handler_t)t_sect4, NULL);
}

static void t_add (xmlNodePtr node, void *data)
{
	download *moo = next_dl(prof);

	moo->url = find_attr(node->children, "ulink", "url");
	moo->proto = check_proto(moo->url);
}

static void t_url (xmlNodePtr node, void *data)
{
	int n, i;
	bool isdl = false;
	char **moo = tokenize(lower_case(xmlNodeGetContent(node)), " ", &n);
	protocol proto; 
	
	for (i=0;i<n;i++)
	{
		if (!strcmp(moo[i], "download"))
			isdl = true;
		
		if ((!strcmp(moo[i], "size:"))||(!strcmp(moo[i], "md5"))||
		   (!strcmp(moo[i], "size"))||(!strcmp(moo[i], "md5sum"))||
		   (!strcmp(moo[i], "mirrors"))||(!strcmp(moo[i], "md5sum:")))
			isdl = false;
		
		if (moo[i][0]=='(')
		{
			char *type = strcut(moo[i], 1, strlen(moo[i])-3);
			proto = check_proto(type);
		}
	}
	
	if (isdl)
	{
		download *moo = next_dl(prof);

		moo->proto = proto;
		moo->url = find_attr(node->children, "ulink", "url");
	}
}

static void t_info (xmlNodePtr node, void *data)
{
	char *title = lower_case(find_value(node->children, "title"));

	if (!title)
		return;
	
	if (!strcmp(title, "package information"))
		foreach(node->children, "para", (xml_handler_t)t_url, NULL);

	if (!strcmp(title, "additional downloads"))
		foreach(node->children, "para", (xml_handler_t)t_add, NULL);
}

static void t_sect1 (xmlNodePtr node, void *data)
{
	package *pkg = next_pkg_title(prof, node);

	foreach(node->children, "sect3", (xml_handler_t)t_info, NULL);
	foreach(node->children, "sect3", (xml_handler_t)t_sect3, data);

	if (pkg->m>0)
	{
		char *url = pkg->dl[0].url, *tball = basename(url);
		int i;
		
		parse_unpck(prof, url, node);
		for (i=0;i<NUM_COMPR;i++)
		{
			char *dir, *com;
			com = strdog(".tar", (char *)compr[i]);
			dir = strkill(tball, com);
			if (strcmp(dir, tball))
				parse_cmd(prof, strdog("__cd ", dir), node);
			free(com);
			free(dir);
		}
	}
	
	foreach(node->children, "userinput", (xml_handler_t)t_userinput, data);
	
	if (!pkg->n)
		prof->ch[prof->n-1].n--;
}

static void t_chapter (xmlNodePtr node, void *data)
{
	chapter *ch = next_chpt(prof);
	ch->name = find_value(node->children, "title");
	ch->ref = xmlGetProp(node, "id");
	foreach(node->children, "sect1", (xml_handler_t)t_sect1, data);
}

static void t_part (xmlNodePtr node, void *data)
{
	foreach(node->children, "chapter", (xml_handler_t)t_chapter, data);
}

profile *parse_blfs (xmlNodePtr node, replaceable *r)
{
	xmlNodePtr info = find_node(node->children, "bookinfo");

	if (strcmp(node->name, "book"))
	{
		fprintf(stderr, "XML document is not a valid BLFS profile.\n");
		return NULL;
	}

	prof = new_prof();
	prof->name = find_value(info->children, "title");
	prof->vers = find_value(info->children, "subtitle");
	prof->vers = strstr(prof->vers, " ");
	prof->vers++;
	foreach(node->children, "part", (xml_handler_t)t_part, r);
	return prof;
}
