#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <libalfs.h>
#include <plugin.h>

command *cmd;
int num;
profile *prof;
replaceable *r;

profile *bookasprofile (xmlNodePtr node, replaceable *r);

static t_plug book_plugin =
{
	name:	"LFS Relax-NG book as profile",
	vers:	PLUG_VER,
	parse:	bookasprofile
};

t_plug *getplug ()
{
	return &book_plugin;
}


static void process_cmd (char *line, xmlNodePtr node)
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

static void __t_userinput (xmlNodePtr node, void *data)
{
	char *line;

	foreach(node->children, "replaceable", (xml_handler_t)t_repl, r);
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

static command *t_userinput (xmlNodePtr node, int *n)
{
	cmd = NULL;
	num = 0;
	foreach(node, "userinput", (xml_handler_t)__t_userinput, NULL);
	*n = num;
	return cmd;
}

static void t_section (xmlNodePtr node, void *data)
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
	prof->ch[i].pkg[j].dl = NULL;
	prof->ch[i].pkg[j].m = 0;
	prof->ch[i].pkg[j].dep = NULL;
	prof->ch[i].pkg[j].o = 0;
	
	prof->ch[i].pkg[j].build = t_userinput(node->children, 
		&prof->ch[i].pkg[j].n);

	if (!prof->ch[i].pkg[j].n)
		prof->ch[i].n--;
}

static void t_chapter (xmlNodePtr node, void *data)
{
	int i;

	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	i = prof->n-1;
	prof->ch[i].name = xmlGetProp(node, "xreflabel");
	prof->ch[i].ref = xmlGetProp(node, "id");
	prof->ch[i].pkg = NULL;
	prof->ch[i].n = 0;
	foreach(node->children, "section", (xml_handler_t)t_section, NULL);
}

void t_part (xmlNodePtr node, void *data)
{
	foreach(node->children, "chapter", (xml_handler_t)t_chapter, NULL);
}

profile *bookasprofile (xmlNodePtr node, replaceable *_r)
{
	xmlNodePtr info = find_node(node, "info");

	if (!info)
	{
		fprintf(stderr, "XML document is not a valid LFS RNG book.\n");
		return NULL;
	}

	r = _r;
	prof = (profile *)malloc(sizeof(profile));
	prof->name = find_value(info, "title");
	prof->vers = find_value(info, "subtitle");
	prof->vers = strcut(prof->vers, strlen("Version"), strlen(prof->vers));
	prof->ch = NULL;
	prof->n = 0;
	
	foreach(node->children, "part", (xml_handler_t)t_part, NULL);
	return prof;
}
