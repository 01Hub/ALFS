#ifndef __PARSE_H__
#define __PARSE_H__

#include <alfs.h>

dep *cur_dep (profile *prof);
package *cur_pkg (profile *prof);

chapter *next_chpt (profile *prof);
command *next_cmd (profile *prof);
dep *next_dep (profile *prof);
download *next_dl (profile *prof);
package *next_pkg (profile *prof);
package *next_pkg_title (profile *prof, xmlNodePtr node);
profile *new_prof ();

void parse_cmd (profile *prof, char *line, xmlNodePtr node);
void parse_cmdblock (profile *prof, xmlNodePtr node);
void parse_cmdblock_str (profile *prof, char *str);
void parse_unpck (profile *prof, char *url, xmlNodePtr node);
	
#endif
