#include <ctype.h>
#include <string.h>

#include <gen.h>
#include <parse.h>
#include <util.h>

chapter *next_chpt (profile *prof)
{
	chapter *ch;

	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	ch = &prof->ch[prof->n-1];
	
	ch->name = NULL;
	ch->ref = NULL;
	ch->pkg = NULL;
	ch->n = 0;
	
	return ch;
}

command *next_cmd (profile *prof)
{
	command *cmd;
	int i, j, k;

	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].build = realloc(prof->ch[i].pkg[j].build,
			(++prof->ch[i].pkg[j].n)*sizeof(command));
	k = prof->ch[i].pkg[j].n-1;

	cmd = &prof->ch[i].pkg[j].build[k];
	cmd->cmd = NULL;
	cmd->arg = NULL;
	cmd->n = 0;
	cmd->role = ROLE_NONE;

	return cmd;
}

dep *next_dep (profile *prof)
{
	dep *dep;
	int i, j, k;
	
	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].dep = realloc(prof->ch[i].pkg[j].dep,
			(++prof->ch[i].pkg[j].o)*sizeof(dep));
	k = prof->ch[i].pkg[j].o-1;

	dep = &prof->ch[i].pkg[j].dep[k];
	dep->type = DEP_NONE;
	dep->name = NULL;

	return dep;
}

download *next_dl (profile *prof)
{
	download *dl;
	int i, j, k;
	
	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].dl = realloc(prof->ch[i].pkg[j].dl,
		(++prof->ch[i].pkg[j].m)*sizeof(download));
	k = prof->ch[i].pkg[j].m-1;
	
	dl = &prof->ch[i].pkg[j].dl[k];
	dl->algo = ALGO_NONE;
	dl->sum = NULL;
	dl->proto = PROTO_NONE;
	dl->url = NULL;
	
	return dl;
}

package *next_pkg (profile *prof)
{
	package *pkg;
	int i, j;
	
	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg, 
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;
	
	pkg = &prof->ch[i].pkg[j];
	pkg->name = NULL;
	pkg->vers = NULL;
	pkg->build = NULL;
	pkg->n = 0;
	pkg->dl = NULL;
	pkg->m = 0;
	pkg->dep = NULL;
	pkg->o = 0;

	return pkg;
}

package *next_pkg_title (profile *prof, xmlNodePtr node)
{
	char *title = find_value(node->children, "title"), 
		*tmp = strrchr(title, '-');
	package *pkg = next_pkg(prof);

	if (tmp)
	{
		char *t = strnrchr(title, '-', 2);
		if ((t) && (isdigit(t[1])))
			tmp = strdog(t, tmp); 
	}
	tmp = chrep(tmp, ' ', '\0');
	
	pkg->name = tmp ? strcut(title, 0, strlen(title)-strlen(tmp)) : title;
	pkg->vers = tmp ? strcut(tmp, 1, strlen(tmp)) : NULL;

	return pkg;
}

profile *new_prof ()
{
	profile *prof = (profile *)malloc(sizeof(profile));
	
	prof->name = NULL;
	prof->vers = NULL;
	prof->ch = NULL;
	prof->n = 0;

	return prof;
}

void parse_cmd (profile *prof, char *line, xmlNodePtr node)
{
	command *cmd = next_cmd(prof);
	if (node)
		cmd->role = parse_role(node);

	if (strcnt(line, " "))
	{
		cmd->cmd = strcut(line, 0, whereis(line, ' '));
		cmd->arg = tokenize(notrail(strstr(line, " "), " "), " ", &cmd->n);
	}
	else
		cmd->cmd = line;
}

static void __parse_cmdblock (profile *prof, xmlNodePtr node, char *str)
{
	char *line = squeeze(str);
	line = strkill(line, "\\\n");
	line = strkill(line, "&&");

	if (strcnt(line, "\n"))
	{
		char *tmp;

		while ((line) && (strlen(line)))
		{
			if (!strncmp(line, "cat >", 5))
			{
				char *t = strnstr(line, "EOF", 2);
				
				if (!t)
				{
					fprintf(stderr, "Unterminated cat command.\n");
					parse_cmd(prof, line, node);
				}
				else
				{
					parse_cmd(prof, strcut(line, 0, t-line+3), node);
					t+=2;
				}
				
				line=t;
				continue;
			}

			tmp = strsep(&line, "\n");
			parse_cmd(prof, tmp, node);
		}
	}
	else
		parse_cmd(prof, line, node);
}

void parse_cmdblock (profile *prof, xmlNodePtr node)
{
	__parse_cmdblock(prof, node, xmlNodeGetContent(node));
}

void parse_cmdblock_str (profile *prof, char *str)
{
	__parse_cmdblock(prof, NULL, str);
}

void parse_unpck (profile *prof, char *url, xmlNodePtr node)
{
	int i;
	char *ext = extonly(url), *ret = NULL;

	for (i=0;i<NUM_COMPR;i++)
	{
		char *tst = strdog(".tar", (char *)compr[i]);
		if (!strcmp(ext, tst))
			ret = (char *)unpck[i];
		free(tst);
	}
	
	if (!ret)
	{
		// TODO: Add support for missing archive types
		//fprintf(stderr, "Archive extension '%s' is unknown.\n", ext);
		return;
	}

	ret = strdog(ret, basename(url));
	parse_cmd(prof, ret, node);
	free(ret);
}
