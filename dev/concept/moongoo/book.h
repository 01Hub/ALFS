#ifndef __BOOK_H__
#define __BOOK_H__

#include <alfs.h>

void t_sect1 (xmlNodePtr node, void *data);
command *t_userinput (xmlNodePtr node, int *n);
profile *bookasprofile (xmlNodePtr node);
file *parse_cat (char *str);

#endif
