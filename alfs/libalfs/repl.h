#ifndef __REPL_H__
#define __REPL_H__

#include <alfs.h>

void t_repl (xmlNodePtr node, void *data);
replaceable *init_repl (char *fname);
char *get_option (replaceable *r, char *key);
void print_config (replaceable *r);
void add_option (replaceable *r, char *key, char *value);

#endif
