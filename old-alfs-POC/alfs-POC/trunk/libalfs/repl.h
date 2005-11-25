#ifndef __REPL_H__
#define __REPL_H__

#include <alfs.h>

/** Replaces contents of node according to an array of replaceables
Pass this function to a foreach() in your plugins.
@param node XML node (should be a \<repl\> node)
@param data An array of replaceables
@see replaceable
*/
void t_repl (xmlNodePtr node, void *data);
/** Initializes an array of replaceables according to a configuration file
@param fname Path of the configuration file
@return An array of replaceables
@see replaceable
*/
replaceable *init_repl (char *fname);
/** Retrieve an option
@param r An array of replaceables
@param key Key of the data to retrieve
@return The replaceable string
@see replaceable
*/
char *get_option (replaceable *r, char *key);
/** Dump an array of replaceables to screen
@param r An array of replaceables
*/
void print_config (replaceable *r);
/** Add an option
@param r Array of replaceables to which you want to add
@param key Key of the new configuration option
@param value Value of the new configuration option
*/
void add_option (replaceable *r, char *key, char *value);

#endif
