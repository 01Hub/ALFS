#include <string.h>
#include <stdlib.h>

#include <book.h>
#include <repl.h>
#include <util.h>

command *cmd;
profile *prof;
int num;

void t_sect1 (xmlNodePtr node, void *data);
void process_cmd (char *line, xmlNodePtr node);
void __t_userinput (xmlNodePtr node, void *data);
command *t_userinput (xmlNodePtr node, int *n);

// TODO: Parse 'cat > foo' commands
file *parse_cat (char *str)
{
	//file *f = (file *)malloc(sizeof(file));
	//printf("%s\n\n", str);
	return NULL;
}

void t_sect1 (xmlNodePtr node, void *data)
{
	int i;
	char *title, *tmp;
		
	title = find_value(node->children, "title");

	if (!title)
	{
		fprintf(stderr, "%s: No title found.\n", node->name);
		return;
	}

	prof->pkg = realloc(prof->pkg, (++prof->n)*sizeof(package));
	i = prof->n-1;
	
	tmp = strdog(lower_case(title), "version");
	prof->pkg[i].vers = entity_val(tmp);
	prof->pkg[i].name = strcut(title, 0, strlen(title)-
		((prof->pkg[i].vers) ? 1 : 0));
	prof->pkg[i].build = t_userinput(node->children, 
		&prof->pkg[i].n);
	free(tmp);

	if (!prof->pkg[i].n)
		prof->n--;
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

	foreach(node->children, "replaceable", (xml_handler_t)t_repl, NULL);
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
				parse_cat(strcut(line, 0, t-line+3));
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

profile *bookasprofile (xmlNodePtr node)
{
	init_repl();
	prof = (profile *)malloc(sizeof(profile));
	prof->pkg = NULL;
	prof->n = 0;
	foreach(node, "sect1", (xml_handler_t)t_sect1, NULL);
	return prof;
}
