#ifndef __ALFS_H__
#define __ALFS_H__

#include <stdbool.h>
#include <sys/types.h>

#include <nc_digest.h>
#include <nc_glib.h>
#include <nc_net.h>
#include <nc_util.h>
#include <nc_xml.h>

/** Command roles */
typedef enum
{
	ROLE_NONE = 1,
	NOEXECUTE,
	INTERACTIVE,
	TESTSUITE,
	INSTALL,
	SPAWN
} role;

/** Dependency types */
typedef enum
{
	DEP_NONE = 1,
	OPT,
	REQ,
	RECOM
} dtype;

/** Download information */
typedef struct
{
	hash_algo algo;
	protocol proto;
	char *url, *sum;
} download;

/** Build command */
typedef struct
{
	char *cmd;
	GList *arg;
	role role;
} command;

/** Dependency information */
typedef struct
{
	dtype type;
	char *name;
} dep;

/** Package information */
typedef struct
{
	char *name, *vers;
	GList *build;
	GList *dl;
	GList *dep;
} package;

/** Chapter information */
typedef struct
{
	char *name, *ref;
	GList *pkg;
} chapter;

/** Profile information */
typedef struct
{
	char *name, *vers;
	GList *ch;
} profile;

/** Replaceable */
typedef struct
{
	char *orig, *repl;
} replaceable;

extern bool colors;

/** Function which performs an action on a package */
typedef void (*pkg_func) (package *p);

/** Print all urls in this profile 
@param prof Profile
*/
void print_links (profile prof);
/** Print all dependencies of a package 
@param pkg Package
*/
void print_deps (package pkg);
/** Print a dependency tree for a package
@param prof Profile
@param pkg Package
*/
void print_deptree (profile prof, package pkg);
/** Print all the package information
@param pkg Package
*/
void print_pkg (package pkg);
/** Print all the packages in a chapter
@param ch Chapter
*/
void print_chapter (chapter ch);
/** Print all chapter in a profile
@param prof Profile
*/
void print_profile (profile prof);
/** Print an XML subtree
@param node Root of the subtree
*/
void print_subtree (xmlNodePtr node);

/** Sets up a filter for certain roles, all packages with that role
will not be printed when using the print_* functions
@param role Array of filtered out roles
*/
void set_filter (role *role);
/** Unsets the role filter */
void unset_filter ();
/** Determine if a role is filtered or not 
@param role The role to ask for
@return Boolean
*/
bool filtered (role role);

/** Search for a package in a profile
@param prof Profile to search in
@param name Name of the package
@param ch Optional chapter in which the package is (NULL if any)
@return Package information
*/
package *search_pkg (profile *prof, char *name, char *ch);
/** Retrieve a string representation of a role
@param role Role
@return String
*/
char *role2str (role role);
/** Retrieve a string representation of an XML element type
@param type xmlElementType
@return String
*/
char *type2str (xmlElementType type);
/** Retrieve a string representation of a hash algorithm identifier
@param algo Hash algorithm identifier
@return String 
*/
char *algo2str (hash_algo algo);
/** Retrieve a string representation of a protocol identifier
@param proto Protocl identifier
@return String
*/
char *proto2str (protocol proto);
/** Retrieve a role from a XML node
@param node XML node
@return Role
*/
role parse_role (xmlNodePtr node);
/** Retrieve a protocol identifier from a XML node
@param node XML node
@return Protocol identifier
*/
protocol parse_proto (xmlNodePtr node);
	
#endif
