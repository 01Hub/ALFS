#include <string.h>

#include <nalfs.h>
#include <util.h>
#include <devel.h>

profile *prof;

void parse_unpack (xmlNodePtr node)
{
	// TODO: Handle <unpack>
}

void parse_remove (xmlNodePtr node)
{
	// TODO: Handle <remove>
}

void parse_make (xmlNodePtr node)
{
	/*node = node->children;
	printf("%s make %s\n", find_value(node, "prefix"), 
		find_values(node, "param"));*/
}

void parse_configure (xmlNodePtr node)
{
	/*node = node->children;
	printf("%s ./configure %s\n", find_value(node, "prefix"),
		find_values(node, "param"));*/
}

void parse_copy (xmlNodePtr node)
{
	char *known[4] = { "source", "destination", "option", NULL };
	dbg_print(node, known);
	dbg_print2(node, "option");
}

void parse_environment (xmlNodePtr node)
{
}

void parse_stageinfo (xmlNodePtr node)
{
	node = node->children;
	while (node)
	{
		if (node->type!=XML_TEXT_NODE)
		{
			if (!strcmp(node->name, "environment"))
				parse_environment(node);
			else
			if (!strcmp(node->name, "base"))
				; //xmlNodeGetContent(node);
			else
				fprintf(stderr, "Tag '%s' not handled.\n", node->name);
		}
		node = node->next;
	}
}

void parse_textdump (xmlNodePtr node)
{
}

void parse_execute (xmlNodePtr node)
{
}

void parse_mkdir (xmlNodePtr node)
{
}

void parse_search_replace (xmlNodePtr node)
{
	/*node = node->children;
	printf("sed -i 's%%%s%%%s%%g' %s\n", find_value(node, "find"),
		find_value(node, "replace"), find_value(node, "file"));*/
}

void parse_permissions (xmlNodePtr node)
{
}

void parse_ownership (xmlNodePtr node)
{
	xmlNodePtr cur;
	char *user, *group, *opts, *names;
	
	user = xmlGetProp(node, "user");
	group = xmlGetProp(node, "group");

	opts = (char *)malloc(2);
	strcpy(opts, "");
	names = (char *)malloc(2);
	strcpy(names, "");
	
	cur = node->children;
	while (cur)
	{
		if (!strcmp(cur->name, "option"))
			if (!strcmp(xmlNodeGetContent(cur), "recursive"))
				opts = strdog2(opts, "-R");	
		
		if (!strcmp(cur->name, "name"))
			names = strdog2(names, xmlNodeGetContent(cur));

		cur = cur->next;
	}

	/*ret = (char *)malloc(512);
	snprintf(ret, 512, "chown %s %s:%s %s", opts, user, group, names);*/
}

void parse_patch (xmlNodePtr node)
{
}

void parse_move (xmlNodePtr node)
{
}

void parse_link (xmlNodePtr node)
{
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
				parse_textdump(node);
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

	//printf("%s\n", xmlGetProp(node, "name"));
	prof->ch[i].pkg[j].name = xmlGetProp(node, "name");
	// TODO: <package version="moo"> cannot be parsed yet
	prof->ch[i].pkg[j].vers = NULL;
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
