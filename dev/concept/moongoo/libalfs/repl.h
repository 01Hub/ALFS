#ifndef __REPL_H__
#define __REPL_H__

#include <alfs.h>

#define MOO_XML		"/.nALFS/answers.xml"

void t_repl (xmlNodePtr node, void *data);
replaceable *init_repl (char *fname);
char *get_option (char *key);

#endif
