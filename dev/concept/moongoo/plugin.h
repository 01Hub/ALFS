#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <libalfs.h>

#define PLUG_VER	4
#define PLUG_EXT	"so"

typedef struct
{
	char *name;
	int vers;
	profile *(* parse) (xmlNodePtr node, replaceable *r);
	void (* write_prof) (profile *prof, char *fname);
} t_plug;

typedef struct
{
	t_plug *info;
	char *path;
	void *hand;
} plug_info;

typedef struct
{
	void *hand;
	t_plug *(*getplug)(void);
} plug_hand;

plug_info *plugscan (char *dir);
plug_info *pluginfo (char *fname);
plug_hand *plugload (char *fname);
void plugerr (char *fname);
void print_plug (plug_info plug);
char *plugarg (char *path);
void plugunload (plug_info *plug);
void print_plugs (plug_info *plugs, char *def);

#endif
