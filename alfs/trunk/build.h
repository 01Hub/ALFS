#ifndef __BUILD_H__
#define __BUILD_H__

#include <alfs.h>

typedef struct 
{
	char *name, **filter, **p1, **p2;
} sed_arg;

extern replaceable *r;

int build_pkg (package pkg);
void sed_paralell (profile *prof, char **filter, char **p1, char **p2);

#endif
