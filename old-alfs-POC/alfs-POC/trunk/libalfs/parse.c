#include <ctype.h>
#include <string.h>

#include <parse.h>

profile *prof_alloc ()
{
	profile *p = (profile *)malloc(sizeof(profile));
	p->name = NULL;
	p->vers = NULL;
	p->ch = NULL;
	return p;
}

void prof_free (profile *p)
{
	g_list_free(p->ch);
}

chapter *chpt_alloc ()
{
	chapter *c = (chapter *)malloc(sizeof(chapter));
	c->name = NULL;
	c->ref = NULL;
	c->pkg = NULL;
	return c;
}

chapter *chpt_append (profile *prof)
{
	chapter *c = chpt_alloc();
	prof->ch = g_list_append(prof->ch, c);
	return c;
}

void chpt_free (chapter *c)
{
	g_list_free(c->pkg);
}

chapter *last_chpt (profile *prof)
{
	return (chapter *)g_list_last_data(prof->ch);
}	

package *pkg_alloc ()
{
	package *p = (package *)malloc(sizeof(package));
	p->name = NULL;
	p->vers = NULL;
	p->build = NULL;
	p->dl = NULL;
	p->dep = NULL;
	return p;
}

package *pkg_append (profile *prof)
{
	package *p = pkg_alloc();
	last_chpt(prof)->pkg = g_list_append(last_chpt(prof)->pkg, p);
	return p;
}

package *pkg_alloc_title (xmlNodePtr node)
{
	char *title = find_value(node->children, "title"), 
			*tmp = strrchr(title, '-');
	package *pkg = pkg_alloc();

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

package *pkg_append_title (xmlNodePtr node, profile *prof)
{
	package *p = pkg_alloc_title(node);
	last_chpt(prof)->pkg = g_list_append(last_chpt(prof)->pkg, p);
	return p;
}

void pkg_free (package *p)
{
	g_list_free(p->build);
	g_list_free(p->dl);
	g_list_free(p->dep);
}

package *last_pkg (profile *prof)
{
	chapter *c = last_chpt(prof);
	return (package *)g_list_last_data(c->pkg);
}

command *cmd_alloc ()
{
	command *c = (command *)malloc(sizeof(command));
	c->cmd = NULL;
	c->role = ROLE_NONE;
	c->arg = NULL;
	return c;
}

command *last_cmd (profile *prof)
{
	package *p = last_pkg(prof);
	return (command *)g_list_last_data(p->build);
}

void cmd_free (command *cmd)
{
	g_list_free(cmd->arg);
}

dep *dep_alloc ()
{
	dep *d = (dep *)malloc(sizeof(dep));

	d->name = NULL;
	d->type = DEP_NONE;

	return d;
}

dep *dep_append (profile *prof)
{
	dep *d = dep_alloc();
	last_pkg(prof)->dep = g_list_append(last_pkg(prof)->dep, d);
	return d;
}

void dep_free (dep *d)
{
}

download *dl_alloc ()
{
	download *d = (download *)malloc(sizeof(download));

	d->algo = ALGO_NONE;
	d->proto = PROTO_NONE;
	d->url = NULL;
	d->sum = NULL;

	return d;
}

download *dl_append (profile *prof)
{
	download *dl = dl_alloc();
	last_pkg(prof)->dl = g_list_append(last_pkg(prof)->dl, dl);
	return dl;
}

void dl_free (download *dl)
{
}

download *last_dl (profile *prof)
{
	package *p = last_pkg(prof);
	return (download *)g_list_last_data(p->dl);
}

void parse_cmd (profile *prof, char *line, xmlNodePtr node)
{
	command *cmd = cmd_alloc(prof);
	last_pkg(prof)->build = g_list_append(last_pkg(prof)->build, cmd);

	if (node)
		cmd->role = parse_role(node);

	if (strcnt(line, " "))
	{
		cmd->cmd = strcut(line, 0, whereis(line, ' '));
		cmd->arg = tokenize(notrail(strstr(line, " "), " "), " ");
	}
	else
		cmd->cmd = line;
}

static void __parse_cmdblock (profile *prof, xmlNodePtr node, char *str)
{
	char *line = squeeze(str);
	line = strkill(line, "\\\n");
	line = strkill(line, "&&");

	if (line[0]=='\n')
		line++;

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
					fprintf(stderr, "Unterminated cat command in %s.\n",
						last_pkg(prof)->name);
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

package *first_pkg (profile *prof)
{
	chapter *ch = (chapter *)g_list_first_data(prof->ch);
	return (package *)g_list_first_data(ch->pkg);
}
