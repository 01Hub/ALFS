#include <string.h>

#include <libalfs.h>
#include <plugin.h>

// Needs to be big enough to hold parsed cats
#define BUF_LEN		4096

profile *prof;
char commando[BUF_LEN];

profile *nalfs_profile (xmlNodePtr node, replaceable *r);
static void t_alfs (xmlNodePtr node, void *data);

static t_plug nalfs_plugin =
{
	name:	"nALFS legacy syntax",
	vers:	PLUG_VER,
	parse:	nalfs_profile,
	write_prof: NULL
};

t_plug *getplug ()
{
	return &nalfs_plugin;
}

static void parse_unpack (xmlNodePtr node)
{
	char *archive = find_value(node->children, "archive");
	node=node->children;
	// TODO: tokenize() problem
	snprintf(commando, BUF_LEN, "tar -C %s %s\necho \"%s  %s\"|md5sum -c -\n", 
		find_value(node, "destination"), archive, find_value(node, "digest"),
		archive);
}

static void parse_remove (xmlNodePtr node)
{
	snprintf(commando, BUF_LEN, "rm -rf %s\n", xmlNodeGetContent(node));
}

static void parse_make (xmlNodePtr node)
{
	node=node->children;
	snprintf(commando, BUF_LEN, "%s make %s\n", find_value(node, "prefix"), 
		find_values(node, "param"));
}

static void parse_configure (xmlNodePtr node)
{
	node = node->children;
	snprintf(commando, BUF_LEN, "%s ./configure %s\n", find_value(node, 
		"prefix"), find_values(node, "param"));
}

static void parse_copy (xmlNodePtr node)
{
	char *orig[4] = { "force", "archive", "recursive", NULL };
	char *repl[4] = { "-f", "-a", "-r", NULL };
	node = node->children;
	snprintf(commando, BUF_LEN, "cp %s %s %s\n", find_values_repl(node, "option",
		orig, repl), find_value(node, "source"), find_value(node, 
		"destination"));
}

static void __parse_env (xmlNodePtr node, void *data)
{
	snprintf(commando, BUF_LEN, "export %s=\"%s\"\n", xmlGetProp(node, "name"), 
			xmlNodeGetContent(node));
}

static void parse_environment (xmlNodePtr node, void *data)
{
	foreach(node->children, "variable", (xml_handler_t)__parse_env, NULL);
}

static void parse_base (xmlNodePtr node, void *data)
{
	snprintf(commando, BUF_LEN, "cd %s\n", xmlNodeGetContent(node));
}

static void parse_stageinfo (xmlNodePtr node)
{
	foreach(node->children, "environment", (xml_handler_t)parse_environment, 
		NULL);
	foreach(node->children, "base", (xml_handler_t)parse_base, NULL);
}

static void parse_textdump (xmlNodePtr node)
{
	node=node->children;
	snprintf(commando, BUF_LEN, "cat >%s << EOF\n%s\nEOF\n", find_value(node,
		"file"), cut_trail(find_value(node, "content"), "="));
}

static void parse_execute (xmlNodePtr node)
{
	snprintf(commando, BUF_LEN, "%s %s\n", xmlGetProp(node, "command"), 
		find_values(node->children, "param"));
}

static void parse_mkdir (xmlNodePtr node)
{
	char *orig[2] = { "parents", NULL };
	char *repl[2] = { "-p", NULL };
	node=node->children;
	snprintf(commando, BUF_LEN, "mkdir %s %s\n", find_values_repl(node, "option",
		orig, repl), find_value(node, "name"));
}

static void parse_search_replace (xmlNodePtr node)
{
	node = node->children;
	snprintf(commando, BUF_LEN, "sed -i 's%%%s%%%s%%g' %s\n", find_value(node,
		"find"), find_value(node, "replace"), find_value(node, "file"));
}

static void parse_permissions (xmlNodePtr node)
{
	char *orig[2] = { "recursive", NULL };
	char *repl[2] = { "-R", NULL };
	snprintf(commando, BUF_LEN, "chmod %s %s %s\n", find_values_repl(node->children,
		"option", orig, repl), xmlGetProp(node, "mode"), 
		find_value(node->children, "name"));
}

static void parse_ownership (xmlNodePtr node)
{
	char *orig[2] = { "recursive", NULL };
	char *repl[2] = { "-R", NULL };
	snprintf(commando, BUF_LEN, "chown %s %s:%s %s\n", 
		find_values_repl(node->children, "option", orig, repl), 
		xmlGetProp(node, "user"), xmlGetProp(node, "group"),
		find_value(node->children, "name"));
}

static void parse_patch (xmlNodePtr node)
{
	snprintf(commando, BUF_LEN, "patch %s\n", find_values(node->children, "param"));
}

static void parse_move (xmlNodePtr node)
{
	snprintf(commando, BUF_LEN, "mv %s %s\n", find_value(node->children, "source"), 
		find_value(node->children, "destination"));
}

static void parse_link (xmlNodePtr node)
{
	char *orig[2] = { "force", NULL };
	char *repl[2] = { "f", NULL };
	snprintf(commando, BUF_LEN, "ln -s%s %s %s\n", find_values_repl(node->children,
		"option", orig, repl), find_value(node->children, "target"),
		find_value(node->children, "name"));
}

static void t_stage2 (xmlNodePtr node, void *data)
{
	//printf("%s\n", xmlGetProp(node, "name"));

	node=node->children;
	while (node)
	{
		if ((node->type!=XML_TEXT_NODE)&&(node->type!=XML_COMMENT_NODE)&&
			(node->type!=XML_XINCLUDE_START)&&(node->type!=XML_XINCLUDE_END))
		{
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
				t_alfs(node, NULL);
			else
				fprintf(stderr, "The tag '%s' is not handled yet.\n", 
					node->name);

			parse_cmdblock_str(prof, commando);
		}
		node=node->next;
	}
}

static void t_pkg (xmlNodePtr node, void *data)
{
	package *pkg = pkg_append(prof);
	pkg->name = xmlGetProp(node, "name");
	pkg->vers = xmlGetProp(node, "version");

	foreach(node->children, "stage", (xml_handler_t)t_stage2, NULL);
}

static void t_alfs (xmlNodePtr node, void *data)
{
	char *sect[3] = { "package", "stage", NULL };
	foreach_multi(node->children, sect, (xml_handler_t)t_pkg, NULL);
}

static void t_stage (xmlNodePtr node, void *data)
{
	chapter *ch = chpt_append(prof);
	ch->name = xmlGetProp(node, "name");
	ch->name = ch->name;
	foreach(node->children, "alfs", (xml_handler_t)t_alfs, NULL);
}

profile *nalfs_profile (xmlNodePtr node, replaceable *r)
{
	node = find_node(node, "alfs");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid nALFS profile.\n");
		return NULL;
	}
	
	prof = prof_alloc();
	prof->name = "nALFS legacy profile";

	foreach(node->children, "stage", (xml_handler_t)t_stage, NULL);
	return prof;
}
