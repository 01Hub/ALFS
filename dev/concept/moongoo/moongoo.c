#include <stdio.h>
#include <string.h>

#include <alfs.h>
#include <book.h>
#include <repl.h>
#include <util.h>

int main (int argc, char **argv)
{
	int i;
	xmlNodePtr cur;
	profile *prof;

	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		return 1;
	}
	
	doc=xmlParseFile(argv[1]);
	if (!doc)
		return 2;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NONET);
	cur=xmlDocGetRootElement(doc);
	resolve_entities(cur);
	
	prof=bookasprofile(cur);
	for (i=0;i<prof->n;i++)
	{
		package pkg = prof->pkg[i];
		
		//if (!strcmp(pkg.name, "Module-Init-Tools"))
		//if (!strcmp(pkg.name, "Creating a file system"))
		if (!strcmp(pkg.name, "Linux"))
			print_pkg(pkg);
	}
	
	xmlFreeDoc(doc);
	return 0;
}
