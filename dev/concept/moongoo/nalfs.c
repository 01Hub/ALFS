#include <string.h>

#include <nalfs.h>
#include <util.h>
#include <devel.h>

profile *prof;

void parse_unpack (xmlNodePtr node)
{
	/*printf("tar -C %s %s\n", find_value(node->children, "destination"),
		find_value(node->children, "archive"));*/
	// find_value(node->children, "digest");
}

void parse_remove (xmlNodePtr node)
{
	//printf("rm -rf %s\n", xmlNodeGetContent(node));
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
	/*char *orig[4] = { "force", "archive", "recursive", NULL };
	char *repl[4] = { "-f", "-a", "-r", NULL };
	node = node->children;
	printf("cp %s %s %s\n", find_values_repl(node, "option", orig, repl),
			find_value(node, "source"), find_value(node, "destination"));*/
}

void __parse_env (xmlNodePtr node, void *data)
{
	/*printf("export %s=\"%s\"\n", xmlGetProp(node, "name"), 
			xmlNodeGetContent(node));*/
}

void parse_environment (xmlNodePtr node, void *data)
{
	foreach(node->children, "variable", (xml_handler_t)__parse_env, NULL);
}

void parse_base (xmlNodePtr node, void *data)
{
	//printf("cd %s\n", xmlNodeGetContent(node));
}

void parse_stageinfo (xmlNodePtr node)
{
	foreach(node->children, "environment", (xml_handler_t)parse_environment, 
		NULL);
	foreach(node->children, "base", (xml_handler_t)parse_base, NULL);
}

void parse_textdump (xmlNodePtr node)
{
	/*printf("cat >%s << EOF\n%s\nEOF\n", find_value(node->children, "file"),
		cut_trail(find_value(node->children, "content"), "="));*/
}

void parse_execute (xmlNodePtr node)
{
	/*printf("%s %s\n", xmlGetProp(node, "command"), 
		find_values(node->children, "param"));*/
}

void parse_mkdir (xmlNodePtr node)
{
	/*char *orig[2] = { "parents", NULL };
	char *repl[2] = { "-p", NULL };
	printf("mkdir %s %s\n", find_values_repl(node->children, "option", orig,
		repl), find_value(node->children, "name"));*/
}

void parse_search_replace (xmlNodePtr node)
{
	/*node = node->children;
	printf("sed -i 's%%%s%%%s%%g' %s\n", find_value(node, "find"),
		find_value(node, "replace"), find_value(node, "file"));*/
}

void parse_permissions (xmlNodePtr node)
{
	/*char *orig[2] = { "recursive", NULL };
	char *repl[2] = { "-R", NULL };
	printf("chmod %s %s %s\n", find_values_repl(node->children, "option",
		orig, repl), xmlGetProp(node, "mode"), 
		find_value(node->children, "name"));*/
}

void parse_ownership (xmlNodePtr node)
{
	/*char *orig[2] = { "recursive", NULL };
	char *repl[2] = { "-R", NULL };
	printf("chown %s %s:%s %s\n", find_values_repl(node->children, "option",
		orig, repl), xmlGetProp(node, "user"), xmlGetProp(node, "group"),
		find_value(node->children, "name"));*/
}

void parse_patch (xmlNodePtr node)
{
	//printf("patch %s\n", find_values(node->children, "param"));
}

void parse_move (xmlNodePtr node)
{
	/*printf("mv %s %s\n", find_value(node->children, "source"), 
		find_value(node->children, "destination"));*/
}

void parse_link (xmlNodePtr node)
{
	/*char *orig[2] = { "force", NULL };
	char *repl[2] = { "f", NULL };
	printf("ln -s%s %s %s\n", find_values_repl(node->children, "option",
		orig, repl), find_value(node->children, "target"),
		find_value(node->children, "name"));*/
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
