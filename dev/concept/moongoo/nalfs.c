#include <nalfs.h>

profile *prof;

// TODO: Parse all nALFS legacy commands to command* structs
void t_stage2 (xmlNodePtr node, void *data)
{
	printf("%s\n", xmlGetProp(node, "name"));
}

void t_pkg2 (xmlNodePtr node, void *data)
{
	int i, j;

	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg,
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;
	
	prof->ch[i].pkg[j].name = xmlGetProp(node, "name");
	// TODO: <package version="moo"> cannot be parsed yet
	prof->ch[i].pkg[j].vers = NULL;
	prof->ch[i].pkg[j].build = NULL;
	prof->ch[i].pkg[j].n = 0;
	foreach(node->children, "stage", (xml_handler_t)t_stage2, NULL);
}

void t_alfs (xmlNodePtr node, void *data)
{
	// TODO: Handle <stage> node only pages, like chapter07/network.xml 
	foreach(node->children, "package", (xml_handler_t)t_pkg2, NULL);
}

void t_stage (xmlNodePtr node, void *data)
{
	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	prof->ch[prof->n-1].name = xmlGetProp(node, "name");
	prof->ch[prof->n-1].name = prof->ch[prof->n-1].name;
	prof->ch[prof->n-1].pkg = NULL;
	prof->ch[prof->n-1].n = 0;
	foreach(node->children, "alfs", (xml_handler_t)t_alfs, NULL);
}

profile *nalfs_profile (xmlNodePtr node)
{
	node = find_node(node, "alfs");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid nALFS profile.\n");
		return NULL;
	}
	
	prof = (profile *)malloc(sizeof(profile));
	prof->name = "nALFS legacy profile";
	prof->vers = entity_val("LFS-version");
	prof->ch = NULL;
	prof->n = 0;
	foreach(node->children, "stage", (xml_handler_t)t_stage, NULL);
	return prof;
}
