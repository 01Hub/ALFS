#include <ctype.h>
#include <string.h>

#include <plugin.h>

profile *prof;

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

static void process_cmd (char *line)
{
	int i, j, k;

	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].build = realloc(prof->ch[i].pkg[j].build,
			(++prof->ch[i].pkg[j].n)*sizeof(command));
	k = prof->ch[i].pkg[j].n-1;

	prof->ch[i].pkg[j].build[k].role = ROLE_NONE;

	
	if (strcnt(line, " "))
	{
		prof->ch[i].pkg[j].build[k].cmd = strcut(line, 0, whereis(line, ' '));
		prof->ch[i].pkg[j].build[k].arg = tokenize(notrail(strstr(line, " "), 
				" "), " ", &prof->ch[i].pkg[j].build[k].n);
	}
	else
	{
		prof->ch[i].pkg[j].build[k].cmd = line;
		prof->ch[i].pkg[j].build[k].arg = NULL;
		prof->ch[i].pkg[j].build[k].n = 0;
	}
}

static void t_command (xmlNodePtr node, void *data)
{
	char *line = squeeze(xmlNodeGetContent(node));
	line = strkill(line, "\\\n");

	if (strcnt(line, "\n"))
	{
		char *tmp;

		while ((line) && (strlen(line)))
		{
			tmp = strsep(&line, "\n");
			process_cmd(tmp);
		}
	}
	else
		process_cmd(line);
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
	int i, j, k;

	if ((role)&&(!strcmp(role, "no")))
		return;
	
	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].dep = realloc(prof->ch[i].pkg[j].dep,
			(++prof->ch[i].pkg[j].o)*sizeof(dep));
	k = prof->ch[i].pkg[j].o-1;

	prof->ch[i].pkg[j].dep[k].name = xmlGetProp(node, "linkend");
	prof->ch[i].pkg[j].dep[k].type = *type;
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
	
	if ((title)&&(!strncmp(lower_case(title), " dependencies", 13)))
		foreach(node->children, "sect4", (xml_handler_t)t_sect4, NULL);
}

static void t_sect1 (xmlNodePtr node, void *data)
{
	int i, j;
	char *title, *tmp;

	title = find_value(node->children, "title");

	if (!title)
	{
		fprintf(stderr, "%s: No title found.\n", node->name);
		return;
	}

	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg, 
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;

	tmp = strrchr(title, '-');
	if (tmp)
	{
		char *t = strnrchr(title, '-', 2);
		if ((t) && (isdigit(t[1])))
			tmp = strdog(t, tmp); 
	}
	tmp = chrep(tmp, ' ', '\0');
	prof->ch[i].pkg[j].vers = tmp ? strcut(tmp, 1, strlen(tmp)) : NULL;
	prof->ch[i].pkg[j].name = tmp ? strcut(title, 0, 
		strlen(title)-strlen(tmp)) : title;
	prof->ch[i].pkg[j].build = NULL;
	prof->ch[i].pkg[j].n = 0;
	prof->ch[i].pkg[j].dl = NULL;
	prof->ch[i].pkg[j].m = 0;
	prof->ch[i].pkg[j].dep = NULL;
	prof->ch[i].pkg[j].o = 0;
	
	foreach(node->children, "sect3", (xml_handler_t)t_sect3, data);
	foreach(node->children, "userinput", (xml_handler_t)t_userinput, data);

	if (!prof->ch[i].pkg[j].n)
		prof->ch[i].n--;
}

static void t_chapter (xmlNodePtr node, void *data)
{
	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	prof->ch[prof->n-1].name = find_value(node->children, "title");
	prof->ch[prof->n-1].ref = xmlGetProp(node, "id");
	prof->ch[prof->n-1].pkg = NULL;
	prof->ch[prof->n-1].n = 0;
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

	prof = (profile *)malloc(sizeof(profile));
	prof->name = find_value(info->children, "title");
	prof->vers = find_value(info->children, "subtitle");
	prof->vers = strstr(prof->vers, " ");
	prof->vers++;
	prof->ch = NULL;
	prof->n = 0;
	foreach(node->children, "part", (xml_handler_t)t_part, r);
	return prof;
}
