#ifndef __REMOTE_H__
#define __REMOTE_H__

#include <libalfs.h>

typedef struct
{
	char name[512], path[1024];
} plug;

void profile_writer (profile *p);
profile *profile_reader ();

#endif
