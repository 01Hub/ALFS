#include <string.h>

#include <repl.h>
#include <alfs.h>

void t_repl (xmlNodePtr node, void *data)
{
	xmlNodePtr text = find_node(node->children, "text");
	int i=0;
	char *repl = xmlNodeGetContent(text);
	replaceable *r = (replaceable *)data;
	
	if (!r)
		return;
	
	while (r[i].orig)
	{
		if (!strcmp(repl, r[i].orig))
		{
			xmlNodeSetContent(node, r[i].repl);
			return;
		}
		i++;
	}
	
	fprintf(stderr, "Unknown replaceable: %s\n", repl);
}

replaceable *init_repl (char *fname)
{
	replaceable *r = NULL;
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
