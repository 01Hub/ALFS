#ifndef __REPL_H__
#define __REPL_H__

#include <alfs.h>

#define REPLC	16

extern replaceable r[REPLC];

void t_repl (xmlNodePtr node, void *data);
void init_repl ();

#endif
