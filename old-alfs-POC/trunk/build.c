#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <build.h>
#include <libalfs.h>

replaceable *r;

static int core_exec (command build)
{	
	if (!strcmp(build.cmd, "cd"))
		return chdir((char *)g_list_first(build.arg)->data);

	return -1;
}

int build_pkg (package pkg)
{
	GList *cmd = pkg.build;
	
	while (cmd)
	{
		GList *args;
		command *build = (command *)cmd->data;
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
		
		args = build->arg;
		while (args)
		{
			argv[2] = strdog2(argv[2], (char *)args->data);
			args = args->next;
		}
		
		if (execvp(argv[0], argv))
		{
			perror("shell");
			return -1;
		}

		cmd = cmd->next;
	}

	return 0;
}

static void __sed_para (gpointer data, gpointer user_data)
{
	command *moo = (command *)data;
	sed_arg *arg = (sed_arg *)user_data;
	bool no = false;
	int m = 0;
	
	if (strcmp(moo->cmd, "make")||(moo->role!=ROLE_NONE))
		return;

	while (arg->filter[m])
	{
		if (g_list_find_custom(moo->arg, arg->filter[m], g_compare_str))
			no = true;
		m++;
	}
				
	if (!no)
	{
		char *opt = "-j";
		
		m = 0;
		while (arg->p1[m])
		{
			if (!strcmp(arg->name, arg->p1[m]))
				opt = arg->p2[m];
			m++;
		}

		moo->arg = g_list_insert(moo->arg, strdog(opt, 
			get_option(r, "PARALELL")), 0);
	}
}				

static void each_pkg (gpointer data, gpointer user_data)
{
	package *p = (package *)data;
	((sed_arg *)user_data)->name = p->name;
	g_list_foreach(p->build, (GFunc)__sed_para, user_data);
}

static void each_ch (gpointer data, gpointer user_data)
{
	chapter *c = (chapter *)data;
	g_list_foreach(c->pkg, (GFunc)each_pkg, user_data);
}

void sed_paralell (profile *prof, char **filter, char **p1, char **p2)
{
	sed_arg arg;
	
	arg.filter = filter;
	arg.p1 = p1;
	arg.p2 = p2;
		
	g_list_foreach(prof->ch, (GFunc)each_ch, &arg);	
}
