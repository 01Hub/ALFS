#ifndef __REPL_H__
#define __REPL_H__

#include <alfs.h>

// TODO: Make moo.xml location configurable
#define MOO_XML		"moo.xml"

void t_repl (xmlNodePtr node, void *data);
replaceable *init_repl (char *fname);

#endif
