#include <string.h>

#include <util.h>
#include <syn.h>

profile *prof;

void process_cmd2 (char *line, xmlNodePtr node)
{
	int i, j, k;

	i = prof->n-1;
	j = prof->ch[i].n-1;

	prof->ch[i].pkg[j].build = realloc(prof->ch[i].pkg[j].build,
			(++prof->ch[i].pkg[j].n)*sizeof(command));
	k = prof->ch[i].pkg[j].n-1;

	prof->ch[i].pkg[j].build[k].role = NONE;

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

void t_shell (xmlNodePtr node, void *data)
{
	char *line = squeeze(xmlNodeGetContent(node));
	line = strkill(line, "\\\n");
	
	if (strcnt(line, "\n"))
	{
		char *tmp;

		while ((line) && (strlen(line)))
		{
			tmp = strsep(&line, "\n");
			process_cmd2(tmp, node);
		}
	}
	else
		process_cmd2(line, node);
}

void t_pkg (xmlNodePtr node, void *data)
{
	int i, j;
	char *tmp;

	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg, 
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;

	tmp = find_value(node, "title");
	prof->ch[i].pkg[j].name = strcut(tmp, 0, whereis(tmp, ' '));
	prof->ch[i].pkg[j].vers = find_value(node, "version");
	prof->ch[i].pkg[j].build = NULL;
	prof->ch[i].pkg[j].n = 0;
	foreach(node->children, "shell", (xml_handler_t)t_shell, NULL);

	if (!prof->ch[i].pkg[j].n)
		prof->ch[i].n--;
}

void t_section (xmlNodePtr node)
{	
	int i = prof->n-1;
	prof->ch[i].name = xmlGetProp(node, "title");
	prof->ch[i].ref = prof->ch[i].name;
	prof->ch[i].pkg = NULL;
	prof->ch[i].n = 0;
	foreach(node->children, "package", (xml_handler_t)t_pkg, NULL);
}

profile *syn_profile (xmlNodePtr node)
{
	node = find_node(node, "hive");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid Hive profile.\n");
		return NULL;
	}

	prof = (profile *)malloc(sizeof(profile));
	prof->name = find_value(node, "title");
	prof->vers = find_value(node, "version");
	prof->ch = NULL;
	prof->n = 0;

	node = node->children;
	while (node)
	{
		if (!strcmp(node->name, "section"))
		{
			printf("%s\n", xmlGetProp(node, "title"));
			prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
			t_section(node);
		}
		node = node->next;
	}

	return prof;
}
