#include <string.h>
#include <unistd.h>

#include <alfs.h>

bool colors = true;
role *filter = NULL;

void print_links (profile prof)
{
	int i, j;

	for (i=0;i<prof.n;i++)
		for (j=0;j<prof.ch[i].n;j++)
			print_urls(prof.ch[i].pkg[j]);
}

void print_urls (package pkg)
{
	int k;
	
	for (k=0;k<pkg.m;k++)
	{
		download moo = pkg.dl[k];
		printf("%s: %s --- ", algo2str(moo.algo), moo.sum);
		printf("%s://%s\n", proto2str(moo.proto), moo.url);
	}
}
		
void print_cmd (command cmd)
{
	int i=0;

	if (filtered(cmd.role))
		return;
	
	printf("%s ", cmd.cmd);
	for (i=0;i<cmd.n;i++)
		printf("%s ", cmd.arg[i]);
	if (cmd.role!=ROLE_NONE)
		printf("(%s)", role2str(cmd.role));
	printf("\n");
}

static void __print_deps (profile *prof, package pkg, int indent)
{
	int i;

	printfi(indent, "%s\n", pkg.name);
	
	if (!pkg.dep)
		return;

	for (i=0;i<pkg.o;i++)
	{
		if (!pkg.dep[i].name)
			continue;

		if (pkg.dep[i].type==OPT)
			continue;
		
		if (prof)
		{
			int j;
			package *p = search_pkg(prof, lower_case(pkg.dep[i].name), NULL);
				
			for (j=0;j<indent;j++)
				printf("\t");
				
			if (!p)
				fprintf(stderr, "Dependency %s does not exist.\n", 
					pkg.dep[i].name);
			else
				__print_deps(NULL, *p, indent+1);
				//print_deptree(*prof, *p);
		}
		else
			printfi(indent+1, "%s\n", pkg.dep[i].name);
	}
}

void print_deps (package pkg)
{
	__print_deps(NULL, pkg, 0);
}

void print_deptree (profile prof, package pkg)
{
	__print_deps(&prof, pkg, 0);
}

void print_pkg (package pkg)
{
	int i;
	
	if (colors)
		term_set(RESET, GREEN, BLACK);
	if (!pkg.vers)
		printf("%s\n\n", pkg.name);
	else
		printf("%s\nVersion: %s\n\n", pkg.name, pkg.vers);
	if (colors)
		term_reset();
	
	for (i=0;i<pkg.n;i++)
		print_cmd(pkg.build[i]);
	printf("\n");
}		

void print_chapter (chapter ch)
{
	int i;

	if (!ch.n)
		return;

	if (colors)
		term_set(RESET, RED, BLACK);
	printf("%s\n", ch.name);
	if (colors)
		term_reset();
	for (i=0;i<ch.n;i++)
		print_pkg(ch.pkg[i]);
}

void print_profile (profile prof)
{
	int i;

	if (colors)
		term_set(RESET, BLUE, BLACK);
	printf("%s %s\n", prof.name, ((prof.vers) ? (prof.vers) : ""));
	if (colors)
		term_reset();
	for (i=0;i<prof.n;i++)
		print_chapter(prof.ch[i]);
}

void print_subtree (xmlNodePtr node)
{
	while (node)
	{
		printf("%s(%s): %s\n", node->name, type2str(node->type), 
			xmlNodeGetContent(node));
		print_subtree(node->children);
		printf("\n");
		node=node->next;
	}
}

char *role2str (role role)
{
	switch (role)
	{
		case (ROLE_NONE):
			return "none";
		case (NOEXECUTE):
			return "noexecute";
		case (INTERACTIVE):
			return "interactive";
		case (TESTSUITE):
			return "testsuite";
		case (INSTALL):
			return "install";
		case (SPAWN):
			return "spawn";
		default:
			return "unknown";
	}
}

