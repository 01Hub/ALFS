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
	foreach(node->children, "command", (xml_handler_t)t_command, NULL);
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
	foreach(node->children, "userinput", (xml_handler_t)t_userinput, NULL);

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
	foreach(node->children, "sect1", (xml_handler_t)t_sect1, NULL);
}

static void t_part (xmlNodePtr node, void *data)
{
	foreach(node->children, "chapter", (xml_handler_t)t_chapter, NULL);
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
	foreach(node->children, "part", (xml_handler_t)t_part, NULL);
	return prof;
}
