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

void write_ass (profile *prof, char *fname)
{
	FILE *ass = stderr;
	int i, j;
	
	fprintf(ass, "<?xml version=\"1.0\"?>\n\n<ass>\n");
	fprintf(ass, "\t<title>%s</title>\n\t<version>%s</version>\n\n", 
		prof->name, prof->vers);
	for (i=0;i<prof->n;i++)
	{
		fprintf(ass, "\t<chapter name=\"%s\" ref=\"%s\">\n",
			prof->ch[i].name, prof->ch[i].ref);
		for (j=0;j<prof->ch[i].n;j++)
		{
			fprintf(ass, "\t\t<page>\n\t\t\t<title>%s</title>\n", 
				prof->ch[i].pkg[j].name);
			fprintf(ass, "\t\t\t<version>%s</version>\n", 
				prof->ch[i].pkg[j].vers);
			fprintf(ass, "\t\t</page>\n");
		}
		fprintf(ass, "\t</chapter>\n");
	}
	fprintf(ass, "</ass>\n");	
}