role parse_role (xmlNodePtr node)
{
	char *prop = xmlGetProp(node, "role");

	if (!prop)
		return ROLE_NONE;
	if (!strcmp(prop, "noexecute"))
		return NOEXECUTE;
	if (!strcmp(prop, "interactive"))
		return INTERACTIVE;
	if (!strcmp(prop, "testsuite"))
		return TESTSUITE;
	if (!strcmp(prop, "install"))
		return INSTALL;
	if (!strcmp(prop, "spawn"))
		return SPAWN;

	fprintf(stderr, "%s is an unknown role-attribute.\n", prop);
	return ROLE_NONE;
}

protocol parse_proto (xmlNodePtr node)
{
	char *prop = xmlGetProp(node, "type");

	if (!prop)
		return PROTO_NONE;
	if (!strcmp(prop, "ftp"))
		return FTP;
	if (!strcmp(prop, "http"))
		return HTTP;

	fprintf(stderr, "%s is an unknown protocol.\n", prop);
	return PROTO_NONE;
}

char *type2str (xmlElementType type)
{
	switch (type)
	{
    	case (XML_ELEMENT_NODE):
			return "element";
		case (XML_ATTRIBUTE_NODE):
			return "attribute";
    	case (XML_TEXT_NODE):
			return "text";
		case (XML_CDATA_SECTION_NODE):
			return "cdata section";
    	case (XML_ENTITY_REF_NODE):
			return "entity ref";
    	case (XML_ENTITY_NODE):
			return "entity";
    	case (XML_PI_NODE):
			return "pi";
    	case (XML_COMMENT_NODE):
			return "comment";
    	case (XML_DOCUMENT_NODE):
			return "document";
    	case (XML_DOCUMENT_TYPE_NODE):
			return "document type";
   		case (XML_DOCUMENT_FRAG_NODE):
			return "document frag";
    	case (XML_NOTATION_NODE):
			return "notation";
    	case (XML_HTML_DOCUMENT_NODE):
			return "HTML document";
    	case (XML_DTD_NODE):
			return "DTD";
    	case (XML_ELEMENT_DECL):
			return "element declaration";
    	case (XML_ATTRIBUTE_DECL):
			return "attribute declaration";
    	case (XML_ENTITY_DECL):
			return "entity declaration";
    	case (XML_NAMESPACE_DECL):
			return "namespace declaration";
    	case (XML_XINCLUDE_START):
			return "xinclude start";
    	case (XML_XINCLUDE_END):
			return "xinclude end";
   		case (XML_DOCB_DOCUMENT_NODE):
			return "docbook";
		default:
			return "unknown";
	}
}

void set_filter (role *role)
{
	filter = role;
}

void unset_filter ()
{
	filter = NULL;
}

bool filtered (role role)
{
	int i=0;

	if (!filter)
		return false;

	while (filter[i])
	{
		if (role==filter[i])
			return true;
		i++;
	}
	return false;
}

package *search_pkg (profile *prof, char *name, char *ch)
{
	int i, j;

	for (i=0;i<prof->n;i++)
	{
		if ((ch) && (strcmp(lower_case(ch), lower_case(prof->ch[i].ref))))
			continue;
		
		for (j=0;j<prof->ch[i].n;j++)
			if (!strcmp(lower_case(name), 
				lower_case(prof->ch[i].pkg[j].name)))
				return &prof->ch[i].pkg[j];
	}

	return NULL;
}

char *algo2str (hash_algo algo)
{
	switch (algo)
	{
		case (ALGO_NONE):
			return "none";
		case (ALGO_MD5):
			return "md5";
		case (ALGO_SHA1):
			return "sha1";
		default:
			return "unknown";
	}
}

char *proto2str (protocol proto)
{
	switch (proto)
	{
		case (PROTO_NONE):
			return "none";
		case (HTTP):
			return "http";
		case (FTP):
			return "ftp";
		default:
			return "unknown";
	}
}
