#include <string.h>

#include <nalfs.h>
#include <util.h>
#include <devel.h>

profile *prof;
char commando[512];

void parse_unpack (xmlNodePtr node)
{
	node=node->children;
	snprintf(commando, 512, "tar -C %s %s\n", find_value(node, 
		"destination"), find_value(node, "archive"));
	
	// TODO: Handle <digest>
	// find_value(node, "digest");
}

void parse_remove (xmlNodePtr node)
{
	snprintf(commando, 512, "rm -rf %s\n", xmlNodeGetContent(node));
}

void parse_make (xmlNodePtr node)
{
	node=node->children;
	snprintf(commando, 512, "%s make %s\n", find_value(node, "prefix"), 
		find_values(node, "param"));
}

void parse_configure (xmlNodePtr node)
{
	node = node->children;
	snprintf(commando, 512, "%s ./configure %s\n", find_value(node, 
		"prefix"), find_values(node, "param"));
}

void parse_copy (xmlNodePtr node)
{
	char *orig[4] = { "force", "archive", "recursive", NULL };
	char *repl[4] = { "-f", "-a", "-r", NULL };
	node = node->children;
	snprintf(commando, 512, "cp %s %s %s\n", find_values_repl(node, "option",
		orig, repl), find_value(node, "source"), find_value(node, 
		"destination"));
}

void __parse_env (xmlNodePtr node, void *data)
{
	snprintf(commando, 512, "export %s=\"%s\"\n", xmlGetProp(node, "name"), 
			xmlNodeGetContent(node));
}

void parse_environment (xmlNodePtr node, void *data)
{
	foreach(node->children, "variable", (xml_handler_t)__parse_env, NULL);
}

void parse_base (xmlNodePtr node, void *data)
{
	snprintf(commando, 512, "cd %s\n", xmlNodeGetContent(node));
}

void parse_stageinfo (xmlNodePtr node)
{
	foreach(node->children, "environment", (xml_handler_t)parse_environment, 
		NULL);
	foreach(node->children, "base", (xml_handler_t)parse_base, NULL);
}

void parse_textdump (xmlNodePtr node)
{
	node=node->children;
	snprintf(commando, 512, "cat >%s << EOF\n%s\nEOF\n", find_value(node,
		"file"), cut_trail(find_value(node, "content"), "="));
}

void parse_execute (xmlNodePtr node)
{
	snprintf(commando, 512, "%s %s\n", xmlGetProp(node, "command"), 
		find_values(node->children, "param"));
}

void parse_mkdir (xmlNodePtr node)
{
	char *orig[2] = { "parents", NULL };
	char *repl[2] = { "-p", NULL };
	node=node->children;
	snprintf(commando, 512, "mkdir %s %s\n", find_values_repl(node, "option",
		orig, repl), find_value(node, "name"));
}

void parse_search_replace (xmlNodePtr node)
{
	node = node->children;
	snprintf(commando, 512, "sed -i 's%%%s%%%s%%g' %s\n", find_value(node,
		"find"), find_value(node, "replace"), find_value(node, "file"));
}

void parse_permissions (xmlNodePtr node)
{
	char *orig[2] = { "recursive", NULL };
	char *repl[2] = { "-R", NULL };
	snprintf(commando, 512, "chmod %s %s %s\n", find_values_repl(node->children,
		"option", orig, repl), xmlGetProp(node, "mode"), 
		find_value(node->children, "name"));
}

void parse_ownership (xmlNodePtr node)
{
	char *orig[2] = { "recursive", NULL };
	char *repl[2] = { "-R", NULL };
	snprintf(commando, 512, "chown %s %s:%s %s\n", 
		find_values_repl(node->children, "option", orig, repl), 
		xmlGetProp(node, "user"), xmlGetProp(node, "group"),
		find_value(node->children, "name"));
}

void parse_patch (xmlNodePtr node)
{
	snprintf(commando, 512, "patch %s\n", find_values(node->children, "param"));
}

void parse_move (xmlNodePtr node)
{
	snprintf(commando, 512, "mv %s %s\n", find_value(node->children, "source"), 
		find_value(node->children, "destination"));
}

void parse_link (xmlNodePtr node)
{
	char *orig[2] = { "force", NULL };
	char *repl[2] = { "f", NULL };
	snprintf(commando, 512, "ln -s%s %s %s\n", find_values_repl(node->children,
		"option", orig, repl), find_value(node->children, "target"),
		find_value(node->children, "name"));
}

