#include <string.h>
#include <unistd.h>

#include <alfs.h>
#include <util.h>

xmlDocPtr doc;
role *filter = NULL;

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

void print_pkg (package pkg)
{
	int i;
	
	term_set(RESET, GREEN, BLACK);
	if (!pkg.vers)
		printf("%s\n\n", pkg.name);
	else
		printf("%s-%s\n\n", pkg.name, pkg.vers);
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

	term_set(RESET, RED, BLACK);
	printf("%s\n", ch.name);
	term_reset();
	for (i=0;i<ch.n;i++)
		print_pkg(ch.pkg[i]);
}

void print_profile (profile prof)
{
	int i;

	term_set(RESET, BLUE, BLACK);
	printf("%s %s\n", prof.name, prof.vers);
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

void foreach (xmlNodePtr node, char *str, xml_handler_t func, void *data)
{
	if (!str)
		return;
	
	while (node)
	{
		if (!strcmp(node->name, str))
			func(node, data);
		foreach(node->children, str, func, data);
		node=node->next;
	}
}

xmlNodePtr find_node (xmlNodePtr root, char *str)
{
	if (!str)
		return NULL;

	while (root)
	{
		xmlNodePtr tmp = NULL;;
		
		if (!strcmp(root->name, str))
			return root;
		tmp=find_node(root->children, str);
		if (tmp)
			return tmp;
		root=root->next;
	}
	
	return NULL;
}

char *find_values (xmlNodePtr node, char *str)
{
	char *ret;

	if ((!node)||(!str))
		return "";

	ret = (char *)malloc(1);
	strcpy(ret, "");

	while (node)
	{
		if (!strcmp(node->name, str))
			ret = strdog2(ret, xmlNodeGetContent(node));
		node = node->next;
	}

	return ret;
}			

char *find_values_repl (xmlNodePtr node, char *str, char **orig, char **repl)
{
	char *ret;

	if ((!node)||(!str))
		return "";

	if ((!orig)||(!repl))
		return find_values(node, str);

	ret = (char *)malloc(1);
	strcpy(ret, "");

	while (node)
	{
		if (!strcmp(node->name, str))
		{
			int i=0;
			bool rep = false;
			char *tmp = xmlNodeGetContent(node);
			
			while ((orig[i])&&(repl[i]))
			{
				if (!strcmp(tmp, orig[i]))
				{
					tmp = repl[i];
					rep = true;
					break;
				}
				i++;
			}

			if (!rep)
				fprintf(stderr, "No replacement for '%s'.\n", tmp);

			ret = strdog2(ret, tmp);
		}
		node = node->next;
	}

	return ret;
}

char *find_value (xmlNodePtr node, char *str)
{
	xmlNodePtr moo = find_node(node, str);
	if (!moo)
		return "";
	return xmlNodeGetContent(moo);
}

char *entity_val (char *name)
{
	xmlEntityPtr ent = xmlGetDocEntity(doc, name);
	char *ret;
	
	if (!ent)
		return NULL;
	ret=(char *)malloc(ent->length+1);
	strncpy(ret, ent->content, ent->length);
	ret[ent->length]='\0';
	return ret;
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
		case (CHROOT):
			return "chroot";
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
	if (!strcmp(prop, "chroot"))
		return CHROOT;

	fprintf(stderr, "%s is an unknown role-attribute.\n", prop);
	
	return ROLE_NONE;
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
