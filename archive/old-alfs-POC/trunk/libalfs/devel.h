#ifndef __DEVEL_H__
#define __DEVEL_H__

#include <nc_xml.h>

/** Prints all subnodes of a node, apart from those with certain names
@param node Root XML node
@param known NULL terminated array of names
*/
void dbg_print (xmlNodePtr node, char **known);
/** Prints all subnodes of a node with a certain name
@param node Root XML node
@param name Node name
*/
void dbg_print2 (xmlNodePtr node, char *name);

#endif
