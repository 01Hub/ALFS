#ifndef __ALFS_H__
#define __ALFS_H__

#include <stdbool.h>
#include <sys/types.h>

#include <nc_digest.h>
#include <nc_glib.h>
#include <nc_net.h>
#include <nc_util.h>
#include <nc_xml.h>

typedef enum
{
	ROLE_NONE = 1,
	NOEXECUTE,
	INTERACTIVE,
	TESTSUITE,
	INSTALL,
	SPAWN
} role;

typedef enum
{
	DEP_NONE = 1,
	OPT,
	REQ,
	RECOM
} dtype;

typedef struct
{
	hash_algo algo;
	protocol proto;
	char *url, *sum;
} download;

typedef struct
{
	char *cmd;
	GList *arg;
	role role;
} command;

typedef struct
{
	dtype type;
	char *name;
} dep;

typedef struct
{
	char *name, *vers;
	GList *build;
	GList *dl;
	GList *dep;
} package;

typedef struct
{
	char *name, *ref;
	GList *pkg;
} chapter;

typedef struct
{
	char *name, *vers;
	GList *ch;
} profile;

typedef struct
{
	char *orig, *repl;
} replaceable;

extern bool colors;

typedef void (*pkg_func) (package *p);

void print_links (profile prof);
void print_deps (package pkg);
void print_deptree (profile prof, package pkg);
void print_pkg (package pkg);
void print_chapter (chapter ch);
void print_profile (profile prof);
void print_subtree (xmlNodePtr node);

void set_filter (role *role);
void unset_filter ();
bool filtered (role role);

package *search_pkg (profile *prof, char *name, char *ch);
char *role2str (role role);
char *type2str (xmlElementType type);
char *algo2str (hash_algo algo);
char *proto2str (protocol proto);
role parse_role (xmlNodePtr node);
protocol parse_proto (xmlNodePtr node);
	
#endif
