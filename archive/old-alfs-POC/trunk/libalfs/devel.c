#include <stdbool.h>
#include <string.h>

#include <devel.h>

void dbg_print (xmlNodePtr node, char **known)
{
	node = node->children;
	while (node)
	{
		if ((node->type!=XML_TEXT_NODE) && (node->type!=XML_COMMENT_NODE) &&
			(node->type!=XML_XINCLUDE_START) && 
			(node->type!=XML_XINCLUDE_END))
		{
			bool k = false;
			if (known)
			{
				int i=0;
				while (known[i])
				{
					if (!strcmp(node->name, known[i]))
						k = true;
					i++;
				}
			}
			
			if (!k)
				printf("%s\n", node->name);
		}
		node=node->next;
	}
}

void dbg_print2 (xmlNodePtr node, char *name)
{
	node = node->children;
	while (node)
	{
		if (!strcmp(node->name, name))
			printf("%s\n", xmlNodeGetContent(node));
		node = node->next;
	}
}
