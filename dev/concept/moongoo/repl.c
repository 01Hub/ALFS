#include <string.h>

#include <repl.h>
#include <alfs.h>

replaceable *r = NULL;

void t_repl (xmlNodePtr node, void *data)
{
	xmlNodePtr text = find_node(node->children, "text");
	char *repl = xmlNodeGetContent(text);
	char *opt = get_option(repl);
	
	if (!strcmp(opt, ""))
	{
		fprintf(stderr, "Unconfigured replaceable: %s\n", repl);
		return;
	}
	
	xmlNodeSetContent(node, opt);
}

char *get_option (char *key)
{
	int i=0;
	
	if (!r)
		return NULL;
	
	while (r[i].orig)
	{
		if (!strcmp(key, r[i].orig))
			return r[i].repl;
		i++;
	}

	return "";
}

replaceable *init_repl (char *fname)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	int count=0;

	doc=xmlParseFile(fname);
	if (!doc)
		return NULL;
	node=xmlDocGetRootElement(doc)->children;

	while (node)
	{
		if (!strcmp(node->name, "answer"))
		{
			r = realloc(r, (++count)*sizeof(replaceable));
			r[count-1].orig = xmlGetProp(node, "name");
			r[count-1].repl = xmlNodeGetContent(node);
		}
		node=node->next;
	}

	r = realloc(r, (++count)*sizeof(replaceable));
	r[count-1].orig = NULL;
	r[count-1].repl = NULL;
	return r;
}	
