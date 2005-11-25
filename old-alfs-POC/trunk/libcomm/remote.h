#ifndef __REMOTE_H__
#define __REMOTE_H__

#include <libalfs.h>

#define	REM_QUIT		0
#define REM_LIST_P		1
#define REM_PARSE		2
#define REM_OUTPUT		3
#define REM_BUILD		4
#define REM_PKG_URLS	5
#define REM_LIST_O		6

typedef struct
{
	char name[512], path[1024];
} plug;

void profile_writer (profile *p);
profile *profile_reader ();

#endif
