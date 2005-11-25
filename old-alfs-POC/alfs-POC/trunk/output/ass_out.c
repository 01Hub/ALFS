#include <plugin.h>

void write_ass (profile *prof, char *fname);

static t_plug ass_plugin =
{
	name:	"ALFS simple syntax writer",
	vers:	PLUG_VER,
	parse:	NULL,
	write_prof: write_ass
};

t_plug *getplug ()
{
	return &ass_plugin;
}

static void each_pkg (gpointer data, gpointer user_data)
{
	package *p = (package *)data;
	FILE *ass = (FILE *)user_data;
		
	fprintf(ass, "\t\t<page>\n\t\t\t<title>%s</title>\n", p->name);
	fprintf(ass, "\t\t\t<version>%s</version>\n", p->vers);
	fprintf(ass, "\t\t</page>\n");
}

static void each_ch (gpointer data, gpointer user_data)
{
	chapter *c = (chapter *)data;
	FILE *ass = (FILE *)user_data;
	
	fprintf(ass, "\t<chapter name=\"%s\" ref=\"%s\">\n", c->name, c->ref);
	g_list_foreach(c->pkg, (GFunc)each_pkg, ass);
	fprintf(ass, "\t</chapter>\n");
}

void write_ass (profile *prof, char *fname)
{
	FILE *ass = stderr;
	
	fprintf(ass, "<?xml version=\"1.0\"?>\n\n<ass>\n");
	fprintf(ass, "\t<title>%s</title>\n\t<version>%s</version>\n\n", 
		prof->name, prof->vers);
	g_list_foreach(prof->ch, (GFunc)each_ch, ass);
	fprintf(ass, "</ass>\n");	
}
