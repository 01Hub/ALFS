#ifndef __ALFS_H__
#define __ALFS_H__

#include <stdbool.h>
#include <sys/types.h>

#include <libxml/xinclude.h>
#include <libxml/tree.h>

typedef enum
{
	NONE,
	NOEXECUTE,
	INTERACTIVE,
	TESTSUITE,
	INSTALL,
	CHROOT
} role;

typedef struct
{
	char *cmd, **arg;
	int n;
	role role;
} command;

typedef struct
{
	char *name, *vers;
	command *build;
	int n;
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

extern xmlDocPtr doc;

typedef void (*xml_handler_t) (xmlNodePtr node, void *data);

void print_subtree (xmlNodePtr node);
void print_pkg (package pkg);
void print_cmd (command cmd);
void print_chapter (chapter ch);
void print_profile (profile prof);

void resolve_entities (xmlNodePtr node);
void foreach (xmlNodePtr node, char *str, xml_handler_t func, void *data);
xmlNodePtr find_node (xmlNodePtr root, char *str);
char *find_value (xmlNodePtr node, char *str);
char *entity_val (char *name);

char *role2str (role role);
char *type2str (xmlElementType type);
role parse_role (xmlNodePtr node);
	
#endif
