#ifndef __ALFS_H__
#define __ALFS_H__

#include <stdbool.h>
#include <sys/types.h>

#include <libxml/tree.h>
#include <libxml/xinclude.h>

#include <net.h>

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
	ALGO_NONE = 1,
	MD5,
	SHA1
} hash_algo;

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
	char *cmd, **arg;
	int n;
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
	command *build;
	int n;
	download *dl;
	int m;
	dep *dep;
	int o;
} package;

typedef struct
{
	char *name, *ref;
	package *pkg;
	int n;
} chapter;

typedef struct
{
	char *name, *vers;
	chapter *ch;
	int n;
} profile;

typedef struct
{
	char *orig, *repl;
} replaceable;

typedef void (*xml_handler_t) (xmlNodePtr node, void *data);
typedef bool (*xml_match_t) (xmlNodePtr node, void *data);

extern bool colors;

void print_subtree (xmlNodePtr node);
void print_pkg (package pkg);
void print_deps (package pkg);
void print_deptree (profile prof, package pkg);
void print_cmd (command cmd);
void print_chapter (chapter ch);
void print_profile (profile prof);
void print_links (profile prof);

void set_filter (role *role);
void unset_filter ();
bool filtered (role role);

void foreach (xmlNodePtr node, char *str, xml_handler_t func, void *data);
xmlNodePtr find_node (xmlNodePtr root, char *str);
xmlNodePtr find_node_match (xmlNodePtr node, xml_match_t func, void *data);
char *find_value (xmlNodePtr node, char *str);
char *find_values (xmlNodePtr node, char *str);
char *find_values_repl (xmlNodePtr node, char *str, char **orig, char **repl);

package *search_pkg (profile *prof, char *name, char *ch);
char *role2str (role role);
char *type2str (xmlElementType type);
char *algo2str (hash_algo algo);
char *proto2str (protocol proto);
role parse_role (xmlNodePtr node);
protocol parse_proto (xmlNodePtr node);
	
#endif
