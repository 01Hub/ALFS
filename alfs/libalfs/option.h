#ifndef __OPT_H__
#define __OPT_H__

#include <stdbool.h>

#include <nc_xml.h>

typedef bool (*xml_opt_t) (xmlNodePtr node, void *data);

void remove_nodes (xmlNodePtr root, xml_opt_t func, void *data);

#endif 
