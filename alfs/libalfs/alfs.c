#include <string.h>
#include <unistd.h>

#include <alfs.h>

static void print_str (gpointer data, gpointer user_data);
static void print_urls (package *pkg);

bool colors = true;
role *filter = NULL;

static void each_pkg (gpointer data, gpointer user_data)
{
	package *p = (package *)data;
	pkg_func func = (pkg_func)user_data;
	func(p);
}

static void each_chpt (gpointer data, gpointer user_data)
{
	chapter *c = (chapter *)data;
	g_list_foreach(c->pkg, (GFunc)each_pkg, user_data);
}

static void print_cmd (gpointer data, gpointer user_data)
{
	command *cmd = (command *)data;

	if (filtered(cmd->role))
		return;
	
	printf("%s ", cmd->cmd);
	if (cmd->arg)
		g_list_foreach(cmd->arg, (GFunc)print_str, " ");	
	if (cmd->role!=ROLE_NONE)
		printf("(%s)", role2str(cmd->role));
	printf("\n");
}

static void print_ch (gpointer data, gpointer user_data)
{
	chapter *c = (chapter *)data;
	print_chapter(*c);
}

static void print_dl (gpointer data, gpointer user_data)
{
	download *dl = (download *)data;
	printf("%s: %s --- ", algo2str(dl->algo), dl->sum);
	printf("%s://%s\n", proto2str(dl->proto), dl->url);
}

static void print_pkg_gfunc (gpointer data, gpointer user_data)
{
	package *pkg = (package *)data;
	print_pkg(*pkg);
}

static void print_str (gpointer data, gpointer user_data)
{
	char *str = (char *)data;
	char *str2 = (char *)user_data;
	printf("%s%s", str, str2);
}

void print_links (profile prof)
{
	g_list_foreach(prof.ch, (GFunc)each_chpt, print_urls);
}

static void print_urls (package *pkg)
{
	g_list_foreach(pkg->dl, (GFunc)print_dl, NULL);
}

static void __print_deps (profile *prof, package pkg, int indent)
{
	GList *list = pkg.dep;

	printfi(indent, "%s\n", pkg.name);
	
	if (!list)
		return;

	while (list)
	{
		dep *d = (dep *)list->data;	
		
		if (!d->name)
			continue;

		if (d->type==OPT)
			continue;
		
		if (prof)
		{
			int j;
			package *p = search_pkg(prof, lower_case(d->name), NULL);
				
			for (j=0;j<indent;j++)
				printf("\t");
				
			if (!p)
				fprintf(stderr, "Dependency %s does not exist.\n", 
					d->name);
			else
				__print_deps(NULL, *p, indent+1);
				//print_deptree(*prof, *p);
		}
		else
			printfi(indent+1, "%s\n", d->name);

		list = list->next;
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
	if (colors)
		term_set(RESET, GREEN, BLACK);
	if (!pkg.vers)
		printf("%s\n\n", pkg.name);
	else
		printf("%s\nVersion: %s\n\n", pkg.name, pkg.vers);
	if (colors)
		term_reset();

	g_list_foreach(pkg.build, (GFunc)print_cmd, NULL);
	printf("\n");
}		

void print_chapter (chapter ch)
{
	if (colors)
		term_set(RESET, RED, BLACK);
	printf("%s\n", ch.name);
	if (colors)
		term_reset();
	g_list_foreach(ch.pkg, (GFunc)print_pkg_gfunc, NULL);
}

void print_profile (profile prof)
{
	if (colors)
		term_set(RESET, BLUE, BLACK);
	printf("%s %s\n", prof.name, ((prof.vers) ? (prof.vers) : ""));
	if (colors)
		term_reset();
	g_list_foreach(prof.ch, (GFunc)print_ch, NULL);
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

// TODO: Inifite loop when the package does not exist
package *search_pkg (profile *prof, char *name, char *ch)
{
	GList *c = prof->ch;

	while (c)
	{
		GList *p;
		chapter *chp = (chapter *)c->data;
		
		if ((ch) && (strcmp(lower_case(ch), lower_case(chp->ref))))
			continue;
		
		p = chp->pkg;
		while (p)
		{
			package *pkg = (package *)p->data;
			
			if (!strcmp(lower_case(name), 
				lower_case(pkg->name)))
				return pkg;

			p=p->next;
		}

		c=c->next;
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
