#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <libalfs.h>

#define PLUG_VER	4
#define PLUG_EXT	"so"

/** Loaded plugin structure */
typedef struct
{
	char *name;
	int vers;
	profile *(* parse) (xmlNodePtr node, replaceable *r);
	void (* write_prof) (profile *prof, char *fname);
} t_plug;

/** Plugin information structure */
typedef struct
{
	t_plug *info;
	char *path;
	void *hand;
} plug_info;

/** Plugin handle structure */
typedef struct
{
	void *hand;
	t_plug *(*getplug)(void);
} plug_hand;

/** Identify and load all plugins in a directory
@param dir Directory name
@return Array of Plugin information
@see plug_info
*/
plug_info *plugscan (char *dir);
/** Retrieve plugin information for a file
@param fname Filename
@return Plugin information
@see plug_info
*/
plug_info *pluginfo (char *fname);
/** Load a plugin
@param fname Filename
@return Plugin handle
@see plug_hand
*/
plug_hand *plugload (char *fname);
/** Print a plug_info structure
@param plug Plugin information
*/
void print_plug (plug_info plug);
/** Return the commandline argument for loading a plugin
@param path Filename of the plugin
@return Expected commandline argument
*/
char *plugarg (char *path);
/** Unload all specified plugins
@param plug Array of plugin information
*/
void plugunload (plug_info *plug);
/** Print an array of plug_info structures
@param plugs Array of plugin information
@param def Name of the plugin which should be marked as 'default'
*/
void print_plugs (plug_info *plugs, char *def);

#endif
