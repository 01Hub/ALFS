#include <string.h>

#include <alfs.h>
#include <util.h>
#include <plugin.h>

profile *prof;

profile *ass_profile (xmlNodePtr node);

static t_plug ass_plugin =
{
	name:	"ALFS simple syntax",
	vers:	1,
	parse:	ass_profile
};

t_plug *getplug ()
{
	return &ass_plugin;
}

// TODO: Support download/directory in the ASS parser
void process_cmd3 (char *line, xmlNodePtr node)
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

void t_shell2 (xmlNodePtr node, void *data)
{
	char *line = squeeze(xmlNodeGetContent(node));
	line = strkill(line, "\\\n");

	if (strcnt(line, "\n"))
	{
		char *tmp;

		while ((line) && (strlen(line)))
		{
			tmp = strsep(&line, "\n");
			process_cmd3(tmp, node);
		}
	}
	else
		process_cmd3(line, node);
}

void t_page (xmlNodePtr node, void *data)
{
	int i, j;

	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg,
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;

	prof->ch[i].pkg[j].name = find_value(node, "title");
	prof->ch[i].pkg[j].vers = find_value(node, "version");
	prof->ch[i].pkg[j].build = NULL;
	prof->ch[i].pkg[j].n = 0;
	foreach(node->children, "shell", (xml_handler_t)t_shell2, NULL);
}

void t_chapter2 (xmlNodePtr node, void *data)
{
	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	prof->ch[prof->n-1].name = xmlGetProp(node, "name");
	prof->ch[prof->n-1].ref = xmlGetProp(node, "ref");
	prof->ch[prof->n-1].pkg = NULL;
	prof->ch[prof->n-1].n = 0;
	foreach(node->children, "page", (xml_handler_t)t_page, NULL);
}

profile *ass_profile (xmlNodePtr node)
{
	node = find_node(node, "ass");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid ASS profile.\n");
		return NULL;
	}
	
	prof = (profile *)malloc(sizeof(profile));
	prof->name = find_value(node, "title");
	prof->vers = find_value(node, "version");
	prof->ch = NULL;
	prof->n = 0;
	foreach(node->children, "chapter", (xml_handler_t)t_chapter2, NULL);
	return prof;
}
