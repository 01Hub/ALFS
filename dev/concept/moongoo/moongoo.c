#include <stdio.h>
#include <string.h>

#include <alfs.h>
#include <book.h>
#include <repl.h>
#include <util.h>

int main (int argc, char **argv)
{
	//int i;
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
	print_profile(*prof);
	
	xmlFreeDoc(doc);
	return 0;
}
