#include <string.h>

#include <build.h>
#include <util.h>
#include <repl.h>

void build_paralell (profile *prof, char **filter, char **p1, char **p2)
{
	int i, j, k;
	
	for (i=0;i<prof->n;i++)
		for (j=0;j<prof->ch[i].n;j++)
			for (k=0;k<prof->ch[i].pkg[j].n;k++)
			{
				// TODO: Does not seem to work properly anymore.
				int m = 0, o;
				bool no = false;
				package *pkg = &prof->ch[i].pkg[j];
				command *moo = &pkg->build[k];

				if (strcmp(moo->cmd, "make")||(moo->role!=ROLE_NONE))
					continue;

				while (filter[m])
				{
					for (o=0;o<moo->n;o++)
						if (!strcmp(moo->arg[o], filter[m]))
							no = true;
					m++;
				}
				
				if (!no)
				{
					char **new = (char **)malloc((moo->n+1)*sizeof(char *)), 
						**old, *opt;
					moo->arg = realloc(moo->arg, (++moo->n)*sizeof(char *));
					for (m=0;m<moo->n-1;m++)
						new[m+1]=moo->arg[m];
					o=0;
					opt = "-j";
					while (p1[o])
					{
						if (!strcmp(pkg->name, p1[o]))
							opt = p2[o];
						o++;
					}
					new[0] = strdog(opt, get_option("PARALELL"));
					old = moo->arg;
					moo->arg = new;
					free(old);
				}
			}
}
