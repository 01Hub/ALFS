#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <build.h>
#include <libalfs.h>

replaceable *r;

static int core_exec (command build)
{	
	if (!strcmp(build.cmd, "cd"))
		return chdir(build.arg[0]);

	return -1;
}

int build_pkg (package pkg)
{
	int i;
	
	for (i=0;i<pkg.n;i++)
	{
		command *build = &pkg.build[i];
		int j;
		char *argv[3];

		if (!strncmp(build->cmd, "__", 2))
		{
			build->cmd+=2;
			if (core_exec(*build))
			{
				fprintf(stderr, "core_exec(): %s: %s\n", build->cmd, 
					strerror(errno));
				return -1;
			}
			continue;
		}
		
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = strcut(build->cmd, 0, strlen(build->cmd));
		for (j=0;j<build->n;j++)
			argv[2] = strdog2(argv[2], build->arg[j]);
		
		if (execvp(argv[0], argv))
		{
			perror("shell");
			return -1;
		}
	}

	return 0;
}

void sed_paralell (profile *prof, char **filter, char **p1, char **p2)
{
	int i, j, k;
	
	for (i=0;i<prof->n;i++)
		for (j=0;j<prof->ch[i].n;j++)
			for (k=0;k<prof->ch[i].pkg[j].n;k++)
			{
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
					new[0] = strdog(opt, get_option(r, "PARALELL"));
					old = moo->arg;
					moo->arg = new;
					free(old);
				}
			}
}
