#ifndef __OPT_H__
#define __OPT_H__

#include <stdbool.h>

#include <nc_xml.h>

/** Function that checks if a certain XML node matches user-defined criteria */ 
typedef bool (*xml_opt_t) (xmlNodePtr node, void *data);

/** Removes all nodes from an XML subtree which match 
@param root Root of the XML subtree
@param func User-defined matching function
@param data Optional data passed to the matching function
*/
void remove_nodes (xmlNodePtr root, xml_opt_t func, void *data);

#endif 