void process_cmd4 (char *cmd)
{
	int i, j, k;
	
	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].build = realloc(prof->ch[i].pkg[j].build, 
		(++prof->ch[i].pkg[j].n)*sizeof(command));
	k = prof->ch[i].pkg[j].n-1;

	prof->ch[i].pkg[j].build[k].role = ROLE_NONE;
	
	if (strcnt(cmd, " "))
	{
		prof->ch[i].pkg[j].build[k].cmd = strcut(cmd, 0, whereis(cmd, ' '));
		prof->ch[i].pkg[j].build[k].arg = tokenize(notrail(strstr(cmd, " "), 
			" "),  " ", &prof->ch[i].pkg[j].build[k].n);
	}
	else
	{
		prof->ch[i].pkg[j].build[k].cmd = cmd;
		prof->ch[i].pkg[j].build[k].arg = NULL;
		prof->ch[i].pkg[j].build[k].n = 0;
	}
}

void t_stage2 (xmlNodePtr node, void *data)
{
	//printf("%s\n", xmlGetProp(node, "name"));

	node=node->children;
	while (node)
	{
		if ((node->type!=XML_TEXT_NODE)&&(node->type!=XML_COMMENT_NODE)&&
			(node->type!=XML_XINCLUDE_START)&&(node->type!=XML_XINCLUDE_END))
		{
			char *temp;
			
			strcpy(commando, "");
			
			if (!strcmp(node->name, "unpack"))
				parse_unpack(node);
			else
			if (!strcmp(node->name, "remove"))
				parse_remove(node);
			else
			if (!strcmp(node->name, "make"))
				parse_make(node);
			else
			if (!strcmp(node->name, "configure"))
				parse_configure(node);
			else
			if (!strcmp(node->name, "copy"))
				parse_copy(node);
			else
			if (!strcmp(node->name, "stageinfo"))
				parse_stageinfo(node);
			else
			if (!strcmp(node->name, "textdump"))
				parse_textdump(node);
			else
			if (!strcmp(node->name, "execute"))
				parse_execute(node);
			else
			if (!strcmp(node->name, "mkdir"))
				parse_mkdir(node);
			else
			if (!strcmp(node->name, "search_replace"))
				parse_search_replace(node);
			else
			if (!strcmp(node->name, "permissions"))
				parse_permissions(node);
			else
			if (!strcmp(node->name, "ownership"))
				parse_ownership(node);
			else
			if (!strcmp(node->name, "patch"))
				parse_patch(node);
			else
			if (!strcmp(node->name, "move"))
				parse_move(node);
			else
			if (!strcmp(node->name, "link"))
				parse_link(node);
			else
			if (!strcmp(node->name, "alfs"))
			// TODO: Do recursive parsing
				;
			else
				fprintf(stderr, "The tag '%s' is not handled yet.\n", 
					node->name);

			strcpy(commando, squeeze(commando));
			strcpy(commando, strkill(commando, "\\\n"));

			temp = (char *)malloc(strlen(commando)+1);
			strcpy(temp, commando);
			
			if (strcnt(temp, "\n"))
			{
				char *tmp;
		
				while ((temp) && (strlen(temp)))
				{
					if (!strncmp(temp, "cat >", 5))
					{
						char *t;
						fprintf(stderr, "%s\n", temp);
						t = strnstr(temp, "EOF", 2);
						process_cmd4(strcut(temp, 0, t-temp+3));
						t+=2;
						strcpy(temp, t);
						continue;
					}
			
					tmp = strsep(&temp, "\n");
					process_cmd4(tmp);
				}
			}
			else
				process_cmd4(temp);
			
		}
		node=node->next;
	}
}

void t_pkg2 (xmlNodePtr node, void *data)
{
	int i, j;

	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg,
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;

	prof->ch[i].pkg[j].name = xmlGetProp(node, "name");
	prof->ch[i].pkg[j].vers = xmlGetProp(node, "version");
	prof->ch[i].pkg[j].build = NULL;
	prof->ch[i].pkg[j].n = 0;
	
	foreach(node->children, "stage", (xml_handler_t)t_stage2, NULL);
}

void t_alfs (xmlNodePtr node, void *data)
{
	// TODO: Handle <stage> node only pages, like chapter07/network.xml 
	foreach(node->children, "package", (xml_handler_t)t_pkg2, NULL);
}

void t_stage (xmlNodePtr node, void *data)
{
	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	prof->ch[prof->n-1].name = xmlGetProp(node, "name");
	prof->ch[prof->n-1].name = prof->ch[prof->n-1].name;
	prof->ch[prof->n-1].pkg = NULL;
	prof->ch[prof->n-1].n = 0;
	foreach(node->children, "alfs", (xml_handler_t)t_alfs, NULL);
}

profile *nalfs_profile (xmlNodePtr node)
{
	node = find_node(node, "alfs");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid nALFS profile.\n");
		return NULL;
	}
	
	prof = (profile *)malloc(sizeof(profile));
	prof->name = "nALFS legacy profile";
	prof->vers = entity_val("LFS-version");
	prof->ch = NULL;
	prof->n = 0;
	foreach(node->children, "stage", (xml_handler_t)t_stage, NULL);
	return prof;
}
