#include <string.h>
#include <stdlib.h>

#include <book.h>
#include <repl.h>
#include <util.h>

command *cmd;
int num;
profile *prof;
replaceable *__r;

void t_sect1 (xmlNodePtr node, void *data);
void process_cmd (char *line, xmlNodePtr node);
void __t_userinput (xmlNodePtr node, void *data);
command *t_userinput (xmlNodePtr node, int *n);
void t_chapter (xmlNodePtr node);

void t_sect1 (xmlNodePtr node, void *data)
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
	
	tmp = strdog(lower_case(title), "version");
	prof->ch[i].pkg[j].vers = entity_val(tmp);
	prof->ch[i].pkg[j].name = strcut(title, 0, strlen(title)-
		((prof->ch[i].pkg[j].vers) ? 1 : 0));
	prof->ch[i].pkg[j].build = t_userinput(node->children, 
		&prof->ch[i].pkg[j].n);
	free(tmp);

	if (!prof->ch[i].pkg[j].n)
		prof->ch[i].n--;
}

void process_cmd (char *line, xmlNodePtr node)
{
	cmd = realloc(cmd, (++num)*sizeof(command));
	cmd[num-1].role = parse_role(node);
	
	if (strcnt(line, " "))
	{
		cmd[num-1].cmd = strcut(line, 0, whereis(line, ' '));
		cmd[num-1].arg = tokenize(notrail(strstr(line, " "), " "), 
				" ", &cmd[num-1].n);
	}
	else
	{
		cmd[num-1].cmd = line;
		cmd[num-1].arg = NULL;
		cmd[num-1].n = 0;
	}
}

void __t_userinput (xmlNodePtr node, void *data)
{
	char *line;

	foreach(node->children, "replaceable", (xml_handler_t)t_repl, __r);
	line = squeeze(xmlNodeGetContent(node));
	line = strkill(line, "\\\n");

	if (strcnt(line, "\n"))
	{
		char *tmp;
		
		while ((line) && (strlen(line)))
		{
			if (!strncmp(line, "cat >", 5))
			{
				char *t = strnstr(line, "EOF", 2);
				process_cmd(strcut(line, 0, t-line+3), node);
				t+=2;
				line=t;
				continue;
			}
			
			tmp = strsep(&line, "\n");
			process_cmd(tmp, node);
		}
	}
	else
		process_cmd(line, node);
}

command *t_userinput (xmlNodePtr node, int *n)
{
	cmd = NULL;
	num = 0;
	foreach(node, "userinput", (xml_handler_t)__t_userinput, NULL);
	*n = num;
	return cmd;
}

void t_chapter (xmlNodePtr node)
{
	int i = prof->n-1;
	prof->ch[i].name = xmlGetProp(node, "xreflabel");
	prof->ch[i].ref = xmlGetProp(node, "id");
	prof->ch[i].pkg = NULL;
	prof->ch[i].n = 0;
	foreach(node->children, "sect1", (xml_handler_t)t_sect1, NULL);
}

profile *bookasprofile (xmlNodePtr node)
{
	xmlNodePtr info = find_node(node, "bookinfo");
	__r = init_repl(MOO_XML);
	
	prof = (profile *)malloc(sizeof(profile));
	prof->name = find_value(info, "title");
	prof->vers = find_value(info, "subtitle");
	prof->vers = strcut(prof->vers, strlen("Version"), strlen(prof->vers));
	prof->ch = NULL;
	prof->n = 0;
	
	node = node->children;
	while (node)
	{
		if (!strcmp(node->name, "part"))
		{
			xmlNodePtr cur = node->children;
			while (cur)
			{
				if (!strcmp(cur->name, "chapter"))
				{
					prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
					t_chapter(cur);
				}
				cur=cur->next;
			}
		}
		node=node->next;
	}
	
	return prof;
}
