#ifndef __PARSE_H__
#define __PARSE_H__

#include <alfs.h>

profile *prof_alloc ();
void prof_free (profile *p);

chapter *chpt_alloc ();
chapter *chpt_append (profile *prof);
void chpt_free (chapter *c);
chapter *last_chpt (profile *prof);

package *pkg_alloc ();
package *pkg_append (profile *prof);
package *pkg_alloc_title (xmlNodePtr node);
package *pkg_append_title (xmlNodePtr node, profile *prof);
void pkg_free (package *p);
package *last_pkg (profile *prof);

command *cmd_alloc ();
void cmd_free (command *c);
command *last_cmd (profile *prof);

dep *dep_alloc ();
dep *dep_append (profile *prof);
void dep_free (dep *d);

download *dl_alloc ();
download *dl_append (profile *prof);
void dl_free (download *d);
download *last_dl (profile *prof);

void parse_cmd (profile *prof, char *line, xmlNodePtr node);
void parse_cmdblock (profile *prof, xmlNodePtr node);
void parse_cmdblock_str (profile *prof, char *str);
void parse_unpck (profile *prof, char *url, xmlNodePtr node);
	
#endif
