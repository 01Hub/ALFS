#ifndef __PARSE_H__
#define __PARSE_H__

#include <alfs.h>

/** Allocates a new profile structure
@return Profile
*/
profile *prof_alloc ();
/** Free the memory taken by a profile structure
@param p Profile
*/
void prof_free (profile *p);

/** Allocates a new chapter structure
@return Chapter
*/
chapter *chpt_alloc ();
/** Allocates and appends a new chapter structure
@param prof Profile
@return Chapter
*/
chapter *chpt_append (profile *prof);
/** Free the memory taken by a chapter structure
@param c Chapter
*/
void chpt_free (chapter *c);
/** Returns the last chapter in a profile
@param prof Profile
@return Chapter
*/
chapter *last_chpt (profile *prof);

/** Allocates a new package structure
@return Package
*/
package *pkg_alloc ();
/** Allocates and appends a new profile structure
@param prof Profile
@return Package
*/
package *pkg_append (profile *prof);
/** Allocates a new package structure and sets title information
@param node XML node with the information
@return Package */
package *pkg_alloc_title (xmlNodePtr node);
/** Allocates and appends a new package structure and sets title information
@param node XML node with the information
@param prof Profile
@return Package
*/
package *pkg_append_title (xmlNodePtr node, profile *prof);
/** Free the memory taken by a package structure
@param p Package
*/
void pkg_free (package *p);
/** Returns the last package in a profile
@param prof Profile
@return Package
*/
package *last_pkg (profile *prof);

/** Allocate a new command structure
@return Command
*/
command *cmd_alloc ();
/** Free the memory taken by a command structure
@param c Command
*/
void cmd_free (command *c);
/** Return the last command in a profile
@param prof Profile
@return Command
*/
command *last_cmd (profile *prof);

/** Allocate a new dependency structure
@return Dependency
*/
dep *dep_alloc ();
/** Allocate and append a new dependency structure
@param prof Profile
@return Dependency
*/
dep *dep_append (profile *prof);
/** Free the memory taken by a dependency structure
@param d Dependency
*/
void dep_free (dep *d);

/** Allocate a new download structure
@return Download
*/
download *dl_alloc ();
/** Allocate and append a new download structure
@param prof Profile
@return Download
*/
download *dl_append (profile *prof);
/** Free the memory taken by a download structure
@param d Download
*/
void dl_free (download *d);
/** Return the last download in a profile
@param prof Profile
@return Download
*/
download *last_dl (profile *prof);

/** Parse one command and add it to the profile
@param prof Profile
@param line Command
@param node Optional XML node with additonal information
*/
void parse_cmd (profile *prof, char *line, xmlNodePtr node);
/** Parse a command block and add it to the profile
@param prof Profile
@param node XML node with the commands
*/
void parse_cmdblock (profile *prof, xmlNodePtr node);
/** Parse a command block and add it to the profile
@param prof Profile
@param str Command block
*/
void parse_cmdblock_str (profile *prof, char *str);
/** Parse an url and create unpack/cd commands out of it
@param prof Profile
@param url URL
@param node XML node which will be passed to a parse_cmd() call
*/
void parse_unpck (profile *prof, char *url, xmlNodePtr node);

// TODO: doxygen

package *first_pkg (profile *prof);
	
#endif
