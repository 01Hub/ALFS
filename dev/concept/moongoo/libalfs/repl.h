#ifndef __REPL_H__
#define __REPL_H__

#include <alfs.h>

void t_repl (xmlNodePtr node, void *data);
replaceable *init_repl (char *fname);
char *get_option (replaceable *r, char *key);

#endif
