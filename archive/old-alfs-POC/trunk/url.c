#include <string.h>

#include <url.h>

// TODO: Tcl URL is not yet found (tcl8.4.7-src.tar.gz)
// TODO: Vim-lang will not be downloaded atm.
static void t_url (xmlNodePtr cur, void *data)
{
	profile *prof = (profile *)data;
	protocol proto = check_proto(xmlNodeGetContent(cur));
	
	if (proto!=PROTO_NONE)
	{
		bool found = false;
		int i;
		char base[512], url[512], *vurl = "";
		package *pkg = search_pkg(prof, xmlGetProp(cur, "name"), NULL);
				
		if (xmlGetProp(cur, "vurl"))
		{
			char *vers = pkg->vers;
			
			if (!strcmp(xmlGetProp(cur, "vurl"), "base"))
				vers = strcut(vers, 0, strlen(vers)-strlen(strrchr(vers, '.')));
			if (!strcmp(xmlGetProp(cur, "vurl"), "nodots"))
				vers = strkill(vers, ".");
			
			vurl = strdog(vers, "/");
		}

		if (xmlGetProp(cur, "append"))
			vurl = strdog(vurl, xmlGetProp(cur, "append"));
				
		snprintf(base, 512, "%s%s%s-%s.tar", xmlNodeGetContent(cur), vurl,
			lower_case(pkg->name), pkg->vers);

		for (i=0;i<NUM_COMPR;i++)
		{
			snprintf(url, 512, "%s%s", base, compr[i]);
					
			if (proto==HTTP)
			{
				memchunk *mem = http_header(url);
				if (mem->res==HTTP_OK)
				{
					found = true;
					break;
				}
			}
			else
			if (proto==FTP)
			{
				CURLcode res = ftp_list(url);
				if (res==CURLE_OK)
				{
					found = true;
					break;
				}
			}	
			else
			{
				fprintf(stderr, "Unknown protocol in URL '%s'.", url);
				break;
			}
		}

		if (!found)
			printf("%s\n", url);
	}
}

void find_urls (char *fname, profile *prof)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	
	doc = xmlParseFile(fname);
	if (!doc)
		return;
	xmlXIncludeProcessFlags(doc, XML_PARSE_NOENT);
	cur = xmlDocGetRootElement(doc);
	foreach(cur->children, "url", (xml_handler_t)t_url, prof);
}

static bool match_pkg (xmlNodePtr node, void *data)
{
	char *name = (char *)node->name;
	char *prop = xmlGetProp(node, "id");
	if ((!strcmp(name, "sect1")) && (prop) && 
		(!strcmp(prop, "materials-packages")))
			return true;
	return false;
}

static void t_ulink (xmlNodePtr node, void *data)
{
	printf("%s\n", xmlGetProp(node, "url"));
}

void book_urls (xmlNodePtr bnode)
{
	bnode = find_node_match(bnode->children, (xml_match_t)match_pkg, NULL);
	if (bnode)
		foreach(bnode->children, "ulink", (xml_handler_t)t_ulink, NULL);
}
