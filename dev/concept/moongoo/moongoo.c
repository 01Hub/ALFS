#include <stdio.h>
#include <string.h>

#include <alfs.h>
#include <book.h>
#include <syn.h>
#include <repl.h>
#include <nalfs.h>
#include <ass.h>
#include <util.h>

role default_filter[4] = { NOEXECUTE, INTERACTIVE, TESTSUITE, 0 };

int main (int argc, char **argv)
{
	xmlNodePtr cur;
	profile *prof;

	if (argc<2)
	{
		fprintf(stderr, "No book to parse.\n");
		return 1;
	}
	
	xmlSubstituteEntitiesDefault(1);
	doc=xmlParseFile(argv[1]);
	if (!doc)
		return 2;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NONET);
	cur=xmlDocGetRootElement(doc);
	resolve_entities(cur);
	
	//prof=bookasprofile(cur);
	//prof=syn_profile(cur);
	prof=nalfs_profile(cur);
	//prof=ass_profile(cur);
	
	/*if (prof)
	{
		set_filter(default_filter);
		print_profile(*prof);
	}*/
	
	xmlFreeDoc(doc);
	return 0;
}
