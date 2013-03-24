#include <option.h>

void remove_nodes (xmlNodePtr node, xml_opt_t func, void *data)
{
	while (node)
	{
		if (func(node, data))
		{
			if ((node->next) && (node->next->type!=XML_TEXT_NODE))
			{
				node->prev->next=node->next;
				xmlFreeNode(node);
			}
			else
			{
				if (!node->prev)
					node->parent->children = NULL;
				else
					node->prev->next=node->next ? node->next->next : NULL;
				
				/*xmlFreeNode(node->next);
				xmlFreeNode(node);*/
			}
		}
		
		remove_nodes(node->children, func, data);
		node=node->next;
	}
}
